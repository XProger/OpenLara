R"====(
#ifdef GL_ES
	precision lowp  int;
	precision highp float;
#endif

varying vec4 vColor;
varying vec2 vTexCoord;

#ifdef VERTEX
	uniform mat4 uViewProj;

	attribute vec4 aCoord;
	attribute vec4 aColor;
	attribute vec4 aTexCoord;

	void main() {
        vColor    = aColor;
		vTexCoord = aTexCoord.xy;
		gl_Position = uViewProj * vec4(aCoord.xyz * 5.0, 1.0);
		gl_Position.z = gl_Position.w;
	}
#else
	uniform sampler2D sDiffuse;

	void main() {
		gl_FragColor = texture2D(sDiffuse, vTexCoord) * vColor;
	}
#endif
)===="