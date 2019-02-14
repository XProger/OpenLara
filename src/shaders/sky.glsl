R"====(
varying vec4 vColor;
varying vec2 vTexCoord;
varying vec3 vCoord;

uniform vec4 uViewPos;

#ifdef VERTEX
	uniform mat4 uViewProj;

	attribute vec4 aCoord;
	attribute vec4 aColor;
	attribute vec4 aTexCoord;

	void main() {
		vColor    = aColor;
		vTexCoord = aTexCoord.xy;
		vCoord    = vec3(aCoord.x, -aCoord.y, aCoord.z);
		gl_Position = uViewProj * vec4(aCoord.xyz * 5.0, 1.0);
		gl_Position.z = gl_Position.w;
	}
#else
	uniform sampler2D sDiffuse;

	#ifdef SKY_CLOUDS_AZURE
		#define SKY_CLOUDS
		#define SKY_AZURE
	#endif

	#ifdef SKY_CLOUDS
		uniform sampler3D sNormal;
		uniform sampler2D sMask;

		uniform mat4      uLightProj;
		uniform vec4      uPosScale[2];
		uniform vec4      uParam;

		#define STEPS       8.0
		#define MIN_HEIGHT  2.0
		#define MAX_HEIGHT  4.0
		#define skyWind     uParam.xyz
		#define skyDown     uLightProj[0].xyz
		#define skyUp       uLightProj[1].xyz
		#define sunDir      uLightProj[2].xyz
		#define sunSize     uLightProj[2].w
		#define sunColor    uLightProj[3].xyz
		#define sunGlare    uLightProj[3].w
		#define cloudsDown  uPosScale[0].xyz
		#define cloudsUp    uPosScale[1].xyz

		// based on https://www.shadertoy.com/view/XsVGz3 / https://www.shadertoy.com/view/XslGRr
		float noise3D(vec3 p) {
			p = p * 0.15 + skyWind;
			return texture3D(sNormal, p).x;
		}

		float density(vec3 pos) {
			float den = noise3D(pos) * 3.0 - 2.0 + (pos.y - MIN_HEIGHT);
			float edge = 1.0 - smoothstep(MIN_HEIGHT, MAX_HEIGHT, pos.y);
			den = clamp(den * edge * edge, 0.0, 1.0);
			return den;
		}

		vec3 raymarching(vec3 dir, float t0, float t1, vec3 backCol) {
			float dither = texture2D(sMask, gl_FragCoord.xy * (1.0 / 8.0)).x;

			vec3 step = dir * ((t1 - t0) / STEPS);
			vec3 pos  = dir * t0 + step * dither;
			vec4 sum  = vec4(0.0);

			for (float i = 0.0; i < STEPS; i++) {
				float den = density(pos);

				if (den > 0.01) {
					float dif = max(0.0, den - density(pos + 0.3 * sunDir)) * 4.0;

					vec4 col = vec4(mix(cloudsUp, cloudsDown, den), den);
					vec3 lin = sunColor * dif + 1.0;

					col.rgb *= lin;

					col.a   *= 0.5;
					col.rgb *= col.a;
					sum = sum + col * (1.0 - sum.a);
				}

				pos += step;
			}

			sum = clamp(sum, 0.0, 1.0);

			float h = dir.y;
			sum.rgb = mix(sum.rgb, backCol, exp(-20.0 * h * h) );

			return mix(backCol, sum.xyz, sum.a);
		}
	#endif

	void main() {
		vec3 dir = normalize(vCoord);

		#ifdef SKY_AZURE
			vec3 col = mix(skyDown, skyUp, dir.y);
		#else
			vec3 col = texture2D(sDiffuse, vTexCoord).xyz * vColor.xyz;
		#endif

		#ifdef SKY_CLOUDS
			float sun = clamp(sunSize + dot(sunDir, dir), 0.0, 1.0);
			col += sunColor * pow(sun, sunGlare);

			vec2 dist = vec2(MIN_HEIGHT, MAX_HEIGHT) / dir.y;

			if (dist.x > 0.0) {
				col = raymarching(dir, dist.x, dist.y, col);
			}
		#endif

		fragColor = vec4(col, 1.0);
	}
#endif
)===="
