#pragma once

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

bool init_terminal(Options opts);
void restore_terminal();

} // NS: termicic
