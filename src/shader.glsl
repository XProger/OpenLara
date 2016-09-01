R"====(
#ifndef SPRITE
	varying vec4 vNormal;
	varying vec3 vLightVec;
#endif
varying vec2 vTexCoord;
varying vec4 vColor;
varying vec3 vViewVec;
	
#ifdef VERTEX
	uniform	mat4 uViewProj;
	uniform	mat4 uModel;
	#ifndef SPRITE	
    	uniform vec3 uViewPos;
		uniform	vec3 uLightPos;
    #else
    	uniform	mat4 uViewInv;
	#endif
	
	attribute vec3 aCoord;
	attribute vec2 aTexCoord;
	attribute vec4 aNormal;
	attribute vec4 aColor;
	
	void main() {
		vec4 coord	= uModel * vec4(aCoord, 1.0);
		vTexCoord	= aTexCoord;
		vColor		= aColor;
		#ifndef SPRITE
    		vViewVec	= uViewPos - coord.xyz;
			vLightVec	= uLightPos - coord.xyz;
			vNormal		= uModel * aNormal;
		#else
			coord.xyz	-= uViewInv[0].xyz * aNormal.x + uViewInv[1].xyz * aNormal.y;
		#endif
		gl_Position	= uViewProj * coord;
	}
#else
	uniform sampler2D sDiffuse;
	#ifndef SPRITE
		uniform vec4      uColor;
		uniform vec3      uAmbient;
		uniform vec4      uLightColor;
	#endif

	void main() {
		vec4 color = texture2D(sDiffuse, vTexCoord);
		if (color.w < 0.9)
			discard;
		color *= vColor;
		#ifndef SPRITE
			color *= uColor;
			color.xyz = pow(abs(color.xyz), vec3(2.2));
			float lum = dot(normalize(vNormal.xyz), normalize(vLightVec));
			float att = max(0.0, 1.0 - dot(vLightVec, vLightVec) / uLightColor.w);
			vec3 light = uLightColor.xyz * max(vNormal.w, lum * att) + uAmbient;
			color.xyz *= light;
			color.xyz = pow(abs(color.xyz), vec3(1.0/2.2));
		#endif
		gl_FragColor = color;
	}
#endif
)===="