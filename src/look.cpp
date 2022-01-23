#include <termic/look.h>

namespace termic
{

namespace color
{

Color lerp(Color A, Color B, float blend)
{
	// TODO: this can probably be made more succinct... ;)

	blend = std::min(1.f, std::max(0.f, blend));

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

} // NS: color

} // NS: termic
