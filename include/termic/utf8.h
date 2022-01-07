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

// extract a single codepoint from the input data, returning its codepoint and the byte sequence
std::pair<char32_t, std::string_view> read_one(std::string_view s, std::size_t *eaten);


struct Iterator
{
	friend Iterator end(std::string_view s);

	explicit Iterator(std::string_view s);

	struct Character
	{
		std::size_t index { 0 };
		std::size_t byte_offset { 0 };
		char32_t    codepoint { 0 };
		std::string_view sequence {};
	};

	inline Character operator * () const { return _current; }
	inline const Character *operator -> () const { return &_current; }

	inline Iterator &operator++() { read_next(); return *this; }

	inline bool operator == (const Iterator &other) const
	{
		return _s.data() == other._s.data() and _head_offset == other._head_offset and _current.codepoint == other._current.codepoint;
	}

private:
	void read_next();

private:
	std::string_view _s;
	std::size_t _head_offset { 0 };
	Character _current;
};

inline Iterator begin(std::string_view s)
{
	return Iterator(s);
}

inline Iterator end(std::string_view s)
{
	auto iter = Iterator(s);
	iter._head_offset = s.size();
	iter._current.codepoint = 0;
	return iter;
}

bool is_brk_space(char32_t codepoint);

inline bool is_space(char32_t codepoint)
{
	// https://jkorpela.fi/chars/spaces.html
	static constexpr std::uint32_t spacechars[] {
		0x00A0,
		0x202F,
		0xFEFF,
	};

	for(const auto sc: spacechars)
	{
		if(sc > codepoint)  // already passed numerically, we'll never find a match
			return is_brk_space(codepoint);
		if(sc == codepoint)
			return true;
	}

	return is_brk_space(codepoint);
}

inline bool is_brk_space(char32_t codepoint)
{
	// https://jkorpela.fi/chars/spaces.html
	static constexpr std::uint32_t breaking_spacechars[] {
		0x0020,
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
		0x205F,
		0x3000,
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

struct Word
{
	std::size_t start { 0 };
	std::size_t end   { 0 };
	std::size_t width { 0 };
};
std::vector<Word> word_split(std::string_view s, std::function<int (char32_t)> char_width);


} // NS: utf8

} // NS: termic
