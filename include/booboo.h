#ifndef BOOBOO_H
#define BOOBOO_H

#include <string>
#include <vector>
#include <map>

class EXCEPTION {
public:
	EXCEPTION(std::string error);

	std::string error;
};

class PARSE_EXCEPTION : public EXCEPTION
{
public:
	PARSE_EXCEPTION(std::string error);
};

struct VARIABLE
{
	enum VARIABLE_TYPE {
		NUMBER,
		STRING,
		VECTOR,
	} type;

	std::string name;

	double n;
	std::string s;

	std::string function;
};

struct LABEL {
	std::string name;
	int p;
	int line;
};

struct PROGRAM {
	std::string name;
	int compare_flag;
	
	std::string code;
	int p;
	int line;
	int start_line;

	int prev_tok_p;
	int prev_tok_line;
	
	std::map<std::string, VARIABLE> variables;
	std::vector< std::map<std::string, VARIABLE> > variables_backup_stack;
	std::vector<PROGRAM> functions;
	std::vector<LABEL> labels;

	std::vector<std::string> parameters;
	VARIABLE result;

	int mml_id;
	int image_id;
	int font_id;
	int vector_id;
	std::map<int, audio::MML *> mmls;
	std::map<int, gfx::Image *> images;
	std::map<int, gfx::TTF *> fonts;
	std::map< int, std::vector<VARIABLE> > vectors;
	std::vector<int> line_numbers;
};

typedef bool (*library_func)(PROGRAM &prg, std::string tok);

extern std::string reset_game_name;

void booboo_init();
void booboo_shutdown();
bool interpret(PROGRAM &prg);
void destroy_program(PROGRAM &prg, bool destroy_vectors);
void call_function(PROGRAM &prg, std::string function_name, std::string result_name);
void add_syntax(std::string name, library_func func);
std::vector<LABEL> process_labels(PROGRAM prg);
bool process_includes(PROGRAM &prg);
void process_functions(PROGRAM &prg);
std::string token(PROGRAM &prg, bool add_lines = false);
int get_line_num(PROGRAM &prg);
VARIABLE &find_variable(PROGRAM &prg, std::string name);
std::vector<double> variable_names_to_numbers(PROGRAM &prg, std::vector<std::string> strings);
void set_string_or_number(PROGRAM &prg, std::string name, std::string value);
void skip_whitespace(PROGRAM &prg, bool add_lines = false);
std::string remove_quotes(std::string s);
std::string unescape(std::string s);

#endif // BOOBOO_H
