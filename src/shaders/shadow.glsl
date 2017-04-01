R"====(
#ifdef GL_ES
    precision lowp   int;
    precision highp  float;
#endif

varying vec3 vNormal;
varying vec4 vLightProj;
varying vec3 vLightVec;

uniform vec3 uLightPos[MAX_LIGHTS];
uniform vec4 uLightColor[MAX_LIGHTS]; // xyz - color, w - radius * intensity

#ifdef VERTEX
    attribute vec4 aCoord;
    attribute vec4 aNormal;

    uniform mat4 uViewProj;
    uniform mat4 uLightProj;

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

    void main() {
        #ifdef TYPE_ENTITY
            int index = int(aCoord.w * 2.0);
            vec4 rBasisRot = uBasis[index];
            vec4 rBasisPos = uBasis[index + 1];
        #else
            vec4 rBasisRot = uBasis[0];
            vec4 rBasisPos = uBasis[1];
        #endif

        vNormal = normalize(mulQuat(rBasisRot, aNormal.xyz));

        vec4 coord = vec4(mulBasis(rBasisRot, rBasisPos, aCoord.xyz), rBasisPos.w);

        vLightProj = uLightProj * coord;
        vLightVec  = (uLightPos[0].xyz - coord.xyz) * uLightColor[0].w;

        gl_Position = uViewProj * coord;
    }
#else
    #ifdef SHADOW_SAMPLER
        uniform sampler2DShadow sShadow;
        #ifdef GL_ES
            #define SHADOW(V) (shadow2DEXT(sShadow, V))             
        #else
            #define SHADOW(V) (shadow2D(sShadow, V).x)
        #endif
    #else
        uniform sampler2D sShadow;
        #define CMP(a,b) step(b, a)

        #ifdef SHADOW_DEPTH
            #define compare(p, z) CMP(texture2D(sShadow, (p)).x, (z));
        #elif defined(SHADOW_COLOR)
            float unpack(vec4 value) {
                vec4 bitSh = vec4(1.0/(256.0*256.0*256.0), 1.0/(256.0*256.0), 1.0/256.0, 1.0);
                return dot(value, bitSh);
            }
            #define compare(p, z) CMP(unpack(texture2D(sShadow, (p))), (z));
        #endif

        float SHADOW(vec3 p) {
            return compare(p.xy, p.z);
        }
    #endif

    #define SHADOW_TEXEL (2.0 / 1024.0)

    float random(vec3 seed, float freq) {
        float dt = dot(floor(seed * freq), vec3(53.1215, 21.1352, 9.1322));
        return fract(sin(dt) * 2105.2354);
    }

    float randomAngle(vec3 seed, float freq) {
        return random(seed, freq) * 6.283285;
    }

    vec3 rotate(vec2 sc, vec2 v) {
        return vec3(v.x * sc.y + v.y * sc.x, v.x * -sc.x + v.y * sc.y, 0.0);
    }

    float getShadow(vec4 lightProj) {
        vec3 p = lightProj.xyz / lightProj.w;

        float rShadow = SHADOW(SHADOW_TEXEL * vec3(-0.93289, -0.03146, 0.0) + p) +
                        SHADOW(SHADOW_TEXEL * vec3( 0.81628, -0.05965, 0.0) + p) +
                        SHADOW(SHADOW_TEXEL * vec3(-0.18455,  0.97225, 0.0) + p) +
                        SHADOW(SHADOW_TEXEL * vec3( 0.04032, -0.85898, 0.0) + p); 

        if (rShadow > 0.1 && rShadow < 3.9) {
            float angle = randomAngle(gl_FragCoord.xyz, 15.0);
            vec2 sc = vec2(sin(angle), cos(angle));

            rShadow += SHADOW(SHADOW_TEXEL * rotate(sc, vec2(-0.54316,  0.21186)) + p);
            rShadow += SHADOW(SHADOW_TEXEL * rotate(sc, vec2(-0.03925, -0.34345)) + p);
            rShadow += SHADOW(SHADOW_TEXEL * rotate(sc, vec2( 0.07695,  0.40667)) + p);
            rShadow += SHADOW(SHADOW_TEXEL * rotate(sc, vec2(-0.66378, -0.54068)) + p);
            rShadow += SHADOW(SHADOW_TEXEL * rotate(sc, vec2(-0.54130,  0.66730)) + p);
            rShadow += SHADOW(SHADOW_TEXEL * rotate(sc, vec2( 0.69301,  0.46990)) + p);
            rShadow += SHADOW(SHADOW_TEXEL * rotate(sc, vec2( 0.37228,  0.03811)) + p);
            rShadow += SHADOW(SHADOW_TEXEL * rotate(sc, vec2( 0.28597,  0.80228)) + p);
            rShadow += SHADOW(SHADOW_TEXEL * rotate(sc, vec2( 0.44801, -0.43844)) + p);
            rShadow /= 13.0;
        } else
            rShadow /= 4.0;

        float fade = clamp(dot(vLightVec, vLightVec), 0.0, 1.0);

        return mix(rShadow, 1.0, fade);
    }

    float getShadow() {
        return min(dot(vNormal.xyz, vLightVec), vLightProj.w) > 0.0 ? getShadow(vLightProj) : 1.0;
    }

    void main() {
        float s = getShadow();
        gl_FragColor = vec4(s, s, s, 1.0);
    }
#endif
)===="