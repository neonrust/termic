#include <termic/input.h>

#include <string_view>
#include <unordered_set>
#include <nlohmann/json.hpp>
#include <fmt/core.h>
#include <fstream>

#include <signal.h>
#include <poll.h>



using namespace std::literals::string_view_literals;

namespace termic
{
extern std::FILE *g_log;


static std::variant<event::Event, int> parse_mouse(const std::string_view in, std::size_t &eaten);
static std::variant<event::Event, int> parse_utf8(const std::string_view in, std::size_t &eaten);
static std::vector<std::string_view> split(const std::string_view s, const std::string_view sep);
static std::string safe(const std::string_view s);
static std::string hex(const std::string_view s);


static constexpr auto mouse_prefix { "\x1b[<"sv };
static constexpr auto max_mouse_seq_len { 16 }; //  \e[<nn;xxx;yyym -> 14

static constexpr auto focus_in { "\x1b[I"sv };
static constexpr auto focus_out { "\x1b[O"sv };


Input::Input(std::istream &s) :
    _in(s)
{
    setup_keys();
}

std::vector<event::Event> Input::read()
{
	if(_in.rdbuf()->in_avail() == 0)
	{
		// no data yet, wait for data to arrive, but allow interruptions
		static pollfd pollfds = {
		    .fd = STDIN_FILENO,
		    .events = POLLIN,
		    .revents = 0,
		};
		sigset_t sigs;
		sigemptyset(&sigs);
		int rc = ::ppoll(&pollfds, 1, nullptr, &sigs);
		if(rc == -1 and errno == EINTR)  // something more urgent came up
			return {};
	}

	std::string in;
	in.resize(std::size_t(_in.rdbuf()->in_avail()));

	_in.read(in.data(), int(in.size()));

	auto revert = [this](const std::string_view chars) {
		for(auto iter = chars.rbegin(); iter != chars.rend(); iter++)
			_in.putback(*iter);
	};


	// TODO: parse cursor position report: "\e[n;mR" (between 6 and 10 characters?)
//	if(in.size() >= 6)
//	{
//		static const auto cpr_ptn = std::regex("^\x1b[(\\d+);(\\d+)R");
//		std::smatch m;
//		if(std::regex_search(in, m, cpr_ptn))
//		{
//			auto cx = m[0];
//			auto cy = m[1];
//		}
//	}


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

		return evs;
	}

	if(g_log) fmt::print(g_log, "\x1b[33;1mparse failed: {}\x1b[m {}  ({})\n", safe(in), hex(in), in.size());
	return {};
}

static std::variant<event::Event, int> parse_mouse(const std::string_view in, std::size_t &eaten)
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

//	if(g_log) fmt::print(g_log, "  mouse seq: {:02x} {} {} {}\n", std::stoi(parts[0].data()), parts[1], parts[2], tail);

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

	if(button_pressed)
		return event::MouseButton{
			.button = mouse_button,
			.pressed = true,
			.x = mouse_x,
			.y = mouse_y,
			.modifiers = mods,
		};

	if(button_released)
		return event::MouseButton{
			.button = mouse_button,
			.pressed = false,
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

// this was ruthlessly stolen from termlib (tkbd.c)
static const std::uint8_t utf8_length[] = {
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // 0x00
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // 0x20
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // 0x40
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // 0x60
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // 0x80
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // 0xa0
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, // 0xc0
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,6,6,1,1  // 0xe0
};
static const std::uint8_t utf8_mask[] = {0x7f, 0x1f, 0x0f, 0x07, 0x03, 0x01};

static std::variant<event::Event, int> parse_utf8(const std::string_view in, std::size_t &eaten)
{
	if(in.empty())
		return -1;

	std::size_t len = utf8_length[(uint8_t)in[0]];
	if (len > in.size())
		return -1;

	const auto mask = utf8_mask[len - 1];
	char32_t codepoint = static_cast<char8_t>(in[0] & mask);

	for(std::size_t idx = 1; idx < len; ++idx)
	{
		codepoint <<= 6;
		codepoint |= static_cast<char32_t>(in[idx] & 0x3f);
	}

	eaten = len;

	return event::Input{
		.codepoint = codepoint,
	};
}

bool Input::setup_keys()
{
	_key_sequences = {
		{ .sequence = "\x7f"          , .key = key::key("BACKSPACE") },
		{ .sequence = "\x1b\x1a"      , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("Z") },
		{ .sequence = "\x1b\x19"      , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("Y") },
		{ .sequence = "\x1b\x18"      , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("X") },
		{ .sequence = "\x1b\x17"      , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("W") },
		{ .sequence = "\x1b\x16"      , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("V") },
		{ .sequence = "\x1b\x15"      , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("U") },
		{ .sequence = "\x1b\x14"      , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("T") },
		{ .sequence = "\x1b\x13"      , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("S") },
		{ .sequence = "\x1b\x12"      , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("R") },
		{ .sequence = "\x1b\x11"      , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("Q") },
		{ .sequence = "\x1b\x10"      , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("P") },
		{ .sequence = "\x1b\x0f"      , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("O") },
		{ .sequence = "\x1b\x0e"      , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("N") },
		{ .sequence = "\x1b\x0d"      , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("M") },
		{ .sequence = "\x1b\x0c"      , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("L") },
		{ .sequence = "\x1b\x0b"      , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("K") },
		{ .sequence = "\x1b\x0a"      , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("J") },
		{ .sequence = "\x1b\x09"      , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("I") },
		{ .sequence = "\x1b\x08"      , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("H") },
		{ .sequence = "\x1b\x07"      , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("G") },
		{ .sequence = "\x1b\x06"      , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("F") },
		{ .sequence = "\x1b\x05"      , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("E") },
		{ .sequence = "\x1b\x04"      , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("D") },
		{ .sequence = "\x1b\x03"      , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("C") },
		{ .sequence = "\x1b\x02"      , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("B") },
		{ .sequence = "\x1b\x01"      , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("A") },
		{ .sequence = "\x1bz"         , .mods = key::modifiers({ "ALT" }), .key = key::key("Z") },
		{ .sequence = "\x1by"         , .mods = key::modifiers({ "ALT" }), .key = key::key("Y") },
		{ .sequence = "\x1bx"         , .mods = key::modifiers({ "ALT" }), .key = key::key("X") },
		{ .sequence = "\x1bw"         , .mods = key::modifiers({ "ALT" }), .key = key::key("W") },
		{ .sequence = "\x1bv"         , .mods = key::modifiers({ "ALT" }), .key = key::key("V") },
		{ .sequence = "\x1bu"         , .mods = key::modifiers({ "ALT" }), .key = key::key("U") },
		{ .sequence = "\x1bt"         , .mods = key::modifiers({ "ALT" }), .key = key::key("T") },
		{ .sequence = "\x1bs"         , .mods = key::modifiers({ "ALT" }), .key = key::key("S") },
		{ .sequence = "\x1br"         , .mods = key::modifiers({ "ALT" }), .key = key::key("R") },
		{ .sequence = "\x1bq"         , .mods = key::modifiers({ "ALT" }), .key = key::key("Q") },
		{ .sequence = "\x1bp"         , .mods = key::modifiers({ "ALT" }), .key = key::key("P") },
		{ .sequence = "\x1bo"         , .mods = key::modifiers({ "ALT" }), .key = key::key("O") },
		{ .sequence = "\x1bn"         , .mods = key::modifiers({ "ALT" }), .key = key::key("N") },
		{ .sequence = "\x1bm"         , .mods = key::modifiers({ "ALT" }), .key = key::key("M") },
		{ .sequence = "\x1bl"         , .mods = key::modifiers({ "ALT" }), .key = key::key("L") },
		{ .sequence = "\x1bk"         , .mods = key::modifiers({ "ALT" }), .key = key::key("K") },
		{ .sequence = "\x1bj"         , .mods = key::modifiers({ "ALT" }), .key = key::key("J") },
		{ .sequence = "\x1bi"         , .mods = key::modifiers({ "ALT" }), .key = key::key("I") },
		{ .sequence = "\x1bh"         , .mods = key::modifiers({ "ALT" }), .key = key::key("H") },
		{ .sequence = "\x1bg"         , .mods = key::modifiers({ "ALT" }), .key = key::key("G") },
		{ .sequence = "\033f"         , .mods = key::modifiers({ "ALT" }), .key = key::key("F") },
		{ .sequence = "\033e"         , .mods = key::modifiers({ "ALT" }), .key = key::key("E") },
		{ .sequence = "\033d"         , .mods = key::modifiers({ "ALT" }), .key = key::key("D") },
		{ .sequence = "\033c"         , .mods = key::modifiers({ "ALT" }), .key = key::key("C") },
		{ .sequence = "\033b"         , .mods = key::modifiers({ "ALT" }), .key = key::key("B") },
		{ .sequence = "\033a"         , .mods = key::modifiers({ "ALT" }), .key = key::key("A") },
		{ .sequence = "\x1b[H"        , .key = key::key("HOME") },
		{ .sequence = "\x1b[F"        , .key = key::key("END") },
		{ .sequence = "\x1b[D"        , .key = key::key("LEFT") },
		{ .sequence = "\x1b[C"        , .key = key::key("RIGHT") },
		{ .sequence = "\x1b[B"        , .key = key::key("DOWN") },
		{ .sequence = "\x1b[A"        , .key = key::key("UP") },
		{ .sequence = "\x1b[6~"       , .key = key::key("PAGE_DOWN") },
		{ .sequence = "\x1b[E"        , .key = key::key("NUMPAD_CENTER") },
		{ .sequence = "\x1b[1;3E"     , .mods = key::modifiers({ "CTRL" }), .key = key::key("NUMPAD_CENTER") },
		{ .sequence = "\x1b[1;7E"     , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("NUMPAD_CENTER") },
		{ .sequence = "\x1b[6;7~"	  , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("PAGE_DOWN") },
		{ .sequence = "\x1b[6;5~"	  , .mods = key::modifiers({ "CTRL" }), .key = key::key("PAGE_DOWN") },
		{ .sequence = "\x1b[6;3~"	  , .mods = key::modifiers({ "ALT" }), .key = key::key("PAGE_DOWN") },
		{ .sequence = "\x1b[5~"       , .key = key::key("PAGE_UP") },
		{ .sequence = "\x1b[5;7~"	  , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("PAGE_UP") },
		{ .sequence = "\x1b[5;5~"	  , .mods = key::modifiers({ "CTRL" }), .key = key::key("PAGE_UP") },
		{ .sequence = "\x1b[5;3~"	  , .mods = key::modifiers({ "ALT" }), .key = key::key("PAGE_UP") },
		{ .sequence = "\x1b[3~"       , .key = key::key("DELETE") },
		{ .sequence = "\x1b[3;8~"     , .mods = key::modifiers({ "ALT", "CTRL", "SHIFT" }), .key = key::key("DELETE") },
		{ .sequence = "\x1b[3;7~"     , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("DELETE") },
		{ .sequence = "\x1b[3;5~"     , .mods = key::modifiers({ "CTRL" }), .key = key::key("DELETE") },
		{ .sequence = "\x1b[3;3~"	  , .mods = key::modifiers({ "ALT" }), .key = key::key("DELETE") },
		{ .sequence = "\x1b[2~"       , .key = key::key("INSERT") },
		{ .sequence = "\x1b[2;5~"     , .mods = key::modifiers({ "CTRL" }), .key = key::key("INSERT") },
		{ .sequence = "\x1b[2;3~"     , .mods = key::modifiers({ "ALT" }), .key = key::key("INSERT") },
		{ .sequence = "\x1b[20~"      , .key = key::key("F9") },
		{ .sequence = "\x1b[20;2~"    , .mods = key::modifiers({ "SHIFT" }), .key = key::key("F9") },
		{ .sequence = "\x1b[20;3~"    , .mods = key::modifiers({ "ALT" }), .key = key::key("F9") },
		{ .sequence = "\x1b[20;4~"    , .mods = key::modifiers({ "ALT", "SHIFT" }), .key = key::key("F9") },
		{ .sequence = "\x1b[20;5~"    , .mods = key::modifiers({ "CTRL" }), .key = key::key("F9") },
		{ .sequence = "\x1b[20;6~"    , .mods = key::modifiers({ "CTRL", "SHIFT" }), .key = key::key("F9") },
		{ .sequence = "\x1b[20;7~"    , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("F9") },
		{ .sequence = "\x1b[20;8~"    , .mods = key::modifiers({ "ALT", "CTRL", "SHIFT" }), .key = key::key("F9") },
		{ .sequence = "\x1b[21~"      , .key = key::key("F10") },
		{ .sequence = "\x1b[21;2~"    , .mods = key::modifiers({ "SHIFT" }), .key = key::key("F10") },
		{ .sequence = "\x1b[21;3~"    , .mods = key::modifiers({ "ALT" }), .key = key::key("F10") },
		{ .sequence = "\x1b[21;4~"    , .mods = key::modifiers({ "ALT", "SHIFT" }), .key = key::key("F10") },
		{ .sequence = "\x1b[21;5~"    , .mods = key::modifiers({ "CTRL" }), .key = key::key("F10") },
		{ .sequence = "\x1b[21;6~"    , .mods = key::modifiers({ "CTRL", "SHIFT" }), .key = key::key("F10") },
		{ .sequence = "\x1b[21;7~"    , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("F10") },
		{ .sequence = "\x1b[21;8~"    , .mods = key::modifiers({ "ALT", "CTRL", "SHIFT" }), .key = key::key("F10") },
		{ .sequence = "\x1b[23~"      , .key = key::key("F11") },
		{ .sequence = "\x1b[23;2~"    , .mods = key::modifiers({ "SHIFT" }), .key = key::key("F11") },
		{ .sequence = "\x1b[23;3~"    , .mods = key::modifiers({ "ALT" }), .key = key::key("F11") },
		{ .sequence = "\x1b[23;4~"    , .mods = key::modifiers({ "ALT", "SHIFT" }), .key = key::key("F11") },
		{ .sequence = "\x1b[23;5~"    , .mods = key::modifiers({ "CTRL" }), .key = key::key("F11") },
		{ .sequence = "\x1b[23;6~"    , .mods = key::modifiers({ "CTRL", "SHIFT" }), .key = key::key("F11") },
		{ .sequence = "\x1b[23;7~"    , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("F11") },
		{ .sequence = "\x1b[23;8~"    , .mods = key::modifiers({ "ALT", "CTRL", "SHIFT" }), .key = key::key("F11") },
		{ .sequence = "\x1b[24~"      , .key = key::key("F12") },
		{ .sequence = "\x1b[24;2~"    , .mods = key::modifiers({ "SHIFT" }), .key = key::key("F12") },
		{ .sequence = "\x1b[24;3~"    , .mods = key::modifiers({ "ALT" }), .key = key::key("F12") },
		{ .sequence = "\x1b[24;4~"    , .mods = key::modifiers({ "ALT", "SHIFT" }), .key = key::key("F12") },
		{ .sequence = "\x1b[24;5~"    , .mods = key::modifiers({ "CTRL" }), .key = key::key("F12") },
		{ .sequence = "\x1b[24;6~"    , .mods = key::modifiers({ "CTRL", "SHIFT" }), .key = key::key("F12") },
		{ .sequence = "\x1b[24;7~"    , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("F12") },
		{ .sequence = "\x1b[24;8~"    , .mods = key::modifiers({ "ALT", "CTRL", "SHIFT" }), .key = key::key("F11") },
		{ .sequence = "\x1b[1;8D"	  , .mods = key::modifiers({ "ALT", "CTRL", "SHIFT" }), .key = key::key("LEFT") },
		{ .sequence = "\x1b[1;8C"	  , .mods = key::modifiers({ "ALT", "CTRL", "SHIFT" }), .key = key::key("RIGHT") },
		{ .sequence = "\x1b[1;8B"	  , .mods = key::modifiers({ "ALT", "CTRL", "SHIFT" }), .key = key::key("DOWN") },
		{ .sequence = "\x1b[1;8A"     , .mods = key::modifiers({ "ALT", "CTRL", "SHIFT" }), .key = key::key("UP") },
		{ .sequence = "\x1b[1;7H"	  , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("HOME") },
		{ .sequence = "\x1b[1;7F"	  , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("END") },
		{ .sequence = "\x1b[1;7D"	  , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("LEFT") },
		{ .sequence = "\x1b[1;7C"	  , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("RIGHT") },
		{ .sequence = "\x1b[1;7B"	  , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("DOWN") },
		{ .sequence = "\x1b[1;7A"     , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("UP") },
		{ .sequence = "\x1b[1;6D"	  , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("LEFT") },
		{ .sequence = "\x1b[1;6C"	  , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("RIGHT") },
		{ .sequence = "\x1b[1;6B"	  , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("DOWN") },
		{ .sequence = "\x1b[1;6A"     , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("UP") },
		{ .sequence = "\x1b[1;5H"	  , .mods = key::modifiers({ "CTRL" }), .key = key::key("HOME") },
		{ .sequence = "\x1b[1;5F"	  , .mods = key::modifiers({ "CTRL" }), .key = key::key("END") },
		{ .sequence = "\x1b[1;5D"	  , .mods = key::modifiers({ "CTRL" }), .key = key::key("LEFT") },
		{ .sequence = "\x1b[1;5C"	  , .mods = key::modifiers({ "CTRL" }), .key = key::key("RIGHT") },
		{ .sequence = "\x1b[1;5B"	  , .mods = key::modifiers({ "CTRL" }), .key = key::key("DOWN") },
		{ .sequence = "\x1b[1;5A"     , .mods = key::modifiers({ "CTRL" }), .key = key::key("UP") },
		{ .sequence = "\x1b[1;2D"	  , .mods = key::modifiers({ "SHIFT" }), .key = key::key("LEFT") },
		{ .sequence = "\x1b[1;2C"	  , .mods = key::modifiers({ "SHIFT" }), .key = key::key("RIGHT") },
		{ .sequence = "\x1b[1;2B"	  , .mods = key::modifiers({ "SHIFT" }), .key = key::key("DOWN") },
		{ .sequence = "\x1b[1;2A"     , .mods = key::modifiers({ "SHIFT" }), .key = key::key("UP") },
		{ .sequence = "\x1b[1;2P"     , .mods = key::modifiers({ "SHIFT" }), .key = key::key("F1") },
		{ .sequence = "\x1b[1;3P"     , .mods = key::modifiers({ "ALT" }), .key = key::key("F1") },
		{ .sequence = "\x1b[1;4P"     , .mods = key::modifiers({ "ALT", "SHIFT" }), .key = key::key("F1") },
		{ .sequence = "\x1b[1;5P"     , .mods = key::modifiers({ "CTRL" }), .key = key::key("F1") },
		{ .sequence = "\x1b[1;6P"     , .mods = key::modifiers({ "CTRL", "SHIFT" }), .key = key::key("F1") },
		{ .sequence = "\x1b[1;7P"     , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("F1") },
		{ .sequence = "\x1b[1;8P"     , .mods = key::modifiers({ "ALT", "CTRL", "SHIFT" }), .key = key::key("F1") },
		{ .sequence = "\x1b[1;2Q"	  , .mods = key::modifiers({ "SHIFT" }), .key = key::key("F2") },
		{ .sequence = "\x1b[1;3Q"	  , .mods = key::modifiers({ "ALT" }), .key = key::key("F2") },
		{ .sequence = "\x1b[1;4Q"	  , .mods = key::modifiers({ "ALT", "SHIFT" }), .key = key::key("F2") },
		{ .sequence = "\x1b[1;5Q"	  , .mods = key::modifiers({ "CTRL" }), .key = key::key("F2") },
		{ .sequence = "\x1b[1;6Q"	  , .mods = key::modifiers({ "CTRL", "SHIFT" }), .key = key::key("F2") },
		{ .sequence = "\x1b[1;7Q"	  , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("F2") },
		{ .sequence = "\x1b[1;8Q"	  , .mods = key::modifiers({ "ALT", "CTRL", "SHIFT" }), .key = key::key("F2") },
		{ .sequence = "\x1b[1;2R"	  , .mods = key::modifiers({ "SHIFT" }), .key = key::key("F3") },
		{ .sequence = "\x1b[1;3R"	  , .mods = key::modifiers({ "ALT" }), .key = key::key("F3") },
		{ .sequence = "\x1b[1;4R"	  , .mods = key::modifiers({ "ALT", "SHIFT" }), .key = key::key("F3") },
		{ .sequence = "\x1b[1;5R"	  , .mods = key::modifiers({ "CTRL" }), .key = key::key("F3") },
		{ .sequence = "\x1b[1;6R"	  , .mods = key::modifiers({ "CTRL", "SHIFT" }), .key = key::key("F3") },
		{ .sequence = "\x1b[1;7R"	  , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("F3") },
		{ .sequence = "\x1b[1;8R"	  , .mods = key::modifiers({ "ALT", "CTRL", "SHIFT" }), .key = key::key("F3") },
		{ .sequence = "\x1b[1;2S"	  , .mods = key::modifiers({ "SHIFT" }), .key = key::key("F4") },
		{ .sequence = "\x1b[1;3S"	  , .mods = key::modifiers({ "ALT" }), .key = key::key("F4") },
		{ .sequence = "\x1b[1;4S"	  , .mods = key::modifiers({ "ALT", "SHIFT" }), .key = key::key("F4") },
		{ .sequence = "\x1b[1;5S"	  , .mods = key::modifiers({ "CTRL" }), .key = key::key("F4") },
		{ .sequence = "\x1b[1;6S"	  , .mods = key::modifiers({ "CTRL", "SHIFT" }), .key = key::key("F4") },
		{ .sequence = "\x1b[1;7S"	  , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("F4") },
		{ .sequence = "\x1b[1;8S"	  , .mods = key::modifiers({ "ALT", "CTRL", "SHIFT" }), .key = key::key("F4") },
		{ .sequence = "\x1b[1;3H"	  , .mods = key::modifiers({ "ALT" }), .key = key::key("HOME") },
		{ .sequence = "\x1b[1;3F"	  , .mods = key::modifiers({ "ALT" }), .key = key::key("END") },
		{ .sequence = "\x1b[1;3D"     , .mods = key::modifiers({ "ALT" }), .key = key::key("LEFT") },
		{ .sequence = "\x1b[1;3C"     , .mods = key::modifiers({ "ALT" }), .key = key::key("RIGHT") },
		{ .sequence = "\x1b[1;3B"     , .mods = key::modifiers({ "ALT" }), .key = key::key("DOWN") },
		{ .sequence = "\x1b[1;3A"     , .mods = key::modifiers({ "ALT" }), .key = key::key("UP") },
		{ .sequence = "\x1b[15~"      , .key = key::key("F5") },
		{ .sequence = "\x1b[15;2~"    , .mods = key::modifiers({ "SHIFT" }), .key = key::key("F5") },
		{ .sequence = "\x1b[15;3~"    , .mods = key::modifiers({ "ALT" }), .key = key::key("F5") },
		{ .sequence = "\x1b[15;4~"    , .mods = key::modifiers({ "ALT", "SHIFT" }), .key = key::key("F5") },
		{ .sequence = "\x1b[15;5~"    , .mods = key::modifiers({ "CTRL" }), .key = key::key("F5") },
		{ .sequence = "\x1b[15;6~"    , .mods = key::modifiers({ "CTRL", "SHIFT" }), .key = key::key("F5") },
		{ .sequence = "\x1b[15;7~"    , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("F5") },
		{ .sequence = "\x1b[15;8~"    , .mods = key::modifiers({ "ALT", "CTRL", "SHIFT" }), .key = key::key("F5") },
		{ .sequence = "\x1b[17~"      , .key = key::key("F6") },
		{ .sequence = "\x1b[17;2~"    , .mods = key::modifiers({ "SHIFT" }), .key = key::key("F6") },
		{ .sequence = "\x1b[17;3~"    , .mods = key::modifiers({ "ALT" }), .key = key::key("F6") },
		{ .sequence = "\x1b[17;4~"    , .mods = key::modifiers({ "ALT", "SHIFT" }), .key = key::key("F6") },
		{ .sequence = "\x1b[17;5~"    , .mods = key::modifiers({ "CTRL" }), .key = key::key("F6") },
		{ .sequence = "\x1b[17;6~"    , .mods = key::modifiers({ "CTRL", "SHIFT" }), .key = key::key("F6") },
		{ .sequence = "\x1b[17;7~"    , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("F6") },
		{ .sequence = "\x1b[17;8~"    , .mods = key::modifiers({ "ALT", "CTRL", "SHIFT" }), .key = key::key("F6") },
		{ .sequence = "\x1b[18~"      , .key = key::key("F7") },
		{ .sequence = "\x1b[18;2~"    , .mods = key::modifiers({ "SHIFT" }), .key = key::key("F7") },
		{ .sequence = "\x1b[18;3~"    , .mods = key::modifiers({ "ALT" }), .key = key::key("F7") },
		{ .sequence = "\x1b[18;4~"    , .mods = key::modifiers({ "ALT", "SHIFT" }), .key = key::key("F7") },
		{ .sequence = "\x1b[18;5~"    , .mods = key::modifiers({ "CTRL" }), .key = key::key("F7") },
		{ .sequence = "\x1b[18;6~"    , .mods = key::modifiers({ "CTRL", "SHIFT" }), .key = key::key("F7") },
		{ .sequence = "\x1b[18;7~"    , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("F7") },
		{ .sequence = "\x1b[18;8~"    , .mods = key::modifiers({ "ALT", "CTRL", "SHIFT" }), .key = key::key("F7") },
		{ .sequence = "\x1b[19~"      , .key = key::key("F8") },
		{ .sequence = "\x1b[19;2~"    , .mods = key::modifiers({ "SHIFT" }), .key = key::key("F8") },
		{ .sequence = "\x1b[19;3~"    , .mods = key::modifiers({ "ALT" }), .key = key::key("F8") },
		{ .sequence = "\x1b[19;4~"    , .mods = key::modifiers({ "ALT", "SHIFT" }), .key = key::key("F8") },
		{ .sequence = "\x1b[19;5~"    , .mods = key::modifiers({ "CTRL" }), .key = key::key("F8") },
		{ .sequence = "\x1b[19;6~"    , .mods = key::modifiers({ "CTRL", "SHIFT" }), .key = key::key("F8") },
		{ .sequence = "\x1b[19;7~"    , .mods = key::modifiers({ "ALT", "CTRL" }), .key = key::key("F8") },
		{ .sequence = "\x1b[19;8~"    , .mods = key::modifiers({ "ALT", "CTRL", "SHIFT" }), .key = key::key("F8") },
		{ .sequence = "\x1bZ"         , .mods = key::modifiers({ "ALT", "SHIFT" }), .key = key::key("Z") },
		{ .sequence = "\x1bY"         , .mods = key::modifiers({ "ALT", "SHIFT" }), .key = key::key("Y") },
		{ .sequence = "\x1bX"         , .mods = key::modifiers({ "ALT", "SHIFT" }), .key = key::key("X") },
		{ .sequence = "\x1bW"         , .mods = key::modifiers({ "ALT", "SHIFT" }), .key = key::key("W") },
		{ .sequence = "\x1bV"         , .mods = key::modifiers({ "ALT", "SHIFT" }), .key = key::key("V") },
		{ .sequence = "\x1bU"         , .mods = key::modifiers({ "ALT", "SHIFT" }), .key = key::key("U") },
		{ .sequence = "\x1bT"         , .mods = key::modifiers({ "ALT", "SHIFT" }), .key = key::key("T") },
		{ .sequence = "\x1bS"         , .mods = key::modifiers({ "ALT", "SHIFT" }), .key = key::key("S") },
		{ .sequence = "\x1bR"         , .mods = key::modifiers({ "ALT", "SHIFT" }), .key = key::key("R") },
		{ .sequence = "\x1bQ"         , .mods = key::modifiers({ "ALT", "SHIFT" }), .key = key::key("Q") },
		{ .sequence = "\x1bP"         , .mods = key::modifiers({ "ALT", "SHIFT" }), .key = key::key("P") },
		{ .sequence = "\x1bOS"        , .key = key::key("F4") },
		{ .sequence = "\x1bOR"        , .key = key::key("F3") },
		{ .sequence = "\x1bOQ"        , .key = key::key("F2") },
		{ .sequence = "\x1bOP"        , .key = key::key("F1") },
		{ .sequence = "\033A"         , .mods = key::modifiers({ "ALT", "SHIFT" }), .key = key::key("A") },
		{ .sequence = "\033B"         , .mods = key::modifiers({ "ALT", "SHIFT" }), .key = key::key("B") },
		{ .sequence = "\033C"         , .mods = key::modifiers({ "ALT", "SHIFT" }), .key = key::key("C") },
		{ .sequence = "\033D"         , .mods = key::modifiers({ "ALT", "SHIFT" }), .key = key::key("D") },
		{ .sequence = "\033E"         , .mods = key::modifiers({ "ALT", "SHIFT" }), .key = key::key("E") },
		{ .sequence = "\033F"         , .mods = key::modifiers({ "ALT", "SHIFT" }), .key = key::key("F") },
		{ .sequence = "\x1bG"         , .mods = key::modifiers({ "ALT", "SHIFT" }), .key = key::key("G") },
		{ .sequence = "\x1bH"         , .mods = key::modifiers({ "ALT", "SHIFT" }), .key = key::key("H") },
		{ .sequence = "\x1bI"         , .mods = key::modifiers({ "ALT", "SHIFT" }), .key = key::key("I") },
		{ .sequence = "\x1bJ"         , .mods = key::modifiers({ "ALT", "SHIFT" }), .key = key::key("J") },
		{ .sequence = "\x1bK"         , .mods = key::modifiers({ "ALT", "SHIFT" }), .key = key::key("K") },
		{ .sequence = "\x1bL"         , .mods = key::modifiers({ "ALT", "SHIFT" }), .key = key::key("L") },
		{ .sequence = "\x1bM"         , .mods = key::modifiers({ "ALT", "SHIFT" }), .key = key::key("M") },
		{ .sequence = "\x1bN"         , .mods = key::modifiers({ "ALT", "SHIFT" }), .key = key::key("N") },
		{ .sequence = "\x1bO"         , .mods = key::modifiers({ "ALT", "SHIFT" }), .key = key::key("O") },
		{ .sequence = "\x1b"          , .key = key::key("ESCAPE") },
		{ .sequence = "\x01"          , .mods = key::modifiers({ "CTRL" }), .key = key::key("A") },
		{ .sequence = "\x02"          , .mods = key::modifiers({ "CTRL" }), .key = key::key("B") },
		{ .sequence = "\x03"          , .mods = key::modifiers({ "CTRL" }), .key = key::key("C") },
		{ .sequence = "\x04"          , .mods = key::modifiers({ "CTRL" }), .key = key::key("D") },
		{ .sequence = "\x05"          , .mods = key::modifiers({ "CTRL" }), .key = key::key("E") },
		{ .sequence = "\x06"          , .mods = key::modifiers({ "CTRL" }), .key = key::key("F") },
		{ .sequence = "\x07"          , .mods = key::modifiers({ "CTRL" }), .key = key::key("G") },
		{ .sequence = "\x08"          , .mods = key::modifiers({ "CTRL" }), .key = key::key("H") },
		{ .sequence = "\x09"          , .mods = key::modifiers({ "CTRL" }), .key = key::key("I") },
		{ .sequence = "\x0a"          , .mods = key::modifiers({ "CTRL" }), .key = key::key("J") },
		{ .sequence = "\x0b"          , .mods = key::modifiers({ "CTRL" }), .key = key::key("K") },
		{ .sequence = "\x0c"          , .mods = key::modifiers({ "CTRL" }), .key = key::key("L") },
		{ .sequence = "\x0d"          , .mods = key::modifiers({ "CTRL" }), .key = key::key("M") },
		{ .sequence = "\x0e"          , .mods = key::modifiers({ "CTRL" }), .key = key::key("N") },
		{ .sequence = "\x0f"          , .mods = key::modifiers({ "CTRL" }), .key = key::key("O") },
		{ .sequence = "\x10"          , .mods = key::modifiers({ "CTRL" }), .key = key::key("P") },
		{ .sequence = "\x11"          , .mods = key::modifiers({ "CTRL" }), .key = key::key("Q") },
		{ .sequence = "\x12"          , .mods = key::modifiers({ "CTRL" }), .key = key::key("R") },
		{ .sequence = "\x13"          , .mods = key::modifiers({ "CTRL" }), .key = key::key("S") },
		{ .sequence = "\x14"          , .mods = key::modifiers({ "CTRL" }), .key = key::key("T") },
		{ .sequence = "\x15"          , .mods = key::modifiers({ "CTRL" }), .key = key::key("U") },
		{ .sequence = "\x16"          , .mods = key::modifiers({ "CTRL" }), .key = key::key("V") },
		{ .sequence = "\x17"          , .mods = key::modifiers({ "CTRL" }), .key = key::key("W") },
		{ .sequence = "\x18"          , .mods = key::modifiers({ "CTRL" }), .key = key::key("X") },
		{ .sequence = "\x19"          , .mods = key::modifiers({ "CTRL" }), .key = key::key("Y") },
		{ .sequence = "\x1a"          , .mods = key::modifiers({ "CTRL" }), .key = key::key("Z") },
	};

	std::unordered_map<std::string, std::pair<std::size_t, KeySequence>> seen_sequences;

	std::size_t idx { 0 };
	for(const auto &ks: _key_sequences)
	{
		auto found = seen_sequences.find(ks.sequence);
		if(found != seen_sequences.end())
		{
			const auto &other = found->second.second;
			fmt::print(stderr, "Key sequence '{}' has multiple mappings:\n", safe(ks.sequence));
			fmt::print(stderr, "  index {}: {}\n", found->second.first, key::to_string(other.key, other.mods));
			fmt::print(stderr, "  index {}: {}\n", idx, key::to_string(ks.key, ks.mods));
			return false;
		}
		seen_sequences[ks.sequence] = std::make_pair(idx, ks);
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


static std::string hex(const std::string_view s)
{
	std::string res;
	for(const auto &c: s)
		res += fmt::format("\\x{:02x}", (unsigned char)c);
	return res;
}

static std::string safe(const std::string_view s)
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
			res += fmt::format("^{:c}", char(c + 'A' - 1));
		else if(c < 0x20)
			res += fmt::format("\\x{:02x}", (unsigned char)c);
		else
			res += c;
	}
	return res;
}

static std::vector<std::string_view> split(const std::string_view s, const std::string_view sep)
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
