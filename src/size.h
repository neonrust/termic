#pragma once

#include <cstdint>

namespace termic
{

struct Size
{
	std::size_t width;
	std::size_t height;

	inline bool operator == (const Size &t) const
	{
		return t.width == width and t.height == height;
	}
};

struct Pos
{
	std::size_t x;
	std::size_t y;
};


} // NS: termic
