#include "common.hlsl"

struct VS_OUTPUT {
	float4 pos		: POSITION;
	float3 coord	: TEXCOORD0;
	float4 texCoord	: TEXCOORD1;
	float4 viewVec	: TEXCOORD2;
	float4 normal	: NORMAL;
	float4 diffuse	: COLOR0;
	float3 ambient	: COLOR1;
	float4 lightMap	: COLOR2;
	float4 light	: COLOR3;
	float4 hpos		: TEXCOORD4;
};

#ifdef VERTEX
float3 mulQuat(float4 q, float3 v) {
	return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + v * q.w);
}

float3 mulBasis(float4 rot, float3 pos, float3 v) {
	return mulQuat(rot, v) + pos;
}

float3 calcAmbient(float3 n) {
	float3 sqr = n * n;
	float3 pos = step(0.0, n);
	return	sqr.x * lerp(uAmbient[1].xyz, uAmbient[0].xyz, pos.x) +
			sqr.y * lerp(uAmbient[3].xyz, uAmbient[2].xyz, pos.y) +
			sqr.z * lerp(uAmbient[5].xyz, uAmbient[4].xyz, pos.z);
}

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

	#ifdef PASS_SHADOW
		Out.viewVec = 0.0;
		Out.normal  = 0.0;
		Out.diffuse = 0.0;
	#else
		Out.viewVec = float4((uViewPos.xyz - Out.coord) * uFogParams.w, Out.coord.y * uParam.z);

		#ifdef PASS_COMPOSE
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
						Out.ambient = min(uMaterial.yyy, In.aLight.xyz);
					}
				} else {
					if (TYPE_SPRITE) {
						lum.x = uMaterial.y;
					} else {
						lum.x = 1.0;
					}
					att.x = 0.0;

					Out.ambient = min(uMaterial.yyy, In.aLight.zyx);
				}

				float4 light;
				lum.y = dot(Out.normal.xyz, normalize(lv1)); att.y = dot(lv1, lv1);
				lum.z = dot(Out.normal.xyz, normalize(lv2)); att.z = dot(lv2, lv2);
				lum.w = dot(Out.normal.xyz, normalize(lv3)); att.w = dot(lv3, lv3);
				light = max(0.0, lum) * max(0.0, 1.0 - att);

				if (UNDERWATER) {
					light.x *= abs(sin(dot(Out.coord.xyz, 1.0 / 512.0) + uParam.x)) * 1.5 + 0.5;

					float d;
					if (uViewPos.y < uParam.y) {
						d = abs((Out.coord.y - uParam.y) / normalize(Out.viewVec.xyz).y);
					} else {
						d = length(uViewPos.xyz - Out.coord.xyz);
					}

					fog = d * WATER_FOG_DIST;
				} else {
					fog = length(Out.viewVec.xyz);
				}
				Out.normal.w = saturate(1.0 / exp(fog));

				if (OPT_SHADOW) {
					Out.light    = light;
					Out.lightMap = In.aLight.zyxw * light.x;
				} else {
					Out.light.xyz = uLightColor[1].xyz * light.y + uLightColor[2].xyz * light.z + uLightColor[3].xyz * light.w;
					Out.light.w = 0.0;

					if (TYPE_ENTITY) {
						Out.light.xyz += Out.ambient + uLightColor[0].xyz * light.x;
					} else {
						Out.light.xyz += In.aLight.xyz * light.x;
					}
				}
			} else
				Out.normal.w = 1.0;

		#else
			Out.normal = In.aNormal;
			Out.light  = float4(In.aLight.xyz, 1.0);
		#endif

		if (TYPE_MIRROR) {
			Out.diffuse = uMaterial;
		} else {
			Out.diffuse = float4(In.aColor.zyx * (uMaterial.x * 1.8), uMaterial.w);

			if (UNDERWATER) {
				Out.diffuse.xyz *= UNDERWATER_COLOR;
			}

			if (TYPE_FLASH) {
				Out.diffuse.xyz += uMaterial.w;
			}
		}
	#endif

	Out.pos = mul(uViewProj, float4(Out.coord, rBasisPos.w));
	Out.hpos = Out.pos;
	
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
	float3 p = lightProj.xyz / lightProj.w;

	p.y = -p.y;
	p.xy = p.xy * 0.5 + 0.5;
	
	p.z -= SHADOW_CONST_BIAS * SHADOW_TEXEL.x;

	float vis = lightProj.w;
	if (TYPE_ROOM) {
		vis = min(vis, dot(normal, lightVec));
	}
	if (vis < 0.0 || p.x < 0.0 || p.y < 0.0 || p.x > 1.0 || p.y > 1.0) return 1.0;

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

	float fade = saturate(dot(lightVec, lightVec));
	return rShadow + (1.0 - rShadow) * fade;
}

float getShadow(float3 coord, float3 lightVec, float3 normal) {
	float factor = clamp(1.0 - dot(normalize(lightVec), normal), 0.0, 1.0);
	factor *= SHADOW_NORMAL_BIAS;
	return getShadow(lightVec, normal, mul(uLightProj, float4(coord + normal * factor, 1.0)));
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

float calcSpecular(float3 normal, float3 viewVec, float3 lightVec, float4 color, float intensity) {
	float3 vv = normalize(viewVec);
	float3 rv = reflect(-vv, normal);
	float3 lv = normalize(lightVec);
	return pow(max(0.0, dot(rv, lv)), 8.0) * intensity;
}

float4 main(VS_OUTPUT In) : COLOR0 {
	float2 uv = In.texCoord.xy;

	#ifdef PASS_COMPOSE
		if (CLIP_PLANE) {
			if (In.viewVec.w > uParam.w) {
				discard;
			}
		}

		if (!TYPE_SPRITE) {
			uv /= In.texCoord.zw;
		}
	#endif

	float4 color;

	if (TYPE_MIRROR) {
		float3 rv = reflect(-normalize(In.viewVec.xyz), normalize(In.normal.xyz));
		color = texCUBE(sEnvironment, normalize(rv));
	} else {
		color = tex2D(sDiffuse, uv).bgra;

		if (ALPHA_TEST) {
			if (color.w <= 0.5)
				discard;
		}
	}

	#ifdef PASS_SHADOW
		#ifdef SHADOW_DEPTH
			return 0.0;
		#else // SHADOW_COLOR
			return pack(In.hpos.z / In.hpos.w);
		#endif
	#else
		color *= In.diffuse;

		if (TYPE_FLASH) {
			return color;
		}

		if (TYPE_MIRROR) {
			return color;
		}

		#ifdef PASS_AMBIENT
			color.xyz *= In.light.xyz;
		#endif

		#ifdef PASS_COMPOSE
			float3 lightVec = (uLightPos[0].xyz - In.coord) * uLightColor[0].w;
			float3 normal   = normalize(In.normal.xyz);

			float rSpecular = 0.0;

			float3 light;
			if (OPT_SHADOW) {
				light = uLightColor[1].xyz * In.light.y + uLightColor[2].xyz * In.light.z + uLightColor[3].xyz * In.light.w;

				if (TYPE_ENTITY) {
					float rShadow = getShadow(In.coord, lightVec, normal);
					rSpecular = (uMaterial.z + 0.03) * rShadow;
					light += In.ambient + uLightColor[0].xyz * (In.light.x * rShadow);
				} else if (TYPE_ROOM) {
					float rShadow = getShadow(In.coord, lightVec, normal);
					light += lerp(In.ambient.xyz, In.lightMap.xyz, rShadow);
				} else if (TYPE_SPRITE) {
					light += In.lightMap.xyz;
				}
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

			if (TYPE_ENTITY) {
				color.xyz += calcSpecular(normal, In.viewVec.xyz, lightVec, uLightColor[0], rSpecular);
			}

			if (UNDERWATER) {
				color.xyz = lerp(UNDERWATER_COLOR * 0.2, color.xyz, In.normal.w);
			} else {
				color.xyz = lerp(uFogParams.xyz, color.xyz, In.normal.w);
			}
		#endif
	#endif

	return color;
}
#endif