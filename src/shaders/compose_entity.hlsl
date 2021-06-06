#include "common.hlsl"

// ALPHA_TEST, UNDERWATER, OPT_SHADOW, OPT_CAUSTICS, OPT_AMBIENT

struct VS_OUTPUT {
	float4 pos       : POSITION;
	float3 coord     : TEXCOORD0;
	float4 texCoord  : TEXCOORD1;
	float4 viewVec   : TEXCOORD2;
	float4 normal    : TEXCOORD3;
	float4 diffuse   : TEXCOORD4;
	float4 light     : TEXCOORD5;

#ifdef OPT_SHADOW
	float4 lightProj : TEXCOORD6;
#endif

#ifdef OPT_AMBIENT
	float3 ambient   : TEXCOORD7;
#endif
};

#ifdef VERTEX

VS_OUTPUT main(VS_INPUT In) {
	VS_OUTPUT Out;

	int index = int(In.aCoord.w);
	float4 rBasisRot = uBasis[index];
	float4 rBasisPos = uBasis[index + 1];

	Out.texCoord = In.aTexCoord * INV_SHORT_HALF;

	Out.coord = mulBasis(rBasisRot, rBasisPos.xyz, In.aCoord.xyz);
	Out.texCoord.xy *= Out.texCoord.zw;

	Out.viewVec = float4((uViewPos.xyz - Out.coord) * uFogParams.w, uParam.w - Out.coord.y * uParam.z);

	Out.normal.xyz = mulQuat(rBasisRot, normalize(In.aNormal.xyz));

	float3 lv0 = (uLightPos[0].xyz - Out.coord) * uLightColor[0].w;
	float3 lv1 = (uLightPos[1].xyz - Out.coord) * uLightColor[1].w;
	float3 lv2 = (uLightPos[2].xyz - Out.coord) * uLightColor[2].w;
	float3 lv3 = (uLightPos[3].xyz - Out.coord) * uLightColor[3].w;

	float3 ambient;
	#ifdef OPT_AMBIENT
		ambient = calcAmbient(Out.normal.xyz);
	#else
		ambient = uMaterial.yyy;
	#endif

	float4 lum, att, light;
	lum.x = dot(Out.normal.xyz, normalize(lv0)); att.x = dot(lv0, lv0);	
	lum.y = dot(Out.normal.xyz, normalize(lv1)); att.y = dot(lv1, lv1);
	lum.z = dot(Out.normal.xyz, normalize(lv2)); att.z = dot(lv2, lv2);
	lum.w = dot(Out.normal.xyz, normalize(lv3)); att.w = dot(lv3, lv3);
	light = max((float4)0.0, lum) * max((float4)0.0, (float4)1.0 - att);

	#ifdef UNDERWATER
		light.x *= calcCausticsV(Out.coord);
		Out.normal.w = 0.0;
	#else
		Out.normal.w = saturate(1.0 / exp(length(Out.viewVec.xyz)));
	#endif

	#ifdef OPT_SHADOW
		Out.light = light;
		Out.lightProj = calcLightProj(Out.coord);
		#ifdef OPT_AMBIENT
			Out.ambient = ambient;
		#endif
	#else
		Out.light.xyz = uLightColor[1].xyz * light.y + uLightColor[2].xyz * light.z + uLightColor[3].xyz * light.w;
		Out.light.w = 0.0;

		Out.light.xyz += ambient + uLightColor[0].xyz * light.x;
	#endif

	Out.diffuse = float4(In.aColor.xyz * (uMaterial.x * 1.8), 1.0);

	Out.diffuse *= uMaterial.w;

	Out.pos = mul(uViewProj, float4(Out.coord, rBasisPos.w));

	return Out;
}

#else // PIXEL

float4 main(VS_OUTPUT In) : COLOR0 {
	float4 color = SAMPLE_2D(sDiffuse, In.texCoord.xy / In.texCoord.zw);

	#ifdef ALPHA_TEST
		clip(color.w - ALPHA_REF);
	#endif

	color *= In.diffuse;

	float3 lightVec = (uLightPos[0].xyz - In.coord) * uLightColor[0].w;
	float3 normal   = normalize(In.normal.xyz);

	float rSpecular = 0.0;

	float3 light;
	#ifdef OPT_SHADOW
		light = uLightColor[1].xyz * In.light.y + uLightColor[2].xyz * In.light.z + uLightColor[3].xyz * In.light.w;
		float rShadow = getShadow(lightVec, In.lightProj);
		rSpecular = (uMaterial.z + 0.03) * rShadow;

		float3 ambient;
		#ifdef OPT_AMBIENT
			ambient = In.ambient;
		#else
			ambient = uMaterial.yyy;
		#endif

		light += ambient + uLightColor[0].xyz * (In.light.x * rShadow);
	#else
		light = In.light.xyz;
	#endif

	#ifdef OPT_CAUSTICS
		light += calcCaustics(In.coord, normal);
	#endif

	color.xyz *= light;

	float specular = calcSpecular(normal, In.viewVec.xyz, lightVec, rSpecular);
	
	#ifdef UNDERWATER
		float uwSign = step(uParam.y, In.coord.y);
		specular *= (1.0 - uwSign);
		color.xyz += specular;

		applyFogUW(color.xyz, In.coord, WATER_FOG_DIST * uwSign, WATER_COLOR_DIST * uwSign);		
	#else
		color.xyz += specular;
		applyFog(color.xyz, In.normal.w);
	#endif

	return color;
}

#endif
