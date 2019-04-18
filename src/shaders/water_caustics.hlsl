#include "common.hlsl"

struct VS_OUTPUT {
	float4 pos      : POSITION;
	float3 oldPos   : TEXCOORD1;
	float3 newPos   : TEXCOORD2;
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

	float3 coord  = In.aCoord.xyz * (1.0 / 32767.0);

	float3 rCoord = float3(coord.x, coord.y, 0.0) * uPosScale[1].xzy;

	float2 uv     = getInvUV(rCoord.xy, uTexParam).xy;
	float2 info   = SAMPLE_2D_LOD0(sNormal, uv).xy;
	float3 normal = calcHeightMapNormal(float2(uv.x + uTexParam.x, uv.y), float2(uv.x, uv.y - uTexParam.y), info.x).xzy;

	float3 light  = float3(0.0, 0.0, 1.0);
	float3 refOld = refract(-light, float3(0.0, 0.0, 1.0), 0.75);
	float3 refNew = refract(-light, normalize(normal + float3(0.0, 0.0, 0.25)), 0.75);

	Out.oldPos = rCoord + refOld * (-1.0 / refOld.z) + refOld * ((-refOld.z - 1.0) / refOld.z);
	Out.newPos = rCoord + refNew * ((info.r - 1.0) / refNew.z) + refOld * ((-refNew.z - 1.0) / refOld.z);

	Out.pos = float4(Out.newPos.xy + refOld.xy / refOld.z, 0.0, 1.0);

	return Out;
}

#else // PIXEL

float4 main(VS_OUTPUT In) : COLOR0 {
	float rOldArea = length(ddx(In.oldPos)) * length(ddy(In.oldPos));
	float rNewArea = length(ddx(In.newPos)) * length(ddy(In.newPos));
	rNewArea = max(rNewArea, 0.00002); // WebGL NVIDIA workaround >_<
	float value = saturate(rOldArea / rNewArea * 0.2);
	return float4(value, 0.0, 0.0, 0.0);
}

#endif
