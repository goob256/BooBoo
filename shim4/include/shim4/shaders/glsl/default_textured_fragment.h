#define DEFAULT_GLSL_TEXTURED_FRAGMENT_SHADER \
	"uniform sampler2D tex;\n" \
	"\n" \
	"varying vec4 colour;\n" \
	"varying vec2 texcoord;\n" \
	"\n" \
	"void main()\n" \
	"{\n" \
	"	gl_FragColor = texture2D(tex, texcoord) * colour;\n" \
	"}\n"
