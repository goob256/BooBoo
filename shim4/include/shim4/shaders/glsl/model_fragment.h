#define MODEL_GLSL_FRAGMENT_SHADER \
	"uniform sampler2D tex;\n" \
	"uniform vec4 tint;\n" \
	"\n" \
	"varying vec4 colour;\n" \
	"varying vec2 texcoord;\n" \
	"\n" \
	"void main()\n" \
	"{\n" \
	"	vec4 result;\n" \
	"	vec4 pix = texture2D(tex, texcoord);\n" \
	"	result = vec4(pix.rgb + colour.rgb * (1.0 - pix.a), 1.0);\n" \
	"	gl_FragColor = result * tint;\n" \
	"}\n"
