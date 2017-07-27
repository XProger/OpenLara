R"====(
#ifdef GL_ES
	precision lowp   int;
	precision highp  float;
#endif

#ifdef OPT_CONTACT
	varying vec3 vCoord;
#endif

varying vec4 vTexCoord; // xy - atlas coords, zw - trapezoidal correction

#if defined(OPT_WATER) && defined(UNDERWATER)
	varying vec2 vCausticsCoord; // - xy caustics texture coord
#endif

//uniform vec4 data[MAX_RANGES + MAX_OFFSETS + 4 + 4 + 1 + 1 + MAX_LIGHTS + MAX_LIGHTS + 1 + 6 + 1 + 32 * 2];

uniform vec2 uAnimTexRanges[MAX_RANGES];
uniform vec2 uAnimTexOffsets[MAX_OFFSETS];
uniform mat4 uLightProj;
uniform mat4 uViewProj;
uniform vec3 uViewPos;
uniform vec4 uParam;	// x - time, y - water height, z - clip plane sign, w - clip plane height
uniform vec3 uLightPos[MAX_LIGHTS];
uniform vec4 uLightColor[MAX_LIGHTS]; // xyz - color, w - radius * intensity
uniform vec4 uRoomSize; // xy - minXZ, zw - maxXZ
uniform vec3 uAmbient[6];
uniform vec4 uMaterial;	// x - diffuse, y - ambient, z - specular, w - alpha
uniform vec4 uBasis[32 * 2];

#ifndef PASS_SHADOW
	varying vec4 vViewVec;  // xyz - dir * dist, w - coord.y
	varying vec4 vDiffuse;

	#ifndef TYPE_FLASH
		#ifdef PASS_COMPOSE
			varying vec3 vNormal;		// xyz - normal dir
			varying vec4 vLightProj;
			varying vec4 vLightVec;		// xyz - dir, w - fog factor

			#ifdef OPT_SHADOW
				varying vec3 vAmbient;
			#endif
		#endif

		varying vec4 vLight;	// lights intensity (MAX_LIGHTS == 4)

		#if defined(OPT_AMBIENT) && defined(TYPE_ENTITY)
			vec3 calcAmbient(vec3 n) {
				vec3 sqr = n * n;
				vec3 pos = step(0.0, n);
				return	sqr.x * mix(uAmbient[1], uAmbient[0], pos.x) +
						sqr.y * mix(uAmbient[3], uAmbient[2], pos.y) +
						sqr.z * mix(uAmbient[5], uAmbient[4], pos.z);
			}
		#endif
	#endif
#endif

#ifdef VERTEX
	attribute vec4 aCoord;
	attribute vec4 aTexCoord;
	attribute vec4 aParam;

	#ifndef PASS_AMBIENT
		attribute vec4 aNormal;
	#endif

	#ifndef PASS_SHADOW
		attribute vec4 aColor;
	#endif

	vec3 mulQuat(vec4 q, vec3 v) {
		return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + v * q.w);
	}

	vec3 mulBasis(vec4 rot, vec4 pos, vec3 v) {
		return mulQuat(rot, v) + pos.xyz;
	}

	vec4 _transform() {
		#ifdef TYPE_ENTITY
			int index = int(aCoord.w * 2.0);
			vec4 rBasisRot = uBasis[index];
			vec4 rBasisPos = uBasis[index + 1];
		#else
			vec4 rBasisRot = uBasis[0];
			vec4 rBasisPos = uBasis[1];
		#endif

		vec4 coord;
		coord.w = rBasisPos.w; // visible flag
		#ifdef TYPE_SPRITE
			coord.xyz = mulBasis(rBasisRot, rBasisPos + aCoord.xyz, vec3(aTexCoord.z, -aTexCoord.w, 0.0) * 32767.0);
		#else
			coord.xyz = mulBasis(rBasisRot, rBasisPos, aCoord.xyz);
		#endif

		#ifndef PASS_SHADOW
			vViewVec = vec4((uViewPos - coord.xyz) * FOG_DIST, coord.y);
		#endif

		#if defined(PASS_COMPOSE) && !defined(TYPE_FLASH)
			#ifdef TYPE_SPRITE
				vNormal.xyz = normalize(vViewVec.xyz);
			#else
				vNormal.xyz = normalize(mulQuat(rBasisRot, aNormal.xyz));
			#endif

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

		#ifdef OPT_CONTACT
			vCoord = coord.xyz;
		#endif
		return coord;
	}

	void _diffuse() {
		#if !defined(PASS_SHADOW)
			vDiffuse = vec4(aColor.xyz * (uMaterial.x * 2.0), uMaterial.w);

			#ifdef UNDERWATER
				vDiffuse.xyz *= UNDERWATER_COLOR;
			#endif

			#ifdef TYPE_MIRROR
				vDiffuse.xyz *= vec3(0.3, 0.3, 2.0); // blue color dodge for crystal
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
					lum.x = aColor.w;
					att.x = 0.0;

					#ifdef TYPE_SPRITE
						lum.x *= uMaterial.y;
					#endif

				#endif
				lum.y = dot(vNormal.xyz, normalize(lv1));	att.y = dot(lv1, lv1);
				lum.z = dot(vNormal.xyz, normalize(lv2));	att.z = dot(lv2, lv2);
				lum.w = dot(vNormal.xyz, normalize(lv3));	att.w = dot(lv3, lv3);
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
					ambient = vec3(min(uMaterial.y, light.x));
				#endif

				#ifdef OPT_SHADOW
					vAmbient = ambient;
					vLight   = light;
				#else
					vLight.w   = 0.0;
					vLight.xyz = uLightColor[1].xyz * light.y + uLightColor[2].xyz * light.z;

					#ifdef TYPE_ENTITY
						vLight.xyz += ambient + uLightColor[0].xyz * light.x;
					#else
						vLight.xyz += light.x;
					#endif

				#endif
			#endif

			#ifdef PASS_AMBIENT
				vLight = aColor.wwww;
			#endif
		#endif
	}

	void _uv(vec3 coord) {
		vTexCoord = aTexCoord;
		#if defined(PASS_COMPOSE) && !defined(TYPE_SPRITE)
			// animated texture coordinates
			vec2 range  = uAnimTexRanges[int(aParam.x)];			// x - start index, y - count
			float frame = fract((aParam.y + uParam.x * 4.0 - range.x) / range.y) * range.y;
			vec2 offset = uAnimTexOffsets[int(range.x + frame)];	// texCoord offset from first frame
			vTexCoord.xy += offset;
			vTexCoord.xy *= vTexCoord.zw;
		#endif

		#if defined(OPT_WATER) && defined(UNDERWATER)
			vCausticsCoord.xy = clamp((coord.xz - uRoomSize.xy) / (uRoomSize.zw - uRoomSize.xy), vec2(0.0), vec2(1.0));
		#endif
	}

	void main() {
		vec4 coord = _transform();

		#ifndef PASS_SHADOW
			_diffuse();
			_lighting(coord.xyz);

			#if defined(PASS_COMPOSE) && !defined(TYPE_FLASH)
				vLightProj = uLightProj * coord;
			#endif
		#endif

		_uv(coord.xyz);

		gl_Position = uViewProj * coord;
	}
#else
	uniform sampler2D sDiffuse;

	#if defined(UNDERWATER) && defined(OPT_WATER)
		uniform sampler2D sReflect;
	#endif

	#ifdef PASS_COMPOSE
		#ifdef TYPE_MIRROR
			uniform samplerCube sEnvironment;
		#endif
	#endif

	#ifdef PASS_SHADOW
		#ifdef SHADOW_COLOR
			vec4 pack(in float value) {
				vec4 bitSh = vec4(256.0*256.0*256.0, 256.0*256.0, 256.0, 1.0);
				vec4 bitMsk = vec4(0.0, 1.0/256.0, 1.0/256.0, 1.0/256.0);
				vec4 res = fract(value * bitSh);
				res -= res.xxyz * bitMsk;
				return res;
			}
		#endif
	#endif

	#ifdef PASS_COMPOSE

		#if defined(OPT_SHADOW) && !defined(TYPE_FLASH)
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

			#define SHADOW_TEXEL (2.0 / SHADOW_TEX_SIZE)

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

			float getShadow(vec4 lightProj) {
				vec3 p = lightProj.xyz / lightProj.w;

				float rShadow = SHADOW(SHADOW_TEXEL * vec3(-0.93289, -0.03146, 0.0) + p) +
								SHADOW(SHADOW_TEXEL * vec3( 0.81628, -0.05965, 0.0) + p) +
								SHADOW(SHADOW_TEXEL * vec3(-0.18455,  0.97225, 0.0) + p) +
								SHADOW(SHADOW_TEXEL * vec3( 0.04032, -0.85898, 0.0) + p);

				if (rShadow > 0.1 && rShadow < 3.9) {
					float angle = randomAngle(vTexCoord.xyy, 15.0);
					vec2 sc = vec2(sin(angle), cos(angle));

					rShadow += SHADOW(SHADOW_TEXEL * rotate(sc, vec2(-0.54316,  0.21186)) + p);
					rShadow += SHADOW(SHADOW_TEXEL * rotate(sc, vec2(-0.03925, -0.34345)) + p);
					rShadow += SHADOW(SHADOW_TEXEL * rotate(sc, vec2( 0.07695,  0.40667)) + p);
					rShadow += SHADOW(SHADOW_TEXEL * rotate(sc, vec2(-0.66378, -0.54068)) + p);
					rShadow += SHADOW(SHADOW_TEXEL * rotate(sc, vec2(-0.54130,  0.66730)) + p);
					rShadow += SHADOW(SHADOW_TEXEL * rotate(sc, vec2( 0.69301,  0.46990)) + p);
					rShadow += SHADOW(SHADOW_TEXEL * rotate(sc, vec2( 0.37228,  0.03811)) + p);
					rShadow += SHADOW(SHADOW_TEXEL * rotate(sc, vec2( 0.28597,  0.80228)) + p);
					rShadow += SHADOW(SHADOW_TEXEL * rotate(sc, vec2( 0.44801, -0.43844)) + p);
					rShadow /= 13.0;
				} else
					rShadow /= 4.0;

				float fade = clamp(dot(vLightVec.xyz, vLightVec.xyz), 0.0, 1.0);
				return rShadow + (1.0 - rShadow) * fade;
			}

			float getShadow() {
				#ifdef TYPE_ROOM
					float vis = min(dot(vNormal.xyz, vLightVec.xyz), vLightProj.w);
				#else
					float vis = vLightProj.w;
				#endif
				return vis > 0.0 ? getShadow(vLightProj) : 1.0;
			}
		#endif

		float calcSpecular(vec3 normal, vec3 viewVec, vec3 lightVec, vec4 color, float intensity) {
			vec3 vv = normalize(viewVec);
			vec3 rv = reflect(-vv, normal);
			vec3 lv = normalize(lightVec);
			return pow(max(0.0, dot(rv, lv)), 8.0) * intensity;
		}

		#if defined(OPT_WATER) && defined(UNDERWATER)
			float calcCaustics(vec3 n) {
				vec2 cc     = vCausticsCoord.xy;
				vec2 border = vec2(256.0) / (uRoomSize.zw - uRoomSize.xy);
				vec2 fade   = smoothstep(vec2(0.0), border, cc) * (1.0 - smoothstep(vec2(1.0) - border, vec2(1.0), cc));
				return texture2D(sReflect, cc).g * max(0.0, -n.y) * fade.x * fade.y;
			}
		#endif
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

	void main() {
		#ifdef PASS_COMPOSE
			#ifdef CLIP_PLANE
				if (vViewVec.w * uParam.z > uParam.w)
					discard;
			#endif
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
			if (color.w <= 0.8)
				discard;
		#endif

		#ifndef PASS_SHADOW
			color *= vDiffuse;
		#endif

		#ifdef PASS_SHADOW

			#ifdef SHADOW_COLOR
				gl_FragColor = pack(gl_FragCoord.z);
			#else
				gl_FragColor = vec4(1.0);
			#endif

		#else

			#ifndef TYPE_FLASH
				#ifdef OPT_SHADOW
					#ifdef PASS_COMPOSE
						vec3 n = normalize(vNormal.xyz);

						vec3 light = uLightColor[1].xyz * vLight.y + uLightColor[2].xyz * vLight.z;

						#ifdef TYPE_ENTITY
							float rShadow = getShadow();
							light += vAmbient + uLightColor[0].xyz * (vLight.x * rShadow);
							#if defined(OPT_WATER) && defined(UNDERWATER)
								light += calcCaustics(n);
							#endif
						#endif

						#ifdef TYPE_ROOM

							light += mix(vAmbient.x, vLight.x, getShadow());
							#if defined(OPT_WATER) && defined(UNDERWATER)
								light += calcCaustics(n);
							#endif

							#ifdef OPT_CONTACT
								light *= getContactAO(vCoord, n) * 0.5 + 0.5;
							#endif

						#endif

						#ifdef TYPE_SPRITE
							light += vLight.x;
						#endif

						#ifndef TYPE_MIRROR
							color.xyz *= light;
						#endif

						#ifdef TYPE_ENTITY
							color.xyz += calcSpecular(n, vViewVec.xyz, vLightVec.xyz, uLightColor[0], uMaterial.z * rShadow + 0.03);
						#endif
					#endif

					#ifdef PASS_AMBIENT
						color.xyz *= vLight.x;
					#endif

				#else
					#ifndef TYPE_MIRROR
						color.xyz *= vLight.xyz;
					#endif
				#endif

				#if defined(PASS_COMPOSE) && !defined(TYPE_FLASH)
					#ifdef UNDERWATER
						color.xyz = mix(UNDERWATER_COLOR * 0.2, color.xyz, vLightVec.w);
					#else
						color.xyz = mix(vec3(0.0), color.xyz, vLightVec.w);
					#endif
				#endif

			#endif

			gl_FragColor = color;
		#endif
	}
#endif
)===="