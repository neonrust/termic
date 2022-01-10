#include <termic/text.h>
#include <termic/utf8.h>

#include <mk-wcwidth.h>

#include <assert.h>


namespace termic
{

extern std::FILE *g_log;


namespace text
{

std::pair<std::string_view, std::size_t> part_upto_width(std::string_view s, std::size_t limit);


std::vector<std::string> wrap(std::string_view s, std::size_t limit, BreakMode brmode)
{
	assert(limit > 0);

	if(g_log) fmt::print(g_log, "wrapping '{}'\n", s);

	auto words = text::words(s, [](char32_t ch) -> int { return ::mk_width(ch); }, brmode);


	std::vector<std::string> lines;
	lines.reserve(10);

	std::string line;
	line.reserve(limit*2);  // an upper limit-guess
	std::size_t line_width { 0 };
	// wrap 's' into lines, maximum 'width' wide,  https://unicode.org/reports/tr14
	std::size_t word_idx { 0 };
//	bool prev_hyphen { false };

	for(auto word: words)
	{
		const auto &[ws, we, ww, hyphen] = word;
		const auto wl = we - ws;

		if(line_width + ww < limit)  // word fits current line
		{
			line += s.substr(ws, wl);
			line_width += word.width;
			if(g_log) fmt::print(g_log, "[{}] word {} fit on current line -> {}\n", lines.size(), word_idx, line_width);
			if(line_width < limit and not hyphen)
			{
				line += ' ';
				++line_width;
			}
		}
		else if(ww < limit) // word doesn't fit on current line but will fit next line (next loop)
		{
			if(g_log) fmt::print(g_log, "[{}] word {} fit on a new line\n", lines.size(), word_idx);
			// just push this line
			lines.push_back(line);
			line.clear();
			line += s.substr(word.start, word.end - word.start);
			line_width = word.width;
			if(line_width < limit and not hyphen)
			{
				line += ' ';
				++line_width;
			}
		}
		else
		{
			if(word.width > limit)  // word doesn't fit on a whole line, we must cut it
			{
				if(g_log) fmt::print(g_log, "[{}] word {} doesn't fit at all, width {} > {}\n", lines.size(), word_idx, word.width, limit);

				while(word.width > 0)
				{
					if(true)//limit < line_width)
					{
						const auto line_remainder = limit - line_width;
						const auto use_hyphen = word.width > line_remainder;

						const auto &[part, part_width] = part_upto_width(s.substr(word.start, word.end - word.start), line_remainder - (use_hyphen? 1: 0));
						word.width -= part_width;
						word.start += part.size();

						line += part;

						line_width += part_width;
						if(use_hyphen)
						{
							line += '-';
							++line_width;
						}
					}

					// TODO: only push line if there's "no more space" on the line
					lines.push_back(line);
					line_width = 0;
					line.clear();
				}
			}
			else
			{
				lines.push_back(line);
				line = s.substr(word.start, word.end - word.start);
				line_width = word.width;
				if(g_log) fmt::print(g_log, "[{}] word {} fits on the next line -> {}\n", lines.size(), word_idx, line_width);
			}
		}
		++word_idx;
	}
	if(not line.empty())
		lines.push_back(line);

	return lines;
};

std::pair<std::string_view, std::size_t> part_upto_width(std::string_view s, std::size_t limit)
{
	std::size_t width { 0 };

	std::size_t end_index { 0 };
	const auto s_end = utf8::end(s);

	for(auto iter = utf8::begin(s); iter != s_end and width < limit; ++iter)
	{
		const auto w = static_cast<std::size_t>(::mk_width(iter->codepoint));
		if(width + w > limit)
			break;
		width += w;
		end_index = iter->byte_offset + iter->sequence.size();
	}
	return { s.substr(0, end_index), width };
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
				.width = curr_word.width,
				.hyphenated = cp == '-',
			});

			const auto &word = words.back();
			if(g_log) fmt::print(g_log, "word {} ({}-{}): '{}'  width {}\n",
						   words.size() - 1,
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
		if(g_log) fmt::print(g_log, "word {} ({}-{}): '{}'  width {}\n",
					   words.size() - 1,
					   curr_word.start,
					   curr_word.end,
					   s.substr(curr_word.start, curr_word.end - curr_word.start),
					   curr_word.width);
	}

	return words;
}


} // NS: text

} // NS: termic
