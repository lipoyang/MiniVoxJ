#include "utf8_decode.h"

std::vector<uint16_t> utf8_decode(const char* str, size_t num)
{
  std::vector<uint16_t> result;
  if (str == nullptr) return result;

  size_t i = 0;
  while (i < num && str[i] != '\0') {
    uint8_t c1 = static_cast<uint8_t>(str[i]);

    // 1-byte character (U+0000 - U+007F)
    // 0xxxxxxx
    if (c1 <= 0x7F) {
      result.push_back(static_cast<uint16_t>(c1));
      i += 1;
    }
    // 2-byte character (U+0080 - U+07FF)
    // 110xxxxx 10xxxxxx
    else if ((c1 & 0xE0) == 0xC0) {
      if (i + 1 >= num || str[i + 1] == '\0') break;
      uint8_t c2 = static_cast<uint8_t>(str[i + 1]);
      if ((c2 & 0xC0) != 0x80) break;
      uint16_t code = ((c1 & 0x1F) << 6) | (c2 & 0x3F);
      result.push_back(code);
      i += 2;
    }
    // 3-byte character (U+0800 - U+FFFF)
    // 1110xxxx 10xxxxxx 10xxxxxx
    else if ((c1 & 0xF0) == 0xE0) {
      if (i + 2 >= num || str[i + 1] == '\0' || str[i + 2] == '\0') break;
      uint8_t c2 = static_cast<uint8_t>(str[i + 1]);
      uint8_t c3 = static_cast<uint8_t>(str[i + 2]);
      if ((c2 & 0xC0) != 0x80) break;
      if ((c3 & 0xC0) != 0x80) break;
      uint16_t code = ((c1 & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
      result.push_back(code);
      i += 3;
    }
    // 4-byte character
    // not supported
    else {
      break;
    }
  }

  return result;
}