#include "common.hlsl"

struct VS_OUTPUT {
	float4 pos       : POSITION;
	float2 texCoord  : TEXCOORD0;
	float2 maskCoord : TEXCOORD1;
};

float2 getInvUV(float2 uv, float4 param) {
	float2 p = (float2(uv.x, -uv.y) * 0.5 + 0.5) * param.zw;
#ifndef _GAPI_GXM
	p.xy += 0.5 * param.xy;
#endif
	return p;
}

float2 getUV(float2 uv, float4 param) {
	return (uv.xy * 0.5 + 0.5) * param.zw;
}

#ifdef VERTEX
VS_OUTPUT main(VS_INPUT In) {
	VS_OUTPUT Out;
	
	float3 coord = In.aCoord.xyz * (1.0 / 32767.0);

	Out.pos       = float4(coord.xyz, 1.0);
	Out.texCoord  = getInvUV(coord.xy, uTexParam);
	Out.maskCoord = getUV(coord.xy, uRoomSize);
	
	return Out;
}

#else // PIXEL

half4 main(VS_OUTPUT In) : COLOR0 {
	float2 uv = In.texCoord.xy;
	if (tex2Dlod(sMask, float4(In.maskCoord, 0, 0)).a < 0.5)
		return 0.0;

	half2 v = (half2)tex2Dlod(sNormal, float4(uv, 0, 0)).xy; // height, speed

	float3 d = float3(float2(uTexParam.x, -uTexParam.y), 0.0);
	float4 f = float4(
			F2_TEX2D(sNormal, uv + d.xz).x, F2_TEX2D(sNormal, uv + d.zy).x,
			F2_TEX2D(sNormal, uv - d.xz).x, F2_TEX2D(sNormal, uv - d.zy).x
		);

	float average = dot(f, (float4)0.25);

// integrate
	const half vel = 1.4;
	const half vis = 0.995;

	v.y += (half)((average - v.x) * vel);
	v.y *= vis;
	v.x += v.y + (half)(tex2Dlod(sDiffuse, float4(uv + uParam.zw, 0, 0)).x * 2.0 - 1.0) * 0.00025;
	
	return half4(v, 0, 0);

}

#endif
