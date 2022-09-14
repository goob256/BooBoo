#ifndef NOO_XML_H
#define NOO_XML_H

#include "shim4/main.h"

namespace noo {

namespace util {

class SHIM4_EXPORT XML {
public:
	struct SHIM4_EXPORT Node {
	public:
		std::string name;
		std::string value; // can be [array]

		std::vector<Node *> children;
		std::vector< std::pair<std::string, std::string> > attributes;

		Node *find(std::string name);
		std::string get_attribute(std::string name);
	};

	XML(std::string filename);
	XML(char *bytes, int size);
	~XML();

	Node *get_root();

private:
	void read(std::string filename);
	void read(char *bytes, int size);
	int read_char(SDL_RWops *file);
	void read_tag(Node *node, SDL_RWops *file);
	void unget(int c);
	void skip_whitespace(SDL_RWops *file);
	void destroy(Node *node);

	Node *root;
	std::vector<int> ungot;
	int line;
};

} // End namespace util

} // End namespace noo

#endif // NOO_XML_H
