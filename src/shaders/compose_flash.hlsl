#include "common.hlsl"

// CLIP_PLANE (D3D9 only)

struct VS_OUTPUT {
	float4 pos		: POSITION;
	float4 texCoord	: TEXCOORD0;
	float3 diffuse	: TEXCOORD1;
#ifdef _GAPI_GXM
	float clipDist  : CLP0;
#else
	float clipDist  : TEXCOORD2;
#endif
};

#ifdef VERTEX

VS_OUTPUT main(VS_INPUT In) {
	VS_OUTPUT Out;

	int index = int(In.aCoord.w * 2.0);
	float4 rBasisRot = uBasis[index];
	float4 rBasisPos = uBasis[index + 1];

	float3 coord = mulBasis(rBasisRot, rBasisPos.xyz, In.aCoord.xyz);

	Out.pos      = mul(uViewProj, float4(coord, rBasisPos.w));
	Out.diffuse  = In.aColor.rgb * (uMaterial.x * 1.8) + uMaterial.w;
	Out.texCoord = In.aTexCoord * (1.0 / 32767.0);
	Out.texCoord.xy *= Out.texCoord.zw;
	
	Out.clipDist = uParam.w - coord.y * uParam.z;

	return Out;
}

#else // PIXEL

float4 main(VS_OUTPUT In) : COLOR0 {
	float4 color = SAMPLE_2D(sDiffuse, In.texCoord.xy / In.texCoord.zw);

	#ifdef CLIP_PLANE
		clip(In.clipDist);
	#endif

	color.xyz *= In.diffuse.xyz;
	return color;
}

#endif
