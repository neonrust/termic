#pragma once

#include <termic/color.h>
#include <termic/style.h>

#include <cstdint>
#include <fmt/core.h>
#include <string_view>

extern std::FILE *g_log;

namespace termic
{

struct Cell
{
	static constexpr wchar_t Unchanged = '\0';

	inline bool operator == (const Cell &other) const
	{
		return ch == other.ch and fg == other.fg and bg == other.bg and style == other.style;
	}

	wchar_t ch   { '\0' };     // a single UTF-8 character
	std::uint_fast8_t width;
	Color fg     { color::Default };
	Color bg     { color::Default };
	Style style  { style::Default };
};


} // NS: termic
