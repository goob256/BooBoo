#include "shim4/tokenizer.h"

using namespace noo;

namespace noo {

namespace util {

Tokenizer::Tokenizer(std::string s, char delimiter, bool skip_bunches) :
	s(s),
	delimiter(delimiter),
	offset(0),
	skip_bunches(skip_bunches)
{
}

std::string Tokenizer::next()
{
	if (offset == s.length()) {
		return "";
	}

	size_t next_delim;

	size_t tmp_o = offset;

	while (true) {
		next_delim = s.find(delimiter, tmp_o);
		if ((next_delim != 0 && next_delim != std::string::npos && s[next_delim-1] == '\\') || (skip_bunches && next_delim+1 < s.length() && s[next_delim+1] == delimiter)) {
			tmp_o = next_delim + 1;
			continue;
		}
		break;
	}

	std::string ret;

	if (next_delim == std::string::npos) {
		ret = s.substr(offset);
		offset = s.length();
		return ret;
	}

	ret = s.substr(offset, next_delim-offset);
	offset = next_delim + 1;

	return ret;
}

} // End namespace util

} // End namespace noo
