#include <termic/text.h>
#include <termic/utf8.h>

#include <mk-wcwidth.h>

#include <assert.h>


namespace termic
{

extern std::FILE *g_log;


namespace text
{

std::vector<std::string_view> wrap(std::string_view s, std::size_t width, BreakMode brmode)
{
	assert(width > 0);

	(void)brmode;
	(void)width;

	std::vector<text::Word> words = text::words(s, [](char32_t ch) -> int { return ::mk_width(ch); });

	// TODO: wrap 's' into lines, maximum 'width' wide
	// https://unicode.org/reports/tr14

	// summary: western langs: breaks at spaces or hyphens.
	//          asian langs: essentially anywhere or impossible (crap!).

	return {};
};

std::vector<Word> words(std::string_view s, std::function<int(char32_t)> char_width, BreakMode brmode)
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

	if(s.empty())
		return {};

	iter = utf8::begin(s);
	s_end = utf8::end(s);

	Word curr_word;

	// collect locations to break at (e.g. whitespace) in 's'
	while(iter != s_end)
	{
		const auto cp = iter->codepoint;
		const bool is_space = utf8::is_brk_space(cp);
		if(is_space or (brmode == WesternBreaks and cp == '-')) // TODO: break at correct hyphen characters
		{
			if(cp == '-')  // include the hyphen in the word
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


			if(is_space)
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
	}

	if(words.back().end != s.size())
	{
		words.push_back({
		    .start = curr_word.start,
		    .end = s.size(),
		    .width = curr_word.width
		});
	}

	return words;
}


} // NS: text

} // NS: termic
