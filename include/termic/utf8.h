#pragma once

#include <string>
#include <string_view>
#include <unordered_set>
#include <limits>
#include <cstdint>


namespace termic
{

namespace utf8
{

// extract the byte sequence worth a single codepoint from the input data (let's see you come up with a better name!)
std::string_view sequence_from_bytes(std::string_view s, std::size_t *eaten=nullptr);

// get first codepoint from the input data
uint32_t codepoint_from_bytes(std::string_view s, std::size_t *eaten=nullptr);

struct Character
{
	std::uint32_t codepoint { 0 };
	std::string_view sequence {};
	std::size_t index { 0 };
};

struct Iterator
{
	friend Iterator end(std::string_view s);

	explicit Iterator(std::string_view s);

	inline Character operator * () const { return _current; }
	inline const Character *operator -> () const { return &_current; }

	inline Iterator &operator++() { read_next(); return *this; }

	inline bool operator == (const Iterator &other) const
	{
		return _s.data() == other._s.data() and _index == other._index and _current.codepoint == other._current.codepoint;
	}

private:
	void read_next();

private:
	std::string_view _s;
	std::size_t _index { 0 };
	Character _current;
};

inline Iterator begin(std::string_view s)
{
	return Iterator(s);
}

inline Iterator end(std::string_view s)
{
	auto iter = Iterator(s);
	iter._index = s.size();
	iter._current.codepoint = 0;
	return iter;
}

inline bool is_brk_space(std::uint32_t codepoint)
{
	// https://jkorpela.fi/chars/spaces.html
	static constexpr std::uint32_t breaking_spacechars[] {
		0x0020,
		//0x00A0,
		0x1680,
		0x180E,
		0x2000,
		0x2001,
		0x2002,
		0x2003,
		0x2004,
		0x2005,
		0x2006,
		0x2007,
		0x2008,
		0x2009,
		0x200A,
		0x200B,
		//0x202F,
		0x205F,
		0x3000,
		//0xFEFF
	};

	for(const auto sc: breaking_spacechars)
	{
		if(sc > codepoint)  // already passed numerically, we'll never find a match
			return false;
		if(sc == codepoint)
			return true;
	}

	return false;
}


} // NS: utf8

} // NS: termic
