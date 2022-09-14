#define DEFAULT_HLSL_TEXTURED_FRAGMENT_SHADER \
	"texture tex;\n" \
	"sampler2D s = sampler_state {\n" \
	"	texture = <tex>;\n" \
	"};\n" \
	"\n" \
	"float4 ps_main(VS_OUTPUT Input) : COLOR0\n" \
	"{\n" \
	"	return Input.Colour * tex2D(s, Input.TexCoord);\n" \
	"}\n"
