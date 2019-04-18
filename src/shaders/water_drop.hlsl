#include "common.hlsl"

struct VS_OUTPUT {
	float4 pos       : POSITION;
	float2 texCoord  : TEXCOORD0;
	float2 dropCoord : TEXCOORD1;
};

float2 getInvUV(float2 uv, float4 param) {
	float2 p = (float2(uv.x, -uv.y) * 0.5 + 0.5) * param.zw;
#ifndef _GAPI_GXM
	p.xy += 0.5 * param.xy;
#endif
	return p;
}

float2 getUV(float2 uv, float4 param) {
	float2 p = (uv.xy * 0.5 + 0.5) * param.zw;
	return p;
}

#ifdef VERTEX

VS_OUTPUT main(VS_INPUT In) {
	VS_OUTPUT Out;

	float3 coord = In.aCoord.xyz * (1.0 / 32767.0);

	Out.pos       = float4(coord.xyz, 1.0);
	Out.texCoord  = getInvUV(coord.xy, uTexParam);
	Out.dropCoord = getUV(coord.xy, uTexParam);
	
	return Out;
}

#else // PIXEL

half4 main(VS_OUTPUT In) : COLOR0 {
	half2 v = SAMPLE_2D_LINEAR(sNormal, In.texCoord.xy).xy;

	float value = max(0.0, 1.0 - length(uParam.xy - In.dropCoord / uTexParam.xy) / uParam.z);
	value = 0.5 - cos(value * PI) * 0.5;
	value *= uParam.w;

	v.x += (half)value;
	//v.x = 1.0;

	return half4(v, 0, 0);
}

#endif
