#ifndef NOO_UTF8_H
#define NOO_UTF8_H

#include "shim4/main.h"

namespace noo {

namespace util {

SHIM4_EXPORT int utf8_len(std::string text);
SHIM4_EXPORT int utf8_len_bytes(std::string text, int char_count);
SHIM4_EXPORT Uint32 utf8_char_next(std::string text, int &offset);
SHIM4_EXPORT Uint32 utf8_char_offset(std::string text, int o);
SHIM4_EXPORT Uint32 utf8_char(std::string text, int i);
SHIM4_EXPORT std::string utf8_char_to_string(Uint32 ch);
SHIM4_EXPORT std::string utf8_substr(std::string s, int start, int count = -1);

} // End namespace util

}

#endif // NOO_UTF8_H
