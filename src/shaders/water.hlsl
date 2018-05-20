#include "common.hlsl"

struct VS_OUTPUT {
	float4 pos		: POSITION;
	float3 coord	: TEXCOORD0;
	float2 texCoord	: TEXCOORD1;
	float3 viewVec	: TEXCOORD2;
	float3 lightVec	: TEXCOORD3;
	float3 oldPos	: TEXCOORD4;
	float3 newPos	: TEXCOORD5;
	float4 hpos		: TEXCOORD6;
};

float2 invUV(float2 uv) {
	return float2(uv.x, 1.0 - uv.y);
}

float3 getInvUV(float2 uv, float4 param) {
	return float3((float2(uv.x, -uv.y) * 0.5 + 0.5) * param.zw + 0.5 * param.xy, 0.0);
}

#ifdef VERTEX
VS_OUTPUT main(VS_INPUT In) {
	VS_OUTPUT Out;
	
	float3 coord = In.aCoord.xyz * (1.0 / 32767.0);

	if (WATER_COMPOSE) {
		Out.coord    = float3(coord.x, 0.0, coord.y) * uPosScale[1].xyz + uPosScale[0].xyz;
		Out.pos      = mul(uViewProj, float4(Out.coord, 1.0));
		Out.viewVec  = uViewPos.xyz - Out.coord.xyz;
		Out.lightVec = uLightPos[0].xyz - Out.coord.xyz;
		Out.oldPos   = getInvUV(coord.xy, uTexParam);
	} else if (WATER_DROP) {
		Out.pos    = float4(coord.xyz, 1.0);
		Out.oldPos = getInvUV(coord.xy, uTexParam);
	} else if (WATER_STEP) {
		Out.pos    = float4(coord.xyz, 1.0);
		Out.oldPos = getInvUV(coord.xy, uTexParam);
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
	}

	Out.texCoord = (coord.xy * 0.5 + 0.5) * uTexParam.zw + 0.5 * uTexParam.xy; // + half texel offset
	Out.hpos     = Out.pos;
	
	return Out;
}

#else // PIXEL

float calcFresnel(float NdotL, float fbias) {
	float f = 1.0 - NdotL;
	return saturate(fbias + (1.0 - fbias) * (f * f));
}

float3 applyFog(float3 color, float3 fogColor, float factor) {
	float fog = saturate(1.0 / exp(factor));
	return lerp(fogColor, color, fog);
}

float4 drop(VS_OUTPUT In) {
	float2 iuv = In.oldPos.xy;
	
	float4 v = tex2D(sDiffuse, iuv);

	float drop = max(0.0, 1.0 - length(uParam.xy - In.texCoord / uTexParam.xy) / uParam.z);
	drop = 0.5 - cos(drop * PI) * 0.5;
	
	v.x += drop * uParam.w;

	return v;
}

float3 hash33(float3 p3) {
	p3 = frac(p3 * float3(.1031,.11369,.13787));
	p3 += dot(p3, p3.yxz+19.19);
	return -1.0 + 2.0 * frac(float3((p3.x + p3.y)*p3.z, (p3.x+p3.z)*p3.y, (p3.y+p3.z)*p3.x));
}

float simplex_noise(float3 p) { // https://www.shadertoy.com/view/4sc3z2
	const float K1 = 0.333333333;
	const float K2 = 0.166666667;

	float3 i = floor(p + (p.x + p.y + p.z) * K1);
	float3 d0 = p - (i - (i.x + i.y + i.z) * K2);

	float3 e = step((float3)0.0, d0 - d0.yzx);
	float3 i1 = e * (1.0 - e.zxy);
	float3 i2 = 1.0 - e.zxy * (1.0 - e);

	float3 d1 = d0 - (i1  - 1.0 * K2);
	float3 d2 = d0 - (i2  - 2.0 * K2);
	float3 d3 = d0 - (1.0 - 3.0 * K2);

	float4 h = max(0.6 - float4(dot(d0, d0), dot(d1, d1), dot(d2, d2), dot(d3, d3)), 0.0);
	float4 n = h * h * h * h * float4(dot(d0, hash33(i)), dot(d1, hash33(i + i1)), dot(d2, hash33(i + i2)), dot(d3, hash33(i + 1.0)));

	return dot((float4)31.316, n);
}

float h(float2 tc) {
	return simplex_noise(float3(tc * 16.0, uParam.w)) * 0.0005;
}

float4 calc(VS_OUTPUT In) {
	float2 iuv = In.oldPos.xy;
	float2 uv  = In.texCoord;
	
	if (tex2D(sMask, uv).x < 0.5)
		return 0.0;

	float4 v = tex2D(sDiffuse, iuv); // height, speed, normal.xz

	float3 d = float3(float2(uTexParam.x, -uTexParam.y), 0.0);
	float4 f = float4(
			tex2D(sDiffuse, iuv + d.xz).x, tex2D(sDiffuse, iuv + d.zy).x,
			tex2D(sDiffuse, iuv - d.xz).x, tex2D(sDiffuse, iuv - d.zy).x
		);

	float average = dot(f, (float4)0.25);

// normal
	v.zw = normalize( float3(f.x - f.z, 64.0 / (1024.0 * 4.0), f.y - f.w) ).xz;

// integrate
	const float vel = 1.4;
	const float vis = 0.995;

	v.y += (average - v.x) * vel;
	v.y *= vis;
	v.x += v.y + h(uv);

	return v;
}

float4 caustics(VS_OUTPUT In) {
	float rOldArea = length(ddx(In.oldPos)) * length(ddy(In.oldPos));
	float rNewArea = length(ddx(In.newPos)) * length(ddy(In.newPos));
	rNewArea = max(rNewArea, 0.00002); // WebGL NVIDIA workaround >_<
	float value = saturate(rOldArea / rNewArea * 0.2);
	return float4(value, 0.0, 0.0, 0.0);
}

float4 mask(VS_OUTPUT In) {
	return 0.0;
}

float4 compose(VS_OUTPUT In) {
	float3 viewVec = normalize(In.viewVec);
	float2 iuv = In.oldPos.xy;
	
	float4 value  = tex2D(sNormal, iuv);
	float3 normal = float3(value.z, sqrt(1.0 - dot(value.zw, value.zw)) * sign(viewVec.y), value.w);	
	float2 dudv   = mul(uViewProj, float4(normal.x, 0.0, normal.z, 0.0)).xy;

	float3 rv = reflect(-viewVec, normal);
	float3 lv = normalize(In.lightVec);

	float spec = pow(max(0.0, dot(rv, lv)), 64.0) * 0.5;

	float2 tc = In.hpos.xy / In.hpos.w * 0.5 + 0.5;

	float4 refrA = tex2D(sDiffuse, uParam.xy * invUV(clamp(tc + dudv * uParam.z, 0.0, 0.999)) );
	float4 refrB = tex2D(sDiffuse, uParam.xy * invUV(tc) );
	float4 refr  = float4(lerp(refrA.xyz, refrB.xyz, refrA.w), 1.0);
	float4 refl  = tex2D(sReflect, tc.xy + dudv * uParam.w);

	float fresnel = calcFresnel(dot(normal, viewVec), 0.04);

	float4 color = lerp(refr, refl, fresnel) + spec * 1.5;

	float d = abs((In.coord.y - uViewPos.y) / normalize(In.viewVec).y);
	d *= step(0.0, uViewPos.y - In.coord.y); // apply fog only when camera is underwater
	color.xyz = applyFog(color.xyz, UNDERWATER_COLOR * 0.2, d * WATER_FOG_DIST);
	color.w *= tex2D(sMask, In.texCoord).x;
	return color;
}

float4 main(VS_OUTPUT In) : COLOR0 {
	if (WATER_DROP)
		return drop(In);

	if (WATER_STEP)
		return calc(In);

	if (WATER_CAUSTICS)
		return caustics(In);

	if (WATER_MASK)
		return mask(In);

	if (WATER_COMPOSE)
		return compose(In);

	return float4(1.0, 0.0, 1.0, 1.0);
}
#endif