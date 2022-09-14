#include "shim4/error.h"
#include "shim4/gfx.h"
#include "shim4/shader.h"
#include "shim4/util.h"
#include "shim4/vertex_cache.h"

#include "shim4/internal/gfx.h"

/* So textures don't bleed into each other when tiling. This is about 1/100th of a pixel on a 1024x1024 texture (less for smaller) */
#define SMALL_TEXTURE_OFFSET 0.00001f
#define CLAMP(v) if (v < 0.0f) v = 0.0f; else if (v > 1.0f) v = 1.0f;

#define INCR_SIZE 1024

using namespace noo;

namespace noo {

namespace gfx {

Vertex_Cache *Vertex_Cache::v;

void Vertex_Cache::static_start()
{
	v = 0;
}

Vertex_Cache *Vertex_Cache::instance()
{
	if (v == 0) {
		try {
			v = new Vertex_Cache();
		}
		catch (util::Error &e) {
			return 0;
		}
	}

	return v;
}

void Vertex_Cache::destroy()
{
	delete v;
	v = 0;
}

Vertex_Cache::Vertex_Cache()
{
	select_cache(0);
}

Vertex_Cache::~Vertex_Cache()
{
	for (std::map<Uint32, float *>::iterator it = vertices.begin(); it != vertices.end(); it++) {
		free((*it).second);
	}
}

void Vertex_Cache::start(bool repeat)
{
	if (*_image != 0) {
		end();
	}

	*_image = 0;
	*_repeat = repeat;
	*_started = true;
}

void Vertex_Cache::start(Image *image, bool repeat)
{
	Image *root;

	if (image == 0) {
		root = 0;
		if (*_image != root) {
			end();
		}
	}
	else {
		root = image->get_root();
		if (*_image != root) {
			end();
		}
	}

	*_image = root;
	*_repeat = repeat;
	*_started = true;
}

void Vertex_Cache::end()
{
	if (*_started == false) {
		return;
	}

	if (*_count == 0) {
		reset();
		return;
	}

#ifdef VERBOSE
	util::debugmsg("Vertex_Cache::end(): image=%s count=%d size=%dx%d\n", *_image == 0 ? "(null)" : (*_image)->filename.c_str(), *_count, *_image == 0 ? 0 : (*_image)->size.w, *_image == 0 ? 0 : (*_image)->size.h);
#endif

	if (*_image) {
		if (shim::current_shader == shim::default_shader) {
			shim::current_shader = internal::gfx_context.textured_shader;
			shim::current_shader->use();
			update_projection();
		}
		shim::current_shader->set_texture("tex", *_image);
		shim::current_shader->set_bool("use_tex", true); // some shaders user this
	}
	else {
		shim::current_shader->set_bool("use_tex", false); // some shaders user this
	}

	if (shim::opengl) {
		shim::current_shader->set_opengl_attributes(*_vertices, &(*_vertices)[3], &(*_vertices)[6], &(*_vertices)[8]);
		glDrawArrays_ptr(GL_TRIANGLES, 0, *_count);
		PRINT_GL_ERROR("glDrawArrays\n");
	}
#ifdef _WIN32
	else if (internal::gfx_context.d3d_lost == false) {
#ifdef USE_D3DX
		if (shim::current_shader->is_d3dx()) {
			LPD3DXEFFECT d3d_effect = shim::current_shader->get_d3d_effect();
			unsigned int required_passes;
			d3d_effect->Begin(&required_passes, 0);
			for (unsigned int i = 0; i < required_passes; i++) {
				d3d_effect->BeginPass(i);
				if (shim::d3d_device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, *_count / 3, (void *)*_vertices, 12*sizeof(float)) != D3D_OK) {
					util::infomsg("DrawPrimitiveUP failed.\n");
					break;
				}
				d3d_effect->EndPass();
			}
			d3d_effect->End();
		}
		else
#endif
		{
			if (shim::d3d_device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, *_count / 3, (void *)*_vertices, 12*sizeof(float)) != D3D_OK) {
				util::infomsg("DrawPrimitiveUP failed.\n");
			}
		}
	}
#endif

	if (*_image) {
		if (shim::current_shader == internal::gfx_context.textured_shader) {
			shim::current_shader = shim::default_shader;
			shim::current_shader->use();
			update_projection();
		}
	}

	reset();
}

bool Vertex_Cache::is_started()
{
	return *_started;
}

void Vertex_Cache::maybe_resize_cache(int increase)
{
	// Some GPUs can't handle a lot of vertices
#if defined IOS || defined ANDROID || defined RASPBERRYPI
	if (*_count + increase > 0x8000) {
		gfx::Image *img = *_image;
		bool rep = *_repeat;
		end();
		start(img, rep);
	}
#endif

	if (*_total - *_count >= increase) {
		return;
	}

	increase = (increase/INCR_SIZE+1) * INCR_SIZE;

	*_vertices = (float *)realloc(*_vertices, sizeof(float)*12*(*_total+increase));

	if (*_vertices == 0) {
		throw util::MemoryError("out of memory in maybe_resize_cache");
	}

	*_total += increase;
}

void Vertex_Cache::select_cache(Uint32 cache_id)
{
	if (vertices.find(cache_id) == vertices.end()) {
		vertices[cache_id] = 0;
		count[cache_id] = 0;
		total[cache_id] = 0;
		started[cache_id] = false;
		repeat[cache_id] = false;
		image[cache_id] = 0;
	}

	_vertices = &vertices[cache_id];
	_count = &count[cache_id];
	_total = &total[cache_id];
	_started = &started[cache_id];
	_repeat = &repeat[cache_id];
	_image = &image[cache_id];

	current_cache = cache_id;
}

Uint32 Vertex_Cache::get_current_cache()
{
	return current_cache;
}

void Vertex_Cache::reset()
{
	*_image = 0;
	*_repeat = false;
	*_started = false;
	*_count = 0;
}

void Vertex_Cache::cache(SDL_Colour vertex_colours[3], util::Point<float> da, util::Point<float> db, util::Point<float> dc)
{
	maybe_resize_cache(3);
	
	glm::vec4 p1(da.x, da.y, 0.0f, 1.0f);
	glm::vec4 p2(db.x, db.y, 0.0f, 1.0f);
	glm::vec4 p3(dc.x, dc.y, 0.0f, 1.0f);

	// Set vertex x, y
	(*_vertices)[12*(*_count+0)+0] = p1.x;
	(*_vertices)[12*(*_count+0)+1] = p1.y;
	(*_vertices)[12*(*_count+1)+0] = p2.x;
	(*_vertices)[12*(*_count+1)+1] = p2.y;
	(*_vertices)[12*(*_count+2)+0] = p3.x;
	(*_vertices)[12*(*_count+2)+1] = p3.y;

	for (int i = 0; i < 3; i++) {
		(*_vertices)[12*(*_count+i)+2] = shim::z_add; // set vertex z (normally 0)
	}

	// Normal ignored

	// Texcoord ignored

	(*_vertices)[12*(*_count+0)+8+0] = (float)vertex_colours[0].r / 255.0f;
	(*_vertices)[12*(*_count+0)+8+1] = (float)vertex_colours[0].g / 255.0f;
	(*_vertices)[12*(*_count+0)+8+2] = (float)vertex_colours[0].b / 255.0f;
	(*_vertices)[12*(*_count+0)+8+3] = (float)vertex_colours[0].a / 255.0f;

	(*_vertices)[12*(*_count+1)+8+0] = (float)vertex_colours[1].r / 255.0f;
	(*_vertices)[12*(*_count+1)+8+1] = (float)vertex_colours[1].g / 255.0f;
	(*_vertices)[12*(*_count+1)+8+2] = (float)vertex_colours[1].b / 255.0f;
	(*_vertices)[12*(*_count+1)+8+3] = (float)vertex_colours[1].a / 255.0f;

	(*_vertices)[12*(*_count+2)+8+0] = (float)vertex_colours[2].r / 255.0f;
	(*_vertices)[12*(*_count+2)+8+1] = (float)vertex_colours[2].g / 255.0f;
	(*_vertices)[12*(*_count+2)+8+2] = (float)vertex_colours[2].b / 255.0f;
	(*_vertices)[12*(*_count+2)+8+3] = (float)vertex_colours[2].a / 255.0f;

	*_count += 3;
}

void Vertex_Cache::cache(SDL_Colour vertex_colours[4], util::Point<float> source_position, util::Size<float> source_size, util::Point<float> da, util::Point<float> db, util::Point<float> dc, util::Point<float> dd, int flags)
{
	maybe_resize_cache(6);
	
	glm::vec4 p1(da.x, da.y, 0.0f, 1.0f);
	glm::vec4 p2(db.x, db.y, 0.0f, 1.0f);
	glm::vec4 p3(dc.x, dc.y, 0.0f, 1.0f);
	glm::vec4 p4(dd.x, dd.y, 0.0f, 1.0f);

	// Set vertex x, y
	(*_vertices)[12*(*_count+0)+0] = p1.x;
	(*_vertices)[12*(*_count+0)+1] = p1.y;
	(*_vertices)[12*(*_count+1)+0] = p2.x;
	(*_vertices)[12*(*_count+1)+1] = p2.y;
	(*_vertices)[12*(*_count+2)+0] = p3.x;
	(*_vertices)[12*(*_count+2)+1] = p3.y;
	(*_vertices)[12*(*_count+3)+0] = p1.x;
	(*_vertices)[12*(*_count+3)+1] = p1.y;
	(*_vertices)[12*(*_count+4)+0] = p3.x;
	(*_vertices)[12*(*_count+4)+1] = p3.y;
	(*_vertices)[12*(*_count+5)+0] = p4.x;
	(*_vertices)[12*(*_count+5)+1] = p4.y;

	for (int i = 0; i < 6; i++) {
		(*_vertices)[12*(*_count+i)+2] = shim::z_add; // set vertex z (normally 0)
	}

	// Normal ignored

	if (*_image) {
		float sx = (float)source_position.x;
		float sy = (float)source_position.y;
		float tu = sx / (float)(*_image)->size.w + SMALL_TEXTURE_OFFSET;
		float tv = sy / (float)(*_image)->size.h + SMALL_TEXTURE_OFFSET;
		float tu2 = float(source_position.x + source_size.w) / (*_image)->size.w - SMALL_TEXTURE_OFFSET;
		float tv2 = float(source_position.y + source_size.h) / (*_image)->size.h - SMALL_TEXTURE_OFFSET;

		CLAMP(tu)
		CLAMP(tv)
		CLAMP(tu2)
		CLAMP(tv2)

		if (shim::opengl || (*_image)->flipped) {
			tv = 1.0f - tv;
			tv2 = 1.0f - tv2;
		}

		if (flags & Image::FLIP_H) {
			float tmp = tu;
			tu = tu2;
			tu2 = tmp;
		}
		if (flags & Image::FLIP_V) {
			float tmp = tv;
			tv = tv2;
			tv2 = tmp;
		}

		// texture coordinates
		(*_vertices)[12*(*_count+0)+6] = tu;
		(*_vertices)[12*(*_count+0)+7] = tv;
		(*_vertices)[12*(*_count+1)+6] = tu2;
		(*_vertices)[12*(*_count+1)+7] = tv;
		(*_vertices)[12*(*_count+2)+6] = tu2;
		(*_vertices)[12*(*_count+2)+7] = tv2;
		(*_vertices)[12*(*_count+3)+6] = tu;
		(*_vertices)[12*(*_count+3)+7] = tv;
		(*_vertices)[12*(*_count+4)+6] = tu2;
		(*_vertices)[12*(*_count+4)+7] = tv2;
		(*_vertices)[12*(*_count+5)+6] = tu;
		(*_vertices)[12*(*_count+5)+7] = tv2;
	}

	(*_vertices)[12*(*_count+0)+8+0] = (float)vertex_colours[0].r / 255.0f;
	(*_vertices)[12*(*_count+0)+8+1] = (float)vertex_colours[0].g / 255.0f;
	(*_vertices)[12*(*_count+0)+8+2] = (float)vertex_colours[0].b / 255.0f;
	(*_vertices)[12*(*_count+0)+8+3] = (float)vertex_colours[0].a / 255.0f;

	(*_vertices)[12*(*_count+1)+8+0] = (float)vertex_colours[1].r / 255.0f;
	(*_vertices)[12*(*_count+1)+8+1] = (float)vertex_colours[1].g / 255.0f;
	(*_vertices)[12*(*_count+1)+8+2] = (float)vertex_colours[1].b / 255.0f;
	(*_vertices)[12*(*_count+1)+8+3] = (float)vertex_colours[1].a / 255.0f;

	(*_vertices)[12*(*_count+2)+8+0] = (float)vertex_colours[2].r / 255.0f;
	(*_vertices)[12*(*_count+2)+8+1] = (float)vertex_colours[2].g / 255.0f;
	(*_vertices)[12*(*_count+2)+8+2] = (float)vertex_colours[2].b / 255.0f;
	(*_vertices)[12*(*_count+2)+8+3] = (float)vertex_colours[2].a / 255.0f;

	(*_vertices)[12*(*_count+3)+8+0] = (float)vertex_colours[0].r / 255.0f;
	(*_vertices)[12*(*_count+3)+8+1] = (float)vertex_colours[0].g / 255.0f;
	(*_vertices)[12*(*_count+3)+8+2] = (float)vertex_colours[0].b / 255.0f;
	(*_vertices)[12*(*_count+3)+8+3] = (float)vertex_colours[0].a / 255.0f;

	(*_vertices)[12*(*_count+4)+8+0] = (float)vertex_colours[2].r / 255.0f;
	(*_vertices)[12*(*_count+4)+8+1] = (float)vertex_colours[2].g / 255.0f;
	(*_vertices)[12*(*_count+4)+8+2] = (float)vertex_colours[2].b / 255.0f;
	(*_vertices)[12*(*_count+4)+8+3] = (float)vertex_colours[2].a / 255.0f;

	(*_vertices)[12*(*_count+5)+8+0] = (float)vertex_colours[3].r / 255.0f;
	(*_vertices)[12*(*_count+5)+8+1] = (float)vertex_colours[3].g / 255.0f;
	(*_vertices)[12*(*_count+5)+8+2] = (float)vertex_colours[3].b / 255.0f;
	(*_vertices)[12*(*_count+5)+8+3] = (float)vertex_colours[3].a / 255.0f;

	*_count += 6;
}

void Vertex_Cache::cache_z_range(SDL_Colour vertex_colours[4], util::Point<float> source_position, util::Size<float> source_size, util::Point<float> dest_position, float z_top, float z_bottom, util::Size<float> dest_size, int flags)
{
	maybe_resize_cache(6);

	z_top += shim::z_add;
	z_bottom += shim::z_add;

	glm::vec4 p1(dest_position.x, dest_position.y, 0.0f, 1.0f);
	glm::vec4 p2(dest_position.x + dest_size.w, dest_position.y, 0.0f, 1.0f);
	glm::vec4 p3(dest_position.x + dest_size.w, dest_position.y + dest_size.h, 0.0f, 1.0f);
	glm::vec4 p4(dest_position.x, dest_position.y + dest_size.h, 0.0f, 1.0f);

	// Set vertex x, y
	(*_vertices)[12*(*_count+0)+0] = p1.x;
	(*_vertices)[12*(*_count+0)+1] = p1.y;
	(*_vertices)[12*(*_count+0)+2] = z_top;
	(*_vertices)[12*(*_count+1)+0] = p2.x;
	(*_vertices)[12*(*_count+1)+1] = p2.y;
	(*_vertices)[12*(*_count+1)+2] = z_top;
	(*_vertices)[12*(*_count+2)+0] = p3.x;
	(*_vertices)[12*(*_count+2)+1] = p3.y;
	(*_vertices)[12*(*_count+2)+2] = z_bottom;
	(*_vertices)[12*(*_count+3)+0] = p1.x;
	(*_vertices)[12*(*_count+3)+1] = p1.y;
	(*_vertices)[12*(*_count+3)+2] = z_top;
	(*_vertices)[12*(*_count+4)+0] = p3.x;
	(*_vertices)[12*(*_count+4)+1] = p3.y;
	(*_vertices)[12*(*_count+4)+2] = z_bottom;
	(*_vertices)[12*(*_count+5)+0] = p4.x;
	(*_vertices)[12*(*_count+5)+1] = p4.y;
	(*_vertices)[12*(*_count+5)+2] = z_bottom;


	// Normal ignored

	if (*_image) {
		float tu, tv, tu2, tv2;

		if (*_repeat) {
			tu = (float)source_position.x / (*_image)->size.w + SMALL_TEXTURE_OFFSET;
			tv = (float)source_position.y / (*_image)->size.h + SMALL_TEXTURE_OFFSET;
			tu2 = (float)dest_size.w / source_size.w - SMALL_TEXTURE_OFFSET;
			tv2 = (float)dest_size.h / source_size.h - SMALL_TEXTURE_OFFSET;

			CLAMP(tu)
			CLAMP(tv)
			CLAMP(tu2)
			CLAMP(tv2)
		}
		else {
			tu = (source_position.x + SMALL_TEXTURE_OFFSET) / (*_image)->size.w;
			tv = (source_position.y + SMALL_TEXTURE_OFFSET) / (*_image)->size.h;
			tu2 = (source_position.x + source_size.w - SMALL_TEXTURE_OFFSET) / (*_image)->size.w;
			tv2 = (source_position.y + source_size.h - SMALL_TEXTURE_OFFSET) / (*_image)->size.h;

			CLAMP(tu)
			CLAMP(tv)
			CLAMP(tu2)
			CLAMP(tv2)

			if (shim::opengl || (*_image)->flipped) {
				tv = 1.0f - tv;
				tv2 = 1.0f - tv2;
			}

			if (flags & Image::FLIP_H) {
				float tmp = tu;
				tu = tu2;
				tu2 = tmp;
			}
			if (flags & Image::FLIP_V) {
				float tmp = tv;
				tv = tv2;
				tv2 = tmp;
			}
		}

		// texture coordinates
		(*_vertices)[12*(*_count+0)+6] = tu;
		(*_vertices)[12*(*_count+0)+7] = tv;
		(*_vertices)[12*(*_count+1)+6] = tu2;
		(*_vertices)[12*(*_count+1)+7] = tv;
		(*_vertices)[12*(*_count+2)+6] = tu2;
		(*_vertices)[12*(*_count+2)+7] = tv2;
		(*_vertices)[12*(*_count+3)+6] = tu;
		(*_vertices)[12*(*_count+3)+7] = tv;
		(*_vertices)[12*(*_count+4)+6] = tu2;
		(*_vertices)[12*(*_count+4)+7] = tv2;
		(*_vertices)[12*(*_count+5)+6] = tu;
		(*_vertices)[12*(*_count+5)+7] = tv2;
	}

	(*_vertices)[12*(*_count+0)+8+0] = (float)vertex_colours[0].r / 255.0f;
	(*_vertices)[12*(*_count+0)+8+1] = (float)vertex_colours[0].g / 255.0f;
	(*_vertices)[12*(*_count+0)+8+2] = (float)vertex_colours[0].b / 255.0f;
	(*_vertices)[12*(*_count+0)+8+3] = (float)vertex_colours[0].a / 255.0f;

	(*_vertices)[12*(*_count+1)+8+0] = (float)vertex_colours[1].r / 255.0f;
	(*_vertices)[12*(*_count+1)+8+1] = (float)vertex_colours[1].g / 255.0f;
	(*_vertices)[12*(*_count+1)+8+2] = (float)vertex_colours[1].b / 255.0f;
	(*_vertices)[12*(*_count+1)+8+3] = (float)vertex_colours[1].a / 255.0f;

	(*_vertices)[12*(*_count+2)+8+0] = (float)vertex_colours[2].r / 255.0f;
	(*_vertices)[12*(*_count+2)+8+1] = (float)vertex_colours[2].g / 255.0f;
	(*_vertices)[12*(*_count+2)+8+2] = (float)vertex_colours[2].b / 255.0f;
	(*_vertices)[12*(*_count+2)+8+3] = (float)vertex_colours[2].a / 255.0f;

	(*_vertices)[12*(*_count+3)+8+0] = (float)vertex_colours[0].r / 255.0f;
	(*_vertices)[12*(*_count+3)+8+1] = (float)vertex_colours[0].g / 255.0f;
	(*_vertices)[12*(*_count+3)+8+2] = (float)vertex_colours[0].b / 255.0f;
	(*_vertices)[12*(*_count+3)+8+3] = (float)vertex_colours[0].a / 255.0f;

	(*_vertices)[12*(*_count+4)+8+0] = (float)vertex_colours[2].r / 255.0f;
	(*_vertices)[12*(*_count+4)+8+1] = (float)vertex_colours[2].g / 255.0f;
	(*_vertices)[12*(*_count+4)+8+2] = (float)vertex_colours[2].b / 255.0f;
	(*_vertices)[12*(*_count+4)+8+3] = (float)vertex_colours[2].a / 255.0f;

	(*_vertices)[12*(*_count+5)+8+0] = (float)vertex_colours[3].r / 255.0f;
	(*_vertices)[12*(*_count+5)+8+1] = (float)vertex_colours[3].g / 255.0f;
	(*_vertices)[12*(*_count+5)+8+2] = (float)vertex_colours[3].b / 255.0f;
	(*_vertices)[12*(*_count+5)+8+3] = (float)vertex_colours[3].a / 255.0f;

	*_count += 6;
}

void Vertex_Cache::cache_z(SDL_Colour vertex_colours[4], util::Point<float> source_position, util::Size<float> source_size, util::Point<float> dest_position, float z, util::Size<float> dest_size, int flags)
{
	cache_z_range(vertex_colours, source_position, source_size, dest_position, z, z, dest_size, flags);
}

void Vertex_Cache::cache(SDL_Colour vertex_colours[4], util::Point<float> source_position, util::Size<float> source_size, util::Point<float> dest_position, util::Size<float> dest_size, int flags)
{
	cache_z(vertex_colours, source_position, source_size, dest_position, 0.0f, dest_size, flags);
}

void Vertex_Cache::cache_z(SDL_Colour vertex_colours[4], util::Point<float> pivot, util::Point<int> source_position, util::Size<int> source_size, util::Point<float> dest_position, float angle, float scale_x, float scale_y, float z, int flags)
{
	if (*_image == 0) {
		return;
	}

	z += shim::z_add;

	maybe_resize_cache(6);

	glm::vec4 p1(0.0f, 0.0f, z, 1.0f);
	glm::vec4 p2((float)source_size.w*scale_x, 0.0f, z, 1.0f);
	glm::vec4 p3((float)source_size.w*scale_x, (float)source_size.h*scale_y, z, 1.0f);
	glm::vec4 p4(0.0f, (float)source_size.h*scale_y, z, 1.0f);

	glm::mat4 m;
	m = glm::translate(m, glm::vec3(dest_position.x, dest_position.y, 0.0f));
	m = glm::rotate(m, angle, glm::vec3(0.0f, 0.0f, 1.0f));
	m = glm::translate(m, glm::vec3(-pivot.x*scale_x, -pivot.y*scale_y, 0.0f));

	p1 = m * p1;
	p2 = m * p2;
	p3 = m * p3;
	p4 = m * p4;

	// Set vertex x, y
	(*_vertices)[12*(*_count+0)+0] = p1.x;
	(*_vertices)[12*(*_count+0)+1] = p1.y;
	(*_vertices)[12*(*_count+0)+2] = p1.z;
	(*_vertices)[12*(*_count+1)+0] = p2.x;
	(*_vertices)[12*(*_count+1)+1] = p2.y;
	(*_vertices)[12*(*_count+1)+2] = p2.z;
	(*_vertices)[12*(*_count+2)+0] = p3.x;
	(*_vertices)[12*(*_count+2)+1] = p3.y;
	(*_vertices)[12*(*_count+2)+2] = p3.z;
	(*_vertices)[12*(*_count+3)+0] = p1.x;
	(*_vertices)[12*(*_count+3)+1] = p1.y;
	(*_vertices)[12*(*_count+3)+2] = p1.z;
	(*_vertices)[12*(*_count+4)+0] = p3.x;
	(*_vertices)[12*(*_count+4)+1] = p3.y;
	(*_vertices)[12*(*_count+4)+2] = p3.z;
	(*_vertices)[12*(*_count+5)+0] = p4.x;
	(*_vertices)[12*(*_count+5)+1] = p4.y;
	(*_vertices)[12*(*_count+5)+2] = p4.z;

	float sx = (float)source_position.x;
	float sy = (float)source_position.y;
	float tu = sx / (float)(*_image)->size.w + SMALL_TEXTURE_OFFSET;
	float tv = sy / (float)(*_image)->size.h + SMALL_TEXTURE_OFFSET;
	float tu2 = float(source_position.x + source_size.w) / (*_image)->size.w - SMALL_TEXTURE_OFFSET;
	float tv2 = float(source_position.y + source_size.h) / (*_image)->size.h - SMALL_TEXTURE_OFFSET;

	if (shim::opengl || (*_image)->flipped) {
		tv = 1.0f - tv;
		tv2 = 1.0f - tv2;
	}

	if (flags & Image::FLIP_H) {
		float tmp = tu;
		tu = tu2;
		tu2 = tmp;
	}
	if (flags & Image::FLIP_V) {
		float tmp = tv;
		tv = tv2;
		tv2 = tmp;
	}

	// texture coordinates
	(*_vertices)[12*(*_count+0)+6] = tu;
	(*_vertices)[12*(*_count+0)+7] = tv;
	(*_vertices)[12*(*_count+1)+6] = tu2;
	(*_vertices)[12*(*_count+1)+7] = tv;
	(*_vertices)[12*(*_count+2)+6] = tu2;
	(*_vertices)[12*(*_count+2)+7] = tv2;
	(*_vertices)[12*(*_count+3)+6] = tu;
	(*_vertices)[12*(*_count+3)+7] = tv;
	(*_vertices)[12*(*_count+4)+6] = tu2;
	(*_vertices)[12*(*_count+4)+7] = tv2;
	(*_vertices)[12*(*_count+5)+6] = tu;
	(*_vertices)[12*(*_count+5)+7] = tv2;

	(*_vertices)[12*(*_count+0)+8+0] = (float)vertex_colours[0].r / 255.0f;
	(*_vertices)[12*(*_count+0)+8+1] = (float)vertex_colours[0].g / 255.0f;
	(*_vertices)[12*(*_count+0)+8+2] = (float)vertex_colours[0].b / 255.0f;
	(*_vertices)[12*(*_count+0)+8+3] = (float)vertex_colours[0].a / 255.0f;

	(*_vertices)[12*(*_count+1)+8+0] = (float)vertex_colours[1].r / 255.0f;
	(*_vertices)[12*(*_count+1)+8+1] = (float)vertex_colours[1].g / 255.0f;
	(*_vertices)[12*(*_count+1)+8+2] = (float)vertex_colours[1].b / 255.0f;
	(*_vertices)[12*(*_count+1)+8+3] = (float)vertex_colours[1].a / 255.0f;

	(*_vertices)[12*(*_count+2)+8+0] = (float)vertex_colours[2].r / 255.0f;
	(*_vertices)[12*(*_count+2)+8+1] = (float)vertex_colours[2].g / 255.0f;
	(*_vertices)[12*(*_count+2)+8+2] = (float)vertex_colours[2].b / 255.0f;
	(*_vertices)[12*(*_count+2)+8+3] = (float)vertex_colours[2].a / 255.0f;

	(*_vertices)[12*(*_count+3)+8+0] = (float)vertex_colours[0].r / 255.0f;
	(*_vertices)[12*(*_count+3)+8+1] = (float)vertex_colours[0].g / 255.0f;
	(*_vertices)[12*(*_count+3)+8+2] = (float)vertex_colours[0].b / 255.0f;
	(*_vertices)[12*(*_count+3)+8+3] = (float)vertex_colours[0].a / 255.0f;

	(*_vertices)[12*(*_count+4)+8+0] = (float)vertex_colours[2].r / 255.0f;
	(*_vertices)[12*(*_count+4)+8+1] = (float)vertex_colours[2].g / 255.0f;
	(*_vertices)[12*(*_count+4)+8+2] = (float)vertex_colours[2].b / 255.0f;
	(*_vertices)[12*(*_count+4)+8+3] = (float)vertex_colours[2].a / 255.0f;

	(*_vertices)[12*(*_count+5)+8+0] = (float)vertex_colours[3].r / 255.0f;
	(*_vertices)[12*(*_count+5)+8+1] = (float)vertex_colours[3].g / 255.0f;
	(*_vertices)[12*(*_count+5)+8+2] = (float)vertex_colours[3].b / 255.0f;
	(*_vertices)[12*(*_count+5)+8+3] = (float)vertex_colours[3].a / 255.0f;

	*_count += 6;
}

void Vertex_Cache::cache(SDL_Colour vertex_colours[4], util::Point<float> pivot, util::Point<int> source_position, util::Size<int> source_size, util::Point<float> dest_position, float angle, float scale, int flags)
{
	cache_z(vertex_colours, pivot, source_position, source_size, dest_position, angle, scale, scale, 0.0f, flags);
}

void Vertex_Cache::cache_3d(SDL_Colour tint, float *in_verts, int *in_faces, float *in_normals, float *in_texcoords, float *in_colours, int num_triangles)
{
	maybe_resize_cache(num_triangles*3);

	float tint_f[4];
	tint_f[0] = tint.r / 255.0f;
	tint_f[1] = tint.g / 255.0f;
	tint_f[2] = tint.b / 255.0f;
	tint_f[3] = tint.a / 255.0f;
	
	for (int i = 0; i < num_triangles; i++) {
		int face_i = i*3;
		for (int vert = 0; vert < 3; vert++) {
			int index = in_faces[face_i+vert];
			//glm::vec4 p(in_verts[index*3+0], in_verts[index*3+1], in_verts[index*3+2], 1.0f);
			//(*_vertices)[12*(*_count+i*3+vert)+0] = p.x; // x
			//(*_vertices)[12*(*_count+i*3+vert)+1] = p.y; // y
			//(*_vertices)[12*(*_count+i*3+vert)+2] = p.z; // z
			(*_vertices)[12*(*_count+i*3+vert)+0] = in_verts[index*3+0]; // x
			(*_vertices)[12*(*_count+i*3+vert)+1] = in_verts[index*3+1]; // y
			(*_vertices)[12*(*_count+i*3+vert)+2] = in_verts[index*3+2]; // z
			if (in_normals != 0) {
				(*_vertices)[12*(*_count+i*3+vert)+3] = in_normals[index*3+0]; // normal
				(*_vertices)[12*(*_count+i*3+vert)+4] = in_normals[index*3+1]; // normal
				(*_vertices)[12*(*_count+i*3+vert)+5] = in_normals[index*3+2]; // normal
			}
			if (in_texcoords != 0) {
				(*_vertices)[12*(*_count+i*3+vert)+6] = in_texcoords[index*2+0]; // u
				(*_vertices)[12*(*_count+i*3+vert)+7] = in_texcoords[index*2+1]; // v
			}
			if (in_colours == 0) {
				(*_vertices)[12*(*_count+i*3+vert)+8] = tint_f[0]; // r
				(*_vertices)[12*(*_count+i*3+vert)+9] = tint_f[1]; // g
				(*_vertices)[12*(*_count+i*3+vert)+10] = tint_f[2]; // b
				(*_vertices)[12*(*_count+i*3+vert)+11] = tint_f[3]; // a
			}
			else {
				(*_vertices)[12*(*_count+i*3+vert)+8] = in_colours[index*4+0] * tint_f[0]; // r
				(*_vertices)[12*(*_count+i*3+vert)+9] = in_colours[index*4+1] * tint_f[1]; // g
				(*_vertices)[12*(*_count+i*3+vert)+10] = in_colours[index*4+2] * tint_f[2]; // b
				(*_vertices)[12*(*_count+i*3+vert)+11] = in_colours[index*4+3] * tint_f[3]; // a
			}
		}
	}

	*_count += num_triangles * 3;
}

void Vertex_Cache::cache_3d_immediate(float *buffer, int num_triangles)
{
	float *backup = *_vertices;
	*_vertices = buffer;
	*_count = num_triangles * 3;
	end();
	*_vertices = backup;
}

} // End namespace gfx

} // End namespace noo
