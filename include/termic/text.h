#pragma once

#include <string_view>
#include <vector>
#include <fmt/core.h>

#include "utf8.h"

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

// TODO: should use something established instead, e.g. https://github.com/DuffsDevice/tiny-utf8
//   these functions are very slow, they always perform iteration from the beginning

utf8::string &insert(utf8::string &s, utf8::string_view insert, std::size_t at);
utf8::string &erase(utf8::string &s, std::size_t start, std::size_t len);
std::size_t size(utf8::string_view s);


} // MS: text

} // NS: termic


