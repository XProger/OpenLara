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

uniform vec4  uTexParam;
uniform vec4  uParam;

uniform sampler2D sNormal;

#ifdef VERTEX
    #define ETA_AIR     1.000
    #define ETA_WATER   1.333

    attribute vec4 aCoord;

    void main() {
        vTexCoord = (aCoord.xy * 0.5 + 0.5) * uTexParam.zw;

        if (uType >= WATER_MASK) {
            vCoord = vec3(aCoord.x, 0.0, aCoord.y) * uPosScale[1] + uPosScale[0];
            vec4 cp = uViewProj * vec4(vCoord, 1.0);

            vProjCoord  = cp;
            gl_Position = cp;
        } else {
            vProjCoord = vec4(0.0);
            if (uType == WATER_CAUSTICS) {
                vec3 rCoord = vec3(aCoord.x, 0.0, aCoord.y) * uPosScale[1];

                vec4 info = texture2D(sNormal, (rCoord.xz * 0.5 + 0.5) * uTexParam.zw);
                vec3 normal = vec3(info.b, -sqrt(1.0 - dot(info.ba, info.ba)), info.a);

                vec3 light = vec3(0.0, -1.0, 0.0);
                vec3 refractedLight = refract(-light, vec3(0.0, 1.0, 0.0), ETA_AIR / ETA_WATER);
                vec3 ray = refract(-light, normal, ETA_AIR / ETA_WATER);
                
                vRefPos1 = rCoord + vec3(0.0, 0.0, 0.0);
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
    uniform sampler2D sEnvironment;

    uniform vec3 uLightPos;
    uniform vec4 uLightColor;

    #define PI   3.141592653589793

    float calcFresnel(float NdotL, float fbias, float fpow) {
        float f = 1.0 - abs(NdotL);
        return clamp(fbias + (1.0 - fbias) * pow(f, fpow), 0.0, 1.0);
    }

    vec4 drop() {
        vec2 tc = gl_FragCoord.xy * uTexParam.xy;
        vec4 v = texture2D(sDiffuse, tc);

        float drop = max(0.0, 1.0 - length(uParam.xy - gl_FragCoord.xy) / uParam.z);
        drop = 0.5 - cos(drop * PI) * 0.5;
        v.x += drop * uParam.w;

        return v * texture2D(sEnvironment, tc).x; // apply coast mask
    }


    float getHeight(float ref, vec2 tc) {
        return mix(ref, texture2D(sDiffuse, tc).x, texture2D(sEnvironment, tc).x);
    }

    vec4 step() {
        vec2 tc = gl_FragCoord.xy * uTexParam.xy;

        vec4 v = texture2D(sDiffuse, tc); // height, speed, normal.xz

/*
        vec4 dx = vec4(0.25, 0.96, -0.25, -0.96) * uTexParam.xyxy + tc.xyxy;
        vec4 dy = vec4(0.25, 0.96, -0.25, -0.96) * uTexParam.yxyx + tc.yxyx;
        float average = (texture2D(sDiffuse, dy.yx).x +
                         texture2D(sDiffuse, dx.zy).x +
                         texture2D(sDiffuse, dy.wz).x +
                         texture2D(sDiffuse, dx.xw).x) * 0.25;

        vec3 d = vec3(uTexParam.xy, 0.0);
        vec2 f = vec2(texture2D(sDiffuse, tc + d.xz).x, texture2D(sDiffuse, tc + d.zy).x);
*/

        vec3 d = vec3(uTexParam.xy, 0.0);
        vec4 f = vec4(getHeight(v.x, tc + d.xz), getHeight(v.x, tc + d.zy),
                      getHeight(v.x, tc - d.xz), getHeight(v.x, tc - d.zy));

//        vec4 f = vec4(texture2D(sDiffuse, tc + d.xz).x, texture2D(sDiffuse, tc + d.zy).x,
//                      texture2D(sDiffuse, tc - d.xz).x, texture2D(sDiffuse, tc - d.zy).x);

        float average = dot(f, vec4(0.25));

    // normal
        f.xy -= v.x;
        vec3 nx = vec3(d.x, f.x, 0.0);
        vec3 ny = vec3(0.0, f.y, d.y);
        v.zw = normalize(cross(ny, nx)).xz * 0.5;

    // velocity
        v.y += (average - v.x) * 1.9;
        v.y *= uParam.x; // fadeout        

    // amplitude
        v.x += v.y * uParam.y;

        return v * texture2D(sEnvironment, tc).x; // apply coast mask
    }

    vec4 caustics() {
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