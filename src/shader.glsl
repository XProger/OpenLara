R"====(
#ifndef SPRITE
    varying vec4 vNormal;
    varying vec3 vLightVec;
    varying vec3 vViewVec;
#endif
varying vec2 vTexCoord;
varying vec4 vColor;
    
#ifdef VERTEX
    uniform mat4 uViewProj;
    uniform mat4 uModel;
    uniform vec4 uColor;

    #ifndef SPRITE  
        uniform vec3 uViewPos;
        uniform vec3 uLightPos;
        uniform vec2 uAnimTexRanges[MAX_RANGES];
        uniform vec2 uAnimTexOffsets[MAX_OFFSETS];
    #else
        uniform mat4 uViewInv;
    #endif

    uniform vec4 uParam; // x - time
    
    attribute vec4 aCoord;
    attribute vec4 aTexCoord;
    attribute vec4 aNormal;
    attribute vec4 aColor;

    #define TEXCOORD_SCALE (1.0 / 32767.0)
    
    void main() {
        vec4 coord  = uModel * vec4(aCoord.xyz, 1.0);
        vColor      = aColor * uColor;
        
        #ifdef CAUSTICS
            float sum = coord.x + coord.y + coord.z;
            vColor.xyz *= abs(sin(sum / 512.0 + uParam.x)) * 0.75 + 0.25;
        #endif

        #ifndef SPRITE
            // animated texture coordinates
            vec2 range  = uAnimTexRanges[int(aTexCoord.z)]; // x - start index, y - count

            float f = fract((aTexCoord.w + uParam.x * 4.0 - range.x) / range.y) * range.y;
            vec2 offset = uAnimTexOffsets[int(range.x + f)]; // texCoord offset from first frame

            vTexCoord   = (aTexCoord.xy + offset) * TEXCOORD_SCALE; // first frame + offset * isAnimated

            vViewVec    = uViewPos - coord.xyz;
            vLightVec   = uLightPos - coord.xyz;
            vNormal     = uModel * aNormal;
        #else
            vTexCoord   = aTexCoord.xy * TEXCOORD_SCALE;
            coord.xyz   -= uViewInv[0].xyz * aTexCoord.z + uViewInv[1].xyz * aTexCoord.w;
        #endif
        gl_Position = uViewProj * coord;
    }
#else
    uniform sampler2D   sDiffuse;

    #ifndef SPRITE
        uniform vec3    uAmbient;
        uniform vec4    uLightColor;
    #endif

    void main() {
        vec4 color = texture2D(sDiffuse, vTexCoord);
        if (color.w < 0.6)
            discard;

        color *= vColor;
        #ifndef SPRITE
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