R"====(
#ifdef GL_ES
	precision lowp	int;
	precision highp float;
#endif

varying vec2 vTexCoord;

#ifdef VERTEX
	uniform mat4 uViewProj;
	uniform vec4 uPosScale;

	attribute vec4 aCoord;
	attribute vec4 aTexCoord;

	#define TEXCOORD_SCALE (1.0 / 32767.0)

	void main() {
		vTexCoord	= aTexCoord.xy * TEXCOORD_SCALE;
		gl_Position = uViewProj * vec4(aCoord.xy * uPosScale.zw + uPosScale.xy, 0.0, 1.0);
	}
#else
	uniform sampler2D	sDiffuse;
	uniform vec4		uMaterial;

	void main() {
		gl_FragColor = texture2D(sDiffuse, vTexCoord) * uMaterial;
	}
#endif
)===="