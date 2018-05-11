#define MAX_LIGHTS		4
#define MAX_CONTACTS	15

struct VS_INPUT {
	float4 aCoord		: POSITION;
	float4 aNormal		: NORMAL;
	float4 aTexCoord	: TEXCOORD0;
	float4 aColor		: COLOR0;
	float4 aLight		: COLOR1;
};

struct VS_OUTPUT {
	float4 wPos			: POSITION;
	float3 vCoord		: TEXCOORD2;
	float4 vTexCoord	: TEXCOORD0;
	float4 vDiffuse		: COLOR0;
	float4 vNormal		: NORMAL;
	float4 vViewVec		: TEXCOORD1;
	float3 vAmbient		: COLOR1;
	float4 vLightMap	: COLOR2;
	float4 vLight		: COLOR3;
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

#ifdef VERTEX
float3 mulQuat(float4 q, float3 v) {
	return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + v * q.w);
}

float3 mulBasis(float4 rot, float3 pos, float3 v) {
	return mulQuat(rot, v) + pos;
}
	
VS_OUTPUT main(VS_INPUT In) {
	VS_OUTPUT Out;

	int index = int(In.aCoord.w * 2.0);
	float4 rBasisRot = uBasis[index];
	float4 rBasisPos = uBasis[index + 1];
	Out.vCoord    = mulBasis(rBasisRot, rBasisPos.xyz, In.aCoord.xyz);
	Out.wPos      = mul(uViewProj, float4(Out.vCoord, rBasisPos.w));

	Out.vTexCoord = In.aTexCoord * (1.0 / 32767.0);
	Out.vTexCoord.xy *= Out.vTexCoord.zw;

	Out.vDiffuse  = float4(In.aColor.xyz * In.aLight.xyz, 1.0);
	Out.vNormal   = In.aNormal;
	Out.vViewVec  = float4(uViewPos.xyz - Out.vCoord, 0.0);
	Out.vAmbient  = float3(0.2, 0.2, 0.2);
	Out.vLightMap = float4(1.0, 1.0, 1.0, 0.0);
	Out.vLight    = float4(0.5, 0.5, 0.5, 0.5);
	return Out;
}

#else // PIXEL

sampler sDiffuse		: register(s0);
sampler sNormal			: register(s1);
sampler sReflect		: register(s2);
sampler sShadow			: register(s3);
sampler sEnvironment	: register(s4);
sampler sMask			: register(s5);

float4 main(VS_OUTPUT In) : COLOR0 {
	return (In.vDiffuse * tex2D(sDiffuse, In.vTexCoord.xy / In.vTexCoord.zw)).bgra;
}
#endif