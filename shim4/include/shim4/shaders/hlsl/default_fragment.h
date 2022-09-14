#define DEFAULT_HLSL_FRAGMENT_SHADER \
	"float4 ps_main(VS_OUTPUT Input) : COLOR0\n" \
	"{\n" \
	"	return Input.Colour;\n" \
	"}\n"
