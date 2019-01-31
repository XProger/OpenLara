R"====(
#ifdef GL_ES
	precision lowp	int;
	precision highp float;
#endif

#define MAX_LIGHTS			4
#define WATER_FOG_DIST		(1.0 / (6.0 * 1024.0))
#define WATER_COLOR_DIST	(1.0 / (2.0 * 1024.0))
#define UNDERWATER_COLOR	vec3(0.6, 0.9, 0.9)

varying vec3 vCoord;
varying vec2 vTexCoord;
varying vec4 vProjCoord;
varying vec3 vViewVec;
varying vec3 vLightVec;

#ifdef WATER_CAUSTICS
	varying vec3 vOldPos;
	varying vec3 vNewPos;
#endif

#if defined(WATER_COMPOSE) || defined(WATER_SIMULATE)
	varying vec2 vMaskCoord;
#endif

uniform vec4  uViewPos;
uniform mat4  uViewProj;
uniform vec4  uLightPos[MAX_LIGHTS];
uniform vec4  uPosScale[2];

uniform vec4  uTexParam;
uniform vec4  uParam;
uniform vec4  uRoomSize;

uniform sampler2D sNormal;

vec3 calcNormal(vec2 tc, float base) {
	float dx = texture2D(sNormal, vec2(tc.x + uTexParam.x, tc.y)).x - base;
	float dz = texture2D(sNormal, vec2(tc.x, tc.y + uTexParam.y)).x - base;
	return normalize( vec3(dx, 64.0 / (1024.0 * 8.0), dz) );
}

#ifdef VERTEX
	attribute vec4 aCoord;

	void main() {
		vec3 coord = aCoord.xyz * (1.0 / 32767.0);

		vTexCoord = (coord.xy * 0.5 + 0.5) * uTexParam.zw;

	#if defined(WATER_COMPOSE) || defined(WATER_SIMULATE)
		vMaskCoord = (coord.xy * 0.5 + 0.5) * uRoomSize.zw;
	#endif

		#if defined(WATER_MASK) || defined(WATER_COMPOSE)

			float height = 0.0;

			vCoord = vec3(coord.x, height, coord.y) * uPosScale[1].xyz + uPosScale[0].xyz;

			vec4 cp = uViewProj * vec4(vCoord, 1.0);

			vProjCoord	= cp;
			gl_Position = cp;
		#else
			vProjCoord = vec4(0.0);
			vCoord	   = vec3(coord.xy, 0.0);
			#ifdef WATER_CAUSTICS
				vec3 rCoord = vec3(coord.x, coord.y, 0.0) * uPosScale[1].xzy;

				vec2 tc = (rCoord.xy * 0.5 + 0.5) * uTexParam.zw;
				vec4 info = texture2D(sNormal, tc);
				vec3 normal = calcNormal(tc, info.x).xzy;

				vec3 light = vec3(0.0, 0.0, 1.0);
				vec3 refOld = refract(-light, vec3(0.0, 0.0, 1.0), 0.75);
				vec3 refNew = refract(-light, normalize(normal + vec3(0.0, 0.0, 0.25)), 0.75);

				vOldPos = rCoord + refOld * (-1.0 / refOld.z) + refOld * ((-refOld.z - 1.0) / refOld.z);
				vNewPos = rCoord + refNew * ((info.r - 1.0) / refNew.z) + refOld * ((-refNew.z - 1.0) / refOld.z);

				gl_Position = vec4(vNewPos.xy + refOld.xy / refOld.z, 0.0, 1.0);
			#else
				gl_Position = vec4(coord.xyz, 1.0);
			#endif

			#ifdef WATER_RAYS
				vCoord = aCoord.xyz + uParam.xyz;
				vec4 cp = uViewProj * vec4(vCoord, 1.0);
				vProjCoord	= cp;
				gl_Position = cp;
			#endif
		#endif
		vViewVec  = uViewPos.xyz - vCoord.xyz;
		vLightVec = uLightPos[0].xyz - vCoord.xyz;

		#ifdef WATER_COMPOSE
			vViewVec.y  = abs(vViewVec.y);
			vLightVec.y = abs(vLightVec.y);
		#endif
	}
#else
	uniform sampler2D sDiffuse;
	uniform sampler2D sReflect;
	uniform sampler2D sMask;

	uniform vec4 uLightColor;

	#define PI	 3.141592653589793

	float calcFresnel(float VoH, float f0) {
		float f = pow(1.0 - VoH, 5.0);
		return f + f0 * (1.0 - f);
	}

#ifdef WATER_DROP
	vec4 drop() {
		vec2 v = texture2D(sNormal, vTexCoord).xy;

		float drop = max(0.0, 1.0 - length(uParam.xy - vTexCoord / uTexParam.xy) / uParam.z);
		drop = 0.5 - cos(drop * PI) * 0.5;
		v.x += drop * uParam.w;

		return vec4(v, 0.0, 0.0);
	}
#endif

#ifdef WATER_SIMULATE
	float noise3D(vec3 x) { // https://www.shadertoy.com/view/XslGRr
		vec3 p = floor(x);
		vec3 f = fract(x);
		f = f * f * (3.0 - 2.0 * f);
		vec2 uv = (p.xy + vec2(37.0, 17.0) * p.z) + f.xy;
		vec2 rg = texture2D(sDiffuse, (uv + 0.5) / 32.0).yx;
		return mix(rg.x, rg.y, f.z) * 2.0 - 1.0;
	}

	vec4 simulate() {
		vec2 tc = vTexCoord;

		vec2 v = texture2D(sNormal, tc).xy; // height, speed

		vec3 d = vec3(uTexParam.xy, 0.0);
		vec4 f = vec4(texture2D(sNormal, tc + d.xz).x, texture2D(sNormal, tc + d.zy).x,
					  texture2D(sNormal, tc - d.xz).x, texture2D(sNormal, tc - d.zy).x);
		float average = dot(f, vec4(0.25));

	// integrate
		const float vel = 1.4;
		const float vis = 0.995;
		v.y += (average - v.x) * vel;
		v.y *= vis;
		v.x += v.y + noise3D(vec3(tc * 32.0, uParam.w)) * 0.00025;
		v *= texture2D(sMask, vMaskCoord).a;

		return vec4(v.xy, 0.0, 0.0);
	}
#endif

#ifdef WATER_CAUSTICS
	vec4 caustics() {
		float rOldArea = length(dFdx(vOldPos)) * length(dFdy(vOldPos));
		float rNewArea = length(dFdx(vNewPos)) * length(dFdy(vNewPos));
		rNewArea = max(rNewArea, 0.00002); // WebGL NVIDIA workaround >_<
		float value = clamp(rOldArea / rNewArea * 0.2, 0.0, 1.0);
		return vec4(value, 0.0, 0.0, 0.0);
	}
#endif

#ifdef WATER_RAYS
	float boxIntersect(vec3 rayPos, vec3 rayDir, vec3 center, vec3 hsize) {
		center -= rayPos;
		vec3 bMin = (center - hsize) / rayDir;
		vec3 bMax = (center + hsize) / rayDir;
		vec3 m = min(bMin, bMax);
		return max(0.0, max(m.x, max(m.y, m.z)));
	}

	vec4 rays() {
		#define RAY_STEPS 16.0

		vec3 viewVec = normalize(vViewVec);

		float t = boxIntersect(uViewPos.xyz, -viewVec, uPosScale[0].xyz, uPosScale[1].xyz);

		vec3 p0 = uViewPos.xyz - viewVec * t;
		vec3 p1 = vCoord.xyz;

		float dither = texture2D(sMask, gl_FragCoord.xy * (1.0 / 8.0)).x;
		vec3 step = (p1 - p0) / RAY_STEPS;
		vec3 pos  = p0 + step * dither;

		float sum = 0.0;
		for (float i = 0.0; i < RAY_STEPS; i++) {
			vec3 wpos = (pos - uPosScale[0].xyz) / uPosScale[1].xyz;
			vec2 tc = wpos.xz * 0.5 + 0.5;
			float light = texture2D(sReflect, tc).x;
			sum += light * (1.0 - (clamp(wpos.y, -1.0, 1.0) * 0.5 + 0.5));
			pos += step;
		}
		sum /= RAY_STEPS;
		sum *= uParam.w;

		return vec4(UNDERWATER_COLOR * sum, 1.0);
	}
#endif

	vec4 mask() {
		return vec4(0.0);
	}

	void applyFog(inout vec3 color, float dist) {
		dist *= step(vCoord.y, uViewPos.y);
		color.xyz *= mix(vec3(1.0), UNDERWATER_COLOR, clamp(dist * WATER_COLOR_DIST, 0.0, 2.0));

		float fog = clamp(1.0 / exp(dist * WATER_FOG_DIST), 0.0, 1.0);
		color.xyz = mix(UNDERWATER_COLOR * 0.2, color.xyz, fog);
	}

#ifdef WATER_COMPOSE
	vec4 compose() {
		vec3 viewVec = normalize(vViewVec);

		vec2 value = texture2D(sNormal, vTexCoord).xy;
		vec3 normal = calcNormal(vTexCoord, value.x);

		vec2 dudv = (uViewProj * vec4(normal.x, 0.0, normal.z, 0.0)).xy;

		vec3 rv = reflect(-viewVec, normal);
		vec3 lv = normalize(vLightVec);

		float spec = pow(max(0.0, dot(rv, lv)), 64.0) * 0.5;

		vec2 tc = vProjCoord.xy / vProjCoord.w * 0.5 + 0.5;

		vec4 refrA = texture2D(sDiffuse, uParam.xy * clamp(tc + dudv * uParam.z, 0.0, 0.999) );
		vec4 refrB = texture2D(sDiffuse, uParam.xy * tc );
		vec4 refr  = vec4(mix(refrA.xyz, refrB.xyz, refrA.w), 1.0);
		vec4 refl  = texture2D(sReflect, vec2(tc.x, 1.0 - tc.y) + dudv * uParam.w);

		float fresnel = calcFresnel(max(0.0, dot(normal, viewVec)), 0.12);

		vec4 color = mix(refr, refl, fresnel) + spec * 1.5;
		color.w *= texture2D(sMask, vMaskCoord).a;
		applyFog(color.xyz, vViewVec.y / viewVec.y);

		return color;
	}
#endif

	vec4 pass() {
		#ifdef WATER_DROP
			return drop();
		#endif

		#ifdef WATER_SIMULATE
			return simulate();
		#endif

		#ifdef WATER_CAUSTICS
			return caustics();
		#endif

		#ifdef WATER_RAYS
			return rays();
		#endif

		#ifdef WATER_MASK
			return mask();
		#endif

		#ifdef WATER_COMPOSE
			return compose();
		#endif

		return vec4(1.0, 0.0, 1.0, 1.0);
	}

	void main() {
		gl_FragColor = pass();
	}
#endif
)===="
