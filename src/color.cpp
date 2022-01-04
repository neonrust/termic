#include <termic/color.h>


#include <assert.h>


namespace termic
{

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

	const auto r = static_cast<std::uint8_t>(Ar - blend*(Ar - Br));
	const auto g = static_cast<std::uint8_t>(Ag - blend*(Ag - Bg));
	const auto b = static_cast<std::uint8_t>(Ab - blend*(Ab - Bb));

	return color::rgb(r, g, b);
}



} // NS: termic
