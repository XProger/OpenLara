R"====(
#ifdef GL_ES
    precision highp int;
    precision highp float;
#endif

varying vec2 vTexCoord;

#ifdef VERTEX
    uniform mat4 uViewProj;
    uniform mat4 uModel;

    attribute vec4 aCoord;
    attribute vec4 aTexCoord;

    #define TEXCOORD_SCALE (1.0 / 32767.0)
    
    void main() {
        vec4 coord  = uModel * vec4(aCoord.xyz, 1.0);
        vTexCoord   = aTexCoord.xy * TEXCOORD_SCALE;
        coord.xy   += aTexCoord.zw;
        gl_Position = uViewProj * coord;
    }
#else
    uniform sampler2D   sDiffuse;
    uniform vec4        uColor;

    void main() {
        gl_FragColor = texture2D(sDiffuse, vTexCoord) * uColor;
    }
#endif
)===="