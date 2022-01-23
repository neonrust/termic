#pragma once

#include <vector>
#include <memory>

#include "cell.h"
#include "size.h"


namespace termic
{

struct ScreenBuffer
{
	friend struct Screen;

	void set_size(Size size);
	inline Size size() const { return { _width, _height }; };

	inline void clear(bool content=true) { clear(color::Default, color::Default, content); }
	void clear(Color bg, Color fg=color::NoChange, bool content=true);
	void clear(Rectangle rect, Color bg, Color fg=color::NoChange, bool content=true);

	const Cell &cell(std::size_t x, std::size_t y) const;
	void set_cell(Pos pos, std::string_view ch, std::size_t width, Look lk=look::Default);

	ScreenBuffer &operator = (const ScreenBuffer &that);

private:
	using CellRow = std::vector<Cell>;
	using CellRowRef = std::unique_ptr<CellRow>;

	std::vector<CellRowRef> _rows;

	std::size_t _width { 0 };
	std::size_t _height { 0 };
};

} // NS: termic
