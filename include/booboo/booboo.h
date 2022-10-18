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
		NUMBER = 0,
		STRING,
		VECTOR
	} type;

	std::string name;

	double n;
	std::string s;
	std::vector<Variable> v;

	std::string function;
};

struct Label {
	std::string name;
	int pc;
};

struct Token {
	enum Token_Type {
		STRING = 0, // "string literal"
		SYMBOL, // alphanumeric and underscores like variable names
		SPECIAL, // special characters mostly
		NUMBER
	};

	Token_Type type;

	std::string s;
	double n;

	std::string token;
};

struct Statement {
	int method;
	std::vector<Token> data;
};

struct Program {
	std::string name;
	int compare_flag;
	
	std::string code;

	// This needs fixin'
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
	std::map<int, audio::MML *> mmls;
	std::map<int, gfx::Image *> images;
	std::map<int, gfx::TTF *> fonts;
	std::vector<int> line_numbers;

	std::vector<Statement> program;
	int pc;
};

typedef bool (*library_func)(Program &prg, std::vector<Token> &v);

extern std::string reset_game_name;
extern bool load_from_filesystem;
extern int return_code;

// These are the main functions
void start();
void end();
Program create_program(std::string code);
bool interpret(Program &prg);
void destroy_program(Program &prg);
void call_function(Program &prg, std::string function_name, std::vector<std::string> params, std::string result_name);

// These are for adding syntax
void add_syntax(std::string name, library_func func);
std::string token(Program &prg, Token::Token_Type &ret_type, bool add_lines = false);
int get_line_num(Program &prg);
Variable &find_variable(Program &prg, std::string name);
void set_string_or_number(Program &prg, std::string name, std::string value);
void skip_whitespace(Program &prg, bool add_lines = false);
std::string remove_quotes(std::string s);

} // end namespace booboo

#endif // BOOBOO_H
