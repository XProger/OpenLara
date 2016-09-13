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
    uniform mat4 uViewInv;
    uniform vec4 uColor;

    #ifndef SPRITE  
        uniform vec3 uViewPos;
        uniform vec3 uLightPos;
        uniform vec2 uAnimTexRanges[MAX_RANGES];
        uniform vec2 uAnimTexOffsets[MAX_OFFSETS];
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
            vColor.xyz *= abs(sin(sum / 512.0 + uParam.x)) * 1.5 + 0.5; // color dodge
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
            vec3 normal   = normalize(vNormal.xyz);
            vec3 lightVec = normalize(vLightVec);
            vec3 viewVec  = normalize(vViewVec);
            float lum = dot(normal, lightVec);
            float att = max(0.0, 1.0 - dot(vLightVec, vLightVec) / uLightColor.w);
            vec3 light = uLightColor.xyz * max(vNormal.w, lum * att) + uAmbient;
        // apply backlight
            light *= max(vNormal.w, dot(normal, viewVec) * 0.5 + 0.5);
            color.xyz *= light;
            color.xyz = pow(abs(color.xyz), vec3(1.0/2.2));
        #endif

    // fog
        float fog = clamp(1.0 / exp(gl_FragCoord.z / gl_FragCoord.w * 0.000025), 0.0, 1.0);

        gl_FragColor = mix(vec4(0.0), color, fog);
    }
#endif
)===="