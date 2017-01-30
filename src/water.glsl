R"====(
#ifdef GL_ES
    #ifdef FRAGMENT
        #extension GL_OES_standard_derivatives : enable
    #endif
    precision highp int;
    precision highp float;
#endif

varying vec3 vCoord;
varying vec2 vTexCoord;
varying vec4 vProjCoord;
varying vec3 vRefPos1;
varying vec3 vRefPos2;

#define WATER_DROP      0
#define WATER_STEP      1
#define WATER_CAUSTICS  2
#define WATER_MASK      3
#define WATER_COMPOSE   4

uniform int   uType;
uniform vec3  uViewPos;
uniform mat4  uViewProj;
uniform vec3  uPosScale[2];
uniform float uScale;

uniform sampler2D sNormal;

#ifdef VERTEX
    #define ETA_AIR     1.000
    #define ETA_WATER   1.333

    attribute vec4 aCoord;

    void main() {
        vTexCoord = aCoord.xy * 0.5 + 0.5;

        if (uType >= WATER_MASK) {
        // hardcoded pool geometry
            vCoord = vec3(aCoord.x, 0.0, aCoord.y) * uPosScale[1] + uPosScale[0];
            vec4 cp = uViewProj * vec4(vCoord, 1.0);

            vProjCoord  = cp;
            gl_Position = cp;
        } else {
            vProjCoord = vec4(0.0);
            if (uType == WATER_CAUSTICS) {
                vec3 rCoord = vec3(aCoord.x, 0.0, aCoord.y) * uPosScale[1];

                vec4 info = texture2D(sNormal, rCoord.xz * 0.5 + 0.5);
                info.ba *= 0.5;
                vec3 normal = vec3(info.b, -sqrt(1.0 - dot(info.ba, info.ba)), info.a);

                vec3 light = vec3(0.0, -1.0, 0.0);
                vec3 refractedLight = refract(-light, vec3(0.0, 1.0, 0.0), ETA_AIR / ETA_WATER);
                vec3 ray = refract(-light, normal, ETA_AIR / ETA_WATER);
                
                vRefPos1 = rCoord + vec3(0.0, 1.0, 0.0);
                vRefPos2 = rCoord + vec3(0.0, info.r, 0.0) + ray / ray.y;
      
                gl_Position = vec4((vRefPos2.xz + 0.0 * refractedLight.xz / refractedLight.y), 0.0, 1.0);
            } else {
                vRefPos1 = vRefPos2 = vec3(0.0);
                vCoord = vec3(aCoord.xy, 0.0);
                
                gl_Position = vec4(aCoord.xyz, 1.0);   
            }
        }        
    }
#else
    uniform sampler2D sDiffuse;
    uniform sampler2D sReflect;

    uniform vec4 uParam; // texture size

    uniform vec3 uLightPos;
    uniform vec4 uLightColor;

    #define PI          3.141592653589793

    vec3 cubeProj(vec3 ray, vec3 cubePos, vec3 cubeMin, vec3 cubeMax) {
        vec3 i1 = (cubeMax - vCoord) / ray;
        vec3 i2 = (cubeMin - vCoord) / ray;
        vec3 i0 = max(i1, i2);
        float dist = min(min(i0.x, i0.y), i0.z);
        return vCoord + ray * dist - cubePos;
    }

    float calcFresnel(float NdotL, float fbias, float fpow) {
        float f = (1.0 - abs(NdotL));
        return clamp(fbias + (1.0 - fbias) * pow(f, fpow), 0.0, 1.0);
    }

    vec4 drop() {
        vec4 value = texture2D(sDiffuse, vTexCoord);    
        float drop = max(0.0, 1.0 - length(uParam.xy - vTexCoord) / uParam.z);
        drop = 0.5 - cos(drop * PI) * 0.5;
        value.r += drop * uParam.w;
        return value;
    }

    vec4 step() {
        vec2 dx = vec2(uParam.x, 0.0);
        vec2 dy = vec2(0.0, uParam.y);

        vec4 v = texture2D(sDiffuse, vTexCoord);
        vec4 f = vec4(texture2D(sDiffuse, vTexCoord + dx).r, texture2D(sDiffuse, vTexCoord + dy).r,
                      texture2D(sDiffuse, vTexCoord - dx).r, texture2D(sDiffuse, vTexCoord - dy).r);

        float average = dot(f, vec4(0.25));

    // normal
        f.xy -= v.r;
        vec3 nx = vec3(uParam.x, f.x, 0.0);
        vec3 ny = vec3(0.0, f.y, uParam.y);
        v.ba = normalize(cross(ny, nx)).xz;

    // velocity
        v.g += (average - v.r) * 2.0;
        v.g *= 0.995;

    // amplitude
        v.r += v.g * 0.4;

        return v;
    }

    vec4 caustics() {
        vec4 v = texture2D(sNormal, vTexCoord);
        v.ba *= 0.5;
        vec3 normal = vec3(v.b, sqrt(1.0 - dot(v.ba, v.ba)), v.a);

        float area1 = length(dFdx(vRefPos1)) * length(dFdy(vRefPos1));
        float area2 = length(dFdx(vRefPos2)) * length(dFdy(vRefPos2));

        return vec4(vec3(area1 / area2 * 0.2), 1.0);
    }

    vec4 mask() {
        return vec4(0.0);
    }

    vec4 compose() {
        vec2 tc = vProjCoord.xy / vProjCoord.w * 0.5 + 0.5;

        vec4 value  = texture2D(sNormal, vTexCoord);

        value.ba *= 0.5;
        vec3 normal = normalize(vec3(value.b, -sqrt(1.0 - dot(value.ba, value.ba)), value.a)); 
        vec2 dudv   = (uViewProj * vec4(normal.x, 0.0, normal.z, 0.0)).xy;

        vec3 viewVec = normalize(uViewPos - vCoord);
        vec3 rv = reflect(-viewVec, normal);
        vec3 lv = normalize(uLightPos - vCoord.xyz);

        float spec = pow(max(0.0, dot(rv, lv)), 64.0) * 0.5;

        vec4 refrA = texture2D(sDiffuse, uParam.xy * clamp(tc + dudv * uParam.z, 0.0, 0.999) );
        vec4 refrB = texture2D(sDiffuse, uParam.xy * (tc) );
        vec4 refr  = vec4(mix(refrA.xyz, refrB.xyz, refrA.w), 1.0);
        vec4 refl  = texture2D(sReflect, vec2(tc.x, 1.0 - tc.y) + dudv * uParam.w);

        float fresnel = calcFresnel(dot(normal, viewVec), 0.1, 2.0);
        return mix(refr, refl, fresnel) + spec;
    }   
    
    vec4 pass() {
        if (uType == WATER_DROP)       return drop();
        if (uType == WATER_STEP)       return step();
        if (uType == WATER_CAUSTICS)   return caustics();
        if (uType == WATER_MASK)       return mask();
        if (uType == WATER_COMPOSE)    return compose();
        return vec4(1.0, 0.0, 0.0, 1.0);
    }
    
    void main() {
        gl_FragColor = pass();
    }
#endif
)===="