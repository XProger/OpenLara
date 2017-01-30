R"====(
#ifdef GL_ES
    precision highp int;
    precision highp float;
#endif

varying vec2 vTexCoord;

#define FILTER_DOWNSAMPLE 0

uniform int uType;

#ifdef VERTEX
    attribute vec4 aCoord;

    void main() {        
        vTexCoord = aCoord.zw;
        gl_Position = vec4(aCoord.xy, 0.0, 1.0);
    }
#else
    uniform sampler2D sDiffuse;
    uniform vec4 uParam; // texture size

    vec4 downsample() {        
        float k = 1.0 / uParam.x;

        vec4 color = vec4(0.0);
        for (float y = -1.5; y < 2.0; y++)
            for (float x = -1.5; x < 2.0; x++) {
                vec4 p;
                p.xyz  = texture2D(sDiffuse, vTexCoord + vec2(x, y) * k).xyz;
                p.w    = dot(p.xyz, vec3(0.299, 0.587, 0.114));
                p.xyz *= p.w;
                color += p;
            }

        return vec4(color.xyz / color.w, 1.0);
    }
    
    vec4 filter() {
        if (uType == FILTER_DOWNSAMPLE) return downsample();
        return vec4(1.0, 0.0, 0.0, 1.0);
    }
    
    void main() {
        gl_FragColor = filter();
    }
#endif
)===="