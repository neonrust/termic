#pragma once

#include "keycodes.h"
#include "size.h"

#include <string>
#include <tuple>
#include <variant>

#include <fmt/format.h>
using namespace fmt::literals;

namespace termic
{

namespace event
{

struct Key
{
	key::Key key;
	key::Modifier modifiers { key::NoMod };

	inline std::string to_string() const
	{
		return key::to_string(key, modifiers);
	}
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

	inline std::string to_string() const
	{
		std::string s;
		s.reserve(15);

		s.append(key::to_string(key::None, modifiers));

		if(not s.empty())
			s += '+';

		return s.append("MouseButton{}{}"_format(released? "Up": (double_clicked? "Dbl": "Down"), button));
	}

};
struct MouseWheel
{
	int delta { 0 };
	std::size_t x;
	std::size_t y;
	key::Modifier modifiers { key::NoMod };

	inline std::string to_string() const
	{
		std::string s;
		s.reserve(15);

		s.append(key::to_string(key::None, modifiers));

		if(not s.empty())
			s += '+';

		return s.append("MouseWheel{}"_format(delta > 1? "Up": "Down"));
	}
};
struct MouseMove
{
	std::size_t x;
	std::size_t y;
	key::Modifier modifiers { key::NoMod };

	inline std::string to_string() const
	{
		std::string s;
		s.reserve(15);

		s.append(key::to_string(key::None, modifiers));

		if(not s.empty())
			s += '+';

		// this is kind of useless
		// TODO: what we might want is movement in any of the orthogonal directions
		//   but that requires knowledge of where the mouse was before and its "general direction" (over a time window?)
		//   something we obviously don't have here (yet).
		//   the event encoder might included that information in the event...
		return s.append("MouseMove{}-{}"_format(x, y));
	}
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
