R"====(
varying vec4 vNormal;
varying vec3 vLightVec[MAX_LIGHTS];
varying vec3 vViewVec;
varying vec2 vTexCoord;
varying vec4 vColor;

#ifdef VERTEX
    uniform mat4 uViewProj;
    uniform mat4 uModel;
    uniform mat4 uViewInv;
    uniform vec3 uLightPos[MAX_LIGHTS];
    uniform vec3 uViewPos;

    #ifndef SPRITE
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
        vColor      = aColor;
        
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
            vNormal     = uModel * aNormal;
        #else
            vTexCoord   = aTexCoord.xy * TEXCOORD_SCALE;
            coord.xyz   -= uViewInv[0].xyz * aTexCoord.z + uViewInv[1].xyz * aTexCoord.w;
            vNormal     = vec4(uViewPos.xyz - coord.xyz, 0.0);
        #endif

        vViewVec = uViewPos - coord.xyz;
        for (int i = 0; i < MAX_LIGHTS; i++)
            vLightVec[i] = uLightPos[i] - coord.xyz;

        gl_Position = uViewProj * coord;
    }
#else
    uniform sampler2D   sDiffuse;
    uniform vec4        uColor;
    uniform vec3        uAmbient;
    uniform vec4        uLightColor[MAX_LIGHTS];

    void main() {
        vec4 color = texture2D(sDiffuse, vTexCoord);
        if (color.w < 0.6)
            discard;

        color *= uColor;
        color.xyz *= vColor.xyz;

        color.xyz = pow(abs(color.xyz), vec3(2.2)); // to linear space

    // calc point lights
        vec3 normal   = normalize(vNormal.xyz);
        vec3 viewVec  = normalize(vViewVec);
        vec3 light    = uAmbient;
        for (int i = 0; i < MAX_LIGHTS; i++) {
            vec3 lv = vLightVec[i];
            vec4 lc = uLightColor[i];
            float lum = max(0.0, dot(normal, normalize(lv)));
            float att = max(0.0, 1.0 - dot(lv, lv) / lc.w);
            light += lc.xyz * (lum * att);
        }
    // calc backlight
        light *= dot(normal, viewVec) * 0.5 + 0.5;

    // apply lighting
        color.xyz *= vColor.w + light;

        color.xyz = pow(abs(color.xyz), vec3(1.0/2.2)); // back to gamma space

    // apply fog
        float fog = clamp(1.0 / exp(gl_FragCoord.z / gl_FragCoord.w * 0.000025), 0.0, 1.0);
        color = mix(vec4(0.0, 0.0, 0.0, 1.0), color, fog);

        gl_FragColor = color;
    }
#endif
)===="