R"====(
uniform mat4 uViewProj;
uniform vec4 uViewPos;
uniform vec4 uFogParams;

varying vec2 vTexCoord;
varying vec4 vDiffuse;

#ifdef VERTEX

	uniform vec4 uBasis[2];
	uniform vec4 uMaterial;

	attribute vec4 aCoord;
	attribute vec4 aTexCoord;

	attribute vec3 aColor;
	attribute vec3 aLight;

	vec3 mulQuat(vec4 q, vec3 v) {
		return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + v * q.w);
	}

	vec3 mulBasis(vec4 rot, vec3 pos, vec3 v) {
		return mulQuat(rot, v) + pos;
	}

	void main() {
		vTexCoord = aTexCoord.xy;

		vec4 rBasisRot = uBasis[0];
		vec4 rBasisPos = uBasis[1];

		vec3 coord =
		#ifdef TYPE_SPRITE
			mulBasis(rBasisRot, rBasisPos.xyz + aCoord.xyz, vec3(aTexCoord.z, aTexCoord.w, 0.0) * 32767.0);
		#else
			mulBasis(rBasisRot, rBasisPos.xyz, aCoord.xyz);
		#endif

		vDiffuse.xyz = aColor.xyz * aLight.xyz * uMaterial.xyz;
		float fog = length(uViewPos.xyz - coord.xyz) * uFogParams.w;
		vDiffuse.w = clamp(1.0 / exp(fog), 0.0, 1.0);

		gl_Position = uViewProj * vec4(coord, 1.0);
	}

#else

	uniform sampler2D sDiffuse;

	void main() {
		vec4 color = texture2D(sDiffuse, vTexCoord);

		#ifdef ALPHA_TEST
			if (color.w <= 0.5)
				discard;
		#endif

		color.xyz *= vDiffuse.xyz;
		color.xyz = mix(uFogParams.xyz, color.xyz, vDiffuse.w);
		
		color.xyz *= color.w;

		fragColor = color;
	}

#endif
)===="
