#include "common.hlsl"

struct VS_OUTPUT {
	float4 pos        : POSITION;
	half2  texCoord   : TEXCOORD0;
	half2  maskCoord  : TEXCOORD1;
	half2  texCoordL  : TEXCOORD2;
	half2  texCoordR  : TEXCOORD3;
	half2  texCoordT  : TEXCOORD4;
	half2  texCoordB  : TEXCOORD5;
	half2  noiseCoord : TEXCOORD6;
};

#ifdef VERTEX

VS_OUTPUT main(VS_INPUT In) {
	VS_OUTPUT Out;
	
	float3 coord = In.aCoord.xyz * (1.0 / 32767.0);
	float4 uv = float4(coord.x, coord.y, coord.x, -coord.y) * 0.5 + 0.5;

	Out.pos       = float4(coord.xyz, 1.0);
	Out.maskCoord = uv.xy * uRoomSize.zw;
	Out.texCoord  = uv.zw * uTexParam.zw;
	#ifndef _GAPI_GXM
		Out.texCoord += 0.5 * uTexParam.xy;
	#endif
	
	float3 d = float3(uTexParam.x, uTexParam.y, 0.0);
	Out.texCoordL  = Out.texCoord - d.xz;
	Out.texCoordR  = Out.texCoord + d.xz;
	Out.texCoordT  = Out.texCoord - d.zy;
	Out.texCoordB  = Out.texCoord + d.zy;
	Out.noiseCoord = Out.texCoord + uParam.zw;
	
	return Out;
}

#else // PIXEL

#define WATER_VEL	1.4
#define WATER_VIS	0.995

half4 main(VS_OUTPUT In) : COLOR0 {
	half4 v = tex2D(sNormal, In.texCoord.xy); // height, speed

	half average = (
		tex2D(sNormal, In.texCoordL).x +
		tex2D(sNormal, In.texCoordR).x +
		tex2D(sNormal, In.texCoordT).x +
		tex2D(sNormal, In.texCoordB).x) * 0.25;
	
// integrate
	v.y += (average - v.x) * WATER_VEL;
	v.y *= WATER_VIS;
	v.x += v.y;
	v.x += tex2D(sDiffuse, In.noiseCoord).x * 0.00025;

	v *= tex2D(sMask, In.maskCoord).a;
	
	return v;
}

#endif
