#ifdef _GAPI_GXM
	#pragma pack_matrix( column_major )
#endif

#define ALPHA_REF           0.5
#define MAX_LIGHTS          4
#define MAX_CONTACTS        15
#define WATER_FOG_DIST      (1.0 / (6.0 * 1024.0))
#define WATER_COLOR_DIST    (1.0 / (2.0 * 1024.0))
#define UNDERWATER_COLOR    float3(0.6, 0.9, 0.9)
#define UNDERWATER_COLOR_H  half3(0.6, 0.9, 0.9)
#define SHADOW_NORMAL_BIAS  1.0
#define SHADOW_CONST_BIAS   0.05

#ifdef _GAPI_GXM
	#define SHADOW_SIZE     1024.0
#else
	#define SHADOW_SIZE     2048.0
#endif

#define SHADOW_TEXEL        (1.0 / SHADOW_SIZE)

#define PI                  3.141592653589793

#define INV_SHORT_HALF      (1.0 / 32767.0)

#if (defined(_GAPI_D3D11) || defined(_GAPI_GXM)) && !defined(_GAPI_D3D11_9_3)
	#define SHADOW_DEPTH
#endif

struct VS_INPUT {
#ifdef _GAPI_GXM
	float4 aCoord    : POSITION;
	float4 aNormal   : NORMAL;
	float4 aTexCoord : TEXCOORD0;
#else
	int4   aCoord    : POSITION;
	int4   aNormal   : NORMAL;
	int4   aTexCoord : TEXCOORD0;
#endif
	float4 aColor    : COLOR0;
	float4 aLight    : COLOR1;
};

#ifdef _GAPI_D3D11
	SamplerState           smpDefault    : register(s0);
	SamplerState           smpPoint      : register(s1);
	SamplerState           smpPointWrap  : register(s2);
	SamplerState           smpLinear     : register(s3);
	SamplerState           smpLinearWrap : register(s4);
	SamplerComparisonState smpCmp        : register(s5);

	#ifdef DIFFUSE_AS_CUBE
		TextureCube  sDiffuse : register(t0);
	#else
		Texture2D    sDiffuse : register(t0);
	#endif

	#ifdef NORMAL_AS_3D
		Texture3D    sNormal  : register(t1);
	#else
		Texture2D    sNormal  : register(t1);
	#endif

	Texture2D    sReflect     : register(t2);
	Texture2D    sShadow      : register(t3);
	Texture2D    sMask        : register(t4);

	#define SAMPLE_2D(T,uv)             T.Sample(smpDefault,     uv)
	#define SAMPLE_2D_POINT(T,uv)       T.Sample(smpPoint,       uv)
	#define SAMPLE_2D_POINT_WRAP(T,uv)  T.Sample(smpPointWrap,   uv)
	#define SAMPLE_2D_LINEAR(T,uv)      T.Sample(smpLinear,      uv)
	#define SAMPLE_2D_LINEAR_WRAP(T,uv) T.Sample(smpLinearWrap,  uv)

	#ifdef _GAPI_D3D11_9_3
		#define SAMPLE_2D_LOD0(T,uv)        SAMPLE_2D_POINT(T, uv)
		#define SAMPLE_SHADOW(T,uv)         ((unpack(SAMPLE_2D_POINT(T, uv.xy/uv.w)) >= uv.z/uv.w) ? 1 : 0)
	#else
		#define SAMPLE_2D_LOD0(T,uv)        T.SampleLevel(smpLinear, uv, 0)
		#define SAMPLE_SHADOW(T,uv)         T.SampleCmpLevelZero(smpCmp, uv.xy/uv.w, uv.z/uv.w)
	#endif

	#define SAMPLE_3D(T,uv)             T.SampleLevel(smpLinearWrap, uv, 0)
	#define SAMPLE_CUBE(T,uv)           T.Sample(smpLinear, uv)

	#define POSITION    SV_POSITION
#else
	#ifdef DIFFUSE_AS_CUBE
		samplerCUBE  sDiffuse : register(s0);
	#else
		sampler2D    sDiffuse : register(s0);
	#endif

	sampler2D    sNormal      : register(s1);
	sampler2D    sReflect     : register(s2);
	sampler2D    sShadow      : register(s3);
	sampler2D    sMask        : register(s4);

	#define SAMPLE_2D(T,uv)             tex2D(T, uv)
	#define SAMPLE_2D_POINT(T,uv)       tex2D(T, uv)
	#define SAMPLE_2D_POINT_WRAP(T,uv)  tex2D(T, uv)
	#define SAMPLE_2D_LINEAR(T,uv)      tex2D(T, uv)
	#define SAMPLE_2D_LINEAR_WRAP(T,uv) tex2D(T, uv)
	#define SAMPLE_2D_LOD0(T,uv)        tex2Dlod(T, float4(uv.xy, 0, 0))

	#if defined(_GAPI_GXM)
		#define SAMPLE_SHADOW(T,uv)     f1tex2Dproj(T, uv)
	#else
		#define SAMPLE_SHADOW(T,uv)     ((unpack(tex2D(T, uv.xy/uv.w)) >= uv.z/uv.w) ? 1 : 0)
	#endif

	#define SAMPLE_3D(T,uv)             tex3D(T, uv)
	#define SAMPLE_CUBE(T,uv)           texCUBE(T, uv)

	#if defined(PIXEL) && !defined(_GAPI_GXM)
		#define POSITION    VPOS
	#endif
#endif

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
float4      uContacts[MAX_CONTACTS] : register( c98 );

// options for compose, shadow, ambient passes
#ifdef _GAPI_GXM
	//#define OPT_AMBIENT
	#define OPT_SHADOW
	//#define OPT_CONTACT
	//#define OPT_CAUSTICS
#elif _GAPI_D3D11_9_3
	//#define OPT_AMBIENT
	//#define OPT_SHADOW
	//#define OPT_CONTACT
	//#define OPT_CAUSTICS
#else
    #define OPT_AMBIENT
    #define OPT_SHADOW
    #define OPT_CONTACT
    #define OPT_CAUSTICS
#endif

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
	float2 border = 256.0 / uRoomSize.zw;
	float2 fade   = smoothstep((float2)0.0, border, cc) * (1.0 - smoothstep(1.0 - border, 1.0, cc));
	return SAMPLE_2D_LINEAR(sReflect, float2(cc.x, 1.0 - cc.y)).x * max(0.0, -n.y) * fade.x * fade.y;
}

float calcCausticsV(float3 coord) {
	return 0.5 + abs(sin(dot(coord.xyz, 1.0 / 1024.0) + uParam.x)) * 0.75;
}

#ifndef NORMAL_AS_3D
float3 calcHeightMapNormal(float2 tcR, float2 tcB, float base) {
	float dx = SAMPLE_2D_LOD0(sNormal, tcR).x - base;
	float dz = SAMPLE_2D_LOD0(sNormal, tcB).x - base;
	return normalize( float3(dx, 64.0 / (1024.0 * 8.0), dz) );
}
#endif

float calcFresnel(float NdotV, float f0) {
	return f0 + (1.0 - f0) * pow(1.0 - NdotV, 5.0);
}

void applyFogUW(inout float3 color, float3 coord, float waterFogDist, float waterColorDist) {
	float h    = coord.y - uParam.y;
	float3 dir = uViewPos.xyz - coord.xyz;
	float dist = lerp(length(dir), abs(h / normalize(dir).y), step(uViewPos.y, uParam.y));
/*
	if (uViewPos.y < uParam.y) {
		dist = abs(h / normalize(dir).y);
	} else {
		dist = length(dir);
	}
*/
	float fog = saturate(1.0 / exp(dist * waterFogDist));
	dist += h;
	color.xyz *= lerp((float3)1.0, UNDERWATER_COLOR, clamp(dist * waterColorDist, 0.0, 2.0));
	color.xyz = lerp(UNDERWATER_COLOR * 0.2, color.xyz, fog);
}

void applyFog(inout float3 color, float fogFactor) {
	color.xyz = lerp(uFogParams.xyz, color.xyz, fogFactor);
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
	lightProj.z -= SHADOW_CONST_BIAS * SHADOW_TEXEL * lightProj.w;


	float rShadow = SAMPLE_SHADOW(sShadow, lightProj);

	float fade = saturate(dot(lightVec, lightVec));
	return rShadow + (1.0 - rShadow) * fade;
}

float getShadow(float3 lightVec, float4 lightProj) {
	return getShadowValue(lightVec, lightProj);
}

float4 calcLightProj(float3 coord) {
	return mul(uLightProj, float4(coord, 1.0));
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
