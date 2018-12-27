#include "common.hlsl"

struct VS_OUTPUT {
	float4 pos		: POSITION;
	float3 coord	: TEXCOORD0;
	float4 texCoord	: TEXCOORD1;
	float4 viewVec	: TEXCOORD2;
	float4 normal	: TEXCOORD3;
	float4 diffuse	: TEXCOORD4;
	float3 ambient	: TEXCOORD5;
	float3 lightMap	: TEXCOORD6;
	float4 light	: TEXCOORD7;
	float4 lightProj : TEXCOORD8;
#ifdef _GAPI_GXM
	//float  clipDist  : CLP0;
#endif
};

#ifdef VERTEX
VS_OUTPUT main(VS_INPUT In) {
	VS_OUTPUT Out;
	Out.ambient   = 0.0;
	Out.lightMap  = 0.0;
	Out.light     = 0.0;

	int index = int(In.aCoord.w * 2.0);
	float4 rBasisRot = uBasis[index];
	float4 rBasisPos = uBasis[index + 1];

	Out.texCoord = In.aTexCoord * (1.0 / 32767.0);

	if (TYPE_SPRITE) {
		Out.coord = mulBasis(rBasisRot, rBasisPos.xyz + In.aCoord.xyz, float3(In.aTexCoord.z, In.aTexCoord.w, 0.0));
	} else {
		Out.coord = mulBasis(rBasisRot, rBasisPos.xyz, In.aCoord.xyz);
		Out.texCoord.xy *= Out.texCoord.zw;
	}

	Out.viewVec = float4((uViewPos.xyz - Out.coord) * uFogParams.w, Out.coord.y * uParam.z);

	if (TYPE_SPRITE) {
		Out.normal.xyz = normalize(Out.viewVec.xyz);
	} else {
		Out.normal.xyz = mulQuat(rBasisRot, normalize(In.aNormal.xyz));
	}

	float fog;

	if (!TYPE_FLASH) {
		float3 lv0 = (uLightPos[0].xyz - Out.coord) * uLightColor[0].w;
		float3 lv1 = (uLightPos[1].xyz - Out.coord) * uLightColor[1].w;
		float3 lv2 = (uLightPos[2].xyz - Out.coord) * uLightColor[2].w;
		float3 lv3 = (uLightPos[3].xyz - Out.coord) * uLightColor[3].w;

		float4 lum, att;
		if (TYPE_ENTITY) {
			lum.x = dot(Out.normal.xyz, normalize(lv0));
			att.x = dot(lv0, lv0);
			if (OPT_AMBIENT) {
				Out.ambient = calcAmbient(Out.normal.xyz);
			} else {
				Out.ambient = min(uMaterial.yyy, RGB(In.aLight));
			}
		} else {
			if (TYPE_SPRITE) {
				lum.x = uMaterial.y;
			} else {
				lum.x = 1.0;
			}
			att.x = 0.0;

			Out.ambient = min(uMaterial.yyy, RGB(In.aLight));
		}

		float4 light;
		lum.y = dot(Out.normal.xyz, normalize(lv1)); att.y = dot(lv1, lv1);
		lum.z = dot(Out.normal.xyz, normalize(lv2)); att.z = dot(lv2, lv2);
		lum.w = dot(Out.normal.xyz, normalize(lv3)); att.w = dot(lv3, lv3);
		light = max((float4)0.0, lum) * max((float4)0.0, (float4)1.0 - att);

		if (UNDERWATER) {
			light.x *= abs(sin(dot(Out.coord.xyz, 1.0 / 512.0) + uParam.x)) * 1.5 + 0.5;

			float d;
			if (uViewPos.y < uParam.y) {
				d = abs((Out.coord.y - uParam.y) / normalize(uViewPos.xyz - Out.coord.xyz).y);
			} else {
				d = length(uViewPos.xyz - Out.coord.xyz);
			}
			fog = d * WATER_FOG_DIST;
			fog *= step(uParam.y, Out.coord.y);
			Out.normal.w = fog;
		} else {
			fog = length(Out.viewVec.xyz);
			Out.normal.w = saturate(1.0 / exp(fog));
		}
		Out.normal.w = saturate(1.0 / exp(fog));

		if (OPT_SHADOW) {
			Out.light    = light;
			Out.lightMap = RGB(In.aLight) * light.x;
		} else {
			Out.light.xyz = uLightColor[1].xyz * light.y + uLightColor[2].xyz * light.z + uLightColor[3].xyz * light.w;
			Out.light.w = 0.0;

			if (TYPE_ENTITY) {
				Out.light.xyz += Out.ambient + uLightColor[0].xyz * light.x;
			} else {
				Out.light.xyz += RGB(In.aLight) * light.x;
			}
		}
	} else
		Out.normal.w = 1.0;

	if (TYPE_MIRROR) {
		Out.diffuse = uMaterial;
	} else {
		Out.diffuse = float4(RGB(In.aColor) * (uMaterial.x * 1.8), 1.0);

		if (TYPE_FLASH) {
			Out.diffuse.xyz += uMaterial.w;
		} else {
			Out.diffuse *= uMaterial.w;
		}
		
		if (TYPE_SPRITE) {
			Out.diffuse *= RGBA(In.aLight).a;
		}
	}

	Out.pos = mul(uViewProj, float4(Out.coord, rBasisPos.w));
Out.lightProj = mul(uLightProj, float4(Out.coord, 1.0));
#ifdef _GAPI_GXM
	//Out.clipDist = uParam.w - Out.viewVec.w;
#endif
	
	return Out;
}

#else // PIXEL

float SHADOW(float2 p) {
	#ifdef SHADOW_DEPTH
		return tex2D(sShadow, p).x;
	#else
		return unpack(tex2D(sShadow, p));
	#endif
}

float getShadow(float3 lightVec, float3 normal, float4 lightProj) {
	float sMin = min(lightProj.x, lightProj.y);
	float sMax = max(lightProj.x, lightProj.y);
	
	//float vis = lightProj.w;
	//if (TYPE_ROOM) {
	//	vis = min(vis, dot(normal, lightVec));
	//}
	//sMin = min(vis, sMin);
	
	float factor = step(0.0, lightProj.w); //((sMin > 0.0f) && (sMax < lightProj.w));

#ifdef _GAPI_GXM
	lightProj.z += SHADOW_CONST_BIAS * SHADOW_TEXEL.x * lightProj.w;
	float rShadow = f1tex2Dproj(sShadow, lightProj);
#else
	float3 p = lightProj.xyz / lightProj.w;
	
	p.z -= SHADOW_CONST_BIAS * SHADOW_TEXEL.x;

	p.z = saturate(p.z);

	float4 samples = float4(
			SHADOW(p.xy                  ),
			SHADOW(p.xy + SHADOW_TEXEL.xz),
			SHADOW(p.xy + SHADOW_TEXEL.zy),
			SHADOW(p.xy + SHADOW_TEXEL.xy)
		);
	samples = step(p.zzzz, samples);

	float2 f = frac(p.xy / SHADOW_TEXEL.xy);
	samples.xy = lerp(samples.xz, samples.yw, f.xx);
	float rShadow = lerp(samples.x, samples.y, f.y);
#endif

	rShadow = lerp(1.0, rShadow, factor);
	
	float fade = saturate(dot(lightVec, lightVec));
	return rShadow + (1.0 - rShadow) * fade;
}

float getShadow(float3 coord, float3 lightVec, float3 normal, float4 lightProj) {
	float factor = clamp(1.0 - dot(normalize(lightVec), normal), 0.0, 1.0);
	factor *= SHADOW_NORMAL_BIAS;
	return getShadow(lightVec, normal, lightProj /*mul(uLightProj, float4(coord + normal * factor, 1.0)) */ );
}

float getContactAO(float3 p, float3 n) {
	float res = 1.0;
	for (int i = 0; i < MAX_CONTACTS; i++) {
		float3 v = uContacts[i].xyz - p;
		float  a = uContacts[i].w;
		float  o = a * saturate(dot(n, v)) / dot(v, v);
		res *= saturate(1.0 - o);
	}
	return res;
}

float calcCaustics(float3 coord, float3 n) {
	float2 cc = saturate((coord.xz - uRoomSize.xy) / uRoomSize.zw);
	return tex2D(sReflect, float2(cc.x, 1.0 - cc.y)).x * max(0.0, -n.y);
}

float calcSpecular(float3 normal, float3 viewVec, float3 lightVec, float intensity) {
	float3 vv = normalize(viewVec);
	float3 rv = reflect(-vv, normal);
	float3 lv = normalize(lightVec);
	return pow(max(0.0, dot(rv, lv)), 8.0) * intensity;
}

float4 main(VS_OUTPUT In) : COLOR0 {
	float2 uv = In.texCoord.xy;

#ifndef _GAPI_GXM
	if (CLIP_PLANE) {
		if (In.viewVec.w > uParam.w) {
			discard;
		}
	}
#endif

	if (!TYPE_SPRITE) {
		uv /= In.texCoord.zw;
	}

	float4 color;

	if (TYPE_MIRROR) {
		float3 rv = reflect(-normalize(In.viewVec.xyz), normalize(In.normal.xyz));
		color = texCUBE(sEnvironment, normalize(rv)).bgra;
	} else {
		color = tex2D(sDiffuse, uv).bgra;

		if (ALPHA_TEST) {
			if (color.w <= 0.5)
				discard;
		}
	}

	color *= In.diffuse;

	if (TYPE_FLASH) {
		return color;
	}

	if (TYPE_MIRROR) {
		return color;
	}

	float3 lightVec = (uLightPos[0].xyz - In.coord) * uLightColor[0].w;
	float3 normal   = normalize(In.normal.xyz);

	float rSpecular = 0.0;

	float3 light;
	if (OPT_SHADOW) {
		light = uLightColor[1].xyz * In.light.y + uLightColor[2].xyz * In.light.z + uLightColor[3].xyz * In.light.w;

		if (TYPE_ENTITY) {
			float rShadow = getShadow(In.coord, lightVec, normal, In.lightProj);
			rSpecular = (uMaterial.z + 0.03) * rShadow;
			light += In.ambient + uLightColor[0].xyz * (In.light.x * rShadow);
		} else if (TYPE_ROOM) {
			float rShadow = getShadow(In.coord, lightVec, normal, In.lightProj);
			light += lerp(In.ambient, In.lightMap, rShadow);
		} else if (TYPE_SPRITE) {
			light += In.lightMap;
		}
	} else {
		light = In.light.xyz;
	}

	float uwSign = 1.0;
	if (UNDERWATER) {
		if (TYPE_ENTITY) {
			uwSign = step(uParam.y, In.coord.y);
		}

		if (OPT_CAUSTICS) {
			light += calcCaustics(In.coord, normal);
		}
	}

	if (OPT_CONTACT) {
		light *= getContactAO(In.coord, normal) * 0.5 + 0.5;
	}

	color.xyz *= light;

	if (TYPE_ENTITY) {
		float specular = calcSpecular(normal, In.viewVec.xyz, lightVec, rSpecular);
		if (UNDERWATER) {
			specular *= (1.0 - uwSign);
		}
		color.xyz += specular;
	}

	if (UNDERWATER) {
		float dist;
		if (uViewPos.y < uParam.y)
			dist = abs((In.coord.y - uParam.y) / normalize(uViewPos.xyz - In.coord.xyz).y);
		else
			dist = length(uViewPos.xyz - In.coord.xyz);
		float fog = saturate(1.0 / exp(dist * WATER_FOG_DIST * uwSign));
		dist += In.coord.y - uParam.y;
		color.xyz *= lerp(float3(1.0, 1.0, 1.0), UNDERWATER_COLOR, clamp(dist * WATER_COLOR_DIST * uwSign, 0.0, 2.0));
		color.xyz = lerp(UNDERWATER_COLOR * 0.2, color.xyz, fog);
	} else {
		color.xyz = lerp(uFogParams.xyz, color.xyz, In.normal.w);
	}

	return color;
}
#endif