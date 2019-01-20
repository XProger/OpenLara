#include "common.hlsl"

// ALPHA_TEST, UNDERWATER, OPT_CAUSTICS, CLIP_PLANE (D3D9 only)

struct VS_OUTPUT {
	float4 pos      : POSITION;
	float3 coord    : TEXCOORD0;
	float4 texCoord : TEXCOORD1;
	float4 normal   : TEXCOORD2;
	float4 diffuse  : TEXCOORD3;
	float4 light    : TEXCOORD4;
#ifdef _GAPI_GXM
	float clipDist  : CLP0;
#else
	float clipDist  : TEXCOORD5;
#endif
};

#ifdef VERTEX

VS_OUTPUT main(VS_INPUT In) {
	VS_OUTPUT Out;

	float4 rBasisRot = uBasis[0];
	float4 rBasisPos = uBasis[1];

	Out.texCoord = In.aTexCoord * (1.0 / 32767.0);

	Out.coord = mulBasis(rBasisRot, rBasisPos.xyz + In.aCoord.xyz, float3(In.aTexCoord.z, In.aTexCoord.w, 0.0));

	float3 viewVec = (uViewPos.xyz - Out.coord) * uFogParams.w;

	Out.normal.xyz = normalize(viewVec);

	float3 lv1 = (uLightPos[1].xyz - Out.coord) * uLightColor[1].w;
	float3 lv2 = (uLightPos[2].xyz - Out.coord) * uLightColor[2].w;
	float3 lv3 = (uLightPos[3].xyz - Out.coord) * uLightColor[3].w;

	float4 lum, att;
	lum.x = uMaterial.y;
	att.x = 0.0;

	float4 light;
	lum.y = dot(Out.normal.xyz, normalize(lv1)); att.y = dot(lv1, lv1);
	lum.z = dot(Out.normal.xyz, normalize(lv2)); att.z = dot(lv2, lv2);
	lum.w = dot(Out.normal.xyz, normalize(lv3)); att.w = dot(lv3, lv3);
	light = max((float4)0.0, lum) * max((float4)0.0, (float4)1.0 - att);

	#ifdef UNDERWATER
		Out.normal.w = 0.0;
	#else
		Out.normal.w = saturate(1.0 / exp(length(viewVec.xyz)));
	#endif
	
	Out.light.xyz = uLightColor[1].xyz * light.y + uLightColor[2].xyz * light.z + uLightColor[3].xyz * light.w;
	Out.light.w = 0.0;

	Out.light.xyz += In.aLight.rgb * light.x;

	Out.diffuse = float4(In.aColor.rgb * (uMaterial.x * 1.8), 1.0);

	Out.diffuse *= uMaterial.w;
	Out.diffuse *= In.aLight.a;

	Out.pos = mul(uViewProj, float4(Out.coord, rBasisPos.w));

	Out.clipDist = uParam.w - Out.coord.y * uParam.z;
	
	return Out;
}

#else // PIXEL

float4 main(VS_OUTPUT In) : COLOR0 {
	float4 color = RGBA(tex2D(sDiffuse, In.texCoord.xy));

	#ifdef ALPHA_TEST
		clip(color.w - ALPHA_REF);
	#endif

	#ifdef CLIP_PLANE
		clip(In.clipDist);
	#endif

	color *= In.diffuse;

	float3 normal   = normalize(In.normal.xyz);

	float3 light = In.light.xyz;

	if (OPT_CAUSTICS) {
		light += calcCaustics(In.coord, normal);
	}

	color.xyz *= light;

	#ifdef UNDERWATER
		applyFogUW(color.xyz, In.coord, WATER_FOG_DIST, WATER_COLOR_DIST);
	#else
		applyFog(color.xyz, In.normal.w);
	#endif

	return color;
}

#endif
