#ifndef NOO_JSON_H
#define NOO_JSON_H

#include "shim4/main.h"

namespace noo {

namespace util {

class Trigger;

class SHIM4_EXPORT JSON {
public:
	static std::string trim_quotes(std::string s);

	struct SHIM4_EXPORT Node {
		enum Type {
			NONE,
			ARRAY,
			HASH,
			INT,
			BOOL,
			FLOAT,
			STRING,
			BYTE
		};

		typedef std::map<std::string, Node *> NodeMap;
		typedef NodeMap::iterator NodeIt;

		std::string key;
		std::string value; // can be [array]
		std::vector<Node *> children;
		std::map<std::string, Node *> child_map;

		Type type;
		void *userdata;

		Node *parent;

		Trigger *trigger; // run() is called whenever the onscreen vars viewer changes this variable (after it has been changed)

		bool readonly; // devsettings can't modify it

		Node *find(std::string loc); // can search nested nodes by separating with >

		std::string to_json(int indent = 0); // get the whole tree as a string in JSON format

		std::string as_string(); // removes quotes if any
		// these do not trim quotes before conversion, so e.g. "74" will not be correct
		int as_int();
		bool as_bool();
		float as_float();
		Uint8 as_byte();

		// If userdata is set, these expect it to be the correct type. these also call trigger->run if trigger != NULL
		void set_string(std::string s);
		void set_int(int i);
		void set_bool(bool b);
		void set_float(float f);
		void set_byte(Uint8 b);

		void set_type(Type t);
		void set_userdata(void *u);

		// sets type, userdata and value
		void set_type_string(void *userdata, std::string s);
		void set_type_int(void *userdata, int i);
		void set_type_bool(void *userdata, bool b);
		void set_type_float(void *userdata, float f);
		void set_type_byte(void *userdata, Uint8 b);

		// returns value at loc of given type, sets type and userdata. If loc doesn't exist, it just returns the default value passed in and/or adds it if add is true
		std::string get_nested_string(std::string loc, void *userdata, std::string def, bool add = true, bool readonly = false);
		int get_nested_int(std::string loc, void *userdata, int def, bool add = true, bool readonly = false);
		bool get_nested_bool(std::string loc, void *userdata, bool def, bool add = true, bool readonly = false);
		float get_nested_float(std::string loc, void *userdata, float def, bool add = true, bool readonly = false);
		Uint8 get_nested_byte(std::string loc, void *userdata, Uint8 def, bool add = true, bool readonly = false);

		void add_nested_string(std::string loc, void *userdata, std::string val, Trigger *trigger = NULL, bool readonly = false);
		void add_nested_int(std::string loc, void *userdata, int val, Trigger *trigger = NULL, bool readonly = false);
		void add_nested_bool(std::string loc, void *userdata, bool val, Trigger *trigger = NULL, bool readonly = false);
		void add_nested_float(std::string loc, void *userdata, float val, Trigger *trigger = NULL, bool readonly = false);
		void add_nested_byte(std::string loc, void *userdata, Uint8 val, Trigger *trigger = NULL, bool readonly = false);

		bool remove_child(Node *child, bool del = true); // del = delete also

		void update_value(); // read value from userdata, store in value
	
	private:
		Node *add_child(Node *child);
		Node *add_child(std::string key, std::string value, Type type = NONE, void *userdata = NULL); // if string, add quotes to value before calling
		void add_nested(std::string loc, Node *add); // doesn't check if node exists, just adds it
	};

	JSON(std::string filename, bool load_from_filesystem = false);
	JSON(SDL_RWops *file);
	~JSON();

	Node *get_root();
	bool remove(std::string loc, bool del = true);

private:
	void read(std::string filename, bool load_from_filesystem = false);
	void read(SDL_RWops *file);
	int read_char(SDL_RWops *file);
	void unget(int c);
	void skip_whitespace(SDL_RWops *file);
	std::string read_token(SDL_RWops *file, bool is_string);
	std::string read_string(SDL_RWops *file);
	std::string read_value(SDL_RWops *file);
	void read_array(Node *node, SDL_RWops *file);
	void read_hash(Node *node, SDL_RWops *file);
	void parse_node(Node *node, SDL_RWops *file);
	void read(Node *node, SDL_RWops *file);
	void destroy(Node *node);

	Node *root;
	int ungot;
	int line;
};

} // End namespace util

} // End namespace noo

#endif // NOO_JSON_H
