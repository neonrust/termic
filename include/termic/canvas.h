#pragma once

#include "look.h"
#include "size.h"

#include <functional>

namespace termic
{

struct Screen;

namespace color
{
struct Sampler;
}
struct UV;

struct Canvas
{
	inline Canvas(Screen &scr) : _screen(scr) {};

	void clear();
	Size size() const;

	void fill(Color c);
	void fill(const color::Sampler *s, float sampler_angle=0);
	void fill(Rectangle rect, Color c);
	void fill(Rectangle rect, const color::Sampler *s, float sampler_angle=0);

	void filter(std::function<void (Look &, UV)> f);
	void filter(Rectangle rect, std::function<void(Look &, UV)> f);
	void fade(float blend);
	void fade(Color fg=color::Black, Color bg=color::Black, float blend=0.5f);
	void fade(Rectangle rect, float blend=0.5f);
	void fade(Rectangle rect, Color fg, Color bg, float blend=0.5f);

private:
	Screen &_screen;
};

} // NS: termic
