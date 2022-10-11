#ifndef BOOBOO_INTERNAL_H
#define BOOBOO_INTERNAL_H

#include <string>
#include <vector>
#include <map>

#include "booboo/booboo.h"

namespace booboo {

std::map<std::string, Label> process_labels(Program prg);
bool process_includes(Program &prg);
void process_functions(Program &prg);
void init_token_map();

} // end namespace booboo

#endif // BOOBOO_INTERNAL_H
