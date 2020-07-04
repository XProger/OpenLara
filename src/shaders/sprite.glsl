R"====(
#define MAX_LIGHTS			4
#define MAX_CONTACTS		15
#define WATER_FOG_DIST		(1.0 / (6.0 * 1024.0))
#define WATER_COLOR_DIST	(1.0 / (2.0 * 1024.0))
#define UNDERWATER_COLOR	vec3(0.6, 0.9, 0.9)

#define SHADOW_NORMAL_BIAS	16.0
#define SHADOW_CONST_BIAS	0.05

uniform mat4 uViewProj;
uniform vec4 uViewPos;

uniform vec4 uParam;	// x - time, y - water height, z - clip plane sign, w - clip plane height
uniform vec4 uLightPos[MAX_LIGHTS];
uniform vec4 uLightColor[MAX_LIGHTS]; // xyz - color, w - radius * intensity
uniform vec4 uMaterial;	// x - diffuse, y - ambient, z - specular, w - alpha
uniform vec4 uFogParams;

varying vec4 vViewVec;	// xyz - dir * dist, w - coord.y * clipPlaneSign
varying vec4 vDiffuse;
varying vec3 vCoord;
varying vec4 vNormal;	// xyz - normal dir, w - fog factor
varying vec4 vLight;	// lights intensity (MAX_LIGHTS == 4)

varying vec4 vTexCoord; // xy - atlas coords, zw - trapezoidal correction

#ifdef VERTEX

	uniform vec4 uBasis[2];

	attribute vec4 aCoord;
	attribute vec4 aTexCoord;
	attribute vec4 aNormal;

	attribute vec4 aColor;
	attribute vec4 aLight;

	vec3 mulQuat(vec4 q, vec3 v) {
		return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + v * q.w);
	}

	vec3 mulBasis(vec4 rot, vec3 pos, vec3 v) {
		return mulQuat(rot, v) + pos;
	}

	vec4 _transform() {
		vec4 rBasisRot = uBasis[0];
		vec4 rBasisPos = uBasis[1];

		vec3 coord = mulBasis(rBasisRot, rBasisPos.xyz + aCoord.xyz, vec3(aTexCoord.z, aTexCoord.w, 0.0) * 32767.0);

		vViewVec = vec4((uViewPos.xyz - coord) * uFogParams.w, 0.0);

		vNormal.xyz = normalize(vViewVec.xyz);

		float fog;
		#if defined(UNDERWATER) && !defined(OPT_UNDERWATER_FOG)
			float d = length(uViewPos.xyz - coord);
			if (uViewPos.y < uParam.y) {
				d *= (coord.y - uParam.y) / (coord.y - uViewPos.y);
			}
			fog = d * WATER_FOG_DIST;
			fog *= step(uParam.y, coord.y);
		#else
			fog = length(vViewVec.xyz);
		#endif
		vNormal.w = clamp(1.0 / exp(fog), 0.0, 1.0);

		vCoord = coord;

		return vec4(coord, rBasisPos.w);
	}

	void _diffuse() {
		vDiffuse = vec4(aColor.xyz * uMaterial.x, 1.0);
		vDiffuse.xyz *= 2.0;

		vDiffuse *= uMaterial.w;

		vDiffuse *= aLight.w;
	}

	void _lighting(vec3 coord) {
		vec3 lv0 = (uLightPos[0].xyz - coord) * uLightColor[0].w;
		vec3 lv1 = (uLightPos[1].xyz - coord) * uLightColor[1].w;
		vec3 lv2 = (uLightPos[2].xyz - coord) * uLightColor[2].w;
		vec3 lv3 = (uLightPos[3].xyz - coord) * uLightColor[3].w;

		vec4 lum, att;
		lum.x = uMaterial.y;
		att.x = 0.0;

		lum.y = dot(vNormal.xyz, normalize(lv1)); att.y = dot(lv1, lv1);
		lum.z = dot(vNormal.xyz, normalize(lv2)); att.z = dot(lv2, lv2);
		lum.w = dot(vNormal.xyz, normalize(lv3)); att.w = dot(lv3, lv3);
		vec4 light = max(vec4(0.0), lum) * max(vec4(0.0), vec4(1.0) - att);

		vec3 ambient = min(uMaterial.yyy, aLight.xyz);

		vLight.xyz = uLightColor[1].xyz * light.y + uLightColor[2].xyz * light.z + uLightColor[3].xyz * light.w;
		vLight.w = 0.0;

		vLight.xyz += aLight.xyz * light.x;
	}

	void _uv(vec3 coord) {
		vTexCoord = aTexCoord;
	}

	void main() {
		vec4 coord = _transform();

		_diffuse();
		_lighting(coord.xyz);

		_uv(coord.xyz);

		gl_Position = uViewProj * coord;
	}

#else

	uniform sampler2D sDiffuse;

	void main() {
		vec2 uv = vTexCoord.xy;
		vec4 color = texture2D(sDiffuse, uv);

		#ifdef ALPHA_TEST
			if (color.w <= 0.5)
				discard;
		#endif

		color *= vDiffuse;

		vec3 normal = normalize(vNormal.xyz);

		color.xyz *= vLight.xyz;

		#ifdef UNDERWATER
			float uwSign = 1.0;

			#ifdef OPT_UNDERWATER_FOG
				float dist;
				if (uViewPos.y < uParam.y)
					dist = abs((vCoord.y - uParam.y) / normalize(uViewPos.xyz - vCoord.xyz).y);
				else
					dist = length(uViewPos.xyz - vCoord.xyz);
				float fog = clamp(1.0 / exp(dist * WATER_FOG_DIST * uwSign), 0.0, 1.0);
				dist += vCoord.y - uParam.y;
				color.xyz *= mix(vec3(1.0), UNDERWATER_COLOR, clamp(dist * WATER_COLOR_DIST * uwSign, 0.0, 2.0));
				color.xyz = mix(UNDERWATER_COLOR * 0.2, color.xyz, fog);
			#else
				color.xyz = mix(color.xyz, color.xyz * UNDERWATER_COLOR, uwSign);
				color.xyz = mix(UNDERWATER_COLOR * 0.2, color.xyz, vNormal.w);
			#endif
		#else
			color.xyz = mix(uFogParams.xyz, color.xyz, vNormal.w);
		#endif

		fragColor = color;
	}

#endif
)===="
