#define MAX_LIGHTS		4
#define MAX_CONTACTS	15

struct VS_INPUT {
	float4 aCoord		: POSITION;
	float4 aNormal		: NORMAL;
	float4 aTexCoord	: TEXCOORD0;
	float4 aColor		: COLOR0;
	float4 aLight		: COLOR1;
};

int4        uInfo                   : register( c0   );
float4		uParam					: register( c1   );
float4		uTexParam				: register( c2   );
float4x4	uViewProj				: register( c3   );
float4		uBasis[32 * 2]			: register( c7   );
float4x4	uLightProj				: register( c71  );
float4		uMaterial				: register( c103 );
float4		uAmbient[6]				: register( c104 );
float4		uFogParams				: register( c110 );
float4		uViewPos				: register( c111 );
float4		uLightPos[MAX_LIGHTS]	: register( c112 );
float4		uLightColor[MAX_LIGHTS]	: register( c116 );
float4		uRoomSize				: register( c120 );
float4		uPosScale[2]			: register( c121 );
float4		uContacts[MAX_CONTACTS]	: register( c123 );