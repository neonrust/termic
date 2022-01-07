#include <termic/app.h>

#include <iostream>
#include <tuple>
#include <variant>
#include <csignal>
#include <fmt/core.h>
#include <cuchar>
#include <string_view>

#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

using namespace std::literals;


namespace termic
{
extern std::FILE *g_log;

namespace esc
{
[[maybe_unused]] const auto screen_alternate { "\x1b[?1049h"sv };
[[maybe_unused]] const auto screen_normal { "\x1b[?1049l"sv };

[[maybe_unused]] const auto cursor_hide { "\x1b[?25l"sv };
[[maybe_unused]] const auto cursor_show { "\x1b[?25h"sv };

// reporting of mouse buttons (including position)
// https://invisible-island.net/xterm/ctlseqs/ctlseqs.pdf
//#define SET_VT200_MOUSE            1000
//#define SET_VT200_HIGHLIGHT_MOUSE  1001
//#define SET_BTN_EVENT_MOUSE        1002
//#define SET_ANY_EVENT_MOUSE        1003
//#define SET_FOCUS_EVENT_MOUSE      1004
//#define SET_ALTERNATE_SCROLL       1007
//#define SET_EXT_MODE_MOUSE         1005
//#define SET_SGR_EXT_MODE_MOUSE     1006
//#define SET_URXVT_EXT_MODE_MOUSE   1015
//#define SET_PIXEL_POSITION_MOUSE   1016
[[maybe_unused]] const auto mouse_buttons_on  { "\x1b[?1000h\x1b[?1002h\x1b[?1015h\x1b[?1006h"sv };
[[maybe_unused]] const auto mouse_buttons_off { "\x1b[?1000l\x1b[?1002l\x1b[?1015l\x1b[?1006l"sv };
// reporting of mouse position
[[maybe_unused]] const auto mouse_move_on  { "\x1b[?1003h"sv };
[[maybe_unused]] const auto mouse_move_off { "\x1b[?1003l"sv };

[[maybe_unused]] const auto focus_on  { "\x1b[?1004h"sv };
[[maybe_unused]] const auto focus_off { "\x1b[?1004l"sv };

// terminal synchronized output markers
[[maybe_unused]] const auto synch_start { "\x1b[?2026h"sv };
[[maybe_unused]] const auto synch_end   { "\x1b[?2026l"sv };

} // NS: esc


ssize_t write(const std::string_view s);

using IOFlag = decltype(termios::c_lflag);
// NOTE: make sure these flag bits does not overlap if used simultaneously
[[maybe_unused]] static constexpr IOFlag LocalEcho      = ECHO;
[[maybe_unused]] static constexpr IOFlag LineBuffering  = ICANON;
[[maybe_unused]] static constexpr IOFlag SignalDecoding = ISIG;
[[maybe_unused]] static constexpr IOFlag EightBit       = CS8;
[[maybe_unused]] static constexpr IOFlag CRtoLF         = ICRNL;

::termios initial_settings; // this is here instead of .h to avoid extra includes (and only one app is supported anyway)

bool modify_io_flags(bool set, IOFlag flags);
bool clear_in_flags(IOFlag flags);

bool init_terminal(Options opts)
{
	if(not isatty(STDIN_FILENO))
		return false;

	if(::tcgetattr(STDIN_FILENO, &initial_settings) != 0)
		return false;

	if(g_log) fmt::print(g_log, "   \x1b[2mterm >> turning off stdio synch...\x1b[m\n");
	std::cin.sync_with_stdio(false);
	std::cout.sync_with_stdio(false);

	if(g_log) fmt::print(g_log, "   \x1b[2mterm >> turning off tied stream...\x1b[m\n");
	std::cin.tie(nullptr);
	std::cout.tie(nullptr);

	if(g_log) fmt::print(g_log, "   \x1b[2mterm >> clear termios flags..\x1b[m\n");
	clear_in_flags(LocalEcho | LineBuffering);
	//modify_io_flags(true, EightBit | CRtoLF);

	if(g_log) fmt::print(g_log, "   \x1b[2mterm >> switching to alternate screen...\x1b[m\n");
	write(esc::screen_alternate);

	if((opts & NoSignalDecode) > 0)
	{
		if(g_log) fmt::print(g_log, "   \x1b[2mterm >> disabling signal sequence decoding...\x1b[m\n");
		clear_in_flags(SignalDecoding);
	}

	if((opts & HideCursor) > 0)
	{
		if(g_log) fmt::print(g_log, "   \x1b[2mterm >> hiding cursor...\x1b[m\n");
		write(esc::cursor_hide);
	}
	if((opts & MouseButtonEvents) > 0)
	{
		if(g_log) fmt::print(g_log, "   \x1b[2mterm >> enabling mouse button events...\x1b[m\n");
		write(esc::mouse_buttons_on);
	}
	if((opts & MouseMoveEvents) > 0)
	{
		if(g_log) fmt::print(g_log, "   \x1b[2mterm >> enabling mouse move events...\x1b[m\n");
		write(esc::mouse_move_on);
	}
	if((opts & FocusEvents) > 0)
	{
		if(g_log) fmt::print(g_log, "   \x1b[2mterm >> enabling focus events...\x1b[m\n");
		write(esc::focus_on);
	}

	return true;
}

void restore_terminal()
{
	if(g_log) fmt::print(g_log, "\x1b[31;1mshutdown()\x1b[m\n");

	::tcsetattr(STDIN_FILENO, TCSANOW, &initial_settings);

	write(esc::focus_off);
	write(esc::mouse_move_off);
	write(esc::mouse_buttons_off);
	write(esc::screen_normal);
	write(esc::cursor_show);
}

ssize_t write(const std::string_view s)
{
	return ::write(STDOUT_FILENO, s.data(), s.size());
}

bool clear_in_flags(IOFlag flags)
{
	return modify_io_flags(false, flags);
}

bool modify_io_flags(bool set, IOFlag flags)
{
	::termios settings;

	if(::tcgetattr(STDIN_FILENO, &settings))
		return false;

	// NOTE: this only works if none of the flag bits overlap between lflags, cflags and iflags
	static constexpr auto iflags_mask = CRtoLF;
	static constexpr auto cflags_mask = EightBit;
	static constexpr auto lflags_mask = LocalEcho | LineBuffering  | SignalDecoding;

	const auto iflags = flags & iflags_mask;
	const auto lflags = flags & lflags_mask;
	const auto cflags = flags & cflags_mask;
	if(set)
	{
		if(iflags)
			settings.c_iflag |= iflags;
		if(cflags)
			settings.c_cflag |= cflags;
		if(lflags)
			settings.c_lflag |= lflags;
	}
	else
	{
		if(iflags)
			settings.c_iflag &= ~iflags;
		if(cflags)
			settings.c_cflag &= ~cflags;
		if(lflags)
			settings.c_lflag &= ~lflags;
	}

	if(::tcsetattr(STDIN_FILENO, TCSANOW, &settings))
		return false;

	if(flags & LineBuffering)
	{
		if(set)
			::setlinebuf(stdin);
		else
			::setbuf(stdin, nullptr);
	}

	return true;
}

} // NS: termic
