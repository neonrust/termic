#pragma once

#include <cstdint>

#include "cell.h"
#include "screen-buffer.h"
#include "size.h"

namespace termic
{

enum Alignment
{
	Left = 0,
	Center,
	Right
};


struct Screen
{
	Screen(int fd);

	inline void clear() { clear(color::Default, color::Default); }
	void clear(Color bg, Color fg=color::NoChange);
	void clear(const Rectangle &rect, Color bg, Color fg=color::NoChange);

	void go_to(Pos pos);
	inline std::size_t print(std::string_view s, Color fg=color::Default, Color bg=color::NoChange, Style style=style::Default)
	{
		return print(_client_cursor, s, fg, bg, style);
	}
	std::size_t print(Alignment align, Pos anchor_pos, std::string_view s, Color fg=color::Default, Color bg=color::NoChange, Style style=style::Default);
	std::size_t print(Pos pos, std::string_view s, Color fg=color::Default, Color bg=color::NoChange, Style style=style::Default);

	void set_cell(Pos pos, std::string_view ch, std::size_t width, Color fg=color::Default, Color bg=color::Default, Style style=style::Default);

	void update();

	void set_size(Size size);
	inline Size size() const { return _back_buffer.size(); }
	Size get_terminal_size();

	std::size_t measure(std::string_view s) const;

private:
	Pos cursor_move(Pos pos);
	void cursor_style(Style style);
	void cursor_color(Color fg, Color bg);

	void _out(const std::string_view text);
	void flush_buffer();

private:
	Pos _client_cursor { 0, 0 };

	int _fd { 0 };
	ScreenBuffer _back_buffer;
	ScreenBuffer _front_buffer;

	struct Cursor
	{
		Pos position { 0, 0 };
		Color fg { color::Default };
		Color bg { color::Default };
		Style style { style::Default };
	} _cursor;

	std::string _output_buffer;
};

} // NS: termic
