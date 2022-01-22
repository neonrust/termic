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

struct Rectangle
{
	Pos top_left;
	Size size;

	inline bool contains(Pos pos) const
	{
		return pos.x >= top_left.x and pos.x < (top_left.x + size.width) and pos.y >= top_left.y and pos.y < (top_left.y + size.height);
	}
};


} // NS: termic
