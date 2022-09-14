#ifndef NOO_SHADER_H
#define NOO_SHADER_H

#include "shim4/main.h"

namespace noo {

namespace gfx {

class Image;

class Shader
{
public:
	enum Precision {
		UNSPECIFIED = 0,
		LOW,
		MEDIUM,
		HIGH
	};

	static void static_start();
	static void release_all(bool force);
	static void reload_all(bool force);
	static std::vector< std::pair<std::string, Image *> > &get_bound_images();
	static void rebind_opengl_texture0();
	static void audit();

#ifdef _WIN32
	struct D3D_Var_Info {
		int num;
		char type;
	};

	struct D3D_Shader {
		std::map<std::string, D3D_Var_Info> vars;
		std::map<std::string, int> sizes;
		Uint8 *data;
		int sz;
	};
	struct D3D_Vertex_Shader : public D3D_Shader {
		IDirect3DVertexShader9 *shader;
	};
	struct D3D_Fragment_Shader : public D3D_Shader {
		IDirect3DPixelShader9 *shader;
	};
	static D3D_Vertex_Shader *load_d3d_vertex_shader(std::string filename);
	static D3D_Fragment_Shader *load_d3d_fragment_shader(std::string filename);
#endif
	struct OpenGL_Shader {
		GLenum type;
		Precision precision;
		GLuint shader;
		std::string source;
	};
	static OpenGL_Shader *load_opengl_vertex_shader(std::string source, Precision precision = LOW);
	static OpenGL_Shader *load_opengl_fragment_shader(std::string source, Precision precision = LOW);
	
	SHIM4_EXPORT Shader(OpenGL_Shader *vertex_shader, OpenGL_Shader *fragment_shader, bool is_master_vertex = true, bool is_master_fragment = true);

#ifdef _WIN32
	SHIM4_EXPORT Shader(D3D_Vertex_Shader *vertex_shader, D3D_Fragment_Shader *fragment_shader, bool is_master_vertex = true, bool is_master_fragment = true);
#ifdef USE_D3DX
	SHIM4_EXPORT Shader(std::string vertex_source, std::string fragment_source);
	SHIM4_EXPORT bool is_d3dx();
	SHIM4_EXPORT LPD3DXEFFECT get_d3d_effect();
#endif
#endif

	SHIM4_EXPORT ~Shader();

	SHIM4_EXPORT void use();

	SHIM4_EXPORT void set_texture(std::string name, Image *image, int unit = 0);
	SHIM4_EXPORT void set_matrix(std::string name, glm::mat4 &matrix);
	SHIM4_EXPORT void set_float(std::string name, float value);
	SHIM4_EXPORT bool set_float_vector(std::string name, int num_components, const float *vector, int num_elements);
	SHIM4_EXPORT void set_bool(std::string name, bool value);
	SHIM4_EXPORT void set_int(std::string name, int value);
	SHIM4_EXPORT bool set_colour(std::string name, SDL_Colour colour);

	SHIM4_EXPORT GLuint get_opengl_shader();
	SHIM4_EXPORT void set_opengl_attributes(float *pos, float *normal, float *texcoord, float *colour);

private:
#ifdef _WIN32
	static void load_d3d_shader(std::string filename, D3D_Shader *shader);
#endif
	static std::string add_opengl_header(bool is_vertex, Precision precision, std::string source);
	static GLuint compile_opengl_vertex_shader(std::string source);
	static GLuint compile_opengl_fragment_shader(std::string source);
	static GLuint compile_opengl_shader(GLenum type, std::string source);

	// Formerly Internal --
	SHIM4_EXPORT void release(bool force);
	SHIM4_EXPORT void reload(bool force);
	SHIM4_EXPORT void unbind(Shader *s);
	SHIM4_EXPORT GLint get_uniform_location(std::string name);

	bool opengl;
	bool is_master_vertex;
	bool is_master_fragment;

	// OpenGL
	OpenGL_Shader *opengl_vertex;
	OpenGL_Shader *opengl_fragment;
	GLuint opengl_shader;
	GLint pos_attrib, normal_attrib, texcoord_attrib, colour_attrib;
	float *pos_ptr, *normal_ptr, *texcoord_ptr, *colour_ptr;
	std::map<std::string, GLint> uniform_locations;

	// D3D
#ifdef _WIN32
#ifdef USE_D3DX
	bool _is_d3dx;
	LPD3DXEFFECT d3d_effect;
	D3DXHANDLE d3d_technique;
	std::string vertex_source;
	std::string fragment_source;
#endif
	D3D_Vertex_Shader *d3d_vertex;
	D3D_Fragment_Shader *d3d_fragment;
#endif
	//--

	static std::vector<Shader *> loaded_shaders;
	static std::vector< std::pair<std::string, Image *> > bound_images;
	static Shader *last_shader;
	static GLuint opengl_texture0;
	static int d3d_vertex_shader_count;
	static int d3d_fragment_shader_count;
};

} // End namespace gfx

} // End namespace noo

#endif // NOO_SHADER_H

