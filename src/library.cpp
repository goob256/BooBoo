#include <shim4/shim4.h>

#include "booboo/booboo.h"
#include "booboo/internal/booboo.h"

extern bool quit;

using namespace booboo;

static std::map< std::string, double > cfg_numbers;
static std::map< std::string, std::string > cfg_strings;

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

static bool breaker_reset(Program &prg, std::vector<std::string> &v)
{
	std::string name = v[0];

	std::string names;

	if (name[0] == '"') {
		names = remove_quotes(util::unescape_string(name));
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

	return false;
}

static bool breaker_exit(Program &prg, std::vector<std::string> &v)
{
	std::string code = v[0];
	std::vector<std::string> strings;
	strings.reserve(1);
	strings.push_back(code);
	std::vector<double> values = variable_names_to_numbers(prg, strings);
	return_code = values[0];
	reset_game_name = "";
	quit = true;
	return false;
}

static bool breaker_return(Program &prg, std::vector<std::string> &v)
{
	std::string value = v[0];

	if (value[0] == '-' || isdigit(value[0])) {
		prg.result.type = Variable::NUMBER;
		prg.result.n = atof(value.c_str());
	}
	else if (value[0] == '"') {
		prg.result.type = Variable::STRING;
		prg.result.s = remove_quotes(util::unescape_string(value));
	}
	else {
		Variable &v1 = find_variable(prg, value);
		prg.result = v1;
	}

	prg.result.name = "result";

	return false;
}

static bool corefunc_var(Program &prg, std::vector<std::string> &v)
{
	Variable var;

	std::string type = v[0];
	std::string name =  v[1];

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
		var.n = prg.vector_id++;
		std::vector<Variable> vec;
		prg.vectors[var.n] = vec;
	}
	else {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
	}

	std::map<std::string, Variable> &variables_backup = prg.variables_backup_stack.top();
	std::map<std::string, Variable>::iterator it3;
	if ((it3 = prg.variables.find(var.name)) != prg.variables.end()) {
		if ((*it3).second.function != prg.name) {
			variables_backup[var.name] = prg.variables[var.name];
		}
	}

	prg.variables[var.name] = var;

	return true;
}

static bool corefunc_set(Program &prg, std::vector<std::string> &v)
{
	std::string dest = v[0];
	std::string src = v[1];

	Variable &v1 = find_variable(prg, dest);

	if (src[0] == '-' || isdigit(src[0])) {
		if (v1.type == Variable::NUMBER) {
			v1.n = atof(src.c_str());
		}
		else if (v1.type == Variable::STRING) {
			v1.s = src;
		}
		else {
			throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
		}
	}
	else if (src[0] == '"') {
		if (v1.type == Variable::NUMBER) {
			v1.n = atof(remove_quotes(src).c_str());
		}
		else if (v1.type == Variable::STRING) {
			v1.s = remove_quotes(util::unescape_string(src));
		}
		else {
			throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
		}
	}
	else {
		Variable &v2 = find_variable(prg, src);

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
			prg.vectors[(int)v1.n] = prg.vectors[(int)v2.n];
		}
		else {
			throw util::ParseError(std::string(__FUNCTION__) + ": " + "Operation undefined for operands on line " + util::itos(get_line_num(prg)));
		}
	}

	return true;
}

static bool corefunc_add(Program &prg, std::vector<std::string> &v)
{
	std::string dest = v[0];
	std::string src = v[1];

	Variable &v1 = find_variable(prg, dest);

	if (src[0] == '-' || isdigit(src[0])) {
		if (v1.type == Variable::NUMBER) {
			v1.n += atof(src.c_str());
		}
		else if (v1.type == Variable::STRING) {
			v1.s += src;
		}
		else {
			throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
		}
	}
	else if (src[0] == '"') {
		if (v1.type == Variable::NUMBER) {
			v1.n += atof(remove_quotes(src).c_str());
		}
		else if (v1.type == Variable::STRING) {
			v1.s += remove_quotes(util::unescape_string(src));
		}
		else {
			throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
		}
	}
	else {
		Variable &v2 = find_variable(prg, src);

		if (v1.type == Variable::NUMBER && v2.type == Variable::NUMBER) {
			v1.n += v2.n;
		}
		else if (v1.type == Variable::STRING && v2.type == Variable::NUMBER) {
			v1.s += util::itos(v2.n);
		}
		else if (v1.type == Variable::STRING && v2.type == Variable::STRING) {
			v1.s += v2.s;
		}
		else {
			throw util::ParseError(std::string(__FUNCTION__) + ": " + "Operation undefined for operands on line " + util::itos(get_line_num(prg)));
		}
	}

	return true;
}

static bool corefunc_subtract(Program &prg, std::vector<std::string> &v)
{
	std::string dest = v[0];
	std::string src = v[1];

	Variable &v1 = find_variable(prg, dest);

	if (src[0] == '-' || isdigit(src[0])) {
		if (v1.type == Variable::NUMBER) {
			v1.n -= atof(src.c_str());
		}
		else {
			throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
		}
	}
	else if (src[0] == '"') {
		if (v1.type == Variable::NUMBER) {
			v1.n -= atof(remove_quotes(src).c_str());
		}
		else {
			throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
		}
	}
	else {
		Variable &v2 = find_variable(prg, src);

		if (v1.type == Variable::NUMBER && v2.type == Variable::NUMBER) {
			v1.n -= v2.n;
		}
		else {
			throw util::ParseError(std::string(__FUNCTION__) + ": " + "Operation undefined for operands on line " + util::itos(get_line_num(prg)));
		}
	}

	return true;
}

static bool corefunc_multiply(Program &prg, std::vector<std::string> &v)
{
	std::string dest = v[0];
	std::string src = v[1];

	Variable &v1 = find_variable(prg, dest);

	if (src[0] == '-' || isdigit(src[0])) {
		if (v1.type == Variable::NUMBER) {
			v1.n *= atof(src.c_str());
		}
		else {
			throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
		}
	}
	else if (src[0] == '"') {
		if (v1.type == Variable::NUMBER) {
			v1.n *= atof(remove_quotes(src).c_str());
		}
		else {
			throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
		}
	}
	else {
		Variable &v2 = find_variable(prg, src);

		if (v1.type == Variable::NUMBER && v2.type == Variable::NUMBER) {
			v1.n *= v2.n;
		}
		else {
			throw util::ParseError(std::string(__FUNCTION__) + ": " + "Operation undefined for operands on line " + util::itos(get_line_num(prg)));
		}
	}

	return true;
}

static bool corefunc_divide(Program &prg, std::vector<std::string> &v)
{
	std::string dest = v[0];
	std::string src = v[1];

	Variable &v1 = find_variable(prg, dest);

	if (src[0] == '-' || isdigit(src[0])) {
		if (v1.type == Variable::NUMBER) {
			v1.n /= atof(src.c_str());
		}
		else {
			throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
		}
	}
	else if (src[0] == '"') {
		if (v1.type == Variable::NUMBER) {
			v1.n /= atof(remove_quotes(src).c_str());
		}
		else {
			throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
		}
	}
	else {
		Variable &v2 = find_variable(prg, src);

		if (v1.type == Variable::NUMBER && v2.type == Variable::NUMBER) {
			v1.n /= v2.n;
		}
		else {
			throw util::ParseError(std::string(__FUNCTION__) + ": " + "Operation undefined for operands on line " + util::itos(get_line_num(prg)));
		}
	}

	return true;
}

static bool corefunc_intmod(Program &prg, std::vector<std::string> &v)
{
	std::string dest = v[0];
	std::string src = v[1];

	Variable &v1 = find_variable(prg, dest);

	if (src[0] == '-' || isdigit(src[0])) {
		if (v1.type == Variable::NUMBER) {
			v1.n = (int)v1.n % (int)atof(src.c_str());
		}
		else {
			throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
		}
	}
	else if (src[0] == '"') {
		if (v1.type == Variable::NUMBER) {
			v1.n = (int)v1.n % (int)atof(remove_quotes(src).c_str());
		}
		else {
			throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
		}
	}
	else {
		Variable &v2 = find_variable(prg, src);

		if (v1.type == Variable::NUMBER && v2.type == Variable::NUMBER) {
			v1.n = (int)v1.n % (int)v2.n;
		}
		else {
			throw util::ParseError(std::string(__FUNCTION__) + ": " + "Operation undefined for operands on line " + util::itos(get_line_num(prg)));
		}
	}

	return true;
}

static bool corefunc_fmod(Program &prg, std::vector<std::string> &v)
{
	std::string dest = v[0];
	std::string src = v[1];

	Variable &v1 = find_variable(prg, dest);

	if (src[0] == '-' || isdigit(src[0])) {
		if (v1.type == Variable::NUMBER) {
			v1.n = fmod(v1.n, atof(src.c_str()));
		}
		else {
			throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
		}
	}
	else if (src[0] == '"') {
		if (v1.type == Variable::NUMBER) {
			v1.n = fmod(v1.n, atof(remove_quotes(src).c_str()));
		}
		else {
			throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
		}
	}
	else {
		Variable &v2 = find_variable(prg, src);

		if (v1.type == Variable::NUMBER && v2.type == Variable::NUMBER) {
			v1.n = fmod(v1.n, v2.n);
		}
		else {
			throw util::ParseError(std::string(__FUNCTION__) + ": " + "Operation undefined for operands on line " + util::itos(get_line_num(prg)));
		}
	}

	return true;
}

static bool corefunc_neg(Program &prg, std::vector<std::string> &v)
{
	std::string name = v[0];
	
	Variable &v1 = find_variable(prg, name);

	if (v1.type == Variable::NUMBER) {
		v1.n = -v1.n;
	}
	else {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Operation undefined for operands on line " + util::itos(get_line_num(prg)));
	}

	return true;
}

static bool corefunc_label(Program &prg, std::vector<std::string> &v)
{
	std::string name = v[0];

	if (name[0] != '_' && isalpha(name[0]) == false) {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid label name on line " + util::itos(get_line_num(prg)));
	}

	Label l;
	l.name = name;
	l.p = prg.p;
	l.line = prg.line;
	l.pc = prg.pc;
	
	//prg.labels.push_back(l);
	//already got these

	return true;
}

static bool corefunc_goto(Program &prg, std::vector<std::string> &v)
{
	std::string name = v[0];

	std::map<std::string, Label>::iterator it = prg.labels.find(name);
	if (it != prg.labels.end()) {
		Label &l = (*it).second;
		prg.p = l.p;
		prg.line = l.line;
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

static bool corefunc_compare(Program &prg, std::vector<std::string> &v)
{
	std::string a = v[0];
	std::string b = v[1];

	bool a_string = false;
	bool b_string = false;
	std::string s1;
	std::string s2;

	if (a[0] == '"') {
		a_string = true;
		s1 = remove_quotes(util::unescape_string(a));
	}
	else if (a[0] == '_' || isalpha(a[0])) {
		Variable &v = find_variable(prg, a);
		if (v.type == Variable::STRING) {
			a_string = true;
			s1 = v.s;
		}
	}

	if (b[0] == '"') {
		b_string = true;
		s2 = remove_quotes(util::unescape_string(b));
	}
	else if (b[0] == '_' || isalpha(b[0])) {
		Variable &v = find_variable(prg, b);
		if (v.type == Variable::STRING) {
			b_string = true;
			s2 = v.s;
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
		std::vector<std::string> strings;
		strings.reserve(2);
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

	return true;
}

static bool corefunc_je(Program &prg, std::vector<std::string> &v)
{
	std::string label = v[0];

	if (prg.compare_flag == 0) {
		std::map<std::string, Label>::iterator it = prg.labels.find(label);
		if (it != prg.labels.end()) {
			Label &l = (*it).second;
			prg.p = l.p;
			prg.line = l.line;
			prg.pc = l.pc;
		}
	}

	return true;
}

static bool corefunc_jne(Program &prg, std::vector<std::string> &v)
{
	std::string label = v[0];

	if (prg.compare_flag != 0) {
		std::map<std::string, Label>::iterator it = prg.labels.find(label);
		if (it != prg.labels.end()) {
			Label &l = (*it).second;
			prg.p = l.p;
			prg.line = l.line;
			prg.pc = l.pc;
		}
	}

	return true;
}

static bool corefunc_jl(Program &prg, std::vector<std::string> &v)
{
	std::string label = v[0];

	if (prg.compare_flag < 0) {
		std::map<std::string, Label>::iterator it = prg.labels.find(label);
		if (it != prg.labels.end()) {
			Label &l = (*it).second;
			prg.p = l.p;
			prg.line = l.line;
			prg.pc = l.pc;
		}
	}

	return true;
}

static bool corefunc_jle(Program &prg, std::vector<std::string> &v)
{
	std::string label = v[0];

	if (prg.compare_flag <= 0) {
		std::map<std::string, Label>::iterator it = prg.labels.find(label);
		if (it != prg.labels.end()) {
			Label &l = (*it).second;
			prg.p = l.p;
			prg.line = l.line;
			prg.pc = l.pc;
		}
	}

	return true;
}

static bool corefunc_jg(Program &prg, std::vector<std::string> &v)
{
	std::string label = v[0];

	if (prg.compare_flag > 0) {
		std::map<std::string, Label>::iterator it = prg.labels.find(label);
		if (it != prg.labels.end()) {
			Label &l = (*it).second;
			prg.p = l.p;
			prg.line = l.line;
			prg.pc = l.pc;
		}
	}

	return true;
}

static bool corefunc_jge(Program &prg, std::vector<std::string> &v)
{
	std::string label = v[0];

	if (prg.compare_flag >= 0) {
		std::map<std::string, Label>::iterator it = prg.labels.find(label);
		if (it != prg.labels.end()) {
			Label &l = (*it).second;
			prg.p = l.p;
			prg.line = l.line;
			prg.pc = l.pc;
		}
	}

	return true;
}

static bool corefunc_call(Program &prg, std::vector<std::string> &v)
{
	std::string tok2 = v[0];
	std::string function_name;
	std::string result_name;
	int _tok = 1;

	if (tok2 == ">") {
		result_name = v[_tok++];
		function_name = v[_tok++];

		// check for errors
		find_variable(prg, result_name);
	}
	else {
		function_name = tok2;
	}

	std::vector<std::string> params;
	for (int i = _tok; i < v.size(); i++) {
		params.push_back(v[i]);
	}

	call_function(prg, function_name, params, result_name);

	return true;
}

static bool corefunc_function(Program &prg, std::vector<std::string> &v)
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

static bool corefunc_comment(Program &prg, std::vector<std::string> &v)
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

static bool corefunc_inspect(Program &prg, std::vector<std::string> &v)
{
	std::string name = v[0];
	
	Variable &v1 = find_variable(prg, name);

	char buf[1000];

	if (name[0] == '-' || isdigit(name[0])) {
		snprintf(buf, 1000, "%s", name.c_str());
	}
	else if (name[0] == '"') {
		std::string s = remove_quotes(util::unescape_string(name));
		snprintf(buf, 1000, "%s", s.c_str());
	}
	else if (v1.type == Variable::NUMBER) {
		snprintf(buf, 1000, "%g", v1.n);
	}
	else if (v1.type == Variable::STRING) {
		snprintf(buf, 1000, "\"%s\"", v1.s.c_str());
	}
	else {
		strcpy(buf, "Unknown");
	}

	gui::popup("INSPECTOR", buf, gui::OK);

	return true;
}

static bool corefunc_string_format(Program &prg, std::vector<std::string> &v)
{
	std::string dest = v[0];
	std::string fmt = v[1];
	int _tok = 2;
	
	std::string fmts;

	if (fmt[0] == '"') {
		fmts = remove_quotes(util::unescape_string(fmt));
	}
	else {
		Variable &v1 = find_variable(prg, fmt);

		if (v1.type == Variable::STRING) {
			fmts = v1.s;
		}
		else {
			throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
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

		std::string param = v[_tok++];

		std::string val;

		if (param[0] == '-' || isdigit(param[0])) {
			val = param;
		}
		else if (param[0] == '"') {
			val = remove_quotes(util::unescape_string(param));
		}
		else {
			Variable &v1 = find_variable(prg, param);
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

	if (c < fmts.length()) {
		result += fmts.substr(c);
	}

	Variable &v1 = find_variable(prg, dest);
	v1.type = Variable::STRING;
	v1.s = result;

	return true;
}

static bool mathfunc_sin(Program &prg, std::vector<std::string> &v)
{
	std::string dest = v[0];
	std::string vs = v[1];
	
	std::vector<std::string> strings;
	strings.reserve(1);
	strings.push_back(vs);
	std::vector<double> values = variable_names_to_numbers(prg, strings);

	Variable &v1 = find_variable(prg, dest);

	if (v1.type == Variable::NUMBER) {
		v1.n = sin(values[0]);
	}
	else {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Operation undefined for operands on line " + util::itos(get_line_num(prg)));
	}

	return true;
}

static bool mathfunc_cos(Program &prg, std::vector<std::string> &v)
{
	std::string dest = v[0];
	std::string vs = v[1];
	
	std::vector<std::string> strings;
	strings.reserve(1);
	strings.push_back(vs);
	std::vector<double> values = variable_names_to_numbers(prg, strings);

	Variable &v1 = find_variable(prg, dest);

	if (v1.type == Variable::NUMBER) {
		v1.n = cos(values[0]);
	}
	else {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Operation undefined for operands on line " + util::itos(get_line_num(prg)));
	}

	return true;
}

static bool mathfunc_atan2(Program &prg, std::vector<std::string> &v)
{
	std::string dest = v[0];
	std::string vs1 = v[1];
	std::string vs2 = v[2];
	
	std::vector<std::string> strings;
	strings.reserve(2);
	strings.push_back(vs1);
	strings.push_back(vs2);
	std::vector<double> values = variable_names_to_numbers(prg, strings);

	Variable &v1 = find_variable(prg, dest);

	if (v1.type == Variable::NUMBER) {
		v1.n = atan2(values[0], values[1]);
	}
	else {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Operation undefined for operands on line " + util::itos(get_line_num(prg)));
	}

	return true;
}

static bool mathfunc_abs(Program &prg, std::vector<std::string> &v)
{
	std::string dest = v[0];
	std::string vs = v[1];
	
	std::vector<std::string> strings;
	strings.reserve(1);
	strings.push_back(vs);
	std::vector<double> values = variable_names_to_numbers(prg, strings);

	Variable &v1 = find_variable(prg, dest);

	if (v1.type == Variable::NUMBER) {
		v1.n = abs(values[0]);
	}
	else {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Operation undefined for operands on line " + util::itos(get_line_num(prg)));
	}

	return true;
}

static bool mathfunc_pow(Program &prg, std::vector<std::string> &v)
{
	std::string dest = v[0];
	std::string vs1 = v[1];
	std::string vs2 = v[2];
	
	std::vector<std::string> strings;
	strings.reserve(2);
	strings.push_back(vs1);
	strings.push_back(vs2);
	std::vector<double> values = variable_names_to_numbers(prg, strings);

	Variable &v1 = find_variable(prg, dest);

	if (v1.type == Variable::NUMBER) {
		v1.n = pow(values[0], values[1]);
	}
	else {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Operation undefined for operands on line " + util::itos(get_line_num(prg)));
	}

	return true;
}

static bool mathfunc_sqrt(Program &prg, std::vector<std::string> &v)
{
	std::string dest = v[0];
	std::string vs1 = v[1];
	
	std::vector<std::string> strings;
	strings.reserve(1);
	strings.push_back(vs1);
	std::vector<double> values = variable_names_to_numbers(prg, strings);

	Variable &v1 = find_variable(prg, dest);

	if (v1.type == Variable::NUMBER) {
		v1.n = sqrt(values[0]);
	}
	else {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Operation undefined for operands on line " + util::itos(get_line_num(prg)));
	}

	return true;
}

static bool mathfunc_rand(Program &prg, std::vector<std::string> &v)
{
	std::string dest = v[0];
	std::string min_incl = v[1];
	std::string max_incl = v[2];

	std::vector<std::string> strings;
	strings.reserve(2);
	strings.push_back(min_incl);
	strings.push_back(max_incl);
	std::vector<double> values = variable_names_to_numbers(prg, strings);

	Variable &v1 = find_variable(prg, dest);

	if (v1.type == Variable::NUMBER) {
		v1.n = util::rand(values[0], values[1]);
	}
	else if (v1.type == Variable::STRING) {
		v1.s = util::itos(util::rand(values[0], values[1]));
	}
	else {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Operation undefined for operands on line " + util::itos(get_line_num(prg)));
	}

	return true;
}

static bool gfxfunc_clear(Program &prg, std::vector<std::string> &v)
{
	std::string r =  v[0];
	std::string g =  v[1];
	std::string b =  v[2];

	std::vector<std::string> strings;
	strings.reserve(3);
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

	return true;
}

static bool primfunc_start_primitives(Program &prg, std::vector<std::string> &v)
{
	gfx::draw_primitives_start();

	return true;
}

static bool primfunc_end_primitives(Program &prg, std::vector<std::string> &v)
{
	gfx::draw_primitives_end();

	return true;
}

static bool primfunc_line(Program &prg, std::vector<std::string> &v)
{
	std::string r =  v[0];
	std::string g =  v[1];
	std::string b =  v[2];
	std::string a =  v[3];
	std::string x =  v[4];
	std::string y =  v[5];
	std::string x2 =  v[6];
	std::string y2 =  v[7];
	std::string thickness = v[8];
	
	std::vector<std::string> strings;
	strings.reserve(9);
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

	return true;
}

static bool primfunc_filled_triangle(Program &prg, std::vector<std::string> &v)
{
	std::string r1 =  v[0];
	std::string g1 =  v[1];
	std::string b1 =  v[2];
	std::string a1 =  v[3];
	std::string r2 =  v[4];
	std::string g2 =  v[5];
	std::string b2 =  v[6];
	std::string a2 =  v[7];
	std::string r3 =  v[8];
	std::string g3 =  v[9];
	std::string b3 =  v[10];
	std::string a3 =  v[11];
	std::string x =  v[12];
	std::string y =  v[13];
	std::string x2 =  v[14];
	std::string y2 =  v[15];
	std::string x3 =  v[16];
	std::string y3 =  v[17];
	
	std::vector<std::string> strings;
	strings.reserve(18);
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

	return true;
}

static bool primfunc_rectangle(Program &prg, std::vector<std::string> &v)
{
	std::string r =  v[0];
	std::string g =  v[1];
	std::string b =  v[2];
	std::string a =  v[3];
	std::string x =  v[4];
	std::string y =  v[5];
	std::string w =  v[6];
	std::string h =  v[7];
	std::string thickness =  v[8];
	
	std::vector<std::string> strings;
	strings.reserve(9);
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

	return true;
}

static bool primfunc_filled_rectangle(Program &prg, std::vector<std::string> &v)
{
	std::string r1 =  v[0];
	std::string g1 =  v[1];
	std::string b1 =  v[2];
	std::string a1 =  v[3];
	std::string r2 =  v[4];
	std::string g2 =  v[5];
	std::string b2 =  v[6];
	std::string a2 =  v[7];
	std::string r3 =  v[8];
	std::string g3 =  v[9];
	std::string b3 =  v[10];
	std::string a3 =  v[11];
	std::string r4 =  v[12];
	std::string g4 =  v[13];
	std::string b4 =  v[14];
	std::string a4 =  v[15];
	std::string x =  v[16];
	std::string y =  v[17];
	std::string w =  v[18];
	std::string h =  v[19];
	
	std::vector<std::string> strings;
	strings.reserve(20);
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

	return true;
}

static bool primfunc_ellipse(Program &prg, std::vector<std::string> &v)
{
	std::string r = v[0];
	std::string g = v[1];
	std::string b = v[2];
	std::string a = v[3];
	std::string x = v[4];
	std::string y = v[5];
	std::string rx = v[6];
	std::string ry = v[7];
	std::string thickness = v[8];
	std::string sections = v[9];
	
	std::vector<std::string> strings;
	strings.reserve(10);
	strings.push_back(r);
	strings.push_back(g);
	strings.push_back(b);
	strings.push_back(a);
	strings.push_back(x);
	strings.push_back(y);
	strings.push_back(rx);
	strings.push_back(ry);
	strings.push_back(thickness);
	strings.push_back(sections);
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
	float _sections = values[9];

	gfx::draw_ellipse(c, p, _rx, _ry, thick, _sections);

	return true;
}

static bool primfunc_filled_ellipse(Program &prg, std::vector<std::string> &v)
{
	std::string r = v[0];
	std::string g = v[1];
	std::string b = v[2];
	std::string a = v[3];
	std::string x = v[4];
	std::string y = v[5];
	std::string rx = v[6];
	std::string ry = v[7];
	std::string sections = v[8];
	
	std::vector<std::string> strings;
	strings.reserve(9);
	strings.push_back(r);
	strings.push_back(g);
	strings.push_back(b);
	strings.push_back(a);
	strings.push_back(x);
	strings.push_back(y);
	strings.push_back(rx);
	strings.push_back(ry);
	strings.push_back(sections);
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
	float _sections = values[8];

	gfx::draw_filled_ellipse(c, p, _rx, _ry, _sections);

	return true;
}

static bool primfunc_circle(Program &prg, std::vector<std::string> &v)
{
	std::string r = v[0];
	std::string g = v[1];
	std::string b = v[2];
	std::string a = v[3];
	std::string x = v[4];
	std::string y = v[5];
	std::string radius = v[6];
	std::string thickness = v[7];
	std::string sections = v[8];
	
	std::vector<std::string> strings;
	strings.reserve(9);
	strings.push_back(r);
	strings.push_back(g);
	strings.push_back(b);
	strings.push_back(a);
	strings.push_back(x);
	strings.push_back(y);
	strings.push_back(radius);
	strings.push_back(thickness);
	strings.push_back(sections);
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
	int _sections = values[8];

	gfx::draw_circle(c, p, _r, thick, _sections);

	return true;
}

static bool primfunc_filled_circle(Program &prg, std::vector<std::string> &v)
{
	std::string r = v[0];
	std::string g = v[1];
	std::string b = v[2];
	std::string a = v[3];
	std::string x = v[4];
	std::string y = v[5];
	std::string radius = v[6];
	std::string sections = v[7];
	
	std::vector<std::string> strings;
	strings.reserve(8);
	strings.push_back(r);
	strings.push_back(g);
	strings.push_back(b);
	strings.push_back(a);
	strings.push_back(x);
	strings.push_back(y);
	strings.push_back(radius);
	strings.push_back(sections);
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
	int _sections = values[7];

	gfx::draw_filled_circle(c, p, _r, _sections);

	return true;
}

static bool mmlfunc_create(Program &prg, std::vector<std::string> &v)
{
	std::string var = v[0];
	std::string str = v[1];

	Variable &v1 = find_variable(prg, var);

	if (v1.type == Variable::NUMBER) {
		v1.n = prg.mml_id;
	}
	else if (v1.type == Variable::STRING) {
		v1.s = util::itos(prg.mml_id);
	}
	else {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
	}

	std::string strs;

	if (str[0] == '"') {
		strs = remove_quotes(util::unescape_string(str));
	}
	else {
		Variable &v1 = find_variable(prg, str);
		if (v1.type == Variable::STRING) {
			strs = v1.s;
		}
		else {
			throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
		}
	}

	Uint8 *bytes = (Uint8 *)strs.c_str();
	SDL_RWops *file = SDL_RWFromMem(bytes, strs.length());
	audio::MML *mml = new audio::MML(file);
	//SDL_RWclose(file);

	prg.mmls[prg.mml_id++] = mml;

	return true;
}

static bool mmlfunc_load(Program &prg, std::vector<std::string> &v)
{
	std::string var = v[0];
	std::string name = v[1];

	Variable &v1 = find_variable(prg, var);

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

static bool mmlfunc_play(Program &prg, std::vector<std::string> &v)
{
	std::string id = v[0];
	std::string volume = v[1];
	std::string loop = v[2];

	std::vector<std::string> strings;
	strings.reserve(3);
	strings.push_back(id);
	strings.push_back(volume);
	strings.push_back(loop);
	std::vector<double> values = variable_names_to_numbers(prg, strings);

	if (values[0] < 0 || values[0] >= prg.mmls.size()) {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid MML on line " + util::itos(get_line_num(prg)));
	}

	audio::MML *mml = prg.mmls[values[0]];

	mml->play(values[1], values[2] == 0 ? false : true);

	return true;
}

static bool mmlfunc_stop(Program &prg, std::vector<std::string> &v)
{
	std::string id = v[0];

	std::vector<std::string> strings;
	strings.reserve(1);
	strings.push_back(id);
	std::vector<double> values = variable_names_to_numbers(prg, strings);

	if (values[0] < 0 || values[0] >= prg.mmls.size()) {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid MML on line " + util::itos(get_line_num(prg)));
	}

	audio::MML *mml = prg.mmls[values[0]];

	mml->stop();

	return true;
}

static bool imagefunc_load(Program &prg, std::vector<std::string> &v)
{
	std::string var = v[0];
	std::string name = v[1];

	Variable &v1 = find_variable(prg, var);

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

static bool imagefunc_draw(Program &prg, std::vector<std::string> &v)
{
	std::string id = v[0];
	std::string r = v[1];
	std::string g = v[2];
	std::string b = v[3];
	std::string a = v[4];
	std::string x = v[5];
	std::string y = v[6];
	std::string flip_h = v[7];
	std::string flip_v = v[8];

	std::vector<std::string> strings;
	strings.reserve(9);
	strings.push_back(id);
	strings.push_back(r);
	strings.push_back(g);
	strings.push_back(b);
	strings.push_back(a);
	strings.push_back(x);
	strings.push_back(y);
	strings.push_back(flip_h);
	strings.push_back(flip_v);
	std::vector<double> values = variable_names_to_numbers(prg, strings);
	
	if (values[0] < 0 || values[0] >= prg.images.size()) {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid Image on line " + util::itos(get_line_num(prg)));
	}

	gfx::Image *img = prg.images[values[0]];

	SDL_Colour c;
	c.r = values[1];
	c.g = values[2];
	c.b = values[3];
	c.a = values[4];

	int flags = 0;
	if (values[7] != 0.0) {
		flags |= gfx::Image::FLIP_H;
	}
	if (values[8] != 0.0) {
		flags |= gfx::Image::FLIP_V;
	}

	img->draw_tinted(c, util::Point<float>(values[5], values[6]), flags);

	return true;
}

static bool imagefunc_stretch_region(Program &prg, std::vector<std::string> &v)
{
	std::string id = v[0];
	std::string r = v[1];
	std::string g = v[2];
	std::string b = v[3];
	std::string a = v[4];
	std::string sx = v[5];
	std::string sy = v[6];
	std::string sw = v[7];
	std::string sh = v[8];
	std::string dx = v[9];
	std::string dy = v[10];
	std::string dw = v[11];
	std::string dh = v[12];
	std::string flip_h = v[13];
	std::string flip_v = v[14];

	std::vector<std::string> strings;
	strings.reserve(15);
	strings.push_back(id);
	strings.push_back(r);
	strings.push_back(g);
	strings.push_back(b);
	strings.push_back(a);
	strings.push_back(sx);
	strings.push_back(sy);
	strings.push_back(sw);
	strings.push_back(sh);
	strings.push_back(dx);
	strings.push_back(dy);
	strings.push_back(dw);
	strings.push_back(dh);
	strings.push_back(flip_h);
	strings.push_back(flip_v);
	std::vector<double> values = variable_names_to_numbers(prg, strings);
	
	if (values[0] < 0 || values[0] >= prg.images.size()) {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid Image on line " + util::itos(get_line_num(prg)));
	}

	gfx::Image *img = prg.images[values[0]];

	SDL_Colour c;
	c.r = values[1];
	c.g = values[2];
	c.b = values[3];
	c.a = values[4];

	int flags = 0;
	if (values[13] != 0.0) {
		flags |= gfx::Image::FLIP_H;
	}
	if (values[14] != 0.0) {
		flags |= gfx::Image::FLIP_V;
	}

	img->stretch_region_tinted(c, util::Point<float>(values[5], values[6]), util::Size<float>(values[7], values[8]), util::Point<float>(values[9], values[10]), util::Size<float>(values[11], values[12]), flags);

	return true;
}

static bool imagefunc_draw_rotated_scaled(Program &prg, std::vector<std::string> &v)
{
	std::string id = v[0];
	std::string r = v[1];
	std::string g = v[2];
	std::string b = v[3];
	std::string a = v[4];
	std::string cx = v[5];
	std::string cy = v[6];
	std::string x = v[7];
	std::string y = v[8];
	std::string angle = v[9];
	std::string scale_x = v[10];
	std::string scale_y = v[11];
	std::string flip_h = v[12];
	std::string flip_v = v[13];

	std::vector<std::string> strings;
	strings.reserve(14);
	strings.push_back(id);
	strings.push_back(r);
	strings.push_back(g);
	strings.push_back(b);
	strings.push_back(a);
	strings.push_back(cx);
	strings.push_back(cy);
	strings.push_back(x);
	strings.push_back(y);
	strings.push_back(angle);
	strings.push_back(scale_x);
	strings.push_back(scale_y);
	strings.push_back(flip_h);
	strings.push_back(flip_v);
	std::vector<double> values = variable_names_to_numbers(prg, strings);
	
	if (values[0] < 0 || values[0] >= prg.images.size()) {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid Image on line " + util::itos(get_line_num(prg)));
	}

	gfx::Image *img = prg.images[values[0]];

	SDL_Colour c;
	c.r = values[1];
	c.g = values[2];
	c.b = values[3];
	c.a = values[4];

	int flags = 0;
	if (values[12] != 0.0) {
		flags |= gfx::Image::FLIP_H;
	}
	if (values[13] != 0.0) {
		flags |= gfx::Image::FLIP_V;
	}

	img->draw_tinted_rotated_scaledxy(c, util::Point<float>(values[5], values[6]), util::Point<float>(values[7], values[8]), values[9], values[10], values[11]);

	return true;
}

static bool imagefunc_start(Program &prg, std::vector<std::string> &v)
{
	std::string img = v[0];

	std::vector<std::string> strings;
	strings.reserve(1);
	strings.push_back(img);
	std::vector<double> values = variable_names_to_numbers(prg, strings);

	if (prg.images.find(values[0]) == prg.images.end()) {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Unknown image \"" + img + "\" on line " + util::itos(get_line_num(prg)));
	}

	gfx::Image *image = prg.images[values[0]];

	image->start_batch();

	return true;
}

static bool imagefunc_end(Program &prg, std::vector<std::string> &v)
{
	std::string img = v[0];

	std::vector<std::string> strings;
	strings.reserve(1);
	strings.push_back(img);
	std::vector<double> values = variable_names_to_numbers(prg, strings);

	if (prg.images.find(values[0]) == prg.images.end()) {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Unknown image \"" + img + "\" on line " + util::itos(get_line_num(prg)));
	}

	gfx::Image *image = prg.images[values[0]];

	image->end_batch();

	return true;
}

static bool imagefunc_size(Program &prg, std::vector<std::string> &v)
{
	std::string id = v[0];
	std::string dest1 = v[1];
	std::string dest2 = v[2];

	std::vector<std::string> strings;
	strings.reserve(1);
	strings.push_back(id);
	std::vector<double> values = variable_names_to_numbers(prg, strings);
	
	if (prg.images.find(values[0]) == prg.images.end()) {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid image on line " + util::itos(get_line_num(prg)));
	}

	gfx::Image *img = prg.images[values[0]];

	Variable &v1 = find_variable(prg, dest1);
	Variable &v2 = find_variable(prg, dest2);

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

static bool fontfunc_load(Program &prg, std::vector<std::string> &v)
{
	std::string var = v[0];
	std::string name = v[1];
	std::string size = v[2];
	std::string smooth = v[3];

	Variable &v1 = find_variable(prg, var);

	if (v1.type == Variable::NUMBER) {
		v1.n = prg.font_id;
	}
	else if (v1.type == Variable::STRING) {
		v1.s = util::itos(prg.font_id);
	}
	else {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
	}

	std::vector<std::string> strings;
	strings.reserve(2);
	strings.push_back(size);
	strings.push_back(smooth);
	std::vector<double> values = variable_names_to_numbers(prg, strings);

	gfx::TTF *font = new gfx::TTF(remove_quotes(util::unescape_string(name)), values[0], 256);
	font->set_smooth(values[1]);

	prg.fonts[prg.font_id++] = font;

	return true;
}

static bool fontfunc_draw(Program &prg, std::vector<std::string> &v)
{
	std::string id = v[0];
	std::string r = v[1];
	std::string g = v[2];
	std::string b = v[3];
	std::string a = v[4];
	std::string text = v[5];
	std::string x = v[6];
	std::string y = v[7];

	std::vector<std::string> strings;
	strings.reserve(7);
	strings.push_back(id);
	strings.push_back(r);
	strings.push_back(g);
	strings.push_back(b);
	strings.push_back(a);
	strings.push_back(x);
	strings.push_back(y);
	std::vector<double> values = variable_names_to_numbers(prg, strings);

	if (values[0] < 0 || values[0] >= prg.fonts.size()) {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid Font on line " + util::itos(get_line_num(prg)));
	}

	gfx::TTF *font = prg.fonts[values[0]];

	SDL_Colour c;
	c.r = values[1];
	c.g = values[2];
	c.b = values[3];
	c.a = values[4];

	std::string txt;

	if (text[0] == '"') {
		txt = remove_quotes(util::unescape_string(text));
	}
	else {
		Variable &v1 = find_variable(prg, text);
		if (v1.type == Variable::NUMBER) {
			char buf[1000];
			snprintf(buf, 1000, "%g", v1.n);
			txt = buf;
		}
		else if (v1.type == Variable::STRING) {
			txt = v1.s;
		}
		else {
			throw util::ParseError(std::string(__FUNCTION__) + ": " + "Unknown variable \"" + text + "\" on line " + util::itos(get_line_num(prg)));
		}
	}

	font->draw(c, txt, util::Point<float>(values[5], values[6]));

	return true;
}

static bool fontfunc_width(Program &prg, std::vector<std::string> &v)
{
	std::string id = v[0];
	std::string dest = v[1];
	std::string text = v[2];
	
	std::vector<std::string> strings;
	strings.reserve(1);
	strings.push_back(id);
	std::vector<double> values = variable_names_to_numbers(prg, strings);

	std::string txt;

	if (text[0] == '"') {
		txt = remove_quotes(util::unescape_string(text));
	}
	else {
		Variable &v1 = find_variable(prg, text);
		if (v1.type == Variable::NUMBER) {
			char buf[1000];
			snprintf(buf, 1000, "%g", v1.n);
			txt = buf;
		}
		else if (v1.type == Variable::STRING) {
			txt = v1.s;
		}
		else {
			throw util::ParseError(std::string(__FUNCTION__) + ": " + "Unknown variable \"" + dest + "\" on line " + util::itos(get_line_num(prg)));
		}
	}

	gfx::TTF *font = prg.fonts[values[0]];

	int w = font->get_text_width(txt);

	Variable &v1 = find_variable(prg, dest);

	if (v1.type == Variable::NUMBER) {
		v1.n = w;
	}
	else {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
	}

	return true;
}

static bool fontfunc_height(Program &prg, std::vector<std::string> &v)
{
	std::string id = v[0];
	std::string dest = v[1];
	
	std::vector<std::string> strings;
	strings.reserve(1);
	strings.push_back(id);
	std::vector<double> values = variable_names_to_numbers(prg, strings);

	gfx::TTF *font = prg.fonts[values[0]];

	int h = font->get_height();

	Variable &v1 = find_variable(prg, dest);

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
	Variable &v1 = find_variable(prg, name);

	if (v1.type == Variable::NUMBER) {
		v1.n = value;
	}
	else {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
	}
}

static bool joyfunc_poll(Program &prg, std::vector<std::string> &v)
{
	std::string num = v[0];
	std::string x1 = v[1];
	std::string y1 = v[2];
	std::string x2 = v[3];
	std::string y2 = v[4];
	std::string x3 = v[5];
	std::string y3 = v[6];
	std::string l = v[7];
	std::string r = v[8];
	std::string u = v[9];
	std::string d = v[10];
	std::string a = v[11];
	std::string b = v[12];
	std::string x = v[13];
	std::string y = v[14];
	std::string lb = v[15];
	std::string rb = v[16];
	std::string ls = v[17];
	std::string rs = v[18];
	std::string back = v[19];
	std::string start = v[20];

	std::vector<std::string> strings;
	strings.reserve(1);
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
	names.push_back(x3);
	names.push_back(y3);
	names.push_back(ls);
	names.push_back(rs);

	for (size_t i = 0; i < names.size(); i++) {
		find_variable(prg, names[i]);
	}

	SDL_JoystickID id = input::get_controller_id(values[0]);
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

static bool joyfunc_count(Program &prg, std::vector<std::string> &v)
{
	std::string dest = v[0];
	
	Variable &v1 = find_variable(prg, dest);

	if (v1.type == Variable::NUMBER) {
		v1.n = input::get_num_joysticks();
	}
	else {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Operation undefined for operands on line " + util::itos(get_line_num(prg)));
	}

	return true;
}

static bool vectorfunc_add(Program &prg, std::vector<std::string> &v)
{
	std::string id = v[0];
	std::string value = v[1];

	std::vector<std::string> strings;
	strings.reserve(1);
	strings.push_back(id);
	std::vector<double> values = variable_names_to_numbers(prg, strings);

	if (prg.vectors.find(values[0]) == prg.vectors.end()) {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid vector on line " + util::itos(get_line_num(prg)));
	}
	/*
	if (values[0] < 0 || values[0] >= prg.vectors.size()) {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid vector on line " + util::itos(get_line_num(prg)));
	}
	*/

	std::vector<Variable> &vec = prg.vectors[values[0]];

	Variable var;

	if (value[0] == '"') {
		var.type = Variable::STRING;
		var.function = prg.name;
		var.name = "-constant-";
		var.s = remove_quotes(util::unescape_string(value));
	}
	else if (value[0] == '-' || isdigit(value[0])) {
		var.type = Variable::NUMBER;
		var.function = prg.name;
		var.name = "-constant-";
		var.n = atof(value.c_str());
	}
	else {
		var = find_variable(prg, value);
	}

	vec.push_back(var);

	return true;
}

static bool vectorfunc_size(Program &prg, std::vector<std::string> &v)
{
	std::string id = v[0];
	std::string dest = v[1];

	std::vector<std::string> strings;
	strings.reserve(1);
	strings.push_back(id);
	std::vector<double> values = variable_names_to_numbers(prg, strings);
	
	if (prg.vectors.find(values[0]) == prg.vectors.end()) {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid vector on line " + util::itos(get_line_num(prg)));
	}

	std::vector<Variable> &vec = prg.vectors[values[0]];

	Variable &v1 = find_variable(prg, dest);

	if (v1.type == Variable::NUMBER) {
		v1.n = vec.size();
	}
	else {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
	}

	return true;
}

static bool vectorfunc_set(Program &prg, std::vector<std::string> &v)
{
	std::string id = v[0];
	std::string index = v[1];
	std::string value = v[2];

	std::vector<std::string> strings;
	strings.reserve(2);
	strings.push_back(id);
	strings.push_back(index);
	std::vector<double> values = variable_names_to_numbers(prg, strings);
	
	if (prg.vectors.find(values[0]) == prg.vectors.end()) {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid vector on line " + util::itos(get_line_num(prg)));
	}

	std::vector<Variable> &vec = prg.vectors[values[0]];

	if (values[1] < 0 || values[1] >= vec.size()) {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid index on line " + util::itos(get_line_num(prg)));
	}

	Variable var;

	if (value[0] == '"') {
		var.type = Variable::STRING;
		var.function = prg.name;
		var.name = "-constant-";
		var.s = remove_quotes(util::unescape_string(value));
	}
	else if (value[0] == '-' || isdigit(value[0])) {
		var.type = Variable::NUMBER;
		var.function = prg.name;
		var.name = "-constant-";
		var.n = atof(value.c_str());
	}
	else {
		var = find_variable(prg, value);
	}

	vec[values[1]] = var;

	return true;
}

static bool vectorfunc_insert(Program &prg, std::vector<std::string> &v)
{
	std::string id = v[0];
	std::string index = v[1];
	std::string value = v[2];

	std::vector<std::string> strings;
	strings.reserve(2);
	strings.push_back(id);
	strings.push_back(index);
	std::vector<double> values = variable_names_to_numbers(prg, strings);
	
	if (prg.vectors.find(values[0]) == prg.vectors.end()) {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid vector on line " + util::itos(get_line_num(prg)));
	}

	std::vector<Variable> &vec = prg.vectors[values[0]];

	if (values[1] < 0 || values[1] > vec.size()) {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid index on line " + util::itos(get_line_num(prg)));
	}

	Variable var;

	if (value[0] == '"') {
		var.type = Variable::STRING;
		var.function = prg.name;
		var.name = "-constant-";
		var.s = remove_quotes(util::unescape_string(value));
	}
	else if (value[0] == '-' || isdigit(value[0])) {
		var.type = Variable::NUMBER;
		var.function = prg.name;
		var.name = "-constant-";
		var.n = atof(value.c_str());
	}
	else {
		var = find_variable(prg, value);
	}

	vec.insert(vec.begin()+values[1], var);

	return true;
}

static bool vectorfunc_get(Program &prg, std::vector<std::string> &v)
{
	std::string id = v[0];
	std::string dest = v[1];
	std::string index = v[2];

	std::vector<std::string> strings;
	strings.reserve(2);
	strings.push_back(id);
	strings.push_back(index);
	std::vector<double> values = variable_names_to_numbers(prg, strings);

	if (prg.vectors.find(values[0]) == prg.vectors.end()) {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid vector on line " + util::itos(get_line_num(prg)));
	}

	std::vector<Variable> &vec = prg.vectors[values[0]];

	if (values[1] < 0 || values[1] >= vec.size()) {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid index on line " + util::itos(get_line_num(prg)));
	}

	Variable &v1 = find_variable(prg, dest);

	std::string bak = v1.name;
	std::string bak2 = v1.function;
	v1 = vec[values[1]];
	v1.name = bak;
	v1.function = bak2;

	return true;
}

static bool vectorfunc_erase(Program &prg, std::vector<std::string> &v)
{
	std::string id = v[0];
	std::string index = v[1];

	std::vector<std::string> strings;
	strings.reserve(2);
	strings.push_back(id);
	strings.push_back(index);
	std::vector<double> values = variable_names_to_numbers(prg, strings);

	if (prg.vectors.find(values[0]) == prg.vectors.end()) {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid vector on line " + util::itos(get_line_num(prg)));
	}

	std::vector<Variable> &vec = prg.vectors[values[0]];

	if (values[1] < 0 || values[1] >= vec.size()) {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid index on line " + util::itos(get_line_num(prg)));
	}

	vec.erase(vec.begin() + int(values[1]));

	return true;
}

static bool cfgfunc_load(Program &prg, std::vector<std::string> &v)
{
	cfg_numbers.clear();
	cfg_strings.clear();

	std::string found = v[0];
	std::string cfg_name = v[1];

	std::string cfg_names;

	if (cfg_name[0] == '"') {
		cfg_names = remove_quotes(util::unescape_string(cfg_name));
	}
	else  {
		Variable &v1 = find_variable(prg, cfg_name);
		if (v1.type == Variable::STRING) {
			cfg_names = v1.s;
		}
		else {
			throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
		}
	}

	bool found_cfg = load_cfg(cfg_names);
	
	Variable &v1 = find_variable(prg, found);

	if (v1.type == Variable::NUMBER) {
		v1.n = found_cfg;
	}
	else {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
	}

	return true;
}

static bool cfgfunc_save(Program &prg, std::vector<std::string> &v)
{
	std::string cfg_name = v[0];

	std::string cfg_names;

	if (cfg_name[0] == '"') {
		cfg_names = remove_quotes(util::unescape_string(cfg_name));
	}
	else  {
		Variable &v1 = find_variable(prg, cfg_name);
		if (v1.type == Variable::STRING) {
			cfg_names = v1.s;
		}
		else {
			throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
		}
	}

	save_cfg(cfg_names);

	return true;
}

static bool cfgfunc_get_number(Program &prg, std::vector<std::string> &v)
{
	std::string dest = v[0];
	std::string name = v[1];

	std::string names;

	if (name[0] == '"') {
		names = remove_quotes(util::unescape_string(name));
	}
	else  {
		Variable &v1 = find_variable(prg, name);
		if (v1.type == Variable::STRING) {
			names = v1.s;
		}
		else {
			throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
		}
	}

	Variable &v1 = find_variable(prg, dest);

	if (v1.type != Variable::NUMBER) {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
	}

	v1.n = cfg_numbers[names];

	return true;
}

static bool cfgfunc_get_string(Program &prg, std::vector<std::string> &v)
{
	std::string dest = v[0];
	std::string name = v[1];

	std::string names;

	if (name[0] == '"') {
		names = remove_quotes(util::unescape_string(name));
	}
	else  {
		Variable &v1 = find_variable(prg, name);
		if (v1.type == Variable::STRING) {
			names = v1.s;
		}
		else {
			throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
		}
	}

	Variable &v1 = find_variable(prg, dest);

	if (v1.type != Variable::STRING) {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
	}

	v1.s = cfg_strings[names];

	return true;
}

static bool cfgfunc_set_number(Program &prg, std::vector<std::string> &v)
{
	std::string name = v[0];
	std::string value = v[1];

	std::string names;

	if (name[0] == '"') {
		names = remove_quotes(util::unescape_string(name));
	}
	else  {
		Variable &v1 = find_variable(prg, name);
		if (v1.type == Variable::STRING) {
			names = v1.s;
		}
		else {
			throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
		}
	}

	std::vector<std::string> strings;
	strings.reserve(1);
	strings.push_back(value);
	std::vector<double> values = variable_names_to_numbers(prg, strings);

	cfg_numbers[names] = values[0];

	return true;
}

static bool cfgfunc_set_string(Program &prg, std::vector<std::string> &v)
{
	std::string name = v[0];
	std::string value = v[1];

	std::string names;

	if (name[0] == '"') {
		names = remove_quotes(util::unescape_string(name));
	}
	else  {
		Variable &v1 = find_variable(prg, name);
		if (v1.type == Variable::STRING) {
			names = v1.s;
		}
		else {
			throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
		}
	}

	std::string values;

	if (value[0] == '"') {
		values = remove_quotes(util::unescape_string(value));
	}
	else  {
		Variable &v1 = find_variable(prg, value);
		if (v1.type == Variable::STRING) {
			values = v1.s;
		}
		else {
			throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
		}
	}

	cfg_strings[names] = values;

	return true;
}

static bool cfgfunc_number_exists(Program &prg, std::vector<std::string> &v)
{
	std::string dest = v[0];
	std::string name = v[1];

	std::string names;

	if (name[0] == '"') {
		names = remove_quotes(util::unescape_string(name));
	}
	else  {
		Variable &v1 = find_variable(prg, name);
		if (v1.type == Variable::STRING) {
			names = v1.s;
		}
		else {
			throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
		}
	}

	int found_n;

	if (cfg_numbers.find(names) == cfg_numbers.end()) {
		found_n = 0;
	}
	else {
		found_n = 1;
	}

	Variable &v1 = find_variable(prg, dest);

	if (v1.type == Variable::NUMBER) {
		v1.n = found_n;
	}
	else {
		throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
	}

	return true;
}

static bool cfgfunc_string_exists(Program &prg, std::vector<std::string> &v)
{
	std::string dest = v[0];
	std::string name = v[1];

	std::string names;

	if (name[0] == '"') {
		names = remove_quotes(util::unescape_string(name));
	}
	else  {
		Variable &v1 = find_variable(prg, name);
		if (v1.type == Variable::STRING) {
			names = v1.s;
		}
		else {
			throw util::ParseError(std::string(__FUNCTION__) + ": " + "Invalid type on line " + util::itos(get_line_num(prg)));
		}
	}

	int found_n;

	if (cfg_strings.find(names) == cfg_strings.end()) {
		found_n = 0;
	}
	else {
		found_n = 1;
	}

	Variable &v1 = find_variable(prg, dest);

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
	add_syntax("function", corefunc_function);
	add_syntax(";", corefunc_comment);
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
