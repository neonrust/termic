#include <termic/utf8.h>

namespace termic
{

namespace utf8
{

CodepointIterator::CodepointIterator(std::string_view s) :
	_s(s)
{
	read_next();
}

void CodepointIterator::read_next()
{
	std::size_t eaten = 0;
	_current = codepoint_from_bytes(_s.substr(_index), &eaten);

	_index += eaten;
}

// this was ruthlessly stolen from termlib (tkbd.c)
static const std::uint8_t utf8_length[] = {
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // 0x00
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // 0x20
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // 0x40
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // 0x60
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // 0x80
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // 0xa0
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, // 0xc0
	3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,6,6,1,1  // 0xe0
};
static const std::uint8_t utf8_mask[] = { 0x7f, 0x1f, 0x0f, 0x07, 0x03, 0x01 };

// ==================================================================

SequenceIterator::SequenceIterator(std::string_view s) :
	_s(s)
{
	read_next();
}

void SequenceIterator::read_next()
{
	std::size_t eaten = 0;
	_current = sequence_from_bytes(_s.substr(_index), &eaten);

	_index += eaten;
}





std::uint32_t codepoint_from_bytes(std::string_view s, std::size_t *eaten)
{
	if(eaten)
		*eaten = 0;

	if(s.empty())
		return 0;

	auto len = utf8_length[uint8_t(s[0])];
	if(len > s.size())
		return 0;

	const auto mask = utf8_mask[len - 1];
	std::uint32_t codepoint = static_cast<char8_t>(s[0] & mask);

	for(std::size_t idx = 1; idx < len; ++idx)
	{
		codepoint <<= 6;
		codepoint |= static_cast<char32_t>(s[idx] & 0x3f);
	}

	if(eaten)
		*eaten = len;

	return codepoint;
}

std::string_view sequence_from_bytes(std::string_view s, std::size_t *eaten)
{
	if(eaten)
		*eaten = 0;

	if(s.empty())
		return {};

	auto len = utf8_length[uint8_t(s[0])];
	if(len > s.size())
		return {};

	if(eaten)
		*eaten = len;

	return s.substr(0, len);
}

} // NS: utf8

} // NS: termic
