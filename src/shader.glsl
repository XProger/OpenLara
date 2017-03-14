R"====(
#ifdef GL_ES
    precision lowp  int;
    precision highp float;
#else
    #define lowp
    #define mediump
    #define highp
#endif

varying vec4 vTexCoord; // xy - atlas coords, zw - caustics coords

#ifndef PASS_SHADOW
    #ifndef PASS_AMBIENT
        varying vec3 vCoord;
        varying vec4 vNormal;
        varying vec3 vViewVec;
        varying vec4 vLightProj;
    #endif
    varying vec4 vColor;
    uniform vec4 uMaterial; // x - diffuse, y - ambient, z - specular, w - alpha
#endif

#ifdef PASS_COMPOSE
    uniform vec3 uViewPos;
    uniform vec4 uParam;    // x - time, y - water height, z - clip plane sign, w - clip plane height
    #ifdef UNDERWATER
        uniform vec4 uRoomSize; // xy - minXZ, zw - maxXZ
    #endif
#endif

#ifdef VERTEX
    uniform mat4 uViewProj;
    
    #ifdef TYPE_ENTITY
        uniform vec4 uBasis[32 * 2];
    #else
        uniform vec4 uBasis[2];
    #endif

    #ifndef PASS_AMBIENT
        uniform mat4 uViewInv;
        uniform mat4 uLightProj;
    #endif

    #ifdef PASS_COMPOSE
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
        #ifdef TYPE_ENTITY
            int index = int(aCoord.w) * 2;
            vec4 rBasisRot = uBasis[index];
            vec4 rBasisPos = uBasis[index + 1];
        #else
            vec4 rBasisRot = uBasis[0];
            vec4 rBasisPos = uBasis[1];
        #endif

        vec4 coord = vec4(mulBasis(rBasisRot, rBasisPos, aCoord.xyz), rBasisPos.w);
        
        #ifndef PASS_SHADOW
            vColor = aColor;
            vColor.xyz *= uMaterial.x; // apply diffuse intensity
            #ifdef TYPE_MIRROR
                vColor.xyz *= vec3(0.6, 0.6, 4.0); // blue color dodge for crystal
            #endif
            #ifdef UNDERWATER
                #ifndef TYPE_ENTITY
                    vColor.xyz *= UNDERWATER_COLOR; // underwater blue color
                #endif
            #endif
        #endif

        #ifdef PASS_COMPOSE
            #ifndef TYPE_SPRITE
                // animated texture coordinates
                vec2 range  = uAnimTexRanges[int(aTexCoord.z)]; // x - start index, y - count

                float f = fract((aTexCoord.w + uParam.x * 4.0 - range.x) / range.y) * range.y;
                vec2 offset = uAnimTexOffsets[int(range.x + f)]; // texCoord offset from first frame

                vTexCoord.xy = (aTexCoord.xy + offset) * TEXCOORD_SCALE; // first frame + offset * isAnimated
                vNormal      = vec4(mulQuat(rBasisRot, aNormal.xyz), aNormal.w);

                #ifdef UNDERWATER
                    float sum = coord.x + coord.y + coord.z;
                    vColor.xyz *= abs(sin(sum / 512.0 + uParam.x)) * 1.5 + 0.5; // color dodge
                #endif
            #else
                coord.xyz += uViewInv[0].xyz * aTexCoord.z - uViewInv[1].xyz * aTexCoord.w;
                vTexCoord.xy = aTexCoord.xy * TEXCOORD_SCALE;
                vNormal    = vec4(uViewPos.xyz - coord.xyz, 0.0);
            #endif

            #ifdef UNDERWATER
                vTexCoord.zw = clamp((coord.xz - uRoomSize.xy) / (uRoomSize.zw - uRoomSize.xy), vec2(0.0), vec2(1.0));
            #else
                vTexCoord.zw = vec2(1.0);
            #endif

            vViewVec   = uViewPos - coord.xyz;          
            vLightProj = uLightProj * coord;

            vCoord = coord.xyz;
        #else
            vTexCoord.xy = aTexCoord.xy * TEXCOORD_SCALE;
            vTexCoord.zw = vec2(0.0);
        #endif

        gl_Position = uViewProj * coord;
    }
#else
    uniform sampler2D sDiffuse;

    #if defined(UNDERWATER) && defined(OPT_WATER)
        uniform sampler2D sReflect;
    #endif

    #ifdef PASS_COMPOSE
        #ifdef TYPE_MIRROR
            uniform samplerCube sEnvironment;
        #endif

        #ifdef OPT_AMBIENT
            uniform vec3 uAmbient[6];
        #endif

        uniform vec3 uLightPos[MAX_LIGHTS];
        uniform vec4 uLightColor[MAX_LIGHTS];
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

        #ifdef OPT_SHADOW
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
                /*
                    vec2 t = vec2(0.0, 1.0 / 1024.0);
                    vec2 c = floor(p.xy * 1024.0 + 0.5) / 1024.0;
                    vec4 s;
                    s.x = compare(c + t.xx, p.z);
                    s.y = compare(c + t.xy, p.z);
                    s.z = compare(c + t.yx, p.z);
                    s.w = compare(c + t.yy, p.z);
                    vec2 f = fract(p.xy * 1024.0 + 0.5);
                    return mix(mix(s.x, s.y, f.y), mix(s.z, s.w, f.y), f.x);
                */
                }
            #endif

            float getShadow(vec4 lightProj) {
                vec3 p = lightProj.xyz / lightProj.w;

                float rShadow = 0.0;
                rShadow += SHADOW(p + (vec3(-0.94201624, -0.39906216, 0.0) * (1.5 / 1024.0)));
                rShadow += SHADOW(p + (vec3( 0.94558609, -0.76890725, 0.0) * (1.5 / 1024.0)));
                rShadow += SHADOW(p + (vec3(-0.09418410, -0.92938870, 0.0) * (1.5 / 1024.0)));
                rShadow += SHADOW(p + (vec3( 0.34495938,  0.29387760, 0.0) * (1.5 / 1024.0)));
                rShadow += SHADOW(p + (vec3(-0.91588581,  0.45771432, 0.0) * (1.5 / 1024.0)));
                rShadow += SHADOW(p + (vec3(-0.81544232, -0.87912464, 0.0) * (1.5 / 1024.0)));
                rShadow += SHADOW(p + (vec3(-0.38277543,  0.27676845, 0.0) * (1.5 / 1024.0)));
                rShadow += SHADOW(p + (vec3( 0.97484398,  0.75648379, 0.0) * (1.5 / 1024.0)));
                rShadow += SHADOW(p + (vec3( 0.44323325, -0.97511554, 0.0) * (1.5 / 1024.0)));
                rShadow += SHADOW(p + (vec3( 0.53742981, -0.47373420, 0.0) * (1.5 / 1024.0)));
                rShadow += SHADOW(p + (vec3(-0.26496911, -0.41893023, 0.0) * (1.5 / 1024.0)));
                rShadow += SHADOW(p + (vec3( 0.79197514,  0.19090188, 0.0) * (1.5 / 1024.0)));
                rShadow += SHADOW(p + (vec3(-0.24188840,  0.99706507, 0.0) * (1.5 / 1024.0)));
                rShadow += SHADOW(p + (vec3(-0.81409955,  0.91437590, 0.0) * (1.5 / 1024.0)));
                rShadow += SHADOW(p + (vec3( 0.19984126,  0.78641367, 0.0) * (1.5 / 1024.0)));
                rShadow += SHADOW(p + (vec3( 0.14383161, -0.14100790, 0.0) * (1.5 / 1024.0)));

                rShadow /= 16.0;

                vec3 lv = uLightPos[0].xyz - vCoord.xyz;
                float fade = clamp(dot(lv, lv) / uLightColor[0].w, 0.0, 1.0);

                return mix(rShadow, 1.0, fade);
            }

            float getShadow() {
                return min(dot(vNormal.xyz, uLightPos[0].xyz - vCoord), vLightProj.w) > 0.0 ? getShadow(vLightProj) : 1.0;
            }
        #endif

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

        #ifdef OPT_AMBIENT
            vec3 calcAmbient(vec3 n) {
                vec3 sqr = n * n;
                vec3 pos = step(0.0, n);
                return sqr.x * mix(uAmbient[1], uAmbient[0], pos.x) + 
                       sqr.y * mix(uAmbient[3], uAmbient[2], pos.y) +
                       sqr.z * mix(uAmbient[5], uAmbient[4], pos.z);
            }
        #endif

        #if defined(UNDERWATER) && defined(OPT_WATER)
            float calcCaustics(vec3 n) {
                vec2 fade = smoothstep(uRoomSize.xy, uRoomSize.xy + vec2(256.0), vCoord.xz) * (1.0 - smoothstep(uRoomSize.zw - vec2(256.0), uRoomSize.zw, vCoord.xz));
                return texture2D(sReflect, vTexCoord.zw).g * (max(0.0, -n.y)) * fade.x * fade.y;
            }
        #endif

        vec3 applyFog(vec3 color, vec3 fogColor, float factor) {
            float fog = clamp(1.0 / exp(factor), 0.0, 1.0);
            return mix(fogColor, color, fog);
        }
    #endif

    void main() {
        #ifndef PASS_SHADOW
            #ifndef PASS_AMBIENT
                #ifdef CLIP_PLANE
                    if (vCoord.y * uParam.z > uParam.w)
                        discard;
                #endif
            #endif
        #endif

        vec4 color = texture2D(sDiffuse, vTexCoord.xy);

        #ifdef ALPHA_TEST
            if (color.w <= 0.1)
                discard;
            //color.xyz *= vec3(1.0, 0.0, 0.0);
        #endif
        
        #ifdef PASS_SHADOW
            #ifdef SHADOW_COLOR
                gl_FragColor = pack(gl_FragCoord.z);
            #else
                gl_FragColor = vec4(1.0);
            #endif
        #else
            color.xyz *= vColor.xyz;
            color.xyz *= color.xyz; // to "linear" space

            #ifdef PASS_AMBIENT
                color.xyz *= vColor.w;
            #else
            // calc point lights
                #ifndef TYPE_FLASH
                    vec3 normal   = normalize(vNormal.xyz);
                    vec3 viewVec  = normalize(vViewVec);
                    vec3 light    = vec3(0.0);

                    #ifdef OPT_LIGHTING
                        light += calcLight(normal, uLightPos[1], uLightColor[1]);
                        light += calcLight(normal, uLightPos[2], uLightColor[2]);
                    #endif

                // apply lighting
                    #ifdef TYPE_SPRITE
                        light += vColor.w * uMaterial.y; // apply diffuse intensity
                    #endif

                    #ifdef TYPE_ROOM
                        #ifdef OPT_SHADOW
                            light += mix(min(uMaterial.y, vColor.w), vColor.w, getShadow());
                        #else
                            light += vColor.w;
                        #endif
                        #if defined(UNDERWATER) && defined(OPT_WATER)
                            light += calcCaustics(normal);
                        #endif
                    #endif

                    #ifdef TYPE_ENTITY
                        vec3 mainLight = calcLight(normal, uLightPos[0], uLightColor[0]);
                        float mainSpec = uMaterial.z;

                        #ifdef OPT_SHADOW
                            float rShadow = getShadow();
                            mainLight *= rShadow;
                            mainSpec  *= rShadow;
                        #endif
                        light += mainLight;

                        #ifdef OPT_AMBIENT
                            light += calcAmbient(normal);
                        #else
                            light += uMaterial.y;
                        #endif

                        #ifdef UNDERWATER
                            if (vCoord.y > uParam.y) {
                                color.xyz *= UNDERWATER_COLOR;
                                #ifdef OPT_WATER
                                    light += calcCaustics(normal);
                                #endif
                            } else
                                color.xyz += calcSpecular(normal, viewVec, uLightPos[0], uLightColor[0], mainSpec + 0.03);
                        #else
                            color.xyz += calcSpecular(normal, viewVec, uLightPos[0], uLightColor[0], mainSpec + 0.03);
                        #endif
                    #endif

                    color.xyz *= light;

                #else // ifndef TYPE_FLASH

                    color.w *= uMaterial.w;

                #endif
            #endif

            color.xyz = sqrt(color.xyz); // back to "gamma" space

            #ifdef PASS_COMPOSE
                #ifdef TYPE_MIRROR
                    vec3 rv = reflect(-normalize(vViewVec), normalize(vNormal.xyz));
                    color.xyz = vColor.xyz * textureCube(sEnvironment, normalize(rv)).xyz;
                #endif

                color.xyz = applyFog(color.xyz, vec3(0.0), length(vViewVec) * FOG_DIST);
                #if defined(UNDERWATER) && defined(OPT_WATER)
                    float d = abs((vCoord.y - max(uViewPos.y, uParam.y)) / normalize(vViewVec).y);
                    d *= step(0.0, vCoord.y - uParam.y); 
                    color.xyz = applyFog(color.xyz, UNDERWATER_COLOR * 0.2, d * WATER_FOG_DIST);
                #endif
            #endif

            color.xyz *= 1.5; // add contrast

            gl_FragColor = color;
        #endif
    }
#endif
)===="