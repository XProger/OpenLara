#ifndef H_CORE
#define H_CORE

#ifdef _DEBUG
    #define PROFILE
#endif

#include <stdio.h>

#define OS_FILEIO_CACHE
#define OS_PTHREAD_MT

#ifdef WIN32
    #define _OS_WIN      1
    #define _GAPI_GL     1
    //#define _GAPI_D3D9   1
    //#define _GAPI_VULKAN 1

    #include <windows.h>

    #undef OS_PTHREAD_MT
#elif ANDROID
    #define _OS_ANDROID 1
    #define _GAPI_GL    1
    #define _GAPI_GLES  1
    //#define _GAPI_VULKAN

    extern void osToggleVR(bool enable);
#elif __RPI__
    #define _OS_RPI    1
    #define _GAPI_GL   1
    #define _GAPI_GLES 1

    #define DYNGEOM_NO_VBO
#elif __CLOVER__
    #define _OS_CLOVER 1
    #define _GAPI_GL   1
    #define _GAPI_GLES 1

    //#define DYNGEOM_NO_VBO
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
#elif _PSP
    #define _OS_PSP     1
    #define _GAPI_SCEGU 1

    #define FFP
    #define TEX_SWIZZLE
    //#define EDRAM_MESH
    #define EDRAM_TEX

    #undef OS_PTHREAD_MT
#endif

#ifndef _OS_PSP
    #define USE_INFLATE
#endif

#ifdef USE_INFLATE
    #include "libs/tinf/tinf.h"
#endif

#ifdef FFP
    #define SPLIT_BY_TILE
    #ifdef _OS_PSP
        #define SPLIT_BY_CLUT
    #endif
#else
    #define MERGE_MODELS
    #define MERGE_SPRITES
    #define GENERATE_WATER_PLANE
#endif

#include "utils.h"

// muse be equal with base shader
#define SHADOW_TEX_WIDTH    1024
#define SHADOW_TEX_HEIGHT   1024

extern void* osMutexInit     ();
extern void  osMutexFree     (void *obj);
extern void  osMutexLock     (void *obj);
extern void  osMutexUnlock   (void *obj);

extern int   osGetTime       ();

extern bool  osJoyReady      (int index);
extern void  osJoyVibrate    (int index, float L, float R);

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

#define OS_LOCK(mutex) Lock _lock(mutex)

/*
extern void* osRWLockInit    ();
extern void  osRWLockFree    (void *obj);
extern void  osRWLockRead    (void *obj);
extern void  osRWUnlockRead  (void *obj);
extern void  osRWLockWrite   (void *obj);
extern void  osRWUnlockWrite (void *obj);

struct RWLock {
    void *obj;

    RWLock()           { obj = osRWLockInit(); }
    ~RWLock()          { osRWLockFree(obj);    }
    void lockRead()    { osRWLockRead(obj);    }
    void unlockRead()  { osRWUnlockRead(obj);  }
    void lockWrite()   { osRWLockWrite(obj);   }
    void unlockWrite() { osRWUnlockWrite(obj); }
};

struct LockRead {
    RWLock &lock;

    LockRead(RWLock &lock) : lock(lock) { lock.lockRead(); }
    ~LockRead() { lock.unlockRead(); }
};

struct LockWrite {
    RWLock &lock;

    LockWrite(RWLock &lock) : lock(lock) { lock.lockWrite(); }
    ~LockWrite() { lock.unlockWrite(); }
};

#define OS_LOCK_READ(rwLock)  LockRead  _rLock(rwLock)
#define OS_LOCK_WRITE(rwLock) LockWrite _wLock(rwLock)
*/

enum InputKey { ikNone,
// keyboard
    ikLeft, ikRight, ikUp, ikDown, ikSpace, ikTab, ikEnter, ikEscape, ikShift, ikCtrl, ikAlt,
    ik0, ik1, ik2, ik3, ik4, ik5, ik6, ik7, ik8, ik9,
    ikA, ikB, ikC, ikD, ikE, ikF, ikG, ikH, ikI, ikJ, ikK, ikL, ikM,
    ikN, ikO, ikP, ikQ, ikR, ikS, ikT, ikU, ikV, ikW, ikX, ikY, ikZ,
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
    float deltaTime;
    int   lastTime;
    int   x, y, width, height;

    struct Support {
        int  maxVectors;
        int  maxAniso;
        bool shaderBinary;
        bool VAO;
        bool depthTexture;
        bool shadowSampler;
        bool discardFrame;
        bool texNPOT;
        bool texRG;
        bool texBorder;
        bool colorFloat, texFloat, texFloatLinear;
        bool colorHalf, texHalf,  texHalfLinear;
    #ifdef PROFILE
        bool profMarker;
        bool profTiming;
    #endif
    } support;

#define SETTINGS_VERSION 2
#define SETTINGS_READING 0xFF

    struct Settings {
        enum Quality  { LOW, MEDIUM, HIGH };
        enum Stereo   { STEREO_OFF, STEREO_ON, STEREO_SPLIT, STEREO_VR };

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
            uint8 vsync;
            uint8 stereo;
            void setFilter(Quality value) {
                if (value > MEDIUM && !(support.maxAniso > 1))
                    value = MEDIUM;
                filter = value;
            }

            void setLighting(Quality value) {
            #ifdef _OS_PSP
                lighting = LOW;
            #else
                lighting = value;
            #endif
            }

            void setShadows(Quality value) {
            #ifdef _OS_PSP
                shadows = LOW;
            #else
                shadows = value;
            #endif
            }

            void setWater(Quality value) {
            #ifdef _OS_PSP
                water = LOW;
            #else
                if (value > LOW && !(support.texFloat || support.texHalf))
                    value = LOW;
                water = value;
            #endif
            }
        } detail;

        struct {
            uint8 music;
            uint8 sound;
            uint8 reverb;
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
        return osGetTime();
    }

    void resetTime() {
        lastTime = getTime();
        resetState = true;
    }

    void quit() {
        isQuit = true;
    }
}

#include "input.h"
#include "sound.h"

#define MAX_LIGHTS           4
#define MAX_RENDER_BUFFERS   32
#define MAX_CONTACTS         15

struct Shader;
struct Texture;

namespace GAPI {
    struct Shader;
    struct Texture;
}

enum RenderState {
    RS_TARGET           = 1 << 0,
    RS_VIEWPORT         = 1 << 1,
    RS_DEPTH_TEST       = 1 << 2,
    RS_DEPTH_WRITE      = 1 << 3,
    RS_COLOR_WRITE_R    = 1 << 4,
    RS_COLOR_WRITE_G    = 1 << 5,
    RS_COLOR_WRITE_B    = 1 << 6,
    RS_COLOR_WRITE_A    = 1 << 7,
    RS_COLOR_WRITE      = RS_COLOR_WRITE_R | RS_COLOR_WRITE_G | RS_COLOR_WRITE_B | RS_COLOR_WRITE_A,
    RS_CULL_BACK        = 1 << 8,
    RS_CULL_FRONT       = 1 << 9,
    RS_CULL             = RS_CULL_BACK | RS_CULL_FRONT,
    RS_BLEND_ALPHA      = 1 << 10,
    RS_BLEND_ADD        = 1 << 11,
    RS_BLEND_MULT       = 1 << 12,
    RS_BLEND_PREMULT    = 1 << 13,
    RS_BLEND            = RS_BLEND_ALPHA | RS_BLEND_ADD | RS_BLEND_MULT | RS_BLEND_PREMULT,
    RS_DISCARD          = 1 << 14,
};

// Texture image format
enum TexFormat {
    FMT_RGBA, 
    FMT_RGB16,
    FMT_RGBA16,
    FMT_RGBA_FLOAT,
    FMT_RGBA_HALF,
    FMT_DEPTH,
    FMT_SHADOW,
    FMT_MAX,
};

// Texture options
enum TexOption {
    OPT_CUBEMAP = 1,
    OPT_MIPMAPS = 2, 
    OPT_NEAREST = 4,
    OPT_TARGET  = 8,
    OPT_VERTEX  = 16,
    OPT_PROXY   = 32,
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

typedef uint16 Index;

struct Vertex {
    short4 coord;      // xyz  - position, w - joint index (for entities only)
    short4 normal;     // xyz  - vertex normal, w - unused
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
    /* water sub-passes */ \
    E( WATER_DROP     ) \
    E( WATER_STEP     ) \
    E( WATER_CAUSTICS ) \
    E( WATER_MASK     ) \
    E( WATER_COMPOSE  ) \
    /* filter types */ \
    E( FILTER_DEFAULT         ) \
    E( FILTER_DOWNSAMPLE      ) \
    E( FILTER_GRAYSCALE       ) \
    E( FILTER_BLUR            ) \
    E( FILTER_EQUIRECTANGULAR ) \
    /* options */ \
    E( UNDERWATER      ) \
    E( ALPHA_TEST      ) \
    E( CLIP_PLANE      ) \
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

enum CullMode  { cmNone, cmBack,  cmFront };
enum BlendMode { bmNone, bmAlpha, bmAdd, bmMult, bmPremult, bmMAX };

struct Viewport {
    int x, y, width, height;

    Viewport() : x(0), y(0), width(0), height(0) {}
    Viewport(int x, int y, int width, int height) : x(x), y(y), width(width), height(height) {}

    inline bool operator == (const Viewport &vp) const { return x == vp.x && y == vp.y && width == vp.width && height == vp.height; }
    inline bool operator != (const Viewport &vp) const { return !(*this == vp); }
};

namespace Core {
    float eye;
    Viewport viewport, viewportDef;
    mat4 mModel, mView, mProj, mViewProj, mViewInv;
    mat4 mLightProj;
    Basis basis;
    vec4 viewPos;
    vec4 lightPos[MAX_LIGHTS];
    vec4 lightColor[MAX_LIGHTS];
    vec4 params;
    vec4 fogParams;
    vec4 contacts[MAX_CONTACTS];

    Texture *whiteTex, *whiteCube, *blackTex;

    enum Pass { passCompose, passShadow, passAmbient, passWater, passFilter, passGUI, passMAX } pass;

    const char *passNames[Core::passMAX] = { "COMPOSE", "SHADOW", "AMBIENT", "WATER", "FILTER", "GUI" };

    Texture *defaultTarget;
    
    int32   renderState;

    struct Active {
        const PSO     *pso;
        GAPI::Shader  *shader;
        GAPI::Texture *textures[8];
        GAPI::Texture *target;
        int32         renderState;
        uint32        targetFace;
        uint32        targetOp;
        Viewport      viewport; // TODO: ivec4
        vec4          material;

    #ifdef _GAPI_GL
        uint32      VAO;
        uint32      iBuffer;
        uint32      vBuffer;
    #endif

        int32       basisCount;
        Basis       *basis;
    } active;
    
    struct ReqTarget {
        Texture *texture;
        uint32  op;
        uint32  face;
    } reqTarget;

    struct Stats {
        int dips, tris, rt, frame, fps, fpsTime;
    #ifdef PROFILE
        int tFrame;
    #endif

        Stats() : frame(0), fps(0), fpsTime(0) {}

        void start() {
            dips = tris = rt = 0;
        }

        void stop() {
            if (fpsTime < Core::getTime()) {
                LOG("FPS: %d DIP: %d TRI: %d RT: %d\n", fps, dips, tris, rt);
            #ifdef PROFILE
                LOG("frame time: %d mcs\n", tFrame / 1000);
            #endif
                fps     = frame;
                frame   = 0;
                fpsTime = Core::getTime() + 1000;
            } else
                frame++;
        }
    } stats;
}

#ifdef _GAPI_GL
    #include "gapi_gl.h"
#elif _GAPI_D3D9
    #include "gapi_d3d9.h"
#elif _GAPI_GX
    #include "gapi_gx.h"
#elif _GAPI_SCEGU
    #include "gapi_gu.h"
#elif _GAPI_VULKAN
    #include "gapi_vk.h"
#endif

#include "texture.h"
#include "shader.h"

namespace Core {

    static const char *version = "22.05.2018";

    void init() {
        LOG("OpenLara %s\n", version);
        x = y = 0;

        #ifdef USE_INFLATE
            tinf_init();
        #endif

        isQuit = false;

        Input::init();
        Sound::init();

        GAPI::init();

        LOG("cache    : %s\n", Stream::cacheDir);
        LOG("supports :\n");
        LOG("  variyngs count : %d\n", support.maxVectors);
        LOG("  binary shaders : %s\n", support.shaderBinary  ? "true" : "false");
        LOG("  vertex arrays  : %s\n", support.VAO           ? "true" : "false");
        LOG("  depth texture  : %s\n", support.depthTexture  ? "true" : "false");
        LOG("  shadow sampler : %s\n", support.shadowSampler ? "true" : "false");
        LOG("  discard frame  : %s\n", support.discardFrame  ? "true" : "false");
        LOG("  NPOT textures  : %s\n", support.texNPOT       ? "true" : "false");
        LOG("  RG   textures  : %s\n", support.texRG         ? "true" : "false");
        LOG("  border color   : %s\n", support.texBorder     ? "true" : "false");
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

        uint32 data = 0xFFFFFFFF;
        whiteTex  = new Texture(1, 1, FMT_RGBA, OPT_NEAREST, &data);
        whiteCube = new Texture(1, 1, FMT_RGBA, OPT_CUBEMAP, &data);
        data = 0;
        blackTex  = new Texture(1, 1, FMT_RGBA, OPT_NEAREST, &data);

    // init settings
        settings.version = SETTINGS_VERSION;

        settings.detail.setFilter   (Core::Settings::HIGH);
        settings.detail.setLighting (Core::Settings::HIGH);
        settings.detail.setShadows  (Core::Settings::HIGH);
        settings.detail.setWater    (Core::Settings::HIGH);
        settings.detail.vsync        = true;
        settings.detail.stereo       = Settings::STEREO_OFF;
        settings.audio.music         = 14;
        settings.audio.sound         = 14;
        settings.audio.reverb        = true;

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
        delete whiteTex;
        delete whiteCube;
        delete blackTex;

        GAPI::deinit();

        Sound::deinit();
    }

    void setVSync(bool enable) {
        GAPI::setVSync(Core::settings.detail.vsync = enable);
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

            Texture *target = reqTarget.texture;
            uint32  face    = reqTarget.face;

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

    void setViewport(const Viewport &vp) {
        viewport = vp;
        renderState |= RS_VIEWPORT;
    }

    void setViewport(int x, int y, int width, int height) {
        setViewport(Viewport(x, y, width, height));
    }

    void setCullMode(CullMode mode) {
        renderState &= ~RS_CULL;
        switch (mode) {
            case cmNone  : break;
            case cmBack  : renderState |= RS_CULL_BACK;  break;
            case cmFront : renderState |= RS_CULL_FRONT; break;
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

    void setTarget(Texture *target, int op, int face = 0) {
        if (!target)
            target = defaultTarget;

        bool color = !target || (target->fmt != FMT_DEPTH && target->fmt != FMT_SHADOW);
        setColorWrite(color, color, color, color);

        if (target == defaultTarget) // backbuffer
            setViewport(viewportDef);
        else
            setViewport(0, 0, target->width, target->height);

        reqTarget.texture = target;
        reqTarget.op      = op;
        reqTarget.face    = face;
        renderState |= RS_TARGET;
    }

    void setBasis(Basis *basis, int count) {
        Core::active.basis      = basis;
        Core::active.basisCount = count;

        Core::active.shader->setParam(uBasis, basis[0], count);
    }

    void setMaterial(float diffuse, float ambient, float specular, float alpha) {
        Core::active.material = vec4(diffuse, ambient, specular, alpha);

        Core::active.shader->setParam(uMaterial, Core::active.material);
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
        mesh->bind(range);

        stats.dips++;
        stats.tris += range.iCount / 3;

        validateRenderState();

        GAPI::DIP(mesh, range);
    }

    PSO* psoCreate(Shader *shader, uint32 renderState, TexFormat colorFormat = FMT_RGBA, TexFormat depthFormat = FMT_DEPTH, const vec4 &clearColor = vec4(0.0f)) {
        PSO *pso = new PSO();
        pso->data        = NULL;
        pso->shader      = shader;
        pso->renderState = renderState;
        pso->colorFormat = colorFormat;
        pso->depthFormat = depthFormat;
        pso->clearColor  = clearColor;
        GAPI::initPSO(pso);
        return pso;
    }

    void psoDestroy(PSO *pso) {
        GAPI::deinitPSO(pso);
        delete pso;
    }

    void psoBind(PSO *pso) {
        ASSERT(pso);
        ASSERT(pso->data);
        ASSERT(pso->shader);
        ((Shader*)pso->shader)->setup();
        GAPI::bindPSO(pso);

        Core::active.pso = pso;
    }
}

#include "mesh.h"

#endif
