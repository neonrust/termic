#pragma once

#include <cstdint>

int mk_wcwidth(wchar_t ucs);
int mk_wcswidth(const wchar_t *pwcs, std::size_t n);
