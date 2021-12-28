#pragma once

#include "cell.h"
#include "size.h"

#include <vector>


namespace termic
{

namespace color
{

struct Sampler
{
	virtual Color sample(float u, float v, float angle=0) const = 0;
};

struct Constant : public Sampler
{
	inline Constant(Color c) : _c(c) {};

	inline Color sample(float, float, float) const override { return _c; }

private:
	Color _c;
};

struct LinearGradient : public Sampler
{
	LinearGradient(std::initializer_list<Color> colors);

	void set_offset(float offset);

	Color sample(float u, float v, float angle) const override;

private:
	std::vector<Color> _colors {};
	float _offset { 0 };
};

Color lerp(Color a, Color b, float blend);

} // NS: color

} // NS: termic
