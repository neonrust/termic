#pragma once

#include <string_view>
#include <vector>
#include <fmt/core.h>


namespace termic
{

namespace text
{

std::vector<std::string_view> wrap(std::string_view s, std::size_t width);

struct Word
{
	std::size_t start { 0 };
	std::size_t end   { 0 };
	std::size_t width { 0 };
};

std::vector<Word> words(std::string_view s, std::function<int (char32_t)> char_width);

} // MS: text

} // NS: termic


