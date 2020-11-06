#include "common.hlsl"

#define RAY_STEPS 16.0

struct VS_OUTPUT {
	float4 pos      : POSITION;
	float3 coord    : TEXCOORD0;
	float3 viewVec  : TEXCOORD1;
};

#ifdef VERTEX

VS_OUTPUT main(VS_INPUT In) {
	VS_OUTPUT Out;
	
	Out.coord   = In.aCoord.xyz + uParam.xyz;
	Out.viewVec = uViewPos.xyz - Out.coord.xyz;
	Out.pos	    = mul(uViewProj, float4(Out.coord, 1.0));
	
	return Out;
}

#else // PIXEL

float boxIntersect(float3 rayPos, float3 rayDir, float3 center, float3 hsize) {
	center -= rayPos;
	float3 bMin = (center - hsize) / rayDir;
	float3 bMax = (center + hsize) / rayDir;
	float3 m = min(bMin, bMax);
	return max(0.0, max(m.x, max(m.y, m.z)));
}

#if defined(_GAPI_GXM)
float4 main(VS_OUTPUT In) : COLOR0 {
	float2 pixelCoord = float2(__pixel_x(), __pixel_y());
#else
float4 main(VS_OUTPUT In) : COLOR0 {
	float2 pixelCoord = In.pos.xy;
#endif
	float3 viewVec = normalize(In.viewVec);

	float t = boxIntersect(uViewPos.xyz, -viewVec, uPosScale[0].xyz, uPosScale[1].xyz);

	float3 p0 = uViewPos.xyz - viewVec * t;
	float3 p1 = In.coord.xyz;

	float dither = SAMPLE_2D_POINT_WRAP(sMask, pixelCoord * (1.0 / 8.0)).x;
	
	float3 delta = (p1 - p0) / RAY_STEPS;
	float3 pos	= p0 + delta * dither;

	float sum = 0.0;
	for (float i = 0.0; i < RAY_STEPS; i++) {
		float3 wpos = (pos - uPosScale[0].xyz) / uPosScale[1].xyz;
		float2 tc = wpos.xz * 0.5 + 0.5;
		float light = SAMPLE_2D_LINEAR(sReflect, tc).x;
		sum += light * (1.0 - (clamp(wpos.y, -1.0, 1.0) * 0.5 + 0.5));
		pos += delta;
	}
	sum /= RAY_STEPS;
	sum *= uParam.w;

	return float4(UNDERWATER_COLOR * sum, 1.0);
}

#endif
