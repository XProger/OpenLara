#include "common.hlsl"

struct VS_OUTPUT {
	float4 pos      : POSITION;
	float2 texCoord : TEXCOORD0;
#ifdef UPSCALE
	float4 diffuse  : COLOR0;
#endif
};

#ifdef VERTEX
VS_OUTPUT main(VS_INPUT In) {
	VS_OUTPUT Out;
	Out.pos      = float4(In.aCoord.xy * (1.0 / 32767.0), 0.0, 1.0);
	Out.texCoord = In.aTexCoord.xy * (1.0 / 32767.0);
	#ifdef UPSCALE
		Out.diffuse = In.aLight;
	#endif
	
    #ifndef _GAPI_GXM
        #ifdef DOWNSAMPLE
            Out.texCoord += float2(2.0, -2.0) * uParam.x;
        #elif BLUR
            Out.texCoord += float2(1.0, -1.0) * uParam.z;
        #endif
    #endif
	
	return Out;
}

#else // PIXEL

float4 downsample(float2 uv) { // uParam (1 / textureSize, unused, unused, unused)
	float4 color = 0.0;

	for (float y = -1.5; y < 2.0; y++) {
		for (float x = -1.5; x < 2.0; x++) {
			float4 p;
			p.xyz  = SAMPLE_2D_LINEAR(sDiffuse, uv + float2(x, y) * uParam.x).xyz;
			p.w    = dot(p.xyz, float3(0.299, 0.587, 0.114));
			p.xyz *= p.w;
			color += p;
		}
	}

	return float4(color.xyz / color.w, 1.0);
}

float4 grayscale(float2 uv) { // uParam (factor, unused, unused, unused)
	float4 color = SAMPLE_2D_POINT(sDiffuse, uv);
	float3 gray  = dot(color, float4(0.299, 0.587, 0.114, 0.0));
	return float4(lerp(color.xyz, gray, uParam.w) * uParam.xyz, color.w);
}

float4 blur(float2 uv) { // uParam (dirX, dirY, 1 / textureSize, unused)
	const float3 offset = float3(         0.0, 1.3846153846, 3.2307692308);
	const float3 weight = float3(0.2270270270, 0.3162162162, 0.0702702703);

	float2 dir = uParam.xy;
	float4 color = SAMPLE_2D_LINEAR(sDiffuse, uv) * weight[0];
	color += SAMPLE_2D_LINEAR(sDiffuse, uv + dir * offset[1]) * weight[1];
	color += SAMPLE_2D_LINEAR(sDiffuse, uv - dir * offset[1]) * weight[1];
	color += SAMPLE_2D_LINEAR(sDiffuse, uv + dir * offset[2]) * weight[2];
	color += SAMPLE_2D_LINEAR(sDiffuse, uv - dir * offset[2]) * weight[2];
	return color;
}

float4 upscale(float2 uv) { // uParam (1 / textureWidth, 1 / textureHeight, unused, unused)
    uv *= uParam.xy + 0.5;
    float2 iuv = floor(uv);
    float2 fuv = frac(uv);
    uv = iuv + fuv * fuv * (3.0 - 2.0 * fuv);
    uv = (uv - 0.5) / uParam.xy;
    return SAMPLE_2D_LINEAR(sDiffuse, uv);
}

float4 main(VS_OUTPUT In) : COLOR0 {
	#ifdef DOWNSAMPLE
		return downsample(In.texCoord.xy);
	#elif GRAYSCALE
		return grayscale(In.texCoord.xy);
	#elif BLUR
		return blur(In.texCoord.xy);
	#elif UPSCALE
		return upscale(In.texCoord.xy) * In.diffuse;
	#else
		#error unsupported filter type
	#endif
}
#endif