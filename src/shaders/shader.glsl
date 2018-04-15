R"====(
#ifdef GL_ES
	precision lowp		int;
	precision highp		float;
#endif

#if defined(PASS_COMPOSE) && !defined(TYPE_FLASH)
	varying vec3 vCoord;
#endif

varying vec4 vTexCoord; // xy - atlas coords, zw - trapezoidal correction

#ifdef OPT_CAUSTICS
	uniform vec4 uRoomSize; // xy - minXZ, zw - maxXZ
#endif

uniform mat4 uLightProj[SHADOW_OBJ_MAX];
uniform mat4 uViewProj;
uniform vec3 uViewPos;
uniform vec4 uParam;	// x - time, y - water height, z - clip plane sign, w - clip plane height
uniform vec4 uLightPos[MAX_LIGHTS];
uniform vec4 uLightColor[MAX_LIGHTS]; // xyz - color, w - radius * intensity
uniform vec4 uFogParams;

uniform vec4 uMaterial;	// x - diffuse, y - ambient, z - specular, w - alpha

#ifndef PASS_SHADOW
	varying vec4 vViewVec;  // xyz - dir * dist, w - coord.y
	varying vec4 vDiffuse;
	varying vec4 vNormal;	// xyz - normal dir, w - blend mode (0 - alpha, 1 - additive blend)

	#ifndef TYPE_FLASH
		#ifdef PASS_COMPOSE
			varying vec4 vLightVec;		// xyz - dir, w - fog factor

			#ifdef OPT_SHADOW
				varying vec3 vAmbient;
				varying vec4 vLightMap;
			#endif
		#endif

		varying vec4 vLight;	// lights intensity (MAX_LIGHTS == 4)
	#endif
#endif

#ifdef VERTEX

	#if defined(TYPE_ENTITY) || defined(TYPE_MIRROR)
		uniform vec4 uBasis[32 * 2];
	#else
		uniform vec4 uBasis[2];
	#endif

	#ifdef OPT_AMBIENT
		uniform vec3 uAmbient[6];
	
		vec3 calcAmbient(vec3 n) {
			vec3 sqr = n * n;
			vec3 pos = step(0.0, n);
			return	sqr.x * mix(uAmbient[1], uAmbient[0], pos.x) +
					sqr.y * mix(uAmbient[3], uAmbient[2], pos.y) +
					sqr.z * mix(uAmbient[5], uAmbient[4], pos.z);
		}
	#endif

	attribute vec4 aCoord;
	attribute vec4 aTexCoord;
	attribute vec4 aNormal;

	#ifndef PASS_SHADOW
		attribute vec4 aColor;
		attribute vec4 aLight;
	#endif

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

		vec4 coord;
		coord.w = rBasisPos.w; // visible flag
		#if defined(TYPE_SPRITE) && defined(ALIGN_SPRITES)
			coord.xyz = mulBasis(rBasisRot, rBasisPos.xyz + aCoord.xyz, vec3(aTexCoord.z, aTexCoord.w, 0.0) * 32767.0);
		#else
			coord.xyz = mulBasis(rBasisRot, rBasisPos.xyz, aCoord.xyz);
		#endif

		#ifndef PASS_SHADOW
			vViewVec = vec4((uViewPos - coord.xyz) * uFogParams.w, coord.y);
		#endif

		#ifdef PASS_AMBIENT
			vNormal = aNormal;
		#endif

		#if defined(PASS_COMPOSE) && !defined(TYPE_FLASH)
			#ifdef TYPE_SPRITE
				vNormal.xyz = normalize(vViewVec.xyz);
			#else
				vNormal.xyz = normalize(mulQuat(rBasisRot, aNormal.xyz));
			#endif
			vNormal.w = aNormal.w;

			float fog;
			#ifdef UNDERWATER
				float d;
				if (uViewPos.y < uParam.y)
					d = abs((coord.y - uParam.y) / normalize(vViewVec.xyz).y);
				else
					d = length(uViewPos - coord.xyz);
				fog = d * WATER_FOG_DIST;
			#else
				fog = length(vViewVec.xyz);
			#endif

			vLightVec.w = clamp(1.0 / exp(fog), 0.0, 1.0);
		#endif

		#if defined(PASS_COMPOSE) && !defined(TYPE_FLASH)
			vCoord = coord.xyz;
		#endif
		return coord;
	}

	void _diffuse() {
		#ifndef PASS_SHADOW
			vDiffuse = vec4(aColor.xyz * (uMaterial.x * 1.8), uMaterial.w);

			#ifdef UNDERWATER
				vDiffuse.xyz *= UNDERWATER_COLOR;
			#endif

			#ifdef TYPE_MIRROR
				vDiffuse.xyz = uMaterial.xyz;
			#endif

			#ifdef TYPE_FLASH
				vDiffuse.xyz += uMaterial.w;
			#endif
		#endif
	}

	void _lighting(vec3 coord) {
		#ifndef TYPE_FLASH
			#ifdef PASS_COMPOSE
				vec3 lv0 = (uLightPos[0].xyz - coord) * uLightColor[0].w;
				vec3 lv1 = (uLightPos[1].xyz - coord) * uLightColor[1].w;
				vec3 lv2 = (uLightPos[2].xyz - coord) * uLightColor[2].w;
				vec3 lv3 = (uLightPos[3].xyz - coord) * uLightColor[3].w;

				vLightVec.xyz = lv0;

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

				#ifdef UNDERWATER
					light.x *= abs(sin(dot(coord.xyz, vec3(1.0 / 512.0)) + uParam.x)) * 1.5 + 0.5;
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
					vLightMap  = aLight * light.x;
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

			#ifdef PASS_AMBIENT
				vLight = vec4(aLight.xyz, 1.0);
			#endif
		#endif
	}

	void _uv(vec3 coord) {
		vTexCoord = aTexCoord;
		#if defined(PASS_COMPOSE) && !defined(TYPE_SPRITE)
			vTexCoord.xy *= vTexCoord.zw;
		#endif
	}

	void main() {
		vec4 coord = _transform();

		#ifndef PASS_SHADOW
			_diffuse();
			_lighting(coord.xyz);
		#endif

		_uv(coord.xyz);

		gl_Position = uViewProj * coord;
	}
#else
	uniform sampler2D sDiffuse;

	#if defined(PASS_COMPOSE) && defined(TYPE_MIRROR)
		uniform samplerCube sEnvironment;
	#endif

	#if defined(PASS_SHADOW) && defined(SHADOW_COLOR)
		vec4 pack(in float value) {
			vec4 bitSh = vec4(256.0*256.0*256.0, 256.0*256.0, 256.0, 1.0);
			vec4 bitMsk = vec4(0.0, 1.0/256.0, 1.0/256.0, 1.0/256.0);
			vec4 res = fract(value * bitSh);
			res -= res.xxyz * bitMsk;
			return res;
		}
	#endif

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
			#define CMP(a,b) step(min(1.0, b), a)

			#ifdef SHADOW_DEPTH
				#define compare(p, z) CMP(texture2D(sShadow, (p)).x, (z));
			#elif defined(SHADOW_COLOR)
				float unpack(vec4 value) {
					vec4 bitSh = vec4(1.0/(256.0*256.0*256.0), 1.0/(256.0*256.0), 1.0/256.0, 1.0);
					return dot(value, bitSh);
				}
				#define compare(p, z) CMP(unpack(texture2D(sShadow, (p))), (z));
			#endif

			float SHADOW(vec3 p) {
				return compare(p.xy, p.z);
			}
		#endif

		float random(vec3 seed, float freq) {
			float dt = dot(floor(seed * freq), vec3(53.1215, 21.1352, 9.1322));
			return fract(sin(dt) * 2105.2354);
		}

		float randomAngle(vec3 seed, float freq) {
			return random(seed, freq) * 6.283285;
		}

		vec3 rotate(vec2 sc, vec2 v) {
			return vec3(v.x * sc.y + v.y * sc.x, v.x * -sc.x + v.y * sc.y, 0.0);
		}

		float getShadow(vec4 lightProj, vec2 tileOffset) {
			vec3 p = lightProj.xyz / lightProj.w;
			float vis = lightProj.w;
			#ifdef TYPE_ROOM
				vis = min(vis, dot(vNormal.xyz, vLightVec.xyz));
			#endif
			if (vis < 0.0 || p.x < 0.0 || p.y < 0.0 || p.x > 1.0 || p.y > 1.0) return 1.0;

			p.xy = p.xy * vec2(0.25, 0.5) + tileOffset;

			float rShadow =(SHADOW(SHADOW_TEXEL * vec3(-0.5, -0.5, 0.0) + p) +  
							SHADOW(SHADOW_TEXEL * vec3( 0.5, -0.5, 0.0) + p) +  
							SHADOW(SHADOW_TEXEL * vec3(-0.5,  0.5, 0.0) + p) +  
							SHADOW(SHADOW_TEXEL * vec3( 0.5,  0.5, 0.0) + p)) * 0.25;

			float fade = clamp(dot(vLightVec.xyz, vLightVec.xyz), 0.0, 1.0);
			return rShadow + (1.0 - rShadow) * fade;
		}

		float getShadow() { // hardcoded for 4x2 shadow atlas
			vec4 c = vec4(vCoord, 1.0);
			return min(min(min(min(min(min(min(
				getShadow(uLightProj[0] * c, vec2(0.00, 0.0)),
				getShadow(uLightProj[1] * c, vec2(0.25, 0.0))),
				getShadow(uLightProj[2] * c, vec2(0.50, 0.0))),
				getShadow(uLightProj[3] * c, vec2(0.75, 0.0))),
				getShadow(uLightProj[4] * c, vec2(0.00, 0.5))),
				getShadow(uLightProj[5] * c, vec2(0.25, 0.5))),
				getShadow(uLightProj[6] * c, vec2(0.50, 0.5))),
				getShadow(uLightProj[7] * c, vec2(0.75, 0.5)));
		}
	#endif

	#ifdef OPT_CAUSTICS
		uniform sampler2D sReflect;

		float calcCaustics(vec3 n) {
			vec2 cc		= clamp((vCoord.xz - uRoomSize.xy) / (uRoomSize.zw - uRoomSize.xy), vec2(0.0), vec2(1.0));
			vec2 border	= vec2(256.0) / (uRoomSize.zw - uRoomSize.xy);
			vec2 fade	= smoothstep(vec2(0.0), border, cc) * (1.0 - smoothstep(vec2(1.0) - border, vec2(1.0), cc));
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
			if (vViewVec.w * uParam.z > uParam.w)
				discard;
		#endif

		vec4 color;
		#ifdef TYPE_MIRROR
			#ifdef PASS_COMPOSE
				vec3 rv = reflect(-normalize(vViewVec.xyz), normalize(vNormal.xyz));
				color = textureCube(sEnvironment, normalize(rv));
			#endif
		#else
			#if defined(PASS_COMPOSE) && !defined(TYPE_SPRITE)
				color = texture2D(sDiffuse, vTexCoord.xy / vTexCoord.zw);
			#else
				color = texture2D(sDiffuse, vTexCoord.xy);
			#endif
		#endif

		#ifdef ALPHA_TEST
//color = vec4(1, 0, 0, 1);
			if (color.w <= 0.5)
				discard;
		#endif

		#ifdef PASS_SHADOW

			#ifdef SHADOW_COLOR
				gl_FragColor = pack(gl_FragCoord.z);
			#else
				gl_FragColor = vec4(1.0);
			#endif

		#else
			color *= vDiffuse;

			#if !defined(TYPE_FLASH) && !defined(TYPE_MIRROR)

				#ifdef PASS_AMBIENT
					color.xyz *= vLight.xyz;
				#endif

				#ifdef PASS_COMPOSE

					vec3 n = normalize(vNormal.xyz);

					#ifdef TYPE_ENTITY
						float rSpecular = uMaterial.z + 0.03;
					#endif

					#ifdef OPT_SHADOW
						vec3 light = uLightColor[1].xyz * vLight.y + uLightColor[2].xyz * vLight.z + uLightColor[3].xyz * vLight.w;

						#if defined(TYPE_ENTITY) || defined(TYPE_ROOM)
							float rShadow = getShadow();
						#endif

						#ifdef TYPE_ENTITY
							rSpecular *= rShadow;
							light += vAmbient + uLightColor[0].xyz * (vLight.x * rShadow);
						#endif

						#ifdef TYPE_ROOM
							light += mix(vAmbient.xyz, vLightMap.xyz, rShadow);
						#endif

						#ifdef TYPE_SPRITE
							light += vLightMap.xyz;
						#endif

					#else
						vec3 light = vLight.xyz;
					#endif

					#ifdef OPT_CAUSTICS
						light += calcCaustics(n);
					#endif

					#ifdef OPT_CONTACT
						light *= getContactAO(vCoord, n) * 0.5 + 0.5;
					#endif

					color.xyz *= light;

					#ifdef TYPE_ENTITY
						color.xyz += calcSpecular(n, vViewVec.xyz, vLightVec.xyz, uLightColor[0], rSpecular);
					#endif

					#ifdef UNDERWATER
						color.xyz = mix(UNDERWATER_COLOR * 0.2, color.xyz, vLightVec.w);
					#else
						color.xyz = mix(uFogParams.xyz, color.xyz, vLightVec.w);
					#endif
				#endif
/* catsuit test
			#elif defined(TYPE_MIRROR)
				color.xyz += calcSpecular(normalize(vNormal.xyz), vViewVec.xyz, vLightVec.xyz, uLightColor[0], 0.4);
				color.w = 1.0;
*/
			#endif

			#ifdef TYPE_PARTICLE_SPRITE
				gl_FragColor = vec4(color.xyz * max(1.0 - vNormal.w, color.w), color.w * vNormal.w); // premultiplied
			#else
				gl_FragColor = color;
			#endif

		#endif
	}
#endif
)===="