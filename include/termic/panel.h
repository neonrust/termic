#pragma once

#include <termic/cell.h>
#include <termic/size.h>

#include <string>

namespace termic
{

struct Layout;
struct Screen;

struct Panel
{
	void print(Pos pos, const std::string_view s, Color fg=color::Default, Color bg=color::Default, Style style=style::Default);

	inline void print(const std::string_view s, Color fg=color::Default, Color bg=color::Default, Style style=style::Default) {
		print(_cursor, s, fg, bg, style);
	};
	void home();


private:
	Panel(Layout *layout);

private:
	Screen &_screen;
	Layout *_layout { nullptr };
	Pos _cursor { 0, 0 };
};


} // NS: termic
