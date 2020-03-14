#include "common.hlsl"

struct VS_OUTPUT {
	float4 pos		: POSITION;
	float4 texCoord	: TEXCOORD0;
	float3 diffuse	: TEXCOORD1;
};

#ifdef VERTEX

VS_OUTPUT main(VS_INPUT In) {
	VS_OUTPUT Out;

	int index = int(In.aCoord.w);
	float4 rBasisRot = uBasis[index];
	float4 rBasisPos = uBasis[index + 1];

	float3 coord = mulBasis(rBasisRot, rBasisPos.xyz, In.aCoord.xyz);

	Out.pos      = mul(uViewProj, float4(coord, 1.0));
	Out.diffuse  = In.aColor.rgb * (uMaterial.x * 1.8) + uMaterial.w;
	Out.texCoord = In.aTexCoord * INV_SHORT_HALF;
	Out.texCoord.xy *= Out.texCoord.zw;

	return Out;
}

#else // PIXEL

float4 main(VS_OUTPUT In) : COLOR0 {
	float4 color = SAMPLE_2D(sDiffuse, In.texCoord.xy / In.texCoord.zw);

	color.xyz *= In.diffuse.xyz;
	return color;
}

#endif
