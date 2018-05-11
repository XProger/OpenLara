#define MAX_LIGHTS		4
#define MAX_CONTACTS	15

struct VS_INPUT {
	float4 aCoord		: POSITION;
	float4 aNormal		: NORMAL;
	float4 aTexCoord	: TEXCOORD0;
	float4 aColor		: COLOR0;
	float4 aLight		: COLOR1;
};

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

struct VS_OUTPUT {
	float4 wPos			: POSITION;
	float2 vTexCoord	: TEXCOORD0;
	float4 vDiffuse		: COLOR0;
};

#ifdef VERTEX
VS_OUTPUT main(VS_INPUT In) {
	VS_OUTPUT Out;
	Out.wPos      = mul(uViewProj, float4(In.aCoord.xy * uPosScale[0].zw + uPosScale[0].xy, 0.0, 1.0));
	Out.vTexCoord = In.aTexCoord.xy * (1.0 / 32767.0);
	Out.vDiffuse  = In.aLight * uMaterial;
	return Out;
}

#else // PIXEL

sampler sDiffuse : register(s0);

float4 main(VS_OUTPUT In) : COLOR0 {
	return float4(1.0, 1.0, 1.0, 1.0);//(In.vDiffuse * tex2D(sDiffuse, In.vTexCoord.xy)).bgra;
}
#endif