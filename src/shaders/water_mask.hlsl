#include "common.hlsl"

#ifdef VERTEX

float4 main(VS_INPUT In) : POSITION {
	float3 coord = In.aCoord.xyz * INV_SHORT_HALF;
	coord = float3(coord.x, 0.0, coord.y) * uPosScale[1].xyz + uPosScale[0].xyz;
	return mul(uViewProj, float4(coord, 1.0));
}

#else // PIXEL

float4 main() : COLOR0 {
	return 0.0;
}

#endif
