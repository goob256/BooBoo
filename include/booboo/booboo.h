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
		VECTOR,
		LABEL,
		FUNCTION
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
	int i;

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
	unsigned int p;
	unsigned int line;
	unsigned int start_line;
	unsigned int prev_tok_p;
	unsigned int prev_tok_line;

	std::vector<Variable> variables;
	std::map<std::string, int> variables_map;
	std::vector<Program> functions;
	std::vector<int> params;

	Variable result;

	int mml_id;
	int image_id;
	int font_id;
	std::map<int, audio::MML *> mmls;
	std::map<int, gfx::Image *> images;
	std::map<int, gfx::TTF *> fonts;
	std::vector<int> line_numbers;

	std::vector<Statement> program;
	unsigned int pc;
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
void call_function(Program &prg, int function, std::vector<Token> params, Variable &result);
void call_function(Program &prg, std::string function, std::vector<Token> params, Variable &result);

// These are for adding syntax
void add_syntax(std::string name, library_func func);
std::string token(Program &prg, Token::Token_Type &ret_type, bool add_lines = false);
int get_line_num(Program &prg);
void set_string_or_number(Program &prg, std::string name, std::string value);
void skip_whitespace(Program &prg, bool add_lines = false);
std::string remove_quotes(std::string s);

} // end namespace booboo

#endif // BOOBOO_H
