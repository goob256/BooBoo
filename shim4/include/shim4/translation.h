#ifndef NOO_TRANSLATION_H
#define NOO_TRANSLATION_H

#include "shim4/main.h"

namespace noo {

namespace util {

class SHIM4_EXPORT Translation {
public:
	Translation(std::string text);

	std::string translate(int id);
	int get_id(std::string text);

	std::string get_entire_translation();

private:
	std::map<int, std::string> translation;
	std::map<std::string, int> id_map;
};

} // End namespace util

} // End namespace noo

#endif // NOO_TRANSLATION_H
