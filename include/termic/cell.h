#pragma once

#include <termic/color.h>
#include <termic/style.h>

#include <cstdint>
#include <fmt/core.h>
#include <string_view>

namespace termic
{

struct Cell
{
	static constexpr std::string_view NoChange {};

	inline bool operator == (const Cell &other) const
	{
		return fg == other.fg and bg == other.bg and style == other.style and std::strncmp(ch, other.ch, sizeof(ch)) == 0;
	}

	char ch[5]   { '\0' };     // a single UTF-8 character, null-terminated  (max utf-8 bytes is 4)
	std::uint_fast8_t width;
	Color fg     { color::Default };
	Color bg     { color::Default };
	Style style  { style::Default };
};


} // NS: termic
