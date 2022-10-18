#include <cctype>

#include <shim4/shim4.h>

#include "booboo/booboo.h"

namespace booboo {

typedef std::string (*token_func)(Program &);

std::map<std::string, int> library_map;
std::vector<library_func> library;
std::map<char, token_func> token_map;
std::string reset_game_name;
bool load_from_filesystem;
int return_code;

void skip_whitespace(Program &prg, bool add_lines)
{
	while (prg.p < prg.code.length() && isspace(prg.code[prg.p])) {
		if (prg.code[prg.p] == '\n') {
			prg.line++;
			if (add_lines) {
				//prg.line_numbers.push_back(prg.line);
			}
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

int get_line_num(Program &prg)
{
	unsigned int ln = prg.prev_tok_line + prg.start_line;

	if (ln >= prg.line_numbers.size()) {
		return ln;
	}

	return prg.line_numbers[ln];
}

Variable &find_variable(Program &prg, std::string name)
{
	std::map< std::string, Variable >::iterator it;

	it = prg.variables.find(name);

	if (it == prg.variables.end()) {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Unknown variable \"" + name + "\" on line " + util::itos(get_line_num(prg)));
	}

	return (*it).second;
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

std::string token(Program &prg, Token::Token_Type &ret_type, bool add_lines)
{
	prg.prev_tok_p = prg.p;
	prg.prev_tok_line = prg.line;

	skip_whitespace(prg, add_lines);

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

	throw util::ParseError(std::string(__FUNCTION__) + ": " + "Parse error on line " + util::itos(prg.line+prg.start_line) + " (pc=" + util::itos(prg.p) + ", tok=\"" + tok + "\")");

	return "";
}

std::map<std::string, Label> process_labels(Program prg)
{
	std::map<std::string, Label> labels;
	std::string tok;

	prg.p = 0;
	prg.line = 1;
	prg.prev_tok_p = 0;
	prg.prev_tok_line = 1;
	int pc = 0;

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
		else if (tok == "function") {
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
				else if (tok == "}") {
					break;
				}
			}
		}
		else if (tok == ":") {
			std::string name = token(prg, tt);

			if (name == "") {
				throw util::ParseError(std::string(__FUNCTION__) + ": " + "Expected label parameters on line " + util::itos(get_line_num(prg)));
			}

			if (name[0] != '_' && isalpha(name[0]) == false) {
				throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid label name on line " + util::itos(get_line_num(prg)));
			}

			Label l;
			l.name = name;
			l.pc = pc;

			labels[name] = l;
		}
		else if (library_map.find(tok) != library_map.end()) {
			pc++;
		}
	}

	return labels;
}

bool process_includes(Program &prg)
{
	bool ret = false;

	std::string code;

	std::string tok;

	prg.p = 0;
	prg.line = 1;
	prg.prev_tok_p = 0;
	prg.prev_tok_line = 1;
	prg.line_numbers.clear();
	//prg.line_numbers.push_back(prg.line);

	int prev = prg.p;
	int start = 0;

	Token::Token_Type tt;

	while ((tok = token(prg, tt, true)) != "") {
		if (tok == ";") {
			while (prg.p < prg.code.length() && prg.code[prg.p] != '\n') {
				prg.p++;
			}
			prg.line++;
			//prg.line_numbers.push_back(prg.line);
			if (prg.p < prg.code.length()) {
				prg.p++;
			}
		}
		else if (tok == "include") {
			std::string name = token(prg, tt);

			if (name == "") {
				throw util::ParseError(std::string(__FUNCTION__) + ": " + "Expected include parameters on line " + util::itos(get_line_num(prg)));
			}

			if (name[0] != '"') {
				throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid include name on line " + util::itos(get_line_num(prg)));
			}

			name = remove_quotes(util::unescape_string(name));

			code += prg.code.substr(start, prev-start);

			std::string new_code;
			if (load_from_filesystem) {
				new_code = util::load_text_from_filesystem(name);
			}
			else {
				new_code = util::load_text(std::string("code/") + name);
			}

			code += std::string("\n");
			code += new_code;
			code += std::string("\n");

			int nlines = 2;
			for (unsigned int i = 0; i < new_code.length(); i++) {
				if (new_code[i] == '\n') {
					nlines++;
				}
			}

			for (int i = 0; i < nlines; i++) {
				//prg.line_numbers.push_back(i+1);
			}

			start = prg.p;

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

static void compile(Program &prg)
{
	int p_bak = prg.p;
	int line_bak = prg.line;

	std::string tok;
	Token::Token_Type tt;

	while ((tok = token(prg, tt, false)) != "") {
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
				else if (tok == "}") {
					break;
				}
			}
		}
		else if (tok == ":") {
			// skip labels
			token(prg, tt, false);
		}
		else if (library_map.find(tok) != library_map.end()) {
			Statement s;
			s.method = library_map[tok];
			prg.program.push_back(s);
		}
		else if (prg.program.size() == 0) {
			throw util::ParseError("Expected keyword");
		}
		else {
			Token t;
			t.token = tok;
			t.type = tt;
			switch (tt) {
				case Token::STRING:
				case Token::SPECIAL:
				case Token::SYMBOL:
					t.s = remove_quotes(util::unescape_string(tok));
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

void process_functions(Program &prg)
{
	std::string code;

	std::string tok;

	prg.p = 0;
	prg.line = 1;
	prg.prev_tok_p = 0;
	prg.prev_tok_line = 1;
	prg.line_numbers.clear();
	//prg.line_numbers.push_back(prg.line);

	Token::Token_Type tt;

	while ((tok = token(prg, tt, true)) != "") {
		if (tok == ";") {
			while (prg.p < prg.code.length() && prg.code[prg.p] != '\n') {
				prg.p++;
			}
			prg.line++;
			//prg.line_numbers.push_back(prg.line);
			if (prg.p < prg.code.length()) {
				prg.p++;
			}
		}
		else if (tok == "function") {
			int start_line = prg.line;
			std::string name = token(prg, tt);
			int save = get_line_num(prg);

			if (name == "") {
				throw util::ParseError(std::string(__FUNCTION__) + ": " + "Expected function parameters on line " + util::itos(get_line_num(prg)));
			}

			Program p;
			
			std::string tok2;

			while ((tok2 = token(prg, tt)) != "") {
				if (tok2 == "{") {
					break;
				}
				else if (tok2 == ";") {
					while (prg.p < prg.code.length() && prg.code[prg.p] != '\n') {
						prg.p++;
					}
					prg.line++;
					if (prg.p < prg.code.length()) {
						prg.p++;
					}
				}
				p.parameters.push_back(tok2);
			}
			
			if (tok2 != "{") {
				throw util::ParseError(std::string(__FUNCTION__) + ": " + "Function not terminated on line " + util::itos(save));
			}

			int save_p = prg.p;
			int end_p = prg.p;

			while ((tok2 = token(prg, tt)) != "") {
				if (tok2 == "}") {
					break;
				}
				else if (tok2 == ";") {
					while (prg.p < prg.code.length() && prg.code[prg.p] != '\n') {
						prg.p++;
					}
					prg.line++;
					if (prg.p < prg.code.length()) {
						prg.p++;
					}
				}
				end_p = prg.p;
			}

			if (tok2 != "}") {
				throw util::ParseError(std::string(__FUNCTION__) + ": " + "Function not terminated on line " + util::itos(save));
			}

			p.name = name;
			p.p = 0;
			p.line = 1;
			p.start_line = start_line;
			p.code = prg.code.substr(save_p, end_p-save_p);
			while (process_includes(p));
			p.labels = process_labels(p);
			p.pc = 0;
			compile(p);
			prg.functions[name] = p;
		}
	}

	prg.p = 0;
	prg.line = 1;
	prg.prev_tok_p = 0;
	prg.prev_tok_line = 1;
}

void call_function(Program &prg, std::string function_name, std::vector<std::string> params, std::string result_name)
{
	int _tok = 0;
	
	std::map<std::string, Program>::iterator it = prg.functions.find(function_name);
	if (it != prg.functions.end()) {
		Program &func = (*it).second;

		std::map<std::string, Variable> tmp;
		prg.variables_backup_stack.push(tmp);

		for (size_t j = 0; j < func.parameters.size(); j++) {
			std::string param = params[_tok++];//token(prg);
			
			if (param == "") {
				throw util::ParseError(std::string(__FUNCTION__) + ": " + "Expected call parameters on line " + util::itos(get_line_num(prg)));
			}
			
			Variable var;

			if (param[0] == '-' || isdigit(param[0])) {
				var.name = func.parameters[j];
				var.type = Variable::NUMBER;
				var.n = atof(param.c_str());
			}
			else {
				Variable &v1 = find_variable(prg, param);
				var = v1;
				var.name = func.parameters[j];
			}
			// FIXME: "strings"
			
			var.function = function_name;

			std::map<std::string, Variable>::iterator it = prg.variables.find(var.name);
			if (it != prg.variables.end()) {
				std::map<std::string, Variable> &variables_backup = prg.variables_backup_stack.top();
				variables_backup[var.name] = (*it).second;
			}

			prg.variables[var.name] = var;
		}

		std::string code_bak = prg.code;
		int p_bak = prg.p;
		int line_bak = prg.line;
		int start_line_bak = prg.start_line;
		std::map<std::string, Label> labels_bak = prg.labels;
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
		prg.labels = func.labels;
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
		prg.labels = labels_bak;
		prg.name = name_bak;
		prg.program = program_bak;
		prg.pc = pc_bak;

		std::map<std::string, Variable> &variables_backup = prg.variables_backup_stack.top();

		std::map<std::string, Variable>::iterator it;
		for (it = prg.variables.begin(); it != prg.variables.end();) {
			Variable &v = (*it).second;
			if (v.function == function_name) {
				it = prg.variables.erase(it);
			}
			else {
				it++;
			}
		}

		for (it = variables_backup.begin(); it != variables_backup.end(); it++) {
			Variable &v = (*it).second;
			prg.variables[v.name] = v;
		}

		prg.variables_backup_stack.pop();

		if (result_name != "") {
			std::map<std::string, Variable>::iterator it;
			it = prg.variables.find(result_name);
			if (it != prg.variables.end()) {
				Variable &v = (*it).second;
				std::string bak = v.name;
				std::string bak2 = v.function;
				v = prg.result;
				v.name = bak;
				v.function = bak2;
			}
		}

		prg.result = result_bak;
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
	prg.labels.clear();
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
	prg.labels = process_labels(prg);
	process_functions(prg);
		
	std::map<std::string, Variable> tmp;
	prg.variables_backup_stack.push(tmp);

	compile(prg);

	return prg;
}

} // end namespace booboo

