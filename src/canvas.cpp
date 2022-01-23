#include <termic/screen.h>
#include <termic/canvas.h>
#include <termic/samplers.h>

#include <fmt/core.h>

namespace termic
{

void Canvas::clear()
{
	_scr.clear();
}

Size Canvas::size() const
{
	return _scr.size();
}

void Canvas::fill(Color c)
{
	auto size = _scr.size();
	fill({ { 0, 0 }, size }, c);
}

void Canvas::fill(const color::Sampler *s, float sampler_angle)
{
	auto size = _scr.size();
	fill({ { 0, 0 }, size }, s, sampler_angle);
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

	const auto size = _scr.size();

	for(auto y = rect.top_left.y; y <= rect.top_left.y + rect.size.height - 1 and y < size.height; y++)
	{
		for(auto x = rect.top_left.x; x <= rect.top_left.x + rect.size.width - 1 and x < size.width; x++)
		{
			const float u = static_cast<float>(x - rect.top_left.x + 1) / float(rect.size.width);
			const float v = static_cast<float>(y - rect.top_left.y + 1) / float(rect.size.height);

			_scr.set_cell({ x, y }, Cell::NoChange, 1, look::bg(s->sample(u, v, sampler_angle)));
		}
	}
}


} // NS: termic
