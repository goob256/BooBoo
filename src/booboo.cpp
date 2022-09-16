#include <cctype>

#include <shim4/shim4.h>

#include "booboo.h"
#include "main.h"

static std::map< std::string, double > cfg_numbers;
static std::map< std::string, std::string > cfg_strings;

static std::string itos(int i)
{
	char buf[1000];
	snprintf(buf, 1000, "%d", i);
	return buf;
}

static void skip_whitespace(PROGRAM &prg, bool add_lines = false)
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

static std::string remove_quotes(std::string s)
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

static std::string unescape(std::string s)
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

static std::string save_dir()
{
	std::string path;

#ifdef ANDROID
	path = util::get_standard_path(util::SAVED_GAMES, true);
#elif defined _WIN32
	path = util::get_standard_path(util::SAVED_GAMES, true);
	path += "/" + shim::game_name;
	util::mkdir(path);
#else
	path = util::get_appdata_dir();
#endif

	return path;
}

static std::string cfg_path(std::string cfg_name)
{
	std::string path = save_dir() + "/" + cfg_name + ".txt";
	return path;
}

static bool load_cfg(std::string cfg_name)
{
	std::string text;

	try {
		text = util::load_text_from_filesystem(cfg_path(cfg_name));
	}
	catch (util::Error e) {
		return false;
	}

	util::Tokenizer t(text, '\n');

	std::string line;

	while ((line = t.next()) != "") {
		util::Tokenizer t2(line, '=');
		std::string part1 = t2.next();
		std::string value = t2.next();
		util::trim(value);

		util::Tokenizer t3(part1, ' ');
		std::string type = t3.next();
		std::string name = t3.next();

		if (type == "number") {
			cfg_numbers[name] = atof(value.c_str());
		}
		else if (type == "string") {
			cfg_strings[name] = remove_quotes(value);
		}
	}

	return true;
}

static void save_cfg(std::string cfg_name)
{
	FILE *f = fopen(cfg_path(cfg_name).c_str(), "w");
	if (f == nullptr) {
		return;
	}

	for (std::map<std::string, double>::iterator it = cfg_numbers.begin(); it != cfg_numbers.end(); it++) {
		std::pair<std::string, double> p = *it;
		fprintf(f, "number %s=%g\n", p.first.c_str(), p.second);
	}

	for (std::map<std::string, std::string>::iterator it = cfg_strings.begin(); it != cfg_strings.end(); it++) {
		std::pair<std::string, std::string> p = *it;
		fprintf(f, "string %s=\"%s\"\n", p.first.c_str(), p.second.c_str());
	}

	fclose(f);
}

static int get_line_num(PROGRAM &prg)
{
	int ln = prg.line + prg.start_line;

	// FIXME
	return ln;

	if (ln < 0 || ln >= prg.line_numbers.size()) {
		return -1;
	}

	return prg.line_numbers[ln];
}

static std::vector<double> variable_names_to_numbers(PROGRAM &prg, std::vector<std::string> strings)
{
	std::vector<double> values;

	for (size_t i = 0; i < strings.size(); i++) {
		int index = -1;

		for (size_t j = 0; j < prg.variables.size(); j++) {
			if (prg.variables[j].name == strings[i]) {
				index = j;
				break;
			}
		}

		if (index < 0) {
			values.push_back(atof(strings[i].c_str()));
		}
		else {
			if (prg.variables[index].type == VARIABLE::NUMBER || prg.variables[index].type == VARIABLE::VECTOR) {
				values.push_back(prg.variables[index].n);
			}
			else if (prg.variables[index].type == VARIABLE::STRING) {
				values.push_back(atof(prg.variables[index].s.c_str()));
			}
			else {
				throw PARSE_EXCEPTION(prg.name + ": " + "Invalid type on line " + itos(get_line_num(prg)));
			}
		}
	}

	return values;
}

static int find_variable_index(PROGRAM &prg, std::string name)
{
	int di = -1;

	for (size_t i = 0; i < prg.variables.size(); i++) {
		if (prg.variables[i].name == name) {
			di = i;
			break;
		}
	}

	if (di < 0) {
		throw PARSE_EXCEPTION(prg.name + ": " + "Unknown variable \"" + name + "\" on line " + itos(get_line_num(prg)));
	}

	return di;
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

static std::string token(PROGRAM &prg, bool add_lines = false)
{
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
			while (prg.p < prg.code.length() && (prg.code[prg.p] != '"' || (prev == '\\' && prev_prev != '\\'))) {
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
		throw PARSE_EXCEPTION(prg.name + ": " + "Parse error on line " + itos(prg.line+prg.start_line) + " (pc=" + itos(prg.p) + ", tok=\"" + tok + "\")");
	}

	return tok;
}

EXCEPTION::EXCEPTION(std::string error) :
	error(error)
{
}

PARSE_EXCEPTION::PARSE_EXCEPTION(std::string error) :
	EXCEPTION(error)
{
}

std::vector<LABEL> process_labels(PROGRAM prg)
{
	std::vector<LABEL> labels;
	std::string tok;

	prg.p = 0;
	prg.line = 1;

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
				else if (tok == "end") {
					break;
				}
			}
		}
		else if (tok == "label") {
			std::string name = token(prg);

			if (name == "") {
				throw PARSE_EXCEPTION(prg.name + ": " + "Expected label parameters on line " + itos(get_line_num(prg)));
			}

			if (name[0] != '_' && isalpha(name[0]) == false) {
				throw PARSE_EXCEPTION(prg.name + ": " + "Invalid label name on line " + itos(get_line_num(prg)));
			}

			LABEL l;
			l.name = name;
			l.p = prg.p;
			l.line = prg.line;

			labels.push_back(l);
		}
	}

	return labels;
}

void process_includes(PROGRAM &prg)
{
	std::string code;

	std::string tok;

	prg.p = 0;
	prg.line = 1;
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
		else if (tok == "function") {
			while ((tok = token(prg)) != "") {
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
				else if (tok == "end") {
					break;
				}
			}
		}
		else if (tok == "include") {
			std::string name = token(prg);

			if (name == "") {
				throw PARSE_EXCEPTION(prg.name + ": " + "Expected include parameters on line " + itos(get_line_num(prg)));
			}

			if (name[0] != '"') {
				throw PARSE_EXCEPTION(prg.name + ": " + "Invalid include name on line " + itos(get_line_num(prg)));
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
		}

		prev = prg.p;
	}

	code += prg.code.substr(start, prg.code.length()-start);

	prg.code = code;
	prg.p = 0;
	prg.line = 1;
}

static void set_string_or_number(PROGRAM &prg, std::string name, std::string value)
{
	if (value.length() == 0) {
		return;
	}

	int di = find_variable_index(prg, name);

	VARIABLE &v1 = prg.variables[di];

	double val;

	if (value[0] == '-' || isdigit(value[0])) {
		val = atof(value.c_str());
	}
	else {
		int index = -1;
		for (size_t i = 0; i < prg.variables.size(); i++) {
			if (prg.variables[i].name == value) {
				index = i;
				break;
			}
		}
		if (index < 0) {
			throw PARSE_EXCEPTION(prg.name + ": " + "Unknown variable \"" + value + "\" on line " + itos(get_line_num(prg)));
		}
		VARIABLE &v2 = prg.variables[index];
		if (v2.type == VARIABLE::NUMBER) {
			val = v2.n;
		}
		else if (v2.type == VARIABLE::STRING) {
			val = atof(v2.s.c_str());
		}
		else {
			throw PARSE_EXCEPTION(prg.name + ": " + "Invalid type on line " + itos(get_line_num(prg)));
		}
	}

	if (v1.type == VARIABLE::NUMBER) {
		v1.n = val;
	}
	else {
		throw PARSE_EXCEPTION(prg.name + ": " + "Invalid type on line " + itos(get_line_num(prg)));
	}
}

static void set_string_or_number(PROGRAM &prg, std::string name, double value)
{
	int di = find_variable_index(prg, name);

	VARIABLE &v1 = prg.variables[di];

	if (v1.type == VARIABLE::NUMBER) {
		v1.n = value;
	}
	else {
		throw PARSE_EXCEPTION(prg.name + ": " + "Invalid type on line " + itos(get_line_num(prg)));
	}
}

bool interpret(PROGRAM &prg)
{
	std::string tok = token(prg);

	if (tok == "") {
		return false;
	}

	if (tok == "var") {
		VARIABLE v;

		std::string type = token(prg);
		std::string name =  token(prg);

		v.name = name;
		v.function = prg.name;

		if (type == "number") {
			v.type = VARIABLE::NUMBER;
		}
		else if (type == "string") {
			v.type = VARIABLE::STRING;
		}
		else if (type == "vector") {
			v.type = VARIABLE::VECTOR;
			v.n = prg.vector_id++;
			std::vector<VARIABLE> vec;
			prg.vectors[v.n] = vec;
		}
		else {
			throw PARSE_EXCEPTION(prg.name + ": " + "Invalid type on line " + itos(get_line_num(prg)));
		}

		bool found = false;
		for (size_t i = 0; i < prg.variables.size(); i++) {
			if (prg.variables[i].name == v.name) {
				prg.variables[i] = v;
				found = true;
				break;
			}
		}
		if (found == false) {
			prg.variables.push_back(v);
		}
	}
	else if (tok == "=") {
		std::string dest =  token(prg);
		std::string src =  token(prg);

		if (dest == "" || src == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected = parameters on line " + itos(get_line_num(prg)));
		}

		int di = find_variable_index(prg, dest);

		if (src[0] == '-' || isdigit(src[0])) {
			VARIABLE &v1 = prg.variables[di];

			if (v1.type == VARIABLE::NUMBER) {
				v1.n = atof(src.c_str());
			}
			else if (v1.type == VARIABLE::STRING) {
				v1.s = src;
			}
			else {
				throw PARSE_EXCEPTION(prg.name + ": " + "Invalid type on line " + itos(get_line_num(prg)));
			}
		}
		else if (src[0] == '"') {
			VARIABLE &v1 = prg.variables[di];

			if (v1.type == VARIABLE::NUMBER) {
				v1.n = atof(remove_quotes(src).c_str());
			}
			else if (v1.type == VARIABLE::STRING) {
				v1.s = remove_quotes(unescape(src));
			}
			else {
				throw PARSE_EXCEPTION(prg.name + ": " + "Invalid type on line " + itos(get_line_num(prg)));
			}
		}
		else {
			int si = -1;

			for (size_t i = 0; i < prg.variables.size(); i++) {
				if (prg.variables[i].name == src) {
					si = i;
					break;
				}
			}

			if (si < 0) {
				throw PARSE_EXCEPTION(prg.name + ": " + "Unknown variable \"" + src + "\" on line " + itos(get_line_num(prg)));
			}

			VARIABLE &v1 = prg.variables[di];
			VARIABLE &v2 = prg.variables[si];

			if (v1.type == VARIABLE::NUMBER && v2.type == VARIABLE::NUMBER) {
				v1.n = v2.n;
			}
			else if (v1.type == VARIABLE::STRING && v2.type == VARIABLE::NUMBER) {
				v1.s = itos(v2.n);
			}
			else if (v1.type == VARIABLE::STRING && v2.type == VARIABLE::STRING) {
				v1.s = v2.s;
			}
			else if (v1.type == VARIABLE::VECTOR && v2.type == VARIABLE::VECTOR) {
				prg.vectors[(int)v1.n] = prg.vectors[(int)v2.n];
			}
			else {
				throw PARSE_EXCEPTION(prg.name + ": " + "Operation undefined for operands on line " + itos(get_line_num(prg)));
			}
		}
	}
	else if (tok == "+") {
		std::string dest =  token(prg);
		std::string src =  token(prg);

		if (dest == "" || src == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected + parameters on line " + itos(get_line_num(prg)));
		}

		int di = find_variable_index(prg, dest);

		if (src[0] == '-' || isdigit(src[0])) {
			VARIABLE &v1 = prg.variables[di];

			if (v1.type == VARIABLE::NUMBER) {
				v1.n += atof(src.c_str());
			}
			else if (v1.type == VARIABLE::STRING) {
				v1.s += src;
			}
			else {
				throw PARSE_EXCEPTION(prg.name + ": " + "Invalid type on line " + itos(get_line_num(prg)));
			}
		}
		else if (src[0] == '"') {
			VARIABLE &v1 = prg.variables[di];

			if (v1.type == VARIABLE::NUMBER) {
				v1.n += atof(remove_quotes(src).c_str());
			}
			else if (v1.type == VARIABLE::STRING) {
				v1.s += remove_quotes(unescape(src));
			}
			else {
				throw PARSE_EXCEPTION(prg.name + ": " + "Invalid type on line " + itos(get_line_num(prg)));
			}
		}
		else {
			int si = -1;

			for (size_t i = 0; i < prg.variables.size(); i++) {
				if (prg.variables[i].name == src) {
					si = i;
					break;
				}
			}

			if (si < 0) {
				throw PARSE_EXCEPTION(prg.name + ": " + "Unknown variable \"" + src + "\" on line " + itos(get_line_num(prg)));
			}

			VARIABLE &v1 = prg.variables[di];
			VARIABLE &v2 = prg.variables[si];

			if (v1.type == VARIABLE::NUMBER && v2.type == VARIABLE::NUMBER) {
				v1.n += v2.n;
			}
			else if (v1.type == VARIABLE::STRING && v2.type == VARIABLE::NUMBER) {
				v1.s += itos(v2.n);
			}
			else if (v1.type == VARIABLE::STRING && v2.type == VARIABLE::STRING) {
				v1.s += v2.s;
			}
			else {
				throw PARSE_EXCEPTION(prg.name + ": " + "Operation undefined for operands on line " + itos(get_line_num(prg)));
			}
		}
	}
	else if (tok == "-") {
		std::string dest =  token(prg);
		std::string src =  token(prg);

		if (dest == "" || src == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected - parameters on line " + itos(get_line_num(prg)));
		}

		int di = find_variable_index(prg, dest);

		if (src[0] == '-' || isdigit(src[0])) {
			VARIABLE &v1 = prg.variables[di];

			if (v1.type == VARIABLE::NUMBER) {
				v1.n -= atof(src.c_str());
			}
			else {
				throw PARSE_EXCEPTION(prg.name + ": " + "Invalid type on line " + itos(get_line_num(prg)));
			}
		}
		else if (src[0] == '"') {
			VARIABLE &v1 = prg.variables[di];

			if (v1.type == VARIABLE::NUMBER) {
				v1.n -= atof(remove_quotes(src).c_str());
			}
			else {
				throw PARSE_EXCEPTION(prg.name + ": " + "Invalid type on line " + itos(get_line_num(prg)));
			}
		}
		else {
			int si = -1;

			for (size_t i = 0; i < prg.variables.size(); i++) {
				if (prg.variables[i].name == src) {
					si = i;
					break;
				}
			}

			if (si < 0) {
				throw PARSE_EXCEPTION(prg.name + ": " + "Unknown variable \"" + src + "\" on line " + itos(get_line_num(prg)));
			}

			VARIABLE &v1 = prg.variables[di];
			VARIABLE &v2 = prg.variables[si];

			if (v1.type == VARIABLE::NUMBER && v2.type == VARIABLE::NUMBER) {
				v1.n -= v2.n;
			}
			else {
				throw PARSE_EXCEPTION(prg.name + ": " + "Operation undefined for operands on line " + itos(get_line_num(prg)));
			}
		}
	}
	else if (tok == "*") {
		std::string dest =  token(prg);
		std::string src =  token(prg);

		if (dest == "" || src == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected * parameters on line " + itos(get_line_num(prg)));
		}

		int di = find_variable_index(prg, dest);

		if (src[0] == '-' || isdigit(src[0])) {
			VARIABLE &v1 = prg.variables[di];

			if (v1.type == VARIABLE::NUMBER) {
				v1.n *= atof(src.c_str());
			}
			else {
				throw PARSE_EXCEPTION(prg.name + ": " + "Invalid type on line " + itos(get_line_num(prg)));
			}
		}
		else if (src[0] == '"') {
			VARIABLE &v1 = prg.variables[di];

			if (v1.type == VARIABLE::NUMBER) {
				v1.n *= atof(remove_quotes(src).c_str());
			}
			else {
				throw PARSE_EXCEPTION(prg.name + ": " + "Invalid type on line " + itos(get_line_num(prg)));
			}
		}
		else {
			int si = -1;

			for (size_t i = 0; i < prg.variables.size(); i++) {
				if (prg.variables[i].name == src) {
					si = i;
					break;
				}
			}

			if (si < 0) {
				throw PARSE_EXCEPTION(prg.name + ": " + "Unknown variable \"" + src + "\" on line " + itos(get_line_num(prg)));
			}

			VARIABLE &v1 = prg.variables[di];
			VARIABLE &v2 = prg.variables[si];

			if (v1.type == VARIABLE::NUMBER && v2.type == VARIABLE::NUMBER) {
				v1.n *= v2.n;
			}
			else {
				throw PARSE_EXCEPTION(prg.name + ": " + "Operation undefined for operands on line " + itos(get_line_num(prg)));
			}
		}
	}
	else if (tok == "/") {
		std::string dest =  token(prg);
		std::string src =  token(prg);

		if (dest == "" || src == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected / parameters on line " + itos(get_line_num(prg)));
		}

		int di = find_variable_index(prg, dest);
		if (src[0] == '-' || isdigit(src[0])) {
			VARIABLE &v1 = prg.variables[di];

			if (v1.type == VARIABLE::NUMBER) {
				v1.n /= atof(src.c_str());
			}
			else {
				throw PARSE_EXCEPTION(prg.name + ": " + "Invalid type on line " + itos(get_line_num(prg)));
			}
		}
		else if (src[0] == '"') {
			VARIABLE &v1 = prg.variables[di];

			if (v1.type == VARIABLE::NUMBER) {
				v1.n /= atof(remove_quotes(src).c_str());
			}
			else {
				throw PARSE_EXCEPTION(prg.name + ": " + "Invalid type on line " + itos(get_line_num(prg)));
			}
		}
		else {
			int si = -1;

			for (size_t i = 0; i < prg.variables.size(); i++) {
				if (prg.variables[i].name == src) {
					si = i;
					break;
				}
			}

			if (si < 0) {
				throw PARSE_EXCEPTION(prg.name + ": " + "Unknown variable \"" + src + "\" on line " + itos(get_line_num(prg)));
			}

			VARIABLE &v1 = prg.variables[di];
			VARIABLE &v2 = prg.variables[si];

			if (v1.type == VARIABLE::NUMBER && v2.type == VARIABLE::NUMBER) {
				v1.n /= v2.n;
			}
			else {
				throw PARSE_EXCEPTION(prg.name + ": " + "Operation undefined for operands on line " + itos(get_line_num(prg)));
			}
		}
	}
	else if (tok == "%") {
		std::string dest =  token(prg);
		std::string src =  token(prg);

		if (dest == "" || src == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected %% parameters on line " + itos(get_line_num(prg)));
		}

		int di = find_variable_index(prg, dest);

		if (src[0] == '-' || isdigit(src[0])) {
			VARIABLE &v1 = prg.variables[di];

			if (v1.type == VARIABLE::NUMBER) {
				v1.n = (int)v1.n % (int)atof(src.c_str());
			}
			else {
				throw PARSE_EXCEPTION(prg.name + ": " + "Invalid type on line " + itos(get_line_num(prg)));
			}
		}
		else if (src[0] == '"') {
			VARIABLE &v1 = prg.variables[di];

			if (v1.type == VARIABLE::NUMBER) {
				v1.n = (int)v1.n % (int)atof(remove_quotes(src).c_str());
			}
			else {
				throw PARSE_EXCEPTION(prg.name + ": " + "Invalid type on line " + itos(get_line_num(prg)));
			}
		}
		else {
			int si = -1;

			for (size_t i = 0; i < prg.variables.size(); i++) {
				if (prg.variables[i].name == src) {
					si = i;
					break;
				}
			}

			if (si < 0) {
				throw PARSE_EXCEPTION(prg.name + ": " + "Unknown variable \"" + src + "\" on line " + itos(get_line_num(prg)));
			}

			VARIABLE &v1 = prg.variables[di];
			VARIABLE &v2 = prg.variables[si];

			if (v1.type == VARIABLE::NUMBER && v2.type == VARIABLE::NUMBER) {
				v1.n = (int)v1.n % (int)v2.n;
			}
			else {
				throw PARSE_EXCEPTION(prg.name + ": " + "Operation undefined for operands on line " + itos(get_line_num(prg)));
			}
		}
	}
	else if (tok == "neg") {
		std::string name = token(prg);
		
		if (name == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected neg parameters on line " + itos(get_line_num(prg)));
		}

	int di = find_variable_index(prg, name);

		VARIABLE &v1 = prg.variables[di];

		if (v1.type == VARIABLE::NUMBER) {
			v1.n = -v1.n;
		}
		else {
			throw PARSE_EXCEPTION(prg.name + ": " + "Operation undefined for operands on line " + itos(get_line_num(prg)));
		}
	}
	else if (tok == "sin") {
		std::string dest = token(prg);
		std::string vs = token(prg);
		float v;
		
		if (dest == "" || vs == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected sin parameters on line " + itos(get_line_num(prg)));
		}

		int di = find_variable_index(prg, dest);

		std::vector<std::string> strings;
		strings.push_back(vs);
		std::vector<double> values = variable_names_to_numbers(prg, strings);

		VARIABLE &v1 = prg.variables[di];

		if (v1.type == VARIABLE::NUMBER) {
			v1.n = sin(values[0]);
		}
		else {
			throw PARSE_EXCEPTION(prg.name + ": " + "Operation undefined for operands on line " + itos(get_line_num(prg)));
		}
	}
	else if (tok == "cos") {
		std::string dest = token(prg);
		std::string vs = token(prg);
		float v;
		
		if (dest == "" || vs == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected cos paramters on line " + itos(get_line_num(prg)));
		}

		int di = find_variable_index(prg, dest);

		std::vector<std::string> strings;
		strings.push_back(vs);
		std::vector<double> values = variable_names_to_numbers(prg, strings);

		VARIABLE &v1 = prg.variables[di];

		if (v1.type == VARIABLE::NUMBER) {
			v1.n = cos(values[0]);
		}
		else {
			throw PARSE_EXCEPTION(prg.name + ": " + "Operation undefined for operands on line " + itos(get_line_num(prg)));
		}
	}
	else if (tok == "atan2") {
		std::string dest = token(prg);
		std::string vs1 = token(prg);
		std::string vs2 = token(prg);
		float v;
		
		if (dest == "" || vs1 == "" || vs2 == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected atan2 parameters on line " + itos(get_line_num(prg)));
		}

		int di = find_variable_index(prg, dest);

		std::vector<std::string> strings;
		strings.push_back(vs1);
		strings.push_back(vs2);
		std::vector<double> values = variable_names_to_numbers(prg, strings);

		VARIABLE &v1 = prg.variables[di];

		if (v1.type == VARIABLE::NUMBER) {
			v1.n = atan2(values[0], values[1]);
		}
		else {
			throw PARSE_EXCEPTION(prg.name + ": " + "Operation undefined for operands on line " + itos(get_line_num(prg)));
		}
	}
	else if (tok == "pow") {
		std::string dest = token(prg);
		std::string vs1 = token(prg);
		std::string vs2 = token(prg);
		float v;
		
		if (dest == "" || vs1 == "" || vs2 == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected pow parameters on line " + itos(get_line_num(prg)));
		}

		int di = find_variable_index(prg, dest);

		std::vector<std::string> strings;
		strings.push_back(vs1);
		strings.push_back(vs2);
		std::vector<double> values = variable_names_to_numbers(prg, strings);

		VARIABLE &v1 = prg.variables[di];

		if (v1.type == VARIABLE::NUMBER) {
			v1.n = pow(values[0], values[1]);
		}
		else {
			throw PARSE_EXCEPTION(prg.name + ": " + "Operation undefined for operands on line " + itos(get_line_num(prg)));
		}
	}
	else if (tok == "sqrt") {
		std::string dest = token(prg);
		std::string vs1 = token(prg);
		float v;
		
		if (dest == "" || vs1 == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected sqrt parameters on line " + itos(get_line_num(prg)));
		}

		int di = find_variable_index(prg, dest);

		std::vector<std::string> strings;
		strings.push_back(vs1);
		std::vector<double> values = variable_names_to_numbers(prg, strings);

		VARIABLE &v1 = prg.variables[di];

		if (v1.type == VARIABLE::NUMBER) {
			v1.n = sqrt(values[0]);
		}
		else {
			throw PARSE_EXCEPTION(prg.name + ": " + "Operation undefined for operands on line " + itos(get_line_num(prg)));
		}
	}
	else if (tok == "label") {
		std::string name = token(prg);

		if (name == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected label paramters on line " + itos(get_line_num(prg)));
		}

		if (name[0] != '_' && isalpha(name[0]) == false) {
			throw PARSE_EXCEPTION(prg.name + ": " + "Invalid label name on line " + itos(get_line_num(prg)));
		}

		LABEL l;
		l.name = name;
		l.p = prg.p;
		l.line = prg.line;
		
		//prg.labels.push_back(l);
		//already got these
	}
	else if (tok == "goto") {
		std::string name = token(prg);

		if (name == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected goto paramters on line " + itos(get_line_num(prg)));
		}

		for (size_t i = 0; i < prg.labels.size(); i++) {
			if (prg.labels[i].name == name) {
				prg.p = prg.labels[i].p;
				prg.line = prg.labels[i].line;
			}
		}
	}
	else if (tok == "?") {
		std::string a = token(prg);
		std::string b = token(prg);

		if (a == "" || b == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected ? parameters on line " + itos(get_line_num(prg)));
		}

		std::vector<std::string> strings;
		strings.push_back(a);
		strings.push_back(b);
		std::vector<double> values = variable_names_to_numbers(prg, strings);

		if (values[0] < values[1]) {
			prg.compare_flag = -1;
		}
		else if (values[0] == values[1]) {
			prg.compare_flag = 0;
		}
		else {
			prg.compare_flag = 1;
		}
	}
	else if (tok == "je") {
		std::string label = token(prg);

		if (label == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected je parameters on line " + itos(get_line_num(prg)));
		}

		if (prg.compare_flag == 0) {
			for (size_t i = 0; i < prg.labels.size(); i++) {
				if (prg.labels[i].name == label) {
					prg.p = prg.labels[i].p;
					prg.line = prg.labels[i].line;
				}
			}
		}
	}
	else if (tok == "jne") {
		std::string label = token(prg);

		if (label == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected jne parameters on line " + itos(get_line_num(prg)));
		}

		if (prg.compare_flag != 0) {
			for (size_t i = 0; i < prg.labels.size(); i++) {
				if (prg.labels[i].name == label) {
					prg.p = prg.labels[i].p;
					prg.line = prg.labels[i].line;
				}
			}
		}
	}
	else if (tok == "jl") {
		std::string label = token(prg);

		if (label == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected jl parameters on line " + itos(get_line_num(prg)));
		}

		if (prg.compare_flag < 0) {
			for (size_t i = 0; i < prg.labels.size(); i++) {
				if (prg.labels[i].name == label) {
					prg.p = prg.labels[i].p;
					prg.line = prg.labels[i].line;
				}
			}
		}
	}
	else if (tok == "jle") {
		std::string label = token(prg);

		if (label == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected jle parameters on line " + itos(get_line_num(prg)));
		}

		if (prg.compare_flag <= 0) {
			for (size_t i = 0; i < prg.labels.size(); i++) {
				if (prg.labels[i].name == label) {
					prg.p = prg.labels[i].p;
					prg.line = prg.labels[i].line;
				}
			}
		}
	}
	else if (tok == "jg") {
		std::string label = token(prg);

		if (label == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected jg parameters on line " + itos(get_line_num(prg)));
		}

		if (prg.compare_flag > 0) {
			for (size_t i = 0; i < prg.labels.size(); i++) {
				if (prg.labels[i].name == label) {
					prg.p = prg.labels[i].p;
					prg.line = prg.labels[i].line;
				}
			}
		}
	}
	else if (tok == "jge") {
		std::string label = token(prg);

		if (label == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected jge parameters on line " + itos(get_line_num(prg)));
		}

		if (prg.compare_flag >= 0) {
			for (size_t i = 0; i < prg.labels.size(); i++) {
				if (prg.labels[i].name == label) {
					prg.p = prg.labels[i].p;
					prg.line = prg.labels[i].line;
				}
			}
		}
	}
	else if (tok == "return") {
		std::string value = token(prg);

		if (value == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected return parameters on line " + itos(get_line_num(prg)));
		}
		
		if (value[0] == '-' || isdigit(value[0])) {
			prg.result.type = VARIABLE::NUMBER;
			prg.result.n = atof(value.c_str());
		}
		else if (value[0] == '"') {
			prg.result.type = VARIABLE::STRING;
			prg.result.s = remove_quotes(unescape(value));
		}
		else {
			int index = -1;
			for (size_t i = 0; i < prg.variables.size(); i++) {
				if (prg.variables[i].name == value) {
					index = i;
					break;
				}
			}
			if (index < 0) {
				throw PARSE_EXCEPTION(prg.name + ": " + "Unknown variable \"" + value + "\" on line " + itos(get_line_num(prg)));
			}
			prg.result = prg.variables[index];
		}

		prg.result.name = "result";
		
		return false;
	}
	else if (tok == "call") {
		std::string tok2 = token(prg);
		std::string function_name;
		std::string result_name;
		
		if (tok2 == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected call parameters on line " + itos(get_line_num(prg)));
		}

		if (tok2 == "=") {
			result_name = token(prg);
			function_name = token(prg);
			if (result_name == "" || function_name == "") {
				throw PARSE_EXCEPTION(prg.name + ": " + "Expected call parameters on line " + itos(get_line_num(prg)));
			}

			bool found = false;
			for (size_t i = 0; i < prg.variables.size(); i++) {
				if (prg.variables[i].name == result_name) {
					found = true;
					break;
				}
			}
			if (found == false) {
				throw PARSE_EXCEPTION(prg.name + ": " + "Unknown variable \"" + result_name + "\" on line " + itos(get_line_num(prg)));
			}
		}
		else {
			function_name = tok2;
		}

		for (size_t i = 0; i < prg.functions.size(); i++) {
			if (prg.functions[i].name == function_name) {
				PROGRAM p = prg.functions[i];
				p.p = 0;
				p.line = 1;
				p.variables = prg.variables;
				p.functions = prg.functions;
				p.mml_id = prg.mml_id;
				p.image_id = prg.image_id;
				p.font_id = prg.font_id;
				p.vector_id = prg.vector_id;
				p.mmls = prg.mmls;
				p.images = prg.images;
				p.fonts = prg.fonts;
				p.vectors = prg.vectors;
	
				for (size_t j = 0; j < prg.functions[i].parameters.size(); j++) {
					std::string param = token(prg);
					
					if (param == "") {
						throw PARSE_EXCEPTION(prg.name + ": " + "Expected call parameters on line " + itos(get_line_num(prg)));
					}
					
					VARIABLE var;

					var.function = function_name;

					if (param[0] == '-' || isdigit(param[0])) {
						var.name = prg.functions[i].parameters[j];
						var.type = VARIABLE::NUMBER;
						var.n = atof(param.c_str());
					}
					else {
						bool found = false;

						for (size_t k = 0; k < prg.variables.size(); k++) {
							if (prg.variables[k].name == param) {
								var =  prg.variables[k];
								var.name = prg.functions[i].parameters[j];
								found = true;
								break;
							}
						}
					
						if (found == false) {
							throw PARSE_EXCEPTION(prg.name + ": " + "Unknown variable \"" + param + "\" on line " + itos(get_line_num(prg)));
						}
					}

					int index = -1;
					for (size_t i = 0; i < p.variables.size(); i++) {
						if (p.variables[i].name == var.name) {
							index = i;
							break;
						}
					}

					if (index < 0) {
						p.variables.push_back(var);
					}
					else {
						p.variables[index] = var;
					}
				}

				process_includes(p);
				p.labels = process_labels(p);

				while (interpret(p)) {
				}

				for (size_t i = 0; i < p.variables.size(); i++) {
					if (p.variables[i].function == "main") {
						for (size_t j = 0; j < prg.variables.size(); j++) {
							if (p.variables[i].name == prg.variables[j].name) {
								prg.variables[j] = p.variables[i];
							}
						}
					}
				}

				prg.vectors = p.vectors;
	
				/*
				for (std::map< int, std::vector<VARIABLE> >::iterator it = prg.vectors.begin(); it != prg.vectors.end(); it++) {
					std::pair< int, std::vector<VARIABLE> > pair = *it;
					prg.vectors[pair.first] = p.vectors[pair.first];
				}
				*/

				if (result_name != "") {
					for (size_t i = 0; i < prg.variables.size(); i++) {
						if (prg.variables[i].name == result_name) {
							std::string bak = prg.variables[i].name;
							std::string bak2 = prg.variables[i].function;
							prg.variables[i] = p.result;
							prg.variables[i].name = bak;
							prg.variables[i].function = bak2;
							if (prg.variables[i].type == VARIABLE::VECTOR && p.result.type == VARIABLE::VECTOR) {
								prg.vectors[(int)prg.variables[i].n] = p.vectors[(int)p.result.n];
							}
							break;
						}
					}
				}

				break;
			}
		}
	}
	else if (tok == "rand") {
		std::string dest = token(prg);
		std::string min_incl = token(prg);
		std::string max_incl = token(prg);

		if (dest == "" || min_incl == "" || max_incl == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected rand parameters on line " + itos(get_line_num(prg)));
		}

		int di = find_variable_index(prg, dest);

		std::vector<std::string> strings;
		strings.push_back(min_incl);
		strings.push_back(max_incl);
		std::vector<double> values = variable_names_to_numbers(prg, strings);

		VARIABLE &v1 = prg.variables[di];

		if (v1.type == VARIABLE::NUMBER) {
			v1.n = util::rand(values[0], values[1]);
		}
		else if (v1.type == VARIABLE::STRING) {
			v1.s = itos(util::rand(values[0], values[1]));
		}
		else {
			throw PARSE_EXCEPTION(prg.name + ": " + "Operation undefined for operands on line " + itos(get_line_num(prg)));
		}
	}
	else if (tok == "function") {
		int start_line = prg.line;
		std::string name = token(prg);

		if (name == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected function parameters on line " + itos(get_line_num(prg)));
		}

		PROGRAM p;
		
		std::string tok2;

		while ((tok2 = token(prg)) != "") {
			if (tok2 == "start") {
				break;
			}
			if (tok2[0] != '_' && !isalpha(tok2[0])) {
				throw PARSE_EXCEPTION(prg.name + ": " + "Invalid variable name " + tok2 + " on line " + itos(get_line_num(prg)));
			}
			p.parameters.push_back(tok2);
		}
		
		if (tok2 != "start") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Function not terminated on line " + itos(get_line_num(prg)));
		}

		int save_p = prg.p;
		int end_p = prg.p;

		while ((tok2 = token(prg)) != "") {
			if (tok2 == "end") {
				break;
			}
			end_p = prg.p;
		}

		if (tok2 != "end") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Function not terminated on line " + itos(get_line_num(prg)));
		}

		p.name = name;
		p.p = 0;
		p.line = 1;
		p.start_line = start_line;
		p.code = prg.code.substr(save_p, end_p-save_p);
		p.labels = process_labels(p);
		prg.functions.push_back(p);
	}
	else if (tok == "clear") {
		std::string r =  token(prg);
		std::string g =  token(prg);
		std::string b =  token(prg);
		
		if (r == "" || g == "" || b == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected clear parameters on line " + itos(get_line_num(prg)));
		}
		
		std::vector<std::string> strings;
		strings.push_back(r);
		strings.push_back(g);
		strings.push_back(b);
		std::vector<double> values = variable_names_to_numbers(prg, strings);

		SDL_Colour c;
		c.r = values[0];
		c.g = values[1];
		c.b = values[2];
		c.a = 255;

		gfx::clear(c);
	}
	else if (tok == "start_primitives") {
		gfx::draw_primitives_start();
	}
	else if (tok == "end_primitives") {
		gfx::draw_primitives_end();
	}
	else if (tok == "line") {
		std::string r =  token(prg);
		std::string g =  token(prg);
		std::string b =  token(prg);
		std::string a =  token(prg);
		std::string x =  token(prg);
		std::string y =  token(prg);
		std::string x2 =  token(prg);
		std::string y2 =  token(prg);
		std::string thickness = token(prg);
		
		if (r == "" || g == "" || b == "" || a == "" || x == "" || y == "" || x2 == "" || y2 == "" || thickness == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected line parameters on line " + itos(get_line_num(prg)));
		}
		
		std::vector<std::string> strings;
		strings.push_back(r);
		strings.push_back(g);
		strings.push_back(b);
		strings.push_back(a);
		strings.push_back(x);
		strings.push_back(y);
		strings.push_back(x2);
		strings.push_back(y2);
		strings.push_back(thickness);
		std::vector<double> values = variable_names_to_numbers(prg, strings);

		SDL_Colour c;
		c.r = values[0];
		c.g = values[1];
		c.b = values[2];
		c.a = values[3];

		util::Point<float> p1, p2;

		p1.x = values[4];
		p1.y = values[5];
		p2.x = values[6];
		p2.y = values[7];

		float thick = values[8];

		gfx::draw_line(c, p1, p2, thick);
	}
	else if (tok == "filled_triangle") {
		std::string r1 =  token(prg);
		std::string g1 =  token(prg);
		std::string b1 =  token(prg);
		std::string a1 =  token(prg);
		std::string r2 =  token(prg);
		std::string g2 =  token(prg);
		std::string b2 =  token(prg);
		std::string a2 =  token(prg);
		std::string r3 =  token(prg);
		std::string g3 =  token(prg);
		std::string b3 =  token(prg);
		std::string a3 =  token(prg);
		std::string x =  token(prg);
		std::string y =  token(prg);
		std::string x2 =  token(prg);
		std::string y2 =  token(prg);
		std::string x3 =  token(prg);
		std::string y3 =  token(prg);
		
		if (r1 == "" || g1 == "" || b1 == "" || a1 == "" || r2 == "" || g2 == "" || b2 == "" || a2 == "" || r3 == "" || g3 =="" || b3 == "" || a3 == "" || x == "" || y == "" || x2 == "" || y2 == "" || x3 == "" || y3 == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected filled_triangle parameters on line " + itos(get_line_num(prg)));
		}
		
		std::vector<std::string> strings;
		strings.push_back(r1);
		strings.push_back(g1);
		strings.push_back(b1);
		strings.push_back(a1);
		strings.push_back(r2);
		strings.push_back(g2);
		strings.push_back(b2);
		strings.push_back(a2);
		strings.push_back(r3);
		strings.push_back(g3);
		strings.push_back(b3);
		strings.push_back(a3);
		strings.push_back(x);
		strings.push_back(y);
		strings.push_back(x2);
		strings.push_back(y2);
		strings.push_back(x3);
		strings.push_back(y3);
		std::vector<double> values = variable_names_to_numbers(prg, strings);

		SDL_Colour c[3];
		c[0].r = values[0];
		c[0].g = values[1];
		c[0].b = values[2];
		c[0].a = values[3];
		c[1].r = values[4];
		c[1].g = values[5];
		c[1].b = values[6];
		c[1].a = values[7];
		c[2].r = values[8];
		c[2].g = values[9];
		c[2].b = values[10];
		c[2].a = values[11];

		util::Point<float> p1, p2, p3;

		p1.x = values[12];
		p1.y = values[13];
		p2.x = values[14];
		p2.y = values[15];
		p3.x = values[16];
		p3.y = values[17];

		gfx::draw_filled_triangle(c, p1, p2, p3);
	}
	else if (tok == "rectangle") {
		std::string r =  token(prg);
		std::string g =  token(prg);
		std::string b =  token(prg);
		std::string a =  token(prg);
		std::string x =  token(prg);
		std::string y =  token(prg);
		std::string w =  token(prg);
		std::string h =  token(prg);
		std::string thickness =  token(prg);
		
		if (r == "" || g == "" || b == "" || a == "" || x == "" || y == "" || w == "" || h == "" || thickness == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected rectangle parameters on line " + itos(get_line_num(prg)));
		}
		
		std::vector<std::string> strings;
		strings.push_back(r);
		strings.push_back(g);
		strings.push_back(b);
		strings.push_back(a);
		strings.push_back(x);
		strings.push_back(y);
		strings.push_back(w);
		strings.push_back(h);
		strings.push_back(thickness);
		std::vector<double> values = variable_names_to_numbers(prg, strings);

		SDL_Colour c;
		c.r = values[0];
		c.g = values[1];
		c.b = values[2];
		c.a = values[3];

		util::Point<float> p;
		util::Size<float> sz;

		p.x = values[4];
		p.y = values[5];
		sz.w = values[6];
		sz.h = values[7];

		float thick = values[8];

		gfx::draw_rectangle(c, p, sz, thick);
	}
	else if (tok == "filled_rectangle") {
		std::string r1 =  token(prg);
		std::string g1 =  token(prg);
		std::string b1 =  token(prg);
		std::string a1 =  token(prg);
		std::string r2 =  token(prg);
		std::string g2 =  token(prg);
		std::string b2 =  token(prg);
		std::string a2 =  token(prg);
		std::string r3 =  token(prg);
		std::string g3 =  token(prg);
		std::string b3 =  token(prg);
		std::string a3 =  token(prg);
		std::string r4 =  token(prg);
		std::string g4 =  token(prg);
		std::string b4 =  token(prg);
		std::string a4 =  token(prg);
		std::string x =  token(prg);
		std::string y =  token(prg);
		std::string w =  token(prg);
		std::string h =  token(prg);
		
		if (r1 == "" || g1 == "" || b1 == "" || a1 == "" || r2 == "" || g2 == "" || b2 == "" || a2 == "" || r3 == "" || g3 =="" || b3 == "" || a3 == "" || r4 == "" || g4 == "" || b4 == "" || a4 == "" || x == "" || y == "" || w == "" || h == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected filled_rectangle parameters on line " + itos(get_line_num(prg)));
		}
		
		std::vector<std::string> strings;
		strings.push_back(r1);
		strings.push_back(g1);
		strings.push_back(b1);
		strings.push_back(a1);
		strings.push_back(r2);
		strings.push_back(g2);
		strings.push_back(b2);
		strings.push_back(a2);
		strings.push_back(r3);
		strings.push_back(g3);
		strings.push_back(b3);
		strings.push_back(a3);
		strings.push_back(r4);
		strings.push_back(g4);
		strings.push_back(b4);
		strings.push_back(a4);
		strings.push_back(x);
		strings.push_back(y);
		strings.push_back(w);
		strings.push_back(h);
		std::vector<double> values = variable_names_to_numbers(prg, strings);

		SDL_Colour c[4];
		c[0].r = values[0];
		c[0].g = values[1];
		c[0].b = values[2];
		c[0].a = values[3];
		c[1].r = values[4];
		c[1].g = values[5];
		c[1].b = values[6];
		c[1].a = values[7];
		c[2].r = values[8];
		c[2].g = values[9];
		c[2].b = values[10];
		c[2].a = values[11];
		c[3].r = values[12];
		c[3].g = values[13];
		c[3].b = values[14];
		c[3].a = values[15];

		util::Point<float> p;

		p.x = values[16];
		p.y = values[17];

		util::Size<float> sz;

		sz.w = values[18];
		sz.h = values[19];

		gfx::draw_filled_rectangle(c, p, sz);
	}
	else if (tok == "ellipse") {
		std::string r =  token(prg);
		std::string g =  token(prg);
		std::string b =  token(prg);
		std::string a =  token(prg);
		std::string x =  token(prg);
		std::string y =  token(prg);
		std::string rx =  token(prg);
		std::string ry =  token(prg);
		std::string thickness =  token(prg);
		
		if (r == "" || g == "" || b == "" || a == "" || x == "" || y == "" || rx == "" || ry == "" || thickness == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected ellipse parameters on line " + itos(get_line_num(prg)));
		}
		
		std::vector<std::string> strings;
		strings.push_back(r);
		strings.push_back(g);
		strings.push_back(b);
		strings.push_back(a);
		strings.push_back(x);
		strings.push_back(y);
		strings.push_back(rx);
		strings.push_back(ry);
		strings.push_back(thickness);
		std::vector<double> values = variable_names_to_numbers(prg, strings);

		SDL_Colour c;
		c.r = values[0];
		c.g = values[1];
		c.b = values[2];
		c.a = values[3];

		util::Point<float> p;

		p.x = values[4];
		p.y = values[5];

		float _rx = values[6];
		float _ry = values[7];
		float thick = values[8];

		gfx::draw_ellipse(c, p, _rx, _ry, thick);
	}
	else if (tok == "filled_ellipse") {
		std::string r =  token(prg);
		std::string g =  token(prg);
		std::string b =  token(prg);
		std::string a =  token(prg);
		std::string x =  token(prg);
		std::string y =  token(prg);
		std::string rx =  token(prg);
		std::string ry =  token(prg);
		
		if (r == "" || g == "" || b == "" || a == "" || x == "" || y == "" || rx == "" || ry == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected filled_ellipse parameters on line " + itos(get_line_num(prg)));
		}
		
		std::vector<std::string> strings;
		strings.push_back(r);
		strings.push_back(g);
		strings.push_back(b);
		strings.push_back(a);
		strings.push_back(x);
		strings.push_back(y);
		strings.push_back(rx);
		strings.push_back(ry);
		std::vector<double> values = variable_names_to_numbers(prg, strings);

		SDL_Colour c;
		c.r = values[0];
		c.g = values[1];
		c.b = values[2];
		c.a = values[3];

		util::Point<float> p;

		p.x = values[4];
		p.y = values[5];

		float _rx = values[6];
		float _ry = values[7];

		gfx::draw_filled_ellipse(c, p, _rx, _ry);
	}
	else if (tok == "circle") {
		std::string r =  token(prg);
		std::string g =  token(prg);
		std::string b =  token(prg);
		std::string a =  token(prg);
		std::string x =  token(prg);
		std::string y =  token(prg);
		std::string radius =  token(prg);
		std::string thickness =  token(prg);
		
		if (r == "" || g == "" || b == "" || a == "" || x == "" || y == "" || radius == "" || thickness == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected circle parameters on line " + itos(get_line_num(prg)));
		}
		
		std::vector<std::string> strings;
		strings.push_back(r);
		strings.push_back(g);
		strings.push_back(b);
		strings.push_back(a);
		strings.push_back(x);
		strings.push_back(y);
		strings.push_back(radius);
		strings.push_back(thickness);
		std::vector<double> values = variable_names_to_numbers(prg, strings);

		SDL_Colour c;
		c.r = values[0];
		c.g = values[1];
		c.b = values[2];
		c.a = values[3];

		util::Point<float> p;

		p.x = values[4];
		p.y = values[5];
		float _r = values[6];
		float thick = values[7];

		gfx::draw_circle(c, p, _r, thick);
	}
	else if (tok == "filled_circle") {
		std::string r =  token(prg);
		std::string g =  token(prg);
		std::string b =  token(prg);
		std::string a =  token(prg);
		std::string x =  token(prg);
		std::string y =  token(prg);
		std::string radius =  token(prg);
		
		if (r == "" || g == "" || b == "" || a == "" || x == "" || y == "" || radius == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected filled_circle parameters on line " + itos(get_line_num(prg)));
		}
		
		std::vector<std::string> strings;
		strings.push_back(r);
		strings.push_back(g);
		strings.push_back(b);
		strings.push_back(a);
		strings.push_back(x);
		strings.push_back(y);
		strings.push_back(radius);
		std::vector<double> values = variable_names_to_numbers(prg, strings);

		SDL_Colour c;
		c.r = values[0];
		c.g = values[1];
		c.b = values[2];
		c.a = values[3];

		util::Point<float> p;

		p.x = values[4];
		p.y = values[5];
		float _r = values[6];

		gfx::draw_filled_circle(c, p, _r);
	}
	else if (tok == ";") {
		while (prg.p < prg.code.length() && prg.code[prg.p] != '\n') {
			prg.p++;
		}
		prg.line++;
		if (prg.p < prg.code.length()) {
			prg.p++;
		}
	}
	else if (tok == "play_music") {
		std::string name = token(prg);

		if (name == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected play_music parameters on line " + itos(get_line_num(prg)));
		}

		audio::play_music(remove_quotes(unescape(name)));
	}
	else if (tok == "stop_music") {
		audio::stop_music();
	}
	else if (tok == "create_mml") {
		std::string var = token(prg);
		std::string str = token(prg);

		if (var == "" || str == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected create_mml parameters on line " + itos(get_line_num(prg)));
		}

		int vi = -1;

		for (size_t i = 0; i < prg.variables.size(); i++) {
			if (prg.variables[i].name == var) {
				vi = i;
				break;
			}
		}

		if (vi < 0) {
			throw PARSE_EXCEPTION(prg.name + ": " + "Unknown variable \"" + var + "\" on line " + itos(get_line_num(prg)));
		}

		VARIABLE &v1 = prg.variables[vi];

		if (v1.type == VARIABLE::NUMBER) {
			v1.n = prg.mml_id;
		}
		else if (v1.type == VARIABLE::STRING) {
			v1.s = itos(prg.mml_id);
		}
		else {
			throw PARSE_EXCEPTION(prg.name + ": " + "Invalid type on line " + itos(get_line_num(prg)));
		}

		std::string strs;

		if (str[0] == '"') {
			strs = remove_quotes(unescape(str));
		}
		else {
			int index = -1;
			for (size_t i = 0; i < prg.variables.size(); i++) {
				if (prg.variables[i].name == str) {
					index = i;
					break;
				}
			}
			if (index < 0) {
				throw PARSE_EXCEPTION(prg.name + ": " + "Unknown variable \"" + str + "\" on line " + itos(get_line_num(prg)));
			}
			if (prg.variables[index].type == VARIABLE::STRING) {
				strs = prg.variables[index].s;
			}
			else {
				throw PARSE_EXCEPTION(prg.name + ": " + "Invalid type on line " + itos(get_line_num(prg)));
			}
		}

		Uint8 *bytes = (Uint8 *)strs.c_str();
		SDL_RWops *file = SDL_RWFromMem(bytes, strs.length());
		audio::MML *mml = new audio::MML(file);
		SDL_RWclose(file);

		prg.mmls[prg.mml_id++] = mml;
	}
	else if (tok == "load_mml") {
		std::string var = token(prg);
		std::string name = token(prg);

		if (var == "" || name == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected load_mml parameters on line " + itos(get_line_num(prg)));
		}

		int vi = -1;

		for (size_t i = 0; i < prg.variables.size(); i++) {
			if (prg.variables[i].name == var) {
				vi = i;
				break;
			}
		}

		if (vi < 0) {
			throw PARSE_EXCEPTION(prg.name + ": " + "Unknown variable \"" + var + "\" on line " + itos(get_line_num(prg)));
		}

		VARIABLE &v1 = prg.variables[vi];

		if (v1.type == VARIABLE::NUMBER) {
			v1.n = prg.mml_id;
		}
		else if (v1.type == VARIABLE::STRING) {
			v1.s = itos(prg.mml_id);
		}
		else {
			throw PARSE_EXCEPTION(prg.name + ": " + "Invalid type on line " + itos(get_line_num(prg)));
		}

		audio::MML *mml = new audio::MML(remove_quotes(unescape(name)));

		prg.mmls[prg.mml_id++] = mml;
	}
	else if (tok == "load_image") {
		std::string var = token(prg);
		std::string name = token(prg);

		if (var == "" || name == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected load_image parameters on line " + itos(get_line_num(prg)));
		}

		int vi = -1;

		for (size_t i = 0; i < prg.variables.size(); i++) {
			if (prg.variables[i].name == var) {
				vi = i;
				break;
			}
		}

		if (vi < 0) {
			throw PARSE_EXCEPTION(prg.name + ": " + "Unknown variable \"" + var + "\" on line " + itos(get_line_num(prg)));
		}

		VARIABLE &v1 = prg.variables[vi];

		if (v1.type == VARIABLE::NUMBER) {
			v1.n = prg.image_id;
		}
		else if (v1.type == VARIABLE::STRING) {
			v1.s = itos(prg.image_id);
		}
		else {
			throw PARSE_EXCEPTION(prg.name + ": " + "Invalid type on line " + itos(get_line_num(prg)));
		}

		gfx::Image *img = new gfx::Image(remove_quotes(unescape(name)));

		prg.images[prg.image_id++] = img;
	}
	else if (tok == "load_font") {
		std::string var = token(prg);
		std::string name = token(prg);
		std::string size = token(prg);

		if (var == "" || name == "" || size == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected load_font parameters on line " + itos(get_line_num(prg)));
		}

		int vi = -1;

		for (size_t i = 0; i < prg.variables.size(); i++) {
			if (prg.variables[i].name == var) {
				vi = i;
				break;
			}
		}

		if (vi < 0) {
			throw PARSE_EXCEPTION(prg.name + ": " + "Unknown variable \"" + var + "\" on line " + itos(get_line_num(prg)));
		}

		VARIABLE &v1 = prg.variables[vi];

		if (v1.type == VARIABLE::NUMBER) {
			v1.n = prg.font_id;
		}
		else if (v1.type == VARIABLE::STRING) {
			v1.s = itos(prg.font_id);
		}
		else {
			throw PARSE_EXCEPTION(prg.name + ": " + "Invalid type on line " + itos(get_line_num(prg)));
		}

		std::vector<std::string> strings;
		strings.push_back(size);
		std::vector<double> values = variable_names_to_numbers(prg, strings);

		gfx::TTF *font = new gfx::TTF(remove_quotes(unescape(name)), values[0], 256);

		prg.fonts[prg.font_id++] = font;
	}
	else if (tok == "play_mml") {
		std::string id = token(prg);

		if (id == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected play_mml parameters on line " + itos(get_line_num(prg)));
		}

		std::vector<std::string> strings;
		strings.push_back(id);
		std::vector<double> values = variable_names_to_numbers(prg, strings);

		if (values[0] < 0 || values[0] >= prg.mmls.size()) {
			throw PARSE_EXCEPTION(prg.name + ": " + "Invalid MML on line " + itos(get_line_num(prg)));
		}

		audio::MML *mml = prg.mmls[values[0]];

		mml->play(false);
	}
	else if (tok == "draw_image") {
		std::string id = token(prg);
		std::string x = token(prg);
		std::string y = token(prg);

		if (id == "" || x == "" || y == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected draw_image parameters on line " + itos(get_line_num(prg)));
		}

		std::vector<std::string> strings;
		strings.push_back(id);
		strings.push_back(x);
		strings.push_back(y);
		std::vector<double> values = variable_names_to_numbers(prg, strings);
		
		if (values[0] < 0 || values[0] >= prg.images.size()) {
			throw PARSE_EXCEPTION(prg.name + ": " + "Invalid Image on line " + itos(get_line_num(prg)));
		}

		gfx::Image *img = prg.images[values[0]];

		img->draw(util::Point<float>(values[1], values[2]));
	}
	else if (tok == "draw_text") {
		std::string id = token(prg);
		std::string r = token(prg);
		std::string g = token(prg);
		std::string b = token(prg);
		std::string a = token(prg);
		std::string text = token(prg);
		std::string x = token(prg);
		std::string y = token(prg);

		if (id == "" || r == "" || g == "" || b == "" || a == "" || text == "" || x == "" || y == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected draw_text parameters on line " + itos(get_line_num(prg)));
		}

		std::vector<std::string> strings;
		strings.push_back(id);
		strings.push_back(r);
		strings.push_back(g);
		strings.push_back(b);
		strings.push_back(a);
		strings.push_back(x);
		strings.push_back(y);
		std::vector<double> values = variable_names_to_numbers(prg, strings);

		if (values[0] < 0 || values[0] >= prg.fonts.size()) {
			throw PARSE_EXCEPTION(prg.name + ": " + "Invalid Font on line " + itos(get_line_num(prg)));
		}

		gfx::TTF *font = prg.fonts[values[0]];

		SDL_Colour c;
		c.r = values[1];
		c.g = values[2];
		c.b = values[3];
		c.a = values[4];

		std::string txt;

		if (text[0] == '"') {
			txt = remove_quotes(unescape(text));
		}
		else {
			for (size_t i = 0; i < prg.variables.size(); i++) {
				if (prg.variables[i].name == text) {
					VARIABLE &v1 = prg.variables[i];
					if (v1.type == VARIABLE::NUMBER) {
						char buf[1000];
						snprintf(buf, 1000, "%g", v1.n);
						txt = buf;
					}
					else if (v1.type == VARIABLE::STRING) {
						txt = v1.s;
					}
					else {
						throw PARSE_EXCEPTION(prg.name + ": " + "Unknown variable \"" + text + "\" on line " + itos(get_line_num(prg)));
					}
				}
			}
		}

		font->draw(c, txt, util::Point<float>(values[5], values[6]));
	}
	else if (tok == "text_width") {
		std::string id = token(prg);
		std::string dest = token(prg);
		std::string text = token(prg);
		
		if (id == "" || dest == "" || text == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected text_width parameters on line " + itos(get_line_num(prg)));
		}

		int di = find_variable_index(prg, dest);

		std::vector<std::string> strings;
		strings.push_back(id);
		std::vector<double> values = variable_names_to_numbers(prg, strings);

		std::string txt;

		if (text[0] == '"') {
			txt = remove_quotes(unescape(text));
		}
		else {
			for (size_t i = 0; i < prg.variables.size(); i++) {
				if (prg.variables[i].name == text) {
					VARIABLE &v1 = prg.variables[i];
					if (v1.type == VARIABLE::NUMBER) {
						char buf[1000];
						snprintf(buf, 1000, "%g", v1.n);
						txt = buf;
					}
					else if (v1.type == VARIABLE::STRING) {
						txt = v1.s;
					}
					else {
						throw PARSE_EXCEPTION(prg.name + ": " + "Unknown variable \"" + dest + "\" on line " + itos(get_line_num(prg)));
					}
				}
			}
		}

		gfx::TTF *font = prg.fonts[values[0]];

		int w = font->get_text_width(txt);

		VARIABLE &v1 = prg.variables[di];

		if (v1.type == VARIABLE::NUMBER) {
			v1.n = w;
		}
		else {
			throw PARSE_EXCEPTION(prg.name + ": " + "Invalid type on line " + itos(get_line_num(prg)));
		}
	}
	else if (tok == "font_height") {
		std::string id = token(prg);
		std::string dest = token(prg);
		
		if (id == "" || dest == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected font_height parameters on line " + itos(get_line_num(prg)));
		}

		int di = find_variable_index(prg, dest);

		std::vector<std::string> strings;
		strings.push_back(id);
		std::vector<double> values = variable_names_to_numbers(prg, strings);

		gfx::TTF *font = prg.fonts[values[0]];

		int h = font->get_height();

		VARIABLE &v1 = prg.variables[di];

		if (v1.type == VARIABLE::NUMBER) {
			v1.n = h;
		}
		else {
			throw PARSE_EXCEPTION(prg.name + ": " + "Invalid type on line " + itos(get_line_num(prg)));
		}
	}
	else if (tok == "font_smooth") {
		std::string id = token(prg);
		std::string smooth = token(prg);
		
		if (id == "" || smooth == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected font_smooth parameters on line " + itos(get_line_num(prg)));
		}

		std::vector<std::string> strings;
		strings.push_back(id);
		strings.push_back(smooth);
		std::vector<double> values = variable_names_to_numbers(prg, strings);

		gfx::TTF *font = prg.fonts[values[0]];

		font->set_smooth(values[1]);
	}
	else if (tok == "poll_joystick") {
		std::string num = token(prg);
		std::string x1 = token(prg);
		std::string y1 = token(prg);
		std::string x2 = token(prg);
		std::string y2 = token(prg);
		std::string l = token(prg);
		std::string r = token(prg);
		std::string u = token(prg);
		std::string d = token(prg);
		std::string a = token(prg);
		std::string b = token(prg);
		std::string x = token(prg);
		std::string y = token(prg);
		std::string lb = token(prg);
		std::string rb = token(prg);
		std::string back = token(prg);
		std::string start = token(prg);
	
		if (num == "" || x1 == "" || y1 == "" || x2 == "" || y2 == "" || l == "" || r == "" || u == "" || d == "" || a == "" || b == "" || x == "" || y == "" || lb == "" || rb == "" || back == "" || start == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected poll_joystick parameters on line " + itos(get_line_num(prg)));
		}

		std::vector<std::string> strings;
		strings.push_back(num);
		std::vector<double> values = variable_names_to_numbers(prg, strings);

		std::vector<std::string> names;
		names.push_back(x1);
		names.push_back(y1);
		names.push_back(x2);
		names.push_back(y2);
		names.push_back(l);
		names.push_back(r);
		names.push_back(u);
		names.push_back(d);
		names.push_back(a);
		names.push_back(b);
		names.push_back(x);
		names.push_back(y);
		names.push_back(lb);
		names.push_back(rb);
		names.push_back(back);
		names.push_back(start);

		for (size_t i = 0; i < names.size(); i++) {
			bool found = false;
			for (size_t j = 0; j < prg.variables.size(); j++) {
				if (prg.variables[j].name == names[i]) {
					found = true;
					break;
				}
			}
			if (found == false) {
				throw PARSE_EXCEPTION(prg.name + ": " + "Invalid variable name " + names[i] + " on line " + itos(get_line_num(prg)));
			}
		}

		SDL_JoystickID id = input::get_controller_id(values[0]);
		SDL_Joystick *joy = input::get_sdl_joystick(id);
		bool connected = joy != nullptr;

		if (connected == false) {
			set_string_or_number(prg, x1, 0);
			set_string_or_number(prg, y1, 0);
			set_string_or_number(prg, x2, 0);
			set_string_or_number(prg, y2, 0);
			set_string_or_number(prg, a, 0);
			set_string_or_number(prg, l, 0);
			set_string_or_number(prg, r, 0);
			set_string_or_number(prg, u, 0);
			set_string_or_number(prg, d, 0);
			set_string_or_number(prg, b, 0);
			set_string_or_number(prg, x, 0);
			set_string_or_number(prg, y, 0);
			set_string_or_number(prg, lb, 0);
			set_string_or_number(prg, rb, 0);
			set_string_or_number(prg, back, 0);
			set_string_or_number(prg, start, 0);
		}
		else {

			Sint16 si_x1 = SDL_JoystickGetAxis(joy, 0);
			Sint16 si_y1 = SDL_JoystickGetAxis(joy, 1);
			Sint16 si_x2 = SDL_JoystickGetAxis(joy, 2);
			Sint16 si_y2 = SDL_JoystickGetAxis(joy, 3);

			double x1f;
			double y1f;
			double x2f;
			double y2f;

			if (si_x1 < 0) {
				x1f = si_x1 / 32768.0;
			}
			else {
				x1f = si_x1 / 32767.0;
			}

			if (si_y1 < 0) {
				y1f = si_y1 / 32768.0;
			}
			else {
				y1f = si_y1 / 32767.0;
			}

			if (si_x2 < 0) {
				x2f = si_x2 / 32768.0;
			}
			else {
				x2f = si_x2 / 32767.0;
			}

			if (si_y2 < 0) {
				y2f = si_y2 / 32768.0;
			}
			else {
				y2f = si_y2 / 32767.0;
			}

			set_string_or_number(prg, x1, x1f);
			set_string_or_number(prg, y1, y1f);
			set_string_or_number(prg, x2, x2f);
			set_string_or_number(prg, y2, y2f);

			Uint8 hat = SDL_JoystickGetHat(joy, 0);
			
			double _lb = 0;
			double _rb = 0;
			double ub = 0;
			double db = 0;

			switch (hat) {
				case SDL_HAT_UP:
					ub = true;
					break;
				case SDL_HAT_RIGHT:
					_rb = true;
					break;
				case SDL_HAT_DOWN:
					db = true;
					break;
				case SDL_HAT_LEFT:
					_lb = true;
					break;
				case SDL_HAT_RIGHTUP:
					_rb = true;
					ub = true;
					break;
				case SDL_HAT_RIGHTDOWN:
					_rb = true;
					db = true;
					break;
				case SDL_HAT_LEFTUP:
					_lb = true;
					ub = true;
					break;

				case SDL_HAT_LEFTDOWN:
					_lb = true;
					db = true;
			}

			//double _lb = SDL_JoystickGetButton(joy, TGUI_B_L);
			//double _rb = SDL_JoystickGetButton(joy, TGUI_B_R);
			//double ub = SDL_JoystickGetButton(joy, TGUI_B_U);
			//double db = SDL_JoystickGetButton(joy, TGUI_B_D);
			double ab = SDL_JoystickGetButton(joy, TGUI_B_A);
			double bb = SDL_JoystickGetButton(joy, TGUI_B_B);
			double xb = SDL_JoystickGetButton(joy, TGUI_B_X);
			double yb = SDL_JoystickGetButton(joy, TGUI_B_Y);
			double lbb = SDL_JoystickGetButton(joy, TGUI_B_LB);
			double rbb = SDL_JoystickGetButton(joy, TGUI_B_RB);
			double backb = SDL_JoystickGetButton(joy, TGUI_B_BACK);
			double startb = SDL_JoystickGetButton(joy, TGUI_B_START);

			set_string_or_number(prg, l, _lb);
			set_string_or_number(prg, r, _rb);
			set_string_or_number(prg, u, ub);
			set_string_or_number(prg, d, db);
			set_string_or_number(prg, a, ab);
			set_string_or_number(prg, b, bb);
			set_string_or_number(prg, x, xb);
			set_string_or_number(prg, y, yb);
			set_string_or_number(prg, lb, lbb);
			set_string_or_number(prg, rb, rbb);
			set_string_or_number(prg, rb, backb);
			set_string_or_number(prg, rb, startb);
		}
	}
	else if (tok == "num_joysticks") {
		std::string dest = token(prg);
		
		if (dest == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected num_joysticks parameters on line " + itos(get_line_num(prg)));
		}

		int di = find_variable_index(prg, dest);

		VARIABLE &v1 = prg.variables[di];

		if (v1.type == VARIABLE::NUMBER) {
			v1.n = input::get_num_joysticks();
		}
		else {
			throw PARSE_EXCEPTION(prg.name + ": " + "Operation undefined for operands on line " + itos(get_line_num(prg)));
		}
	}
	else if (tok == "vector_add") {
		std::string id = token(prg);
		std::string value = token(prg);

		if (id == "" || value == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected vector_add parameters on line " + itos(get_line_num(prg)));
		}
		
		std::vector<std::string> strings;
		strings.push_back(id);
		std::vector<double> values = variable_names_to_numbers(prg, strings);
		
		if (values[0] < 0 || values[0] >= prg.vectors.size()) {
			throw PARSE_EXCEPTION(prg.name + ": " + "Invalid vector on line " + itos(get_line_num(prg)));
		}

		std::vector<VARIABLE> &v = prg.vectors[values[0]];

		VARIABLE var;

		if (value[0] == '"') {
			var.type = VARIABLE::STRING;
			var.function = prg.name;
			var.name = "-constant-";
			var.s = remove_quotes(unescape(value));
		}
		else if (value[0] == '-' || isdigit(value[0])) {
			var.type = VARIABLE::NUMBER;
			var.function = prg.name;
			var.name = "-constant-";
			var.n = atof(value.c_str());
		}
		else {
			bool found = false;
			for (size_t i = 0; i < prg.variables.size(); i++) {
				if (prg.variables[i].name == value) {
					var = prg.variables[i];
					found = true;
					break;
				}
			}
			if (found == false) {
				throw PARSE_EXCEPTION(prg.name + ": " + "Invalid variable name " + value + " on line " + itos(get_line_num(prg)));
			}
		}

		v.push_back(var);
	}
	else if (tok == "vector_size") {
		std::string id = token(prg);
		std::string dest = token(prg);

		if (id == "" || dest == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected vector_size parameters on line " + itos(get_line_num(prg)));
		}
		
		std::vector<std::string> strings;
		strings.push_back(id);
		std::vector<double> values = variable_names_to_numbers(prg, strings);
		
		if (values[0] < 0 || values[0] >= prg.vectors.size()) {
			throw PARSE_EXCEPTION(prg.name + ": " + "Invalid vector on line " + itos(get_line_num(prg)));
		}

		std::vector<VARIABLE> &v = prg.vectors[values[0]];

		int di = find_variable_index(prg, dest);

		VARIABLE &v1 = prg.variables[di];
		if (v1.type == VARIABLE::NUMBER) {
			v1.n = v.size();
		}
		else {
			throw PARSE_EXCEPTION(prg.name + ": " + "Invalid type on line " + itos(get_line_num(prg)));
		}
	}
	else if (tok == "vector_set") {
		std::string id = token(prg);
		std::string index = token(prg);
		std::string value = token(prg);

		if (id == "" || index == "" || value == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected vector_set parameters on line " + itos(get_line_num(prg)));
		}
		
		std::vector<std::string> strings;
		strings.push_back(id);
		strings.push_back(index);
		std::vector<double> values = variable_names_to_numbers(prg, strings);
		
		if (values[0] < 0 || values[0] >= prg.vectors.size()) {
			throw PARSE_EXCEPTION(prg.name + ": " + "Invalid vector on line " + itos(get_line_num(prg)));
		}

		std::vector<VARIABLE> &v = prg.vectors[values[0]];

		if (values[1] < 0 || values[1] >= v.size()) {
			throw PARSE_EXCEPTION(prg.name + ": " + "Invalid index on line " + itos(get_line_num(prg)));
		}

		VARIABLE var;

		if (value[0] == '"') {
			var.type = VARIABLE::STRING;
			var.function = prg.name;
			var.name = "-constant-";
			var.s = remove_quotes(unescape(value));
		}
		else if (value[0] == '-' || isdigit(value[0])) {
			var.type = VARIABLE::NUMBER;
			var.function = prg.name;
			var.name = "-constant-";
			var.n = atof(value.c_str());
		}
		else {
			bool found = false;
			for (size_t i = 0; i < prg.variables.size(); i++) {
				if (prg.variables[i].name == value) {
					var = prg.variables[i];
					found = true;
					break;
				}
			}
			if (found == false) {
				throw PARSE_EXCEPTION(prg.name + ": " + "Invalid variable name " + value + " on line " + itos(get_line_num(prg)));
			}
		}

		v[values[1]] = var;
	}
	else if (tok == "vector_insert") {
		std::string id = token(prg);
		std::string index = token(prg);
		std::string value = token(prg);

		if (id == "" || index == "" || value == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected vector_insert parameters on line " + itos(get_line_num(prg)));
		}
		
		std::vector<std::string> strings;
		strings.push_back(id);
		strings.push_back(index);
		std::vector<double> values = variable_names_to_numbers(prg, strings);
		
		if (values[0] < 0 || values[0] >= prg.vectors.size()) {
			throw PARSE_EXCEPTION(prg.name + ": " + "Invalid vector on line " + itos(get_line_num(prg)));
		}

		std::vector<VARIABLE> &v = prg.vectors[values[0]];

		if (values[1] < 0 || values[1] > v.size()) {
			throw PARSE_EXCEPTION(prg.name + ": " + "Invalid index on line " + itos(get_line_num(prg)));
		}

		VARIABLE var;

		if (value[0] == '"') {
			var.type = VARIABLE::STRING;
			var.function = prg.name;
			var.name = "-constant-";
			var.s = remove_quotes(unescape(value));
		}
		else if (value[0] == '-' || isdigit(value[0])) {
			var.type = VARIABLE::NUMBER;
			var.function = prg.name;
			var.name = "-constant-";
			var.n = atof(value.c_str());
		}
		else {
			bool found = false;
			for (size_t i = 0; i < prg.variables.size(); i++) {
				if (prg.variables[i].name == value) {
					var = prg.variables[i];
					found = true;
					break;
				}
			}
			if (found == false) {
				throw PARSE_EXCEPTION(prg.name + ": " + "Invalid variable name " + value + " on line " + itos(get_line_num(prg)));
			}
		}

		v.insert(v.begin()+values[1], var);
	}
	else if (tok == "vector_get") {
		std::string id = token(prg);
		std::string dest = token(prg);
		std::string index = token(prg);

		if (id == "" || dest == "" || index == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected vector_get parameters on line " + itos(get_line_num(prg)));
		}
		
		std::vector<std::string> strings;
		strings.push_back(id);
		strings.push_back(index);
		std::vector<double> values = variable_names_to_numbers(prg, strings);

		if (values[0] < 0 || values[0] >= prg.vectors.size()) {
			throw PARSE_EXCEPTION(prg.name + ": " + "Invalid vector on line " + itos(get_line_num(prg)));
		}

		std::vector<VARIABLE> &v = prg.vectors[values[0]];

		if (values[1] < 0 || values[1] >= v.size()) {
			throw PARSE_EXCEPTION(prg.name + ": " + "Invalid index on line " + itos(get_line_num(prg)));
		}

		bool found = false;
		for (size_t i = 0; i < prg.variables.size(); i++) {
			if (prg.variables[i].name == dest) {
				std::string bak = prg.variables[i].name;
				std::string bak2 = prg.variables[i].function;
				prg.variables[i] = v[values[1]];
				prg.variables[i].name = bak;
				prg.variables[i].function = bak2;
				found = true;
				break;
			}
		}
		if (found == false) {
			throw PARSE_EXCEPTION(prg.name + ": " + "Invalid variable name " + dest + " on line " + itos(get_line_num(prg)));
		}
	}
	else if (tok == "vector_erase") {
		std::string id = token(prg);
		std::string index = token(prg);

		if (id == "" || index == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected vector_erase parameters on line " + itos(get_line_num(prg)));
		}
		
		std::vector<std::string> strings;
		strings.push_back(id);
		strings.push_back(index);
		std::vector<double> values = variable_names_to_numbers(prg, strings);

		if (values[0] < 0 || values[0] >= prg.vectors.size()) {
			throw PARSE_EXCEPTION(prg.name + ": " + "Invalid vector on line " + itos(get_line_num(prg)));
		}

		std::vector<VARIABLE> &v = prg.vectors[values[0]];

		if (values[1] < 0 || values[1] >= v.size()) {
			throw PARSE_EXCEPTION(prg.name + ": " + "Invalid index on line " + itos(get_line_num(prg)));
		}

		v.erase(v.begin() + int(values[1]));
	}
	else if (tok == "inspect") {
		std::string name = token(prg);
		
		if (name == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected inspect parameters on line " + itos(get_line_num(prg)));
		}

		int di = -1;

		for (size_t i = 0; i < prg.variables.size(); i++) {
			if (prg.variables[i].name == name) {
				di = i;
				break;
			}
		}

		if (di < 0) {
			if (name[0] != '-' && !isdigit(name[0]) && name[0] != '"') {
				throw PARSE_EXCEPTION(prg.name + ": " + "Unknown variable \"" + name + "\" on line " + itos(get_line_num(prg)));
			}
		}

		VARIABLE &v1 = prg.variables[di];

		char buf[1000];

		if (name[0] == '-' || isdigit(name[0])) {
			snprintf(buf, 1000, "%s", name.c_str());
		}
		else if (name[0] == '"') {
			std::string s = remove_quotes(unescape(name));
			snprintf(buf, 1000, "%s", s.c_str());
		}
		else if (v1.type == VARIABLE::NUMBER) {
			snprintf(buf, 1000, "%g", v1.n);
		}
		else if (v1.type == VARIABLE::STRING) {
			snprintf(buf, 1000, "\"%s\"", v1.s.c_str());
		}
		else {
			strcpy(buf, "Unknown");
		}

		gui::popup("INSPECTOR", buf, gui::OK);
	}
	else if (tok == "sub") {
		std::string result = token(prg);
		std::string name = token(prg);
		std::string params = token(prg);

		if (result == "" || name == "" || params == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected sub parameters on line " + itos(get_line_num(prg)));
		}

		int di = find_variable_index(prg, result);

		std::string names;

		if (name[0] == '"') {
			names = remove_quotes(unescape(name));
		}
		else {
			bool found = false;
			for (size_t i = 0; i < prg.variables.size(); i++) {
				if (prg.variables[i].name == name) {
					VARIABLE &v2 = prg.variables[i];
					if (v2.type == VARIABLE::STRING) {
						names = v2.s;
					}
					else {
						throw PARSE_EXCEPTION(prg.name + ": " + "Invalid type on line " + itos(get_line_num(prg)));
					}
					found = true;
					break;
				}
			}
			if (found == false) {
				throw PARSE_EXCEPTION(prg.name + ": " + "Invalid variable name " + result + " on line " + itos(get_line_num(prg)));
			}
		}

		int index = -1;
		for (size_t i = 0; i < prg.variables.size(); i++) {
			if (prg.variables[i].name == params) {
				index = i;
				break;
			}
		}
		if (index < 0) {
			VARIABLE var;
			var.name = params;
			if (params[0] == '"') {
				var.type = VARIABLE::STRING;
				var.s = remove_quotes(unescape(params));
			}
			else if (params[0] == '-' || isdigit(params[0])) {
				var.type = VARIABLE::NUMBER;
				var.n = atof(params.c_str());
			}
			else {
				throw PARSE_EXCEPTION(prg.name + ": " + "Invalid type on line " + itos(get_line_num(prg)));
			}

			var.function = prg.name;

			prg.variables.push_back(var);

			index = prg.variables.size() - 1;

		}

		PROGRAM p;
		p.name = "main";
		p.mml_id = 0;
		p.image_id = 0;
		p.font_id = 0;
		p.variables = prg.variables;
		p.functions = prg.functions;
		p.mml_id = prg.mml_id;
		p.image_id = prg.image_id;
		p.font_id = prg.font_id;
		p.vector_id = prg.vector_id;
		p.mmls = prg.mmls;
		p.images = prg.images;
		p.fonts = prg.fonts;
		p.vectors = prg.vectors;

		if (load_from_filesystem) {
			p.code = util::load_text_from_filesystem(names);
		}
		else {
			p.code = util::load_text(std::string("code/") + names);
		}

		p.line = 1;
		p.start_line = 0;
		p.p = 0;

		p.variables.push_back(prg.variables[index]);
		p.variables[p.variables.size()-1].name = "params";

		process_includes(p);
		p.labels = process_labels(p);

		while (interpret(p)) {
		}

		std::string bak = prg.variables[di].name;
		std::string bak2 = prg.variables[di].function;
		prg.variables[di] = p.result;
		prg.variables[di].name = bak;
		prg.variables[di].function = bak2;

		// Remove mml/image/font assets that are from the main program so they don't get destroyed

		for (std::map<int, audio::MML *>::iterator it = p.mmls.begin(); it != p.mmls.end();) {
			std::pair<int, audio::MML *> pair = *it;
			int id = pair.first;
			bool found = false;
			for (std::map<int, audio::MML *>::iterator it2 = prg.mmls.begin(); it2 != prg.mmls.end(); it2++) {
				std::pair<int, audio::MML *> pair2 = *it2;
				int id2 = pair2.first;
				if (id == id2) {
					found = true;
				}
			}
			if (found) {
				it = p.mmls.erase(it);
			}
			else {
				it++;
			}
		}

		for (std::map<int, gfx::Image *>::iterator it = p.images.begin(); it != p.images.end();) {
			std::pair<int, gfx::Image *> pair = *it;
			int id = pair.first;
			bool found = false;
			for (std::map<int, gfx::Image *>::iterator it2 = prg.images.begin(); it2 != prg.images.end(); it2++) {
				std::pair<int, gfx::Image *> pair2 = *it2;
				int id2 = pair2.first;
				if (id == id2) {
					found = true;
				}
			}
			if (found) {
				it = p.images.erase(it);
			}
			else {
				it++;
			}
		}

		for (std::map<int, gfx::TTF *>::iterator it = p.fonts.begin(); it != p.fonts.end();) {
			std::pair<int, gfx::TTF *> pair = *it;
			int id = pair.first;
			bool found = false;
			for (std::map<int, gfx::TTF *>::iterator it2 = prg.fonts.begin(); it2 != prg.fonts.end(); it2++) {
				std::pair<int, gfx::TTF *> pair2 = *it2;
				int id2 = pair2.first;
				if (id == id2) {
					found = true;
				}
			}
			if (found) {
				it = p.fonts.erase(it);
			}
			else {
				it++;
			}
		}

		destroy_program(p, false);
	}
	else if (tok == "string_format") {
		std::string dest = token(prg);
		std::string fmt = token(prg);
		
		if (dest == "" || fmt == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected string_format parameters on line " + itos(get_line_num(prg)));
		}

		int di = find_variable_index(prg, dest);

		std::string fmts;

		if (fmt[0] == '"') {
			fmts = remove_quotes(unescape(fmt));
		}
		else {
			int index = -1;
			for (size_t i = 0; i < prg.variables.size(); i++) {
				if (prg.variables[i].name == fmt) {
					index = i;
					break;
				}
			}
			if (index < 0) {
				throw PARSE_EXCEPTION(prg.name + ": " + "Unknown variable \"" + fmt + "\" on line " + itos(get_line_num(prg)));
			}

			VARIABLE &v1 = prg.variables[index];

			if (v1.type == VARIABLE::STRING) {
				fmts = v1.s;
			}
			else {
				throw PARSE_EXCEPTION(prg.name + ": " + "Invalid type on line " + itos(get_line_num(prg)));
			}
		}

		int prev = 0;
		int arg_count = 0;

		for (int i = 0; i < fmts.length(); i++) {
			if (fmts[i] == '%' && prev != '%') {
				arg_count++;
			}
			prev = fmts[i];
		}

		std::string result;
		int c = 0;
		prev = 0;

		for (int arg = 0; arg < arg_count; arg++) {
			int start = c;
			while (c < fmts.length()) {
				if (fmts[c] == '%' && prev != '%') {
					break;
				}
				prev = fmts[c];
				c++;
			}

			result += fmts.substr(start, c-start);

			std::string param = token(prg);

			if (param == "") {
				throw PARSE_EXCEPTION(prg.name + ": " + "Expected string_format parameters on line " + itos(get_line_num(prg)));
			}

			std::string val;

			if (param[0] == '-' || isdigit(param[0])) {
				val = param;
			}
			else if (param[0] == '"') {
				val = remove_quotes(unescape(param));
			}
			else {
				int index = -1;
				for (size_t i = 0; i < prg.variables.size(); i++) {
					if (prg.variables[i].name == param) {
						index = i;
						break;
					}
				}
				if (index < 0) {
					throw PARSE_EXCEPTION(prg.name + ": " + "Unknown variable \"" + param + "\" on line " + itos(get_line_num(prg)));
				}
				VARIABLE &v1 = prg.variables[index];
				if (v1.type == VARIABLE::NUMBER) {
					char buf[1000];
					snprintf(buf, 1000, "%g", v1.n);
					val = buf;
				}
				else if (v1.type == VARIABLE::STRING) {
					val = v1.s;
				}
				else if (v1.type == VARIABLE::VECTOR) {
					val = "-vector-";
				}
				else {
					val = "-unknown-";
				}
			}

			result += val;

			c++;
		}

		if (c < fmts.length()) {
			result += fmts.substr(c);
		}

		VARIABLE &v1 = prg.variables[di];
		v1.type = VARIABLE::STRING;
		v1.s = result;
	}
	else if (tok == "cfg_load") {
		cfg_numbers.clear();
		cfg_strings.clear();

		std::string found = token(prg);
		std::string cfg_name = token(prg);

		if (found == "" || cfg_name == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected cfg_load parameters on line " + itos(get_line_num(prg)));
		}

		std::string cfg_names;

		if (cfg_name[0] == '"') {
			cfg_names = remove_quotes(unescape(cfg_name));
		}
		else  {
			int index = -1;
			for (size_t i = 0; i < prg.variables.size(); i++) {
				if (prg.variables[i].name == cfg_name) {
					index = i;
					break;
				}
			}
			if (index < 0) {
				throw PARSE_EXCEPTION(prg.name + ": " + "Unknown variable \"" + cfg_name + "\" on line " + itos(get_line_num(prg)));
			}
			VARIABLE &v1 = prg.variables[index];
			if (v1.type == VARIABLE::STRING) {
				cfg_names = v1.s;
			}
			else {
				throw PARSE_EXCEPTION(prg.name + ": " + "Invalid type on line " + itos(get_line_num(prg)));
			}
		}

		bool found_cfg = load_cfg(cfg_names);
		
		int di = find_variable_index(prg, found);

		VARIABLE &v1 = prg.variables[di];

		if (v1.type == VARIABLE::NUMBER) {
			v1.n = found_cfg;
		}
		else {
			throw PARSE_EXCEPTION(prg.name + ": " + "Invalid type on line " + itos(get_line_num(prg)));
		}
	}
	else if (tok == "cfg_save") {
		std::string cfg_name = token(prg);

		if (cfg_name == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected cfg_save parameters on line " + itos(get_line_num(prg)));
		}

		std::string cfg_names;

		if (cfg_name[0] == '"') {
			cfg_names = remove_quotes(unescape(cfg_name));
		}
		else  {
			int index = -1;
			for (size_t i = 0; i < prg.variables.size(); i++) {
				if (prg.variables[i].name == cfg_name) {
					index = i;
					break;
				}
			}
			if (index < 0) {
				throw PARSE_EXCEPTION(prg.name + ": " + "Unknown variable \"" + cfg_name + "\" on line " + itos(get_line_num(prg)));
			}
			VARIABLE &v1 = prg.variables[index];
			if (v1.type == VARIABLE::STRING) {
				cfg_names = v1.s;
			}
			else {
				throw PARSE_EXCEPTION(prg.name + ": " + "Invalid type on line " + itos(get_line_num(prg)));
			}
		}

		save_cfg(cfg_names);
	}
	else if (tok == "cfg_get_number") {
		std::string dest = token(prg);
		std::string name = token(prg);

		if (dest == "" || name == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected cfg_get_number parameters on line " + itos(get_line_num(prg)));
		}

		std::string names;

		if (name[0] == '"') {
			names = remove_quotes(unescape(name));
		}
		else  {
			int index = -1;
			for (size_t i = 0; i < prg.variables.size(); i++) {
				if (prg.variables[i].name == name) {
					index = i;
					break;
				}
			}
			if (index < 0) {
				throw PARSE_EXCEPTION(prg.name + ": " + "Unknown variable \"" + name + "\" on line " + itos(get_line_num(prg)));
			}
			VARIABLE &v1 = prg.variables[index];
			if (v1.type == VARIABLE::STRING) {
				names = v1.s;
			}
			else {
				throw PARSE_EXCEPTION(prg.name + ": " + "Invalid type on line " + itos(get_line_num(prg)));
			}
		}

		int di = find_variable_index(prg, dest);

		if (prg.variables[di].type != VARIABLE::NUMBER) {
			throw PARSE_EXCEPTION(prg.name + ": " + "Invalid type on line " + itos(get_line_num(prg)));
		}

		prg.variables[di].n = cfg_numbers[names];
	}
	else if (tok == "cfg_get_string") {
		std::string dest = token(prg);
		std::string name = token(prg);

		if (dest == "" || name == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected cfg_get_string parameters on line " + itos(get_line_num(prg)));
		}

		std::string names;

		if (name[0] == '"') {
			names = remove_quotes(unescape(name));
		}
		else  {
			int index = -1;
			for (size_t i = 0; i < prg.variables.size(); i++) {
				if (prg.variables[i].name == name) {
					index = i;
					break;
				}
			}
			if (index < 0) {
				throw PARSE_EXCEPTION(prg.name + ": " + "Unknown variable \"" + name + "\" on line " + itos(get_line_num(prg)));
			}
			VARIABLE &v1 = prg.variables[index];
			if (v1.type == VARIABLE::STRING) {
				names = v1.s;
			}
			else {
				throw PARSE_EXCEPTION(prg.name + ": " + "Invalid type on line " + itos(get_line_num(prg)));
			}
		}

		int di = find_variable_index(prg, dest);

		if (prg.variables[di].type != VARIABLE::STRING) {
			throw PARSE_EXCEPTION(prg.name + ": " + "Invalid type on line " + itos(get_line_num(prg)));
		}

		prg.variables[di].s = cfg_strings[names];
	}
	else if (tok == "cfg_set_number") {
		std::string name = token(prg);
		std::string value = token(prg);

		if (value == "" || name == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected cfg_set_number parameters on line " + itos(get_line_num(prg)));
		}

		std::string names;

		if (name[0] == '"') {
			names = remove_quotes(unescape(name));
		}
		else  {
			int index = -1;
			for (size_t i = 0; i < prg.variables.size(); i++) {
				if (prg.variables[i].name == name) {
					index = i;
					break;
				}
			}
			if (index < 0) {
				throw PARSE_EXCEPTION(prg.name + ": " + "Unknown variable \"" + name + "\" on line " + itos(get_line_num(prg)));
			}
			VARIABLE &v1 = prg.variables[index];
			if (v1.type == VARIABLE::STRING) {
				names = v1.s;
			}
			else {
				throw PARSE_EXCEPTION(prg.name + ": " + "Invalid type on line " + itos(get_line_num(prg)));
			}
		}

		std::vector<std::string> strings;
		strings.push_back(value);
		std::vector<double> values = variable_names_to_numbers(prg, strings);

		cfg_numbers[names] = values[0];
	}
	else if (tok == "cfg_set_string") {
		std::string name = token(prg);
		std::string value = token(prg);

		if (value == "" || name == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected cfg_set_string parameters on line " + itos(get_line_num(prg)));
		}

		std::string names;

		if (name[0] == '"') {
			names = remove_quotes(unescape(name));
		}
		else  {
			int index = -1;
			for (size_t i = 0; i < prg.variables.size(); i++) {
				if (prg.variables[i].name == name) {
					index = i;
					break;
				}
			}
			if (index < 0) {
				throw PARSE_EXCEPTION(prg.name + ": " + "Unknown variable \"" + name + "\" on line " + itos(get_line_num(prg)));
			}
			VARIABLE &v1 = prg.variables[index];
			if (v1.type == VARIABLE::STRING) {
				names = v1.s;
			}
			else {
				throw PARSE_EXCEPTION(prg.name + ": " + "Invalid type on line " + itos(get_line_num(prg)));
			}
		}

		std::string values;

		if (value[0] == '"') {
			values = remove_quotes(unescape(value));
		}
		else  {
			int index = -1;
			for (size_t i = 0; i < prg.variables.size(); i++) {
				if (prg.variables[i].name == value) {
					index = i;
					break;
				}
			}
			if (index < 0) {
				throw PARSE_EXCEPTION(prg.name + ": " + "Unknown variable \"" + value + "\" on line " + itos(get_line_num(prg)));
			}
			VARIABLE &v1 = prg.variables[index];
			if (v1.type == VARIABLE::STRING) {
				values = v1.s;
			}
			else {
				throw PARSE_EXCEPTION(prg.name + ": " + "Invalid type on line " + itos(get_line_num(prg)));
			}
		}

		cfg_strings[names] = values;
	}
	else if (tok == "cfg_number_exists") {
		std::string dest = token(prg);
		std::string name = token(prg);

		if (dest == "" || name == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected cfg_number_exists parameters on line " + itos(get_line_num(prg)));
		}

		std::string names;

		if (name[0] == '"') {
			names = remove_quotes(unescape(name));
		}
		else  {
			int index = -1;
			for (size_t i = 0; i < prg.variables.size(); i++) {
				if (prg.variables[i].name == name) {
					index = i;
					break;
				}
			}
			if (index < 0) {
				throw PARSE_EXCEPTION(prg.name + ": " + "Unknown variable \"" + name + "\" on line " + itos(get_line_num(prg)));
			}
			VARIABLE &v1 = prg.variables[index];
			if (v1.type == VARIABLE::STRING) {
				names = v1.s;
			}
			else {
				throw PARSE_EXCEPTION(prg.name + ": " + "Invalid type on line " + itos(get_line_num(prg)));
			}
		}

		int di = find_variable_index(prg, dest);

		int found_n;

		if (cfg_numbers.find(names) == cfg_numbers.end()) {
			found_n = 0;
		}
		else {
			found_n = 1;
		}

		VARIABLE &v1 = prg.variables[di];

		if (v1.type == VARIABLE::NUMBER) {
			v1.n = found_n;
		}
		else {
			throw PARSE_EXCEPTION(prg.name + ": " + "Invalid type on line " + itos(get_line_num(prg)));
		}
	}
	else if (tok == "cfg_string_exists") {
		std::string dest = token(prg);
		std::string name = token(prg);

		if (dest == "" || name == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected cfg_string_exists parameters on line " + itos(get_line_num(prg)));
		}

		std::string names;

		if (name[0] == '"') {
			names = remove_quotes(unescape(name));
		}
		else  {
			int index = -1;
			for (size_t i = 0; i < prg.variables.size(); i++) {
				if (prg.variables[i].name == name) {
					index = i;
					break;
				}
			}
			if (index < 0) {
				throw PARSE_EXCEPTION(prg.name + ": " + "Unknown variable \"" + name + "\" on line " + itos(get_line_num(prg)));
			}
			VARIABLE &v1 = prg.variables[index];
			if (v1.type == VARIABLE::STRING) {
				names = v1.s;
			}
			else {
				throw PARSE_EXCEPTION(prg.name + ": " + "Invalid type on line " + itos(get_line_num(prg)));
			}
		}

		int di = find_variable_index(prg, dest);

		int found_n;

		if (cfg_strings.find(names) == cfg_strings.end()) {
			found_n = 0;
		}
		else {
			found_n = 1;
		}

		VARIABLE &v1 = prg.variables[di];

		if (v1.type == VARIABLE::NUMBER) {
			v1.n = found_n;
		}
		else {
			throw PARSE_EXCEPTION(prg.name + ": " + "Invalid type on line " + itos(get_line_num(prg)));
		}
	}
	else if (tok == "reset") {
		std::string name = token(prg);

		if (name == "") {
			throw PARSE_EXCEPTION(prg.name + ": " + "Expected reset parameters on line " + itos(get_line_num(prg)));
		}

		std::string names;

		if (name[0] == '"') {
			names = remove_quotes(unescape(name));
		}
		else {
			int index = -1;
			for (size_t i = 0; i < prg.variables.size(); i++) {
				if (prg.variables[i].name == name) {
					index = i;
					break;
				}
			}
			if (index < 0) {
				throw PARSE_EXCEPTION(prg.name + ": " + "Unknown variable \"" + name + "\" on line " + itos(get_line_num(prg)));
			}
			VARIABLE &v1 = prg.variables[index];
			if (v1.type == VARIABLE::STRING) {
				names = v1.s;
			}
			else {
				throw PARSE_EXCEPTION(prg.name + ": " + "Invalid type on line " + itos(get_line_num(prg)));
			}
		}

		reset_game_name = names;

		return false;
	}
	else {
		throw PARSE_EXCEPTION(prg.name + ": " + "Invalid token \"" + tok + "\" on line " + itos(get_line_num(prg)));
	}

	return true;
}

void destroy_program(PROGRAM &prg, bool destroy_vectors)
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

	if (destroy_vectors) {
		prg.vectors.clear();
	}

	prg.variables.clear();
	prg.functions.clear();
	prg.labels.clear();
}
