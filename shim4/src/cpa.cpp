#include <zlib.h>

#include "shim4/cpa.h"
#include "shim4/error.h"
#include "shim4/gui.h"
#include "shim4/shim.h"
#include "shim4/util.h"

#ifdef IOS
#include "shim4/ios.h"
#endif

using namespace noo;

static Uint8 *safe_find_char(Uint8 *haystack, char needle, Uint8 *end)
{
	Uint8 *p = haystack;
	while (p != end) {
		if (*p == needle) {
			return p;
		}
		p++;
	}
	return 0;
}

namespace noo {

namespace util {

SDL_RWops *CPA::open(std::string filename, int *sz, bool data_only)
{
	if (load_from_filesystem) {
#if !defined ANDROID && !defined IOS
		if (shim::use_cwd == false) {
			char *base = SDL_GetBasePath();
			filename = std::string(base) + "data/" + filename;
			SDL_free(base);
		}
		else
#endif
		{
			filename = "data/" + filename;
		}
		SDL_RWops *file = SDL_RWFromFile(filename.c_str(), "rb");
		int size = 0;
		if (file) {
			size = (int)SDL_RWsize(file);
			if (sz) {
				*sz = size;
			}
		}
		if (file == 0) {
			return 0;
		}
		// Read the whole file into a memory buffer because reads are slow on Android
		Uint8 *buf = new Uint8[size];
		if (buf) {
			int remain = size;
			while (remain >= 8192) {
				int result = (int)SDL_RWread(file, buf+(size-remain), 8192, 1);
				if (result != 1) {
					break;
				}
				remain -= 8192;
			}
			if (remain > 0) {
				int result = (int)SDL_RWread(file, buf+(size-remain), remain, 1);
				if (result == 1) {
					remain = 0;
				}
			}
			SDL_RWclose(file);
			if (data_only) {
				return (SDL_RWops *)buf;
			}
			if (remain == 0) {
				SDL_RWops *memfile = SDL_RWFromMem(buf, size);
				if (memfile) {
					bytes[memfile] = buf;
					return memfile;
				}
				else {
					delete[] buf;
					return 0;
				}
			}
			else {
				delete[] buf;
				return 0;
			}
		}
		else {
			SDL_RWclose(file);
			return 0;
		}
	}

	if (!exists(filename)) {
		return 0;
	}

	Info &i = info[filename];

	if (sz != 0) {
		*sz = (int)i.uncompressed_size;
	}

	SDL_RWseek(file, i.offset + exe_data_offset, RW_SEEK_SET);

	Uint8 *uncompressed;

	if (i.data == 0) {
		uncompressed = new Uint8[i.uncompressed_size];
		i.data = uncompressed;
		i.num_open = 1;
	}
	else {
		uncompressed = i.data;
		i.num_open++;
	}

	Uint8 *compressed = new Uint8[i.compressed_size];
			
	int remain = i.compressed_size;
	while (remain >= 8192) {
		int result = (int)SDL_RWread(file, compressed+(i.compressed_size-remain), 8192, 1);
		if (result != 1) {
			break;
		}
		remain -= 8192;
	}
	if (remain > 0) {
		int result = (int)SDL_RWread(file, compressed+(i.compressed_size-remain), remain, 1);
		if (result == 1) {
			remain = 0;
		}
	}
	if (remain != 0) {
		delete[] compressed;
		i.num_open--;
		if (i.num_open == 0) {
			delete[] i.data;
			i.data = 0;
		}
		return 0;
	}

	unsigned long u = i.uncompressed_size;
	if (uncompress(uncompressed, &u, compressed, i.compressed_size) != Z_OK) {
		delete[] compressed;
		i.num_open--;
		if (i.num_open == 0) {
			delete[] i.data;
			i.data = 0;
		}
		return 0;
	}

	delete[] compressed;

	if (data_only) {
		// with data_only, all deallocations are up to the caller
		i.num_open--;
		if (i.num_open == 0) {
			i.data = 0;
		}
		return (SDL_RWops *)uncompressed;
	}

	SDL_RWops *f = SDL_RWFromMem(uncompressed, (int)u);

	files[f] = filename;

	return f;
}

void CPA::free_data(SDL_RWops *file)
{
	if (load_from_filesystem == false) {
		std::string filename = files[file];
		std::map<std::string, Info>::iterator it = info.find(filename);
		if (it != info.end()) {
			std::pair<const std::string, Info> &p = *it;
			Info &i = p.second;
			i.num_open--;
			if (i.num_open == 0) {
				delete[] i.data;
				i.data = 0;
			}
		}
	}
	else {
		std::map<SDL_RWops *, Uint8 *>::iterator it = bytes.find(file);
		if (it != bytes.end()) {
			Uint8 *buf = (*it).second;
			delete[] buf;
			bytes.erase(it);
		}
	}
}
void CPA::close(SDL_RWops *file)
{
	SDL_RWclose(file);
	free_data(file);
}

bool CPA::exists(std::string filename)
{
	return info.find(filename) != info.end();
}

std::vector<std::string> CPA::get_all_filenames()
{
	std::vector<std::string> v;
	
#if !defined ANDROID && !defined IOS
	if (load_from_filesystem) {
		char *base = SDL_GetBasePath();

		// Read directory listing from filesystem

		std::vector< std::vector<std::string> > stack;
		std::vector<std::string> name_stack;
		std::vector<std::string> curr;
		List_Directory l(std::string(base) + "data/*");
		std::string s;

		while ((s = l.next()) != "") {
			if (s.c_str()[0] != '.') {
				curr.push_back(s);
			}
		}

		stack.push_back(curr);
		name_stack.push_back(std::string(SDL_GetBasePath()) + "data");

		while (stack.size() > 0) {
			while (stack[0].size() > 0) {
				s = stack[0][0];
				stack[0].erase(stack[0].begin());

				// we only want the filename
				if (s.rfind("/") != std::string::npos) {
					s = s.substr(s.rfind("/") + 1);
				}

				std::string dir_name;

				for (int i = (int)name_stack.size()-1; i >= 0; i--) {
					dir_name += name_stack[i] + "/";
				}

				List_Directory l(dir_name + s + "/*");

				std::string s2;

				curr.clear();

				while ((s2 = l.next()) != "") {
					if (s2.c_str()[0] != '.') {
						curr.push_back(s2);
					}
				}

				if (curr.size() > 0) {
					stack.insert(stack.begin(), curr);
					name_stack.insert(name_stack.begin(), s);
				}
				else {
					std::string filename = dir_name + s;
					filename = filename.substr(filename.rfind("data/") + 5); // chop path leading up to and including data/
					v.push_back(filename);
				}
			}
			stack.erase(stack.begin());
			name_stack.erase(name_stack.begin());
		}

		SDL_free(base);
	}
	else
#else
	if (load_from_filesystem) {
		// unsupported ATM
	}
#endif
	{
		std::map<std::string, Info>::iterator it;

		for (it = info.begin(); it != info.end(); it++) {
			std::pair<std::string, Info> p = *it;
			v.push_back(p.first);
		}
	}
	
	return v;
}

CPA::CPA() :
	load_from_filesystem(false),
	load_from_exe(false),
	exe_data_offset(0)
{
	bool loaded_cpa = false;

	file = 0;
#if defined ANDROID || defined IOS
	load_from_filesystem = true;
#elif defined ANDROID_XXX
	// Don't use a compressed .cpa on Android -- the APK is already compressed
	//file = SDL_RWFromFile("data.cpa", "rb");
#elif defined IOS_XXX 
	// Don't use a compressed .cpa on iOS -- it's too slow
	//std::string path = ios_get_resource_path("data.cpa");
	//file = SDL_RWFromFile(path.c_str(), "rb");
#elif defined __APPLE__
	char *base = SDL_GetBasePath();
	std::string filename = std::string(base) + "data.cpa";
	SDL_free(base);
	file = SDL_RWFromFile(filename.c_str(), "rb");
#else
	List_Directory ld("*.cpa");
	std::string filename;

	while ((filename = ld.next()) != "") {
		file = SDL_RWFromFile(filename.c_str(), "rb");
		if (file != 0) {
			infomsg("Using %s.\n", filename.c_str());
			break;
		}
	}
#endif

	if (file != 0) {
		try {
			load_datafile();
			loaded_cpa = true;
		}
		catch (Error &e) {
			infomsg("Couldn't load CPA (%s), trying to load from the filesystem...\n", e.error_message.c_str());
		}
	}

#if !defined ANDROID && !defined IOS
	if (loaded_cpa == false) {
		List_Directory ld("*");
		std::string filename;

		while ((filename = ld.next()) != "") {
			if (filename == "data") {
				infomsg("Using data/.\n");
				load_from_filesystem = true;
				break;
			}
		}
	}
#endif
	
	if (loaded_cpa == false && load_from_filesystem == false) {
		throw Error("No CPA archive or data/ directory found!");
	}
}

CPA::CPA(std::string argv0) :
	load_from_filesystem(false),
	load_from_exe(true)
{
#ifdef _WIN32
	TCHAR exename[MAX_PATH];
	GetModuleFileName(NULL, exename, MAX_PATH);
	argv0 = exename;
#endif
	file = SDL_RWFromFile(argv0.c_str(), "rb");

	if (file == 0) {
		throw FileNotFoundError("Can't open exe (" + argv0 + ")");
	}

	SDL_RWseek(file, -(4+shim::cpa_extra_bytes_after_exe_data), RW_SEEK_END);

	int cpa_size = SDL_ReadLE32(file);

	SDL_RWseek(file, -(4+cpa_size+shim::cpa_extra_bytes_after_exe_data), RW_SEEK_END);

	exe_data_offset = (int)SDL_RWtell(file);

	try {
		load_datafile();
		infomsg("Using EXE data.\n");
	}
	catch (Error &e) {
		SDL_RWclose(file);
		file = 0;
		throw Error("No data in executable!");
	}
}

CPA::CPA(Uint8 *buf, int sz) :
	load_from_filesystem(false),
	load_from_exe(false),
	exe_data_offset(0)
{
	file = SDL_RWFromMem(buf, sz);

	try {
		load_datafile();
	}
	catch (Error &e) {
		throw Error("No datafile detected in buffer!");
	}
}

CPA::~CPA()
{
	for (std::map<std::string, Info>::iterator it = info.begin(); it != info.end(); it++) {
		std::pair<std::string, Info> p = *it;
		if (p.second.data != 0) {
			util::infomsg("Unfreed CPA data (unclosed file): %s!\n", p.first.c_str());
			delete[] p.second.data;
		}
	}

	for (std::map<SDL_RWops *, Uint8 *>::iterator it = bytes.begin(); it != bytes.end(); it++) {
		std::pair<SDL_RWops *, Uint8 *> p = *it;
		util::infomsg("Unclosed file!\n");
		delete[] p.second;
	}

	if (file) {
		SDL_RWclose(file);
	}
}

void CPA::load_datafile()
{
	Uint32 magic = SDL_ReadBE32(file);
	Uint32 cpa2 = ((Uint32)'C' << 24) | ((Uint32)'P' << 16) | ((Uint32)'A' << 8) | (Uint32)'2';

	if (magic != cpa2) {
		throw Error(string_printf("CPA magic is unknown! (0x%x)", magic));
	}

	std::string header_compressed;
	Uint8 c;
	int header_size = 4;

	while (1) {
		if (SDL_RWread(file, &c, 1, 1) != 1) {
			throw Error("Invalid CPA: corrupt header");
		}
		header_size++;
		if (c == '\n') {
			break;
		}
		header_compressed.push_back(c);
	}

	int data_size = atoi(header_compressed.c_str());

	// Skip to the info section at the end
	SDL_RWseek(file, header_size + data_size + exe_data_offset, RW_SEEK_SET);
	// Keep track of the byte offset of each file
	int count = header_size;

	int total_size = 0;

	Uint8 line[1000];

	while (1) {
		int i;
		for (i = 0; i < 999; i++) {
			if (SDL_RWread(file, &c, 1, 1) != 1) {
				// end of index
				break;
			}
			line[i] = c;
			if (c == '\n') {
				break;
			}
		}

		line[i] = 0;

		if (line[0] == 0) {
			break;
		}

		if (i < 999) {
			char compressed_size_text[1000];
			char uncompressed_size_text[1000];
			char name_text[1000];
			Uint8 *uncompressed_size_text_end = (Uint8 *)safe_find_char(line, ' ', line+i);
			if (uncompressed_size_text_end == 0) {
				throw Error("Invalid CPA: corrupt info section");
			}
			Uint8 *compressed_size_text_end = (Uint8 *)safe_find_char(uncompressed_size_text_end+1, ' ', line+i);
			if (compressed_size_text_end == 0) {
				throw Error("Invalid CPA: corrupt info section");
			}
			memcpy(uncompressed_size_text, line, uncompressed_size_text_end-line);
			uncompressed_size_text[uncompressed_size_text_end-line] = 0;
			memcpy(compressed_size_text, uncompressed_size_text_end+1, line+i-uncompressed_size_text_end-1);
			uncompressed_size_text[line+i-uncompressed_size_text_end-1] = 0;
			memcpy(name_text, compressed_size_text_end+1, line+i-compressed_size_text_end-1);
			name_text[line+i-compressed_size_text_end-1] = 0;
			Info inf;
			inf.offset = count;
			inf.compressed_size = atoi(compressed_size_text);
			inf.uncompressed_size = atoi(uncompressed_size_text);
			inf.num_open = 0;
			inf.data = 0;
			info[name_text] = inf;
			count += inf.compressed_size;
			total_size += inf.compressed_size;
		}

		if (load_from_exe == true && SDL_RWtell(file)+4+shim::cpa_extra_bytes_after_exe_data == SDL_RWsize(file)) {
			break;
		}
	}

	if (total_size > data_size) {
		throw Error("Invalid CPA: total file sizes > data size");
	}
}

} // End namespace util

} // End namespace noo
