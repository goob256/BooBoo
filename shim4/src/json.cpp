#ifdef TVOS
#include "shim4/ios.h"
#endif

#include "shim4/error.h"
#include "shim4/json.h"
#include "shim4/tokenizer.h"
#include "shim4/trigger.h"
#include "shim4/util.h"

using namespace noo;

namespace noo {

namespace util {

JSON::Node *JSON::Node::find(std::string loc)
{
	Tokenizer t(loc, '>');
	std::string k;
	Node *result = this;

	while ((k = t.next()) != "") {
		// FIXME: use quotes or no?
		NodeIt it = result->child_map.find(k);
		//NodeIt it = result->child_map.find(k);
		if (it == result->child_map.end()) {
			return NULL;
		}
		result = it->second;
	}

	return result;
}

std::string JSON::Node::to_json(int indent)
{
	update_value();

	std::string s(indent, '\t');

	if (key.length() >= 1 && key[0] != '[') {
		s += string_printf("\"%s\": ", key.c_str());
	}

	if (value == "[hash]" || value == "[array]") {
		if (value == "[hash]") {
			s += "{\n";
		}
		else {
			s += "[\n";
		}
		for (size_t i = 0; i < children.size(); i++) {
			s += children[i]->to_json(indent+1);
			if (i < children.size()-1) {
				s += ",";
			}
			s += "\n";
		}
		s += std::string(indent, '\t');
		if (value == "[hash]") {
			s += "}";
		}
		else {
			s += "]";
		}
	}
	else {
		s += value;
	}

	return s;
}

std::string JSON::Node::as_string()
{
	update_value();

	if (value.length() >= 2 && value[0] == '"') {
		return value.substr(1, value.length()-2);
	}

	return value;
}

int JSON::Node::as_int()
{
	update_value();

	return atoi(value.c_str());
}

bool JSON::Node::as_bool()
{
	update_value();

	return value == "true";
}

float JSON::Node::as_float()
{
	update_value();

	return (float)atof(value.c_str());
}

Uint8 JSON::Node::as_byte()
{
	update_value();

	return (Uint8)atoi(value.c_str());
}

void JSON::Node::set_string(std::string s)
{
	value = "\"" + s + "\"";
	if (userdata != NULL) {
		assert(type == NONE || type == STRING);

		std::string *u = static_cast<std::string *>(userdata);
		if (u) {
			*u = s;
		}
	}
	if (trigger != NULL) {
		trigger->run();
	}
}

void JSON::Node::set_int(int i)
{
	value = string_printf("%d", i);
	if (userdata != NULL) {
		assert(type == NONE || type == INT);
		
		int *u = static_cast<int *>(userdata);
		if (u) {
			*u = i;
		}
	}
	if (trigger != NULL) {
		trigger->run();
	}
}

void JSON::Node::set_bool(bool b)
{
	value = b ? "true" : "false";
	if (userdata != NULL) {
		assert(type == NONE || type == BOOL);
		
		bool *u = static_cast<bool *>(userdata);
		if (u) {
			*u = b;
		}
	}
	if (trigger != NULL) {
		trigger->run();
	}
}

void JSON::Node::set_float(float f)
{
	value = string_printf("%f", f);
	if (userdata != NULL) {
		assert(type == NONE || type == FLOAT);
		
		float *u = static_cast<float *>(userdata);
		if (u) {
			*u = f;
		}
	}
	if (trigger != NULL) {
		trigger->run();
	}
}

void JSON::Node::set_byte(Uint8 b)
{
	value = string_printf("%d", b);
	if (userdata != NULL) {
		assert(type == NONE || type == BYTE);
		
		Uint8 *u = static_cast<Uint8 *>(userdata);
		if (u) {
			*u = b;
		}
	}
	if (trigger != NULL) {
		trigger->run();
	}
}

void JSON::Node::set_type(Type t)
{
	type = t;
}

void JSON::Node::set_userdata(void *u)
{
	userdata = u;
}
		
void JSON::Node::set_type_string(void *userdata, std::string s)
{
	type = STRING;
	this->userdata = userdata;
	set_string(s);
}

void JSON::Node::set_type_int(void *userdata, int i)
{
	type = INT;
	this->userdata = userdata;
	set_int(i);
}

void JSON::Node::set_type_bool(void *userdata, bool b)
{
	type = BOOL;
	this->userdata = userdata;
	set_bool(b);
}

void JSON::Node::set_type_float(void *userdata, float f)
{
	type = FLOAT;
	this->userdata = userdata;
	set_float(f);
}

void JSON::Node::set_type_byte(void *userdata, Uint8 b)
{
	type = BYTE;
	this->userdata = userdata;
	set_byte(b);
}

std::string JSON::Node::get_nested_string(std::string loc, void *userdata, std::string def, bool add, bool readonly)
{
	Node *n = find(loc);
	if (n == NULL) {
		if (add) {
			add_nested_string(loc, userdata, def, NULL, readonly);
		}
		return def;
	}
	std::string ret = n->as_string();
	n->type = STRING;
	n->userdata = userdata;
	n->readonly = readonly;
	return ret;
}

int JSON::Node::get_nested_int(std::string loc, void *userdata, int def, bool add, bool readonly)
{
	Node *n = find(loc);
	if (n == NULL) {
		if (add) {
			add_nested_int(loc, userdata, def, NULL, readonly);
		}
		return def;
	}
	int ret = n->as_int();
	n->type = INT;
	n->userdata = userdata;
	n->readonly = readonly;
	return ret;
}

bool JSON::Node::get_nested_bool(std::string loc, void *userdata, bool def, bool add, bool readonly)
{
	Node *n = find(loc);
	if (n == NULL) {
		if (add) {
			add_nested_bool(loc, userdata, def, NULL, readonly);
		}
		return def;
	}
	bool ret = n->as_bool();
	n->type = BOOL;
	n->userdata = userdata;
	n->readonly = readonly;
	return ret;
}

float JSON::Node::get_nested_float(std::string loc, void *userdata, float def, bool add, bool readonly)
{
	Node *n = find(loc);
	if (n == NULL) {
		if (add) {
			add_nested_float(loc, userdata, def, NULL, readonly);
		}
		return def;
	}
	float ret = n->as_float();
	n->type = FLOAT;
	n->userdata = userdata;
	n->readonly = readonly;
	return ret;
}

Uint8 JSON::Node::get_nested_byte(std::string loc, void *userdata, Uint8 def, bool add, bool readonly)
{
	Node *n = find(loc);
	if (n == NULL) {
		if (add) {
			add_nested_byte(loc, userdata, def, NULL, readonly);
		}
		return def;
	}
	Uint8 ret = n->as_byte();
	n->type = BYTE;
	n->userdata = userdata;
	n->readonly = readonly;
	return ret;
}

JSON::Node *JSON::Node::add_child(Node *child)
{
	child->parent = this;
	children.push_back(child);
	child_map[child->key] = child;
	return child;
}

JSON::Node *JSON::Node::add_child(std::string key, std::string value, Type type, void *userdata)
{
	Node *n = new Node;
	n->key = key;
	n->value = value;
	n->type = type;
	n->userdata = userdata;
	n->trigger = NULL;
	n->readonly = false;
	add_child(n);
	return n;
}

void JSON::Node::add_nested(std::string loc, Node *add)
{
	Tokenizer t(loc, '>');
	std::string k, all;
	k = t.next();
	JSON::Node *n = this;

	do {
		all += k;
		if (all == loc) {
			n->add_child(add);
		}
		else {
			NodeIt it = n->child_map.find(k);
			if (it == n->child_map.end()) {
				n = n->add_child(k, "[hash]", HASH, NULL);
			}
			else {
				n = it->second;
			}
			all += ">";
		}
	} while ((k = t.next()) != "");
}

void JSON::Node::add_nested_string(std::string loc, void *userdata, std::string val, Trigger *trigger, bool readonly)
{
	Node *existing = find(loc);
	if (existing != NULL) {
		existing->set_type(STRING);
		existing->set_userdata(userdata);
		existing->set_string(val);
		existing->readonly = readonly;
		return;
	}

	std::string last;
	size_t pos = loc.rfind('>');
	if (pos == std::string::npos) {
		last = loc;
	}
	else {
		last = loc.substr(pos+1);
	}
	Node *add = new Node;
	add->key = last;
	add->value = "\"" + val + "\"";
	add->type = STRING;
	add->userdata = userdata;
	add->trigger = trigger;
	add->readonly = readonly;

	add_nested(loc, add);
}

void JSON::Node::add_nested_int(std::string loc, void *userdata, int val, Trigger *trigger, bool readonly)
{
	Node *existing = find(loc);
	if (existing != NULL) {
		existing->set_type(INT);
		existing->set_userdata(userdata);
		existing->set_int(val);
		existing->readonly = readonly;
		return;
	}

	std::string last;
	size_t pos = loc.rfind('>');
	if (pos == std::string::npos) {
		last = loc;
	}
	else {
		last = loc.substr(pos+1);
	}
	Node *add = new Node;
	add->key = last;
	add->value = util::string_printf("%d", val);
	add->type = INT;
	add->userdata = userdata;
	add->trigger = trigger;
	add->readonly = readonly;

	add_nested(loc, add);
}

void JSON::Node::add_nested_bool(std::string loc, void *userdata, bool val, Trigger *trigger, bool readonly)
{
	Node *existing = find(loc);
	if (existing != NULL) {
		existing->set_type(BOOL);
		existing->set_userdata(userdata);
		existing->set_bool(val);
		existing->readonly = readonly;
		return;
	}

	std::string last;
	size_t pos = loc.rfind('>');
	if (pos == std::string::npos) {
		last = loc;
	}
	else {
		last = loc.substr(pos+1);
	}
	Node *add = new Node;
	add->key = last;
	add->value = val ? "true" : "false";
	add->type = BOOL;
	add->userdata = userdata;
	add->trigger = trigger;
	add->readonly = readonly;

	add_nested(loc, add);
}

void JSON::Node::add_nested_float(std::string loc, void *userdata, float val, Trigger *trigger, bool readonly)
{
	Node *existing = find(loc);
	if (existing != NULL) {
		existing->set_type(FLOAT);
		existing->set_userdata(userdata);
		existing->set_float(val);
		existing->readonly = readonly;
		return;
	}

	std::string last;
	size_t pos = loc.rfind('>');
	if (pos == std::string::npos) {
		last = loc;
	}
	else {
		last = loc.substr(pos+1);
	}
	Node *add = new Node;
	add->key = last;
	add->value = util::string_printf("%f", val);
	add->type = FLOAT;
	add->userdata = userdata;
	add->trigger = trigger;
	add->readonly = readonly;

	add_nested(loc, add);
}

void JSON::Node::add_nested_byte(std::string loc, void *userdata, Uint8 val, Trigger *trigger, bool readonly)
{
	Node *existing = find(loc);
	if (existing != NULL) {
		existing->set_type(BYTE);
		existing->set_userdata(userdata);
		existing->set_byte(val);
		existing->readonly = readonly;
		return;
	}

	std::string last;
	size_t pos = loc.rfind('>');
	if (pos == std::string::npos) {
		last = loc;
	}
	else {
		last = loc.substr(pos+1);
	}
	Node *add = new Node;
	add->key = last;
	add->value = util::string_printf("%d", val);
	add->type = BYTE;
	add->userdata = userdata;
	add->trigger = trigger;
	add->readonly = readonly;

	add_nested(loc, add);
}

bool JSON::Node::remove_child(Node *child, bool del)
{
	for (NodeIt it = child_map.begin(); it != child_map.end(); it++) {
		if (it->second == child) {
			child_map.erase(it);
			break;
		}
	}

	bool found = false;

	for (size_t i = 0; i < children.size(); i++) {
		if (children[i] == child) {
			children.erase(children.begin()+i);
			found = true;
			break;
		}
	}

	if (del) {
		delete child;
	}

	return found;
}

void JSON::Node::update_value()
{
	if (userdata == NULL) {
		return;
	}

	switch (type) {
		case STRING:
			value = "\"" + *static_cast<std::string *>(userdata) + "\"";
			break;
		case INT:
			value = util::string_printf("%d", *static_cast<int *>(userdata));
			break;
		case BOOL:
			value = *static_cast<bool *>(userdata) == true ? "true" : "false";
			break;
		case FLOAT:
			value = util::string_printf("%f", *static_cast<float *>(userdata));
			break;
		case BYTE:
			value = util::string_printf("%d", *static_cast<Uint8 *>(userdata));
			break;
		default:
			break;
	}
}

//--

std::string JSON::trim_quotes(std::string s)
{
	if (s[0] == '"') {
		s = s.substr(1);
	}
	if (s[s.length()-1] == '"') {
		s = s.substr(0, s.length()-1);
	}
	return s;
}

JSON::JSON(std::string filename, bool load_from_filesystem) :
	root(NULL),
	ungot(-1),
	line(1)
{
	read(filename, load_from_filesystem);
}

JSON::JSON(SDL_RWops *file) :
	root(NULL),
	ungot(-1),
	line(1)
{
	read(file);
}

JSON::~JSON()
{
	destroy(root);
}

void JSON::destroy(Node *node)
{
	for (size_t i = 0; i < node->children.size(); i++) {
		destroy(node->children[i]);
	}
	delete node;
}

void JSON::read(std::string filename, bool load_from_filesystem)
{
	int sz;
	char *bytes;

#ifdef TVOS
	std::string s;
#endif
    
	if (load_from_filesystem) {
#ifdef TVOS
		if (tvos_read_file(filename, s) == false) {
			throw util::FileNotFoundError(filename + " could not be loaded from NSUserDefaults");
		}
		sz = (int)s.length();
		bytes = (char *)s.c_str();
#else
		bytes = slurp_file_from_filesystem(filename, &sz);
#endif
	}
	else {
		bytes = slurp_file(filename, &sz);
	}

	SDL_RWops *file = SDL_RWFromMem(bytes, sz);

	read(file);

	SDL_RWclose(file);

#ifndef TVOS
	delete[] bytes;
#endif
}

void JSON::read(SDL_RWops *file)
{
	skip_whitespace(file);

	int c = read_char(file);

	if (c == EOF) {
		throw util::LoadError("Premature end of file");
	}
	else if (c != '{' && c != '[') {
		throw util::LoadError(util::string_printf("Unexpected symbol on line %d", line));
	}

	root = new Node;

	root->parent = NULL;

	root->type = Node::NONE;
	root->userdata = NULL;

	root->key = "[root]";

	if (c == '{') {
		root->value = "[hash]";
		read_hash(root, file);
	}
	else {
		root->value = "[array]";
		read_array(root, file);
	}

	root->trigger = NULL;
	root->readonly = false;
}

JSON::Node *JSON::get_root()
{
	return root;
}

bool JSON::remove(std::string loc, bool del)
{
	if (root == NULL) {
		return false;
	}

	Node *n = root->find(loc);

	if (n == NULL) {
		return false;
	}

	Node *parent = n->parent;

	parent->remove_child(n, del);

	return true;
}

int JSON::read_char(SDL_RWops *file)
{
	if (ungot != -1) {
		int ret = ungot;
		ungot = -1;
		return ret;
	}

	int c = SDL_fgetc(file);
	if (c == '\n') {
		line++;
	}
	return c;
}

void JSON::unget(int c)
{
	ungot = c;
}

void JSON::skip_whitespace(SDL_RWops *file)
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

	assert(NULL && "Error skipping whitespace");
}

std::string JSON::read_token(SDL_RWops *file, bool is_string)
{
	std::string token;
	int prev = -1;

	while (true) {
		int c = read_char(file);

		if (c == EOF) {
			throw util::LoadError("Premature end of file reading token");
		}

		if (is_string) {
			if (c == '"') {
				if (prev == '\\') {
					token = token.substr(0, token.length()-1);
					token += "\"";
				}
				else {
					break;
				}
			}
		}
		else {
			if (c == ',' || c == '}' || c == ']') {
				unget(c);
				break;
			}
		}

		if (!isspace(c) || is_string) {
			char s[2];
			s[0] = c;
			s[1] = 0;

			token += s;
		}

		prev = c;
	}

	return token;
}

std::string JSON::read_string(SDL_RWops *file)
{
	return "\"" + read_token(file, true) + "\"";
}

std::string JSON::read_value(SDL_RWops *file)
{
	return read_token(file, false);
}

void JSON::parse_node(Node *node, SDL_RWops *file)
{
	skip_whitespace(file);

	int c = read_char(file);

	if (c == EOF) {
		throw util::LoadError("Premature end of file");
	}
	else if (c == '{') {
		node->value = "[hash]";
	}
	else if (c == '[') {
		node->value = "[array]";
	}
	else if (c == '"') {
		node->value = read_string(file);
	}
	else {
		unget(c);
		node->value = read_value(file);
	}

	if (node->value == "[hash]") {
		read_hash(node, file);
	}
	else if (node->value == "[array]") {
		read_array(node, file);
	}
}

void JSON::read_array(Node *node, SDL_RWops *file)
{
	for (int count = 0;;) {
		skip_whitespace(file);

		int c = read_char(file);

		if (c == EOF) {
			throw util::LoadError("Premature end of file reading array");
		}
		else if (c == ']') {
			return;
		}
		else if (c == ',') {
			continue;
		}

		unget(c);

		Node *json = new Node;

		json->parent = node;

		json->type = Node::NONE;
		json->userdata = NULL;

		json->key = "[" + itos(count) + "]";

		json->trigger = NULL;
		json->readonly = false;

		parse_node(json, file);
		
		node->children.push_back(json);

		node->child_map[json->key] = json;

		count++;
	}

	assert(NULL && "Unknown error reading array\n");
}

void JSON::read_hash(Node *node, SDL_RWops *file)
{
	while (true) {
		skip_whitespace(file);

		int c = read_char(file);

		if (c == EOF) {
			throw util::LoadError("Premature end of file");
		}
		else if (c == '}') {
			return;
		}
		else if (c == ',') {
			continue;
		}
		else if (c != '"') {
			throw util::LoadError(util::string_printf("Parse error on line %d, expected } or \"", line));
		}

		Node *json = new Node;

		json->parent = node;
		
		json->type = Node::NONE;
		json->userdata = NULL;

		json->key = trim_quotes(read_string(file));

		json->trigger = NULL;
		json->readonly = false;

		skip_whitespace(file);

		if (read_char(file) != ':') {
			throw util::LoadError(util::string_printf(": expected on line %d", line));
		}

		parse_node(json, file);

		node->children.push_back(json);
		
		node->child_map[json->key] = json;
	}
}

} // End namespace util

} // End namespace noo
