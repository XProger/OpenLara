R"====(
uniform mat4 uViewProj;
uniform vec4 uViewPos;

uniform vec4 uMaterial;	// x - diffuse, y - ambient, z - specular, w - alpha
uniform vec4 uFogParams;

varying vec4 vViewVec;	// xyz - dir * dist, w - coord.y * clipPlaneSign
varying vec4 vColor;
varying vec4 vNormal;	// xyz - normal dir, w - fog factor

varying vec4 vTexCoord; // xy - atlas coords, zw - trapezoidal correction

#ifdef VERTEX

	#ifdef MESH_SKINNING
		uniform vec4 uBasis[32 * 2];
	#else
		uniform vec4 uBasis[2];
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
		#ifdef MESH_SKINNING
			int index = int(aCoord.w);
			vec4 rBasisRot = uBasis[index];
			vec4 rBasisPos = uBasis[index + 1];
		#else
			vec4 rBasisRot = uBasis[0];
			vec4 rBasisPos = uBasis[1];
		#endif

		vec3 coord = mulBasis(rBasisRot, rBasisPos.xyz, aCoord.xyz);

		vViewVec = vec4((uViewPos.xyz - coord) * uFogParams.w, 0.0);

		vNormal.xyz = normalize(mulQuat(rBasisRot, aNormal.xyz));

		float fog = length(vViewVec.xyz);
		vNormal.w = clamp(1.0 / exp(fog), 0.0, 1.0);

		return vec4(coord, rBasisPos.w);
	}

	void main() {
		vec4 coord = _transform();

		vColor = vec4(uMaterial.xyz, 1.0) * uMaterial.w;

		gl_Position = uViewProj * coord;
	}

#else
	uniform samplerCube sEnvironment;

	void main() {
		vec3 rv = reflect(-normalize(vViewVec.xyz), normalize(vNormal.xyz));
		fragColor = textureCube(sEnvironment, normalize(rv)) * vColor;
	}

#endif
)===="
