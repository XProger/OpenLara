R"====(
#ifdef ALPHA_TEST
	varying vec2 vTexCoord;
#endif

#ifdef VERTEX

	uniform mat4 uViewProj;
	uniform vec4 uBasis[32 * 2];

	attribute vec4 aCoord;
	#ifdef ALPHA_TEST
		attribute vec4 aTexCoord;
	#endif

	vec3 mulQuat(vec4 q, vec3 v) {
		return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + v * q.w);
	}

	vec3 mulBasis(vec4 rot, vec3 pos, vec3 v) {
		return mulQuat(rot, v) + pos;
	}

	void main() {
		int index = int(aCoord.w * 2.0);
		vec4 rBasisRot = uBasis[index];
		vec4 rBasisPos = uBasis[index + 1];
		#ifdef ALPHA_TEST
			vTexCoord = aTexCoord.xy;
		#endif
		vec3 coord  = mulBasis(rBasisRot, rBasisPos.xyz, aCoord.xyz);
		gl_Position = uViewProj * vec4(coord, rBasisPos.w);
	}
	
#else
	
	#ifdef ALPHA_TEST
		uniform sampler2D sDiffuse;
	#endif

	vec4 pack(float value) {
		vec4 v = fract(value * vec4(1.0, 255.0, 65025.0, 16581375.0));
		return v - v.yzww * vec4(1.0/255.0, 1.0/255.0, 1.0/255.0, 0.0);
	}

	void main() {
		#ifdef ALPHA_TEST
			if (texture2D(sDiffuse, vTexCoord).w <= 0.5)
				discard;
		#endif

		#ifdef SHADOW_COLOR
			fragColor = pack(gl_FragCoord.z);
		#else
			fragColor = vec4(1.0);
		#endif
	}

#endif
)===="
