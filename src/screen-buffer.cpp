#include <termic/screen-buffer.h>

#include <fmt/core.h>

#include <assert.h>


namespace termic
{
extern std::FILE *g_log;

void ScreenBuffer::clear(Color bg, Color fg, bool content)
{
	for(auto &row: _rows)
	{
		for(auto &cell: *row)
		{
			if(content)
			{
				cell.ch[0] = '\0';
				cell.width = 1;
			}
			if(fg != color::NoChange)
				cell.fg = fg;
			if(bg != color::NoChange)
				cell.bg = bg;
			cell.style = style::Default;
		}
	}
}

void ScreenBuffer::clear(Rectangle rect, Color bg, Color fg, bool content)
{
	rect.size.width = std::max(1ul, rect.size.width);
	rect.size.height = std::max(1ul, rect.size.height);

	const auto &[width, height] = size();

	auto row_iter = _rows.begin() + int(rect.top_left.y);

	for(auto y = rect.top_left.y; y <= rect.top_left.y + rect.size.height - 1 and y < height; ++y, ++row_iter)
	{
		auto col_iter = (*row_iter)->begin() + int(rect.top_left.x);

		for(auto x = rect.top_left.x; x <= rect.top_left.x + rect.size.width - 1 and x < width; ++x, ++col_iter)
		{
			auto &cell = *col_iter;

			if(content)
			{
				cell.ch[0] = '\0';
				cell.width = 1;
			}
			if(fg != color::NoChange)
				cell.fg = fg;
			if(bg != color::NoChange)
				cell.bg = bg;
			cell.style = style::Default;
		}
	}
}

const Cell &ScreenBuffer::cell(std::size_t x, std::size_t y) const
{
	assert(x < _width and y < _height);

	return _rows[y]->operator[](x);
}

void ScreenBuffer::set_cell(Pos pos, std::string_view ch, std::size_t width, Color fg, Color bg, Style style)
{
	if(pos.x >= _width or pos.y >= _height)
		return;

	auto &cell = _rows[pos.y]->operator[](pos.x);

	if(ch != Cell::NoChange)
		std::strncpy(cell.ch, ch.data(), sizeof(cell.ch));

	cell.width = static_cast<std::uint_fast8_t>(width);

	if(fg != color::NoChange)
		cell.fg = fg;

	if(bg != color::NoChange)
		cell.bg = bg;

	if(style != style::NoChange)
		cell.style = style;
}

ScreenBuffer &ScreenBuffer::operator = (const ScreenBuffer &src)
{
	// since the rows are pointers, we need to copy row by row

	assert(src.size().operator == (size()));

	auto iter = _rows.begin();
	auto src_iter = src._rows.begin();

	while(iter != _rows.end())
		*(*iter++) = *(*src_iter++);

	return *this;
}

void ScreenBuffer::set_size(Size new_size)
{
	const auto &[new_width, new_height] = new_size;

	if(new_width == _width and new_height == _height)
		return;

	const bool initial = _width == 0 and _height == 0;

	if(g_log) fmt::print(g_log, "resize: {}x{} -> {}x{}\n", _width, _height, new_width, new_height);

	_rows.resize(new_height); // if shorter, removed rows will be deallocated by the smart pointer

	if(new_height > _height)
	{
		for(auto idx = _height; idx < new_height; ++idx) // if initial (re)size, all rows
		{
			auto new_row = std::make_unique<CellRow>(new_width);
			//if(g_log) fmt::print(g_log, "resize:   adding row {} ({})\n", idx, new_row->size());
			_rows[idx] = std::move(new_row);
		}
	}

	if(not initial and new_width != _width)  // if initial (re)size, we already did the required work above
	{
		auto row_iter = _rows.begin();
		// if taller: resize only the "old" rows; new rows are sized upon creation, above. if not, all rows
		const auto num_rows = new_height > _height? _height: new_height;
		for(std::size_t row = 0; row < num_rows; ++row)
			(*row_iter++)->resize(new_width);
	}

	_width = new_width;
	_height = new_height;
}


} // NS: termic
