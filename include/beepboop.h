#ifndef BEEPBOOP_H
#define BEEPBOOP_H

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
	
	std::vector<VARIABLE> variables;
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
};

bool interpret(PROGRAM &prg);
std::vector<LABEL> find_labels(PROGRAM prg);
void destroy_program(PROGRAM &prg);
void start_beepboop();
void process_includes(PROGRAM &prg);

#endif // BEEPBOOP_H
