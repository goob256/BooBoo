#include "shim4/error.h"
#include "shim4/util.h"
#include "shim4/xml.h"

using namespace noo;

namespace noo {

namespace util {

XML::Node *XML::Node::find(std::string name)
{
	for (size_t i = 0; i < children.size(); i++) {
		if (children[i]->name == name) {
			return children[i];
		}
	}

	return 0;
}

std::string XML::Node::get_attribute(std::string name)
{
	for (size_t i = 0; i < attributes.size(); i++) {
		std::pair<std::string, std::string> &p = attributes[i];
		if (p.first == name) {
			return p.second;
		}
	}

	return "";
}

XML::XML(std::string filename) :
	line(1)
{
	read(filename);
}

XML::XML(char *bytes, int size) :
	line(1)
{
	read(bytes, size);
}

XML::~XML()
{
	destroy(root);
}

void XML::destroy(Node *node)
{
	for (size_t i = 0; i < node->children.size(); i++) {
		destroy(node->children[i]);
	}
	delete node;
}

void XML::read(char *bytes, int sz)
{
	SDL_RWops *file = SDL_RWFromMem(bytes, sz);

	skip_whitespace(file);

	// skip <?xml tag

	int c = read_char(file);
	int c2 = read_char(file);

	if (c == '<' && c2 == '?') {
		while (true) {
			c = read_char(file);
			if (c == '>') {
				break;
			}
		}
	}
	else {
		unget(c2);
		unget(c);
	}

	skip_whitespace(file);

	root = new Node;

	try {
		read_tag(root, file);
	}
	catch (util::Error e) {
		SDL_RWclose(file);
		throw e;
	}

	SDL_RWclose(file);
}

void XML::read(std::string filename)
{
	int sz;
	char *bytes = slurp_file(filename, &sz);

	try {
		read(bytes, sz);
	}
	catch (util::Error e) {
		delete[] bytes;
		throw e;
	}

	delete[] bytes;
}

XML::Node *XML::get_root()
{
	return root;
}

int XML::read_char(SDL_RWops *file)
{
	if (ungot.size() > 0) {
		int ret = ungot[ungot.size()-1];
		ungot.pop_back();
		return ret;
	}

	int c = SDL_fgetc(file);
	if (c == '\n') {
		line++;
	}
	return c;
}

void XML::read_tag(Node *node, SDL_RWops *file)
{
	int c = read_char(file);

	if (c == EOF) {
		if (node != root) {
			throw util::LoadError(util::string_printf("Unexpected EOF on line %d", line));
		}
		return;
	}

	if (c != '<') {
		throw util::LoadError(util::string_printf("Expected < on line %d", line));
	}

	skip_whitespace(file);

	// open tag

	while (true) {
		c = read_char(file);
		if (isspace(c)) {
			break;
		}
		else if (c == '>') {
			unget(c);
			break;
		}
		else if (c == '/') {
			c = read_char(file);
			if (c == '>') {
				return;
			}
			else {
				unget(c);
			}
		}
		char s[2];
		s[0] = c;
		s[1] = 0;
		node->name += s;

		if (node->name == "/") {
			throw util::LoadError(util::string_printf("Unexpected end tag on line %d", line));
		}
	}

	skip_whitespace(file);

	// attributes

	std::string attr_name;
	std::string attr_value;
	bool attr_open_single = false;
	bool attr_open_double = false;
	bool reading_name = true;

	while (true) {
		c = read_char(file);

		if (c == EOF) {
			throw util::LoadError(util::string_printf("Unexpected EOF on line %d", line));
		}

		if (c == '>') {
			if (attr_name != "") {
				node->attributes.push_back(std::pair<std::string, std::string>(attr_name, attr_value));
			}
			break;
		}
		else if (c == '/') {
			c = read_char(file);
			if (c == '>') {
				return;
			}
			else {
				unget(c);
			}
		}
		else if (reading_name && c == '=') {
			reading_name = false;
			c = read_char(file);
			if (c == EOF) {
				throw util::LoadError(util::string_printf("Unexpected EOF on line %d", line));
			}
			if (c == '\'') {
				attr_open_single = true;
			}
			else if (c == '"') {
				attr_open_double = true;
			}
			else {
				unget(c);
			}
			continue;
		}
		else if (
			(reading_name == false && attr_open_single && c == '\'') ||
			(reading_name == false && attr_open_double && c == '"') ||
			(reading_name == false && attr_open_single == false && attr_open_double == false && isspace(c))
		) {
			node->attributes.push_back(std::pair<std::string, std::string>(attr_name, attr_value));
			attr_open_single = false;
			attr_open_double = false;
			reading_name = true;
			attr_name = "";
			attr_value = "";
			skip_whitespace(file);
			continue;
		}

		char s[2];
		s[0] = c;
		s[1] = 0;

		if (reading_name) {
			attr_name += s;
		}
		else {
			attr_value += s;
		}
	}

	// data and end tag

	std::string data;

	while (true) {
		c = read_char(file);

		if (c == EOF) {
			throw util::LoadError(util::string_printf("Unexpected EOF on line %d", line));
		}

		if (c == '<') {
			c = read_char(file);
			if (c == EOF) {
				throw util::LoadError(util::string_printf("Unexpected EOF on line %d", line));
			}
			if (c == '/') {
				while (true) {
					c = read_char(file);
					if (c == EOF) {
						throw util::LoadError(util::string_printf("Unexpected EOF on line %d", line));
					}
					if (c == '>') {
						break;
					}
				}
				break;
			}
			else {
				unget(c);
				unget('<');
				Node *xml = new Node;
				read_tag(xml, file);
				node->children.push_back(xml);
			}
		}
		else {
			char s[2];
			s[0] = c;
			s[1] = 0;
			data += s;
		}
	}

	node->value = data;
}

void XML::unget(int c)
{
	ungot.push_back(c);
}

void XML::skip_whitespace(SDL_RWops *file)
{
	while (true) {
		int c = read_char(file);
		if (c == EOF) {
			return;
		}
		else if (!isspace(c)) {
			unget(c);
			return;
		}
	}

	assert(0 && "Error skipping whitespace");
}

} // End namespace util

} // End namespace noo
