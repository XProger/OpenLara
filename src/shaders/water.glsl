R"====(
#ifdef GL_ES
	precision lowp	int;
	precision highp float;
#endif

#define WATER_FOG_DIST		(1.0 / (6.0 * 1024.0))
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

uniform vec4  uViewPos;
uniform mat4  uViewProj;
uniform vec4  uLightPos;
uniform vec4  uPosScale[2];

uniform vec4  uTexParam;
uniform vec4  uParam;

uniform sampler2D sNormal;

#ifdef VERTEX
	attribute vec4 aCoord;

	void main() {
		vec3 coord = aCoord.xyz * (1.0 / 32767.0);

		vTexCoord = (coord.xy * 0.5 + 0.5) * uTexParam.zw;

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

				vec4 info = texture2D(sNormal, (rCoord.xy  * 0.5 + 0.5) * uTexParam.zw);
				vec3 normal = vec3(info.z, info.w, sqrt(1.0 - dot(info.zw, info.zw)));

				vec3 light = vec3(0.0, 0.0, 1.0);
				vec3 refOld = refract(-light, vec3(0.0, 0.0, 1.0), 0.75);
				vec3 refNew = refract(-light, normal, 0.75);

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
		vLightVec = uLightPos.xyz - vCoord.xyz;
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

	vec3 applyFog(vec3 color, vec3 fogColor, float factor) {
		float fog = clamp(1.0 / exp(factor), 0.0, 1.0);
		return mix(fogColor, color, fog);
	}

	vec4 drop() {
		vec4 v = texture2D(sDiffuse, vTexCoord);

		float drop = max(0.0, 1.0 - length(uParam.xy - vTexCoord / uTexParam.xy) / uParam.z);
		drop = 0.5 - cos(drop * PI) * 0.5;
		v.x += drop * uParam.w;

		return v;
	}

	vec3 hash33(vec3 p3) {
		p3 = fract(p3 * vec3(.1031,.11369,.13787));
		p3 += dot(p3, p3.yxz+19.19);
		return -1.0 + 2.0 * fract(vec3((p3.x + p3.y)*p3.z, (p3.x+p3.z)*p3.y, (p3.y+p3.z)*p3.x));
	}

	float simplex_noise(vec3 p) { // https://www.shadertoy.com/view/4sc3z2
		const float K1 = 0.333333333;
		const float K2 = 0.166666667;

		vec3 i = floor(p + (p.x + p.y + p.z) * K1);
		vec3 d0 = p - (i - (i.x + i.y + i.z) * K2);

		vec3 e = step(vec3(0.0), d0 - d0.yzx);
		vec3 i1 = e * (1.0 - e.zxy);
		vec3 i2 = 1.0 - e.zxy * (1.0 - e);

		vec3 d1 = d0 - (i1  - 1.0 * K2);
		vec3 d2 = d0 - (i2  - 2.0 * K2);
		vec3 d3 = d0 - (1.0 - 3.0 * K2);

		vec4 h = max(0.6 - vec4(dot(d0, d0), dot(d1, d1), dot(d2, d2), dot(d3, d3)), 0.0);
		vec4 n = h * h * h * h * vec4(dot(d0, hash33(i)), dot(d1, hash33(i + i1)), dot(d2, hash33(i + i2)), dot(d3, hash33(i + 1.0)));

		return dot(vec4(31.316), n);
	}

	float h(vec2 tc) {
		return simplex_noise(vec3(tc * 16.0, uParam.w)) * 0.0005;
	}

	vec4 simulate() {
		vec2 tc = vTexCoord;

		if (texture2D(sMask, tc).x < 0.5)
			return vec4(0.0);

		vec4 v = texture2D(sDiffuse, tc); // height, speed, normal.xz

		vec3 d = vec3(uTexParam.xy, 0.0);
		vec4 f = vec4(texture2D(sDiffuse, tc + d.xz).x, texture2D(sDiffuse, tc + d.zy).x,
					  texture2D(sDiffuse, tc - d.xz).x, texture2D(sDiffuse, tc - d.zy).x);
		float average = dot(f, vec4(0.25));

	// normal
		v.zw = normalize( vec3(f.x - f.z, 64.0 / (1024.0 * 4.0), f.y - f.w) ).xz;

	// integrate
		const float vel = 1.4;
		const float vis = 0.995;

		v.y += (average - v.x) * vel;
		v.y *= vis;
		v.x += v.y + h(tc);

		return v;
	}

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

	vec4 compose() {
		vec3 viewVec = normalize(vViewVec);

		vec4 value	= texture2D(sNormal, vTexCoord);
		vec3 normal = vec3(value.z, sqrt(1.0 - dot(value.zw, value.zw)) * sign(viewVec.y), value.w);
		vec2 dudv   = (uViewProj * vec4(normal.x, 0.0, normal.z, 0.0)).xy;

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

		float d = abs((vCoord.y - uViewPos.y) / normalize(vViewVec).y);
		d *= step(0.0, uViewPos.y - vCoord.y); // apply fog only when camera is underwater
		color.xyz = applyFog(color.xyz, UNDERWATER_COLOR * 0.2, d * WATER_FOG_DIST);
		color.w *= texture2D(sMask, vTexCoord).x;
		return color;
	}

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
