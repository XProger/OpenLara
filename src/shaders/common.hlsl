#ifdef __psp2__
	#define _GAPI_GXM
	#pragma pack_matrix( column_major )
#endif

#define ALPHA_REF			0.5
#define MAX_LIGHTS          4
#define MAX_CONTACTS        15
#define WATER_FOG_DIST      (1.0 / (6.0 * 1024.0))
#define WATER_COLOR_DIST    (1.0 / (2.0 * 1024.0))
#define UNDERWATER_COLOR    float3(0.6, 0.9, 0.9)
#define UNDERWATER_COLOR_H  half3(0.6, 0.9, 0.9)
#define SHADOW_NORMAL_BIAS  16.0
#define SHADOW_CONST_BIAS   0.05
#define SHADOW_SIZE         1024
#define PI                  3.141592653589793

static const float3 SHADOW_TEXEL = float3(1.0 / SHADOW_SIZE, 1.0 / SHADOW_SIZE, 0.0);

#ifdef _GAPI_GXM
	#define FLAGS_REG   c94
	#define FLAGS_TYPE  float4
	#define RGBA(c)     (c).rgba
	#define RGB(c)      (c).rgb
#else
	#define FLAGS_REG     b0
	#define FLAGS_TYPE    bool4
	#define RGBA(c)       (c).rgba
	#define RGB(c)        (c).rgb
	#define CLIP_PLANE
#endif

struct VS_INPUT {
	float4 aCoord    : POSITION;
	float4 aNormal   : NORMAL;
	float4 aTexCoord : TEXCOORD0;
	float4 aColor    : COLOR0;
	float4 aLight    : COLOR1;
};

sampler2D   sDiffuse     : register(s0);
sampler2D   sNormal      : register(s1);
sampler2D   sReflect     : register(s2);
sampler2D   sShadow      : register(s3);
samplerCUBE sEnvironment : register(s4);
sampler2D   sMask        : register(s5);

float4      uParam                  : register(  c0 );
float4      uTexParam               : register(  c1 );
float4x4    uViewProj               : register(  c2 );
float4      uBasis[32 * 2]          : register(  c6 );
float4x4    uLightProj              : register( c70 );
float4      uMaterial               : register( c74 );
float4      uAmbient[6]             : register( c75 );
float4      uFogParams              : register( c81 );
float4      uViewPos                : register( c82 );
float4      uLightPos[MAX_LIGHTS]   : register( c83 );
float4      uLightColor[MAX_LIGHTS] : register( c87 );
float4      uRoomSize               : register( c91 );
float4      uPosScale[2]            : register( c92 );
FLAGS_TYPE  uFlags[4]               : register( FLAGS_REG );
float4      uContacts[MAX_CONTACTS] : register( c98 );

// options for compose, shadow, ambient passes
#define OPT_AMBIENT             uFlags[0].x
#define OPT_SHADOW              uFlags[0].y
#define OPT_CONTACT             uFlags[0].z
#define OPT_CAUSTICS            uFlags[0].w

float4 pack(float value) {
	float4 v = frac(value * float4(1.0, 255.0, 65025.0, 16581375.0));
	return v - v.yzww * float4(1.0/255.0, 1.0/255.0, 1.0/255.0, 0.0);
}

float unpack(float4 value) {
	return dot(value, float4(1.0, 1.0/255.0, 1.0/65025.0, 1.0/16581375.0));
}

float3 mulQuat(float4 q, float3 v) {
	return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + v * q.w);
}

float3 mulBasis(float4 rot, float3 pos, float3 v) {
	return mulQuat(rot, v) + pos;
}

float3 calcAmbient(float3 n) {
	float3 sqr = n * n;
	float3 pos = step((float3)0.0, n);
	return	sqr.x * lerp(uAmbient[1].xyz, uAmbient[0].xyz, pos.x) +
			sqr.y * lerp(uAmbient[3].xyz, uAmbient[2].xyz, pos.y) +
			sqr.z * lerp(uAmbient[5].xyz, uAmbient[4].xyz, pos.z);
}

float calcSpecular(float3 normal, float3 viewVec, float3 lightVec, float intensity) {
	float3 vv = normalize(viewVec);
	float3 rv = reflect(-vv, normal);
	float3 lv = normalize(lightVec);
	return pow(max(0.0, dot(rv, lv)), 8.0) * intensity;
}

float calcCaustics(float3 coord, float3 n) {
	float2 cc = saturate((coord.xz - uRoomSize.xy) / uRoomSize.zw);
	return tex2Dlod(sReflect, float4(cc.x, 1.0 - cc.y, 0, 0)).x * max(0.0, -n.y);
}

half3 calcNormalV(float2 tc, half base) {
	half dx = (half)tex2Dlod(sNormal, float4(tc.x + uTexParam.x, tc.y, 0, 0)).x - base;
	half dz = (half)tex2Dlod(sNormal, float4(tc.x, tc.y + uTexParam.y, 0, 0)).x - base;
	return normalize( half3(dx, 64.0 / (1024.0 * 8.0), dz) );
}

float3 calcNormalF(float2 tcR, float2 tcB, float base) {
	float dx = tex2D(sNormal, tcR).x - base;
	float dz = tex2D(sNormal, tcB).x - base;
	return normalize( float3(dx, 64.0 / (1024.0 * 8.0), dz) );
}

half calcFresnel(half VoH, half f0) {
	half f = pow(1.0 - VoH, 5.0);
	return f + f0 * (1.0f - f);
}

void applyFogUW(inout float3 color, float3 coord, float waterFogDist) {
	float dist;
	if (uViewPos.y < uParam.y)
		dist = abs((coord.y - uParam.y) / normalize(uViewPos.xyz - coord.xyz).y);
	else
		dist = length(uViewPos.xyz - coord.xyz);
	float fog = saturate(1.0 / exp(dist * waterFogDist));
	dist += coord.y - uParam.y;
	color.xyz *= lerp(float3(1.0, 1.0, 1.0), UNDERWATER_COLOR, clamp(dist * waterFogDist, 0.0, 2.0));
	color.xyz = lerp(UNDERWATER_COLOR * 0.2, color.xyz, fog);
}

void applyFog(inout float3 color, float fogFactor) {
	color.xyz = lerp(uFogParams.xyz, color.xyz, fogFactor);
}

float SHADOW(float2 p) {
	#ifdef SHADOW_DEPTH
		return tex2Dlod(sShadow, float4(p, 0, 0)).x;
	#else
		return unpack(tex2Dlod(sShadow, float4(p, 0, 0)));
	#endif
}

float getShadowValue(float3 lightVec, float4 lightProj) {
/*
	float sMin = min(lightProj.x, lightProj.y);
	float sMax = max(lightProj.x, lightProj.y);
	float vis = lightProj.w;
	if (TYPE_ROOM) {
		vis = min(vis, dot(normal, lightVec));
	}
	sMin = min(vis, sMin);
*/
	float factor = step(0.0, lightProj.w); //float((sMin > 0.0f) && (sMax < lightProj.w)); // 
	lightProj.xyz *= factor;

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

	//rShadow = lerp(1.0, rShadow, factor);
	
	float fade = saturate(dot(lightVec, lightVec));
	return rShadow + (1.0 - rShadow) * fade;
}

float getShadow(float3 lightVec, float3 normal, float4 lightProj) {
	float factor = clamp(1.0 - dot(normalize(lightVec), normal), 0.0, 1.0);
	factor *= SHADOW_NORMAL_BIAS;
	return getShadowValue(lightVec, lightProj /*mul(uLightProj, float4(coord + normal * factor, 1.0)) */ );
}

float getContactAO(float3 p, float3 n) {
	float res = 1.0;
#ifdef _GAPI_GXM
	#pragma loop (unroll: always)
#else
	[unroll]
#endif
	for (int i = 0; i < MAX_CONTACTS; i++) {
		float3 v = uContacts[i].xyz - p;
		float  a = uContacts[i].w;
		float  o = a * saturate(dot(n, v)) / dot(v, v);
		res *= saturate(1.0 - o);
	}
	return res;
}
