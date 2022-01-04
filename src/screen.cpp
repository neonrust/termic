#include <termic/screen.h>
#include <termic/utf8.h>

#include <mk-wcwidth.h>

#include <string_view>
#include <algorithm>
#include <chrono>
#include <fmt/format.h>
using namespace fmt::literals;

#include <sys/ioctl.h>

using namespace std::literals;


namespace termic
{
extern std::FILE *g_log;

namespace esc
{

[[maybe_unused]] static constexpr auto esc { "\x1b"sv };
[[maybe_unused]] static constexpr auto csi { "\x1b["sv };

[[maybe_unused]] static constexpr auto cuu { "\x1b[{:d}A"sv };
[[maybe_unused]] static constexpr auto cud { "\x1b[{:d}B"sv };
[[maybe_unused]] static constexpr auto cuf { "\x1b[{:d}C"sv };
[[maybe_unused]] static constexpr auto cub { "\x1b[{:d}D"sv };
[[maybe_unused]] static constexpr auto cup { "\x1b[{1:d};{0:d}H"sv };
[[maybe_unused]] static constexpr auto ed  { "\x1b[{}J"sv }; // erase lines: 0 = before cursor, 1 = after cursor, 2 = entire screen
[[maybe_unused]] static constexpr auto el  { "\x1b[{}K"sv }; // erase line:  0 = before cursor, 1 = after cursor, 2 = entire line

[[maybe_unused]] static constexpr auto fg { "\x1b[3{:s}m"sv };
[[maybe_unused]] static constexpr auto bg { "\x1b[4{:s}m"sv };
[[maybe_unused]] static constexpr auto fg_bg { "\x1b[3{:s};4{:s}m"sv };
[[maybe_unused]] static constexpr auto style { "\x1b[{}m"sv };
[[maybe_unused]] static constexpr auto clear_screen { "\x1b[2J"sv }; // ed[2]


} // NS: esc

static std::string safe(const std::string_view s);


Screen::Screen(int fd) :
	_fd(fd)
{
	_output_buffer.append(fmt::format(esc::cup, 1, 1)); // go to origin (b/c default _cursor.pos = 0,0)
}

std::size_t Screen::print(Alignment align, Pos anchor_pos, std::string_view s, Color fg, Color bg, Style style)
{
	if(align == Left)  // no need to measure the text
		return print(anchor_pos, s, fg, bg, style);

	const auto text_width = measure(s);

	auto pos = anchor_pos;
	if(align == Right)
		pos.x -= text_width - 1;  // anchor is last cell of the text
	else if(align == Center)
		pos.x -= text_width/2;    // anchor is center of the text (truncated)

	return print(pos, s, fg, bg, style);
}

std::size_t Screen::print(Pos pos, std::string_view s, const Color fg, const Color bg, const Style style)
{
	go_to(pos);

	auto size = _back_buffer.size();

	if(pos.y >= size.height)
	{
		if(g_log) fmt::print(g_log, "print: off-screen: y  ({})\n", pos.y);
		return 0;
	}

	auto cx = pos.x;

	//std::u8string u8s;
	//u8s.resize(s.size());
	//::mbrtoc8(u8s.data(), s.c_str(), s.size(), nullptr);

//	auto num_updated { 0u };
	auto total_width { 0ul };

	auto seqiter = utf8::SequenceIterator(s);
	for(auto cpiter = utf8::CodepointIterator(s); cpiter.good();)
	{
		if(cx >= size.width)
		{
			if(g_log) fmt::print(g_log, "print: off-screen: x  ({})\n", cx);
			break;
		}

		//wchar_t wch = ch;
		//const auto width = wch < 0x20? 0: static_cast<std::size_t>(::wcwidth(wch));
		const auto width = static_cast<std::size_t>(std::max(0, ::mk_wcwidth(static_cast<wchar_t>(*cpiter))));

//		if(g_log) fmt::print(g_log, "ch: {} \\u{:04x} width: {}\n", *seqiter, *cpiter, width);

		_back_buffer.set_cell({ cx, pos.y }, *seqiter, width, fg, bg, style);

		// set right-neighbour of double width cell to zero width
		if(width == 2 and cx < size.width - 1)
			_back_buffer.set_cell({ cx + 1, pos.y }, " "sv, 0, fg, bg, style);

//		++num_updated;
		total_width += width;

		cx += static_cast<std::size_t>(width);

		++seqiter;
		++cpiter;
	}

//	if(g_log) fmt::print(g_log, "print: updated cells: {}, width: {}\n", num_updated, total_width);

	_client_cursor.x += total_width;

	return total_width;
}

void Screen::clear(Color bg, Color fg)
{
	_back_buffer.clear(bg, fg);

	cursor_move({ 0, 0 });
}

void Screen::clear(const Rectangle &rect, Color bg, Color fg)
{
	_back_buffer.clear(rect, bg, fg);

	cursor_move({ 0, 0 });
}

void Screen::go_to(Pos pos)
{
	_client_cursor = pos;
}

void Screen::set_size(Size size)
{
	const auto curr_size = _back_buffer.size();

	_output_buffer.reserve(std::max(100ul, size.width)*std::max(100ul, size.height)*4);  // an over-estimate in an attempt to avoid re-allocation

	_back_buffer.set_size(size);
	_front_buffer.set_size(size);

	if(size.width < curr_size.width)
		_front_buffer.clear(color::Default, color::Default, true);
}

void Screen::update()
{
	const auto t0 = std::chrono::high_resolution_clock::now();

	// compare '_back_buffer' and '_front_buffer',
	//   write the difference to the output buffer (such that '_front_buffer' becomes identical to '_back_buffer')

	const auto size = _back_buffer.size();

	const auto start_pos { _cursor.position };

	auto num_updated { 0u };

	for(std::size_t cy = 0; cy < size.height; ++cy)
	{
		for(std::size_t cx = 0; cx < size.width;)
		{
			auto &back_cell = _back_buffer.cell(cx, cy);
			auto &front_cell = _front_buffer.cell(cx, cy);

			if(back_cell != front_cell)
			{
				cursor_move({ cx, cy });
				cursor_color(back_cell.fg, back_cell.bg);
				cursor_style(back_cell.style);

				// if we're at the right edge of the screen and current cell is double width, it's not possible to draw it
				if((back_cell.ch[1] == '\0' and back_cell.ch[0] <= 0x20) or (cx == size.width - 1 and back_cell.width > 1))  // <= 0x20 should actually be "non-printable"
				{
					_output_buffer += ' ';
					++_cursor.position.x;
				}
				else
				{
//					_out(fmt::format("{:c}"sv, char(back_cell.ch))); // TODO: one unicode codepoint
					if(back_cell.ch[0] != '\0')
						_out(back_cell.ch);
					else
						_output_buffer += ' ';
					_cursor.position.x += back_cell.width;
				}

				++num_updated;
			}

			cx += back_cell.width? back_cell.width: 1;
		}
	}

	if(num_updated)
		cursor_move(start_pos);

	// should always flush, even if we didn't output anything in this function
	flush_buffer();

	if(num_updated > 0)
	{
		// the terminal content is now in synch with back buffer, we can copy back -> front
		_front_buffer = _back_buffer;

//		if(g_log) fmt::print(g_log, "updated cells: {}\n", num_updated);
		const auto t1 = std::chrono::high_resolution_clock::now();
		if(g_log) fmt::print(g_log, "screen updated, {} µs  ({} cells)\n", std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count(), num_updated);
	}
}

Size Screen::get_terminal_size()
{
	::winsize size { 0, 0, 0, 0 };

	if(::ioctl(_fd, TIOCGWINSZ, &size) < 0)
		return { 0, 0 };

	return { std::size_t(size.ws_col), std::size_t(size.ws_row) };
}

std::size_t Screen::measure(std::string_view s) const
{
	std::size_t width { 0 };

	for(auto cpiter = utf8::CodepointIterator(s); cpiter.good(); ++cpiter)
		width += static_cast<std::size_t>(std::max(0, ::mk_wcwidth(static_cast<wchar_t>(*cpiter))));

	return width;
}

void Screen::_out(const std::string_view text)
{
	_output_buffer.append(text);
}

Pos Screen::cursor_move(Pos pos)
{
	const Pos prev_pos { _cursor.position };

	if(pos.x != _cursor.position.x or pos.y != _cursor.position.y)
	{
//		if(g_log) fmt::print(g_log, "cursor: {},{}  ->  {},{}\n", _cursor.position.x, _cursor.position.y, pos.x, pos.y);
		_cursor.position = pos;

		pos.x++; // 1-based
		pos.y++;

		if(pos.x != _cursor.position.x and pos.y != _cursor.position.y)
			_out(fmt::format(esc::cup, pos.x, pos.y));

		else if(pos.y == _cursor.position.y)
		{
			if(pos.x > _cursor.position.x)
				_out(fmt::format(esc::cuf, pos.x - _cursor.position.x));
			else
				_out(fmt::format(esc::cub, _cursor.position.x - pos.x));
		}
		else
		{
			if(pos.y > _cursor.position.y)
				_out(fmt::format(esc::cuu, pos.y - _cursor.position.y));
			else
				_out(fmt::format(esc::cud, _cursor.position.y - pos.y));
		}
	}

	return prev_pos;
}

void Screen::cursor_color(Color fg, Color bg)
{
	if(fg != _cursor.fg)
	{
		_out(fmt::format(esc::fg, escify(fg)));
		_cursor.fg = fg;
	}
	if(bg != _cursor.bg)
	{
		_out(fmt::format(esc::bg, escify(bg)));
		_cursor.bg = bg;
	}
}

void Screen::cursor_style(Style style)
{
	if(style != _cursor.style)
	{
		auto curr = [this]  (style::Bit sb) -> bool { return (_cursor.style & sb) > 0; };
		auto to =   [&style](style::Bit sb) -> bool { return (style         & sb) > 0; };

		// TODO: would ideally like to avoid heap allocation here...
		std::string seq;
		seq.reserve(13);

		if(to(style::Bold) and not curr(style::Bold))
			seq += '1';     // set bold
		else if(to(style::Dim) and not curr(style::Dim))
			seq += '2';     // set dim
		else if(not to(style::Bold) and not to(style::Dim) and (curr(style::Bold) or curr(style::Dim)))
			seq += "22"sv;  // clear intensity bit
		if(not seq.empty() and seq[seq.size() - 1] != ';')
			seq += ';';

		if(to(style::Italic) and not curr(style::Italic))
			seq += '3';     // set italic
		if(not to(style::Italic) and curr(style::Italic))
			seq += "23"sv;  // clear italic
		if(not seq.empty() and seq[seq.size() - 1] != ';')
			seq += ';';

		if(to(style::Underline) and not curr(style::Underline))
			seq += '4';     // set underline
		if(not to(style::Underline) and curr(style::Underline))
			seq += "24"sv;  // clear underline
		if(not seq.empty() and seq[seq.size() - 1] != ';')
			seq += ';';

		if(to(style::Overstrike) and not curr(style::Overstrike))
			seq += '9';     // set overstrike
		if(not to(style::Overstrike) and curr(style::Overstrike))
			seq += "29"sv;  // clear overstrike

		// remove final trailing semicolon
		if(not seq.empty() and seq[seq.size() - 1] == ';')
			seq.resize(seq.size() - 1);

		_out(fmt::format(esc::style, seq));

		_cursor.style = style;
	}
}

void Screen::set_cell(Pos pos, std::string_view ch, std::size_t width, Color fg, Color bg, Style style)
{
	_back_buffer.set_cell(pos, ch, width, fg, bg, style);
}

void Screen::flush_buffer()
{
	if(not _output_buffer.empty())
	{
		const auto t0 = std::chrono::high_resolution_clock::now();
		//if(g_log) fmt::print(g_log, "write: {}\n", safe(_output_buffer));
		[[maybe_unused]] auto rc = ::write(_fd, _output_buffer.c_str(), _output_buffer.size());
		_output_buffer.clear();

		const auto t1 = std::chrono::high_resolution_clock::now();
		if(g_log) fmt::print(g_log, "\x1b[2mbuffer flushed, {} µs\x1b[m\n", std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count());
	}
}

[[maybe_unused]] static std::string safe(const std::string_view s)
{
	std::string res;
	for(const auto &c: s)
	{
		if(c == 0x1b)
			res += "\x1b[33;1m\\e\x1b[m";
		else if(c == '\n')
			res += "\\n";
		else if(c == '\r')
			res += "\\r";
		else if(c >= 1 and c <= 26)
			res += fmt::format("\x1b[1m^{:c}\x1b[m", char(c + 'A' - 1));
		else if(c < 0x20)
			res += fmt::format("\x1b[1m\\x{:02x}\x1b[m", (unsigned char)c);
		else
			res += c;
	}
	return res;
}


} // NS: termic
