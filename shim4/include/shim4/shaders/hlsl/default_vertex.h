#define DEFAULT_HLSL_VERTEX_SHADER \
	"struct VS_INPUT\n" \
	"{\n" \
	"	float3 Position : POSITION0;\n" \
	"       float3 Normal   : NORMAL0;\n" \
	"	float2 TexCoord : TEXCOORD0;\n" \
	"	float4 Colour	: TEXCOORD1;\n" \
	"};\n" \
	"struct VS_OUTPUT\n" \
	"{\n" \
	"	float4 Position : POSITION0;\n" \
	"	float2 TexCoord : TEXCOORD0;\n" \
	"	float4 Colour	: COLOR0;\n" \
	"};\n" \
	"\n" \
	"float4x4 proj;\n" \
	"float4x4 modelview;\n" \
	"\n" \
	"VS_OUTPUT vs_main(VS_INPUT Input)\n" \
	"{\n" \
	"	VS_OUTPUT Output;\n" \
	"	Output.Colour = Input.Colour;\n" \
	"	Output.TexCoord = Input.TexCoord;\n" \
	"	Output.Position = mul(float4(Input.Position, 1.0f), mul(modelview, proj));\n" \
	"	return Output;\n" \
	"}\n"
