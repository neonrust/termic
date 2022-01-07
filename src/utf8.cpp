#include <termic/utf8.h>

namespace termic
{

namespace utf8
{

Iterator::Iterator(std::string_view s) :
	  _s(s)
{
	read_next();
	_current.index = 0;
}

void Iterator::read_next()
{
	if(_head_offset == _s.size())  // already at the end, can't read more
		_current.codepoint = 0;
	else
	{
		std::size_t eaten { 0 };

		++_current.index;
		_current.byte_offset = _head_offset;

		auto ch = read_one(_s.substr(_head_offset), &eaten);
		_current.codepoint = ch.first;
		_current.sequence = ch.second;

		_head_offset += eaten;
	}
}


// this was ruthlessly stolen from termlib (tkbd.c)

static constexpr std::uint8_t sequence_length[] = {
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x00
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x20
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x40
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x60
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x80
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0xa0
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 0xc0
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 1, 1  // 0xe0
};
static constexpr std::uint8_t initial_mask[] = {
	0x7f, 0x1f, 0x0f, 0x07, 0x03, 0x01
};
static constexpr std::uint8_t subsequent_mask { 0x3f };

std::pair<char32_t, std::string_view> read_one(std::string_view s, std::size_t *eaten)
{
	if(eaten)
		*eaten = 0;

	const auto first = static_cast<char8_t>(s[0]);

	auto len = sequence_length[first];  // using a char8_t as index :|
	if(len > s.size())
		return { 0, {} };

	if(eaten)
		*eaten = len;

	s = s.substr(0, len);

	char32_t codepoint = first & initial_mask[len - 1];

	for(auto idx = 1u; idx < len; ++idx)
	{
		codepoint <<= 6;
		codepoint |= static_cast<char32_t>(s[idx] & subsequent_mask);
	}

	return { codepoint, s };
}

} // NS: utf8

} // NS: termic
