#ifndef BOOBOO_H
#define BOOBOO_H

#include <string>
#include <vector>

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

struct VARIABLE {
	enum VARIABLE_TYPE {
		NUMBER,
		STRING,
		VECTOR
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

bool interpret(PROGRAM &prg);
std::vector<LABEL> process_labels(PROGRAM prg);
void destroy_program(PROGRAM &prg, bool destroy_vectors);
void process_includes(PROGRAM &prg);
void call_function(PROGRAM &prg, std::string function_name, std::string result_name);
void booboo_init();
void booboo_shutdown();

#endif // BOOBOO_H
