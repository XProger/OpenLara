R"====(
varying vec2 vTexCoord;

#ifndef PASS_SHADOW
    #ifndef PASS_AMBIENT
        varying vec3  vCoord;
        varying vec4  vNormal;
        varying vec3  vViewVec;
        varying vec4  vLightProj;
    #endif
    varying vec4  vColor;
#endif

#define TYPE_SPRITE 0
#define TYPE_FLASH  1
#define TYPE_ROOM   2
#define TYPE_ENTITY 3
#define TYPE_MIRROR 4

uniform int   uType;

#ifdef PASS_COMPOSE
    uniform int   uCaustics;
    uniform float uTime;
#endif

#ifdef VERTEX
    uniform mat4 uViewProj;
    uniform vec4 uBasis[32 * 2];

    #ifndef PASS_AMBIENT
        uniform mat4 uViewInv;
        uniform mat4 uLightProj;
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

    vec3 mulQuat(vec4 q, vec3 v) { 
        return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + v * q.w);
    }

    vec3 mulBasis(vec4 rot, vec4 pos, vec3 v) {
        return mulQuat(rot, v) + pos.xyz;
    }
    
    void main() {
        int index = int(aCoord.w) * 2;
        vec4 rBasisRot = uBasis[index];
        vec4 rBasisPos = uBasis[index + 1];

        vec4 coord = vec4(mulBasis(rBasisRot, rBasisPos, aCoord.xyz), rBasisPos.w);
        
        #ifdef PASS_COMPOSE
            if (uType != TYPE_SPRITE) {
                // animated texture coordinates
                vec2 range  = uAnimTexRanges[int(aTexCoord.z)]; // x - start index, y - count

                float f = fract((aTexCoord.w + uTime * 4.0 - range.x) / range.y) * range.y;
                vec2 offset = uAnimTexOffsets[int(range.x + f)]; // texCoord offset from first frame

                vTexCoord  = (aTexCoord.xy + offset) * TEXCOORD_SCALE; // first frame + offset * isAnimated
                vNormal    = vec4(mulQuat(rBasisRot, aNormal.xyz), aNormal.w);
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

            vViewVec   = uViewPos - coord.xyz;          
            vLightProj = uLightProj * coord;

            vCoord = coord.xyz;
        #endif

        gl_Position = uViewProj * coord;
    }
#else
    uniform sampler2D   sDiffuse;
    uniform vec4        uColor;
    #ifdef PASS_COMPOSE
        uniform samplerCube sEnvironment;
        uniform int         uLightsCount;
        uniform vec3        uLightPos[MAX_LIGHTS];
        uniform vec4        uLightColor[MAX_LIGHTS];
        uniform vec3        uAmbient[6];
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

            vec3 lv = uLightPos[0].xyz - vCoord.xyz;
            float fade = clamp(dot(lv, lv) / uLightColor[0].w, 0.0, 1.0);

            return mix(rShadow, 1.0, fade);
        }

        vec3 calcLight(vec3 normal, vec3 pos, vec4 color) {
            vec3 lv = pos - vCoord.xyz;
            float lum = max(0.0, dot(normal, normalize(lv)));
            float att = max(0.0, 1.0 - dot(lv, lv) / color.w);
            return color.xyz * (lum * att);
        }

        vec3 calcSpecular(vec3 normal, vec3 viewVec, vec3 pos, vec4 color, float intensity) {
            vec3 rv = reflect(-viewVec, normal);            
            vec3 lv = normalize(pos - vCoord.xyz);
            float spec = pow(max(0.0, dot(rv, lv)), 8.0) * intensity;
            return vec3(spec);
        }

        vec3 calcAmbient(vec3 n) {
            vec3 sqr = n * n;
            vec3 pos = step(0.0, n);
            return sqr.x * mix(uAmbient[1], uAmbient[0], pos.x) + 
                   sqr.y * mix(uAmbient[3], uAmbient[2], pos.y) +
                   sqr.z * mix(uAmbient[5], uAmbient[4], pos.z);
        }

    #endif
/*
        float getLuminance(vec3 color) {
            return dot(color.xyz, vec3(0.299, 0.587, 0.114));
        }

        vec3 getNormal() {
            const vec2 size = vec2(2.0, 0.0);
            const vec3 off = vec3(-1, 0, 1) / 1024.0;

            float s01 = getLuminance(texture2D(sDiffuse, vTexCoord + off.xy).xyz);
            float s21 = getLuminance(texture2D(sDiffuse, vTexCoord + off.zy).xyz);
            float s10 = getLuminance(texture2D(sDiffuse, vTexCoord + off.yx).xyz);
            float s12 = getLuminance(texture2D(sDiffuse, vTexCoord + off.yz).xyz);
            vec3 va = vec3(size.xy * 0.25, s21-s01);
            vec3 vb = vec3(size.yx * 0.25, s12-s10);
            return normalize(cross(va, vb));
        }
*/
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

                    //vec3 n = getNormal();;
	                //vec3 b = normalize(cross(n, vec3(.0, -1.0, 0.0)));
	                //vec3 t = normalize(cross(b, n));
                    //normal = normalize(normal.x * t + normal.y * b + normal.z * n);

                    vec3 viewVec  = normalize(vViewVec);
                    vec3 light    = vec3(0.0);


                    for (int i = 1; i < MAX_LIGHTS; i++) // additional lights
                        light += calcLight(normal, uLightPos[i], uLightColor[i]);

                // apply lighting
                    if (uType == TYPE_SPRITE) {
                        light += vColor.w * uColor.w;
                    }

                    if (uType == TYPE_ROOM) {
                        float rShadow = dot(normal, uLightPos[0].xyz - vCoord) > 0.0 ? getShadow(vLightProj) : 1.0;
                        //light += calcLight(normal, uLightPos[0], uLightColor[0]);
                        light += mix(min(uColor.w, vColor.w), vColor.w, rShadow);
                    }

                    if (uType == TYPE_ENTITY) {
                        vec3 rAmbient = pow(abs(calcAmbient(normal)), vec3(2.2));
                        float rShadow = getShadow(vLightProj);
                        light += calcLight(normal, uLightPos[0], uLightColor[0]) * rShadow + rAmbient;
                        color.xyz += calcSpecular(normal, viewVec, uLightPos[0], uLightColor[0], uColor.w * rShadow + 0.03);
                    }

                    if (uType == TYPE_MIRROR) {
                        vec3 rv = reflect(-viewVec, normal);
                        color.xyz = uColor.xyz * pow(abs(textureCube(sEnvironment, normalize(rv)).xyz), vec3(2.2));
                        light.xyz = vec3(1.0);
                    }

                    color.xyz *= light;
//color.xyz = normal * 0.5 + 0.5;
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