R"====(
#ifdef GL_ES
    precision lowp  int;
    precision highp float;
#endif


varying vec3 color;

#ifdef VERTEX
    attribute vec4 aCoord;
    attribute vec4 aNormal;
    attribute vec4 aColor;

    uniform mat4 uViewProj;
    uniform vec4 uBasis[2]; // quat rot, pos
    uniform vec3 uPosScale; // xyz - scale
    uniform vec4 uLightPos; // xyz - pos, w - radius

    vec3 mulQuat(vec4 q, vec3 v) {
        return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + v * q.w);
    }

    vec3 mulBasis(vec4 rot, vec4 pos, vec3 v) {
        return mulQuat(rot, v) + pos.xyz;
    }

    void main() {
        vec4 c  = vec4(mulBasis(uBasis[0], uBasis[1], aCoord.xyz * uPosScale), 1.0);
        vec3 n  = mulQuat(uBasis[0], normalize(aNormal.xyz));
        vec3 lv = uLightPos.xyz - c.xyz;

        if (dot(lv, n) < 0.0)
            c.xyz -= normalize(lv) * uLightPos.w;

        color = aColor.xyz;

        gl_Position = uViewProj * c;
    }
#else
    void main() {
        gl_FragColor = vec4(color, 1.0);
    }
#endif
)===="