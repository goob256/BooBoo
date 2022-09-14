#include "shim4/primitives.h"
#include "shim4/shim.h"
#include "shim4/util.h"
#include "shim4/vertex_cache.h"

using namespace noo;

static bool primitives_held;
static bool vertex_cache_already_held;

static void draw_line_worker(SDL_Colour colour, noo::util::Point<float> a, noo::util::Point<float> b, float thickness)
{
	float half_thickness = thickness / 2.0f;
	SDL_Colour vertex_colours[4];
	for (int i = 0; i < 4; i++) {
		vertex_colours[i] = colour;
	}
	float dx = float(a.x - b.x);
	float dy = float(a.y - b.y);
	float angle = atan2f(dy, dx);
	/* Make 4 points for thickness */
	float a1 = angle + (float)M_PI / 2.0f;
	float a2 = angle - (float)M_PI / 2.0f;
	noo::util::Point<float> da = a;
	noo::util::Point<float> db = a;
	noo::util::Point<float> dc = b;
	noo::util::Point<float> dd = b;
	da.x += cos(a1) * half_thickness;
	da.y += sin(a1) * half_thickness;
	db.x += cos(a2) * half_thickness;
	db.y += sin(a2) * half_thickness;
	dc.x += cos(a1) * half_thickness;
	dc.y += sin(a1) * half_thickness;
	dd.x += cos(a2) * half_thickness;
	dd.y += sin(a2) * half_thickness;
	noo::gfx::Vertex_Cache::instance()->cache(vertex_colours, {0.0f, 0.0f}, {0.0f, 0.0f}, da, dc, dd, db, 0);
}

static void draw_straight_line_worker(SDL_Colour colour, noo::util::Point<float> a, noo::util::Point<float> b, float thickness)
{
	SDL_Colour vertex_colours[4];
	for (int i = 0; i < 4; i++) {
		vertex_colours[i] = colour;
	}
	noo::util::Point<float> da, db, dc, dd;
	if (a.x == b.x) {
		da = a;
		db = a+noo::util::Point<float>(thickness, 0.0f);
		dc = b+noo::util::Point<float>(thickness, 0.0f);
		dd = b;
	}
	else {
		da = a;
		db = b;
		dc = b+noo::util::Point<float>(0.0f, thickness);
		dd = a+noo::util::Point<float>(0.0f, thickness);
	}
	noo::gfx::Vertex_Cache::instance()->cache(vertex_colours, {0.0f, 0.0f}, {0.0f, 0.0f}, da, db, dc, dd, 0);
}

namespace noo {

namespace gfx {

void static_start_primitives()
{
	primitives_held = false;
	vertex_cache_already_held = false;
}

void draw_primitives_start()
{
	if (noo::gfx::Vertex_Cache::instance()->is_started()) {
		vertex_cache_already_held = true;
	}
	else {
		noo::gfx::Vertex_Cache::instance()->start();
	}

	primitives_held = true;
}

void draw_primitives_end()
{
	if (vertex_cache_already_held == false) {
		noo::gfx::Vertex_Cache::instance()->end();
	}
	else {
		vertex_cache_already_held = false;
	}
	primitives_held = false;
}

void draw_line(SDL_Colour colour, util::Point<float> a, util::Point<float> b, float thickness)
{
	bool prim_held = primitives_held;
	if (prim_held == false) {
		draw_primitives_start();
	}
	draw_line_worker(colour, a, b, thickness);
	if (prim_held == false) {
		draw_primitives_end();
	}
}

void draw_filled_triangle(SDL_Colour vertex_colours[3], util::Point<float> a, util::Point<float> b, util::Point<float> c)
{
	bool prim_held = primitives_held;
	if (prim_held == false) {
		draw_primitives_start();
	}
	Vertex_Cache::instance()->cache(vertex_colours, a, b, c);
	if (prim_held == false) {
		draw_primitives_end();
	}
}

void draw_filled_triangle(SDL_Colour colour, util::Point<float> a, util::Point<float> b, util::Point<float> c)
{
	static SDL_Colour vertex_colours[3];
	vertex_colours[0] = colour;
	vertex_colours[1] = colour;
	vertex_colours[2] = colour;
	draw_filled_triangle(vertex_colours, a, b, c);
}

void draw_rectangle(SDL_Colour colour, util::Point<float> pos, util::Size<float> size, float thickness)
{
	bool prim_held = primitives_held;
	if (prim_held == false) {
		draw_primitives_start();
	}
	// top
	draw_straight_line_worker(colour, pos, pos+util::Size<float>(size.w, 0.0f), thickness);
	// bottom
	draw_straight_line_worker(colour, pos+util::Size<float>(0.0f, size.h-thickness), pos+util::Size<float>(size.w, size.h-thickness), thickness);
	// left
	draw_straight_line_worker(colour, pos+util::Point<float>(0.0f, thickness), pos+util::Size<float>(0.0f, size.h-thickness), thickness);
	// right
	draw_straight_line_worker(colour, pos+util::Point<float>(size.w-thickness, thickness), pos+util::Size<float>(size.w-thickness, size.h-thickness), thickness);
	if (prim_held == false) {
		draw_primitives_end();
	}
}

void draw_filled_ellipse(SDL_Colour colour, util::Point<float> centre, float rx, float ry, int sections, float start_angle)
{
	SDL_Colour colours[3] = { colour, colour, colour };

	bool prim_held = primitives_held;
	if (prim_held == false) {
		draw_primitives_start();
	}

	if (sections == -1) {
		sections = M_PI * MAX(rx, ry); // sections equal to half of circumference
	}

	if (sections < 4) {
		sections = 4;
	}

	for (int n = 0; n < sections; n++) {
		int n2 = (n+1) % sections;
		float a1 = start_angle + (float)n/sections * (float)M_PI * 2.0f;
		float a2 = start_angle + (float)n2/sections * (float)M_PI * 2.0f;
		noo::util::Point<float> a, b;
		a = centre + noo::util::Point<float>(cos(a1) * rx, sin(a1) * ry);
		b = centre + noo::util::Point<float>(cos(a2) * rx, sin(a2) * ry);
		noo::gfx::Vertex_Cache::instance()->cache(colours, centre, a, b);
	}

	if (prim_held == false) {
		draw_primitives_end();
	}
}

void draw_ellipse(SDL_Colour colour, util::Point<float> centre, float rx, float ry, float thickness, int sections, float start_angle)
{
	SDL_Colour colours[3] = { colour, colour, colour };

	bool prim_held = primitives_held;
	if (prim_held == false) {
		draw_primitives_start();
	}

	if (sections == -1) {
		sections = M_PI * MAX(rx, ry); // sections equal to half of circumference
	}

	if (sections < 4) {
		sections = 4;
	}

	for (int n = 0; n < sections; n++) {
		int n2 = (n+1) % sections;
		float a1 = start_angle + (float)n/sections * (float)M_PI * 2.0f;
		float a2 = start_angle + (float)n2/sections * (float)M_PI * 2.0f;
		float half = 0.5f/sections * (float)M_PI * 2.0f;
		float a3 = a1 + half;
		float a4 = (n == sections - 1) ? half : a2 + half;
		noo::util::Point<float> a, b, c, d;
		a = centre + noo::util::Point<float>(cos(a1) * rx, sin(a1) * ry);
		b = centre + noo::util::Point<float>(cos(a2) * rx, sin(a2) * ry);
		c = centre + noo::util::Point<float>(cos(a3) * (rx - thickness), sin(a3) * (ry - thickness));
		d = centre + noo::util::Point<float>(cos(a4) * (rx - thickness), sin(a4) * (ry - thickness));
		noo::gfx::Vertex_Cache::instance()->cache(colours, a, b, c);
		noo::gfx::Vertex_Cache::instance()->cache(colours, b, d, c);
	}

	if (prim_held == false) {
		draw_primitives_end();
	}
}

void draw_filled_circle(SDL_Colour colour, util::Point<float> centre, float radius, int sections, float start_angle)
{
	draw_filled_ellipse(colour, centre, radius, radius, sections, start_angle);
}

void draw_circle(SDL_Colour colour, util::Point<float> centre, float radius, float thickness, int sections, float start_angle)
{
	draw_ellipse(colour, centre, radius, radius, thickness, sections, start_angle);
}

void draw_filled_rectangle(SDL_Colour vertex_colours[4], util::Point<float> dest_position, util::Size<float> dest_size)
{
	bool prim_held = primitives_held;
	if (prim_held == false) {
		draw_primitives_start();
	}
	Vertex_Cache::instance()->cache(vertex_colours, {0.0f, 0.0f}, {0.0f, 0.0f}, dest_position, dest_size, 0);
	if (prim_held == false) {
		draw_primitives_end();
	}
}

void draw_filled_rectangle(SDL_Colour colour, util::Point<float> dest_position, util::Size<float> dest_size)
{
	static SDL_Colour vertex_colours[4];
	for (int i = 0; i < 4; i++) {
		vertex_colours[i] = colour;
	}
	draw_filled_rectangle(vertex_colours, dest_position, dest_size);
}

} // End namespace gfx

} // End namespace noo
