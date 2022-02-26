#pragma once

#include <termic/look.h>

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
		return look == other.look and width == other.width and std::strncmp(ch, other.ch, sizeof(ch)) == 0;
	}

	char ch[5]   { '\0' };     // a single UTF-8 character, null-terminated  (max utf-8 bytes is 4)
	std::uint_fast8_t width;
	Look look;
};

} // NS: termic
