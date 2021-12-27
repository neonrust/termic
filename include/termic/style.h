#pragma once

#include <cstdint>
#include <string>


namespace termic
{

using Style = std::uint_fast8_t;

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


constexpr Style operator | (style::Bit a, style::Bit b)
{
	return static_cast<Style>(static_cast<std::uint8_t>(a) | static_cast<std::uint8_t>(b));
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

} // NS: termic
