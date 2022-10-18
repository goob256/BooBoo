#include <shim4/shim4.h>

#include "booboo/booboo.h"
#include "booboo/internal/booboo.h"

extern bool quit;

using namespace booboo;

static std::map< std::string, double > cfg_numbers;
static std::map< std::string, std::string > cfg_strings;

static Variable &as_variable(Program &prg, Token &t)
{
	return prg.variables[t.s];
}

static double as_number(Program &prg, Token &t)
{
	if (t.type == Token::NUMBER) {
		return t.n;
	}
	else if (t.type == Token::SYMBOL) {
		Variable &v = prg.variables[t.s];
		if (v.type == Variable::NUMBER) {
			return v.n;
		}
		else {
			return atof(v.s.c_str());
		}
	}
	else {
		return atof(t.s.c_str());
	}
}

static std::string as_string(Program &prg, Token &t)
{
	if (t.type == Token::NUMBER) {
		char buf[1000];
		snprintf(buf, 1000, "%g", t.n);
		return buf;
	}
	else if (t.type == Token::SYMBOL) {
		Variable &v = prg.variables[t.s];
		if (v.type == Variable::STRING) {
			return v.s;
		}
		else {
			char buf[1000];
			snprintf(buf, 1000, "%g", v.n);
			return buf;
		}
	}
	else {
		return t.s;
	}
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
	catch (util::Error &e) {
		return false;
	}

	util::Tokenizer t(text, '\n');

	std::string line;

	while ((line = t.next()) != "") {
		util::Tokenizer t2(line, '=');
		std::string name = t2.next();
		std::string value = t2.next();
		util::trim(value);

		if (name == "") {
			continue;
		}

		if (name[0] == '"') {
			cfg_strings[name] = remove_quotes(value);
		}
		else {
			cfg_numbers[name] = atof(value.c_str());
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
		fprintf(f, "%s=%g\n", p.first.c_str(), p.second);
	}

	for (std::map<std::string, std::string>::iterator it = cfg_strings.begin(); it != cfg_strings.end(); it++) {
		std::pair<std::string, std::string> p = *it;
		fprintf(f, "%s=\"%s\"\n", p.first.c_str(), p.second.c_str());
	}

	fclose(f);
}

static bool breaker_reset(Program &prg, std::vector<Token> &v)
{
	std::string name = as_string(prg, v[0]);

	reset_game_name = name;

	return false;
}

static bool breaker_exit(Program &prg, std::vector<Token> &v)
{
	/*
	std::string code = v[0];
	std::vector<std::string> strings;
	strings.reserve(1);
	strings.push_back(code);
	std::vector<double> values = variable_names_to_numbers(prg, strings);
	return_code = values[0];
	*/
	return_code = as_number(prg, v[0]);
	reset_game_name = "";
	quit = true;
	return false;
}

static bool breaker_return(Program &prg, std::vector<Token> &v)
{
	if (v[0].type == Token::NUMBER) {
		prg.result.type = Variable::NUMBER;
		prg.result.n = v[0].n;
	}
	else if (v[0].type == Token::SYMBOL) {
		std::map<std::string, Variable>::iterator it;
		it = prg.variables.find(v[0].s);
		if (it != prg.variables.end()) {
			prg.result = (*it).second;
		}
		else {
			throw util::ParseError("Unknown var " + v[0].token);
		}
	}
	else {
		prg.result.type = Variable::STRING;
		prg.result.s = v[0].s;
	}

	prg.result.name = "result";

	return false;
}

static bool corefunc_var(Program &prg, std::vector<Token> &v)
{
	Variable var;

	std::string type = v[0].token;
	std::string name =  v[1].token;

	var.name = name;
	var.function = prg.name;

	if (type == "number") {
		var.type = Variable::NUMBER;
	}
	else if (type == "string") {
		var.type = Variable::STRING;
	}
	else if (type == "vector") {
		var.type = Variable::VECTOR;
	}
	else {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
	}

	std::map<std::string, Variable>::iterator it;
	if ((it = prg.variables.find(var.name)) != prg.variables.end()) {
		Variable &v = (*it).second;
		if (v.function != prg.name) {
			std::map<std::string, Variable> &variables_backup = prg.variables_backup_stack.top();
			variables_backup[v.name] = v;
		}
	}

	prg.variables[var.name] = var;

	return true;
}

static bool corefunc_set(Program &prg, std::vector<Token> &v)
{
	//std::string dest = v[0];
	//std::string src = v[1];

	//Variable &v1 = find_variable(prg, dest);

	Variable &v1 = as_variable(prg, v[0]);

	if (v[1].type == Token::NUMBER) {
		if (v1.type == Variable::NUMBER) {
			v1.n = v[1].n;
		}
		else if (v1.type == Variable::STRING) {
			char buf[1000];
			snprintf(buf, 1000, "%g", v[1].n);
			v1.s = buf;
		}
		else {
			throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
		}
	}
	else if (v[1].type == Token::STRING || v[1].type == Token::SPECIAL) {
		if (v1.type == Variable::NUMBER) {
			v1.n = atof(v[1].s.c_str());
		}
		else if (v1.type == Variable::STRING) {
			v1.s = v[1].s;
		}
		else {
			throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
		}
	}
	else {
		std::map<std::string, Variable>::iterator it;
		it = prg.variables.find(v[1].s);
		if (it != prg.variables.end()) {
			Variable &v2 = (*it).second;

			if (v1.type == Variable::NUMBER && v2.type == Variable::NUMBER) {
				v1.n = v2.n;
			}
			else if (v1.type == Variable::STRING && v2.type == Variable::NUMBER) {
				v1.s = util::itos(v2.n);
			}
			else if (v1.type == Variable::STRING && v2.type == Variable::STRING) {
				v1.s = v2.s;
			}
			else if (v1.type == Variable::VECTOR && v2.type == Variable::VECTOR) {
				v1.v = v2.v;
			}
			else {
				throw util::ParseError(std::string(__FUNCTION__) + ": " + "Operation undefined for operands on line " + util::itos(get_line_num(prg)));
			}
		}
		else {
			throw util::ParseError("Uknown variable " + v[1].token);
		}
	}

	return true;
}

static bool corefunc_add(Program &prg, std::vector<Token> &v)
{
	Variable &v1 = as_variable(prg, v[0]);
	double d = as_number(prg, v[1]);

	if (v1.type == Variable::NUMBER) {
		v1.n += d;
	}
	else if (v1.type == Variable::STRING) {
		char buf[1000];
		snprintf(buf, 1000, "%g", d);
		v1.s += buf;
	}
	else {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
	}

	return true;
}

static bool corefunc_subtract(Program &prg, std::vector<Token> &v)
{
	Variable &v1 = as_variable(prg, v[0]);
	double d = as_number(prg, v[1]);

	if (v1.type == Variable::NUMBER) {
		v1.n -= d;
	}
	else {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
	}

	return true;
}

static bool corefunc_multiply(Program &prg, std::vector<Token> &v)
{
	Variable &v1 = as_variable(prg, v[0]);
	double d = as_number(prg, v[1]);

	if (v1.type == Variable::NUMBER) {
		v1.n *= d;
	}
	else {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
	}

	return true;
}

static bool corefunc_divide(Program &prg, std::vector<Token> &v)
{
	Variable &v1 = as_variable(prg, v[0]);
	double d = as_number(prg, v[1]);

	if (v1.type == Variable::NUMBER) {
		v1.n /= d;
	}
	else {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
	}

	return true;
}

static bool corefunc_intmod(Program &prg, std::vector<Token> &v)
{
	Variable &v1 = as_variable(prg, v[0]);
	int d = as_number(prg, v[1]);

	if (v1.type == Variable::NUMBER) {
		v1.n = int(v1.n) % int(d);
	}
	else {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
	}

	return true;
}

static bool corefunc_fmod(Program &prg, std::vector<Token> &v)
{
	Variable &v1 = as_variable(prg, v[0]);
	double d = as_number(prg, v[1]);

	if (v1.type == Variable::NUMBER) {
		v1.n = fmod(v1.n, d);
	}
	else {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
	}

	return true;
}

static bool corefunc_neg(Program &prg, std::vector<Token> &v)
{
	Variable &v1 = as_variable(prg, v[0]);

	if (v1.type == Variable::NUMBER) {
		v1.n = -v1.n;
	}
	else {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Operation undefined for operands on line " + util::itos(get_line_num(prg)));
	}

	return true;
}

/*
static bool corefunc_label(Program &prg, std::vector<Token> &v)
{
	std::string name = as_string(prg, v[0]);

	if (name[0] != '_' && isalpha(name[0]) == false) {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid label name on line " + util::itos(get_line_num(prg)));
	}

	Label l;
	l.name = name;
	l.pc = prg.pc;
	
	//prg.labels.push_back(l);
	//already got these

	return true;
}
*/

static bool corefunc_goto(Program &prg, std::vector<Token> &v)
{
	std::string name = v[0].token;

	std::map<std::string, Label>::iterator it = prg.labels.find(name);
	if (it != prg.labels.end()) {
		Label &l = (*it).second;
		prg.pc = l.pc;
	}

	/*
	for (size_t i = 0; i < prg.labels.size(); i++) {
		if (prg.labels[i].name == name) {
			prg.p = prg.labels[i].p;
			prg.line = prg.labels[i].line;
		}
	}
	*/

	return true;
}

static bool corefunc_compare(Program &prg, std::vector<Token> &v)
{
	bool a_string = false;
	bool b_string = false;
	std::string s1;
	std::string s2;

	if (v[0].type == Token::STRING || v[0].type == Token::SPECIAL) {
		a_string = true;
		s1 = v[0].s;
	}
	else if (v[0].type == Token::SYMBOL) {
		Variable &var = as_variable(prg, v[0]);
		if (var.type == Variable::STRING) {
			a_string = true;
			s1 = var.s;
		}
	}
	
	if (v[1].type == Token::STRING || v[1].type == Token::SPECIAL) {
		b_string = true;
		s2 = v[1].s;
	}
	else if (v[1].type == Token::SYMBOL) {
		Variable &var = as_variable(prg, v[1]);
		if (var.type == Variable::STRING) {
			b_string = true;
			s2 = var.s;
		}
	}
	
	if (a_string && b_string) {
		prg.compare_flag = strcmp(s1.c_str(), s2.c_str());
		//throw util::ParseError(util::itos(prg.compare_flag) + " " + a + " " + b + " " + s1 + " " + s2);
	}
	else if (a_string || b_string) {
		prg.compare_flag = 0;
	}
	// FIXME: if they're vectors or something then it should report an error
	else {
		double ad = as_number(prg, v[0]);
		double bd = as_number(prg, v[1]);

		if (ad < bd) {
			prg.compare_flag = -1;
		}
		else if (ad == bd) {
			prg.compare_flag = 0;
		}
		else {
			prg.compare_flag = 1;
		}
	}

	return true;
}

static bool corefunc_je(Program &prg, std::vector<Token> &v)
{
	std::string label = v[0].token;

	if (prg.compare_flag == 0) {
		std::map<std::string, Label>::iterator it = prg.labels.find(label);
		if (it != prg.labels.end()) {
			Label &l = (*it).second;
			prg.pc = l.pc;
		}
	}

	return true;
}

static bool corefunc_jne(Program &prg, std::vector<Token> &v)
{
	std::string label = v[0].token;

	if (prg.compare_flag != 0) {
		std::map<std::string, Label>::iterator it = prg.labels.find(label);
		if (it != prg.labels.end()) {
			Label &l = (*it).second;
			prg.pc = l.pc;
		}
	}

	return true;
}

static bool corefunc_jl(Program &prg, std::vector<Token> &v)
{
	std::string label = v[0].token;

	if (prg.compare_flag < 0) {
		std::map<std::string, Label>::iterator it = prg.labels.find(label);
		if (it != prg.labels.end()) {
			Label &l = (*it).second;
			prg.pc = l.pc;
		}
	}

	return true;
}

static bool corefunc_jle(Program &prg, std::vector<Token> &v)
{
	std::string label = v[0].token;

	if (prg.compare_flag <= 0) {
		std::map<std::string, Label>::iterator it = prg.labels.find(label);
		if (it != prg.labels.end()) {
			Label &l = (*it).second;
			prg.pc = l.pc;
		}
	}

	return true;
}

static bool corefunc_jg(Program &prg, std::vector<Token> &v)
{
	std::string label = v[0].token;

	if (prg.compare_flag > 0) {
		std::map<std::string, Label>::iterator it = prg.labels.find(label);
		if (it != prg.labels.end()) {
			Label &l = (*it).second;
			prg.pc = l.pc;
		}
	}

	return true;
}

static bool corefunc_jge(Program &prg, std::vector<Token> &v)
{
	std::string label = v[0].token;

	if (prg.compare_flag >= 0) {
		std::map<std::string, Label>::iterator it = prg.labels.find(label);
		if (it != prg.labels.end()) {
			Label &l = (*it).second;
			prg.pc = l.pc;
		}
	}

	return true;
}

static bool corefunc_call(Program &prg, std::vector<Token> &v)
{
	std::string tok2 = v[0].token;
	std::string function_name;
	std::string result_name;
	int _tok = 1;

	if (tok2 == ">") {
		result_name = v[_tok++].token;
		function_name = v[_tok++].token;
	}
	else {
		function_name = tok2;
	}

	std::vector<std::string> params;
	for (size_t i = _tok; i < v.size(); i++) {
		params.push_back(v[i].token);
	}

	call_function(prg, function_name, params, result_name);

	return true;
}

/*
static bool corefunc_function(Program &prg, std::vector<Token> &v)
{
	int start_line = prg.line;
	std::string name = v[0];
	int save = get_line_num(prg);
	int _tok = 1;

	Program p;
	
	std::string tok2;

	while ((tok2 = v[_tok++]) != "") {
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

	while ((tok2 = v[_tok++]) != "") {
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
	//prg.functions.push_back(p);

	return true;
}

static bool corefunc_comment(Program &prg, std::vector<Token> &v)
{
	while (prg.p < prg.code.length() && prg.code[prg.p] != '\n') {
		prg.p++;
	}
	prg.line++;
	if (prg.p < prg.code.length()) {
		prg.p++;
	}

	return true;
}
*/

static bool corefunc_inspect(Program &prg, std::vector<Token> &v)
{
	char buf[1000];

	if (v[0].type == Token::NUMBER) {
		snprintf(buf, 1000, "%g", v[0].n);
	}
	else if (v[0].type == Token::SYMBOL) {
		std::map<std::string, Variable>::iterator it;
		it = prg.variables.find(v[0].s);
		if (it != prg.variables.end()) {
			Variable &var = (*it).second;
			if (var.type == Variable::NUMBER) {
				snprintf(buf, 1000, "%g", var.n);
			}
			else if (var.type == Variable::STRING) {
				snprintf(buf, 1000, "%s", var.s.c_str());
			}
			else if (var.type == Variable::VECTOR) {
				snprintf(buf, 1000, "-vector-");
			}
		}
	}
	else {
		strcpy(buf, "Unknown");
	}

	gui::popup("INSPECTOR", buf, gui::OK);

	return true;
}

static bool corefunc_string_format(Program &prg, std::vector<Token> &v)
{
	Variable &v1 = as_variable(prg, v[0]);
	std::string fmt = as_string(prg, v[1]);
	int _tok = 2;
	
	int prev = 0;
	int arg_count = 0;

	for (size_t i = 0; i < fmt.length(); i++) {
		if (fmt[i] == '%' && prev != '%') {
			arg_count++;
		}
		prev = fmt[i];
	}

	std::string result;
	int c = 0;
	prev = 0;

	for (int arg = 0; arg < arg_count; arg++) {
		int start = c;
		while (c < (int)fmt.length()) {
			if (fmt[c] == '%' && prev != '%') {
				break;
			}
			prev = fmt[c];
			c++;
		}

		result += fmt.substr(start, c-start);

		std::string param = v[_tok++].token;

		std::string val;

		if (param[0] == '-' || isdigit(param[0])) {
			val = param;
		}
		else if (param[0] == '"') {
			val = remove_quotes(util::unescape_string(param));
		}
		else {
			Variable &v1 = as_variable(prg, v[_tok-1]);
			if (v1.type == Variable::NUMBER) {
				char buf[1000];
				snprintf(buf, 1000, "%g", v1.n);
				val = buf;
			}
			else if (v1.type == Variable::STRING) {
				val = v1.s;
			}
			else if (v1.type == Variable::VECTOR) {
				val = "-vector-";
			}
			else {
				val = "-unknown-";
			}
		}

		result += val;

		c++;
	}

	if (c < (int)fmt.length()) {
		result += fmt.substr(c);
	}

	v1.type = Variable::STRING;
	v1.s = result;

	return true;
}

static bool mathfunc_sin(Program &prg, std::vector<Token> &v)
{
	Variable &v1 = as_variable(prg, v[0]);

	if (v1.type == Variable::NUMBER) {
		v1.n = sin(as_number(prg, v[1]));
	}
	else {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Operation undefined for operands on line " + util::itos(get_line_num(prg)));
	}

	return true;
}

static bool mathfunc_cos(Program &prg, std::vector<Token> &v)
{
	Variable &v1 = as_variable(prg, v[0]);

	if (v1.type == Variable::NUMBER) {
		v1.n = cos(as_number(prg, v[1]));
	}
	else {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Operation undefined for operands on line " + util::itos(get_line_num(prg)));
	}

	return true;
}

static bool mathfunc_atan2(Program &prg, std::vector<Token> &v)
{
	Variable &v1 = as_variable(prg, v[0]);

	if (v1.type == Variable::NUMBER) {
		v1.n = atan2(as_number(prg, v[1]), as_number(prg, v[2]));
	}
	else {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Operation undefined for operands on line " + util::itos(get_line_num(prg)));
	}

	return true;
}

static bool mathfunc_abs(Program &prg, std::vector<Token> &v)
{
	Variable &v1 = as_variable(prg, v[0]);

	if (v1.type == Variable::NUMBER) {
		v1.n = fabs(as_number(prg, v[1]));
	}
	else {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Operation undefined for operands on line " + util::itos(get_line_num(prg)));
	}

	return true;
}

static bool mathfunc_pow(Program &prg, std::vector<Token> &v)
{
	Variable &v1 = as_variable(prg, v[0]);

	if (v1.type == Variable::NUMBER) {
		v1.n = pow(as_number(prg, v[1]), as_number(prg, v[2]));
	}
	else {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Operation undefined for operands on line " + util::itos(get_line_num(prg)));
	}

	return true;
}

static bool mathfunc_sqrt(Program &prg, std::vector<Token> &v)
{
	Variable &v1 = as_variable(prg, v[0]);

	if (v1.type == Variable::NUMBER) {
		v1.n = sqrt(as_number(prg, v[1]));
	}
	else {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Operation undefined for operands on line " + util::itos(get_line_num(prg)));
	}

	return true;
}

static bool mathfunc_rand(Program &prg, std::vector<Token> &v)
{
	Variable &v1 = as_variable(prg, v[0]);

	int min_incl = as_number(prg, v[1]);
	int max_incl = as_number(prg, v[2]);

	if (v1.type == Variable::NUMBER) {
		v1.n = util::rand(min_incl, max_incl);
	}
	else if (v1.type == Variable::STRING) {
		v1.s = util::itos(util::rand(min_incl, max_incl));
	}
	else {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Operation undefined for operands on line " + util::itos(get_line_num(prg)));
	}

	return true;
}

static bool gfxfunc_clear(Program &prg, std::vector<Token> &v)
{
	SDL_Colour c;
	c.r = as_number(prg, v[0]);
	c.g = as_number(prg, v[1]);
	c.b = as_number(prg, v[2]);
	c.a = 255;

	gfx::clear(c);

	return true;
}

static bool primfunc_start_primitives(Program &prg, std::vector<Token> &v)
{
	gfx::draw_primitives_start();

	return true;
}

static bool primfunc_end_primitives(Program &prg, std::vector<Token> &v)
{
	gfx::draw_primitives_end();

	return true;
}

static bool primfunc_line(Program &prg, std::vector<Token> &v)
{
	SDL_Colour c;
	c.r = as_number(prg, v[0]);
	c.g = as_number(prg, v[1]);
	c.b = as_number(prg, v[2]);
	c.a = as_number(prg, v[3]);

	util::Point<float> p1, p2;

	p1.x = as_number(prg, v[4]);
	p1.y = as_number(prg, v[5]);
	p2.x = as_number(prg, v[6]);
	p2.y = as_number(prg, v[7]);

	float thick = as_number(prg, v[8]);

	gfx::draw_line(c, p1, p2, thick);

	return true;
}

static bool primfunc_filled_triangle(Program &prg, std::vector<Token> &v)
{
	SDL_Colour c[3];
	c[0].r = as_number(prg, v[0]);
	c[0].g = as_number(prg, v[1]);
	c[0].b = as_number(prg, v[2]);
	c[0].a = as_number(prg, v[3]);
	c[1].r = as_number(prg, v[4]);
	c[1].g = as_number(prg, v[5]);
	c[1].b = as_number(prg, v[6]);
	c[1].a = as_number(prg, v[7]);
	c[2].r = as_number(prg, v[8]);
	c[2].g = as_number(prg, v[9]);
	c[2].b = as_number(prg, v[10]);
	c[2].a = as_number(prg, v[11]);

	util::Point<float> p1, p2, p3;

	p1.x = as_number(prg, v[12]);
	p1.y = as_number(prg, v[13]);
	p2.x = as_number(prg, v[14]);
	p2.y = as_number(prg, v[15]);
	p3.x = as_number(prg, v[16]);
	p3.y = as_number(prg, v[17]);

	gfx::draw_filled_triangle(c, p1, p2, p3);

	return true;
}

static bool primfunc_rectangle(Program &prg, std::vector<Token> &v)
{
	SDL_Colour c;
	c.r = as_number(prg, v[0]);
	c.g = as_number(prg, v[1]);
	c.b = as_number(prg, v[2]);
	c.a = as_number(prg, v[3]);

	util::Point<float> p;
	util::Size<float> sz;

	p.x = as_number(prg, v[4]);
	p.y = as_number(prg, v[5]);
	sz.w = as_number(prg, v[6]);
	sz.h = as_number(prg, v[7]);

	float thick = as_number(prg, v[8]);

	gfx::draw_rectangle(c, p, sz, thick);

	return true;
}

static bool primfunc_filled_rectangle(Program &prg, std::vector<Token> &v)
{
	SDL_Colour c[4];
	c[0].r = as_number(prg, v[0]);
	c[0].g = as_number(prg, v[1]);
	c[0].b = as_number(prg, v[2]);
	c[0].a = as_number(prg, v[3]);
	c[1].r = as_number(prg, v[4]);
	c[1].g = as_number(prg, v[5]);
	c[1].b = as_number(prg, v[6]);
	c[1].a = as_number(prg, v[7]);
	c[2].r = as_number(prg, v[8]);
	c[2].g = as_number(prg, v[9]);
	c[2].b = as_number(prg, v[10]);
	c[2].a = as_number(prg, v[11]);
	c[3].r = as_number(prg, v[12]);
	c[3].g = as_number(prg, v[13]);
	c[3].b = as_number(prg, v[14]);
	c[3].a = as_number(prg, v[15]);

	util::Point<float> p;

	p.x = as_number(prg, v[16]);
	p.y = as_number(prg, v[17]);

	util::Size<float> sz;

	sz.w = as_number(prg, v[18]);
	sz.h = as_number(prg, v[19]);

	gfx::draw_filled_rectangle(c, p, sz);

	return true;
}

static bool primfunc_ellipse(Program &prg, std::vector<Token> &v)
{
	SDL_Colour c;
	c.r = as_number(prg, v[0]);
	c.g = as_number(prg, v[1]);
	c.b = as_number(prg, v[2]);
	c.a = as_number(prg, v[3]);

	util::Point<float> p;

	p.x = as_number(prg, v[4]);
	p.y = as_number(prg, v[5]);

	float _rx = as_number(prg, v[6]);
	float _ry = as_number(prg, v[7]);
	float thick = as_number(prg, v[8]);
	float _sections = as_number(prg, v[9]);

	gfx::draw_ellipse(c, p, _rx, _ry, thick, _sections);

	return true;
}

static bool primfunc_filled_ellipse(Program &prg, std::vector<Token> &v)
{
	SDL_Colour c;
	c.r = as_number(prg, v[0]);
	c.g = as_number(prg, v[1]);
	c.b = as_number(prg, v[2]);
	c.a = as_number(prg, v[3]);

	util::Point<float> p;

	p.x = as_number(prg, v[4]);
	p.y = as_number(prg, v[5]);

	float _rx = as_number(prg, v[6]);
	float _ry = as_number(prg, v[7]);
	float _sections = as_number(prg, v[8]);

	gfx::draw_filled_ellipse(c, p, _rx, _ry, _sections);

	return true;
}

static bool primfunc_circle(Program &prg, std::vector<Token> &v)
{
	SDL_Colour c;
	c.r = as_number(prg, v[0]);
	c.g = as_number(prg, v[1]);
	c.b = as_number(prg, v[2]);
	c.a = as_number(prg, v[3]);

	util::Point<float> p;

	p.x = as_number(prg, v[4]);
	p.y = as_number(prg, v[5]);
	float _r = as_number(prg, v[6]);
	float thick = as_number(prg, v[7]);
	int _sections = as_number(prg, v[8]);

	gfx::draw_circle(c, p, _r, thick, _sections);

	return true;
}

static bool primfunc_filled_circle(Program &prg, std::vector<Token> &v)
{
	SDL_Colour c;
	c.r = as_number(prg, v[0]);
	c.g = as_number(prg, v[1]);
	c.b = as_number(prg, v[2]);
	c.a = as_number(prg, v[3]);

	util::Point<float> p;

	p.x = as_number(prg, v[4]);
	p.y = as_number(prg, v[5]);
	float _r = as_number(prg, v[6]);
	int _sections = as_number(prg, v[7]);

	gfx::draw_filled_circle(c, p, _r, _sections);

	return true;
}

static bool mmlfunc_create(Program &prg, std::vector<Token> &v)
{
	Variable &v1 = as_variable(prg, v[0]);
	std::string str = as_string(prg, v[1]);

	if (v1.type == Variable::NUMBER) {
		v1.n = prg.mml_id;
	}
	else if (v1.type == Variable::STRING) {
		v1.s = util::itos(prg.mml_id);
	}
	else {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
	}

	Uint8 *bytes = (Uint8 *)str.c_str();
	SDL_RWops *file = SDL_RWFromMem(bytes, str.length());
	audio::MML *mml = new audio::MML(file);
	//SDL_RWclose(file);

	prg.mmls[prg.mml_id++] = mml;

	return true;
}

static bool mmlfunc_load(Program &prg, std::vector<Token> &v)
{
	Variable &v1 = as_variable(prg, v[0]);
	std::string name = as_string(prg, v[1]);

	if (v1.type == Variable::NUMBER) {
		v1.n = prg.mml_id;
	}
	else if (v1.type == Variable::STRING) {
		v1.s = util::itos(prg.mml_id);
	}
	else {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
	}

	audio::MML *mml = new audio::MML(remove_quotes(util::unescape_string(name)));

	prg.mmls[prg.mml_id++] = mml;

	return true;
}

static bool mmlfunc_play(Program &prg, std::vector<Token> &v)
{
	int id = as_number(prg, v[0]);
	float volume = as_number(prg, v[1]);
	bool loop = as_number(prg, v[2]);

	if (id < 0 || id >= (int)prg.mmls.size()) {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid MML on line " + util::itos(get_line_num(prg)));
	}

	audio::MML *mml = prg.mmls[id];

	mml->play(volume, loop);

	return true;
}

static bool mmlfunc_stop(Program &prg, std::vector<Token> &v)
{
	int id = as_number(prg, v[0]);

	if (id < 0 || id >= (int)prg.mmls.size()) {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid MML on line " + util::itos(get_line_num(prg)));
	}

	audio::MML *mml = prg.mmls[id];

	mml->stop();

	return true;
}

static bool imagefunc_load(Program &prg, std::vector<Token> &v)
{
	Variable &v1 = as_variable(prg, v[0]);
	std::string name = as_string(prg, v[1]);

	if (v1.type == Variable::NUMBER) {
		v1.n = prg.image_id;
	}
	else if (v1.type == Variable::STRING) {
		v1.s = util::itos(prg.image_id);
	}
	else {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
	}

	gfx::Image *img = new gfx::Image(remove_quotes(util::unescape_string(name)));

	prg.images[prg.image_id++] = img;

	return true;
}

static bool imagefunc_draw(Program &prg, std::vector<Token> &v)
{
	double id = as_number(prg, v[0]);
	double r = as_number(prg, v[1]);
	double g = as_number(prg, v[2]);
	double b = as_number(prg, v[3]);
	double a = as_number(prg, v[4]);
	double x = as_number(prg, v[5]);
	double y = as_number(prg, v[6]);
	double flip_h = as_number(prg, v[7]);
	double flip_v = as_number(prg, v[8]);

	if (id < 0 || id >= prg.images.size()) {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid Image on line " + util::itos(get_line_num(prg)));
	}

	gfx::Image *img = prg.images[id];

	SDL_Colour c;
	c.r = r;
	c.g = g;
	c.b = b;
	c.a = a;

	int flags = 0;
	if (flip_h != 0.0) {
		flags |= gfx::Image::FLIP_H;
	}
	if (flip_v != 0.0) {
		flags |= gfx::Image::FLIP_V;
	}

	img->draw_tinted(c, util::Point<float>(x, y), flags);

	return true;
}

static bool imagefunc_stretch_region(Program &prg, std::vector<Token> &v)
{
	double id = as_number(prg, v[0]);
	double r = as_number(prg, v[1]);
	double g = as_number(prg, v[2]);
	double b = as_number(prg, v[3]);
	double a = as_number(prg, v[4]);
	double sx = as_number(prg, v[5]);
	double sy = as_number(prg, v[6]);
	double sw = as_number(prg, v[7]);
	double sh = as_number(prg, v[8]);
	double dx = as_number(prg, v[9]);
	double dy = as_number(prg, v[10]);
	double dw = as_number(prg, v[11]);
	double dh = as_number(prg, v[12]);
	double flip_h = as_number(prg, v[13]);
	double flip_v = as_number(prg, v[14]);
	
	if (id < 0 || id >= prg.images.size()) {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid Image on line " + util::itos(get_line_num(prg)));
	}

	gfx::Image *img = prg.images[id];

	SDL_Colour c;
	c.r = r;
	c.g = g;
	c.b = b;
	c.a = a;

	int flags = 0;
	if (flip_h != 0.0) {
		flags |= gfx::Image::FLIP_H;
	}
	if (flip_v != 0.0) {
		flags |= gfx::Image::FLIP_V;
	}

	img->stretch_region_tinted(c, util::Point<float>(sx, sy), util::Size<float>(sw, sh), util::Point<float>(dx, dy), util::Size<float>(dw, dh), flags);

	return true;
}

static bool imagefunc_draw_rotated_scaled(Program &prg, std::vector<Token> &v)
{
	double id = as_number(prg, v[0]);
	double r = as_number(prg, v[1]);
	double g = as_number(prg, v[2]);
	double b = as_number(prg, v[3]);
	double a = as_number(prg, v[4]);
	double cx = as_number(prg, v[5]);
	double cy = as_number(prg, v[6]);
	double x = as_number(prg, v[7]);
	double y = as_number(prg, v[8]);
	double angle = as_number(prg, v[9]);
	double scale_x = as_number(prg, v[10]);
	double scale_y = as_number(prg, v[11]);
	double flip_h = as_number(prg, v[12]);
	double flip_v = as_number(prg, v[13]);

	if (id < 0 || id >= prg.images.size()) {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid Image on line " + util::itos(get_line_num(prg)));
	}

	gfx::Image *img = prg.images[id];

	SDL_Colour c;
	c.r = r;
	c.g = g;
	c.b = b;
	c.a = a;

	int flags = 0;
	if (flip_h != 0.0) {
		flags |= gfx::Image::FLIP_H;
	}
	if (flip_v != 0.0) {
		flags |= gfx::Image::FLIP_V;
	}

	img->draw_tinted_rotated_scaledxy(c, util::Point<float>(cx, cy), util::Point<float>(x, y), angle, scale_x, scale_y, flags);

	return true;
}

static bool imagefunc_start(Program &prg, std::vector<Token> &v)
{
	double img = as_number(prg, v[0]);

	if (prg.images.find(img) == prg.images.end()) {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Unknown image on line " + util::itos(get_line_num(prg)));
	}

	gfx::Image *image = prg.images[img];

	image->start_batch();

	return true;
}

static bool imagefunc_end(Program &prg, std::vector<Token> &v)
{
	double img = as_number(prg, v[0]);

	if (prg.images.find(img) == prg.images.end()) {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Unknown image on line " + util::itos(get_line_num(prg)));
	}

	gfx::Image *image = prg.images[img];

	image->end_batch();

	return true;
}

static bool imagefunc_size(Program &prg, std::vector<Token> &v)
{
	double id = as_number(prg, v[0]);

	Variable &v1 = as_variable(prg, v[1]);
	Variable &v2 = as_variable(prg, v[2]);
	
	if (prg.images.find(id) == prg.images.end()) {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid image on line " + util::itos(get_line_num(prg)));
	}

	gfx::Image *img = prg.images[id];

	if (v1.type == Variable::NUMBER) {
		v1.n = img->size.w;
	}
	else {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
	}
	if (v2.type == Variable::NUMBER) {
		v2.n = img->size.h;
	}
	else {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
	}

	return true;
}

static bool fontfunc_load(Program &prg, std::vector<Token> &v)
{
	std::string name = as_string(prg, v[1]);
	int size = as_number(prg, v[2]);
	bool smooth = as_number(prg, v[3]);

	Variable &v1 = as_variable(prg, v[0]);

	if (v1.type == Variable::NUMBER) {
		v1.n = prg.font_id;
	}
	else if (v1.type == Variable::STRING) {
		v1.s = util::itos(prg.font_id);
	}
	else {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
	}

	gfx::TTF *font = new gfx::TTF(remove_quotes(util::unescape_string(name)), size, 256);
	font->set_smooth(smooth);

	prg.fonts[prg.font_id++] = font;

	return true;
}

static bool fontfunc_draw(Program &prg, std::vector<Token> &v)
{
	double id = as_number(prg, v[0]);
	double r = as_number(prg, v[1]);
	double g = as_number(prg, v[2]);
	double b = as_number(prg, v[3]);
	double a = as_number(prg, v[4]);
	std::string text = as_string(prg, v[5]);
	double x = as_number(prg, v[6]);
	double y = as_number(prg, v[7]);

	if (id < 0 || id >= prg.fonts.size()) {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid Font on line " + util::itos(get_line_num(prg)));
	}

	gfx::TTF *font = prg.fonts[id];

	SDL_Colour c;
	c.r = r;
	c.g = g;
	c.b = b;
	c.a = a;

	font->draw(c, text, util::Point<float>(x, y));

	return true;
}

static bool fontfunc_width(Program &prg, std::vector<Token> &v)
{
	double id = as_number(prg, v[0]);
	Variable &v1 = as_variable(prg, v[1]);
	std::string text = as_string(prg, v[2]);
	
	gfx::TTF *font = prg.fonts[id];

	int w = font->get_text_width(text);

	if (v1.type == Variable::NUMBER) {
		v1.n = w;
	}
	else {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
	}

	return true;
}

static bool fontfunc_height(Program &prg, std::vector<Token> &v)
{
	double id = as_number(prg, v[0]);
	Variable &v1 = as_variable(prg, v[1]);
	
	gfx::TTF *font = prg.fonts[id];

	int h = font->get_height();

	if (v1.type == Variable::NUMBER) {
		v1.n = h;
	}
	else {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
	}

	return true;
}

void set_string_or_number(Program &prg, std::string name, double value)
{
	std::map<std::string, Variable>::iterator it = prg.variables.find(name);

	Variable &v1 = (*it).second;

	if (v1.type == Variable::NUMBER) {
		v1.n = value;
	}
	else {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
	}
}

static bool joyfunc_poll(Program &prg, std::vector<Token> &v)
{
	double num = as_number(prg, v[0]);
	std::string x1 = v[1].token;
	std::string y1 = v[2].token;
	std::string x2 = v[3].token;
	std::string y2 = v[4].token;
	std::string x3 = v[5].token;
	std::string y3 = v[6].token;
	std::string l = v[7].token;
	std::string r = v[8].token;
	std::string u = v[9].token;
	std::string d = v[10].token;
	std::string a = v[11].token;
	std::string b = v[12].token;
	std::string x = v[13].token;
	std::string y = v[14].token;
	std::string lb = v[15].token;
	std::string rb = v[16].token;
	std::string ls = v[17].token;
	std::string rs = v[18].token;
	std::string back = v[19].token;
	std::string start = v[20].token;

	SDL_JoystickID id = input::get_controller_id(num);
	SDL_GameController *gc = input::get_sdl_gamecontroller(id);
	bool connected = gc != nullptr;

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
		set_string_or_number(prg, ls, 0);
		set_string_or_number(prg, rs, 0);
		set_string_or_number(prg, x3, 0);
		set_string_or_number(prg, y3, 0);
	}
	else {

		Sint16 si_x1 = SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_LEFTX);
		Sint16 si_y1 = SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_LEFTY);
		Sint16 si_x2 = SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_RIGHTX);
		Sint16 si_y2 = SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_RIGHTY);
		Sint16 si_x3 = SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
		Sint16 si_y3 = SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);

		double x1f;
		double y1f;
		double x2f;
		double y2f;
		double x3f;
		double y3f;

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

		if (si_x3 < 0) {
			x3f = si_x3 / 32768.0;
		}
		else {
			x3f = si_x3 / 32767.0;
		}

		if (si_y3 < 0) {
			y3f = si_y3 / 32768.0;
		}
		else {
			y3f = si_y3 / 32767.0;
		}

		set_string_or_number(prg, x1, x1f);
		set_string_or_number(prg, y1, y1f);
		set_string_or_number(prg, x2, x2f);
		set_string_or_number(prg, y2, y2f);
		set_string_or_number(prg, x3, x3f);
		set_string_or_number(prg, y3, y3f);

		double ab = SDL_GameControllerGetButton(gc, (SDL_GameControllerButton)TGUI_B_A);
		double bb = SDL_GameControllerGetButton(gc, (SDL_GameControllerButton)TGUI_B_B);
		double xb = SDL_GameControllerGetButton(gc, (SDL_GameControllerButton)TGUI_B_X);
		double yb = SDL_GameControllerGetButton(gc, (SDL_GameControllerButton)TGUI_B_Y);
		double lbb = SDL_GameControllerGetButton(gc, (SDL_GameControllerButton)TGUI_B_LB);
		double rbb = SDL_GameControllerGetButton(gc, (SDL_GameControllerButton)TGUI_B_RB);
		double backb = SDL_GameControllerGetButton(gc, (SDL_GameControllerButton)TGUI_B_BACK);
		double startb = SDL_GameControllerGetButton(gc, (SDL_GameControllerButton)TGUI_B_START);
		double lsb = SDL_GameControllerGetButton(gc, (SDL_GameControllerButton)TGUI_B_LS);
		double rsb = SDL_GameControllerGetButton(gc, (SDL_GameControllerButton)TGUI_B_RS);
		double _lb = SDL_GameControllerGetButton(gc, (SDL_GameControllerButton)TGUI_B_L);
		double _rb = SDL_GameControllerGetButton(gc, (SDL_GameControllerButton)TGUI_B_R);
		double ub = SDL_GameControllerGetButton(gc, (SDL_GameControllerButton)TGUI_B_U);
		double db = SDL_GameControllerGetButton(gc, (SDL_GameControllerButton)TGUI_B_D);

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
		set_string_or_number(prg, ls, lsb);
		set_string_or_number(prg, rs, rsb);
		set_string_or_number(prg, back, backb);
		set_string_or_number(prg, start, startb);
	}

	return true;
}

static bool joyfunc_count(Program &prg, std::vector<Token> &v)
{
	Variable &v1 = as_variable(prg, v[0]);

	if (v1.type == Variable::NUMBER) {
		v1.n = input::get_num_joysticks();
	}
	else {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Operation undefined for operands on line " + util::itos(get_line_num(prg)));
	}

	return true;
}

static bool vectorfunc_add(Program &prg, std::vector<Token> &v)
{
	Variable &id = as_variable(prg, v[0]);

	Variable var;

	if (v[1].type == Token::NUMBER) {
		var.type = Variable::NUMBER;
		var.function = prg.name;
		var.name = "-constant-";
		var.n = v[1].n;
	}
	else if (v[1].type == Token::SYMBOL) {
		var = as_variable(prg, v[1]);
	}
	else {
		var.type = Variable::STRING;
		var.function = prg.name;
		var.name = "-constant-";
		var.s = v[1].s;
	}

	id.v.push_back(var);

	return true;
}

static bool vectorfunc_size(Program &prg, std::vector<Token> &v)
{
	Variable &id = as_variable(prg, v[0]);
	Variable &v1 = as_variable(prg, v[1]);

	if (v1.type == Variable::NUMBER) {
		v1.n = id.v.size();
	}
	else {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
	}

	return true;
}

static bool vectorfunc_set(Program &prg, std::vector<Token> &v)
{
	Variable &id = as_variable(prg, v[0]);
	double index = as_number(prg, v[1]);
	
	if (index < 0 || index >= id.v.size()) {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid index on line " + util::itos(get_line_num(prg)));
	}

	Variable var;

	if (v[2].type == Token::NUMBER) {
		var.type = Variable::NUMBER;
		var.function = prg.name;
		var.name = "-constant-";
		var.n = v[2].n;
	}
	else if (v[2].type == Token::SYMBOL) {
		var = as_variable(prg, v[2]);
	}
	else {
		var.type = Variable::STRING;
		var.function = prg.name;
		var.name = "-constant-";
		var.s = v[2].s;
	}

	id.v[index] = var;

	return true;
}

static bool vectorfunc_insert(Program &prg, std::vector<Token> &v)
{
	Variable &id = as_variable(prg, v[0]);
	double index = as_number(prg, v[1]);

	if (index < 0 || index > id.v.size()) {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid index on line " + util::itos(get_line_num(prg)));
	}

	Variable var;

	if (v[2].type == Token::NUMBER) {
		var.type = Variable::NUMBER;
		var.function = prg.name;
		var.name = "-constant-";
		var.n = v[2].n;
	}
	else if (v[2].type == Token::SYMBOL) {
		var = as_variable(prg, v[2]);
	}
	else {
		var.type = Variable::STRING;
		var.function = prg.name;
		var.name = "-constant-";
		var.s = v[2].s;
	}

	id.v.insert(id.v.begin()+index, var);

	return true;
}

static bool vectorfunc_get(Program &prg, std::vector<Token> &v)
{
	Variable &id = as_variable(prg, v[0]);
	Variable &v1 = as_variable(prg, v[1]);
	double index = as_number(prg, v[2]);

	if (index < 0 || index >= id.v.size()) {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid index on line " + util::itos(get_line_num(prg)));
	}

	std::string bak = v1.name;
	std::string bak2 = v1.function;
	v1 = id.v[index];
	v1.name = bak;
	v1.function = bak2;

	return true;
}

static bool vectorfunc_erase(Program &prg, std::vector<Token> &v)
{
	Variable &id = as_variable(prg, v[0]);
	double index = as_number(prg, v[1]);

	if (index < 0 || index >= id.v.size()) {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid index on line " + util::itos(get_line_num(prg)));
	}

	id.v.erase(id.v.begin() + int(index));

	return true;
}

static bool vectorfunc_clear(Program &prg, std::vector<Token> &v)
{
	Variable &id = as_variable(prg, v[0]);

	id.v.clear();

	return true;
}

static bool cfgfunc_load(Program &prg, std::vector<Token> &v)
{
	cfg_numbers.clear();
	cfg_strings.clear();

	Variable &v1 = as_variable(prg, v[0]);
	std::string cfg_name = as_string(prg, v[1]);

	bool found_cfg = load_cfg(cfg_name);
	
	if (v1.type == Variable::NUMBER) {
		v1.n = found_cfg;
	}
	else {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
	}

	return true;
}

static bool cfgfunc_save(Program &prg, std::vector<Token> &v)
{
	std::string cfg_name = as_string(prg, v[0]);

	save_cfg(cfg_name);

	return true;
}

static bool cfgfunc_get_number(Program &prg, std::vector<Token> &v)
{
	Variable &v1 = as_variable(prg, v[0]);
	std::string name = as_string(prg, v[1]);

	if (v1.type != Variable::NUMBER) {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
	}

	v1.n = cfg_numbers[name];

	return true;
}

static bool cfgfunc_get_string(Program &prg, std::vector<Token> &v)
{
	Variable &v1 = as_variable(prg, v[0]);
	std::string name = as_string(prg, v[1]);

	if (v1.type != Variable::STRING) {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
	}

	v1.s = cfg_strings[name];

	return true;
}

static bool cfgfunc_set_number(Program &prg, std::vector<Token> &v)
{
	std::string name = as_string(prg, v[0]);
	double value = as_number(prg, v[1]);

	cfg_numbers[name] = value;

	return true;
}

static bool cfgfunc_set_string(Program &prg, std::vector<Token> &v)
{
	std::string name = as_string(prg, v[0]);
	std::string value = as_string(prg, v[1]);

	cfg_strings[name] = value;

	return true;
}

static bool cfgfunc_number_exists(Program &prg, std::vector<Token> &v)
{
	Variable &v1 = as_variable(prg, v[0]);
	std::string name = as_string(prg, v[1]);

	int found_n;

	if (cfg_numbers.find(name) == cfg_numbers.end()) {
		found_n = 0;
	}
	else {
		found_n = 1;
	}

	if (v1.type == Variable::NUMBER) {
		v1.n = found_n;
	}
	else {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
	}

	return true;
}

static bool cfgfunc_string_exists(Program &prg, std::vector<Token> &v)
{
	Variable &v1 = as_variable(prg, v[0]);
	std::string name = as_string(prg, v[1]);

	int found_n;

	if (cfg_strings.find(name) == cfg_strings.end()) {
		found_n = 0;
	}
	else {
		found_n = 1;
	}

	if (v1.type == Variable::NUMBER) {
		v1.n = found_n;
	}
	else {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
	}

	return true;
}

namespace booboo {

void start()
{
	init_token_map();

	add_syntax("reset", breaker_reset);
	add_syntax("exit", breaker_exit);
	add_syntax("return", breaker_return);

	add_syntax("var", corefunc_var);
	add_syntax("=", corefunc_set);
	add_syntax("+", corefunc_add);
	add_syntax("-", corefunc_subtract);
	add_syntax("*", corefunc_multiply);
	add_syntax("/", corefunc_divide);
	add_syntax("%", corefunc_intmod);
	add_syntax("fmod", corefunc_fmod);
	add_syntax("neg", corefunc_neg);
	//add_syntax(":", corefunc_label);
	add_syntax("goto", corefunc_goto);
	add_syntax("?", corefunc_compare);
	add_syntax("je", corefunc_je);
	add_syntax("jne", corefunc_jne);
	add_syntax("jl", corefunc_jl);
	add_syntax("jle", corefunc_jle);
	add_syntax("jg", corefunc_jg);
	add_syntax("jge", corefunc_jge);
	add_syntax("call", corefunc_call);
	//add_syntax("function", corefunc_function);
	//add_syntax(";", corefunc_comment);
	add_syntax("inspect", corefunc_inspect);
	add_syntax("string_format", corefunc_string_format);
	add_syntax("sin", mathfunc_sin);
	add_syntax("cos", mathfunc_cos);
	add_syntax("atan2", mathfunc_atan2);
	add_syntax("abs", mathfunc_abs);
	add_syntax("pow", mathfunc_pow);
	add_syntax("sqrt", mathfunc_sqrt);
	add_syntax("rand", mathfunc_rand);
	add_syntax("clear", gfxfunc_clear);
	add_syntax("start_primitives", primfunc_start_primitives);
	add_syntax("end_primitives", primfunc_end_primitives);
	add_syntax("line", primfunc_line);
	add_syntax("filled_triangle", primfunc_filled_triangle);
	add_syntax("rectangle", primfunc_rectangle);
	add_syntax("filled_rectangle", primfunc_filled_rectangle);
	add_syntax("ellipse", primfunc_ellipse);
	add_syntax("filled_ellipse", primfunc_filled_ellipse);
	add_syntax("circle", primfunc_circle);
	add_syntax("filled_circle", primfunc_filled_circle);
	add_syntax("mml_create", mmlfunc_create);
	add_syntax("mml_load", mmlfunc_load);
	add_syntax("mml_play", mmlfunc_play);
	add_syntax("mml_stop", mmlfunc_stop);
	add_syntax("image_load", imagefunc_load);
	add_syntax("image_draw", imagefunc_draw);
	add_syntax("image_stretch_region", imagefunc_stretch_region);
	add_syntax("image_draw_rotated_scaled", imagefunc_draw_rotated_scaled);
	add_syntax("image_start", imagefunc_start);
	add_syntax("image_end", imagefunc_end);
	add_syntax("image_size", imagefunc_size);
	add_syntax("font_load", fontfunc_load);
	add_syntax("font_draw", fontfunc_draw);
	add_syntax("font_width", fontfunc_width);
	add_syntax("font_height", fontfunc_height);
	add_syntax("joystick_poll", joyfunc_poll);
	add_syntax("joystick_count", joyfunc_count);
	add_syntax("vector_add", vectorfunc_add);
	add_syntax("vector_size", vectorfunc_size);
	add_syntax("vector_set", vectorfunc_set);
	add_syntax("vector_insert", vectorfunc_insert);
	add_syntax("vector_get", vectorfunc_get);
	add_syntax("vector_erase", vectorfunc_erase);
	add_syntax("vector_clear", vectorfunc_clear);
	add_syntax("cfg_load", cfgfunc_load);
	add_syntax("cfg_save", cfgfunc_save);
	add_syntax("cfg_get_number", cfgfunc_get_number);
	add_syntax("cfg_get_string", cfgfunc_get_string);
	add_syntax("cfg_set_number", cfgfunc_set_number);
	add_syntax("cfg_set_string", cfgfunc_set_string);
	add_syntax("cfg_number_exists", cfgfunc_number_exists);
	add_syntax("cfg_string_exists", cfgfunc_string_exists);

	return_code = 0;
}

} // end namespace booboo
