#pragma once

#include <string_view>
#include <vector>
#include <fmt/core.h>


namespace termic
{

namespace text
{

enum BreakMode  // https://unicode.org/reports/tr14/#BreakOpportunities
{
	WesternBreaks,
	EastAsianBreaks,
	SouthEastAsianBreaks,
};

std::vector<std::string> wrap(std::string_view s, std::size_t limit, termic::text::BreakMode brmode=WesternBreaks);

struct Word
{
	std::size_t start { 0 };
	std::size_t end   { 0 };
	std::size_t width { 0 };
	bool hyphenated { false };
};

std::vector<Word> words(std::string_view s, std::function<int (char32_t)> char_width, termic::text::BreakMode brmode=WesternBreaks);

} // MS: text

} // NS: termic


