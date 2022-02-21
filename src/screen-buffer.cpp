#include <termic/screen-buffer.h>

#include <fmt/core.h>

#include <assert.h>


namespace termic
{
extern std::FILE *g_log;

void ScreenBuffer::clear(Color bg, Color fg, bool content)
{
	for(auto &cell: _buffer)
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

void ScreenBuffer::clear(Rectangle rect, Color bg, Color fg, bool content)
{
	rect.size.width = std::max(1ul, rect.size.width);
	rect.size.height = std::max(1ul, rect.size.height);

	const auto &[width, height] = size();

	auto row_iter = _buffer.begin() + int(rect.top_left.y * _width);

	for(auto y = rect.top_left.y; y <= rect.top_left.y + rect.size.height - 1 and y < height; ++y, ++row_iter)
	{
		auto col_iter = row_iter + int(rect.top_left.x);

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

Cell &ScreenBuffer::cell(Pos pos)
{
	return _buffer[pos.y*_width + pos.x];
}

void ScreenBuffer::set_cell(Pos pos, std::string_view ch, std::size_t width, Look lk)
{
	if(pos.x >= _width or pos.y >= _height)
		return;

	auto &cell = this->cell(pos);

	if(ch != Cell::NoChange)
	{
		std::strncpy(cell.ch, ch.data(), sizeof(cell.ch) - 1);
		cell.ch[std::min(sizeof(cell.ch), ch.size())] = '\0';
	}

	cell.width = static_cast<std::uint_fast8_t>(width);

	if(lk.fg != color::NoChange)
		cell.fg = lk.fg;

	if(lk.bg != color::NoChange)
		cell.bg = lk.bg;

	if(lk.style != style::NoChange)
		cell.style = lk.style;
}

ScreenBuffer &ScreenBuffer::operator = (const ScreenBuffer &src)
{
	assert(src.size().operator == (size()));

	_buffer = src._buffer;

	return *this;
}

void ScreenBuffer::set_size(Size new_size)
{
	const auto &[new_width, new_height] = new_size;

	if(new_width == _width and new_height == _height)
		return;

	auto resized { false };

	if(preserve_content)
	{
		const bool initial = _width == 0 and _height == 0;

		if(g_log) fmt::print(g_log, "resize: {}x{} -> {}x{}\n", _width, _height, new_width, new_height);


		if(not initial and new_width != _width)
		{
			// width changed (i.e. row stride changed),
			//   we need to copy each row from the original to the new size
			const auto copy = _buffer;

			// resize to 0 and then to new size, to force all cells to default  (is there a better way?)
			_buffer.resize(0);
			_buffer.resize(new_height*new_width);
			resized = true;

			const auto copy_len = int(std::min(_width, new_width));

			auto src_iter = copy.cbegin();
			auto dst_iter = _buffer.begin();
			for (auto y = 0u; y < std::min(new_height, _height); ++y, std::advance(dst_iter, new_width), std::advance(src_iter, _width))
				std::copy(src_iter, src_iter + copy_len, dst_iter);
		}

		// set added rows to defaults
		if(initial or new_height > _height)
		{
			_buffer.resize(new_height*new_width);
			resized = true;

			auto row_iter = _buffer.begin() + int(_height*_width);
			for (auto y = _height; y < new_height; ++y, std::advance(row_iter, _width))
			{
				for(auto iter = row_iter; iter != row_iter + int(_width); ++iter)
				{
					auto &cell = *iter;

					cell.ch[0] = '\0';
					cell.width = 0;
					cell.fg = color::Default;
					cell.bg = color::Default;
					cell.style = style::Default;
				};
			}
		}
	}

	if(not resized)
	{
		// resize to 0 and then to new size, to force all cells to default  (is there a better way?)
		_buffer.resize(0);
		_buffer.resize(new_height*new_width);
	}

	_width = new_width;
	_height = new_height;
}


} // NS: termic
