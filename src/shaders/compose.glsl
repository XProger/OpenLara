R"====(
#define MAX_LIGHTS			4
#define MAX_CONTACTS		15
#define WATER_FOG_DIST		(1.0 / (6.0 * 1024.0))
#define WATER_COLOR_DIST	(1.0 / (2.0 * 1024.0))
#define UNDERWATER_COLOR	vec3(0.6, 0.9, 0.9)

#define SHADOW_NORMAL_BIAS	16.0
#define SHADOW_CONST_BIAS	0.05

#ifdef OPT_CAUSTICS
	uniform vec4 uRoomSize; // xy - minXZ, zw - maxXZ
#endif

#ifdef OPT_SHADOW
	#define SHADOW_TEXEL	vec3(1.0 / 1024.0, 1.0 / 1024.0, 0.0)
	uniform mat4 uLightProj;

	#ifdef OPT_VLIGHTPROJ
		varying vec4 vLightProj;
	#endif
#endif

uniform mat4 uViewProj;
uniform vec4 uViewPos;

uniform vec4 uParam;	// x - time, y - water height, z - clip plane sign, w - clip plane height
uniform vec4 uLightPos[MAX_LIGHTS];
uniform vec4 uLightColor[MAX_LIGHTS]; // xyz - color, w - radius * intensity
uniform vec4 uMaterial;	// x - diffuse, y - ambient, z - specular, w - alpha
uniform vec4 uFogParams;

varying vec4 vViewVec;	// xyz - dir * dist, w - coord.y * clipPlaneSign
varying vec4 vDiffuse;

#ifndef TYPE_FLASH
	varying vec3 vCoord;
	varying vec4 vNormal;	// xyz - normal dir, w - fog factor

	#ifdef OPT_SHADOW
		varying vec3 vAmbient;
		varying vec3 vLightMap;
	#endif

	varying vec4 vLight;	// lights intensity (MAX_LIGHTS == 4)
#endif

varying vec4 vTexCoord; // xy - atlas coords, zw - trapezoidal correction

#ifdef OPT_VLIGHTVEC
	varying vec3 vLightVec;
#endif

#ifdef OPT_SHADOW
	vec4 calcLightProj(vec3 coord, vec3 lightVec, vec3 normal) {
		float factor = clamp(1.0 - dot(normalize(lightVec), normal), 0.0, 1.0);
		factor *= SHADOW_NORMAL_BIAS;
		return uLightProj * vec4(coord + normal * factor, 1.0);
	}
#endif

#ifdef VERTEX

	#if defined(TYPE_ENTITY) || defined(TYPE_MIRROR)
		uniform vec4 uBasis[32 * 2];
	#else
		uniform vec4 uBasis[2];
	#endif

	#ifdef OPT_AMBIENT
		uniform vec4 uAmbient[6];
	
		vec3 calcAmbient(vec3 n) {
			vec3 sqr = n * n;
			vec3 pos = step(0.0, n);
			return	sqr.x * mix(uAmbient[1].xyz, uAmbient[0].xyz, pos.x) +
					sqr.y * mix(uAmbient[3].xyz, uAmbient[2].xyz, pos.y) +
					sqr.z * mix(uAmbient[5].xyz, uAmbient[4].xyz, pos.z);
		}
	#endif

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
		#if defined(TYPE_ENTITY) || defined(TYPE_MIRROR)
			int index = int(aCoord.w * 2.0);
			vec4 rBasisRot = uBasis[index];
			vec4 rBasisPos = uBasis[index + 1];
		#else
			vec4 rBasisRot = uBasis[0];
			vec4 rBasisPos = uBasis[1];
		#endif

		vec3 coord =
		#ifdef TYPE_SPRITE
			mulBasis(rBasisRot, rBasisPos.xyz + aCoord.xyz, vec3(aTexCoord.z, aTexCoord.w, 0.0) * 32767.0);
		#else
			mulBasis(rBasisRot, rBasisPos.xyz, aCoord.xyz);
		#endif

		vViewVec = vec4((uViewPos.xyz - coord) * uFogParams.w, coord.y * uParam.z);

		#ifndef TYPE_FLASH
			#ifdef TYPE_SPRITE
				vNormal.xyz = normalize(vViewVec.xyz);
			#else
				vNormal.xyz = normalize(mulQuat(rBasisRot, aNormal.xyz));
			#endif

			float fog;
			#if defined(UNDERWATER) && !defined(OPT_UNDERWATER_FOG)
				float d;
				if (uViewPos.y < uParam.y) // TODO: fix for mediump
					d = abs((coord.y - uParam.y) / normalize(uViewPos.xyz - coord).y);
				else
					d = length(uViewPos.xyz - coord);
				fog = d * WATER_FOG_DIST;
				fog *= step(uParam.y, coord.y);
				vNormal.w = clamp(1.0 / exp(fog), 0.0, 1.0);
			#else
				fog = length(vViewVec.xyz);
    			vNormal.w = clamp(1.0 / exp(fog), 0.0, 1.0);
			#endif

			vCoord = coord;
		#endif
		return vec4(coord, rBasisPos.w);
	}

	void _diffuse() {
		vDiffuse = vec4(aColor.xyz * uMaterial.x, 1.0);
		vDiffuse.xyz *= 2.0;

		#ifdef TYPE_MIRROR
			vDiffuse.xyz = uMaterial.xyz;
		#endif

		#ifdef TYPE_FLASH
			vDiffuse.xyz += uMaterial.w;
		#else
			vDiffuse *= uMaterial.w;
		#endif

		#ifdef TYPE_SPRITE
			vDiffuse *= aLight.w;
		#endif
	}

	void _lighting(vec3 coord) {
		#ifndef TYPE_FLASH
			vec3 lv0 = (uLightPos[0].xyz - coord) * uLightColor[0].w;
			vec3 lv1 = (uLightPos[1].xyz - coord) * uLightColor[1].w;
			vec3 lv2 = (uLightPos[2].xyz - coord) * uLightColor[2].w;
			vec3 lv3 = (uLightPos[3].xyz - coord) * uLightColor[3].w;

			#ifdef OPT_VLIGHTVEC
				vLightVec = lv0;
			#endif

			vec4 lum, att;
			#ifdef TYPE_ENTITY
				lum.x = dot(vNormal.xyz, normalize(lv0));
				att.x = dot(lv0, lv0);
			#else
				lum.x = 1.0;
				att.x = 0.0;

				#ifdef TYPE_SPRITE
					lum.x *= uMaterial.y;
				#endif

			#endif

			lum.y = dot(vNormal.xyz, normalize(lv1)); att.y = dot(lv1, lv1);
			lum.z = dot(vNormal.xyz, normalize(lv2)); att.z = dot(lv2, lv2);
			lum.w = dot(vNormal.xyz, normalize(lv3)); att.w = dot(lv3, lv3);
			vec4 light = max(vec4(0.0), lum) * max(vec4(0.0), vec4(1.0) - att);

			#if (defined(TYPE_ENTITY) || defined(TYPE_ROOM)) && defined(UNDERWATER)
				light.x *= 0.5 + abs(sin(dot(coord.xyz, vec3(1.0 / 1024.0)) + uParam.x)) * 0.75;
			#endif

			vec3 ambient;
			#ifdef TYPE_ENTITY
				#ifdef OPT_AMBIENT
					ambient = calcAmbient(vNormal.xyz);
				#else
					ambient = vec3(uMaterial.y);
				#endif
			#else
				ambient = min(uMaterial.yyy, aLight.xyz);
			#endif

			#ifdef OPT_SHADOW
				vAmbient   = ambient;
				vLight     = light;
				vLightMap  = aLight.xyz * light.x;

				#ifdef OPT_VLIGHTPROJ
					vLightProj = calcLightProj(coord, lv0, vNormal.xyz);
				#endif

			#else
				vLight.xyz = uLightColor[1].xyz * light.y + uLightColor[2].xyz * light.z + uLightColor[3].xyz * light.w;
				vLight.w = 0.0;

				#ifdef TYPE_ENTITY
					vLight.xyz += ambient + uLightColor[0].xyz * light.x;
				#else
					vLight.xyz += aLight.xyz * light.x;
				#endif

			#endif
		#endif
	}

	void _uv(vec3 coord) {
		vTexCoord = aTexCoord;
		#ifndef TYPE_SPRITE
			#ifdef OPT_TRAPEZOID
				vTexCoord.xy *= vTexCoord.zw;
			#endif
		#endif
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

	#ifdef TYPE_MIRROR
		uniform samplerCube sEnvironment;
	#endif

	float unpack(vec4 value) {
		return dot(value, vec4(1.0, 1.0/255.0, 1.0/65025.0, 1.0/16581375.0));
	}

	#ifdef OPT_SHADOW
		#ifdef SHADOW_SAMPLER
			uniform sampler2DShadow sShadow;
			#ifdef GL_ES
				#define SHADOW(V) (shadow2DEXT(sShadow, V))
			#else
				#define SHADOW(V) (shadow2D(sShadow, V).x)
			#endif
		#else
			uniform sampler2D sShadow;

			float SHADOW(vec2 p) {
				#ifdef SHADOW_DEPTH
					return texture2D(sShadow, p).x;
				#else
					return unpack(texture2D(sShadow, p));
				#endif
			}
		#endif

		float getShadow(vec3 lightVec, vec3 normal, vec4 lightProj) {
			vec3 p = lightProj.xyz / lightProj.w;
			p.z -= SHADOW_CONST_BIAS * SHADOW_TEXEL.x;

			float vis = lightProj.w;
			#ifdef TYPE_ROOM
				vis = min(vis, dot(normal, lightVec));
			#endif
			if (min(vis, min(p.x, p.y)) < 0.0 || max(p.x, p.y) > 1.0) return 1.0;

			#ifdef SHADOW_SAMPLER
				float rShadow = SHADOW(p);
			#else
				#ifndef OPT_SHADOW_ONETAP
					vec4 samples = vec4(SHADOW(                  p.xy),
										SHADOW(SHADOW_TEXEL.xz + p.xy),
										SHADOW(SHADOW_TEXEL.zy + p.xy),
										SHADOW(SHADOW_TEXEL.xy + p.xy));

					samples = step(vec4(p.z), samples);

					vec2 f = fract(p.xy / SHADOW_TEXEL.xy);
					samples.xy = mix(samples.xz, samples.yw, f.x);
					float rShadow = mix(samples.x, samples.y, f.y);
				#else
					float rShadow = step(p.z, SHADOW(p.xy));
				#endif
			#endif

			float fade = clamp(dot(lightVec, lightVec), 0.0, 1.0);
			return rShadow + (1.0 - rShadow) * fade;
		}

		float getShadow(vec3 lightVec, vec3 normal) {
			#ifndef OPT_VLIGHTPROJ
				vec4 vLightProj = calcLightProj(vCoord, lightVec, normal);
			#endif
			return getShadow(lightVec, normal, vLightProj);
		}
	#endif

	#ifdef OPT_CAUSTICS
		uniform sampler2D sReflect;

		float calcCaustics(vec3 n) {
			vec2 cc = clamp((vCoord.xz - uRoomSize.xy) / uRoomSize.zw, vec2(0.0), vec2(1.0));
			vec2 border = vec2(256.0) / uRoomSize.zw;
			vec2 fade   = smoothstep(vec2(0.0), border, cc) * (1.0 - smoothstep(vec2(1.0) - border, vec2(1.0), cc));
			return texture2D(sReflect, cc).x * max(0.0, -n.y) * fade.x * fade.y;
		}
	#endif

	#ifdef OPT_CONTACT
		uniform vec4 uContacts[MAX_CONTACTS];
	
		float getContactAO(vec3 p, vec3 n) {
			float res = 1.0;
			for (int i = 0; i < MAX_CONTACTS; i++) {
				vec3  v = uContacts[i].xyz - p;
				float a = uContacts[i].w;
				float o = a * clamp(dot(n, v), 0.0, 1.0) / dot(v, v);
				res *= clamp(1.0 - o, 0.0, 1.0);
			}
			return res;
		}
	#endif

	float calcSpecular(vec3 normal, vec3 viewVec, vec3 lightVec, vec4 color, float intensity) {
		vec3 vv = normalize(viewVec);
		vec3 rv = reflect(-vv, normal);
		vec3 lv = normalize(lightVec);
		return pow(max(0.0, dot(rv, lv)), 8.0) * intensity;
	}

	void main() {
		#ifdef CLIP_PLANE
			if (vViewVec.w > uParam.w)
				discard;
		#endif

		vec2 uv = vTexCoord.xy;
		vec4 color;
		#ifdef TYPE_MIRROR
			vec3 rv = reflect(-normalize(vViewVec.xyz), normalize(vNormal.xyz));
			color = textureCube(sEnvironment, normalize(rv));
		#else
			#ifndef TYPE_SPRITE
				#ifdef OPT_TRAPEZOID
					uv /= vTexCoord.zw;
				#endif
			#endif
			color = texture2D(sDiffuse, uv);
		#endif

		#ifdef ALPHA_TEST
//color = vec4(1, 0, 0, 1);
			if (color.w <= 0.5)
				discard;
		#endif

		color *= vDiffuse;

		#if !defined(TYPE_FLASH) && !defined(TYPE_MIRROR)

			#ifndef OPT_VLIGHTVEC
				vec3 vLightVec = (uLightPos[0].xyz - vCoord) * uLightColor[0].w;
			#endif

			vec3 normal = normalize(vNormal.xyz);

			#ifdef TYPE_ENTITY
				float rSpecular = uMaterial.z;
			#endif

			#ifdef OPT_SHADOW
				vec3 light = uLightColor[1].xyz * vLight.y + uLightColor[2].xyz * vLight.z + uLightColor[3].xyz * vLight.w;

				#if defined(TYPE_ENTITY) || defined(TYPE_ROOM)
					float rShadow = getShadow(vLightVec, normal);
				#endif

				#ifdef TYPE_ENTITY
					rSpecular *= rShadow;
					light += vAmbient + uLightColor[0].xyz * (vLight.x * rShadow);
				#endif

				#ifdef TYPE_ROOM
					light += mix(vAmbient, vLightMap, rShadow);
				#endif

				#ifdef TYPE_SPRITE
					light += vLightMap;
				#endif

			#else
				vec3 light = vLight.xyz;
			#endif

			#ifdef UNDERWATER
				float uwSign = 1.0;
				#ifdef TYPE_ENTITY
					uwSign = step(uParam.y, vCoord.y);
				#endif

				#ifdef OPT_CAUSTICS
					light += calcCaustics(normal) * uwSign;
				#endif
			#endif

			#ifdef OPT_CONTACT
				light *= getContactAO(vCoord, normal) * 0.5 + 0.5;
			#endif

			color.xyz *= light;

			#if defined(TYPE_ENTITY) && defined(OPT_UNDERWATER_FOG)
				float specular = calcSpecular(normal, vViewVec.xyz, vLightVec, uLightColor[0], rSpecular);
				#ifdef UNDERWATER
					specular *= (1.0 - uwSign);
				#endif
				color.xyz += specular;
			#endif

			#ifdef UNDERWATER
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
		#endif

		fragColor = color;
	}

#endif
)===="
