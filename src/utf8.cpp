#include <termic/utf8.h>

#include <vector>
#include <fmt/core.h>


namespace termic
{
extern std::FILE *g_log;


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

// TODO: this isn't the best location for this; this is more like some "higher level" text manipulation
std::vector<Word> word_split(std::string_view s, std::function<int(char32_t)> char_width)
{
	std::vector<Word> words;
	words.reserve(std::max(5ul, s.size() / 5));  // a stab in the dark

	auto s2 = s;

	auto s_end = utf8::end(s);

	auto iter = utf8::begin(s);
	// skip initial spaces
	while(iter != s_end and utf8::is_space(iter->codepoint))
		s2 = s2.substr(iter->sequence.size());

	if(s2.size() < s.size())
		if(g_log) fmt::print(g_log, "skipped {} initial space\n", s.size() - s2.size());

	s = s2;
	iter = utf8::begin(s);
	s_end = utf8::end(s);

	Word curr_word;
	// collect wrap points (i.e. whitespace) in 's'
	while(iter != s_end)
	{
		const auto cp = iter->codepoint;
		if(utf8::is_brk_space(cp) or cp == '-') // TODO: break at correct dash/hyphen characters: https://unicode.org/reports/tr14/#Hyphen
		{
			if(cp == '-')
				++iter;

			words.push_back({
				.start = curr_word.start,
				.end = iter->byte_offset,
				.width = curr_word.width
			});
			const auto &word = words.back();
			if(g_log) fmt::print(g_log, "word {} ({}-{}): '{}'  width {}\n",
						   words.size(),
						   word.start,
						   word.end,
						   s.substr(word.start, word.end - word.start),
						   word.width);


			if(cp != L'-')
			{
				// skip all consecutive spaces
				while(++iter != s_end and utf8::is_space(iter->codepoint))
					;
			}

			curr_word.start = iter->byte_offset;
			curr_word.width = 0;
		}
		else
		{
			if(char_width)
				curr_word.width += static_cast<std::size_t>(std::max(0, char_width(iter->codepoint)));
			++iter;
		}

		if(words.back().end != s.size())
		{
			words.push_back({
				.start = curr_word.start,
				.end = s.size(),
				.width = curr_word.width
			});
		}
	}

	return words;
}


} // NS: utf8

} // NS: termic
