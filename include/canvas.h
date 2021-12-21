#pragma once

#include "screen.h"

namespace termic
{

namespace color
{
struct Sampler;
}

struct Rectangle
{
	Pos top_left;
	Size size;
};


struct Canvas
{
	Canvas(Screen &scr) : _scr(scr) {};

	inline void clear() { _scr.clear(); };
	inline Size size() const { return _scr.size(); };

	void fill(Color c);
	void fill(const color::Sampler *s, float sampler_angle=0);
	void fill(Rectangle rect, Color c);
	void fill(Rectangle rect, const color::Sampler *s, float sampler_angle=0);


private:
	Screen &_scr;
};

} // NS: termic
