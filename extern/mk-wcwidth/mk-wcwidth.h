#pragma once

#include <string_view>
#include <cstdint>

int mk_width(char32_t ucs);
int mk_width(std::u32string_view s);
