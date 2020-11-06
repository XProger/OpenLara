#define NORMAL_AS_3D

#include "common.hlsl"

struct VS_OUTPUT {
	float4 pos       : POSITION;
	half4  color     : TEXCOORD0;
	half2  texCoord  : TEXCOORD1;
	float3 coord     : TEXCOORD2;
};

#ifdef VERTEX
	VS_OUTPUT main(VS_INPUT In) {
		VS_OUTPUT Out;
		
		Out.color    = (half4)In.aColor;
		Out.texCoord = (half2)In.aTexCoord.xy;
		Out.coord    = float3(In.aCoord.x, -In.aCoord.y, In.aCoord.z);
		
		Out.pos   = mul(uViewProj, float4(In.aCoord.xyz * 5.0, 1.0));
		Out.pos.z = Out.pos.w;
		
		return Out;
	}
	
#else // PIXEL

	#ifdef SKY_AZURE
		#define SKY_CLOUDS
	#endif

	#ifdef SKY_CLOUDS
		#define STEPS       8.0
		#define MIN_HEIGHT  2.0
		#define MAX_HEIGHT  4.0
		#define skyWind     uParam.xyz
		#define skyDown     float3(uLightProj[0].x, uLightProj[1].x, uLightProj[2].x)
		#define skyUp       float3(uLightProj[0].y, uLightProj[1].y, uLightProj[2].y)
		#define sunDir      float3(uLightProj[0].z, uLightProj[1].z, uLightProj[2].z)
		#define sunSize     uLightProj[3].z
		#define sunColor    float3(uLightProj[0].w, uLightProj[1].w, uLightProj[2].w)
		#define sunGlare    uLightProj[3].w
		#define cloudsDown  uPosScale[0].xyz
		#define cloudsUp    uPosScale[1].xyz

		// based on https://www.shadertoy.com/view/XsVGz3 / https://www.shadertoy.com/view/XslGRr
		float noise3D(float3 p) {
			p = p * 0.15 + skyWind;
			return SAMPLE_3D(sNormal, p).x;
		}

		float density(float3 pos) {
			float den = noise3D(pos) * 3.0 - 2.0 + (pos.y - MIN_HEIGHT);
			float edge = 1.0 - smoothstep(MIN_HEIGHT, MAX_HEIGHT, pos.y);
			den = clamp(den * edge * edge, 0.0, 1.0);
			return den;
		}

		float3 raymarching(float2 screenPos, float3 dir, float t0, float t1, float3 backCol) {
			float dither = SAMPLE_2D_POINT_WRAP(sMask, screenPos * (1.0 / 8.0)).x;

			float3 step = dir * ((t1 - t0) / STEPS);
			float3 pos  = dir * t0 + step * dither;
			float4 sum  = 0.0;

			for (float i = 0.0; i < STEPS; i++) {
				float den = density(pos);

				if (den > 0.01) {
					float dif = max(0.0, den - density(pos + 0.3 * sunDir)) * 4.0;

					float4 col = float4(lerp(cloudsUp, cloudsDown, den), den);
					float3 lin = sunColor * dif + 1.0;

					col.rgb *= lin;

					col.a   *= 0.5;
					col.rgb *= col.a;
					sum = sum + col * (1.0 - sum.a);
				}

				pos += step;
			}

			sum = clamp(sum, 0.0, 1.0);

			float h = dir.y;
			sum.rgb = lerp(sum.rgb, backCol, exp(-20.0 * h * h) );

			return lerp(backCol, sum.xyz, sum.a);
		}
	#endif

	half4 main(VS_OUTPUT In) : COLOR0 {
		float3 dir = normalize(In.coord);

		#ifdef SKY_AZURE
			float3 col = lerp(skyDown, skyUp, dir.y);
		#else
			float3 col = SAMPLE_2D_LINEAR(sDiffuse, In.texCoord).xyz * In.color.xyz;
		#endif

		#ifdef SKY_CLOUDS
			float sun = clamp(sunSize + dot(sunDir, dir), 0.0, 1.0);
			col += sunColor * pow(sun, sunGlare);

			float2 dist = float2(MIN_HEIGHT, MAX_HEIGHT) / dir.y;

			if (dist.x > 0.0) {
				col = raymarching(In.pos.xy, dir, dist.x, dist.y, col);
			}
		#endif

		return half4(col, 1.0h);
	}
#endif
