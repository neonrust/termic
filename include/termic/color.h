#pragma once

#include <cstdint>
#include <initializer_list>
#include <string>
#include <string_view>
#include <fmt/core.h>
#include <fmt/format.h>
using namespace fmt::literals;

using namespace std::literals::string_view_literals;

namespace termic
{

using Color = std::uint_fast32_t;

namespace color
{

enum
{
	Default   = 0x01000000,
	NoChange  = 0x02000000,

	Black     = 0x000000,
	Red       = 0xff0000,
	Green     = 0x00ff00,
	Blue      = 0x0000ff,
	Yellow    = 0xffff00,
	Orange    = 0xff8800,
	Cyan      = 0x00ffff,
	Purple    = 0xcd00e0,
	Pink      = 0xf797f8,
	White     = 0xffffff,
	Grey1     = 0x111111,
	Grey2     = 0x222222,
	Grey3     = 0x333333,
	Grey4     = 0x444444,
	Grey5     = 0x555555,
	Grey6     = 0x666666,
	Grey7     = 0x777777,
	Grey8     = 0x888888,
	Grey9     = 0x999999,
	Grey10    = 0xaaaaaa,
	Grey11    = 0xbbbbbb,
	Grey12    = 0xcccccc,
	Grey13    = 0xdddddd,
	Grey14    = 0xeeeeee,
	Grey      = Grey8,
	DarkGrey  = Grey4,
	LightGrey = Grey12,
};

static constexpr Color special_mask { 0xff000000 };

constexpr std::uint8_t red(Color c)   { return (c >> 16) & 0xff; };
constexpr std::uint8_t green(Color c) { return (c >>  8) & 0xff; };
constexpr std::uint8_t blue(Color c)  { return  c        & 0xff; };

constexpr Color rgb(std::initializer_list<std::uint8_t> components)
{
	Color color { 0 };

	//static_assert(components.size() == 3)  why doesn't this work?  :(
	std::uint_fast16_t count = 0;

	for(const auto &c: components)
	{
		color = (color << 8) | c;
		if(++count == 3)
			break;
	}

	return color;
}

constexpr Color rgb(std::uint8_t r, std::uint8_t g, std::uint8_t b)
{
	return Color(Color(r) << 16 | Color(g) << 8 | Color(b));
}

} // NS: color

inline std::string escify(Color c)
{
	if(c == color::Default)
		return "9";

	// TODO: generate 256 or "classic" colors if 24-bit isn't supported
	return "8;2;{};{};{}"_format(color::red(c), color::green(c), color::blue(c));
}

} // NS: termic
