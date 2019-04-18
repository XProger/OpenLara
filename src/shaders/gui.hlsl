#include "common.hlsl"

struct VS_OUTPUT {
	float4 pos      : POSITION;
	float2 texCoord : TEXCOORD0;
	float4 diffuse  : COLOR0;
};

#ifdef VERTEX
VS_OUTPUT main(VS_INPUT In) {
	VS_OUTPUT Out;
	Out.pos       = mul(uViewProj, float4(In.aCoord.xy, 0.0, 1.0));
	Out.texCoord  = In.aTexCoord.xy * (1.0 / 32767.0);
	Out.diffuse   = In.aLight * uMaterial;
	return Out;
}

#else // PIXEL

float4 main(VS_OUTPUT In) : COLOR0 {
	return In.diffuse * SAMPLE_2D_LINEAR(sDiffuse, In.texCoord);
}
#endif