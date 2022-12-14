//#define DEBUG_LINE_NUMBERS

#include <cctype>

#include <shim4/shim4.h>

#include "booboo/booboo.h"

namespace booboo {

typedef std::string (*token_func)(Program &);

std::map<std::string, int> library_map;
std::vector<library_func> library;
std::map<char, token_func> token_map;
std::string reset_game_name;
std::string main_program_name;
bool load_from_filesystem;
int return_code;

void skip_whitespace(Program &prg)
{
	while (prg.p < prg.code.length() && isspace(prg.code[prg.p])) {
		if (prg.code[prg.p] == '\n') {
			prg.line++;
		}
		prg.p++;
	}
}

std::string remove_quotes(std::string s)
{
	int start = 0;
	int count = s.length();

	if (s[0] == '"') {
		start++;
		count--;
	}

	if (s[s.length()-1] == '"') {
		count--;
	}

	return s.substr(start, count);
}

std::string get_file_name(Program &prg)
{
	if (prg.line_numbers.size() <= prg.pc) {
		return "UNKNOWN";
	}

	int l = prg.line_numbers[prg.pc];

	if (prg.real_file_names.size() <= l) {
		return "UNKNOWN";
	}

	return prg.real_file_names[prg.line_numbers[prg.pc]];
}

int get_line_num(Program &prg)
{
	int l;

	l = prg.pc;

	if (prg.line_numbers.size() <= l) {
		if (prg.line_numbers.size() > 0) {
			return prg.line_numbers[prg.line_numbers.size()-1];
		}
		else {
			return 1;
		}
	}

	l = prg.line_numbers[l];

	if (prg.real_line_numbers.size() <= l) {
		if (prg.real_line_numbers.size() > 0) {
			return prg.real_line_numbers[prg.real_line_numbers.size()-1];
		}
		else {
			return 1;
		}
	}

	return prg.real_line_numbers[prg.line_numbers[prg.pc]];
}

std::string get_error_info(Program &prg)
{
	return get_file_name(prg) + ":" + util::itos(get_line_num(prg));
}

static std::string tokenfunc_add(booboo::Program &prg)
{
	prg.p++;
	return "+";
}

static std::string tokenfunc_callset(booboo::Program &prg)
{
	prg.p++;
	return ">";
}

static std::string tokenfunc_subtract(booboo::Program &prg)
{
	char s[2];
	s[1] = 0;

	prg.p++;
	if (prg.p < prg.code.length() && isdigit(prg.code[prg.p])) {
		std::string tok = "-";
		while (prg.p < prg.code.length() && (isdigit(prg.code[prg.p]) || prg.code[prg.p] == '.')) {
			s[0] = prg.code[prg.p];
			tok += s;
			prg.p++;
		}
		return tok;
	}
	else {
		return "-";
	}
}

static std::string tokenfunc_equals(booboo::Program &prg)
{
	prg.p++;
	return "=";
}

static std::string tokenfunc_compare(booboo::Program &prg)
{
	prg.p++;
	return "?";
}

static std::string tokenfunc_multiply(booboo::Program &prg)
{
	prg.p++;
	return "*";
}

static std::string tokenfunc_divide(booboo::Program &prg)
{
	prg.p++;
	return "/";
}

static std::string tokenfunc_label(booboo::Program &prg)
{
	prg.p++;
	return ":";
}

static std::string tokenfunc_modulus(booboo::Program &prg)
{
	prg.p++;
	return "%";
}

static std::string tokenfunc_string(booboo::Program &prg)
{
	char s[2];
	s[1] = 0;

	int prev = -1;
	int prev_prev = -1;

	std::string tok = "\"";
	prg.p++;

	if (prg.p < prg.code.length()) {
		while (prg.p < prg.code.length() && (prg.code[prg.p] != '"' || (prev == '\\' && prev_prev != '\\')) && prg.code[prg.p] != '\n') {
			s[0] = prg.code[prg.p];
			tok += s;
			prev_prev = prev;
			prev = prg.code[prg.p];
			prg.p++;
		}

		tok += "\"";

		prg.p++;
	}

	return tok;
}

static std::string tokenfunc_openbrace(booboo::Program &prg)
{
	prg.p++;
	return "{";
}

static std::string tokenfunc_closebrace(booboo::Program &prg)
{
	prg.p++;
	return "}";
}

static std::string tokenfunc_comment(booboo::Program &prg)
{
	prg.p++;
	return ";";
}

std::string token(Program &prg, Token::Token_Type &ret_type)
{
	prg.prev_tok_p = prg.p;
	prg.prev_tok_line = prg.line;

	skip_whitespace(prg);

	if (prg.p >= prg.code.length()) {
		return "";
	}

	std::string tok;
	char s[2];
	s[1] = 0;

	if ((prg.p < prg.code.length()-1 && prg.code[prg.p] == '-' && isdigit(prg.code[prg.p+1])) || isdigit(prg.code[prg.p])) {
		while (prg.p < prg.code.length() && (isdigit(prg.code[prg.p]) || prg.code[prg.p] == '.' || prg.code[prg.p] == '-')) {
			s[0] = prg.code[prg.p];
			tok += s;
			prg.p++;
		}
		ret_type = Token::NUMBER;
		return tok;
	}
	else if (isalpha(prg.code[prg.p]) || prg.code[prg.p] == '_') {
		while (prg.p < prg.code.length() && (isdigit(prg.code[prg.p]) || isalpha(prg.code[prg.p]) || prg.code[prg.p] == '_')) {
			s[0] = prg.code[prg.p];
			tok += s;
			prg.p++;
		}
		ret_type = Token::SYMBOL;
		return tok;
	}

	if (prg.code[prg.p] == '"') {
		ret_type = Token::STRING;
	}
	else {
		ret_type = Token::SPECIAL;
	}

	std::map<char, token_func>::iterator it = token_map.find(prg.code[prg.p]);
	if (it != token_map.end()) {
		return (*it).second(prg);
	}

	prg.line_numbers.push_back(prg.line); // can help give better line number
	throw util::ParseError(std::string(__FUNCTION__) + ": " + "Parse error at " + get_error_info(prg) + " (pc=" + util::itos(prg.p) + ", tok=\"" + tok + "\")");

	return "";
}

bool process_includes(Program &prg)
{
	bool ret = false;

	int total_added = 0;

	std::string code;

	std::string tok;

	prg.p = 0;
	prg.line = 1;
	prg.prev_tok_p = 0;
	prg.prev_tok_line = 1;
	prg.line_numbers.clear();

	int prev = prg.p;
	int start = 0;

	Token::Token_Type tt;

	while ((tok = token(prg, tt)) != "") {
		if (tok == ";") {
			while (prg.p < prg.code.length() && prg.code[prg.p] != '\n') {
				prg.p++;
			}
			prg.line++;
			if (prg.p < prg.code.length()) {
				prg.p++;
			}
		}
		else if (tok == "include") {
			std::string name = token(prg, tt);

			int start_line = prg.line;

			if (name == "") {
				throw util::ParseError(std::string(__FUNCTION__) + ": " + "Expected include parameters at " + get_error_info(prg));
			}

			if (name[0] != '"') {
				throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid include name at " + get_error_info(prg));
			}

			name = remove_quotes(util::unescape_string(name));

			code += prg.code.substr(start, prev-start);

			std::string new_code;
			std::string fn;
			if (load_from_filesystem) {
				fn = name;
				new_code = util::load_text_from_filesystem(name);
			}
			else {
				fn = "code/" + name;
				new_code = util::load_text(std::string("code/") + name);
			}

			new_code = util::trim(new_code);

			int nlines = 1;
			int i = 0;
			while (new_code[i] != 0) {
				if (new_code[i] == '\n') {
					nlines++;
				}
				i++;
			}

			code += "\n" + new_code;
			
			prg.real_line_numbers[start_line-1+total_added] = 1;
			prg.real_file_names[start_line-1+total_added] = fn;
			for (int i = 1; i < nlines; i++) {
				prg.real_line_numbers.insert(prg.real_line_numbers.begin()+start_line+(i-1+total_added), i+1);
				prg.real_file_names.insert(prg.real_file_names.begin()+start_line+(i-1+total_added), fn);
			}

			//for (int i = start_line+nlines; i < prg.real_line_numbers.size(); i++) {
				//prg.real_line_numbers[i] += nlines-1;
			//}

			start = prg.p;

			total_added += nlines;

			ret = true;
		}

		prev = prg.p;
	}

	code += prg.code.substr(start, prg.code.length()-start);

	prg.code = code;
	prg.p = 0;
	prg.line = 1;
	prg.prev_tok_p = 0;
	prg.prev_tok_line = 1;

	return ret;
}

enum Pass {
	PASS1,
	PASS2
};

static void compile(Program &prg, Pass pass)
{
	int p_bak = prg.p;
	int line_bak = prg.line;

	std::string tok;
	Token::Token_Type tt;

	int var_i = 0;
	int func_i = 0;

	while ((tok = token(prg, tt)) != "") {
		if (tok == ";") {
			while (prg.p < prg.code.length() && prg.code[prg.p] != '\n') {
				prg.p++;
			}
			prg.line++;
			if (prg.p < prg.code.length()) {
				prg.p++;
			}
		}
		else if (tok == "function") {
			std::string func_name = token(prg, tt);

			Variable v;
			v.name = func_name;
			v.type = Variable::FUNCTION;
			v.n = func_i++;
			v.function = prg.name;
			prg.variables_map[func_name] = var_i++;
			if (pass == PASS1) {
				prg.variables.push_back(v);
			}

			Program func;
			func.name = func_name;
			func.pc = 0;
			func.real_line_numbers = prg.real_line_numbers;
			func.real_file_names = prg.real_file_names;
			bool is_param = true;
			bool finished = false;
			std::map<std::string, int> backup;
			//std::vector<std::string> new_vars;
			while ((tok = token(prg, tt)) != "") {
				if (tok == ";") {
					while (prg.p < prg.code.length() && prg.code[prg.p] != '\n') {
						prg.p++;
					}
					prg.line++;
					if (prg.p < prg.code.length()) {
						prg.p++;
					}
				}
				else if (tok == "{") {
					is_param = false;
				}
				else if (tok == "}") {
					finished = true;
					break;
				}
				else if (tok == ":") {
					std::string tok2 = token(prg, tt);
					Variable v;
					v.name = tok2;
					v.function = func.name;
					v.type = Variable::LABEL;
					v.n = func.program.size();
					if (prg.variables_map.find(tok2) != prg.variables_map.end()) {
						if (pass == PASS1) {
							util::infomsg("duplicate label %s\n", tok2.c_str());
						}
						backup[tok2] = prg.variables_map[tok2];
					}
					/*
					else {
						new_vars.push_back(tok2);
					}
					*/

					prg.variables_map[tok2] = var_i++;
					if (pass == PASS1) {
						prg.variables.push_back(v);
					}
				}
				else if (tok == "number" || tok == "string" || tok == "vector") {
					std::string tok2 = token(prg, tt);
					Statement s;
					s.method = library_map[tok];
					func.program.push_back(s);
					if (pass == PASS2) {
						if (func.line_numbers.size() != 0) {
							func.pc++;
						}
						func.line_numbers.push_back(prg.line);
					}
					//if (std::find(new_vars.begin(), new_vars.end(), tok2) == new_vars.end()) {
						if (prg.variables_map.find(tok2) != prg.variables_map.end()) {
							backup[tok2] = prg.variables_map[tok2];
						}
						/*
						else {
							new_vars.push_back(tok2);
						}
						*/
						prg.variables_map[tok2] = var_i++;
						Variable v;
						v.name = tok2;
						if (tok == "number") {
							v.type = Variable::NUMBER;
						}
						else if (tok == "string") {
							v.type = Variable::STRING;
						}
						else {
							v.type = Variable::VECTOR;
						}
						if (pass == PASS1) {
							prg.variables.push_back(v);
						}
					//}
					Token t;
					t.type = Token::SYMBOL;
					t.i = prg.variables_map[tok2];
					t.s = tok2;
					t.token = tok2;
					func.program[func.program.size()-1].data.push_back(t);
				}
				else if (library_map.find(tok) != library_map.end()) {
					Statement s;
					s.method = library_map[tok];
					func.program.push_back(s);
					if (pass == PASS2) {
						if (func.line_numbers.size() != 0) {
							func.pc++;
						}
						func.line_numbers.push_back(prg.line);
					}
				}
				else if (is_param) {
					if (prg.variables_map.find(tok) != prg.variables_map.end()) {
						backup[tok] = prg.variables_map[tok];
					}
					/*
					else {
						new_vars.push_back(tok);
					}
					*/
					int param_i = var_i++;
					prg.variables_map[tok] = param_i;
					Variable v;
					v.name = tok;
					v.function = func_name;
					if (pass == PASS1) {
						prg.variables.push_back(v);
					}
					func.params.push_back(param_i);
				}
				else {
					if (func.program.size() == 0) {
						func.line_numbers.push_back(prg.line); // can help give better line number
						throw util::ParseError("Expected keyword at " + get_error_info(func));
					}
					Token t;
					t.token = tok;
					t.type = tt;
					switch (tt) {
						case Token::STRING:
						case Token::SPECIAL:
							t.s = remove_quotes(util::unescape_string(tok));
							break;
						case Token::SYMBOL:
							t.s = remove_quotes(util::unescape_string(tok));
							if (pass == PASS2 && prg.variables_map.find(t.s) == prg.variables_map.end()) {
								func.line_numbers.push_back(prg.line); // can help give better line number
								throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid variable name " + tok + " at " + get_error_info(func));
							}
							if (pass == PASS2) {
								t.i = prg.variables_map[t.s];
							}
							break;
						case Token::NUMBER:
							t.n = atof(tok.c_str());
							break;
					}
					func.program[func.program.size()-1].data.push_back(t);
				}
			}

			if (is_param == true) {
				func.line_numbers.push_back(prg.line); // can help give better line number
				throw util::ParseError(std::string(__FUNCTION__) + ": " + "Missing { at " + get_error_info(func));
			}

			if (finished == false) {
				func.line_numbers.push_back(prg.line); // can help give better line number
				throw util::ParseError(std::string(__FUNCTION__) + ": " + "Missing } at " + get_error_info(func));
			}

			prg.functions.push_back(func);
					
			std::map<std::string, int>::iterator it;
			for (it = backup.begin(); it != backup.end(); it++) {
				prg.variables_map[(*it).first] = (*it).second;
			}
		}
		else if (tok == ":") {
			std::string tok2 = token(prg, tt);
			Variable v;
			v.name = tok2;
			v.function = prg.name;
			v.type = Variable::LABEL;
			v.n = prg.program.size();
			if (prg.variables_map.find(tok2) != prg.variables_map.end()) {
				if (pass == PASS1) {
					util::infomsg("duplicate label %s\n", tok2.c_str());
				}
			}

			prg.variables_map[tok2] = var_i++;
			if (pass == PASS1) {
				prg.variables.push_back(v);
			}
		}
		else if (tok == "number" || tok == "string" || tok == "vector") {
			std::string tok2 = token(prg, tt);
			Statement s;
			s.method = library_map[tok];
			prg.program.push_back(s);
			if (pass == PASS2) {
				if (prg.line_numbers.size() != 0) {
					prg.pc++;
				}
				prg.line_numbers.push_back(prg.line);
			}
			prg.variables_map[tok2] = var_i++;
			Variable v;
			v.name = tok2;
			if (tok == "number") {
				v.type = Variable::NUMBER;
			}
			else if (tok == "string") {
				v.type = Variable::STRING;
			}
			else {
				v.type = Variable::VECTOR;
			}
			if (pass == PASS1) {
				prg.variables.push_back(v);
			}
			Token t;
			t.type = Token::SYMBOL;
			t.i = prg.variables_map[tok2];
			t.s = tok2;
			t.token = tok2;
			prg.program[prg.program.size()-1].data.push_back(t);
		}
		else if (library_map.find(tok) != library_map.end()) {
			Statement s;
			s.method = library_map[tok];
			prg.program.push_back(s);
			if (pass == PASS2) {
				if (prg.line_numbers.size() != 0) {
					prg.pc++;
				}
				prg.line_numbers.push_back(prg.line);
			}
		}
		else if (prg.program.size() == 0) {
			prg.line_numbers.push_back(prg.line); // can help give better line number
			throw util::ParseError("Expected keyword at " + get_error_info(prg));
		}
		else {
			Token t;
			t.token = tok;
			t.type = tt;
			switch (tt) {
				case Token::STRING:
				case Token::SPECIAL:
					t.s = remove_quotes(util::unescape_string(tok));
					break;
				case Token::SYMBOL:
					t.s = remove_quotes(util::unescape_string(tok));
					if (pass == PASS2 && prg.variables_map.find(t.s) == prg.variables_map.end()) {
						throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid variable name " + tok + " at " + get_error_info(prg));
					}
					if (pass == PASS2) {
						t.i = prg.variables_map[t.s];
					}
					break;
				case Token::NUMBER:
					t.n = atof(tok.c_str());
					break;
			}
			prg.program[prg.program.size()-1].data.push_back(t);
		}
	}

	prg.p = p_bak;
	prg.line = line_bak;
}

void call_function(Program &prg, int function, std::vector<Token> params, Variable &result)
{
	Program &func = prg.functions[function];

	std::map<std::string, int> stack;

	for (size_t j = 0; j < func.params.size(); j++) {
		Token param = params[j];
		
		Variable var;

		if (param.type == Token::NUMBER) {
			var.type = Variable::NUMBER;
			var.n = param.n;
			var.name = param.token;
		}
		else if (param.type == Token::STRING || param.type == Token::SPECIAL) {
			var.type = Variable::STRING;
			var.s = param.s;
			var.name = param.token;
		}
		else {
			Variable &v1 = prg.variables[param.i];
			var = v1;
			//var.name = func.parameters[j];
		}
		
		var.function = func.name;

		prg.variables[func.params[j]] = var;
	}

	std::string code_bak = prg.code;
	int p_bak = prg.p;
	int line_bak = prg.line;
	int start_line_bak = prg.start_line;
	Variable result_bak = prg.result;
	std::string name_bak = prg.name;
	std::vector<Statement> program_bak = prg.program;
	unsigned int pc_bak = prg.pc;

	prg.p = 0;
	prg.line = 1;
	prg.prev_tok_p = 0;
	prg.prev_tok_line = 1;
	prg.code = func.code;
	prg.start_line = func.start_line;
	prg.name = func.name;
	prg.program = func.program;
	prg.pc = 0;

	//while (process_includes(prg));
	//prg.labels = process_labels(prg);
	//process_functions(prg);

	while (interpret(prg)) {
	}

	prg.code = code_bak;
	prg.p = p_bak;
	prg.line = line_bak;
	prg.start_line = start_line_bak;
	prg.name = name_bak;
	prg.program = program_bak;
	prg.pc = pc_bak;

	std::string bak = result.name;
	std::string bak2 = result.function;
	result = prg.result;
	result.name = bak;
	result.function = bak2;

	prg.result = result_bak;
}

void call_function(Program &prg, std::string function_name, std::vector<Token> params, Variable &result)
{
	for (size_t i = 0; i < prg.functions.size(); i++) {
		if (prg.functions[i].name == function_name) {
			call_function(prg, i, params, result);
			return;
		}
	}
}

bool interpret(Program &prg)
{
	bool ret = true;

	if (prg.pc >= prg.program.size()) {
		return false;
	}

	Statement &s = prg.program[prg.pc];

	unsigned int pc_bak = prg.pc;

	library_func func = library[s.method];
	ret = func(prg, s.data);

	if (pc_bak == prg.pc) {
		prg.pc++;
	}

	return ret;
}

void destroy_program(Program &prg)
{
	for (std::map<int, audio::MML *>::iterator it =  prg.mmls.begin(); it != prg.mmls.end(); it++) {
		audio::MML *mml = (*it).second;
		delete mml;
	}
	for (std::map<int, gfx::Image *>::iterator it =  prg.images.begin(); it != prg.images.end(); it++) {
		gfx::Image *image = (*it).second;
		delete image;
	}
	for (std::map<int, gfx::TTF *>::iterator it =  prg.fonts.begin(); it != prg.fonts.end(); it++) {
		gfx::TTF *font = (*it).second;
		delete font;
	}

	prg.mmls.clear();
	prg.images.clear();
	prg.fonts.clear();

	prg.variables.clear();
	prg.functions.clear();
}

void add_syntax(std::string name, library_func processing)
{
	library_map[name] = library.size();
	library.push_back(processing);
}

void end()
{
	library_map.clear();
}

void init_token_map()
{
	token_map['+'] = tokenfunc_add;
	token_map['-'] = tokenfunc_subtract;
	token_map['='] = tokenfunc_equals;
	token_map['?'] = tokenfunc_compare;
	token_map['*'] = tokenfunc_multiply;
	token_map['/'] = tokenfunc_divide;
	token_map[':'] = tokenfunc_label;
	token_map['%'] = tokenfunc_modulus;
	token_map['"'] = tokenfunc_string;
	token_map['{'] = tokenfunc_openbrace;
	token_map['}'] = tokenfunc_closebrace;
	token_map[';'] = tokenfunc_comment;
	token_map['>'] = tokenfunc_callset;
}

Program create_program(std::string code)
{
	Program prg;

	prg.real_line_numbers.push_back(1);
	prg.real_file_names.push_back(main_program_name);
	int i = 0;
	int ln = 2;
	while (code[i] != 0) {
		if (code[i] == '\n') {
			prg.real_line_numbers.push_back(ln++);
		}
		prg.real_file_names.push_back(main_program_name);
		i++;
	}

	prg.code = code;
	prg.name = "main";
	prg.mml_id = 0;
	prg.image_id = 0;
	prg.font_id = 0;
	prg.line = 1;
	prg.line_numbers.clear();
	prg.start_line = 0;
	prg.p = 0;
	prg.prev_tok_p = 0;
	prg.prev_tok_line = 1;
	prg.pc = 0;
	
	while(process_includes(prg));

#ifdef DEBUG_LINE_NUMBERS
FILE *foo = fopen("debug_line_numbers.txt", "w");
fprintf(foo, "%s", prg.code.c_str());
fprintf(foo, "---\n");
for (size_t i = 0; i < prg.real_line_numbers.size(); i++) {
	fprintf(foo, "%d\n", prg.real_line_numbers[i]);
}
fclose(foo);
#endif
		
	compile(prg, PASS1);

	prg.p = 0;
	prg.prev_tok_p = 0;
	prg.line = 1;
	prg.prev_tok_line = 1;
	prg.start_line = 0;
	prg.program.clear();
	prg.functions.clear();

	compile(prg, PASS2);
	
#ifdef DEBUG_LINE_NUMBERS
foo = fopen("debug_line_numbers.txt", "a");
fprintf(foo, "---\n");
for (size_t i = 0; i < prg.line_numbers.size(); i++) {
	if (i >= prg.program.size()) {
		fprintf(foo, "-- line_numbers bigger than program --\n");
		break;
	}
	Statement &s = prg.program[i];
	std::map<std::string, int>::iterator it;
	std::string op = "UNKNOWN";
	for (it = library_map.begin(); it != library_map.end(); it++) {
		if ((*it).second == s.method) {
			op = (*it).first;
			break;
		}
	}
	fprintf(foo, "%d (%s)\n", prg.line_numbers[i], op.c_str());
}
for (size_t i = 0; i < prg.functions.size(); i++) {
	fprintf(foo, "--- %s ---\n", prg.functions[i].name.c_str());
	for (size_t j = 0; j < prg.functions[i].line_numbers.size(); j++) {
		if (j >= prg.functions[i].program.size()) {
			fprintf(foo, "-- line_numbers bigger than program --\n");
			break;
		}
		Statement &s = prg.functions[i].program[j];
		std::map<std::string, int>::iterator it;
		std::string op = "UNKNOWN";
		for (it = library_map.begin(); it != library_map.end(); it++) {
			if ((*it).second == s.method) {
				op = (*it).first;
				break;
			}
		}
		fprintf(foo, "%d (%s)\n", prg.functions[i].line_numbers[j], op.c_str());
	}
}
fclose(foo);
#endif

	prg.p = 0;
	prg.prev_tok_p = 0;
	prg.line = 1;
	prg.prev_tok_line = 1;
	prg.start_line = 0;

	prg.p = 0;
	prg.prev_tok_p = 0;
	prg.line = 1;
	prg.prev_tok_line = 1;
	prg.start_line = 0;
	prg.pc = 0;

	return prg;
}

} // end namespace booboo

