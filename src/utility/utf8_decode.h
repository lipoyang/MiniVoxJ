#pragma once

// UTF-8 decoder
// only for Basic Multilingual Plane (BMP, U+0000 - U+FFFF)

#include <vector>
#include <stdint.h>

std::vector<uint16_t> utf8_decode(const char* str, size_t num);
