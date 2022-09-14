#ifndef NOO_CD_H
#define NOO_CD_H

#include "shim4/main.h"

namespace noo {

namespace cd {

bool SHIM4_EXPORT box_box(util::Point<float> topleft_a, util::Point<float> bottomright_a, util::Point<float> topleft_b, util::Point<float> bottomright_b);
bool SHIM4_EXPORT box_box(util::Point<float> topleft_a, util::Size<float> size_a, util::Point<float> topleft_b, util::Size<float> size_b);
bool SHIM4_EXPORT line_line(const util::Point<float> *a1, const util::Point<float> *a2,	const util::Point<float> *a3, const util::Point<float> *a4, util::Point<float> *result);
float SHIM4_EXPORT dist_point_line(util::Point<float> point, util::Point<float> a, util::Point<float> b);

} // End namespace cd

} // End namespace noo

#endif // NOO_CD_H
