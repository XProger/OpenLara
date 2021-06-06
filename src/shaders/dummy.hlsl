#include "common.hlsl"

struct VS_OUTPUT {
	float4 pos      : POSITION;
};

#ifdef VERTEX
VS_OUTPUT main(VS_INPUT In) {
	VS_OUTPUT Out;
	Out.pos = 0.0;
	return Out;
}

#else // PIXEL

float4 main(VS_OUTPUT In) : COLOR0 {
	return float4(1.0, 0.0, 1.0, 0.0);
}
#endif