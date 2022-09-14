#ifndef NOO_IOS_H
#define NOO_IOS_H

#include "shim4/util.h"

namespace noo {

namespace util {

std::string ios_get_resource_path(std::string file);
std::string ios_get_standard_path(Path_Type type);
void ios_mkdir(std::string path);
bool ios_is_ipad();

#ifdef TVOS
char *tvos_read_bytes(std::string filename, int *sz);
bool tvos_read_file(std::string filename, std::string &file);
bool tvos_save_bytes(std::string filename, char *bytes, int sz);
bool tvos_save_file(std::string filename, std::string file);
void tvos_delete_file(std::string filename);
Sint64 tvos_file_date(std::string filename);
void tvos_touch(std::string filename);
bool tvos_rename(std::string from, std::string to);
#endif

}

} // End namespace noo

#endif // NOO_IOS_H

