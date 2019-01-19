#include "common.hlsl"

struct VS_OUTPUT {
	float4 pos       : POSITION;
	float3 coord     : TEXCOORD0;
	float2 texCoord  : TEXCOORD1;
	float2 maskCoord : TEXCOORD2;
	float3 viewVec   : TEXCOORD3;
	float3 lightVec  : TEXCOORD4;
	float3 hpos      : TEXCOORD5;
};

float2 invUV(float2 uv) {
	return float2(uv.x, 1.0 - uv.y);
}

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

	Out.coord     = float3(coord.x, 0.0, coord.y) * uPosScale[1].xyz + uPosScale[0].xyz;
	Out.pos       = mul(uViewProj, float4(Out.coord, 1.0));
	Out.viewVec   = uViewPos.xyz - Out.coord.xyz;
	Out.lightVec  = uLightPos[0].xyz - Out.coord.xyz;
	Out.texCoord  = getInvUV(coord.xy, uTexParam);
	Out.maskCoord = getUV(coord.xy, uRoomSize);
	Out.hpos      = Out.pos.xyw;
	
	return Out;
}

#else // PIXEL

float calcFresnel(float VoH, float f0) {
	float f = pow(1.0 - VoH, 5.0);
	return f + f0 * (1.0f - f);
}

float4 main(VS_OUTPUT In) : COLOR0 {
	float3 viewVec = normalize(In.viewVec);
	
	float2 value = tex2D(sNormal, In.texCoord).xy;

	float3 normal = calcNormal(In.texCoord, value.x);
	normal.y *= sign(viewVec.y);

	float2 dudv = mul(uViewProj, float4(normal.x, 0.0, normal.z, 0.0)).xy;

	float3 rv = reflect(-viewVec, normal);
	float3 lv = normalize(In.lightVec);

	float spec = pow(max(0.0, dot(rv, lv)), 64.0) * 0.5;

	float2 tc = In.hpos.xy / In.hpos.z * 0.5 + 0.5;

	float4 refrA = tex2D(sDiffuse, uParam.xy * invUV(clamp(tc + dudv * uParam.z, 0.0, 0.999)));
	float4 refrB = tex2D(sDiffuse, uParam.xy * invUV(tc));
	float4 refr	 = float4(lerp(refrA.xyz, refrB.xyz, refrA.w), 1.0);
	float4 refl	 = tex2D(sReflect, tc.xy + dudv * uParam.w);

	float fresnel = calcFresnel(max(0.0, dot(normal, viewVec)), 0.12);

	float4 color = lerp(refr, refl, fresnel) + spec * 1.5;
	color.w *= tex2D(sMask, In.maskCoord).a;
	
	float dist = In.viewVec.y / viewVec.y;
	dist *= step(In.coord.y, uViewPos.y);
	color.xyz *= lerp(float3(1.0, 1.0, 1.0), UNDERWATER_COLOR, clamp(dist * WATER_COLOR_DIST, 0.0, 2.0));
	float fog = saturate(1.0 / exp(dist * WATER_FOG_DIST));
	color.xyz = lerp(UNDERWATER_COLOR * 0.2, color.xyz, fog);
	return color;
}

#endif
