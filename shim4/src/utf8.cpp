#include "shim4/utf8.h"
#include "shim4/util.h"

namespace noo {

namespace util {

static inline int utf8_size(unsigned const char *p)
{
	unsigned char ch = *p;

	if (ch == 0) {
		return 0;
	}

	if ((ch & 0x80) == 0) {
		return 1;
	}
	else if ((ch & 0xE0) == 0xC0) {
		return 2;
	}
	else if ((ch & 0xF0) == 0xE0) {
		return 3;
	}
	else {
		return 4;
	}
}

static inline Uint32 utf8_next(unsigned const char *ptr, int &offset)
{
	unsigned const char *p = ptr + offset;
	unsigned char ch = *p;
	Uint32 result = 0;

	if ((ch & 0x80) == 0) {
		result = *p;
		offset++;
	}
	else if ((ch & 0xE0) == 0xC0) {
		unsigned char a = *p++;
		unsigned char b = *p;
		result = ((a & 0x1F) << 6) | (b & 0x3F);
		offset += 2;
	}
	else if ((ch & 0xF0) == 0xE0) {
		unsigned char a = *p++;
		unsigned char b = *p++;
		unsigned char c = *p;
		result = ((a & 0xF) << 12) | ((b & 0x3F) << 6) | (c & 0x3F);
		offset += 3;
	}
	else {
		unsigned char a = *p++;
		unsigned char b = *p++;
		unsigned char c = *p++;
		unsigned char d = *p;
		result = ((a & 0x7) << 18) | ((b & 0x3F) << 12) | ((c & 0x3F) << 6) | (d & 0x3F);
		offset += 4;
	}

	return result;
}

static inline int utf8_len_ptr(unsigned const char *p)
{
	int len = 0;

	while (*p != 0) {
		len++;
		p += utf8_size(p);
	}

	return len;
}

int utf8_len(std::string text)
{
	return utf8_len_ptr((unsigned const char *)text.c_str());
}

static int utf8_len_bytes_ptr(unsigned const char *p, int char_count)
{
	unsigned const char *save = p;

	for (int i = 0; *p != 0 && (char_count == -1 || i < char_count); i++) {
		p += utf8_size(p);
	}

	return (int)(p - save);
}

int utf8_len_bytes(std::string s, int char_count)
{
	return utf8_len_bytes_ptr((unsigned const char *)s.c_str(), char_count);
}

Uint32 utf8_char_next(std::string text, int &offset)
{
	unsigned const char *p = (unsigned const char *)text.c_str();

	Uint32 result = utf8_next(p, offset);

	return result;
}

Uint32 utf8_char_offset(std::string text, int o)
{
	unsigned const char *p = (unsigned const char *)text.c_str();

	Uint32 result = utf8_next(p, o);

	return result;
}

Uint32 utf8_char(std::string text, int i)
{
	unsigned const char *p = (unsigned const char *)text.c_str();

	for (int count = 0; count < i; count++) {
		int size = utf8_size(p);
		if (size == 0) {
			return 0;
		}
		p += size;
	}

	int o = 0;

	return utf8_next(p, o);
}

std::string utf8_char_to_string(Uint32 ch)
{
	unsigned char buf[5];

	int bytes;

	if (ch & 0x001F0000) {
		bytes = 4;
	}
	else if (ch & 0x0000F800) {
		bytes = 3;
	}
	else if (ch & 0x00000780) {
		bytes = 2;
	}
	else {
		bytes = 1;
	}

	if (bytes == 1) {
		buf[0] = (unsigned char)ch;
	}
	else if (bytes == 2) {
		buf[1] = (ch & 0x3F) | 0x80;
		ch >>= 6;
		buf[0] = (ch & 0x1F) | 0xC0;
	}
	else if (bytes == 3) {
		buf[2] = (ch & 0x3F) | 0x80;
		ch >>= 6;
		buf[1] = (ch & 0x3F) | 0x80;
		ch >>= 6;
		buf[0] = (ch & 0xF) | 0xE0;
	}
	else {
		buf[3] = (ch & 0x3F) | 0x80;
		ch >>= 6;
		buf[2] = (ch & 0x3F) | 0x80;
		ch >>= 6;
		buf[1] = (ch & 0x3F) | 0x80;
		ch >>= 6;
		buf[0] = (ch & 0x7) | 0xF0;
	}

	buf[bytes] = 0;

	return std::string((char *)buf);
}

std::string utf8_substr(std::string s, int start, int count)
{
	unsigned const char *p = (unsigned const char *)s.c_str();
	int begin = utf8_len_bytes(s, start);
	int end = utf8_len_bytes_ptr(p+begin, count);
	return std::string((char *)p + begin, end);
}

} // End namespace util

} // End namespace noo
