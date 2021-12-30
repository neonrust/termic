#pragma once

#include <string>
#include <string_view>
#include <cstdint>

namespace termic
{

namespace utf8
{

// extract the byte sequence worth a single codepoint from the input data (let's see you come up with a better name!)
std::string_view sequence_from_bytes(std::string_view s, std::size_t *eaten=nullptr);

// get first codepoint from the input data
uint32_t codepoint_from_bytes(std::string_view s, std::size_t *eaten=nullptr);

struct CodepointIterator
{
	CodepointIterator(std::string_view s);

	inline bool good() const { return _current != 0; }

	inline std::uint32_t operator *() const { return _current; }
	inline CodepointIterator &operator++() { read_next(); return *this; }

private:
	void read_next();

private:
	std::string_view _s;
	std::size_t _index { 0 };
	std::uint32_t _current;
};


struct SequenceIterator
{
	SequenceIterator(std::string_view s);

	inline bool good() const { return not _current.empty(); }

	inline const std::string &operator *() const { return _current; }
	inline SequenceIterator &operator++() { read_next(); return *this; }

private:
	void read_next();

private:
	std::string_view _s;
	std::size_t _index { 0 };
	std::string _current;
};

} // NS: utf8

} // NS: termic
