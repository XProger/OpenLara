R"====(
varying vec2 vTexCoord;

#ifndef PASS_SHADOW
    #ifndef PASS_AMBIENT
        varying vec3  vCoord;
        varying vec4  vNormal;
        varying vec3  vViewVec;
        varying vec4  vLightProj;
        varying float vLightTargetDist;
    #endif
    varying vec4  vColor;
#endif

#define TYPE_SPRITE 0
#define TYPE_ROOM   1
#define TYPE_ENTITY 2
#define TYPE_FLASH  3

uniform int   uType;

#ifdef PASS_COMPOSE
    uniform int   uCaustics;
    uniform float uTime;
#endif

#ifdef VERTEX
    uniform mat4 uViewProj;
    uniform mat4 uModel;

    #ifndef PASS_AMBIENT
        uniform mat4 uViewInv;
        uniform mat4 uLightProj;
        uniform vec3 uLightTarget;
    #endif

    #ifdef PASS_COMPOSE
        uniform vec3 uViewPos;
        uniform vec2 uAnimTexRanges[MAX_RANGES];
        uniform vec2 uAnimTexOffsets[MAX_OFFSETS];
    #endif
    
    attribute vec4 aCoord;
    attribute vec4 aTexCoord;

    #ifndef PASS_AMBIENT
        attribute vec4 aNormal;
    #endif

    #ifndef PASS_SHADOW
        attribute vec4 aColor;
    #endif

    #define TEXCOORD_SCALE (1.0 / 32767.0)
    
    void main() {
        vec4 coord  = uModel * vec4(aCoord.xyz, 1.0);
        
        #ifdef PASS_COMPOSE
            if (uType != TYPE_SPRITE) {
                // animated texture coordinates
                vec2 range  = uAnimTexRanges[int(aTexCoord.z)]; // x - start index, y - count

                float f = fract((aTexCoord.w + uTime * 4.0 - range.x) / range.y) * range.y;
                vec2 offset = uAnimTexOffsets[int(range.x + f)]; // texCoord offset from first frame

                vTexCoord  = (aTexCoord.xy + offset) * TEXCOORD_SCALE; // first frame + offset * isAnimated
                vNormal    = vec4((uModel * vec4(aNormal.xyz, 0.0)).xyz, aNormal.w);
            } else {
                coord.xyz += uViewInv[0].xyz * aTexCoord.z - uViewInv[1].xyz * aTexCoord.w;
                vTexCoord  = aTexCoord.xy * TEXCOORD_SCALE;
                vNormal    = vec4(uViewPos.xyz - coord.xyz, 0.0);
            }
        #else
            vTexCoord = aTexCoord.xy * TEXCOORD_SCALE;
        #endif

        #ifndef PASS_SHADOW
            vColor = aColor;
        #endif

        #ifdef PASS_COMPOSE
            if (uCaustics != 0) {
                float sum = coord.x + coord.y + coord.z;
                vColor.xyz *= abs(sin(sum / 512.0 + uTime)) * 1.5 + 0.5; // color dodge
            }

            vViewVec = uViewPos - coord.xyz;

            vec2 dist = (uLightTarget - coord.xyz).xz;
            vLightTargetDist = dot(dist, dist) / (MAX_SHADOW_DIST * MAX_SHADOW_DIST);
          
            vLightProj = uLightProj * coord;

            vCoord = coord.xyz;
        #endif

        gl_Position = uViewProj * coord;
    }
#else
    uniform sampler2D   sDiffuse;
    uniform vec4        uColor;
    #ifdef PASS_COMPOSE
        uniform vec3    uLightPos[MAX_LIGHTS];
        uniform vec4    uLightColor[MAX_LIGHTS];
    #endif

    #ifdef PASS_SHADOW
        #ifdef SHADOW_COLOR
            vec4 pack(in float value) {
                vec4 bitSh = vec4(256.0*256.0*256.0, 256.0*256.0, 256.0, 1.0);
                vec4 bitMsk = vec4(0.0, 1.0/256.0, 1.0/256.0, 1.0/256.0);
                vec4 res = fract(value * bitSh);
                res -= res.xxyz * bitMsk;
                return res;
            }
        #endif
    #endif

    #ifdef PASS_COMPOSE
        #ifdef AMBIENT_CUBE
            uniform vec3 uAmbientCube[6];

            vec3 getAmbientLight(vec3 n) {
                vec3 sqr = n * n;
                vec3 pos = step(0.0, n);
                return sqr.x * mix(uAmbientCube[0], uAmbientCube[1], pos.x) + 
                       sqr.y * mix(uAmbientCube[2], uAmbientCube[3], pos.y) +
                       sqr.z * mix(uAmbientCube[4], uAmbientCube[5], pos.z);
            }
        #else
            uniform vec3 uAmbient;

            vec3 getAmbientLight(vec3 n) {
                return vec3(vColor.w);
            }
        #endif

        #ifdef SHADOW_SAMPLER
            uniform sampler2DShadow sShadow;
            #ifdef MOBILE
                #define SHADOW(V) (shadow2DEXT(sShadow, V))             
            #else
                #define SHADOW(V) (shadow2D(sShadow, V).x)
            #endif
        #else
            uniform sampler2D sShadow;
            #define CMP(a,b) float(a > b)

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
                vec2 t = vec2(0.0, 1.0 / 1024.0);
                vec2 c = floor(p.xy * 1024.0 + 0.5) / 1024.0;
                vec4 s;
                s.x = compare(c + t.xx, p.z);
                s.y = compare(c + t.xy, p.z);
                s.z = compare(c + t.yx, p.z);
                s.w = compare(c + t.yy, p.z);
                vec2 f = fract(p.xy * 1024.0 + 0.5);
                return mix(mix(s.x, s.y, f.y), mix(s.z, s.w, f.y), f.x);
            }
        #endif

        float getShadow(vec4 lightProj) {
            vec3 p = lightProj.xyz / lightProj.w;
            if (lightProj.w < 0.0) return 1.0; 

            vec2 poissonDisk[16];
            poissonDisk[ 0] = vec2( -0.94201624, -0.39906216 );
            poissonDisk[ 1] = vec2(  0.94558609, -0.76890725 );
            poissonDisk[ 2] = vec2( -0.09418410, -0.92938870 );
            poissonDisk[ 3] = vec2(  0.34495938,  0.29387760 );
            poissonDisk[ 4] = vec2( -0.91588581,  0.45771432 );
            poissonDisk[ 5] = vec2( -0.81544232, -0.87912464 );
            poissonDisk[ 6] = vec2( -0.38277543,  0.27676845 );
            poissonDisk[ 7] = vec2(  0.97484398,  0.75648379 );
            poissonDisk[ 8] = vec2(  0.44323325, -0.97511554 );
            poissonDisk[ 9] = vec2(  0.53742981, -0.47373420 );
            poissonDisk[10] = vec2( -0.26496911, -0.41893023 );
            poissonDisk[11] = vec2(  0.79197514,  0.19090188 );
            poissonDisk[12] = vec2( -0.24188840,  0.99706507 );
            poissonDisk[13] = vec2( -0.81409955,  0.91437590 );
            poissonDisk[14] = vec2(  0.19984126,  0.78641367 );
            poissonDisk[15] = vec2(  0.14383161, -0.14100790 );

            float rShadow = 0.0;
            for (int i = 0; i < 16; i += 1)
                rShadow += SHADOW(p + vec3(poissonDisk[i] * 1.5, 0.0) * (1.0 / 1024.0));
            rShadow /= 16.0;

            float fade = min(1.0, vLightTargetDist);
            return mix(rShadow, 1.0, fade);
        }

        vec3 calcLight(vec3 normal, vec3 pos, vec4 color) {
            vec3 lv = pos - vCoord.xyz;
            float lum = max(0.0, dot(normal, normalize(lv)));
            float att = max(0.0, 1.0 - dot(lv, lv) / color.w);
            return color.xyz * (lum * att);
        }
    #endif

    void main() {
        vec4 color = texture2D(sDiffuse, vTexCoord);
        if (color.w < 0.6)
            discard;
        
        #ifdef PASS_SHADOW
            #ifdef SHADOW_COLOR
                gl_FragColor = pack(gl_FragCoord.z);
            #else
                gl_FragColor = vec4(1.0);
            #endif
        #else
            color.xyz *= uColor.xyz;
            color.xyz *= vColor.xyz;

            color.xyz = pow(abs(color.xyz), vec3(2.2)); // to linear space

            #ifdef PASS_AMBIENT
                color.xyz *= vColor.w;
            #else
            // calc point lights
                if (uType != TYPE_FLASH) {
                    vec3 normal   = normalize(vNormal.xyz);
                    vec3 viewVec  = normalize(vViewVec);
                    vec3 light    = vec3(0.0);
            
                    for (int i = 1; i < MAX_LIGHTS; i++) // additional lights
                        light += calcLight(normal, uLightPos[i], uLightColor[i]);

                // apply lighting
                    if (uType == TYPE_SPRITE) {
                        light += vColor.w * uColor.w;
                    }

                    if (uType == TYPE_ROOM) {
                        light += mix(min(uColor.w, vColor.w), vColor.w, getShadow(vLightProj));
                    }

                    if (uType == TYPE_ENTITY) {
                        float shadow = getShadow(vLightProj);
                        vec3 lum = calcLight(normal, uLightPos[0], uLightColor[0]) * shadow + uColor.w;
                        light += lum;
                        light *= dot(normal, viewVec) * 0.2 + 0.8; // apply backlight
                    }    
                    color.xyz *= light;
                } else {
                    color.w = uColor.w;
                }
            #endif

            color.xyz = pow(abs(color.xyz), vec3(1.0/2.2)); // back to gamma space

        // apply fog
            float fog = clamp(1.0 / exp(gl_FragCoord.z / gl_FragCoord.w * 0.000025), 0.0, 1.0);
            color = mix(vec4(0.0, 0.0, 0.0, 1.0), color, fog);

            gl_FragColor = color;
        #endif
    }
#endif
)===="