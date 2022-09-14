#include "shim3/shim3.h"
#include "shim3/savetool.h"

#ifdef _WIN32
#define chdir _chdir
#define getcwd _getcwd
#endif

std::vector<std::string> filenames;

FILE *out;
std::string curr_dir;

int go(std::string path, bool print_only)
{
	util::List_Directory l("*");

	std::string fn;

	int size = 0;

	while ((fn = l.next()) != "") {
		if (fn[0] == '.') {
			continue;
		}
		if (fn.find('.') != std::string::npos) {
			if (print_only == false) {
				size += do_compress(fn.c_str(), "__tmp__", false);
				remove(fn.c_str());
				rename("__tmp__", fn.c_str());
				filenames.push_back(path + fn);
			}
			else {
				FILE *f = fopen((curr_dir + "/" + path + fn).c_str(), "rb");
				fseek(f, 0, SEEK_END);
				int sz = ftell(f);
				fprintf(out, "%d ", sz);
				fclose(f);
				f = fopen(fn.c_str(), "rb");
				fseek(f, 0, SEEK_END);
				sz = ftell(f);
				fprintf(out, "%d %s\n", sz, (path + fn).c_str());
				fclose(f);
			}
		}
		else {
			chdir(fn.c_str());
			size += go(path + fn + "/", print_only);
			chdir("..");
		}
	}

	return size;
}

int main(int argc, char **argv)
{
	if (shim::static_start_all() == false) {
		return 1;
	}

	if (util::start() == false) {
		return 1;
	}

	out = fopen("../data.cpa", "wb");

	fputc('C', out);
	fputc('P', out);
	fputc('A', out);
	fputc('2', out);

	std::string tmpdir = "__tmp__hopefully_you_dont_have_anything_named_this__";

	char buf[1000];
	getcwd(buf, 1000);
	curr_dir = buf;

#ifdef _WIN32
	system((std::string("xcopy /q /e /y ..\\data ..\\") + tmpdir + "\\").c_str());
	chdir((std::string("..\\") + tmpdir).c_str());
#else
	system((std::string("cp -a ../data ../") + tmpdir).c_str());
	chdir((std::string("../") + tmpdir).c_str());
#endif

	int size = go("", false);

	fprintf(out, "%d\n", size+1); // +1 for the newline below

	for (size_t i = 0; i < filenames.size(); i++) {
		std::string fn = filenames[i];
		FILE *f = fopen(fn.c_str(), "rb");
		fseek(f, 0, SEEK_END);
		int sz = ftell(f);
		fseek(f, 0, SEEK_SET);
		char *bytes = new char[sz];
		fread(bytes, sz, 1, f);
		fwrite(bytes, sz, 1, out);
		fclose(f);
		delete[] bytes;
	}

	fprintf(out, "\n");

	go("", true);

	chdir(curr_dir.c_str());

#ifdef _WIN32
	system((std::string("rmdir /s /q ..\\") + tmpdir).c_str());
#else
	system((std::string("rm -rf ../") + tmpdir).c_str());
#endif

	fclose(out);

	util::end();

	shim::static_end();

	return 0;
}
