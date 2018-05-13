#define MAX_LIGHTS			4
#define MAX_CONTACTS		15
#define WATER_FOG_DIST		(1.0 / (6.0 * 1024.0))
#define UNDERWATER_COLOR	float3(0.6, 0.9, 0.9)

struct VS_INPUT {
	float4 aCoord		: POSITION;
	float4 aNormal		: NORMAL;
	float4 aTexCoord	: TEXCOORD0;
	float4 aColor		: COLOR0;
	float4 aLight		: COLOR1;
};

#ifdef PIXEL
	sampler sDiffuse		: register(s0);
	sampler sNormal			: register(s1);
	sampler sReflect		: register(s2);
	sampler sShadow			: register(s3);
	sampler sEnvironment	: register(s4);
	sampler sMask			: register(s5);
#endif

bool		uFlags[16]				: register( b0   );
float4		uParam					: register( c0   );
float4		uTexParam				: register( c1   );
float4x4	uViewProj				: register( c2   );
float4		uBasis[32 * 2]			: register( c6   );
float4x4	uLightProj				: register( c70  );
float4		uMaterial				: register( c102 );
float4		uAmbient[6]				: register( c103 );
float4		uFogParams				: register( c109 );
float4		uViewPos				: register( c110 );
float4		uLightPos[MAX_LIGHTS]	: register( c111 );
float4		uLightColor[MAX_LIGHTS]	: register( c115 );
float4		uRoomSize				: register( c119 );
float4		uPosScale[2]			: register( c120 );
float4		uContacts[MAX_CONTACTS]	: register( c122 );

#define TYPE_SPRITE				uFlags[0]
#define TYPE_FLASH				uFlags[1]
#define TYPE_ROOM				uFlags[2]
#define TYPE_ENTITY				uFlags[3]
#define TYPE_MIRROR				uFlags[4]

#define FILTER_DEFAULT			uFlags[0]
#define FILTER_DOWNSAMPLE		uFlags[1]
#define FILTER_GRAYSCALE		uFlags[2]
#define FILTER_BLUR				uFlags[3]
#define FILTER_EQUIRECTANGULAR	uFlags[4]

#define WATER_DROP				uFlags[0]
#define WATER_STEP				uFlags[1]
#define WATER_CAUSTICS			uFlags[2]
#define WATER_MASK				uFlags[3]
#define WATER_COMPOSE			uFlags[4]

// options for compose, shadow, ambient passes
#define UNDERWATER				uFlags[5]
#define ALPHA_TEST				uFlags[6]
#define CLIP_PLANE				uFlags[7]
#define OPT_AMBIENT				uFlags[8]
#define OPT_SHADOW				uFlags[9]
#define OPT_SHADOW_HIGH			uFlags[10]
#define OPT_CONTACT				uFlags[11]
#define OPT_CAUSTICS			uFlags[12]