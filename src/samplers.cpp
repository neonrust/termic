#include <termic/samplers.h>

#include <cmath>
#include <algorithm>

#include <assert.h>


namespace termic
{
extern std::FILE *g_log;

namespace color
{

[[maybe_unused]] static constexpr auto deg2rad = std::numbers::pi_v<float>/180.f;

LinearGradient::LinearGradient(std::initializer_list<Color> colors) :
	 _colors(colors)
{
	assert(_colors.size() > 0);

	// strip special bits, to avoid strange behaviour
	std::for_each(_colors.begin(), _colors.end(), [](auto &c) {
		c = c & ~color::special_mask;
	});
}

void LinearGradient::set_offset(float offset)
{
	_offset = offset;
}

Color LinearGradient::sample(float u, float v, float angle) const
{
	assert(u >= 0.f and u <= 1.f and v >= 0.f and v <= 1.f);

	if(_colors.size() == 1)
		return _colors.front();

	angle = std::fmod(std::fmod(angle, 360.f) + 360.f, 360.f); // ensure in range [0, 360]

	// rotate the vector 'uv' by -_rotation degrees
	auto degrees = angle;

	if(degrees >= 270)
	{
		degrees = 360 - degrees;
		v = 1.f - v;
	}
	else if(degrees >= 180)
	{
		degrees = degrees - 180;
		u = 1.f - u;
		v = 1.f - v;
	}
	else if(degrees >= 90)
	{
		degrees = 180 - degrees;
		u = 1.f - u;
	}

	auto radians = degrees*deg2rad;

	auto alpha = u*std::cos(-radians) - v*std::sin(-radians);

	if(alpha == 0.f)
		return _colors.front();
	else if(alpha == 1.f)
		return _colors.back();

	// this is definitely not the correct way to do it...
	alpha *= std::max(std::abs(std::sin(-radians)), std::abs(std::cos(-radians)));


	alpha = std::fmod(alpha + _offset, 1.f);

	const auto idx = alpha*static_cast<float>(_colors.size() - 1);
	const auto idx0 = static_cast<std::size_t>(std::floor(idx));

	const auto blend = idx - float(idx0);
	assert(blend >= 0 and blend <= 1);

	const auto color0 = _colors[idx0];
	if(idx0 == _colors.size() - 1)
		return color0;

	assert(idx0 + 1 < _colors.size());
	const auto color1 = _colors[idx0 + 1];

	return lerp(color0, color1, blend);
}

Color lerp(Color A, Color B, float blend)
{
	// TODO: this can probably be made more succinct... ;)

	assert(blend >= 0 and blend <= 1);

	const auto Ar = color::red(A);
	const auto Ag = color::green(A);
	const auto Ab = color::blue(A);

	const auto Br = color::red(B);
	const auto Bg = color::green(B);
	const auto Bb = color::blue(B);

	const auto r = static_cast<std::uint32_t>(Ar - blend*(Ar - Br));
	const auto g = static_cast<std::uint32_t>(Ag - blend*(Ag - Bg));
	const auto b = static_cast<std::uint32_t>(Ab - blend*(Ab - Bb));

	return Color(r << 16 | g << 8 | b);
}

} // NS: color

} // NS: termic
