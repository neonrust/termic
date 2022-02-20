#include <termic/input.h>
#include <termic/utf8.h>

#include <string_view>
#include <unordered_set>
#include <chrono>
#include <nlohmann/json.hpp>
#include <fmt/core.h>
#include <fmt/format.h>
using namespace fmt::literals;
#include <fstream>

#include <signal.h>
#include <poll.h>



using namespace std::literals;

namespace termic
{
extern std::FILE *g_log;


static std::variant<event::Event, int> parse_utf8(std::string_view in, std::size_t &eaten);
static std::vector<std::string_view> split(std::string_view s, std::string_view sep);
static std::string safe(std::string_view s);
static std::string hex(std::string_view s);


static constexpr auto mouse_prefix { "\x1b[<"sv };
static constexpr auto max_mouse_seq_len { 16 }; //  \e[<nn;xxx;yyym -> 14

static constexpr auto focus_in { "\x1b[I"sv };
static constexpr auto focus_out { "\x1b[O"sv };


Input::Input(std::istream &s) : // TODO: use file descriptor instead
    _in(s)
{
    setup_keys();
}

void Input::set_double_click_duration(float duration)
{
    _double_click_duration = std::max(0.01f, duration);
}

std::vector<event::Event> Input::read()
{
	// TODO: use file descriptor instead
	//std::size_t avail { 0 };
	//::ioctl(_in, FIONREAD, &avail);
	if(_in.rdbuf()->in_avail() == 0) // TODO: use file descriptor instead
	{
		// no data yet, wait for data to arrive

		static pollfd pollfds[2] = {
			{
				.fd = STDIN_FILENO,  // TODO: '_in' when it's a file descriptor
				.events = POLLIN,
				.revents = 0,
			},
		};

		const auto timer_enabled = _timer_fd > 0;

		if(timer_enabled)
		{
			// timer is enabled, check that fd as well
			pollfds[1] = {
				.fd = _timer_fd,
				.events = POLLIN,
				.revents = 0,
			};
		}

		sigset_t sigs;
		sigemptyset(&sigs);

		while(true)
		{
			pollfds[0].revents = 0;
			pollfds[1].revents = 0;

			int rc = ::ppoll(pollfds, 1 + (timer_enabled? 1: 0), nullptr, &sigs);
			if(rc == -1 and errno == EINTR)  // something more urgent came up
				return {};

			if(timer_enabled and pollfds[1].revents > 0)
			{
				timer();
				// reset timer event
				static std::uint64_t count { 0 };
				::read(_timer_fd, &count, sizeof(count)); // reads the number of times it has triggered, but we don't care
			}
			else
			{
				if(g_log) fmt::print(g_log, "[0]revents: {} timer_enabled: {} \n", pollfds[0].revents, timer_enabled);
				break; // stdin FD was triggered
			}
		}
	}

	std::string in;
	in.resize(std::size_t(_in.rdbuf()->in_avail()));  // TODO: use file descriptor instead

	_in.read(in.data(), int(in.size()));  // TODO: use file descriptor instead

	auto revert = [this](const std::string_view chars) {
		for(auto iter = chars.rbegin(); iter != chars.rend(); iter++)
			_in.putback(*iter);  // TODO: use file descriptor instead
	};


//	if(g_log) fmt::print(g_log, "input seq: {}\n", safe(in));

	if(in.size() >= 9 and in.starts_with(mouse_prefix))
	{
		std::size_t eaten { 0 };

		std::string_view mouse_seq(in.begin(), in.begin() + max_mouse_seq_len);
		mouse_seq.remove_prefix(mouse_prefix.size());

		auto event = parse_mouse(mouse_seq, eaten);
		if(eaten > 0)
		{
			revert(in.substr(mouse_prefix.size() + eaten));
			return { std::get<event::Event>(event) };
		}
	}

	if(in.size() >= 3)
	{
		if(in.starts_with(focus_in))
		{
			revert(in.substr(focus_in.size()));
			return { event::Focus{ .focused = true } };
		}
		else if(in.starts_with(focus_out))
		{
			revert(in.substr(focus_out.size()));
			return { event::Focus{ .focused = false } };
		}
	}

	for(const auto &kseq: _key_sequences)
	{
		if(in.starts_with(kseq.sequence))
		{
			// put the rest of the read chars
			revert(in.substr(kseq.sequence.size()));

			return { event::Key{
				.key = kseq.key,
				.modifiers = kseq.mods,
			} };
		}
	}


	// TODO: parse utf-8 character
	std::size_t eaten { 0 };
	auto event = parse_utf8(std::string_view(in.begin(), in.end()), eaten);
	if(eaten > 0)
	{
		revert(in.substr(eaten));

		const auto &iev { std::get<event::Input>(std::get<event::Event>(event)) };

		std::vector<event::Event> evs { iev };

		if(iev.codepoint >= 'A' and iev.codepoint <= 'Z')
		{
			evs.push_back(event::Key{
				.key = key::Key(iev.codepoint - 'A' + key::A),
				.modifiers = key::SHIFT,
			});
		}
		else if(iev.codepoint >= 'a' and iev.codepoint <= 'z')
		{
			evs.push_back(event::Key{
				.key = key::Key(iev.codepoint - 'a' + key::A),
			});
		}
		else if(iev.codepoint >= '0' and iev.codepoint <= '9')
		{
			evs.push_back(event::Key{
				.key = key::Key(iev.codepoint - '0' + key::_0),
			});
		}
		else if(iev.codepoint == ' ')
		{
			evs.push_back(event::Key{
				.key = key::SPACE,
			});
		}

		return evs;
	}

	if(g_log) fmt::print(g_log, "\x1b[33;1mparse failed: {}\x1b[m {}  ({})\n", safe(in), hex(in), in.size());
	return {};
}

std::variant<event::Event, int> Input::parse_mouse(std::string_view in, std::size_t &eaten)
{
	// '0;63;16M'  (button | modifiers ; X ; Y ; pressed or motion)
	// '0;63;16m'  (button | modifiers ; X ; Y ; released)

//	if(g_log) fmt::print(g_log, "mouse: {}\n", safe(std::string(in)));

	// read until 'M' or 'm' (max 11 chars; 2 + 1 + 3 + 1 + 3 + 1)
	std::size_t len = 0;
	while(len < in.size())
	{
		char c = in[len++];
		if(c == 'M' or c == 'm')
			break;
	}

	if(len < 6) // shortest possible is 6 chars
		return -1;

	const auto tail = in[len - 1];

	// must end with 'M' or 'm'
	if(tail != 'M' and tail != 'm')
		return -1;

	// skip trailing M/m
	std::string_view seq(in.begin(), in.begin() + len - 1);

	// split by ';'
	const auto parts = split(seq, ";");
	if(parts.size() != 3)
		return -1;

	std::uint64_t buttons_modifiers = std::stoul(parts[0].data());
	const std::size_t mouse_x = std::stoul(parts[1].data()) - 1;
	const std::size_t mouse_y = std::stoul(parts[2].data()) - 1;

	const auto movement = (buttons_modifiers & 0x20) > 0;

	auto button_pressed = not movement and tail == 'M';
	auto button_released = not movement and tail == 'm';

	std::uint8_t mouse_button = 0;
	int mouse_wheel = 0;

	if(not movement)
	{
		// what about mouse buttons 8 - 11 ?
		if(buttons_modifiers >= 128u)
			mouse_button = static_cast<std::uint8_t>((buttons_modifiers & ~0x80u) + 5);
		else if(buttons_modifiers >= 64u)
		{
			mouse_button = static_cast<std::uint8_t>((buttons_modifiers & ~0x40u) + 3);
			mouse_wheel = -(mouse_button - 3)*2 + 1;  // -1 or +1
			// same as wheel?
			button_pressed = button_released = false;
		}
		else
			mouse_button = (buttons_modifiers & 0x0f);
	}

	key::Modifier mods { key::NoMod };
	if((buttons_modifiers & 0x04) > 0)
		mods = key::Modifier(mods | key::SHIFT);
	if((buttons_modifiers & 0x08) > 0)
		mods = key::Modifier(mods | key::ALT);
	if((buttons_modifiers & 0x10) > 0)
		mods = key::Modifier(mods | key::CTRL);

	eaten = len;

	if(movement)
		return event::MouseMove{
			.x = mouse_x,
			.y = mouse_y,
			.modifiers = mods,
		};

	// TODO: detect dragging  (press then move then release)
	if(button_pressed)
	{

		bool pressed { true };
		bool double_clicked { false };

		if(mouse_button == 0)
		{
			// detect double-click (only button 0 at the moment)
			if(_mouse_button_press.elapsed_s() < _double_click_duration)
			{
				pressed = false;
				double_clicked = true;
			}
			else
				_mouse_button_press.reset();
		}

		return event::MouseButton{
			.button = mouse_button,
			.pressed = pressed,
			.double_clicked = double_clicked,
			.x = mouse_x,
			.y = mouse_y,
			.modifiers = mods,
		};
	}

	if(button_released)
		return event::MouseButton{
			.button = mouse_button,
			.released = true,
			.x = mouse_x,
			.y = mouse_y,
			.modifiers = mods,
		};

	if(mouse_wheel != 0)
		return event::MouseWheel{
			.delta = mouse_wheel,
			.x = mouse_x,
			.y = mouse_y,
			.modifiers = mods,
		};


	eaten = 0;

	return -1;
}

static std::variant<event::Event, int> parse_utf8(std::string_view in, std::size_t &eaten)
{
	const auto ch = utf8::read_one(in, &eaten);
	if(eaten == 0)
		return -1;

	return event::Input{
		.codepoint = ch.first,
	};
}

bool Input::setup_keys()
{
	// TODO: shouldn't all this be possible to do in compile-time?  (i.e. constexpr)

	static constexpr auto ALT            { key::ALT };
	static constexpr auto CTRL           { key::CTRL };
	static constexpr auto SHIFT          { key::SHIFT };
	static constexpr auto ALT_CTRL       { key::ALT | key::CTRL };
	static constexpr auto ALT_CTRL_SHIFT { key::ALT | key::CTRL | key::SHIFT };
	static constexpr auto ALT_SHIFT      { key::ALT | key::SHIFT };
	static constexpr auto CTRL_SHIFT     { key::CTRL | key::SHIFT };

	_key_sequences = {
		{ .sequence = "\x7f"          , .key = key::BACKSPACE },
		{ .sequence = "\x00"sv        , .mods = CTRL,     .key = key::SPACE },
		{ .sequence = "\x1b\x00"sv    , .mods = ALT_CTRL, .key = key::SPACE },
		{ .sequence = "\x1b\x1a"sv    , .mods = ALT_CTRL, .key = key::Z },
		{ .sequence = "\x1b\x19"sv    , .mods = ALT_CTRL, .key = key::Y },
		{ .sequence = "\x1b\x18"sv    , .mods = ALT_CTRL, .key = key::X },
		{ .sequence = "\x1b\x17"sv    , .mods = ALT_CTRL, .key = key::W },
		{ .sequence = "\x1b\x16"sv    , .mods = ALT_CTRL, .key = key::V },
		{ .sequence = "\x1b\x15"sv    , .mods = ALT_CTRL, .key = key::U },
		{ .sequence = "\x1b\x14"sv    , .mods = ALT_CTRL, .key = key::T },
		{ .sequence = "\x1b\x13"sv    , .mods = ALT_CTRL, .key = key::S },
		{ .sequence = "\x1b\x12"sv    , .mods = ALT_CTRL, .key = key::R },
		{ .sequence = "\x1b\x11"sv    , .mods = ALT_CTRL, .key = key::Q },
		{ .sequence = "\x1b\x10"sv    , .mods = ALT_CTRL, .key = key::P },
		{ .sequence = "\x1b\x0f"sv    , .mods = ALT_CTRL, .key = key::O },
		{ .sequence = "\x1b\x0e"sv    , .mods = ALT_CTRL, .key = key::N },
		{ .sequence = "\x1b\x0d"sv    , .mods = ALT_CTRL, .key = key::M },
		{ .sequence = "\x1b\x0c"sv    , .mods = ALT_CTRL, .key = key::L },
		{ .sequence = "\x1b\x0b"sv    , .mods = ALT_CTRL, .key = key::K },
		{ .sequence = "\x1b\x0a"sv    , .mods = ALT_CTRL, .key = key::J },
		{ .sequence = "\x1b\x09"sv    , .mods = ALT, .key = key::TAB },  // ALT_CTRL TAB also the same
		{ .sequence = "\x1b\x08"sv    , .mods = ALT_CTRL, .key = key::BACKSPACE },
		{ .sequence = "\x1b\x07"sv    , .mods = ALT_CTRL, .key = key::G },
		{ .sequence = "\x1b\x06"sv    , .mods = ALT_CTRL, .key = key::F },
		{ .sequence = "\x1b\x05"sv    , .mods = ALT_CTRL, .key = key::E },
		{ .sequence = "\x1b\x04"sv    , .mods = ALT_CTRL, .key = key::D },
		{ .sequence = "\x1b\x03"sv    , .mods = ALT_CTRL, .key = key::C },
		{ .sequence = "\x1b\x02"sv    , .mods = ALT_CTRL, .key = key::B },
		{ .sequence = "\x1b\x01"sv    , .mods = ALT_CTRL, .key = key::A },
		{ .sequence = "\x1bz"sv       , .mods = ALT, .key = key::Z },
		{ .sequence = "\x1by"sv       , .mods = ALT, .key = key::Y },
		{ .sequence = "\x1bx"sv       , .mods = ALT, .key = key::X },
		{ .sequence = "\x1bw"sv       , .mods = ALT, .key = key::W },
		{ .sequence = "\x1bv"sv       , .mods = ALT, .key = key::V },
		{ .sequence = "\x1bu"sv       , .mods = ALT, .key = key::U },
		{ .sequence = "\x1bt"sv       , .mods = ALT, .key = key::T },
		{ .sequence = "\x1bs"sv       , .mods = ALT, .key = key::S },
		{ .sequence = "\x1br"sv       , .mods = ALT, .key = key::R },
		{ .sequence = "\x1bq"sv       , .mods = ALT, .key = key::Q },
		{ .sequence = "\x1bp"sv       , .mods = ALT, .key = key::P },
		{ .sequence = "\x1bo"sv       , .mods = ALT, .key = key::O },
		{ .sequence = "\x1bn"sv       , .mods = ALT, .key = key::N },
		{ .sequence = "\x1bm"sv       , .mods = ALT, .key = key::M },
		{ .sequence = "\x1bl"sv       , .mods = ALT, .key = key::L },
		{ .sequence = "\x1bk"sv       , .mods = ALT, .key = key::K },
		{ .sequence = "\x1bj"sv       , .mods = ALT, .key = key::J },
		{ .sequence = "\x1bi"sv       , .mods = ALT, .key = key::I },
		{ .sequence = "\x1bh"sv       , .mods = ALT, .key = key::H },
		{ .sequence = "\x1bg"sv       , .mods = ALT, .key = key::G },
		{ .sequence = "\033f"sv       , .mods = ALT, .key = key::F },
		{ .sequence = "\033e"sv       , .mods = ALT, .key = key::E },
		{ .sequence = "\033d"sv       , .mods = ALT, .key = key::D },
		{ .sequence = "\033c"sv       , .mods = ALT, .key = key::C },
		{ .sequence = "\033b"sv       , .mods = ALT, .key = key::B },
		{ .sequence = "\033a"sv       , .mods = ALT, .key = key::A },
		{ .sequence = "\x1b[H"sv      , .key = key::HOME },
		{ .sequence = "\x1b[F"sv      , .key = key::END },
		{ .sequence = "\x1b[D"sv      , .key = key::LEFT },
		{ .sequence = "\x1b[C"sv      , .key = key::RIGHT },
		{ .sequence = "\x1b[B"sv      , .key = key::DOWN },
		{ .sequence = "\x1b[A"sv      , .key = key::UP },
		{ .sequence = "\x1b[6~"sv     , .key = key::PAGE_DOWN },
		{ .sequence = "\x1b[E"sv      , .key = key::NUMPAD_CENTER },
		{ .sequence = "\x1b[Z"sv      , .mods = SHIFT, .key = key::TAB },
		{ .sequence = "\x1b[1;3E"sv   , .mods = CTRL, .key = key::NUMPAD_CENTER },
		{ .sequence = "\x1b[1;7E"sv   , .mods = ALT_CTRL, .key = key::NUMPAD_CENTER },
		{ .sequence = "\x1b[6;7~"sv	  , .mods = ALT_CTRL, .key = key::PAGE_DOWN },
		{ .sequence = "\x1b[6;5~"sv	  , .mods = CTRL, .key = key::PAGE_DOWN },
		{ .sequence = "\x1b[6;3~"sv	  , .mods = ALT, .key = key::PAGE_DOWN },
		{ .sequence = "\x1b[5~"sv     , .key = key::PAGE_UP },
		{ .sequence = "\x1b[5;7~"sv	  , .mods = ALT_CTRL, .key = key::PAGE_UP },
		{ .sequence = "\x1b[5;5~"sv	  , .mods = CTRL, .key = key::PAGE_UP },
		{ .sequence = "\x1b[5;3~"sv	  , .mods = ALT, .key = key::PAGE_UP },
		{ .sequence = "\x1b[3~"sv     , .key = key::DELETE },
		{ .sequence = "\x1b[3;8~"sv   , .mods = ALT_CTRL_SHIFT, .key = key::DELETE },
		{ .sequence = "\x1b[3;7~"sv   , .mods = ALT_CTRL, .key = key::DELETE },
		{ .sequence = "\x1b[3;5~"sv   , .mods = CTRL, .key = key::DELETE },
		{ .sequence = "\x1b[3;3~"sv	  , .mods = ALT, .key = key::DELETE },
		{ .sequence = "\x1b[2~"sv     , .key = key::INSERT },
		{ .sequence = "\x1b[2;5~"sv   , .mods = CTRL, .key = key::INSERT },
		{ .sequence = "\x1b[2;3~"sv   , .mods = ALT, .key = key::INSERT },
		{ .sequence = "\x1b[20~"sv    , .key = key::F9 },
		{ .sequence = "\x1b[20;2~"sv  , .mods = SHIFT, .key = key::F9 },
		{ .sequence = "\x1b[20;3~"sv  , .mods = ALT, .key = key::F9 },
		{ .sequence = "\x1b[20;4~"sv  , .mods = ALT_SHIFT, .key = key::F9 },
		{ .sequence = "\x1b[20;5~"sv  , .mods = CTRL, .key = key::F9 },
		{ .sequence = "\x1b[20;6~"sv  , .mods = CTRL_SHIFT, .key = key::F9 },
		{ .sequence = "\x1b[20;7~"sv  , .mods = ALT_CTRL, .key = key::F9 },
		{ .sequence = "\x1b[20;8~"sv  , .mods = ALT_CTRL_SHIFT, .key = key::F9 },
		{ .sequence = "\x1b[21~"sv    , .key = key::F10 },
		{ .sequence = "\x1b[21;2~"sv  , .mods = SHIFT, .key = key::F10 },
		{ .sequence = "\x1b[21;3~"sv  , .mods = ALT, .key = key::F10 },
		{ .sequence = "\x1b[21;4~"sv  , .mods = ALT_SHIFT, .key = key::F10 },
		{ .sequence = "\x1b[21;5~"sv  , .mods = CTRL, .key = key::F10 },
		{ .sequence = "\x1b[21;6~"sv  , .mods = CTRL_SHIFT, .key = key::F10 },
		{ .sequence = "\x1b[21;7~"sv  , .mods = ALT_CTRL, .key = key::F10 },
		{ .sequence = "\x1b[21;8~"sv  , .mods = ALT_CTRL_SHIFT, .key = key::F10 },
		{ .sequence = "\x1b[23~"sv    , .key = key::F11 },
		{ .sequence = "\x1b[23;2~"sv  , .mods = SHIFT, .key = key::F11 },
		{ .sequence = "\x1b[23;3~"sv  , .mods = ALT, .key = key::F11 },
		{ .sequence = "\x1b[23;4~"sv  , .mods = ALT_SHIFT, .key = key::F11 },
		{ .sequence = "\x1b[23;5~"sv  , .mods = CTRL, .key = key::F11 },
		{ .sequence = "\x1b[23;6~"sv  , .mods = CTRL_SHIFT, .key = key::F11 },
		{ .sequence = "\x1b[23;7~"sv  , .mods = ALT_CTRL, .key = key::F11 },
		{ .sequence = "\x1b[23;8~"sv  , .mods = ALT_CTRL_SHIFT, .key = key::F11 },
		{ .sequence = "\x1b[24~"sv    , .key = key::F12 },
		{ .sequence = "\x1b[24;2~"sv  , .mods = SHIFT, .key = key::F12 },
		{ .sequence = "\x1b[24;3~"sv  , .mods = ALT, .key = key::F12 },
		{ .sequence = "\x1b[24;4~"sv  , .mods = ALT_SHIFT, .key = key::F12 },
		{ .sequence = "\x1b[24;5~"sv  , .mods = CTRL, .key = key::F12 },
		{ .sequence = "\x1b[24;6~"sv  , .mods = CTRL_SHIFT, .key = key::F12 },
		{ .sequence = "\x1b[24;7~"sv  , .mods = ALT_CTRL, .key = key::F12 },
		{ .sequence = "\x1b[24;8~"sv  , .mods = ALT_CTRL_SHIFT, .key = key::F11 },
		{ .sequence = "\x1b[1;8D"sv	  , .mods = ALT_CTRL_SHIFT, .key = key::LEFT },
		{ .sequence = "\x1b[1;8C"sv	  , .mods = ALT_CTRL_SHIFT, .key = key::RIGHT },
		{ .sequence = "\x1b[1;8B"sv	  , .mods = ALT_CTRL_SHIFT, .key = key::DOWN },
		{ .sequence = "\x1b[1;8A"sv   , .mods = ALT_CTRL_SHIFT, .key = key::UP },
		{ .sequence = "\x1b[1;7H"sv	  , .mods = ALT_CTRL, .key = key::HOME },
		{ .sequence = "\x1b[1;7F"sv	  , .mods = ALT_CTRL, .key = key::END },
		{ .sequence = "\x1b[1;7D"sv	  , .mods = ALT_CTRL, .key = key::LEFT },
		{ .sequence = "\x1b[1;7C"sv	  , .mods = ALT_CTRL, .key = key::RIGHT },
		{ .sequence = "\x1b[1;7B"sv	  , .mods = ALT_CTRL, .key = key::DOWN },
		{ .sequence = "\x1b[1;7A"sv   , .mods = ALT_CTRL, .key = key::UP },
		{ .sequence = "\x1b[1;6D"sv	  , .mods = ALT_CTRL, .key = key::LEFT },
		{ .sequence = "\x1b[1;6C"sv	  , .mods = ALT_CTRL, .key = key::RIGHT },
		{ .sequence = "\x1b[1;6B"sv	  , .mods = ALT_CTRL, .key = key::DOWN },
		{ .sequence = "\x1b[1;6A"sv   , .mods = ALT_CTRL, .key = key::UP },
		{ .sequence = "\x1b[1;5H"sv	  , .mods = CTRL, .key = key::HOME },
		{ .sequence = "\x1b[1;5F"sv	  , .mods = CTRL, .key = key::END },
		{ .sequence = "\x1b[1;5D"sv	  , .mods = CTRL, .key = key::LEFT },
		{ .sequence = "\x1b[1;5C"sv	  , .mods = CTRL, .key = key::RIGHT },
		{ .sequence = "\x1b[1;5B"sv	  , .mods = CTRL, .key = key::DOWN },
		{ .sequence = "\x1b[1;5A"sv   , .mods = CTRL, .key = key::UP },
		{ .sequence = "\x1b[1;2D"sv	  , .mods = SHIFT, .key = key::LEFT },
		{ .sequence = "\x1b[1;2C"sv	  , .mods = SHIFT, .key = key::RIGHT },
		{ .sequence = "\x1b[1;2B"sv	  , .mods = SHIFT, .key = key::DOWN },
		{ .sequence = "\x1b[1;2A"sv   , .mods = SHIFT, .key = key::UP },
		{ .sequence = "\x1b[1;2P"sv   , .mods = SHIFT, .key = key::F1 },
		{ .sequence = "\x1b[1;3P"sv   , .mods = ALT, .key = key::F1 },
		{ .sequence = "\x1b[1;4P"sv   , .mods = ALT_SHIFT, .key = key::F1 },
		{ .sequence = "\x1b[1;5P"sv   , .mods = CTRL, .key = key::F1 },
		{ .sequence = "\x1b[1;6P"sv   , .mods = CTRL_SHIFT, .key = key::F1 },
		{ .sequence = "\x1b[1;7P"sv   , .mods = ALT_CTRL, .key = key::F1 },
		{ .sequence = "\x1b[1;8P"sv   , .mods = ALT_CTRL_SHIFT, .key = key::F1 },
		{ .sequence = "\x1b[1;2Q"sv	  , .mods = SHIFT, .key = key::F2 },
		{ .sequence = "\x1b[1;3Q"sv	  , .mods = ALT, .key = key::F2 },
		{ .sequence = "\x1b[1;4Q"sv	  , .mods = ALT_SHIFT, .key = key::F2 },
		{ .sequence = "\x1b[1;5Q"sv	  , .mods = CTRL, .key = key::F2 },
		{ .sequence = "\x1b[1;6Q"sv	  , .mods = CTRL_SHIFT, .key = key::F2 },
		{ .sequence = "\x1b[1;7Q"sv	  , .mods = ALT_CTRL, .key = key::F2 },
		{ .sequence = "\x1b[1;8Q"sv	  , .mods = ALT_CTRL_SHIFT, .key = key::F2 },
		{ .sequence = "\x1b[1;2R"sv	  , .mods = SHIFT, .key = key::F3 },
		{ .sequence = "\x1b[1;3R"sv	  , .mods = ALT, .key = key::F3 },
		{ .sequence = "\x1b[1;4R"sv	  , .mods = ALT_SHIFT, .key = key::F3 },
		{ .sequence = "\x1b[1;5R"sv	  , .mods = CTRL, .key = key::F3 },
		{ .sequence = "\x1b[1;6R"sv	  , .mods = CTRL_SHIFT, .key = key::F3 },
		{ .sequence = "\x1b[1;7R"sv	  , .mods = ALT_CTRL, .key = key::F3 },
		{ .sequence = "\x1b[1;8R"sv	  , .mods = ALT_CTRL_SHIFT, .key = key::F3 },
		{ .sequence = "\x1b[1;2S"sv	  , .mods = SHIFT, .key = key::F4 },
		{ .sequence = "\x1b[1;3S"sv	  , .mods = ALT, .key = key::F4 },
		{ .sequence = "\x1b[1;4S"sv	  , .mods = ALT_SHIFT, .key = key::F4 },
		{ .sequence = "\x1b[1;5S"sv	  , .mods = CTRL, .key = key::F4 },
		{ .sequence = "\x1b[1;6S"sv	  , .mods = CTRL_SHIFT, .key = key::F4 },
		{ .sequence = "\x1b[1;7S"sv	  , .mods = ALT_CTRL, .key = key::F4 },
		{ .sequence = "\x1b[1;8S"sv	  , .mods = ALT_CTRL_SHIFT, .key = key::F4 },
		{ .sequence = "\x1b[1;3H"sv	  , .mods = ALT, .key = key::HOME },
		{ .sequence = "\x1b[1;3F"sv	  , .mods = ALT, .key = key::END },
		{ .sequence = "\x1b[1;3D"sv   , .mods = ALT, .key = key::LEFT },
		{ .sequence = "\x1b[1;3C"sv   , .mods = ALT, .key = key::RIGHT },
		{ .sequence = "\x1b[1;3B"sv   , .mods = ALT, .key = key::DOWN },
		{ .sequence = "\x1b[1;3A"sv   , .mods = ALT, .key = key::UP },
		{ .sequence = "\x1b[15~"sv    , .key = key::F5 },
		{ .sequence = "\x1b[15;2~"sv  , .mods = SHIFT, .key = key::F5 },
		{ .sequence = "\x1b[15;3~"sv  , .mods = ALT, .key = key::F5 },
		{ .sequence = "\x1b[15;4~"sv  , .mods = ALT_SHIFT, .key = key::F5 },
		{ .sequence = "\x1b[15;5~"sv  , .mods = CTRL, .key = key::F5 },
		{ .sequence = "\x1b[15;6~"sv  , .mods = CTRL_SHIFT, .key = key::F5 },
		{ .sequence = "\x1b[15;7~"sv  , .mods = ALT_CTRL, .key = key::F5 },
		{ .sequence = "\x1b[15;8~"sv  , .mods = ALT_CTRL_SHIFT, .key = key::F5 },
		{ .sequence = "\x1b[17~"sv    , .key = key::F6 },
		{ .sequence = "\x1b[17;2~"sv  , .mods = SHIFT, .key = key::F6 },
		{ .sequence = "\x1b[17;3~"sv  , .mods = ALT, .key = key::F6 },
		{ .sequence = "\x1b[17;4~"sv  , .mods = ALT_SHIFT, .key = key::F6 },
		{ .sequence = "\x1b[17;5~"sv  , .mods = CTRL, .key = key::F6 },
		{ .sequence = "\x1b[17;6~"sv  , .mods = CTRL_SHIFT, .key = key::F6 },
		{ .sequence = "\x1b[17;7~"sv  , .mods = ALT_CTRL, .key = key::F6 },
		{ .sequence = "\x1b[17;8~"sv  , .mods = ALT_CTRL_SHIFT, .key = key::F6 },
		{ .sequence = "\x1b[18~"sv    , .key = key::F7 },
		{ .sequence = "\x1b[18;2~"sv  , .mods = SHIFT, .key = key::F7 },
		{ .sequence = "\x1b[18;3~"sv  , .mods = ALT, .key = key::F7 },
		{ .sequence = "\x1b[18;4~"sv  , .mods = ALT_SHIFT, .key = key::F7 },
		{ .sequence = "\x1b[18;5~"sv  , .mods = CTRL, .key = key::F7 },
		{ .sequence = "\x1b[18;6~"sv  , .mods = CTRL_SHIFT, .key = key::F7 },
		{ .sequence = "\x1b[18;7~"sv  , .mods = ALT_CTRL, .key = key::F7 },
		{ .sequence = "\x1b[18;8~"sv  , .mods = ALT_CTRL_SHIFT, .key = key::F7 },
		{ .sequence = "\x1b[19~"sv    , .key = key::F8 },
		{ .sequence = "\x1b[19;2~"sv  , .mods = SHIFT, .key = key::F8 },
		{ .sequence = "\x1b[19;3~"sv  , .mods = ALT, .key = key::F8 },
		{ .sequence = "\x1b[19;4~"sv  , .mods = ALT_SHIFT, .key = key::F8 },
		{ .sequence = "\x1b[19;5~"sv  , .mods = CTRL, .key = key::F8 },
		{ .sequence = "\x1b[19;6~"sv  , .mods = CTRL_SHIFT, .key = key::F8 },
		{ .sequence = "\x1b[19;7~"sv  , .mods = ALT_CTRL, .key = key::F8 },
		{ .sequence = "\x1b[19;8~"sv  , .mods = ALT_CTRL_SHIFT, .key = key::F8 },
		{ .sequence = "\x1bZ"sv       , .mods = ALT_SHIFT, .key = key::Z }, // or SHIFT TAB
		{ .sequence = "\x1bY"sv       , .mods = ALT_SHIFT, .key = key::Y },
		{ .sequence = "\x1bX"sv       , .mods = ALT_SHIFT, .key = key::X },
		{ .sequence = "\x1bW"sv       , .mods = ALT_SHIFT, .key = key::W },
		{ .sequence = "\x1bV"sv       , .mods = ALT_SHIFT, .key = key::V },
		{ .sequence = "\x1bU"sv       , .mods = ALT_SHIFT, .key = key::U },
		{ .sequence = "\x1bT"sv       , .mods = ALT_SHIFT, .key = key::T },
		{ .sequence = "\x1bS"sv       , .mods = ALT_SHIFT, .key = key::S },
		{ .sequence = "\x1bR"sv       , .mods = ALT_SHIFT, .key = key::R },
		{ .sequence = "\x1bQ"sv       , .mods = ALT_SHIFT, .key = key::Q },
		{ .sequence = "\x1bP"sv       , .mods = ALT_SHIFT, .key = key::P },
		{ .sequence = "\x1bOS"sv      , .key = key::F4 },
		{ .sequence = "\x1bOR"sv      , .key = key::F3 },
		{ .sequence = "\x1bOQ"sv      , .key = key::F2 },
		{ .sequence = "\x1bOP"sv      , .key = key::F1 },
		{ .sequence = "\033A"sv       , .mods = ALT_SHIFT, .key = key::A },
		{ .sequence = "\033B"sv       , .mods = ALT_SHIFT, .key = key::B },
		{ .sequence = "\033C"sv       , .mods = ALT_SHIFT, .key = key::C },
		{ .sequence = "\033D"sv       , .mods = ALT_SHIFT, .key = key::D },
		{ .sequence = "\033E"sv       , .mods = ALT_SHIFT, .key = key::E },
		{ .sequence = "\033F"sv       , .mods = ALT_SHIFT, .key = key::F },
		{ .sequence = "\x1bG"sv       , .mods = ALT_SHIFT, .key = key::G },
		{ .sequence = "\x1bH"sv       , .mods = ALT_SHIFT, .key = key::H },
		{ .sequence = "\x1bI"sv       , .mods = ALT_SHIFT, .key = key::I },
		{ .sequence = "\x1bJ"sv       , .mods = ALT_SHIFT, .key = key::J },
		{ .sequence = "\x1bK"sv       , .mods = ALT_SHIFT, .key = key::K },
		{ .sequence = "\x1bL"sv       , .mods = ALT_SHIFT, .key = key::L },
		{ .sequence = "\x1bM"sv       , .mods = ALT_SHIFT, .key = key::M },
		{ .sequence = "\x1bN"sv       , .mods = ALT_SHIFT, .key = key::N },
		{ .sequence = "\x1bO"sv       , .mods = ALT_SHIFT, .key = key::O },
		{ .sequence = "\x1b"sv        , .key = key::ESCAPE },
		{ .sequence = "\x1b\x1b"sv    , .mods = ALT,  .key = key::ESCAPE },
		{ .sequence = "\x01"sv        , .mods = CTRL, .key = key::A },
		{ .sequence = "\x02"sv        , .mods = CTRL, .key = key::B },
		{ .sequence = "\x03"sv        , .mods = CTRL, .key = key::C },
		{ .sequence = "\x04"sv        , .mods = CTRL, .key = key::D },
		{ .sequence = "\x05"sv        , .mods = CTRL, .key = key::E },
		{ .sequence = "\x06"sv        , .mods = CTRL, .key = key::F },
		{ .sequence = "\x07"sv        , .mods = CTRL, .key = key::G },
		{ .sequence = "\x08"sv        , .mods = CTRL, .key = key::BACKSPACE },
		{ .sequence = "\x09"sv        , .key = key::TAB },
		{ .sequence = "\x0a"sv        , .key = key::ENTER },
		{ .sequence = "\x0b"sv        , .mods = CTRL, .key = key::K },
		{ .sequence = "\x0c"sv        , .mods = CTRL, .key = key::L },
		{ .sequence = "\x0d"sv        , .key = key::ENTER },
		{ .sequence = "\x0e"sv        , .mods = CTRL, .key = key::N },
		{ .sequence = "\x0f"sv        , .mods = CTRL, .key = key::O },
		{ .sequence = "\x10"sv        , .mods = CTRL, .key = key::P },
		{ .sequence = "\x11"sv        , .mods = CTRL, .key = key::Q },
		{ .sequence = "\x12"sv        , .mods = CTRL, .key = key::R },
		{ .sequence = "\x13"sv        , .mods = CTRL, .key = key::S },
		{ .sequence = "\x14"sv        , .mods = CTRL, .key = key::T },
		{ .sequence = "\x15"sv        , .mods = CTRL, .key = key::U },
		{ .sequence = "\x16"sv        , .mods = CTRL, .key = key::V },
		{ .sequence = "\x17"sv        , .mods = CTRL, .key = key::W },
		{ .sequence = "\x18"sv        , .mods = CTRL, .key = key::X },
		{ .sequence = "\x19"sv        , .mods = CTRL, .key = key::Y },
		{ .sequence = "\x1a"sv        , .mods = CTRL, .key = key::Z },
	};

	std::unordered_map<std::string, std::pair<std::size_t, KeySequence>> seen_sequences;

	std::size_t idx { 0 };
	for(const auto &ks: _key_sequences)
	{
		auto found = seen_sequences.find(std::string(ks.sequence));
		if(found != seen_sequences.end())
		{
			const auto &other = found->second.second;
			fmt::print(stderr, "Key sequence '{}' has multiple mappings:\n", safe(ks.sequence));
			fmt::print(stderr, "  index {}: {}\n", found->second.first, key::to_string(other.key, other.mods));
			fmt::print(stderr, "  index {}: {}\n", idx, key::to_string(ks.key, ks.mods));
			return false;
		}
		seen_sequences[std::string(ks.sequence)] = std::make_pair(idx, ks);
		idx++;
	}

	// sort, longest sequence first
	// TODO:  better to sort alphabetically?
	//   when searching, we can then stop if 'in' is past (alphabetically) than the 'sequence'
	std::sort(_key_sequences.begin(), _key_sequences.end(), [](const auto &A, const auto &B) {
		return A.sequence.size() > B.sequence.size();
	});

	return true;
}


static std::string hex(std::string_view s)
{
	std::string res;
	for(const auto &c: s)
		res += "\\x{:02x}"_format((unsigned char)c);
	return res;
}

static std::string safe(std::string_view s)
{
	std::string res;
	for(const auto &c: s)
	{
		if(c == 0x1b)
			res += "\\e";
		else if(c == '\n')
			res += "\\n";
		else if(c == '\r')
			res += "\\r";
		else if(c >= 1 and c <= 26)
			res += "^{:c}"_format(char(c + 'A' - 1));
		else if(c < 0x20)
			res += "\\x{:02x}"_format((unsigned char)c);
		else
			res += c;
	}
	return res;
}

static std::vector<std::string_view> split(std::string_view s, std::string_view sep)
{
	std::vector<std::string_view> parts;

	std::size_t start { 0 };
	std::size_t end { s.find(sep) };

	while(end != std::string::npos)
	{
		parts.push_back({ s.begin() + int(start), s.begin() + int(end) });
		start = end + sep.size();

		end = s.find(sep, start);
	}
	parts.push_back({ s.begin() + int(start), s.end() });

	return parts;
}


} // NS: termic
