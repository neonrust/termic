#pragma once

#include <cstdint>

#include <fmt/core.h>
#include <fmt/format.h>
using namespace fmt::literals;


namespace termic
{

using Color = std::uint_fast32_t;

namespace color
{

constexpr static Color Default   = 0x01000000;
constexpr static Color NoChange  = 0x02000000;
constexpr static Color Black     = 0x000000;
constexpr static Color Red       = 0xff0000;
constexpr static Color Green     = 0x00ff00;
constexpr static Color Blue      = 0x0000ff;
constexpr static Color Yellow    = 0xffff00;
constexpr static Color Orange    = 0xff8800;
constexpr static Color Cyan      = 0x00ffff;
constexpr static Color Purple    = 0xcd00e0;
constexpr static Color Pink      = 0xf797f8;
constexpr static Color White     = Color(0xffffff);
constexpr static Color Grey10    = 0x191919;
constexpr static Color Grey20    = 0x323232;
constexpr static Color Grey30    = 0x4c4c4c;
constexpr static Color Grey40    = 0x666666;
constexpr static Color Grey50    = 0x808080;
constexpr static Color Grey60    = 0x999999;
constexpr static Color Grey70    = 0xb3b3b3;
constexpr static Color Grey80    = 0xcccccc;
constexpr static Color Grey90    = 0xe6e6e6;
constexpr static Color Grey      = Grey50;
constexpr static Color DarkGrey  = Grey20;
constexpr static Color LightGrey = Grey80;

static constexpr Color special_mask { 0xff000000 };

constexpr inline std::uint8_t red(Color c)   { return (c >> 16) & 0xff; };
constexpr inline std::uint8_t green(Color c) { return (c >>  8) & 0xff; };
constexpr inline std::uint8_t blue(Color c)  { return  c        & 0xff; };

constexpr inline Color rgb(std::uint8_t r, std::uint8_t g, std::uint8_t b)
{
	return
		static_cast<std::uint32_t>(r) << 16 |
		static_cast<std::uint32_t>(g) << 8 |
		static_cast<std::uint32_t>(b)
	;
}

//constexpr Color rgb(std::uint8_t r, std::uint8_t g, std::uint8_t b)
//{
//	return Color(Color(r) << 16 | Color(g) << 8 | Color(b));
//}

Color lerp(Color a, Color b, float blend);

} // NS: color

inline std::string escify(Color c)
{
	if(c == color::Default)
		return "9";

	// TODO: generate 256 or "classic" colors if 24-bit isn't supported
	return fmt::format("8;2;{};{};{}", color::red(c), color::green(c), color::blue(c));
}


using Style = std::uint16_t;

namespace style
{

constexpr static Style NoChange   { 0xff };
constexpr static Style Normal     { 0 };
constexpr static Style Default    { Normal };
constexpr static Style Intense    { 1 << 0 };   // can't be combined with Faint/Dim
constexpr static Style Bold       { Intense };
constexpr static Style Faint      { 1 << 1 };   // can't be combined with Intense/Bold
constexpr static Style Dim        { Faint };
constexpr static Style Italic     { 1 << 2 };
constexpr static Style Underline  { 1 << 3 };
constexpr static Style Overstrike { 1 << 4 };
constexpr static Style Inverse    { 1 << 5 };
constexpr static Style Reverse    { Inverse };
// diminishing returns for remaining styles...

} // NS: style


inline std::string escify(Style s)
{
	// TODO: avoid heap allocation
	std::string seq;

	if((s & style::Intense) > 0)
		seq += "1;";
	else if((s & style::Faint) > 0)
		seq += "2;";
	if((s & style::Italic) > 0)
		seq += "3;";
	if((s & style::Underline) > 0)
		seq += "4;";
	if((s & style::Overstrike) > 0)
		seq += "9;";
	if((s & style::Inverse) > 0)
		seq += "7;";

	if(seq.empty())
		return "0";

	if(seq[seq.size() - 1] == ';')
		seq.resize(seq.size() - 1);

	return seq;
}



struct Look
{
	inline Look() : fg(color::Default), style(style::Default), bg(color::Default) {}
	inline Look(Color fg, Style style=style::Default, Color bg=color::NoChange) : fg(fg), style(style), bg(bg) {}
	inline Look(Color fg, Color bg, Style style=style::Default) : fg(fg), style(style), bg(bg) {}

	inline bool operator == (const Look &other) const
	{
		return fg == other.fg and style == other.style and bg == other.bg;
	}

	Color fg    { color::Default };
	Style style { style::Default };
	Color bg    { color::NoChange };
};

namespace look
{

static const Look Default { color::Default, style::Default, color::NoChange };

inline Look bg(Color bg) { return { color::NoChange, style::NoChange, bg }; }

} // NS: look

} // NS: termic
