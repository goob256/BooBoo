#define DEFAULT_GLSL_VERTEX_SHADER \
	"uniform mat4 modelview;\n" \
	"uniform mat4 proj;\n" \
	"\n" \
	"attribute vec3 in_position;\n" \
	"attribute vec2 in_texcoord;\n" \
	"attribute vec4 in_colour;\n" \
	"\n" \
	"varying vec2 texcoord;\n" \
	"varying vec4 colour;\n" \
	"\n" \
	"void main() {\n" \
	"	colour = in_colour;\n" \
	"	texcoord = in_texcoord;\n" \
	"	gl_Position = proj * modelview * vec4(in_position, 1.0);\n" \
	"}\n"
