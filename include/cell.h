#pragma once

#include <cstdint>
#include <fmt/core.h>
#include <string_view>

using namespace std::literals::string_view_literals;

extern std::FILE *g_log;

namespace termic
{

constexpr std::size_t max_color_seq_len { 16 };  // e.g. "8;5;r;g;b"
constexpr std::size_t max_style_seq_len { 8 };   // e.g. "1;2;3"

using Style = std::uint_fast8_t;
using Color = std::uint_fast32_t;

namespace color
{

enum
{
	Default =   0x01000000,
	Unchanged = 0x02000000,

	Black  = 0x000000,
	Red    = 0xff0000,
	Green  = 0x00ff00,
	Blue   = 0x0000ff,
	Yellow = 0xffff00,
	Orange = 0xff8800,
	Cyan   = 0x00ffff,
	Purple = 0xcd00e0,
	Pink   = 0xf797f8,
	White  = 0xffffff,
};

static constexpr Color special_mask { 0xff000000 };

inline std::uint8_t red_part(Color c)   { return (c >> 16) & 0xff; };
inline std::uint8_t green_part(Color c) { return (c >>  8) & 0xff; };
inline std::uint8_t blue_part(Color c)  { return  c        & 0xff; };

inline Color rgb(std::initializer_list<std::uint8_t> components)
{
	//static_assert(components.size() == 3)  why doesn't this work?  :(
	Color color { 0 };
	std::uint_fast16_t count = 0;
	for(const auto &c: components)
	{
		color = (color << 8) | c;
		if(++count == 3)
			break;
	}

	return color;
}

inline Color rgb(std::uint8_t r, std::uint8_t g, std::uint8_t b)
{
	return Color(Color(r) << 16 | Color(g) << 8 | Color(b));
}

} // NS: color

namespace style
{
enum Bit
{
	Normal     = 0,
	Default    = Normal,
	Intense    = 1 << 0,   // can't be combined with Faint/Dim
	Bold       = Intense,
	Faint      = 1 << 1,   // can't be combined with Intense/Bold
	Dim        = Faint,
	Italic     = 1 << 2,
	Underline  = 1 << 3,
	Overstrike = 1 << 4,
	// diminishing returns for remaining styles...

	Unchanged  = 0xff,
};

} // NS: style

inline Style operator | (style::Bit a, style::Bit b)
{
	return static_cast<Style>(static_cast<std::uint8_t>(a) | static_cast<std::uint8_t>(b));
}

inline std::string escify(Color c)
{
	if(c == color::Default)
		return "9";

	// TODO: generate 256 or "classic" colors if 24-bit isn't supported
	return fmt::format("8;2;{};{};{}"sv, color::red_part(c), color::green_part(c), color::blue_part(c));
}

inline std::string escify(Style s)
{
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

	if(seq.empty())
		return "0";

	if(seq[seq.size() - 1] == ';')
		seq.resize(seq.size() - 1);

	return seq;
}

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
