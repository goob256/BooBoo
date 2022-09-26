#include <cctype>

#include <shim4/shim4.h>

#include "booboo/booboo.h"

namespace booboo {

std::map<std::string, library_func> library_map;
std::string reset_game_name;
bool load_from_filesystem;

void skip_whitespace(Program &prg, bool add_lines)
{
	while (prg.p < prg.code.length() && isspace(prg.code[prg.p])) {
		if (prg.code[prg.p] == '\n') {
			prg.line++;
			if (add_lines) {
				prg.line_numbers.push_back(prg.line);
			}
		}
		prg.p++;
	}
}

std::string remove_quotes(std::string s)
{
	std::string ret;

	if (s.length() == 0) {
		return ret;
	}
	
	if (s[0] == '"') {
		ret = s.substr(1);
	}
	else {
		ret = s;
	}

	if (ret.length() <= 1) {
		return "";
	}

	if (ret[ret.length()-1] == '"') {
		ret = ret.substr(0, ret.length()-1);
	}

	return ret;
}

std::string unescape(std::string s)
{
	std::string ret;
	int p = 0;
	char buf[2];
	buf[1] = 0;

	if (s.length() == 0) {
		return "";
	}

	while (p < s.length()) {
		if (s[p] == '\\') {
			if (p+1 < s.length()) {
				if (s[p+1] == '\\' || s[p+1] == '"') {
					p++;
					buf[0] = s[p];
					ret += buf;
					p++;
				}
				else if (s[p+1] == 'n') {
					p++;
					buf[0] = '\n';
					ret += buf;
					p++;
				}
				else if (s[p+1] == 't') {
					p++;
					buf[0] = '\t';
					ret += buf;
					p++;
				}
				else {
					buf[0] = '\\';
					ret += buf;
					p++;
				}
			}
			else {
				buf[0] = '\\';
				ret += buf;
				p++;
			}
		}
		else {
			buf[0] = s[p];
			ret += buf;
			p++;
		}
	}

	return ret;
}

int get_line_num(Program &prg)
{
	int ln = prg.prev_tok_line + prg.start_line;

	if (ln < 0 || ln >= prg.line_numbers.size()) {
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

std::vector<double> variable_names_to_numbers(Program &prg, std::vector<std::string> strings)
{
	std::vector<double> values;

	for (size_t i = 0; i < strings.size(); i++) {
		if (strings[i].length() > 0 && strings[i][0] == '-' || isdigit(strings[i][0])) {
			values.push_back(atof(strings[i].c_str()));
		}
		else {
			Variable &v1 = find_variable(prg, strings[i]);

			if (v1.type == Variable::NUMBER || v1.type == Variable::VECTOR) {
				values.push_back(v1.n);
			}
			else if (v1.type == Variable::STRING) {
				values.push_back(atof(v1.s.c_str()));
			}
			else {
				throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
			}
		}
	}

	return values;
}

static int count_lines(std::string s)
{
	int count = 0;

	for (int i = 0; i < s.length(); i++) {
		if (s[i] == '\n') {
			count++;
		}
	}

	return count;
}

std::string token(Program &prg, bool add_lines)
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

	if (prg.code[prg.p] == ';') {
		prg.p++;
		return ";";
	}
	else if (prg.code[prg.p] == '-') {
		prg.p++;
		if (prg.p < prg.code.length() && isdigit(prg.code[prg.p])) {
			tok = "-";
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
	else if (prg.code[prg.p] == ':') {
		prg.p++;
		return ":";
	}
	else if (prg.code[prg.p] == '=') {
		prg.p++;
		return "=";
	}
	else if (prg.code[prg.p] == '+') {
		prg.p++;
		return "+";
	}
	else if (prg.code[prg.p] == '*') {
		prg.p++;
		return "*";
	}
	else if (prg.code[prg.p] == '/') {
		prg.p++;
		return "/";
	}
	else if (prg.code[prg.p] == '%') {
		prg.p++;
		return "%";
	}
	else if (prg.code[prg.p] == '?') {
		prg.p++;
		return "?";
	}
	else if (prg.code[prg.p] == '{') {
		prg.p++;
		return "{";
	}
	else if (prg.code[prg.p] == '}') {
		prg.p++;
		return "}";
	}
	else if (isdigit(prg.code[prg.p])) {
		while (prg.p < prg.code.length() && (isdigit(prg.code[prg.p]) || prg.code[prg.p] == '.')) {
			s[0] = prg.code[prg.p];
			tok += s;
			prg.p++;
		}
		return tok;
	}
	else if (isalpha(prg.code[prg.p]) || prg.code[prg.p] == '_') {
		while (prg.p < prg.code.length() && (isdigit(prg.code[prg.p]) || isalpha(prg.code[prg.p]) || prg.code[prg.p] == '_')) {
			s[0] = prg.code[prg.p];
			tok += s;
			prg.p++;
		}
		return tok;
	}
	else if (prg.code[prg.p] == '"') {
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
	else if (tok == "") {
		return "";
	}
	else {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Parse error on line " + util::itos(prg.line+prg.start_line) + " (pc=" + util::itos(prg.p) + ", tok=\"" + tok + "\")");
	}

	return tok;
}

std::vector<Label> process_labels(Program prg)
{
	std::vector<Label> labels;
	std::string tok;

	prg.p = 0;
	prg.line = 1;
	prg.prev_tok_p = 0;
	prg.prev_tok_line = 1;

	while ((tok = token(prg)) != "") {
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
			while ((tok = token(prg)) != "") {
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
			std::string name = token(prg);

			if (name == "") {
				throw util::ParseError(std::string(__FUNCTION__) + ": " + "Expected label parameters on line " + util::itos(get_line_num(prg)));
			}

			if (name[0] != '_' && isalpha(name[0]) == false) {
				throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid label name on line " + util::itos(get_line_num(prg)));
			}

			Label l;
			l.name = name;
			l.p = prg.p;
			l.line = prg.line;

			labels.push_back(l);
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
	prg.line_numbers.push_back(prg.line);

	int prev = prg.p;
	int start = 0;

	while ((tok = token(prg, true)) != "") {
		if (tok == ";") {
			while (prg.p < prg.code.length() && prg.code[prg.p] != '\n') {
				prg.p++;
			}
			prg.line++;
			prg.line_numbers.push_back(prg.line);
			if (prg.p < prg.code.length()) {
				prg.p++;
			}
		}
		else if (tok == "include") {
			std::string name = token(prg);

			if (name == "") {
				throw util::ParseError(std::string(__FUNCTION__) + ": " + "Expected include parameters on line " + util::itos(get_line_num(prg)));
			}

			if (name[0] != '"') {
				throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid include name on line " + util::itos(get_line_num(prg)));
			}

			name = remove_quotes(unescape(name));

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
			for (int i = 0; i < new_code.length(); i++) {
				if (new_code[i] == '\n') {
					nlines++;
				}
			}

			for (int i = 0; i < nlines; i++) {
				prg.line_numbers.push_back(i+1);
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

void process_functions(Program &prg)
{
	std::string code;

	std::string tok;

	prg.p = 0;
	prg.line = 1;
	prg.prev_tok_p = 0;
	prg.prev_tok_line = 1;
	prg.line_numbers.clear();
	prg.line_numbers.push_back(prg.line);

	while ((tok = token(prg, true)) != "") {
		if (tok == ";") {
			while (prg.p < prg.code.length() && prg.code[prg.p] != '\n') {
				prg.p++;
			}
			prg.line++;
			prg.line_numbers.push_back(prg.line);
			if (prg.p < prg.code.length()) {
				prg.p++;
			}
		}
		else if (tok == "function") {
			int start_line = prg.line;
			std::string name = token(prg);
			int save = get_line_num(prg);

			if (name == "") {
				throw util::ParseError(std::string(__FUNCTION__) + ": " + "Expected function parameters on line " + util::itos(get_line_num(prg)));
			}

			Program p;
			
			std::string tok2;

			while ((tok2 = token(prg)) != "") {
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

			while ((tok2 = token(prg)) != "") {
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
			p.labels = process_labels(p);
			prg.functions.push_back(p);
		}
	}

	prg.p = 0;
	prg.line = 1;
	prg.prev_tok_p = 0;
	prg.prev_tok_line = 1;
}

static void set_string_or_number(Program &prg, std::string name, double value)
{
	Variable &v1 = find_variable(prg, name);

	if (v1.type == Variable::NUMBER) {
		v1.n = value;
	}
	else if (v1.type == Variable::STRING)
	{
		char buf[1000];
		snprintf(buf, 1000, "%g", value);
		v1.s = buf;
	}
	else {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
	}
}

void call_function(Program &prg, std::string function_name, std::string result_name)
{
	for (size_t i = 0; i < prg.functions.size(); i++) {
		if (prg.functions[i].name == function_name) {
			std::map<std::string, Variable> tmp;
			prg.variables_backup_stack.push_back(tmp);

			for (size_t j = 0; j < prg.functions[i].parameters.size(); j++) {
				std::string param = token(prg);
				
				if (param == "") {
					throw util::ParseError(std::string(__FUNCTION__) + ": " + "Expected call parameters on line " + util::itos(get_line_num(prg)));
				}
				
				Variable var;

				var.function = function_name;

				if (param[0] == '-' || isdigit(param[0])) {
					var.name = prg.functions[i].parameters[j];
					var.type = Variable::NUMBER;
					var.n = atof(param.c_str());
				}
				else {
					Variable &v1 = find_variable(prg, param);
					var = v1;
					var.name = prg.functions[i].parameters[j];
				}

				std::map<std::string, Variable> &variables_backup = prg.variables_backup_stack[prg.variables_backup_stack.size()-1];
				std::map<std::string, Variable>::iterator it3;
				if ((it3 = prg.variables.find(var.name)) != prg.variables.end()) {
					variables_backup[var.name] = prg.variables[var.name];
				}

				prg.variables[var.name] = var;
			}

			std::string code_bak = prg.code;
			int p_bak = prg.p;
			int line_bak = prg.line;
			int start_line_bak = prg.start_line;
			std::vector<Label> labels_bak = prg.labels;
			Variable result_bak = prg.result;
			std::string name_bak = prg.name;

			prg.p = 0;
			prg.line = 1;
			prg.prev_tok_p = 0;
			prg.prev_tok_line = 1;
			prg.code = prg.functions[i].code;
			prg.start_line = prg.functions[i].start_line;
			prg.name = prg.functions[i].name;
			prg.labels = prg.functions[i].labels;

			while (process_includes(prg));
			prg.labels = process_labels(prg);
			process_functions(prg);

			while (interpret(prg)) {
			}

			prg.code = code_bak;
			prg.p = p_bak;
			prg.line = line_bak;
			prg.start_line = start_line_bak;
			prg.labels = labels_bak;
			prg.name = name_bak;

			for (std::map<std::string, Variable>::iterator it = prg.variables.begin(); it != prg.variables.end();) {
				if ((*it).second.function == function_name) {
					it = prg.variables.erase(it);
				}
				else {
					it++;
				}
			}

			std::map<std::string, Variable> &variables_backup = prg.variables_backup_stack[prg.variables_backup_stack.size()-1];
			for (std::map<std::string, Variable>::iterator it = variables_backup.begin(); it != variables_backup.end(); it++) {
				prg.variables[(*it).first] = (*it).second;
			}
			prg.variables_backup_stack.erase(prg.variables_backup_stack.begin()+(prg.variables_backup_stack.size()-1));


			if (result_name != "") {
				for (std::map<std::string, Variable>::iterator it = prg.variables.begin(); it != prg.variables.end(); it++) {
					Variable &v = (*it).second;
					if (result_name == (*it).first) {
						std::string bak = v.name;
						std::string bak2 = v.function;
						v = prg.result;
						v.name = bak;
						v.function = bak2;
						/*
						if (prg.variables[i].type == Variable::VECTOR && prg.result.type == Variable::VECTOR) {
							prg.vectors[(int)prg.variables[i].n] = prg.vectors[(int)p.result.n];
						}
						*/
						break;
					}
				}
			}

			prg.result = result_bak;
		}
	}
}

static bool interpret_breakers(Program &prg, std::string tok)
{
	if (tok == "reset") {
		std::string name = token(prg);

		if (name == "") {
			throw util::ParseError(std::string(__FUNCTION__) + ": " + "Expected reset parameters on line " + util::itos(get_line_num(prg)));
		}

		std::string names;

		if (name[0] == '"') {
			names = remove_quotes(unescape(name));
		}
		else {
			Variable &v1 = find_variable(prg, name);
			if (v1.type == Variable::STRING) {
				names = v1.s;
			}
			else {
				throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
			}
		}

		reset_game_name = names;
	}
	else if (tok == "return") {
		std::string value = token(prg);

		if (value == "") {
			throw util::ParseError(std::string(__FUNCTION__) + ": " + "Expected return parameters on line " + util::itos(get_line_num(prg)));
		}
		
		if (value[0] == '-' || isdigit(value[0])) {
			prg.result.type = Variable::NUMBER;
			prg.result.n = atof(value.c_str());
		}
		else if (value[0] == '"') {
			prg.result.type = Variable::STRING;
			prg.result.s = remove_quotes(unescape(value));
		}
		else {
			Variable &v1 = find_variable(prg, value);
			prg.result = v1;
		}

		prg.result.name = "result";
	}
	else {
		return false;
	}

	return true;
}

bool interpret(Program &prg)
{
	std::string tok = token(prg);

	if (tok == "") {
		return false;
	}

	if (interpret_breakers(prg, tok) == true) {
		return false;
	}

	std::map<std::string, library_func>::iterator it = library_map.find(tok);
	if (it == library_map.end()) {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid token \"" + tok + "\" on line " + util::itos(get_line_num(prg)));
	}
	else {
		library_func func = (*it).second;
		return func(prg, tok);
	}

	return true;
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
	prg.vectors.clear();

	prg.variables.clear();
	prg.functions.clear();
	prg.labels.clear();
}

void add_syntax(std::string name, library_func processing)
{
	library_map[name] = processing;
}

void end()
{
	library_map.clear();
}

Program create_program(std::string code)
{
	Program prg;

	prg.code = code;
	prg.name = "main";
	prg.mml_id = 0;
	prg.image_id = 0;
	prg.font_id = 0;
	prg.vector_id = 0;
	prg.line = 1;
	prg.line_numbers.clear();
	prg.start_line = 0;
	prg.p = 0;
	prg.prev_tok_p = 0;
	prg.prev_tok_line = 1;
	
	while(process_includes(prg));
	prg.labels = process_labels(prg);
	process_functions(prg);

	return prg;
}

} // end namespace booboo
