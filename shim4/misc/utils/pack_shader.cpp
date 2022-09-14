#include <shim3/shim3.h>

using namespace noo;

static void write_string(SDL_RWops *f, std::string s)
{
	if (s.length() > 255) {
		throw util::Error("Variable name too long! (" + s + ")");
	}
	util::SDL_fputc(s.length(), f);
	for (int i = 0; i < (int)s.length(); i++) {
		util::SDL_fputc(s[i], f);
	}
}

int main(int argc, char **argv)
{
	try {
		shim::static_start_all();
		
		shim::argc = argc;
		shim::argv = argv;
		shim::use_cwd = true;
		shim::error_level = 3;
		shim::log_tags = false;

		util::start();

		if (argc < 3) {
			util::infomsg("Usage: pack_shader <file.txt> [v|p]\n");
			return 0;
		}

		std::string text = std::string(util::slurp_file_from_filesystem(std::string(argv[1]), 0));

		for (size_t i = 0; i < text.length(); i++) {
			if (text[i] == '\n') {
				text[i] = ' ';
			}
		}

		// find sampler names
		std::map<std::string, std::string> sampler_names;

		util::Tokenizer t3(text, ' ', true);
		std::string l;
		while ((l = t3.next()) != "") {
			util::trim(l);
			if (l == "sampler2D") {
				std::string id = t3.next();
				util::trim(id);
				t3.next();
				t3.next();
				t3.next();
				t3.next();
				t3.next();
				std::string name = t3.next();
				util::trim(name);
				name = name.substr(1);
				name = name.substr(0, name.length()-2);
				sampler_names[id] = name;
			}
		}
		
		std::string h_filename = std::string(argv[1]);
		size_t dot = h_filename.rfind('.');
		h_filename = h_filename.substr(0, dot+1) + "h";

		std::string command = "fxc /Fh " + h_filename + " /T " + (std::string(argv[2]) == "v" ? "vs_3_0" : "ps_3_0") + " " + std::string(argv[1]);

		system(command.c_str());

		std::string orig = std::string(util::slurp_file_from_filesystem(h_filename, 0));

		if (orig == "") {
			throw util::LoadError("Empty file!");
			return 1;
		}

		std::string out_filename = h_filename;
		dot = out_filename.rfind('.');
		out_filename = out_filename.substr(0, dot+1) + "shader";

		SDL_RWops *f = SDL_RWFromFile(out_filename.c_str(), "wb");

		if (f == 0) {
			return 1;
		}

		util::Tokenizer t(orig, '\n');

		std::string line;

		bool in_registers = false;
		int count_to_registers = -1;
		bool done_registers = false;
		bool in_data = false;
		std::vector<std::string> registers;
		
		while ((line = t.next()) != "") {
			util::trim(line);
			if (done_registers == false) {
				if (in_registers == false) {
					if (line == "// Registers:") {
						in_registers = true;
						count_to_registers = 3;
					}
					else if (line == "#endif") {
						util::SDL_fputc(0, f); // no registers used
						done_registers = true;
					}
				}
				else {
					if (count_to_registers > 0) {
						count_to_registers--;
					}
					else {
						if (line == "//") {
							in_registers = false;
							done_registers = true;
							util::SDL_fputc(registers.size(), f);
							for (size_t i = 0; i < registers.size(); i++) {
								std::string line = registers[i];
								util::Tokenizer t2(line, ' ', true);
								t2.next(); // skip "//"
								std::string name = t2.next();
								std::string reg = t2.next();
								std::string sz_s = t2.next();
								util::trim(name);
								util::trim(reg);
								util::trim(sz_s);
								int reg_num = atoi(reg.c_str()+1);
								int sz_num = atoi(sz_s.c_str());
								std::string n;
								if (reg[0] == 's') {
									n = sampler_names[name];
								}
								else {
									n = name;
								}
								if (util::check_args(shim::argc, shim::argv, "+debug") > 0) {
									util::infomsg("%c%d=%s (size=%d)\n", reg[0], reg_num, n.c_str(), sz_num);
								}
								write_string(f, n);
								util::SDL_fputc(reg[0], f);
								util::SDL_fputc(reg_num, f);
								SDL_WriteLE32(f, sz_num);
							}
						}
						else {
							registers.push_back(line);
						}
					}
				}
			}
			else {
				if (in_data == false) {
					if (line == "{") {
						in_data = true;
					}
				}
				else {
					if (line == "};") {
						break;
					}
					else {
						util::Tokenizer t2(line, ',');
						std::string byte;
						while ((byte = t2.next()) != "") {
							util::trim(byte);
							util::SDL_fputc(atoi(byte.c_str()), f);
						}
					}
				}
			}
		}

		SDL_RWclose(f);

		util::end();

		shim::static_end();
	}
	catch (util::Error e) {
		util::errormsg("Fatal error: %s\n", e.error_message.c_str());
		return 1;
	}

	return 0;
}
