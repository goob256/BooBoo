#ifndef NOO_APPLE_H
#define NOO_APPLE_H

#include <string>

namespace noo {

namespace util {

std::string apple_get_system_language();

#if defined IOS || defined MAS
void apple_set_cloud_info(std::string _cloud_container_name, std::string _cloud_record_name);
Sint64 apple_cloud_date(std::string filename);
bool apple_cloud_delete(std::string filename);
char *apple_cloud_read(std::string filename, int *sz);
bool apple_cloud_save(std::string filename, const char *bytes, int sz);
void apple_cloud_synchronise();
int apple_cloud_get_error_code();
#endif

} // End namespace util

} // End namespace noo

#endif // NOO_APPLE_H

