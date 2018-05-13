R"====(
#ifdef GL_ES
	precision highp int;
	precision highp float;
#endif

varying vec2 vTexCoord;
varying vec4 vColor;
uniform vec4 uParam;

#ifdef VERTEX
	attribute vec4 aCoord;
	attribute vec4 aTexCoord;
	attribute vec4 aLight;

	void main() {
		vTexCoord	= aTexCoord.xy;
		vColor		= aLight;
		gl_Position	= vec4(aCoord.xy * (1.0 / 32767.0), 0.0, 1.0);
	}
#else
	uniform sampler2D sDiffuse;
	uniform sampler2D sNormal;

	vec4 downsample() { // uParam (textureSize, unused, unused, unused)
		vec4 color = vec4(0.0);
		for (float y = -1.5; y < 2.0; y++)
			for (float x = -1.5; x < 2.0; x++) {
				vec4 p;
				p.xyz  = texture2D(sDiffuse, vTexCoord + vec2(x, y) * uParam.x).xyz;
				p.w    = dot(p.xyz, vec3(0.299, 0.587, 0.114));
				p.xyz *= p.w;
				color += p;
			}

		return vec4(color.xyz / color.w, 1.0);
	}

	vec4 grayscale() { // uParam (factor, unused, unused, unused)
		vec4 color = texture2D(sDiffuse, vTexCoord);
		vec3 gray  = vec3(dot(color, vec4(0.299, 0.587, 0.114, 0.0)));
		return vec4(mix(color.xyz, gray, uParam.x), color.w);
	}

	vec4 blur() { // uParam (dirX, dirY, 1 / textureSize, unused)
		const vec3 offset = vec3(0.0, 1.3846153846, 3.2307692308);
		const vec3 weight = vec3(0.2270270270, 0.3162162162, 0.0702702703);

		vec2 dir   = uParam.xy * uParam.z;
		vec4 color = texture2D(sDiffuse, vTexCoord) * weight[0];
		color += texture2D(sDiffuse, vTexCoord + dir * offset[1]) * weight[1];
		color += texture2D(sDiffuse, vTexCoord - dir * offset[1]) * weight[1];
		color += texture2D(sDiffuse, vTexCoord + dir * offset[2]) * weight[2];
		color += texture2D(sDiffuse, vTexCoord - dir * offset[2]) * weight[2];
		return color;
	}

	#ifdef FILTER_EQUIRECTANGULAR
		uniform samplerCube sEnvironment;

		#define PI 3.14159265358979323846

		vec4 equirectangular() {
			vec2 a = (vTexCoord - 0.5) * vec2(PI * 2.0, PI);
			vec3 v = vec3(sin(a.x) * cos(a.y), -sin(a.y), cos(a.x) * cos(a.y));
			return textureCube(sEnvironment, normalize(v));
		}
	#endif

	vec4 filter() {
		#ifdef FILTER_DOWNSAMPLE
			return downsample();
		#endif

		#ifdef FILTER_GRAYSCALE
			return grayscale();
		#endif

		#ifdef FILTER_BLUR
			return blur();
		#endif

		#ifdef FILTER_EQUIRECTANGULAR
			return equirectangular();
		#endif

		return texture2D(sDiffuse, vTexCoord) * vColor;
	}

	void main() {
		gl_FragColor = filter();
	}
#endif
)===="