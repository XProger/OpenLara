#include "common.hlsl"

// ALPHA_TEST, UNDERWATER, CLIP_PLANE (D3D9 only), OPT_SHADOW, OPT_CAUSTICS, OPT_CONTACT

struct VS_OUTPUT {
	float4 pos       : POSITION;
	float3 coord     : TEXCOORD0;
	float4 texCoord  : TEXCOORD1;
	float4 normal    : TEXCOORD2;
	float4 diffuse   : TEXCOORD3;
	float3 ambient   : TEXCOORD4;
	float3 lightMap  : TEXCOORD5;
	float4 light     : TEXCOORD6;
	float4 lightProj : TEXCOORD7;
#ifdef _GAPI_GXM
	float clipDist   : CLP0;
#else
	float clipDist   : TEXCOORD8;
#endif
};

#ifdef VERTEX
VS_OUTPUT main(VS_INPUT In) {
	VS_OUTPUT Out;

	float4 rBasisRot = uBasis[0];
	float4 rBasisPos = uBasis[1];

	Out.texCoord = In.aTexCoord * (1.0 / 32767.0);

	Out.coord = mulBasis(rBasisRot, rBasisPos.xyz, In.aCoord.xyz);
	Out.texCoord.xy *= Out.texCoord.zw;

	Out.normal.xyz = mulQuat(rBasisRot, normalize(In.aNormal.xyz));

	float3 lv1 = (uLightPos[1].xyz - Out.coord) * uLightColor[1].w;
	float3 lv2 = (uLightPos[2].xyz - Out.coord) * uLightColor[2].w;
	float3 lv3 = (uLightPos[3].xyz - Out.coord) * uLightColor[3].w;

	float4 lum, att;
	lum.x = 1.0;
	att.x = 0.0;

	Out.ambient = min(uMaterial.yyy, In.aLight.rgb);

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

	if (OPT_SHADOW) {
		Out.light    = light;
		Out.lightMap = In.aLight.rgb * light.x;
	} else {
		Out.light.xyz = uLightColor[1].xyz * light.y + uLightColor[2].xyz * light.z + uLightColor[3].xyz * light.w;
		Out.light.w = 0.0;
		Out.light.xyz += In.aLight.rgb * light.x;
		Out.lightMap  = 0.0;
	}

	Out.diffuse = float4(In.aColor.rgb * (uMaterial.x * 1.8), 1.0);

	Out.diffuse *= uMaterial.w;
	
	Out.pos = mul(uViewProj, float4(Out.coord, rBasisPos.w));
	Out.lightProj = mul(uLightProj, float4(Out.coord, 1.0));

	Out.clipDist = uParam.w - Out.coord.y * uParam.z;

	return Out;
}

#else // PIXEL

float4 main(VS_OUTPUT In) : COLOR0 {
	float4 color = SAMPLE_2D(sDiffuse, In.texCoord.xy / In.texCoord.zw);

	#ifdef ALPHA_TEST
		clip(color.w - ALPHA_REF);
	#endif

	#ifdef CLIP_PLANE
		clip(In.clipDist);
	#endif

	color *= In.diffuse;

	float3 lightVec = (uLightPos[0].xyz - In.coord) * uLightColor[0].w;
	float3 normal   = normalize(In.normal.xyz);

	float3 light;
	if (OPT_SHADOW) {
		light = uLightColor[1].xyz * In.light.y + uLightColor[2].xyz * In.light.z + uLightColor[3].xyz * In.light.w;
		float rShadow = getShadow(lightVec, normal, In.lightProj);
		light += lerp(In.ambient, In.lightMap, rShadow);
	} else {
		light = In.light.xyz;
	}

	if (OPT_CAUSTICS) {
		light += calcCaustics(In.coord, normal);
	}

	if (OPT_CONTACT) {
		light *= getContactAO(In.coord, normal) * 0.5 + 0.5;
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
