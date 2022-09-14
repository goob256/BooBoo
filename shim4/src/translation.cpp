#include "shim4/translation.h"
#include "shim4/util.h"

using namespace noo;

namespace noo {

namespace util {

Translation::Translation(std::string text)
{
	SDL_RWops *file = SDL_RWFromMem((void *)text.c_str(), (int)text.length());

	if (file) {
		const int max = 5000;
		char line[max];

		while (SDL_fgets(file, line, max)) {
			if (line[strlen(line)-1] == '\r' || line[strlen(line)-1] == '\n') line[strlen(line)-1] = 0;
			if (line[strlen(line)-1] == '\r' || line[strlen(line)-1] == '\n') line[strlen(line)-1] = 0;
			const char *p = strchr(line, ':');
			if (p) {
				int num = atoi(line);
				p++;
				translation[num] = p;
				id_map[p] = num;
			}
		}
	}

	SDL_RWclose(file);
}


std::string Translation::translate(int id)
{
	std::map<int, std::string>::iterator it;
	if ((it = translation.find(id)) != translation.end()) {
		return (*it).second;
	}
	return "X";
}

int Translation::get_id(std::string text)
{
	std::map<std::string, int>::iterator it;
	if ((it = id_map.find(text)) != id_map.end()) {
		return (*it).second;
	}
	return -1;
}

std::string Translation::get_entire_translation()
{
	std::string s;

	for (std::map<int, std::string>::iterator it = translation.begin(); it != translation.end(); it++) {
		std::pair<int, std::string> p = *it;
		s += p.second;
	}

	return s;
}

} // End namespace util

} // End namespace noo
