#ifndef BOOBOO_H
#define BOOBOO_H

#include <string>
#include <vector>
#include <map>
#include <stack>

namespace booboo {

struct Variable
{
	enum Variable_Type {
		NUMBER,
		STRING,
		VECTOR,
		USER
	} type;

	std::string name;

	double n;
	std::string s;
	void *u;

	std::string function;
};

struct Label {
	std::string name;
	int p;
	int line;
};

struct Program {
	std::string name;
	int compare_flag;
	
	std::string code;
	int p;
	int line;
	int start_line;

	int prev_tok_p;
	int prev_tok_line;
	
	std::map<std::string, Variable> variables;
	std::stack< std::map<std::string, Variable> > variables_backup_stack;
	std::map<std::string, Program> functions;
	std::map<std::string, Label> labels;

	std::vector<std::string> parameters;
	Variable result;

	int mml_id;
	int image_id;
	int font_id;
	int vector_id;
	std::map<int, audio::MML *> mmls;
	std::map<int, gfx::Image *> images;
	std::map<int, gfx::TTF *> fonts;
	std::map< int, std::vector<Variable> > vectors;
	std::vector<int> line_numbers;
};

typedef bool (*library_func)(Program &prg, std::string tok);

extern std::string reset_game_name;
extern bool load_from_filesystem;
extern int return_code;

// These are the main functions
void start();
void end();
Program create_program(std::string code);
bool interpret(Program &prg);
void destroy_program(Program &prg);
void call_function(Program &prg, std::string function_name, std::string result_name);

// These are for adding syntax
void add_syntax(std::string name, library_func func);
std::string token(Program &prg, bool add_lines = false);
int get_line_num(Program &prg);
Variable &find_variable(Program &prg, std::string name);
std::vector<double> variable_names_to_numbers(Program &prg, std::vector<std::string> &strings);
void set_string_or_number(Program &prg, std::string name, std::string value);
void skip_whitespace(Program &prg, bool add_lines = false);
std::string remove_quotes(std::string s);
std::string unescape(std::string s);

} // end namespace booboo

#endif // BOOBOO_H
