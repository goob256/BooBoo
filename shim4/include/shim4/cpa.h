// Crystal Picnic Archive: archive format created for the game Crystal Picnic.
// Now we only use uncompressed archives.

#ifndef NOO_CPA_H
#define NOO_CPA_H

#include "shim4/main.h"

namespace noo {

namespace util {

class SHIM4_EXPORT CPA
{
public:
	// Only one file returned from this function can be used at a time,
	// because it's really an SDL_RWops pointing to the data file. Don't
	// close the file and use sz to determine the end, don't rely on
	// SDL_fgetc etc returning EOF. If you need to get SDL_fgetc to return
	// EOF, create a memory file with SDL_RWFromMem using the value this
	// returns.
	// Above does not apply if loading from the filesystem. In both cases,
	// you should always use close (below) to close the file, and never
	// use SDL_RWclose directly.
	SDL_RWops *open(std::string filename, int *sz, bool data_only = false);
	void close(SDL_RWops *file);
	bool exists(std::string filename);
	std::vector<std::string> get_all_filenames();
	void free_data(SDL_RWops *file); // special case where something already closed the file. same as close() except doesn't close the file

	CPA(); // tries CPA archive then data/ directory
	CPA(std::string argv0); // tries EXE
	CPA(Uint8 *buf, int sz); // tries buffer
	~CPA();

private:
	struct Info {
		int offset;
		int compressed_size;
		int uncompressed_size;
		int num_open;
		Uint8 *data;
	};

	void load_datafile();
	void delete_bytes(SDL_RWops *file);

	SDL_RWops *file;

	std::map<std::string, Info> info;
	std::map<SDL_RWops *, std::string> files;
	std::map<SDL_RWops *, Uint8 *> bytes;

	bool load_from_filesystem;
	bool load_from_exe;
	int exe_data_offset;
};

} // End namespace util

} // End namespace noo

#endif // NOO_CPA_H
