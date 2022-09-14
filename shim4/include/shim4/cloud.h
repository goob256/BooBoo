#ifndef NOO_CLOUD_H
#define NOO_CLOUD_H

#include "shim4/main.h"

namespace noo {

namespace util {

//bool cloud_exists(std::string filename);
Sint64 cloud_date(std::string filename); // modified date
//bool cloud_mkdir(std::string filename);
bool cloud_delete(std::string filename);
char *cloud_read(std::string filename, int *sz);
bool cloud_save(std::string filename, const char *bytes, int sz);
void cloud_synchronise();
int cloud_get_error_code();

} // End namespace util

} // End namespace noo

#endif // NOO_CLOUD_H
