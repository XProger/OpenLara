R"====(
varying vec3 vLightVec;
varying vec2 vTexCoord;
varying vec4 vNormal;
varying vec4 vColor;
	
#ifdef VERTEX
	uniform	mat4 uViewProj;
	uniform	mat4 uModel;
	uniform	vec3 uLightPos;
	
	attribute vec3 aCoord;
	attribute vec2 aTexCoord;
	attribute vec4 aNormal;
	attribute vec4 aColor;
	
	void main() {
		vec4 coord	= uModel * vec4(aCoord, 1.0);
		vLightVec	= uLightPos - coord.xyz;
		vTexCoord	= aTexCoord;
		vNormal		= uModel * aNormal;
		vColor		= aColor;
		gl_Position	= uViewProj * coord;
	}
#else
	uniform sampler2D sDiffuse;
	uniform vec4      uColor;
	uniform vec3      uAmbient;
	uniform vec4      uLightColor;
	
	void main() {
		vec4 color = texture2D(sDiffuse, vTexCoord) * vColor * uColor;
//	#ifdef LIGHTING
		color.xyz = pow(color.xyz, vec3(2.2));
		float lum = dot(normalize(vNormal.xyz), normalize(vLightVec));
		float att = max(0.0, 1.0 - dot(vLightVec, vLightVec) / uLightColor.w);
		vec3 light = uLightColor.xyz * max(vNormal.w, lum * att) + uAmbient;
		color.xyz *= light;
		color.xyz = pow(color.xyz, vec3(1.0/2.2));
//	#endif
		gl_FragColor = color;
	}
#endif
)===="