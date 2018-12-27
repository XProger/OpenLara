#ifdef __psp2__
	#define _GAPI_GXM
	#pragma pack_matrix( column_major )
#endif

#define MAX_LIGHTS          4
#define MAX_CONTACTS        15
#define WATER_FOG_DIST      (1.0 / (6.0 * 1024.0))
#define WATER_COLOR_DIST    (1.0 / (2.0 * 1024.0))
#define UNDERWATER_COLOR    float3(0.6, 0.9, 0.9)
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
	#define FLAGS_REG   b0
	#define FLAGS_TYPE  bool4
	#define RGBA(c)     (c).bgra
	#define RGB(c)      (c).bgr
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


#define TYPE_SPRITE             uFlags[0].x
#define TYPE_FLASH              uFlags[0].y
#define TYPE_ROOM               uFlags[0].z
#define TYPE_ENTITY             uFlags[0].w
#define TYPE_MIRROR             uFlags[1].x

#define FILTER_DEFAULT          uFlags[0].x
#define FILTER_DOWNSAMPLE       uFlags[0].y
#define FILTER_GRAYSCALE        uFlags[0].z
#define FILTER_BLUR             uFlags[0].w
#define FILTER_EQUIRECTANGULAR  uFlags[1].x

#define WATER_DROP              uFlags[0].x
#define WATER_SIMULATE          uFlags[0].y
#define WATER_CAUSTICS          uFlags[0].z
#define WATER_RAYS              uFlags[0].w
#define WATER_MASK              uFlags[1].x
#define WATER_COMPOSE           uFlags[1].y

// options for compose, shadow, ambient passes
#define UNDERWATER              uFlags[1].y
#define ALPHA_TEST              uFlags[1].z
#define CLIP_PLANE              uFlags[1].w
#define OPT_AMBIENT             uFlags[2].x
#define OPT_SHADOW              uFlags[2].y
#define OPT_CONTACT             uFlags[2].z
#define OPT_CAUSTICS            uFlags[2].w

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