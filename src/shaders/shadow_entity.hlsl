#include "common.hlsl"

struct VS_OUTPUT {
	float4 pos      : POSITION;
	float4 texCoord : TEXCOORD0;
#ifndef SHADOW_DEPTH
	float4 hpos     : TEXCOORD1;
#endif
};

#ifdef VERTEX

VS_OUTPUT main(VS_INPUT In) {
	VS_OUTPUT Out;

	int index = int(In.aCoord.w);
	float4 rBasisRot = uBasis[index];
	float4 rBasisPos = uBasis[index + 1];

	float3 cpos  = In.aCoord.xyz - normalize(In.aNormal.xyz) * SHADOW_NORMAL_BIAS;
	float3 coord = mulBasis(rBasisRot, rBasisPos.xyz, cpos);

	Out.texCoord = In.aTexCoord * INV_SHORT_HALF;
	Out.pos      = mul(uViewProj, float4(coord, rBasisPos.w));
#ifndef SHADOW_DEPTH
	Out.hpos     = Out.pos;
#endif
	return Out;
}

#else // PIXEL

float4 main(VS_OUTPUT In) : COLOR0 {
#ifdef ALPHA_TEST
	clip(SAMPLE_2D_LINEAR(sDiffuse, In.texCoord.xy).a - ALPHA_REF);
#endif

#ifdef SHADOW_DEPTH
	return 0.0;
#else
	return pack(In.hpos.z / In.hpos.w);
#endif
}

#endif
