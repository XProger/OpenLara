#include "common.hlsl"

// CLIP_PLANE (D3D9 only)

struct VS_OUTPUT {
	float4 pos		: POSITION;
	float4 viewVec	: TEXCOORD0;
	float4 normal	: TEXCOORD1;
#ifdef _GAPI_GXM
	float  clipDist : CLP0;
#endif
};

#ifdef VERTEX

VS_OUTPUT main(VS_INPUT In) {
	VS_OUTPUT Out;

	int index = int(In.aCoord.w * 2.0);
	float4 rBasisRot = uBasis[index];
	float4 rBasisPos = uBasis[index + 1];

	float3 coord = mulBasis(rBasisRot, rBasisPos.xyz, In.aCoord.xyz);

	Out.viewVec = float4((uViewPos.xyz - coord) * uFogParams.w, uParam.w - coord.y * uParam.z);

	Out.normal.xyz = mulQuat(rBasisRot, normalize(In.aNormal.xyz));
	Out.normal.w = saturate(1.0 / exp(length(Out.viewVec.xyz)));
	
	Out.pos = mul(uViewProj, float4(coord, rBasisPos.w));

#ifdef _GAPI_GXM
	Out.clipDist = Out.viewVec.w;
#endif
	
	return Out;
}

#else // PIXEL

float4 main(VS_OUTPUT In) : COLOR0 {
	float3 rv = reflect(-In.viewVec.xyz, In.normal.xyz);
	float4 color = texCUBE(sEnvironment, normalize(rv));

	#ifdef CLIP_PLANE
		clip(In.viewVec.w);
	#endif

	color *= uMaterial;
    color.xyz = saturate(color.xyz);
    
    applyFog(color.xyz, In.normal.w);

	return color;
}

#endif
