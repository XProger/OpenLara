#include "common.hlsl"

struct VS_OUTPUT {
	float4 pos		: POSITION;
	float4 texCoord	: TEXCOORD0;
	float4 hpos		: TEXCOORD1;
};

#ifdef VERTEX

VS_OUTPUT main(VS_INPUT In) {
	VS_OUTPUT Out;

	int index = int(In.aCoord.w * 2.0);
	float4 rBasisRot = uBasis[index];
	float4 rBasisPos = uBasis[index + 1];

	float3 coord = mulBasis(rBasisRot, rBasisPos.xyz, In.aCoord.xyz);
	Out.texCoord = In.aTexCoord * (1.0 / 32767.0);
	Out.pos      = mul(uViewProj, float4(coord, rBasisPos.w));
	Out.hpos     = Out.pos;
	return Out;
}

#else // PIXEL

float4 main(VS_OUTPUT In) : COLOR0 {
	#ifdef ALPHA_TEST
		clip(SAMPLE_2D_LINEAR(sDiffuse, In.texCoord.xy).a - ALPHA_REF);
	#endif

#ifdef _GAPI_GXM
    return 0.0;
#else
	#ifdef SHADOW_DEPTH
		return 0.0;
	#else // SHADOW_COLOR
		return pack(In.hpos.z / In.hpos.w);
	#endif
#endif
}

#endif
