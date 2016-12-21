R"====(
varying vec2 vTexCoord;

#define TYPE_DOWNSAMPLE 10

uniform int uType;

#ifdef VERTEX
    attribute vec4 aCoord;

    void main() {
        vTexCoord   = aCoord.zw;
        gl_Position = vec4(aCoord.xy, 0.0, 1.0);
    }
#else
    uniform sampler2D sDiffuse;

    uniform float uTime; // texture size

    vec4 downsample() {        
        float k = 1.0 / uTime;

        vec4 color = vec4(0.0);
        for (float y = -1.5; y < 2.0; y++)
            for (float x = -1.5; x < 2.0; x++)
                color += texture2D(sDiffuse, vTexCoord + vec2(x, y) * k);
        color /= 16.0;

        return color;
    }
    
    vec4 filter() {
        if (uType == TYPE_DOWNSAMPLE) {
            return downsample();
        }
        return vec4(1.0, 0.0, 0.0, 1.0);
    }
    
    void main() {
        gl_FragColor = filter();
    }
#endif
)===="