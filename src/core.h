#ifndef H_CORE
#define H_CORE

#ifdef _DEBUG
    #define PROFILE
#endif

#include <stdio.h>
#include <memory.h>
#include <stdint.h>

#define OS_FILEIO_CACHE
#define OS_PTHREAD_MT

#define USE_CUBEMAP_MIPS

#ifdef WIN32
    #define _OS_WIN      1
    #define _GAPI_GL     1
    //#define _GAPI_D3D9   1
    //#define _GAPI_D3D11  1
    //#define _GAPI_VULKAN 1
    //#define _GAPI_SW     1

    //#define _NAPI_SOCKET

    #include <windows.h>

    #undef OS_PTHREAD_MT

    #ifdef _GAPI_GL
        #define VR_SUPPORT
    #endif
#elif ANDROID
    #define _OS_ANDROID 1
    #define _GAPI_GL    1
    #define _GAPI_GLES  1
    //#define _GAPI_VULKAN

    #define VR_SUPPORT
#elif __SDL2__
    #define _GAPI_GL   1
    #ifdef SDL2_GLES
        #define _GAPI_GLES 1
        #define DYNGEOM_NO_VBO
    #endif
#elif __RPI__
    #define _OS_RPI    1
    #define _GAPI_GL   1
    #define _GAPI_GLES 1

    #define DYNGEOM_NO_VBO
#elif __CLOVER__
    #define _OS_CLOVER 1
    #define _GAPI_GL   1
    #define _GAPI_GLES 1

    #define DYNGEOM_NO_VBO
#elif __PSC__
    #define _OS_PSC    1
    #define _GAPI_GL   1
    #define _GAPI_GLES 1

    #define DYNGEOM_NO_VBO
#elif __BITTBOY__
    #define _OS_BITTBOY 1
    #define _OS_LINUX   1
    #define _GAPI_SW    1
#elif __GCW0__
    #define _OS_GCW0   1
    #define _GAPI_GL   1
    #define _GAPI_GLES 1

    #define DYNGEOM_NO_VBO

    // etnaviv driver has a bug with cubemap mips generator
    #undef USE_CUBEMAP_MIPS
#elif __linux__
    #define _OS_LINUX 1
    #define _GAPI_GL  1
#elif __APPLE__
    #define _GAPI_GL 1
    #include "TargetConditionals.h"

    #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
        #define _OS_IOS    1
        #define _GAPI_GLES 1
    #else
        #define _OS_MAC    1
    #endif
#elif __EMSCRIPTEN__
    #define _OS_WEB    1
    #define _GAPI_GL   1
    #define _GAPI_GLES 1

    #undef  OS_FILEIO_CACHE

    extern int WEBGL_VERSION;
#elif _3DS
    #define _OS_3DS    1
    #define _GAPI_C3D  1

    #undef OS_PTHREAD_MT
#elif _PSP
    #define _OS_PSP  1
    #define _GAPI_GU 1

    #define TEX_SWIZZLE
    //#define EDRAM_MESH
    #define EDRAM_TEX

    #undef OS_PTHREAD_MT
#elif __vita__
    #define _OS_PSV   1
    #define _GAPI_GXM 1

    #undef OS_PTHREAD_MT
#elif __SWITCH__
    #define _OS_SWITCH 1
    #define _GAPI_GL   1

    #undef OS_PTHREAD_MT
#endif

#ifndef _OS_PSP
    #define USE_INFLATE
#endif

#ifdef USE_INFLATE
    #include "libs/tinf/tinf.h"
#endif

#if defined(_GAPI_SW) || defined(_GAPI_GU)
    #define FFP
#endif

#ifdef FFP
    #define SPLIT_BY_TILE
    #if defined(_GAPI_GU)
        #define SPLIT_BY_CLUT
    #endif
#else
    // current etnaviv driver implementation uses uncompatible Mesa GLSL compiler
    // it produce unimplemented TRUNC/ARL instructions instead of F2I
    // so we can't use joints indexing in the shader (see MESH_SKINNING)
    #ifndef _OS_GCW0
        #define MERGE_MODELS
    #endif
    #define MERGE_SPRITES
    #define GENERATE_WATER_PLANE
#endif

#include "utils.h"

// muse be equal with base shader
#if defined(_OS_3DS)
    #define SHADOW_TEX_SIZE      512
#elif defined(_OS_GCW0)
    #define SHADOW_TEX_SIZE      256
#else
    #define SHADOW_TEX_SIZE      2048
#endif

extern void* osMutexInit     ();
extern void  osMutexFree     (void *obj);
extern void  osMutexLock     (void *obj);
extern void  osMutexUnlock   (void *obj);

extern int   osGetTimeMS     ();

extern bool  osJoyReady      (int index);
extern void  osJoyVibrate    (int index, float L, float R);

#define OS_LOCK(mutex) Core::Lock _lock(mutex)

enum InputKey { ikNone,
// keyboard
    ikLeft, ikRight, ikUp, ikDown, ikSpace, ikTab, ikEnter, ikEscape, ikShift, ikCtrl, ikAlt,
    ik0, ik1, ik2, ik3, ik4, ik5, ik6, ik7, ik8, ik9,
    ikA, ikB, ikC, ikD, ikE, ikF, ikG, ikH, ikI, ikJ, ikK, ikL, ikM,
    ikN, ikO, ikP, ikQ, ikR, ikS, ikT, ikU, ikV, ikW, ikX, ikY, ikZ,
    ikN0, ikN1, ikN2, ikN3, ikN4, ikN5, ikN6, ikN7, ikN8, ikN9, ikNAdd, ikNSub, ikNMul, ikNDiv, ikNDot, 
    ikF1, ikF2, ikF3, ikF4, ikF5, ikF6, ikF7, ikF8, ikF9, ikF10, ikF11, ikF12,
    ikMinus, ikPlus, ikLSB, ikRSB, ikSlash, ikBSlash, ikComa, ikDot, ikTilda, ikColon, ikApos, ikPrev, ikNext, ikHome, ikEnd, ikDel, ikIns, ikBack,
// mouse
    ikMouseL, ikMouseR, ikMouseM,
// touch
    ikTouchA, ikTouchB, ikTouchC, ikTouchD, ikTouchE, ikTouchF,

    ikMAX 
};

enum JoyKey {
// gamepad
    jkNone, jkA, jkB, jkX, jkY, jkLB, jkRB, jkSelect, jkStart, jkL, jkR, jkLT, jkRT, jkLeft, jkRight, jkUp, jkDown, jkMAX
};

enum ControlKey {
    cLeft, cRight, cUp, cDown, cJump, cWalk, cAction, cWeapon, cLook, cDuck, cDash, cRoll, cInventory, cStart, cMAX
};

struct KeySet {
    uint8 key;
    uint8 joy;
    
    KeySet() {}
    KeySet(InputKey key, JoyKey joy) : key(key), joy(joy) {}
};

enum RenderTargetOp {
    RT_CLEAR_COLOR   = 0x0001,
    RT_LOAD_COLOR    = 0x0002,
    RT_STORE_COLOR   = 0x0004,

    RT_CLEAR_DEPTH   = 0x0008,
    RT_LOAD_DEPTH    = 0x0010,
    RT_STORE_DEPTH   = 0x0020,
};

namespace Core {

    struct Mutex {
        void *obj;
    
        Mutex()       { obj = osMutexInit(); }
        ~Mutex()      { osMutexFree(obj);    }
        void lock()   { osMutexLock(obj);    }
        void unlock() { osMutexUnlock(obj);  }
    };
    
    struct Lock {
        Mutex &mutex;
    
        Lock(Mutex &mutex) : mutex(mutex) { mutex.lock(); }
        ~Lock() { mutex.unlock(); }
    };

    float deltaTime;
    int   lastTime;
    int   x, y, width, height;

    struct Support {
        int  maxVectors;
        int  maxAniso;
        int  texMinSize;
        bool shaderBinary;
        bool VAO;
        bool depthTexture;
        bool shadowSampler;
        bool discardFrame;
        bool derivatives;
        bool texNPOT;
        bool tex3D;
        bool texRG;
        bool texBorder;
        bool texMaxLevel;
        bool colorFloat, texFloat, texFloatLinear;
        bool colorHalf, texHalf,  texHalfLinear;
    #ifdef PROFILE
        bool profMarker;
        bool profTiming;
    #endif
    } support;

#define SETTINGS_VERSION 7
#define SETTINGS_READING 0xFF

    struct Settings {
        enum Quality  { LOW, MEDIUM, HIGH };
        enum Stereo   { STEREO_OFF, STEREO_SBS, STEREO_ANAGLYPH, STEREO_SPLIT, STEREO_VR };
        enum Scale    { SCALE_25, SCALE_50, SCALE_75, SCALE_100 };

        uint8 version;

        struct {
            union {
                struct {
                    uint8 filter;
                    uint8 lighting;
                    uint8 shadows;
                    uint8 water;
                };
                uint8 quality[4];
            };
            uint8 simple;
            uint8 scale;
            uint8 vsync;
            uint8 stereo;
            void setFilter(Quality value) {
                if (value > MEDIUM && !(support.maxAniso > 1))
                    value = MEDIUM;
                filter = value;
            }

            void setLighting(Quality value) {
            #if defined(_GAPI_SW) || defined(_GAPI_GU)
                lighting = LOW;
            #else
                lighting = value;
            #endif
            }

            void setShadows(Quality value) {
            #if defined(_GAPI_SW) || defined(_GAPI_GU)
                shadows = LOW;
            #else
                shadows = value;
            #endif
            }

            void setWater(Quality value) {
            #if defined(_GAPI_SW) || defined(_GAPI_GU)
                water = LOW;
            #else
                if (value > LOW && !(support.texFloat || support.texHalf))
                    value = LOW;
                if (value > MEDIUM && !support.derivatives)
                    value = MEDIUM;
                water = value;
            #endif
            }

            bool isStereo() {
                return stereo == STEREO_SBS || stereo == STEREO_ANAGLYPH || stereo == STEREO_VR;
            }
        } detail;

        struct {
            uint8 music;
            uint8 sound;
            uint8 reverb;
            uint8 subtitles;
            uint8 language;
        } audio;

        struct Controls {
            uint8  joyIndex;
            uint8  vibration;
            uint8  retarget;
            uint8  multiaim;
            KeySet keys[cMAX];
        } controls[2];

        // temporary, used only for setting controls
        uint8 playerIndex;
        uint8 ctrlIndex;
    } settings;

    bool resetState;
    bool isQuit;

    int getTime() {
        return osGetTimeMS();
    }

    void resetTime() {
        lastTime = getTime();
        resetState = true;
    }

    void quit() {
        isQuit = true;
    }
}

#ifdef VR_SUPPORT
extern void osToggleVR(bool enable);
#else
void osToggleVR(bool enable) {
    Core::settings.detail.stereo = Core::Settings::STEREO_OFF;
}
#endif

#ifdef PROFILE
    struct TimingCPU {
        int &result;

        TimingCPU(int &result) : result(result) {
            result = Core::getTime();
        }

        ~TimingCPU() {
            result = Core::getTime() - result;
        }
    };

    #define PROFILE_CPU_TIMING(result)  TimingCPU timingCPU(result)
#else
    #define PROFILE_CPU_TIMING(result)
#endif

#include "input.h"
#include "sound.h"

#if defined(_NAPI_SOCKET)
    #include "napi_socket.h"
#else
    #include "napi_dummy.h"
#endif

#define LIGHT_STACK_SIZE     1
#define MAX_LIGHTS           4
#define MAX_RENDER_BUFFERS   32
#define MAX_CONTACTS         15
#define NOISE_TEX_SIZE       32
#define PERLIN_TEX_SIZE      128
#define PERLIN_TEX_NAME      "perlin3_128.raw"

struct Shader;
struct Texture;

namespace GAPI {
    struct Shader;
    struct Texture;
}

enum RenderState {
    RS_TARGET           = 1 << 0,
    RS_VIEWPORT         = 1 << 1,
    RS_SCISSOR          = 1 << 2,
    RS_DEPTH_TEST       = 1 << 3,
    RS_DEPTH_WRITE      = 1 << 4,
    RS_COLOR_WRITE_R    = 1 << 5,
    RS_COLOR_WRITE_G    = 1 << 6,
    RS_COLOR_WRITE_B    = 1 << 7,
    RS_COLOR_WRITE_A    = 1 << 8,
    RS_COLOR_WRITE      = RS_COLOR_WRITE_R | RS_COLOR_WRITE_G | RS_COLOR_WRITE_B | RS_COLOR_WRITE_A,
    RS_CULL_BACK        = 1 << 9,
    RS_CULL_FRONT       = 1 << 10,
    RS_CULL             = RS_CULL_BACK | RS_CULL_FRONT,
    RS_BLEND_ALPHA      = 1 << 11,
    RS_BLEND_ADD        = 1 << 12,
    RS_BLEND_MULT       = 1 << 13,
    RS_BLEND_PREMULT    = 1 << 14,
    RS_BLEND            = RS_BLEND_ALPHA | RS_BLEND_ADD | RS_BLEND_MULT | RS_BLEND_PREMULT,
    RS_DISCARD          = 1 << 15,
};

// Texture image format
enum TexFormat {
    FMT_LUMINANCE,
    FMT_RGBA, 
    FMT_RGB16,
    FMT_RGBA16,
    FMT_RG_FLOAT,
    FMT_RG_HALF,
    FMT_DEPTH,
    FMT_SHADOW,
    FMT_MAX,
};

// Texture options
enum TexOption {
    OPT_REPEAT  = 0x0001,
    OPT_CUBEMAP = 0x0002,
    OPT_VOLUME  = 0x0004,
    OPT_MIPMAPS = 0x0008, 
    OPT_NEAREST = 0x0010,
    OPT_TARGET  = 0x0020,
    OPT_VERTEX  = 0x0040,
    OPT_DYNAMIC = 0x0080,
    OPT_DEPEND  = 0x0100,
    OPT_PROXY   = 0x0200,
};

// Pipeline State Object
struct PSO {
    void       *data;
    void       *shader;
    vec4       clearColor;
    TexFormat  colorFormat;
    TexFormat  depthFormat;
    uint32     renderState;
};

#if defined(_OS_WIN) || defined(_OS_LINUX) || defined(_OS_MAC) || defined(_OS_WEB)
    typedef uint32 Index;
#else
    typedef uint16 Index;
#endif

struct Edge {
    Index a, b;

    Edge() {}
    Edge(Index a, Index b) : a(a), b(b) {}
};

struct Vertex {
    short4 coord;      // xyz  - position, w - joint index (for entities only)
    short4 normal;     // xyz  - vertex normal, w - quad(0) or triangle (1)
    short4 texCoord;   // xy   - texture coordinates, zw - trapezoid warping
    ubyte4 color;      // for non-textured geometry
    ubyte4 light;      // xyz  - color, w - use premultiplied alpha
};

struct MeshRange {
    int iStart;
    int iCount;
    int vStart;
    int aIndex;

    uint16 tile;
    uint16 clut;

    MeshRange() : iStart(0), iCount(0), vStart(0), aIndex(-1), tile(0), clut(0) {}
};

#define SHADER_ATTRIBS(E) \
    E( aCoord           ) \
    E( aNormal          ) \
    E( aTexCoord        ) \
    E( aColor           ) \
    E( aLight           )

#define SHADER_SAMPLERS(E) \
    E( sDiffuse         ) \
    E( sNormal          ) \
    E( sReflect         ) \
    E( sShadow          ) \
    E( sEnvironment     ) \
    E( sMask            )

#define SHADER_UNIFORMS(E) \
    E( uFlags           ) \
    E( uParam           ) \
    E( uTexParam        ) \
    E( uViewProj        ) \
    E( uBasis           ) \
    E( uLightProj       ) \
    E( uMaterial        ) \
    E( uAmbient         ) \
    E( uFogParams       ) \
    E( uViewPos         ) \
    E( uLightPos        ) \
    E( uLightColor      ) \
    E( uRoomSize        ) \
    E( uPosScale        ) \
    E( uContacts        )

#define SHADER_DEFINES(E) \
    /* shadow types */ \
    E( SHADOW_SAMPLER ) \
    E( SHADOW_DEPTH   ) \
    E( SHADOW_COLOR   ) \
    /* compose types */ \
    E( TYPE_SPRITE    ) \
    E( TYPE_FLASH     ) \
    E( TYPE_ROOM      ) \
    E( TYPE_ENTITY    ) \
    E( TYPE_MIRROR    ) \
    /* sky */ \
    E( SKY_TEXTURE      ) \
    E( SKY_CLOUDS       ) \
    E( SKY_CLOUDS_AZURE ) \
    /* water sub-passes */ \
    E( WATER_DROP     ) \
    E( WATER_SIMULATE ) \
    E( WATER_CAUSTICS ) \
    E( WATER_RAYS     ) \
    E( WATER_MASK     ) \
    E( WATER_COMPOSE  ) \
    /* filter types */ \
    E( FILTER_UPSCALE          ) \
    E( FILTER_DOWNSAMPLE       ) \
    E( FILTER_DOWNSAMPLE_DEPTH ) \
    E( FILTER_GRAYSCALE        ) \
    E( FILTER_BLUR             ) \
    E( FILTER_ANAGLYPH         ) \
    E( FILTER_EQUIRECTANGULAR  ) \
    /* options */ \
    E( UNDERWATER      ) \
    E( ALPHA_TEST      ) \
    E( OPT_AMBIENT     ) \
    E( OPT_SHADOW      ) \
    E( OPT_CONTACT     ) \
    E( OPT_CAUSTICS    )

enum AttribType   { SHADER_ATTRIBS(DECL_ENUM)  aMAX };
enum SamplerType  { SHADER_SAMPLERS(DECL_ENUM) sMAX };
enum UniformType  { SHADER_UNIFORMS(DECL_ENUM) uMAX };

#define DECL_SD_ENUM(v) SD_##v,
enum ShaderDefine { SHADER_DEFINES(DECL_SD_ENUM) SD_MAX };
#undef DECL_SD_ENUM

const char *AttribName[aMAX]  = { SHADER_ATTRIBS(DECL_STR)  };
const char *SamplerName[sMAX] = { SHADER_SAMPLERS(DECL_STR) };
const char *UniformName[uMAX] = { SHADER_UNIFORMS(DECL_STR) };

#undef SHADER_ATTRIBS
#undef SHADER_SAMPLERS
#undef SHADER_UNIFORMS

enum CullMode  { cmNone, cmBack,  cmFront, cmMAX };
enum BlendMode { bmNone, bmAlpha, bmAdd, bmMult, bmPremult, bmMAX };

namespace Core {
    float eye;
    Texture *eyeTex[2];
    short4 viewport, viewportDef, scissor;
    mat4 mModel, mView, mProj, mViewProj, mViewInv;
    mat4 mLightProj;
    Basis basis;
    vec4 viewPos;
    vec4 lightPos[MAX_LIGHTS];
    vec4 lightColor[MAX_LIGHTS];
    vec4 params;
    vec4 fogParams;
    vec4 contacts[MAX_CONTACTS];

    struct LightStack {
        vec4 pos[MAX_LIGHTS];
        vec4 color[MAX_LIGHTS];
    } lightStack[LIGHT_STACK_SIZE];
    int lightStackCount;

    Texture *whiteTex, *whiteCube, *blackTex, *ditherTex, *noiseTex, *perlinTex;

    enum Pass { passCompose, passShadow, passAmbient, passSky, passWater, passFilter, passGUI, passMAX } pass;

    GAPI::Texture *defaultTarget;
    
    int32   renderState;

    struct Active {
        const PSO     *pso;
        GAPI::Shader  *shader;
        GAPI::Texture *textures[8];
        GAPI::Texture *target;
        int32         renderState;
        uint32        targetFace;
        uint32        targetOp;
        short4        viewport; // x, y, width, height
        short4        scissor;  // x, y, width, height
        vec4          material;

    #ifdef _GAPI_GL
        uint32 VAO;
        uint32 iBuffer;
        uint32 vBuffer;
    #endif

    #ifdef _GAPI_GXM
        Vertex *vBuffer;
    #endif

    #ifdef _GAPI_C3D
        void *VAO;
    #endif

        int32       basisCount;
        Basis       *basis;
    } active;
    
    struct ReqTarget {
        GAPI::Texture *texture;
        uint32  op;
        uint32  face;
    } reqTarget;

    struct Stats {
        uint32 dips, tris, rt, cb, frame, frameIndex, fps;
        int fpsTime;
    #ifdef PROFILE
        int tFrame;
        int video;
    #endif

        Stats() : frame(0), frameIndex(0), fps(0), fpsTime(0) {}

        void start() {
            dips = tris = rt = cb = 0;
        }

        void stop() {
            if (fpsTime < Core::getTime()) {
                LOG("FPS: %d DIP: %d TRI: %d RT: %d CB: %d\n", fps, dips, tris, rt, cb);
            #ifdef PROFILE
                LOG("frame time: %d mcs\n", tFrame / 1000);
                LOG("sound: mix %d rev %d ren %d/%d ogg %d\n", Sound::stats.mixer, Sound::stats.reverb, Sound::stats.render[0], Sound::stats.render[1], Sound::stats.ogg);
                LOG("video: %d\n", video);
            #endif
                fps     = frame;
                frame   = 0;
                fpsTime = Core::getTime() + 1000;
            } else
                frame++;

            frameIndex++;
        }
    } stats;
}

#ifdef _GAPI_SW
    #include "gapi/sw.h"
#elif _GAPI_GL
    #include "gapi/gl.h"
#elif _GAPI_D3D9
    #include "gapi/d3d9.h"
#elif _GAPI_D3D11
    #include "gapi/d3d11.h"
#elif _OS_3DS
    #include "gapi/c3d.h"
#elif _GAPI_GU
    #include "gapi/gu.h"
#elif _GAPI_GXM
    #include "gapi/gxm.h"
#elif _GAPI_VULKAN
    #include "gapi/vk.h"
#endif

#include "texture.h"
#include "shader.h"
#include "video.h"

namespace Core {

    static const char *version = __DATE__;
    static int defLang = 0;

    void readPerlinAsync(Stream *stream, void *userData) {
        int size = PERLIN_TEX_SIZE * PERLIN_TEX_SIZE * PERLIN_TEX_SIZE;
        uint8 *perlinData = NULL;

        if (stream && stream->size == size) {
            perlinData = new uint8[size];
            stream->raw(perlinData, size);
        } else {
            perlinData = Noise::generate(123456, PERLIN_TEX_SIZE, 5, 8, 1.0f);
            Stream::cacheWrite(PERLIN_TEX_NAME, (char*)perlinData, size);
        }
        delete stream;

        perlinTex = new Texture(PERLIN_TEX_SIZE, PERLIN_TEX_SIZE, PERLIN_TEX_SIZE, FMT_LUMINANCE, OPT_REPEAT | OPT_VOLUME, perlinData);
    /*/
        uint8 *pdata = new uint8[SQR(PERLIN_TEX_SIZE) * 4];
        int offset = 0;
        for (int k = 0; k < PERLIN_TEX_SIZE; k++) {
            int j = 0;
            for (int i = 0; i < SQR(PERLIN_TEX_SIZE); i++) {
                pdata[j + 0] = pdata[j + 1] = pdata[j + 2] = perlinData[offset + i];
                pdata[j + 3] = 255;
                j += 4;
            }
            char buf[256];
            sprintf(buf, "noise/perlin_%03d", k);
            Texture::SaveBMP(buf, (char*)pdata, PERLIN_TEX_SIZE, PERLIN_TEX_SIZE);
            offset += PERLIN_TEX_SIZE * PERLIN_TEX_SIZE;
        }
        delete[] pdata;
    */
        delete[] perlinData;
    }

    void init() {
        LOG("OpenLara (%s)\n", version);

        x = y = 0;
        eyeTex[0] = eyeTex[1] = NULL;
        lightStackCount = 0;

        memset(&support, 0, sizeof(support));
        support.texMinSize  = 1;
        support.texMaxLevel = true;
        support.derivatives = true;

        #ifdef USE_INFLATE
            tinf_init();
        #endif

        isQuit = false;

        Input::init();
        Sound::init();
        NAPI::init();

        GAPI::init();

        LOG("cache    : %s\n", cacheDir);
        LOG("supports :\n");
        LOG("  variyngs count : %d\n", support.maxVectors);
        LOG("  binary shaders : %s\n", support.shaderBinary  ? "true" : "false");
        LOG("  vertex arrays  : %s\n", support.VAO           ? "true" : "false");
        LOG("  depth texture  : %s\n", support.depthTexture  ? "true" : "false");
        LOG("  shadow sampler : %s\n", support.shadowSampler ? "true" : "false");
        LOG("  discard frame  : %s\n", support.discardFrame  ? "true" : "false");
        LOG("  NPOT textures  : %s\n", support.texNPOT       ? "true" : "false");
        LOG("  3D   textures  : %s\n", support.tex3D         ? "true" : "false");
        LOG("  RG   textures  : %s\n", support.texRG         ? "true" : "false");
        LOG("  border color   : %s\n", support.texBorder     ? "true" : "false");
        LOG("  max level      : %s\n", support.texMaxLevel   ? "true" : "false");
        LOG("  anisotropic    : %d\n", support.maxAniso);
        LOG("  float textures : float = %s, half = %s\n", 
            support.colorFloat ? "full" : (support.texFloat ? (support.texFloatLinear ? "linear" : "nearest") : "false"),
            support.colorHalf  ? "full" : (support.texHalf  ? (support.texHalfLinear  ? "linear" : "nearest") : "false"));
        LOG("\n");

        defaultTarget = NULL;

        for (int i = 0; i < MAX_LIGHTS; i++) {
            lightPos[i]   = vec3(0.0);
            lightColor[i] = vec4(0, 0, 0, 1);
        }
        eye = 0.0f;

        { // init dummy textures
            int size = SQR(support.texMinSize) * 6;
            uint32 *data = new uint32[size];
            memset(data, 0xFF, size * sizeof(data[0]));
            whiteTex  = new Texture(support.texMinSize, support.texMinSize, 1, FMT_RGBA, OPT_NEAREST, data);
            whiteCube = new Texture(support.texMinSize, support.texMinSize, 1, FMT_RGBA, OPT_CUBEMAP, data);
            memset(data, 0x00, size * sizeof(data[0]));
            blackTex  = new Texture(support.texMinSize, support.texMinSize, 1, FMT_RGBA, OPT_NEAREST, data);
            delete[] data;
        }

        { // generate dithering texture
            uint8 ditherData[] = {
                0x00, 0x7F, 0x1F, 0x9F, 0x07, 0x87, 0x27, 0xA7,
                0xBF, 0x3F, 0xDF, 0x5F, 0xC7, 0x47, 0xE7, 0x67,
                0x2F, 0xAF, 0x0F, 0x8F, 0x37, 0xB7, 0x17, 0x97,
                0xEF, 0x6F, 0xCF, 0x4F, 0xF7, 0x77, 0xD7, 0x57,
                0x0B, 0x8B, 0x2B, 0xAB, 0x03, 0x83, 0x23, 0xA3,
                0xCB, 0x4B, 0xEB, 0x6B, 0xC3, 0x43, 0xE3, 0x63,
                0x3B, 0xBB, 0x1B, 0x9B, 0x33, 0xB3, 0x13, 0x93,
                0xFB, 0x7B, 0xDB, 0x5B, 0xF3, 0x73, 0xD3, 0x53,
            };
            ditherTex = new Texture(8, 8, 1, FMT_LUMINANCE, OPT_REPEAT | OPT_NEAREST, &ditherData);
        }

        { // generate noise texture
            uint8 *noiseData = new uint8[SQR(NOISE_TEX_SIZE) * 4];
            for (int i = 0; i < SQR(NOISE_TEX_SIZE) * 4; i++) {
                noiseData[i] = rand() % 255;
            }
            noiseTex = new Texture(NOISE_TEX_SIZE, NOISE_TEX_SIZE, 1, FMT_RGBA, OPT_REPEAT, noiseData);
            delete[] noiseData;
        }

        perlinTex = NULL;
        if (support.tex3D) {
            Stream::cacheRead(PERLIN_TEX_NAME, readPerlinAsync);
        }

    // init settings
        settings.version = SETTINGS_VERSION;

        settings.detail.setFilter   (Core::Settings::HIGH);
        settings.detail.setLighting (Core::Settings::HIGH);
        settings.detail.setShadows  (Core::Settings::HIGH);
        settings.detail.setWater    (Core::Settings::HIGH);
        settings.detail.simple       = false;
        settings.detail.vsync        = true;
        settings.detail.stereo       = Settings::STEREO_OFF;
        settings.detail.scale        = Settings::SCALE_100;
        settings.audio.music         = 14;
        settings.audio.sound         = 14;
        settings.audio.reverb        = true;
        settings.audio.subtitles     = true;
        settings.audio.language      = defLang;

    // player 1
        {
            Settings::Controls &ctrl = settings.controls[0];
            ctrl.joyIndex  = 0;
            ctrl.vibration = true;
            ctrl.retarget  = true;
            ctrl.multiaim  = true;

            ctrl.keys[ cLeft      ] = KeySet( ikLeft,   jkLeft   );
            ctrl.keys[ cRight     ] = KeySet( ikRight,  jkRight  );
            ctrl.keys[ cUp        ] = KeySet( ikUp,     jkUp     );
            ctrl.keys[ cDown      ] = KeySet( ikDown,   jkDown   );
            ctrl.keys[ cJump      ] = KeySet( ikAlt,    jkX      );
            ctrl.keys[ cWalk      ] = KeySet( ikShift,  jkRB     );
            ctrl.keys[ cAction    ] = KeySet( ikCtrl,   jkA      );
            ctrl.keys[ cWeapon    ] = KeySet( ikSpace,  jkY      );
            ctrl.keys[ cLook      ] = KeySet( ikC,      jkLB     );
            ctrl.keys[ cDuck      ] = KeySet( ikZ,      jkLT     );
            ctrl.keys[ cDash      ] = KeySet( ikX,      jkRT     );
            ctrl.keys[ cRoll      ] = KeySet( ikA,      jkB      );
            ctrl.keys[ cInventory ] = KeySet( ikEscape, jkSelect );
            ctrl.keys[ cStart     ] = KeySet( ikEnter,  jkStart  );
        }

    // player 2
        {
            Settings::Controls &ctrl = settings.controls[1];
            ctrl.joyIndex  = 1;
            ctrl.vibration = true;
            ctrl.retarget  = true;
            ctrl.multiaim  = true;

            ctrl.keys[ cLeft      ] = KeySet( ikNone,   jkLeft   );
            ctrl.keys[ cRight     ] = KeySet( ikNone,   jkRight  );
            ctrl.keys[ cUp        ] = KeySet( ikNone,   jkUp     );
            ctrl.keys[ cDown      ] = KeySet( ikNone,   jkDown   );
            ctrl.keys[ cJump      ] = KeySet( ikNone,   jkX      );
            ctrl.keys[ cWalk      ] = KeySet( ikNone,   jkRB     );
            ctrl.keys[ cAction    ] = KeySet( ikNone,   jkA      );
            ctrl.keys[ cWeapon    ] = KeySet( ikNone,   jkY      );
            ctrl.keys[ cLook      ] = KeySet( ikNone,   jkLB     );
            ctrl.keys[ cDuck      ] = KeySet( ikNone,   jkLT     );
            ctrl.keys[ cDash      ] = KeySet( ikNone,   jkRT     );
            ctrl.keys[ cRoll      ] = KeySet( ikNone,   jkB      );
            ctrl.keys[ cInventory ] = KeySet( ikNone,   jkSelect );
            ctrl.keys[ cStart     ] = KeySet( ikNone,   jkStart  );
        }

    // use S key for action on Mac because Ctrl + Left/Right used by system (default)
    #ifdef _OS_MAC
        settings.controls[0].keys[ cAction    ].key = ikS;
    #endif

    // use D key for jump in browsers
    #ifdef _OS_WEB
        settings.controls[0].keys[ cJump      ].key = ikD;
        settings.controls[0].keys[ cInventory ].key = ikTab;
    #endif

    #if defined(_OS_RPI) || defined(_OS_CLOVER)
        settings.detail.setShadows  (Core::Settings::LOW);
        settings.detail.setLighting (Core::Settings::MEDIUM);
    #endif

    #if defined(_OS_GCW0)
        settings.detail.setFilter   (Core::Settings::MEDIUM);
        settings.detail.setShadows  (Core::Settings::MEDIUM);
        settings.detail.setLighting (Core::Settings::MEDIUM);
        settings.audio.subtitles = false;
    #endif

    #ifdef _OS_PSC
        settings.detail.setLighting (Core::Settings::MEDIUM);
        settings.detail.setShadows  (Core::Settings::LOW);
        settings.detail.setWater    (Core::Settings::LOW);
    #endif

    #ifdef _OS_3DS
        settings.detail.setFilter   (Core::Settings::MEDIUM);
        settings.detail.setLighting (Core::Settings::LOW);
        settings.detail.setShadows  (Core::Settings::LOW);
        settings.detail.setWater    (Core::Settings::LOW);

        settings.audio.reverb = false;
    #endif

    #ifdef FFP
        settings.detail.setFilter   (Core::Settings::MEDIUM);
        settings.detail.setLighting (Core::Settings::LOW);
        settings.detail.setShadows  (Core::Settings::LOW);
        settings.detail.setWater    (Core::Settings::LOW);
    #endif

    #ifdef _OS_PSP
        settings.audio.reverb = false;
    #endif

        memset(&active, 0, sizeof(active));
        renderState = 0;

        resetTime();
    }

    void deinit() {
        delete eyeTex[0];
        delete eyeTex[1];
        delete whiteTex;
        delete whiteCube;
        delete blackTex;
        delete ditherTex;
        delete noiseTex;
        delete perlinTex;

        GAPI::deinit();
        NAPI::deinit();
        Sound::deinit();
    }

    void setVSync(bool enable) {
        GAPI::setVSync((Core::settings.detail.vsync = enable));
    }

    void waitVBlank() {
        if (Core::settings.detail.vsync)
            GAPI::waitVBlank();
    }

    bool update() {
        resetState = false;
        int time = getTime();
        if (time - lastTime <= 0)
            return false;
        deltaTime = (time - lastTime) * 0.001f;
        lastTime = time;
        return true;
    }

    void validateRenderState() {
        int32 mask = renderState ^ active.renderState;
        if (!mask) return;

        if (mask & RS_TARGET) {
            GAPI::discardTarget(!(active.targetOp & RT_STORE_COLOR), !(active.targetOp & RT_STORE_DEPTH));

            GAPI::Texture *target = reqTarget.texture;
            uint32  face          = reqTarget.face;

            if (target != active.target || face != active.targetFace) {
                Core::stats.rt++;

                GAPI::bindTarget(target, face);

                bool depth = target && (target->fmt == FMT_DEPTH || target->fmt == FMT_SHADOW);
                if (support.discardFrame) {
                    if (!(reqTarget.op & RT_LOAD_COLOR) && !depth) reqTarget.op |= RT_CLEAR_COLOR;
                    if (!(reqTarget.op & RT_LOAD_DEPTH) &&  depth) reqTarget.op |= RT_CLEAR_DEPTH;
                }

                active.target     = target;
                active.targetOp   = reqTarget.op;
                active.targetFace = face;
            }
        }

        if (mask & RS_VIEWPORT) {
            if (viewport != active.viewport) {
                active.viewport = viewport;
                GAPI::setViewport(viewport);
            }
            renderState &= ~RS_VIEWPORT;
        }

        if (mask & RS_SCISSOR) {
            if (scissor != active.scissor) {
                active.scissor = scissor;
                GAPI::setScissor(scissor);
            }
            renderState &= ~RS_SCISSOR;
        }

        if (mask & RS_DEPTH_TEST)
            GAPI::setDepthTest((renderState & RS_DEPTH_TEST) != 0);
        
        if (mask & RS_DEPTH_WRITE)
            GAPI::setDepthWrite((renderState & RS_DEPTH_WRITE) != 0);

        if (mask & RS_COLOR_WRITE)
            GAPI::setColorWrite((renderState & RS_COLOR_WRITE_R) != 0, (renderState & RS_COLOR_WRITE_G) != 0, (renderState & RS_COLOR_WRITE_B) != 0, (renderState & RS_COLOR_WRITE_A) != 0);

        if (mask & RS_CULL)
            GAPI::setCullMode(renderState & RS_CULL);

        if (mask & RS_BLEND)
            GAPI::setBlendMode(renderState & RS_BLEND);

        if (mask & RS_DISCARD)
            GAPI::setAlphaTest((renderState & RS_DISCARD) != 0);

        if (mask & RS_TARGET) {
            GAPI::clear((reqTarget.op & RT_CLEAR_COLOR) != 0, (reqTarget.op & RT_CLEAR_DEPTH) != 0);
            renderState &= ~RS_TARGET;
        }

        active.renderState = renderState;
    }

    void setClearColor(const vec4 &color) {
        GAPI::setClearColor(color);
    }

    void setViewport(const short4 &v) {
        viewport = v;
        renderState |= RS_VIEWPORT;
    }

    void setViewport(int x, int y, int width, int height) {
        setViewport(short4(x, y, width, height));
    }

    void setScissor(const short4 &s) {
        scissor = s;
        renderState |= RS_SCISSOR;
    }

    void setCullMode(CullMode mode) {
        renderState &= ~RS_CULL;
        switch (mode) {
            case cmBack  : renderState |= RS_CULL_BACK;  break;
            case cmFront : renderState |= RS_CULL_FRONT; break;
            default      : ;
        }
    }

    void setBlendMode(BlendMode mode) {
        renderState &= ~RS_BLEND;
        switch (mode) {
            case bmAlpha    : renderState |= RS_BLEND_ALPHA;   break;
            case bmAdd      : renderState |= RS_BLEND_ADD;     break;
            case bmMult     : renderState |= RS_BLEND_MULT;    break;
            case bmPremult  : renderState |= RS_BLEND_PREMULT; break;
            default         : ;
        }
    }

    void setAlphaTest(bool enable) {
        renderState &= ~RS_DISCARD;
        if (enable)
            renderState |= RS_DISCARD;
    }

    void setColorWrite(bool r, bool g, bool b, bool a) {
        renderState &= ~RS_COLOR_WRITE;
        if (r) renderState |= RS_COLOR_WRITE_R;
        if (g) renderState |= RS_COLOR_WRITE_G;
        if (b) renderState |= RS_COLOR_WRITE_B;
        if (a) renderState |= RS_COLOR_WRITE_A;
    }

    void setDepthWrite(bool enable) {
        if (enable)
            renderState |= RS_DEPTH_WRITE;
        else
            renderState &= ~RS_DEPTH_WRITE;
    }

    void setDepthTest(bool enable) {
        if (enable)
            renderState |= RS_DEPTH_TEST;
        else
            renderState &= ~RS_DEPTH_TEST;
    }

    void setTarget(GAPI::Texture *color, GAPI::Texture *depth, int op, int face = 0) {
        if (!color)
            color = defaultTarget;

        bool isColor = !color || (color->fmt != FMT_DEPTH && color->fmt != FMT_SHADOW);
        setColorWrite(isColor, isColor, isColor, isColor);

        if (color == defaultTarget) // backbuffer
            setViewport(viewportDef);
        else
            setViewport(0, 0, color->origWidth, color->origHeight);

        setScissor(viewport);

        reqTarget.texture = color;
        reqTarget.op      = op;
        reqTarget.face    = face;
        renderState |= RS_TARGET;
    }

    void setBasis(Basis *basis, int count) {
        Core::active.basis      = basis;
        Core::active.basisCount = count;

        Core::active.shader->setParam(uBasis, *(vec4*)basis, count * 2);
    }

    void setMaterial(float diffuse, float ambient, float specular, float alpha) {
        Core::active.material = vec4(diffuse, ambient, specular, alpha);

        Core::active.shader->setParam(uMaterial, Core::active.material);
    }

    void updateLights() {
        GAPI::updateLights(lightPos, lightColor, MAX_LIGHTS);
    }

    void resetLights() {
        for (int i = 0; i < MAX_LIGHTS; i++) {
            lightPos[i]   = vec4(0, 0, 0, 0);
            lightColor[i] = vec4(0, 0, 0, 1);
        }
        updateLights();
    }

    void pushLights() {
        ASSERT(lightStackCount < LIGHT_STACK_SIZE);
        memcpy(lightStack[lightStackCount].pos,   lightPos,   sizeof(lightPos));
        memcpy(lightStack[lightStackCount].color, lightColor, sizeof(lightColor));
        lightStackCount++;
    }

    void popLights() {
        ASSERT(lightStackCount > 0);
        lightStackCount--;
        memcpy(lightPos,   lightStack[lightStackCount].pos,   sizeof(lightPos));
        memcpy(lightColor, lightStack[lightStackCount].color, sizeof(lightColor));
        updateLights();
    }

    void copyTarget(Texture *dst, int xOffset, int yOffset, int x, int y, int width, int height) {
        validateRenderState();
        GAPI::copyTarget(dst, xOffset, yOffset, x, y, width, height);
    }

    vec4 copyPixel(int x, int y) { // GPU sync!
        validateRenderState();
        return GAPI::copyPixel(x, y);
    }

    void reset() {
        GAPI::resetState();

        memset(&active, 0, sizeof(active));
        renderState = 0;

        setViewport(Core::x, Core::y, Core::width, Core::height);
        viewportDef = viewport;
        scissor     = viewport;

        setCullMode(cmFront);
        setBlendMode(bmAlpha);
        setDepthTest(true);
        setDepthWrite(true);
        setColorWrite(true, true, true, true);
        validateRenderState();
    }

    bool beginFrame() {
        Core::stats.start();
        return GAPI::beginFrame();
    }

    void endFrame() {
        if (active.target != defaultTarget) {
            GAPI::setTarget(NULL, NULL, 0);
            validateRenderState();
        }
        GAPI::endFrame();
        Core::stats.stop();
    }

    void setViewProj(const mat4 &mView, const mat4 &mProj) {
        Core::mProj     = mProj;
        Core::mView     = mView;
        Core::mViewProj = mProj * mView;

        GAPI::setViewProj(mView, mProj);
    }

    void DIP(GAPI::Mesh *mesh, const MeshRange &range) {
        validateRenderState();

        mesh->bind(range);
        GAPI::DIP(mesh, range);

        stats.dips++;
        stats.tris += range.iCount / 3;
    }
}

#include "mesh.h"

#endif
