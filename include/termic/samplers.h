#pragma once

#include "cell.h"
#include "size.h"

#include <vector>


namespace termic
{

struct UV
{
	float u;
	float v;
};

namespace color
{
struct Sampler
{
	virtual Color sample(UV uv, float angle=0) const = 0;
};

struct Constant : public Sampler
{
	inline Constant(Color c) : _c(c) {};

	inline Color sample(UV, float) const override { return _c; }

private:
	Color _c;
};

struct LinearGradient : public Sampler
{
	LinearGradient(std::initializer_list<Color> colors);

	void set_offset(float offset);

	Color sample(UV uv, float angle) const override;

private:
	Color _colors[16];
	std::size_t _num_colors;
	float _offset { 0 };
};

} // NS: color

} // NS: termic
