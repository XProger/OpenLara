#include "common.hlsl"

// ALPHA_TEST, UNDERWATER, OPT_SHADOW, OPT_CAUSTICS, OPT_CONTACT

#ifdef _GAPI_D3D11_9_3
	#define MONOCHROME_AMBIENT
#endif

struct VS_OUTPUT {
	float4 pos       : POSITION;
	float3 coord     : TEXCOORD0;
	float4 texCoord  : TEXCOORD1;
	float4 normal    : TEXCOORD2;
	float4 diffuse   : TEXCOORD3;
	float4 light     : TEXCOORD4;

#ifdef OPT_SHADOW
	float4 lightProj : TEXCOORD5;
	float4 lightMap  : TEXCOORD6;

	#ifndef MONOCHROME_AMBIENT
		float3 ambient   : TEXCOORD7;
	#endif

#endif
};

#ifdef VERTEX
VS_OUTPUT main(VS_INPUT In) {
	VS_OUTPUT Out;

	float4 rBasisRot = uBasis[0];
	float4 rBasisPos = uBasis[1];

	Out.texCoord = In.aTexCoord * INV_SHORT_HALF;

	Out.coord = mulBasis(rBasisRot, rBasisPos.xyz, In.aCoord.xyz);
	Out.texCoord.xy *= Out.texCoord.zw;

	Out.normal.xyz = normalize(In.aNormal.xyz);

	float3 lv1 = (uLightPos[1].xyz - Out.coord) * uLightColor[1].w;
	float3 lv2 = (uLightPos[2].xyz - Out.coord) * uLightColor[2].w;
	float3 lv3 = (uLightPos[3].xyz - Out.coord) * uLightColor[3].w;

	float4 lum, att;
	lum.x = 1.0;
	att.x = 0.0;

	float4 light;
	lum.y = dot(Out.normal.xyz, normalize(lv1)); att.y = dot(lv1, lv1);
	lum.z = dot(Out.normal.xyz, normalize(lv2)); att.z = dot(lv2, lv2);
	lum.w = dot(Out.normal.xyz, normalize(lv3)); att.w = dot(lv3, lv3);
	light = max((float4)0.0, lum) * max((float4)0.0, (float4)1.0 - att);

	#ifdef UNDERWATER
		light.x *= calcCausticsV(Out.coord);
		Out.normal.w = 0.0;
	#else
		float3 viewVec = (uViewPos.xyz - Out.coord) * uFogParams.w;
		Out.normal.w = saturate(1.0 / exp(length(viewVec.xyz)));
	#endif

	#ifdef OPT_SHADOW
		Out.light    = light;
		Out.lightMap.rgb = In.aLight.rgb * light.x;
		Out.lightProj = calcLightProj(Out.coord);

		#ifdef MONOCHROME_AMBIENT
			Out.lightMap.a = min(uMaterial.y, In.aLight.r);
		#else
			Out.lightMap.a = 0.0;
			Out.ambient = min(uMaterial.yyy, In.aLight.rgb);
		#endif
	#else
		Out.light.xyz = uLightColor[1].xyz * light.y + uLightColor[2].xyz * light.z + uLightColor[3].xyz * light.w;
		Out.light.w = 0.0;
		Out.light.xyz += In.aLight.rgb * light.x;
	#endif

	Out.diffuse = float4(In.aColor.rgb * (uMaterial.x * 1.8), 1.0);

	Out.diffuse *= uMaterial.w;

	Out.pos = mul(uViewProj, float4(Out.coord, 1.0));

	return Out;
}

#else // PIXEL

float4 main(VS_OUTPUT In) : COLOR0 {
	float4 color = SAMPLE_2D(sDiffuse, In.texCoord.xy / In.texCoord.zw);

	#ifdef ALPHA_TEST
		clip(color.w - ALPHA_REF);
	#endif

	color *= In.diffuse;

	float3 light;
	#ifdef OPT_SHADOW
		float3 lightVec = (uLightPos[0].xyz - In.coord) * uLightColor[0].w;
		light = uLightColor[1].xyz * In.light.y + uLightColor[2].xyz * In.light.z + uLightColor[3].xyz * In.light.w;
		float rShadow = getShadow(lightVec, In.lightProj);
		#ifdef MONOCHROME_AMBIENT
			light += lerp(In.lightMap.aaa, In.lightMap.rgb, rShadow);
		#else
			light += lerp(In.ambient, In.lightMap.rgb, rShadow);
		#endif
	#else
		light = In.light.xyz;
	#endif

	#if defined(OPT_CAUSTICS) || defined(OPT_CONTACT)
		float3 normal = normalize(In.normal.xyz);
	#endif

	#ifdef OPT_CAUSTICS
		light += calcCaustics(In.coord, normal);
	#endif

	#ifdef OPT_CONTACT
		light *= getContactAO(In.coord, normal) * 0.5 + 0.5;
	#endif

	color.xyz *= light;

	#ifdef UNDERWATER
		applyFogUW(color.xyz, In.coord, WATER_FOG_DIST, WATER_COLOR_DIST);
	#else
		applyFog(color.xyz, In.normal.w);
	#endif

	return color;
}

#endif
