R"====(
varying vec2 vTexCoord;

#define TYPE_DOWNSAMPLE 10

uniform int uType;

#ifdef VERTEX
    attribute vec4 aCoord;
    
    void main() {
        vTexCoord   = aCoord.zw;
        gl_Position = vec4(aCoord.xy, 1.0, 1.0);
    }
#else
    uniform sampler2D sDiffuse;

    vec4 downsample() {
        vec4 color = texture2D(sDiffuse, vTexCoord);
        return vec4(vec3((color.x + color.y + color.z) / 3.0), color.w);
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