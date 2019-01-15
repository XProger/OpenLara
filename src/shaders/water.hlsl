#include "common.hlsl"

struct VS_OUTPUT {
	float4 pos      : POSITION;
	float3 coord    : TEXCOORD0;
	float2 texCoord : TEXCOORD1;
	float3 viewVec  : TEXCOORD2;
	float3 lightVec : TEXCOORD3;
	float3 oldPos   : TEXCOORD4;
	float3 newPos   : TEXCOORD5;
	float4 hpos     : TEXCOORD6;
};

float2 invUV(float2 uv) {
	return float2(uv.x, 1.0 - uv.y);
}

float3 getInvUV(float2 uv, float4 param) {
	float2 p = (float2(uv.x, -uv.y) * 0.5 + 0.5) * param.zw;
#ifndef _GAPI_GXM
	p.xy += 0.5 * param.xy;
#endif
	return float3(p, 0.0);
}

float2 getUV(float2 uv, float4 param) {
	float2 p = (uv.xy * 0.5 + 0.5) * param.zw;
#ifndef _GAPI_GXM
	//p.xy += 0.5 * param.xy;
#endif
	return p;
}

#ifdef VERTEX
VS_OUTPUT main(VS_INPUT In) {
	VS_OUTPUT Out;

	Out.pos      = 0.0;
	Out.coord    = 0.0;
	Out.viewVec  = 0.0;
	Out.lightVec = 0.0;
	Out.oldPos   = 0.0;
	Out.newPos   = 0.0;
	Out.hpos     = 0.0;
	
	float3 coord = In.aCoord.xyz * (1.0 / 32767.0);

	Out.texCoord = getUV(coord.xy, uTexParam);

	if (WATER_COMPOSE) {
		Out.coord    = float3(coord.x, 0.0, coord.y) * uPosScale[1].xyz + uPosScale[0].xyz;
		Out.pos      = mul(uViewProj, float4(Out.coord, 1.0));
		Out.viewVec  = uViewPos.xyz - Out.coord.xyz;
		Out.lightVec = uLightPos[0].xyz - Out.coord.xyz;
		Out.oldPos   = getInvUV(coord.xy, uTexParam);
		Out.texCoord = getUV(coord.xy, uRoomSize);
	} else if (WATER_DROP) {
		Out.pos    = float4(coord.xyz, 1.0);
		Out.oldPos = getInvUV(coord.xy, uTexParam);
	} else if (WATER_SIMULATE) {
		Out.pos      = float4(coord.xyz, 1.0);
		Out.oldPos   = getInvUV(coord.xy, uTexParam);
		Out.texCoord = getUV(coord.xy, uRoomSize);
	} else if (WATER_CAUSTICS) {
		float3 rCoord = float3(coord.x, coord.y, 0.0) * uPosScale[1].xzy;
		float2 uv     = getInvUV(rCoord.xy, uTexParam).xy;
		float4 info	  = tex2Dlod(sNormal, float4(uv, 0, 0));
		float3 normal = float3(info.z, info.w, sqrt(1.0 - dot(info.zw, info.zw)));

		float3 light  = float3(0.0, 0.0, 1.0);
		float3 refOld = refract(-light, float3(0.0, 0.0, 1.0), 0.75);
		float3 refNew = refract(-light, normal, 0.75);

		Out.oldPos = rCoord + refOld * (-1.0 / refOld.z) + refOld * ((-refOld.z - 1.0) / refOld.z);
		Out.newPos = rCoord + refNew * ((info.r - 1.0) / refNew.z) + refOld * ((-refNew.z - 1.0) / refOld.z);

		Out.pos = float4(Out.newPos.xy + refOld.xy / refOld.z, 0.0, 1.0);
	} else if (WATER_MASK) {
		Out.coord = float3(coord.x, 0.0, coord.y) * uPosScale[1].xyz + uPosScale[0].xyz;
		Out.pos   = mul(uViewProj, float4(Out.coord, 1.0));
	} else if (WATER_RAYS) {
        Out.coord   = In.aCoord.xyz + uParam.xyz;
		Out.viewVec = uViewPos.xyz - Out.coord.xyz;
        Out.pos	    = mul(uViewProj, float4(Out.coord, 1.0));
    }

	Out.hpos = Out.pos;
	
	return Out;
}

#else // PIXEL

float calcFresnel(float VoH, float f0) {
	float f = pow(1.0 - VoH, 5.0);
	return f + f0 * (1.0f - f);
}

float4 drop(VS_OUTPUT In) {
	float2 iuv = In.oldPos.xy;
	
	float4 v = tex2D(sNormal, iuv);

	float value = max(0.0, 1.0 - length(uParam.xy - In.texCoord / uTexParam.xy) / uParam.z);
	value = 0.5 - cos(value * PI) * 0.5;
	
	v.x += value * uParam.w;

	return v;
}

float4 simulate(VS_OUTPUT In) {
	float2 iuv = In.oldPos.xy;
	float2 uv  = In.texCoord;
	
	if (tex2D(sMask, uv).a < 0.5)
		return 0.0;

	float4 v = tex2D(sNormal, iuv); // height, speed, normal.xz

	float3 d = float3(float2(uTexParam.x, -uTexParam.y), 0.0);
	float4 f = float4(
			tex2D(sNormal, iuv + d.xz).x, tex2D(sNormal, iuv + d.zy).x,
			tex2D(sNormal, iuv - d.xz).x, tex2D(sNormal, iuv - d.zy).x
		);

	float average = dot(f, (float4)0.25);

// normal
	v.zw = normalize( float3(f.x - f.z, 64.0 / (1024.0 * 4.0), f.y - f.w) ).xz;

// integrate
	const float vel = 1.4;
	const float vis = 0.995;

	v.y += (average - v.x) * vel;
	v.y *= vis;
	v.x += v.y + (tex2D(sDiffuse, uv + uParam.zw).x * 2.0 - 1.0) * 0.00025;

	return v;
}

float4 caustics(VS_OUTPUT In) {
	float rOldArea = length(ddx(In.oldPos)) * length(ddy(In.oldPos));
	float rNewArea = length(ddx(In.newPos)) * length(ddy(In.newPos));
	rNewArea = max(rNewArea, 0.00002); // WebGL NVIDIA workaround >_<
	float value = saturate(rOldArea / rNewArea * 0.2);
	return float4(value, 0.0, 0.0, 0.0);
}

float boxIntersect(float3 rayPos, float3 rayDir, float3 center, float3 hsize) {
	center -= rayPos;
	float3 bMin = (center - hsize) / rayDir;
	float3 bMax = (center + hsize) / rayDir;
	float3 m = min(bMin, bMax);
	return max(0.0, max(m.x, max(m.y, m.z)));
}

float4 rays(VS_OUTPUT In, float2 pixelCoord) {
	#define RAY_STEPS 16.0
	
	float3 viewVec = normalize(In.viewVec);

	float t = boxIntersect(uViewPos.xyz, -viewVec, uPosScale[0].xyz, uPosScale[1].xyz);

	float3 p0 = uViewPos.xyz - viewVec * t;
	float3 p1 = In.coord.xyz;

	float dither = tex2Dlod(sMask, float4(pixelCoord * (1.0 / 8.0), 0, 0)).x;
	
	float3 delta = (p1 - p0) / RAY_STEPS;
	float3 pos	= p0 + delta * dither;

	float sum = 0.0;
	for (float i = 0.0; i < RAY_STEPS; i++) {
		float3 wpos = (pos - uPosScale[0].xyz) / uPosScale[1].xyz;
		float2 tc = wpos.xz * 0.5 + 0.5;
		float light = tex2Dlod(sReflect, float4(tc, 0, 0)).x;
		sum += light * (1.0 - (clamp(wpos.y, -1.0, 1.0) * 0.5 + 0.5));
		pos += delta;
	}
	sum /= RAY_STEPS;
	sum *= uParam.w;

	return float4(UNDERWATER_COLOR * sum, 1.0);
}

float4 mask() {
	return 0.0;
}

float4 compose(VS_OUTPUT In) {
	float3 viewVec = normalize(In.viewVec);
	float2 iuv = In.oldPos.xy;
	
	float4 value  = tex2D(sNormal, iuv);
	float3 normal = float3(value.z, sqrt(1.0 - dot(value.zw, value.zw)) * sign(viewVec.y), value.w);	
	float2 dudv	  = mul(uViewProj, float4(normal.x, 0.0, normal.z, 0.0)).xy;

	float3 rv = reflect(-viewVec, normal);
	float3 lv = normalize(In.lightVec);

	float spec = pow(max(0.0, dot(rv, lv)), 64.0) * 0.5;

	float2 tc = In.hpos.xy / In.hpos.w * 0.5 + 0.5;

	float4 refrA = tex2D(sDiffuse, uParam.xy * invUV(clamp(tc + dudv * uParam.z, 0.0, 0.999)) );
	float4 refrB = tex2D(sDiffuse, uParam.xy * invUV(tc) );
	float4 refr	 = float4(lerp(refrA.xyz, refrB.xyz, refrA.w), 1.0);
	float4 refl	 = tex2D(sReflect, tc.xy + dudv * uParam.w);

	float fresnel = calcFresnel(max(0.0, dot(normal, viewVec)), 0.12);

	float4 color = lerp(refr, refl, fresnel) + spec * 1.5;
	color.w *= tex2D(sMask, In.texCoord).a;
	
	float dist = In.viewVec.y / viewVec.y;
	dist *= step(In.coord.y, uViewPos.y);
	color.xyz *= lerp(float3(1.0, 1.0, 1.0), UNDERWATER_COLOR, clamp(dist * WATER_COLOR_DIST, 0.0, 2.0));
	float fog = saturate(1.0 / exp(dist * WATER_FOG_DIST));
	color.xyz = lerp(UNDERWATER_COLOR * 0.2, color.xyz, fog);
	return color;
}

#ifdef _GAPI_GXM
float4 main(VS_OUTPUT In) : COLOR0 {
	float2 pixelCoord = float2(__pixel_x(), __pixel_y());
#else
float4 main(VS_OUTPUT In, float4 pixelCoord: VPOS) : COLOR0 {
#endif
	if (WATER_DROP)
		return drop(In);

	if (WATER_SIMULATE)
		return simulate(In);

	if (WATER_CAUSTICS)
		return caustics(In);
		
	if (WATER_RAYS)
		return rays(In, pixelCoord.xy);

	if (WATER_MASK)
		return mask();

	if (WATER_COMPOSE)
		return compose(In);

	return float4(1.0, 0.0, 1.0, 1.0);
}
#endif