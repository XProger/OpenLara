R"====(
uniform mat4 uViewProj;
uniform vec4 uMaterial;	// x - diffuse, y - ambient, z - specular, w - alpha

varying vec4 vColor;
varying vec2 vTexCoord; // xy - atlas coords, zw - trapezoidal correction

#ifdef VERTEX

	uniform vec4 uBasis[2];

	attribute vec4 aCoord;
	attribute vec4 aTexCoord;
	attribute vec4 aColor;

	vec3 mulQuat(vec4 q, vec3 v) {
		return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + v * q.w);
	}

	vec3 mulBasis(vec4 rot, vec3 pos, vec3 v) {
		return mulQuat(rot, v) + pos;
	}

	void main() {
		vColor    = vec4(aColor.xyz * uMaterial.x * 2.0 + uMaterial.w, 1.0);
		vTexCoord = aTexCoord.xy;

		vec3 coord = mulBasis(uBasis[0], uBasis[1].xyz, aCoord.xyz);

		gl_Position = uViewProj * vec4(coord, uBasis[1].w);
	}

#else

	uniform sampler2D sDiffuse;

	void main() {
		fragColor = texture2D(sDiffuse, vTexCoord.xy) * vColor;
	}

#endif
)===="
