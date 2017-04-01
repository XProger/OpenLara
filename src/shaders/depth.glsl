R"====(
#ifdef GL_ES
    precision lowp  int;
    precision highp float;
#endif

#if defined(ALPHA_TEST)
    varying vec2 vTexCoord;
#endif

#define TEXCOORD_SCALE (1.0 / 32767.0)

#ifdef VERTEX
    attribute vec4 aCoord;

    #if defined(TYPE_SPRITE) || defined(ALPHA_TEST)
        attribute vec4 aTexCoord;
    #endif

    #ifdef TYPE_SPRITE
        uniform mat4 uViewInv;
    #endif

    uniform mat4 uViewProj;

    #ifdef TYPE_ENTITY
        uniform vec4 uBasis[32 * 2];
    #else
        uniform vec4 uBasis[2];
    #endif

    vec3 mulQuat(vec4 q, vec3 v) {
        return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + v * q.w);
    }

    vec3 mulBasis(vec4 rot, vec4 pos, vec3 v) {
        return mulQuat(rot, v) + pos.xyz;
    }

    vec4 _transform() {
        #ifdef TYPE_ENTITY
            int index = int(aCoord.w * 2.0);
            vec4 rBasisRot = uBasis[index];
            vec4 rBasisPos = uBasis[index + 1];
        #else
            vec4 rBasisRot = uBasis[0];
            vec4 rBasisPos = uBasis[1];
        #endif

        vec4 coord = vec4(mulBasis(rBasisRot, rBasisPos, aCoord.xyz), rBasisPos.w);

        #ifdef TYPE_SPRITE
            coord.xyz += uViewInv[0].xyz * aTexCoord.z - uViewInv[1].xyz * aTexCoord.w;
        #endif

        #if defined(ALPHA_TEST)
            vTexCoord = aTexCoord.xy * TEXCOORD_SCALE;
        #endif

        return coord;
    }

    void main() {
        gl_Position = uViewProj * _transform();
    }
#else
    #ifdef ALPHA_TEST
        uniform sampler2D sDiffuse;
    #endif

    void main() {
        #ifdef ALPHA_TEST
            if (texture2D(sDiffuse, vTexCoord).w <= 0.5)
                discard;
        #endif

        gl_FragColor = vec4(0.0);
    }
#endif
)===="