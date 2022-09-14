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
	struct STRUCT {
		std::vector<VARIABLE> variables;
		std::vector<STRUCT> children;
	};

	enum VARIABLE_TYPE {
		NUMBER,
		STRING,
		STRUCT,
		VECTOR
	} type;

	VARIABLE_TYPE vec_type;

	std::string name;

	double n;
	std::string s;
	struct STRUCT st;

	std::vector<double> vn;
	std::vector<std::string> vs;
	std::vector<struct STRUCT> vst;
	
	std::string function;
};

struct LABEL {
	std::string name;
	int line_number;
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
};

bool interpret(PROGRAM &prg);
std::vector<LABEL> find_labels(PROGRAM prg);

#endif // BEEPBOOP_H
