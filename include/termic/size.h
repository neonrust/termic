#pragma once

#include <cstdint>
#include <utility>

namespace termic
{

struct Size
{
	std::size_t width;
	std::size_t height;

	inline bool empty() const
	{
		return width == 0 and height == 0;
	}

	inline bool operator == (const Size &t) const
	{
		return t.width == width and t.height == height;
	}

	inline Size cap(Size size) const
	{
		return {
			size.width > 0 and size.width < width? size.width: width,
			size.height > 0 and size.height < height? size.height: height,
		};
	}

	inline std::size_t area() const
	{
		return width*height;
	}
};

struct Pos
{
	std::size_t x;
	std::size_t y;

	inline Pos move(std::pair<int, int> delta) const
	{
		return {
			static_cast<decltype(x)>(static_cast<int>(x) + delta.first),
			static_cast<decltype(y)>(static_cast<int>(y) + delta.second),
		};
	}
};

struct Rectangle
{
	Pos top_left;
	Size size;

	inline bool contains(Pos pos) const
	{
		return pos.x >= top_left.x and pos.x < (top_left.x + size.width) and pos.y >= top_left.y and pos.y < (top_left.y + size.height);
	}

	inline std::size_t area() const
	{
		return size.area();
	}

	inline Rectangle move(std::pair<int, int> delta) const
	{
		return { top_left.move(delta), size };
	}

	inline Rectangle cap(Size size) const
	{
		return { top_left, this->size.cap(size) };
	}

	inline std::size_t bottom() const
	{
		return top_left.y + size.height - 1;
	}

	inline std::size_t right() const
	{
		return top_left.x + size.width - 1;
	}
};


} // NS: termic
