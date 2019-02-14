R"====(
varying vec2 vTexCoord;
varying vec4 vColor;

#ifdef VERTEX
	uniform mat4 uViewProj;
	uniform vec4 uMaterial;

	attribute vec4 aCoord;
	attribute vec4 aTexCoord;
	attribute vec4 aLight;

	void main() {
		vTexCoord	= aTexCoord.xy;
		vColor		= aLight * uMaterial;
		gl_Position = uViewProj * vec4(aCoord.xyz, 1.0);
	}
#else
	uniform sampler2D	sDiffuse;

	void main() {
		fragColor = texture2D(sDiffuse, vTexCoord) * vColor;
	}
#endif
)===="