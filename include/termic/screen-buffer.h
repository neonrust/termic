#pragma once

#include <vector>
#include <memory>

#include "cell.h"
#include "size.h"


namespace termic
{

struct ScreenBuffer
{
	void set_size(Size size);
	inline Size size() const { return { _width, _height }; };

	inline void clear(bool content=true) { clear(color::Default, color::Default, content); }
	void clear(Color bg, Color fg=color::NoChange, bool content=true);
	void clear(Rectangle rect, Color bg, Color fg=color::NoChange, bool content=true);

	inline Cell &cell(Pos pos)
	{
		return _buffer[pos.y*_width + pos.x];
	}
	inline const Cell &cell(Pos pos) const
	{
		return _buffer[pos.y*_width + pos.x];
	}
	void set_cell(Pos pos, std::string_view ch, std::size_t width, Look lk=look::Default);

	ScreenBuffer &operator = (const ScreenBuffer &that);

	// if true, set_size() attempts to preserve existing content
	bool preserve_content { false };

private:
	std::vector<Cell> _buffer;

	std::size_t _width { 0 };
	std::size_t _height { 0 };
};

} // NS: termic
