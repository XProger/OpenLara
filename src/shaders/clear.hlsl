#include "common.hlsl"

#ifdef VERTEX

float4 main(VS_INPUT In) : POSITION {
    return In.aCoord;
}

#else // PIXEL

float4 main() : COLOR {
	return uMaterial;
}

#endif