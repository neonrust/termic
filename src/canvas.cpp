#include <termic/screen.h>
#include <termic/canvas.h>
#include <termic/samplers.h>
#include <termic/look.h>

#include <fmt/core.h>

namespace termic
{

void Canvas::clear()
{
	_screen.clear();
}

Size Canvas::size() const
{
	return _screen.size();
}

void Canvas::fill(Color c)
{
	fill(_screen.rect(), c);
}

void Canvas::fill(const color::Sampler *s, float sampler_angle)
{
	fill(_screen.rect(), s, sampler_angle);
}

void Canvas::fill(Rectangle rect, Color c)
{
	color::Constant cc(c);
	fill(rect, &cc);
}

void Canvas::fill(Rectangle rect, const color::Sampler *s, float sampler_angle)
{
	rect.size.width = std::max(1ul, rect.size.width);
	rect.size.height = std::max(1ul, rect.size.height);

	// TODO: _scr.iterator(rect) ?

	const auto size = _screen.size();

	for(auto y = rect.top_left.y; y <= rect.top_left.y + rect.size.height - 1 and y < size.height; y++)
	{
		for(auto x = rect.top_left.x; x <= rect.top_left.x + rect.size.width - 1 and x < size.width; x++)
		{
			const float u = static_cast<float>(x - rect.top_left.x + 1) / float(rect.size.width);
			const float v = static_cast<float>(y - rect.top_left.y + 1) / float(rect.size.height);

			_screen.set_cell({ x, y }, Cell::NoChange, 1, look::bg(s->sample({ u, v }, sampler_angle)));
		}
	}
	_screen.invalidate();
}

void Canvas::filter(std::function<void(Look &, UV)> f)
{
	filter(_screen.rect(), f);
}

void Canvas::filter(Rectangle rect, std::function<void (Look &, UV)> f)
{
	rect.size.width = std::max(1ul, rect.size.width);
	rect.size.height = std::max(1ul, rect.size.height);

	// TODO: _scr.iterator(rect) ?

	const auto size = _screen.size();

	for(auto y = rect.top_left.y; y <= rect.top_left.y + rect.size.height - 1 and y < size.height; y++)
	{
		for(auto x = rect.top_left.x; x <= rect.top_left.x + rect.size.width - 1 and x < size.width; x++)
		{
			const float u = static_cast<float>(x - rect.top_left.x + 1) / float(rect.size.width);
			const float v = static_cast<float>(y - rect.top_left.y + 1) / float(rect.size.height);

			auto &cell = _screen.cell({ x, y });
			Look lk { cell.look };

			f(lk, UV{ u, v });

			cell.look = lk;
		}
	}
	_screen.invalidate();
}

void Canvas::fade(float blend)
{
	fade(_screen.rect(), color::Black, color::Black, blend);
}

void Canvas::fade(Color fg, Color bg, float blend)
{
	fade(_screen.rect(), fg, bg, blend);
}

void Canvas::fade(Rectangle rect, float blend)
{
	fade(rect, color::Black, color::Black, blend);
}

void Canvas::fade(Rectangle rect, Color fg, Color bg, float blend)
{
	if(blend == 0 or (fg == color::NoChange and bg == color::NoChange))
		return;

	filter(rect, [=](Look &lk, UV) {
		if(fg != color::NoChange)
			lk.fg = color::lerp(lk.fg, fg, blend);
		if(bg != color::NoChange)
			lk.bg = color::lerp(lk.bg, bg, blend);
	});
}


} // NS: termic
