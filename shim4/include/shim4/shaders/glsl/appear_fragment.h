#define APPEAR_GLSL_FRAGMENT_SHADER \
	"uniform sampler2D tex;\n" \
	"uniform sampler2D plasma;\n" \
	"uniform float p;\n" \
	"\n" \
	"varying vec4 colour;\n" \
	"varying vec2 texcoord;\n" \
	"\n" \
	"void main()\n" \
	"{\n" \
	"	vec4 pix = texture2D(tex, texcoord);\n" \
	"	float a = texture2D(plasma, texcoord).a;\n" \
	"	a = min(1.0, a*p+p);\n" \
	"	gl_FragColor = pix * a;\n" \
	"}\n"
