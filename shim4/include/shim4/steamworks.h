#ifndef NOO_STEAMWORKS_H
#define NOO_STEAMWORKS_H

#ifdef STEAMWORKS

#include <steam/steam_api.h>

namespace noo {

namespace util {

bool achieve_steam(std::string name);
bool start_steamworks();
std::string get_steam_language();


} // End namespace util

} // End namespace noo

#endif // STEAMWORKS

#endif // NOO_STEAMWORKS_H
