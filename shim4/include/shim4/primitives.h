#ifndef NOO_PRIMITIVES_H
#define NOO_PRIMITIVES_H

#include "shim4/main.h"

namespace noo {

namespace gfx {

void SHIM4_EXPORT static_start_primitives();

// these are for batching (speeds up drawing)
void SHIM4_EXPORT draw_primitives_start();
void SHIM4_EXPORT draw_primitives_end();

void SHIM4_EXPORT draw_line(SDL_Colour colour, util::Point<float> a, util::Point<float> b, float thickness = 1.0f);
// winding order matters for triangles (use gfx::set_cull_mode(gfx::NO_FACE) to never cull these)
void SHIM4_EXPORT draw_filled_triangle(SDL_Colour vertex_colours[3], util::Point<float> a, util::Point<float> b, util::Point<float> c);
void SHIM4_EXPORT draw_filled_triangle(SDL_Colour colour, util::Point<float> a, util::Point<float> b, util::Point<float> c);
void SHIM4_EXPORT draw_filled_triangle_3d(SDL_Colour vertex_colours[3], util::Vec3D<float> a, util::Vec3D<float> b, util::Vec3D<float> c);
void SHIM4_EXPORT draw_filled_triangle_3d(SDL_Colour colour, util::Vec3D<float> a, util::Vec3D<float> b, util::Vec3D<float> c);
void SHIM4_EXPORT draw_rectangle(SDL_Colour colour, util::Point<float> pos, util::Size<float> size, float thickness = 1.0f);
void SHIM4_EXPORT draw_filled_rectangle(SDL_Colour vertex_colours[4], util::Point<float> dest_position, util::Size<float> dest_size);
void SHIM4_EXPORT draw_filled_rectangle(SDL_Colour colour, util::Point<float> dest_position, util::Size<float> dest_size);
void SHIM4_EXPORT draw_ellipse(SDL_Colour colour, util::Point<float> centre, float rx, float ry, float thickness = 1.0f, int sections = -1, float start_angle = 0.0f);
void SHIM4_EXPORT draw_filled_ellipse(SDL_Colour colour, util::Point<float> centre, float rx, float ry, int sections = -1, float start_angle = 0.0f);
void SHIM4_EXPORT draw_circle(SDL_Colour colour, util::Point<float> centre, float radius, float thickness = 1.0f, int sections = -1, float start_angle = 0.0f);
void SHIM4_EXPORT draw_filled_circle(SDL_Colour colour, util::Point<float> centre, float radius, int sections = -1, float start_angle = 0.0f);

} // End namespace gfx

} // End namespace noo

#endif // NOO_PRIMITIVES_H
