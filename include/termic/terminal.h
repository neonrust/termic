#pragma once

#include <termic/size.h>

namespace termic
{

enum Options
{
	Defaults          = 0,
	HideCursor        = 1 << 0,
	MouseButtonEvents = 1 << 1,
	MouseMoveEvents   = 1 << 2,
	MouseEvents       = MouseButtonEvents | MouseMoveEvents,
	FocusEvents       = 1 << 3,
	NoSignalDecode    = 1 << 16,
};

// bitwise OR of multiple 'Options' is still an 'Options'
inline Options operator | (Options a, Options b)
{
	return static_cast<Options>(static_cast<int>(a) | static_cast<int>(b));
}

namespace term
{

bool init(int in_fd, int out_fd, Options opts);
void restore(int in_fd, int out_fd);

Size get_size(int fd);

} // NS: term

} // NS: termicic
