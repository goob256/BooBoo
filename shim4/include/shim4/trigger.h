#ifndef NOO_TRIGGER_H
#define NOO_TRIGGER_H

#include "shim4/main.h"

namespace noo {

namespace util {

class SHIM4_EXPORT Trigger {
public:
	virtual void run() = 0;
};

} // End namespace util

} // End namespace noo

#endif // NOO_TRIGGER_H
