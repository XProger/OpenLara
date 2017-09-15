R"====(
#ifdef GL_ES
	precision lowp	int;
	precision highp float;
#endif

varying vec2 vTexCoord;
varying vec4 vColor;

#ifdef VERTEX
	uniform mat4 uViewProj;
	uniform vec4 uPosScale;
	uniform vec4 uMaterial;

	attribute vec4 aCoord;
	attribute vec4 aTexCoord;
	attribute vec4 aColor;

	void main() {
		vTexCoord	= aTexCoord.xy;
		vColor		= aColor;
		gl_Position = uViewProj * vec4(aCoord.xy * uPosScale.zw + uPosScale.xy, 0.0, 1.0);
	}
#else
	uniform sampler2D	sDiffuse;

	void main() {
		gl_FragColor = texture2D(sDiffuse, vTexCoord) * vColor;
	}
#endif
)===="