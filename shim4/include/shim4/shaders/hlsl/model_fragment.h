#define MODEL_HLSL_FRAGMENT_SHADER \
	"texture tex;\n" \
	"float4 tint;\n" \
	"sampler2D s = sampler_state {\n" \
	"	texture = <tex>;\n" \
	"};\n" \
	"\n" \
	"float4 ps_main(VS_OUTPUT Input) : COLOR0\n" \
	"{\n" \
	"	float4 result;\n" \
	"	float4 pix = tex2D(s, Input.TexCoord);\n" \
	"	result = float4(pix.rgb + Input.Colour.rgb * (1.0f - pix.a), 1.0f);\n" \
	"	return result * tint;\n" \
	"}\n"
