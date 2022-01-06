#pragma once

#include "keycodes.h"
#include "size.h"

#include <string>
#include <tuple>
#include <variant>


namespace termic
{

namespace event
{

struct Key
{
	key::Key key;
	key::Modifier modifiers { key::NoMod };
};
struct Input
{
	char32_t codepoint;

	inline std::string to_string() const
	{
		// TODO: move this to utf8.cpp
		auto cp = codepoint;

		std::size_t len { 0 };
		char8_t first { 0 };

		if(cp < 0x80)
		{
			first = 0;
			len = 1;
		}
		else if(cp < 0x800)
		{
			first = 0xc0;
			len = 2;
		}
		else if(cp < 0x10000)
		{
			first = 0xe0;
			len = 3;
		}
		else if(cp <= 0x10FFFF)
		{
			first = 0xf0;
			len = 4;
		}
		else
			return "";

		std::string s;
		s.resize(len);

		for(int idx = int(len - 1); idx > 0; --idx)
		{
			s[std::size_t(idx)] = static_cast<char>((cp & 0x3f) | 0x80);
			cp >>= 6;
		}
		s[0] = static_cast<char>(cp | first);

		return s;
	}
};
struct MouseButton
{
	int button;
	bool pressed { false };
	bool released { false };
	bool double_clicked { false };
	std::size_t x;
	std::size_t y;
	key::Modifier modifiers { key::NoMod };
};
struct MouseWheel
{
	int delta { 0 };
	std::size_t x;
	std::size_t y;
	key::Modifier modifiers { key::NoMod };
};
struct MouseMove
{
	std::size_t x;
	std::size_t y;
	key::Modifier modifiers { key::NoMod };
};
struct Resize
{
	std::size_t x { 0 };  // only applicable for sub-surfaces
	std::size_t y { 0 };  // only applicable for sub-surfaces
	Size size { 0, 0 };

	struct {
		std::size_t x { 0 };  // only applicable for sub-surfaces
		std::size_t y { 0 };  // only applicable for sub-surfaces
		Size size { 0, 0 };
	} old {};
};
struct Focus
{
	bool focused;
};


using Event = std::variant<Key, Input, MouseButton, MouseWheel, MouseMove, Resize, Focus>;

} // NS: event

} // NS: termic
