#ifndef H_CORE
#define H_CORE

#ifndef _PSP
    #define USE_INFLATE
#endif

#include <stdio.h>
#ifdef WIN32
    #include <windows.h>
    #include <gl/GL.h>
    #include <gl/glext.h>
#elif ANDROID
    #define MOBILE
    #include <GLES2/gl2.h>
    #include <GLES2/gl2ext.h>
    #include <dlfcn.h>

    #define GL_CLAMP_TO_BORDER          0x812D
    #define GL_TEXTURE_BORDER_COLOR     0x1004

    #define GL_TEXTURE_COMPARE_MODE     0x884C
    #define GL_TEXTURE_COMPARE_FUNC     0x884D
    #define GL_COMPARE_REF_TO_TEXTURE   0x884E

    #define GL_RGBA16F                  0x881A
    #define GL_RGBA32F                  0x8814
    #define GL_HALF_FLOAT               0x140B

    #define GL_DEPTH_STENCIL            GL_DEPTH_STENCIL_OES
    #define GL_UNSIGNED_INT_24_8        GL_UNSIGNED_INT_24_8_OES

    #define PFNGLGENVERTEXARRAYSPROC     PFNGLGENVERTEXARRAYSOESPROC
    #define PFNGLDELETEVERTEXARRAYSPROC  PFNGLDELETEVERTEXARRAYSOESPROC
    #define PFNGLBINDVERTEXARRAYPROC     PFNGLBINDVERTEXARRAYOESPROC
    #define glGenVertexArrays            glGenVertexArraysOES
    #define glDeleteVertexArrays         glDeleteVertexArraysOES
    #define glBindVertexArray            glBindVertexArrayOES

    #define PFNGLGETPROGRAMBINARYPROC    PFNGLGETPROGRAMBINARYOESPROC
    #define PFNGLPROGRAMBINARYPROC       PFNGLPROGRAMBINARYOESPROC
    #define glGetProgramBinary           glGetProgramBinaryOES
    #define glProgramBinary              glProgramBinaryOES

    #define GL_PROGRAM_BINARY_LENGTH     GL_PROGRAM_BINARY_LENGTH_OES
#elif __RPI__
    #define MOBILE
    #include <GLES2/gl2.h>
    #include <GLES2/gl2ext.h>
    #include <EGL/egl.h>
    #include <EGL/eglext.h>
    
    #define GL_CLAMP_TO_BORDER          0x812D
    #define GL_TEXTURE_BORDER_COLOR     0x1004

    #define GL_TEXTURE_COMPARE_MODE     0x884C
    #define GL_TEXTURE_COMPARE_FUNC     0x884D
    #define GL_COMPARE_REF_TO_TEXTURE   0x884E

    #undef  GL_RGBA32F
    #undef  GL_RGBA16F
    #undef  GL_HALF_FLOAT

    #define GL_RGBA32F      GL_RGBA
    #define GL_RGBA16F      GL_RGBA
    #define GL_HALF_FLOAT   GL_HALF_FLOAT_OES

    #define GL_DEPTH_STENCIL        GL_DEPTH_STENCIL_OES
    #define GL_UNSIGNED_INT_24_8    GL_UNSIGNED_INT_24_8_OES
    
    #define glGenVertexArrays(...)
    #define glDeleteVertexArrays(...)
    #define glBindVertexArray(...)
    
    #define GL_PROGRAM_BINARY_LENGTH     GL_PROGRAM_BINARY_LENGTH_OES
    #define glGetProgramBinary(...)
    #define glProgramBinary(...)
#elif __linux__
    #define LINUX 1
    #include <GL/gl.h>
    #include <GL/glext.h>
    #include <GL/glx.h>
#elif __APPLE__
    #include "TargetConditionals.h"

    #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
        #define MOBILE
        #include <OpenGLES/ES2/gl.h>
        #include <OpenGLES/ES2/glext.h>
        #include <OpenGLES/ES3/glext.h>

        #define PFNGLGENVERTEXARRAYSPROC    PFNGLGENVERTEXARRAYSOESPROC
        #define PFNGLDELETEVERTEXARRAYSPROC PFNGLDELETEVERTEXARRAYSOESPROC
        #define PFNGLBINDVERTEXARRAYPROC    PFNGLBINDVERTEXARRAYOESPROC
        #define glGenVertexArrays           glGenVertexArraysOES
        #define glDeleteVertexArrays        glDeleteVertexArraysOES
        #define glBindVertexArray           glBindVertexArrayOES

        #define GL_CLAMP_TO_BORDER          0x812D
        #define GL_TEXTURE_BORDER_COLOR     0x1004

        #define GL_TEXTURE_COMPARE_MODE     GL_TEXTURE_COMPARE_MODE_EXT
        #define GL_TEXTURE_COMPARE_FUNC     GL_TEXTURE_COMPARE_FUNC_EXT
        #define GL_COMPARE_REF_TO_TEXTURE   GL_COMPARE_REF_TO_TEXTURE_EXT
    #else
        #include <Carbon/Carbon.h>
        #include <AudioToolbox/AudioQueue.h>
        #include <OpenGL/OpenGL.h>
        #include <OpenGL/gl.h>
        #include <OpenGL/glext.h>
        #include <AGL/agl.h>

        #define GL_RGBA16F                  0x881A
        #define GL_RGBA32F                  0x8814
        #define GL_HALF_FLOAT               0x140B

        #define GL_RGB565                   GL_RGBA
        #define GL_TEXTURE_COMPARE_MODE     0x884C
        #define GL_TEXTURE_COMPARE_FUNC     0x884D
        #define GL_COMPARE_REF_TO_TEXTURE   0x884E

        #define glGenVertexArrays    glGenVertexArraysAPPLE
        #define glDeleteVertexArrays glDeleteVertexArraysAPPLE
        #define glBindVertexArray    glBindVertexArrayAPPLE

        #define GL_PROGRAM_BINARY_LENGTH 0
        #define glGetProgramBinary(...)  0
        #define glProgramBinary(...)     0
    #endif
#elif __EMSCRIPTEN__
    #define MOBILE
    #include <emscripten/emscripten.h>
    #include <emscripten/html5.h>
    #include <GLES3/gl3.h>
    #include <GLES3/gl2ext.h>

    #define GL_CLAMP_TO_BORDER          GL_CLAMP_TO_BORDER_EXT
    #define GL_TEXTURE_BORDER_COLOR     GL_TEXTURE_BORDER_COLOR_EXT

    #define glGetProgramBinary(...)
    #define glProgramBinary(...)
#elif _PSP
    #include <pspgu.h>
    #include <pspgum.h>

    #define FFP
    //#define TEX_SWIZZLE
    #define EDRAM_MESH
    //#define EDRAM_TEX
#endif

#ifdef USE_INFLATE
    #include "libs/tinf/tinf.h"
#endif

#ifdef FFP
    #define SPLIT_BY_TILE
    #ifdef _PSP
        #define SPLIT_BY_CLUT
    #endif
#else
    #define MERGE_MODELS
    #define MERGE_SPRITES
    #define GENERATE_WATER_PLANE
#endif

extern int osGetTime();
extern bool osSave(const char *name, const void *data, int size);

#include "utils.h"

extern void* osMutexInit     ();
extern void  osMutexFree     (void *obj);
extern void  osMutexLock     (void *obj);
extern void  osMutexUnlock   (void *obj);

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
    jkNone, jkA, jkB, jkX, jkY, jkLB, jkRB, jkSelect, jkStart, jkL, jkR, jkLT, jkRT, jkPOV, jkLeft, jkRight, jkUp, jkDown, jkMAX
};

enum ControlKey {
    cLeft, cRight, cUp, cDown, cJump, cWalk, cAction, cWeapon, cLook, cStepLeft, cStepRight, cRoll, cInventory, cStart, cMAX
};

struct KeySet {
    InputKey key;
    JoyKey   joy;
    
    KeySet() {}
    KeySet(InputKey key, JoyKey joy) : key(key), joy(joy) {}
};

namespace Core {
    float deltaTime;
    int   lastTime;
    int   width, height;

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

    struct Settings {
        enum Quality  { LOW, MEDIUM, HIGH };
        enum Stereo   { STEREO_OFF, STEREO_ON, STEREO_SPLIT };

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
            uint8 vr;
            void setFilter(Quality value) {
                if (value > MEDIUM && !(support.maxAniso > 1))
                    value = MEDIUM;
                filter = value;
            }

            void setLighting(Quality value) {
                lighting = value;
            }

            void setShadows(Quality value) {
                if (value > MEDIUM && !(support.maxVectors > 8))
                    value = MEDIUM;
                shadows = value;
            }

            void setWater(Quality value) {
                if (value > LOW && !(support.texFloat || support.texHalf))
                    value = LOW;
                else
                    if (value > MEDIUM && !(support.maxVectors > 8))
                        value = MEDIUM;
                water = value;
            }
        } detail;

        struct Controls {
            KeySet keys[cMAX];
            uint8  joyIndex;
            uint8  retarget;
            uint8  multitarget;
            uint8  vibration;
        } controls[2];

        struct {
            uint8 music;
            uint8 sound;
            uint8 reverb;
        } audio;
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

#if defined(WIN32) || (defined(LINUX) && !defined(__RPI__)) || defined(ANDROID)

    #ifdef ANDROID
        #define GetProc(x) dlsym(libGL, x);
    #else
        void* GetProc(const char *name) {
            #ifdef WIN32
                return (void*)wglGetProcAddress(name);
            #elif __RPI__
                return (void*)eglGetProcAddress(name);
            #elif LINUX
                return (void*)glXGetProcAddress((GLubyte*)name);
            #endif
        }
    #endif

    #define GetProcOGL(x) x=(decltype(x))GetProc(#x);

// Texture
    #ifdef WIN32
        PFNGLACTIVETEXTUREPROC              glActiveTexture;
    #endif

// VSync
    #ifdef WIN32
        typedef BOOL (WINAPI * PFNWGLSWAPINTERVALEXTPROC) (int interval);
        PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;
    #elif LINUX
        typedef int (*PFNGLXSWAPINTERVALSGIPROC) (int interval);
        PFNGLXSWAPINTERVALSGIPROC glXSwapIntervalSGI;
    #endif

    #if defined(WIN32) || defined(LINUX)
        PFNGLGENERATEMIPMAPPROC             glGenerateMipmap;
    // Profiling
        #ifdef PROFILE
            PFNGLOBJECTLABELPROC                glObjectLabel;
            PFNGLPUSHDEBUGGROUPPROC             glPushDebugGroup;
            PFNGLPOPDEBUGGROUPPROC              glPopDebugGroup;
            PFNGLGENQUERIESPROC                 glGenQueries;
            PFNGLDELETEQUERIESPROC              glDeleteQueries;
            PFNGLGETQUERYOBJECTIVPROC           glGetQueryObjectiv;
            PFNGLBEGINQUERYPROC                 glBeginQuery;
            PFNGLENDQUERYPROC                   glEndQuery;
        #endif
    // Shader
        PFNGLCREATEPROGRAMPROC              glCreateProgram;
        PFNGLDELETEPROGRAMPROC              glDeleteProgram;
        PFNGLLINKPROGRAMPROC                glLinkProgram;
        PFNGLUSEPROGRAMPROC                 glUseProgram;
        PFNGLGETPROGRAMINFOLOGPROC          glGetProgramInfoLog;
        PFNGLCREATESHADERPROC               glCreateShader;
        PFNGLDELETESHADERPROC               glDeleteShader;
        PFNGLSHADERSOURCEPROC               glShaderSource;
        PFNGLATTACHSHADERPROC               glAttachShader;
        PFNGLCOMPILESHADERPROC              glCompileShader;
        PFNGLGETSHADERINFOLOGPROC           glGetShaderInfoLog;
        PFNGLGETUNIFORMLOCATIONPROC         glGetUniformLocation;
        PFNGLUNIFORM1IVPROC                 glUniform1iv;
        PFNGLUNIFORM1FVPROC                 glUniform1fv;
        PFNGLUNIFORM2FVPROC                 glUniform2fv;
        PFNGLUNIFORM3FVPROC                 glUniform3fv;
        PFNGLUNIFORM4FVPROC                 glUniform4fv;
        PFNGLUNIFORMMATRIX4FVPROC           glUniformMatrix4fv;
        PFNGLBINDATTRIBLOCATIONPROC         glBindAttribLocation;
        PFNGLENABLEVERTEXATTRIBARRAYPROC    glEnableVertexAttribArray;
        PFNGLDISABLEVERTEXATTRIBARRAYPROC   glDisableVertexAttribArray;
        PFNGLVERTEXATTRIBPOINTERPROC        glVertexAttribPointer;
        PFNGLGETPROGRAMIVPROC               glGetProgramiv;
    // Render to texture
        PFNGLGENFRAMEBUFFERSPROC            glGenFramebuffers;
        PFNGLBINDFRAMEBUFFERPROC            glBindFramebuffer;
        PFNGLGENRENDERBUFFERSPROC           glGenRenderbuffers;
        PFNGLBINDRENDERBUFFERPROC           glBindRenderbuffer;
        PFNGLFRAMEBUFFERTEXTURE2DPROC       glFramebufferTexture2D;
        PFNGLFRAMEBUFFERRENDERBUFFERPROC    glFramebufferRenderbuffer;
        PFNGLRENDERBUFFERSTORAGEPROC        glRenderbufferStorage;
        PFNGLCHECKFRAMEBUFFERSTATUSPROC     glCheckFramebufferStatus;
        PFNGLDELETEFRAMEBUFFERSPROC         glDeleteFramebuffers;
        PFNGLDELETERENDERBUFFERSPROC        glDeleteRenderbuffers;
    // Mesh
        PFNGLGENBUFFERSARBPROC              glGenBuffers;
        PFNGLDELETEBUFFERSARBPROC           glDeleteBuffers;
        PFNGLBINDBUFFERARBPROC              glBindBuffer;
        PFNGLBUFFERDATAARBPROC              glBufferData;
        PFNGLBUFFERSUBDATAARBPROC           glBufferSubData;
    #endif

    PFNGLGENVERTEXARRAYSPROC            glGenVertexArrays;
    PFNGLDELETEVERTEXARRAYSPROC         glDeleteVertexArrays;
    PFNGLBINDVERTEXARRAYPROC            glBindVertexArray;
    PFNGLGETPROGRAMBINARYPROC           glGetProgramBinary;
    PFNGLPROGRAMBINARYPROC              glProgramBinary;
#endif

#if defined(ANDROID) || defined(__EMSCRIPTEN__)
    PFNGLDISCARDFRAMEBUFFEREXTPROC      glDiscardFramebufferEXT;
#endif

#define MAX_LIGHTS           4
#define MAX_CACHED_LIGHTS    3
#define MAX_RENDER_BUFFERS   32
#define MAX_CONTACTS         15
#define MAX_ANIM_TEX_RANGES  16
#define MAX_ANIM_TEX_OFFSETS 32

struct Shader;
struct Texture;

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
    RS_BLEND            = RS_BLEND_ADD | RS_BLEND_ALPHA | RS_BLEND_MULT | RS_BLEND_PREMULT,
};

typedef uint16 Index;

struct Vertex {
    short4  coord;      // xyz  - position, w - joint index (for entities only)
    short4  normal;     // xyz  - vertex normal, w - unused
    short4  texCoord;   // xy   - texture coordinates, zw - trapezoid warping
    ubyte4  param;      // xy   - anim tex range and frame index, zw - unused
    ubyte4  color;      // for non-textured geometry
    ubyte4  light;      // xyz  - color, w - use premultiplied alpha
};

#ifdef FFP
    #ifdef _PSP
        struct VertexGPU {
            short2 texCoord;
            ubyte4 color;
            short3 normal;
            short3 coord;
        };
    #else
/*
        struct VertexGPU {
            short2 texCoord;
            ubyte4 color;
            short3 normal;
            uint16 _alignN;
            short3 coord;
            uint16 _alignC;
        };
*/
        typedef Vertex VertexGPU;
    #endif
#else
    typedef Vertex VertexGPU;
#endif

#ifdef PROFILE
   //#define USE_CV_MARKERS

   #ifdef USE_CV_MARKERS
       #include <libs/cvmarkers/cvmarkersobj.h>  
       using namespace Concurrency::diagnostic;

       marker_series *series[256];
       int seriesIndex;
   #endif

    struct Marker {
        #ifdef USE_CV_MARKERS
            span *cvSpan;
        #endif

        Marker(const char *title) {
            if (Core::support.profMarker) glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, title);
            #ifdef USE_CV_MARKERS
                marker_series *&s = series[seriesIndex];
                if (s == NULL) {
                   char seriesTitle[64];
                   sprintf(seriesTitle, "events - %d", seriesIndex);
                   s = new marker_series(seriesTitle);
                }
                cvSpan = new span(*s, normal_importance, _T(title));
                seriesIndex++;
            #endif
        }

        ~Marker() {
            if (Core::support.profMarker) glPopDebugGroup();
            #ifdef USE_CV_MARKERS
                delete cvSpan;
                seriesIndex--;
            #endif
        }

        static void setLabel(GLenum id, GLuint name, const char *label) {
            if (Core::support.profMarker) glObjectLabel(id, name, -1, label);
        }
    };

    struct Timing {
        GLuint  ID;
        int     &result;

        Timing(int &result) : result(result) {
            if (!Core::support.profTiming) return;
            glGenQueries(1, &ID);
            glBeginQuery(GL_TIME_ELAPSED, ID);
        }

        ~Timing() {
            if (!Core::support.profTiming) return;
            glEndQuery(GL_TIME_ELAPSED);
            glGetQueryObjectiv(ID, GL_QUERY_RESULT, (GLint*)&result);
            glDeleteQueries(1, &ID);
        }
    };

    #define PROFILE_MARKER(title)               Marker marker(title)
    #define PROFILE_LABEL(id, name, label)      Marker::setLabel(GL_##id, name, label)
    #define PROFILE_TIMING(result)              Timing timing(result)
#else
    #define PROFILE_MARKER(title)
    #define PROFILE_LABEL(id, name, label)
    #define PROFILE_TIMING(time)
#endif

enum CullFace  { cfNone, cfBack, cfFront };
enum BlendMode { bmNone, bmAlpha, bmAdd, bmMult, bmPremult };

namespace Core {
    float eye;
    vec4 viewport, viewportDef;
    mat4 mModel, mView, mProj, mViewProj, mViewInv, mLightProj;
    Basis basis;
    vec3 viewPos;
    vec4 lightPos[MAX_LIGHTS];
    vec4 lightColor[MAX_LIGHTS];
    vec4 params;
    vec4 contacts[MAX_CONTACTS];

    Texture *whiteTex;

    enum Pass { passCompose, passShadow, passAmbient, passWater, passFilter, passGUI, passMAX } pass;

    #ifdef _PSP
        void    *curBackBuffer;
    #else
        GLuint  FBO, defaultFBO;
        struct RenderTargetCache {
            int count;
            struct Item {
                GLuint  ID;
                int     width;
                int     height;
            } items[MAX_RENDER_BUFFERS];
        } rtCache[2];
    #endif

    Texture *defaultTarget;
    
    int32   renderState;

    struct Active {
        Shader      *shader;
        Texture     *textures[8];
        Texture     *target;
        vec4        viewport;
        vec4        material;
        uint32      targetFace;
    #ifdef _PSP
        Index       *iBuffer;
        VertexGPU   *vBuffer;
    #else
        GLuint      VAO;
        GLuint      iBuffer;
        GLuint      vBuffer;
    #endif
        int32       renderState;

        int32       basisCount;
        Basis       *basis;
    } active;
    
    struct ReqTarget {
        Texture *texture;
        bool    clear;
        uint8   face;
    } reqTarget;

    struct Stats {
        int dips, tris, frame, fps, fpsTime;
    #ifdef PROFILE
        int tFrame;
    #endif

        Stats() : frame(0), fps(0), fpsTime(0) {}

        void start() {
            dips = tris = 0;
        }

        void stop() {
            if (fpsTime < Core::getTime()) {
                LOG("FPS: %d DIP: %d TRI: %d\n", fps, dips, tris);
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

#ifdef _PSP
    uint32 *cmdBuf = NULL;

    static int EDRAM_OFFSET;
    static int EDRAM_SIZE;

    void* allocEDRAM(int size) {
        LOG("EDRAM ALLOC: offset: %d size %d\n", Core::EDRAM_OFFSET, size);
        if (Core::EDRAM_OFFSET + size > EDRAM_SIZE)
            LOG("! EDRAM overflow !\n");

        void *ptr = ((char*)sceGeEdramGetAddr()) + EDRAM_OFFSET;
        EDRAM_OFFSET += (size + 15) / 16 * 16;
        return ptr;
    }

    void freeEDRAM() {
        EDRAM_OFFSET = (512 * 272 * 2 * 2) + (512 * 272 * 2);
        LOG("EDRAM FREE: offset: %d\n", EDRAM_OFFSET);
    }
#endif

    void beginCmdBuf() {
    #ifdef _PSP
        if (!cmdBuf)
            cmdBuf = new uint32[262144];

        sceGuStart(GU_DIRECT, cmdBuf);
    #endif
    }

    void submitCmdBuf() {
    #ifdef _PSP
        ASSERT(cmdBuf);
        sceGuFinish();
        sceGuSync(GU_SYNC_WAIT, GU_SYNC_FINISH);
    #endif
    }
}

#include "texture.h"
#include "shader.h"

namespace Core {

    Texture *eyeTex[2];

    bool extSupport(const char *str, const char *ext) {
        return strstr(str, ext) != NULL;
    }

    void init() {
        #ifdef USE_INFLATE
            tinf_init();
        #endif

        isQuit = false;

        Input::init();
        #ifdef ANDROID
            void *libGL = dlopen("libGLESv2.so", RTLD_LAZY);
        #endif

        #if defined(WIN32) || (defined(LINUX) && !defined(__RPI__)) || defined(ANDROID)
            #ifdef WIN32
                GetProcOGL(glActiveTexture);
            #endif

            #ifdef WIN32
                GetProcOGL(wglSwapIntervalEXT);
            #elif LINUX
                GetProcOGL(glXSwapIntervalSGI);
            #endif

            #if defined(WIN32) || defined(LINUX)
                GetProcOGL(glGenerateMipmap);

                #ifdef PROFILE
                    GetProcOGL(glObjectLabel);
                    GetProcOGL(glPushDebugGroup);
                    GetProcOGL(glPopDebugGroup);
                    GetProcOGL(glGenQueries);
                    GetProcOGL(glDeleteQueries);
                    GetProcOGL(glGetQueryObjectiv);
                    GetProcOGL(glBeginQuery);
                    GetProcOGL(glEndQuery);
                #endif

                GetProcOGL(glCreateProgram);
                GetProcOGL(glDeleteProgram);
                GetProcOGL(glLinkProgram);
                GetProcOGL(glUseProgram);
                GetProcOGL(glGetProgramInfoLog);
                GetProcOGL(glCreateShader);
                GetProcOGL(glDeleteShader);
                GetProcOGL(glShaderSource);
                GetProcOGL(glAttachShader);
                GetProcOGL(glCompileShader);
                GetProcOGL(glGetShaderInfoLog);
                GetProcOGL(glGetUniformLocation);
                GetProcOGL(glUniform1iv);
                GetProcOGL(glUniform1fv);
                GetProcOGL(glUniform2fv);
                GetProcOGL(glUniform3fv);
                GetProcOGL(glUniform4fv);
                GetProcOGL(glUniformMatrix4fv);
                GetProcOGL(glBindAttribLocation);
                GetProcOGL(glEnableVertexAttribArray);
                GetProcOGL(glDisableVertexAttribArray);
                GetProcOGL(glVertexAttribPointer);
                GetProcOGL(glGetProgramiv);

                GetProcOGL(glGenFramebuffers);
                GetProcOGL(glBindFramebuffer);
                GetProcOGL(glGenRenderbuffers);
                GetProcOGL(glBindRenderbuffer);
                GetProcOGL(glFramebufferTexture2D);
                GetProcOGL(glFramebufferRenderbuffer);
                GetProcOGL(glRenderbufferStorage);
                GetProcOGL(glCheckFramebufferStatus);
                GetProcOGL(glDeleteFramebuffers);
                GetProcOGL(glDeleteRenderbuffers);

                GetProcOGL(glGenBuffers);
                GetProcOGL(glDeleteBuffers);
                GetProcOGL(glBindBuffer);
                GetProcOGL(glBufferData);
                GetProcOGL(glBufferSubData);
            #endif

            #if defined(ANDROID) || defined(__EMSCRIPTEN__)
                GetProcOGL(glDiscardFramebufferEXT);
            #endif

            GetProcOGL(glGenVertexArrays);
            GetProcOGL(glDeleteVertexArrays);
            GetProcOGL(glBindVertexArray);
            GetProcOGL(glGetProgramBinary);
            GetProcOGL(glProgramBinary);
        #endif

        const char *vendor, *renderer, *version;

    #ifdef _PSP
        vendor   = "Sony";
        renderer = "SCE GU";
        version  = "1.0";
    #else
        vendor   = (char*)glGetString(GL_VENDOR);
        renderer = (char*)glGetString(GL_RENDERER);
        version  = (char*)glGetString(GL_VERSION);

        char *ext = (char*)glGetString(GL_EXTENSIONS);
/*
        if (ext != NULL) {
            char buf[255];
            int len = strlen(ext);
            int start = 0;
            for (int i = 0; i < len; i++)
                if (ext[i] == ' ' || (i == len - 1)) {
                    memcpy(buf, &ext[start], i - start);
                    buf[i - start] = 0;
                    LOG("%s\n", buf);
                    start = i + 1;
                }
        }
*/
    #endif

    #ifdef FFP
        support.maxAniso       = 1;
        support.maxVectors     = 0;
        support.shaderBinary   = false;
        support.VAO            = false;
        support.depthTexture   = false;
        support.shadowSampler  = false;
        support.discardFrame   = false;
        support.texNPOT        = false;
        support.texRG          = false;
        support.texBorder      = false;
        support.maxAniso       = false;
        support.colorFloat     = false;
        support.colorHalf      = false;
        support.texFloatLinear = false;
        support.texFloat       = false;
        support.texHalfLinear  = false;
        support.texHalf        = false;
    #else
        support.shaderBinary   = extSupport(ext, "_program_binary");
        support.VAO            = extSupport(ext, "_vertex_array_object");
        support.depthTexture   = extSupport(ext, "_depth_texture");
        support.shadowSampler  = support.depthTexture && (extSupport(ext, "_shadow_samplers") || extSupport(ext, "GL_ARB_shadow"));
        support.discardFrame   = extSupport(ext, "_discard_framebuffer");
        support.texNPOT        = extSupport(ext, "_texture_npot") || extSupport(ext, "_texture_non_power_of_two");
        support.texRG          = extSupport(ext, "_texture_rg ");   // hope that isn't last extension in string ;)
        support.texBorder      = extSupport(ext, "_texture_border_clamp");
        support.maxAniso       = extSupport(ext, "_texture_filter_anisotropic");
        support.colorFloat     = extSupport(ext, "_color_buffer_float");
        support.colorHalf      = extSupport(ext, "_color_buffer_half_float") || extSupport(ext, "GL_ARB_half_float_pixel");
        support.texFloatLinear = support.colorFloat || extSupport(ext, "GL_ARB_texture_float") || extSupport(ext, "_texture_float_linear");
        support.texFloat       = support.texFloatLinear || extSupport(ext, "_texture_float");
        support.texHalfLinear  = support.colorHalf || extSupport(ext, "GL_ARB_texture_float") || extSupport(ext, "_texture_half_float_linear") || extSupport(ext, "_color_buffer_half_float");
        support.texHalf        = support.texHalfLinear || extSupport(ext, "_texture_half_float");

        if (support.maxAniso)
            glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &support.maxAniso);
        glGetIntegerv(GL_MAX_VARYING_VECTORS, &support.maxVectors);
    #endif

    #ifdef PROFILE
        support.profMarker     = extSupport(ext, "_KHR_debug");
        support.profTiming     = extSupport(ext, "_timer_query");
    #endif

        LOG("Vendor   : %s\n", vendor);
        LOG("Renderer : %s\n", renderer);
        LOG("Version  : %s\n", version);
        LOG("cache    : %s\n", Stream::cacheDir);
    #ifdef _PSP
        EDRAM_SIZE = sceGeEdramGetSize();
        LOG("VRAM     : %d\n", EDRAM_SIZE);
    #endif
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

    #ifdef FFP
        #ifdef _PSP
            Core::width  = 480;
            Core::height = 272;

            sceGuDepthFunc(GU_LEQUAL);
            sceGuDepthRange(0x0000, 0xFFFF);
            sceGuClearDepth(0xFFFF);

            sceGuShadeModel(GU_SMOOTH);
            sceGuAlphaFunc(GU_GREATER, 127, 255);
            sceGuEnable(GU_ALPHA_TEST);

            int swizzle = GU_FALSE;
            #ifdef TEX_SWIZZLE
                swizzle = GU_TRUE;
            #endif

            sceGuClutMode(GU_PSM_5551, 0, 0xFF, 0);
            sceGuTexMode(GU_PSM_T4, 0, 0, swizzle);
            sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
            sceGuTexScale(1.0f, 1.0f);
            sceGuTexOffset(0.0f, 0.0f);
            sceGuTexFilter(GU_LINEAR, GU_LINEAR);
            //sceGuTexFilter(GU_NEAREST, GU_NEAREST);
            sceGuEnable(GU_CLIP_PLANES);

            const ScePspIMatrix4 dith =
              { {-4,  0, -3,  1},
                { 2, -2,  3, -1},
                {-3,  1, -4,  0},
                { 3, -1,  2, -2} };
            sceGuSetDither(&dith);
            sceGuEnable(GU_DITHER);

            sceGuEnable(GU_LIGHT0);
            sceGuDisable(GU_LIGHT1);
            sceGuDisable(GU_LIGHT2);
            sceGuDisable(GU_LIGHT3);
            sceGuAmbientColor(0xFFFFFFFF);
            sceGuColor(0xFFFFFFFF);

            freeEDRAM();
        #else
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            glEnableClientState(GL_COLOR_ARRAY);
            glEnableClientState(GL_NORMAL_ARRAY);
            glEnableClientState(GL_VERTEX_ARRAY);
            
            glAlphaFunc(GL_GREATER, 0.5f);
            glEnable(GL_ALPHA_TEST);

            glEnable(GL_LIGHT0);
            glDisable(GL_LIGHT1);
            glDisable(GL_LIGHT2);
            glDisable(GL_LIGHT3);
            glEnable(GL_NORMALIZE);

            glMatrixMode(GL_TEXTURE);
            glLoadIdentity();
            glScalef(1.0f / 32767.0f, 1.0f / 32767.0f, 1.0f / 32767.0f);
        #endif
    #endif

    #ifndef _PSP
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&defaultFBO);
        glGenFramebuffers(1, &FBO);
        glDepthFunc(GL_LEQUAL);

        memset(rtCache, 0, sizeof(rtCache));
    #endif

    #if defined(FFP) && defined(SPLIT_BY_TILE)
        #ifdef _PSP
            sceGuEnable(GU_TEXTURE_2D);
        #else
            glEnable(GL_TEXTURE_2D);
        #endif
    #endif

        defaultTarget = NULL;

        Sound::init();

        for (int i = 0; i < MAX_LIGHTS; i++) {
            lightPos[i]   = vec3(0.0);
            lightColor[i] = vec4(0, 0, 0, 1);
        }
        eye = 0.0f;

        uint32 data = 0xFFFFFFFF;
        whiteTex = new Texture(1, 1, Texture::RGBA, Texture::NEAREST, &data);

    // init settings
        settings.detail.setFilter   (Core::Settings::HIGH);
        settings.detail.setLighting (Core::Settings::HIGH);
        settings.detail.setShadows  (Core::Settings::HIGH);
        settings.detail.setWater    (Core::Settings::HIGH);
        settings.detail.vsync         = true;
        settings.detail.stereo        = Settings::STEREO_OFF;
        settings.audio.music          = 14;
        settings.audio.sound          = 14;
        settings.audio.reverb         = true;

    // player 1
        {
            Settings::Controls &ctrl   = settings.controls[0];
            ctrl.retarget    = true;
            ctrl.multitarget = true;
            ctrl.vibration   = true;
            ctrl.joyIndex    = 0;

            ctrl.keys[ cLeft      ] = KeySet( ikLeft,   jkLeft   );
            ctrl.keys[ cRight     ] = KeySet( ikRight,  jkRight  );
            ctrl.keys[ cUp        ] = KeySet( ikUp,     jkUp     );
            ctrl.keys[ cDown      ] = KeySet( ikDown,   jkDown   );
            ctrl.keys[ cJump      ] = KeySet( ikAlt,    jkX      );
            ctrl.keys[ cWalk      ] = KeySet( ikShift,  jkRB     );
            ctrl.keys[ cAction    ] = KeySet( ikCtrl,   jkA      );
            ctrl.keys[ cWeapon    ] = KeySet( ikSpace,  jkY      );
            ctrl.keys[ cLook      ] = KeySet( ikC,      jkLB     );
            ctrl.keys[ cStepLeft  ] = KeySet( ikZ,      jkLT     );
            ctrl.keys[ cStepRight ] = KeySet( ikX,      jkRT     );
            ctrl.keys[ cRoll      ] = KeySet( ikA,      jkB      );
            ctrl.keys[ cInventory ] = KeySet( ikTab,    jkSelect );
            ctrl.keys[ cStart     ] = KeySet( ikEnter,  jkStart  );
        }

    // player 2
        {
            Settings::Controls &ctrl   = settings.controls[1];
            ctrl.retarget    = true;
            ctrl.multitarget = true;
            ctrl.vibration   = true;
            ctrl.joyIndex    = 1;

            ctrl.keys[ cLeft      ] = KeySet( ikNone,   jkLeft   );
            ctrl.keys[ cRight     ] = KeySet( ikNone,   jkRight  );
            ctrl.keys[ cUp        ] = KeySet( ikNone,   jkUp     );
            ctrl.keys[ cDown      ] = KeySet( ikNone,   jkDown   );
            ctrl.keys[ cJump      ] = KeySet( ikNone,   jkX      );
            ctrl.keys[ cWalk      ] = KeySet( ikNone,   jkRB     );
            ctrl.keys[ cAction    ] = KeySet( ikNone,   jkA      );
            ctrl.keys[ cWeapon    ] = KeySet( ikNone,   jkY      );
            ctrl.keys[ cLook      ] = KeySet( ikNone,   jkLB     );
            ctrl.keys[ cStepLeft  ] = KeySet( ikNone,   jkLT     );
            ctrl.keys[ cStepRight ] = KeySet( ikNone,   jkRT     );
            ctrl.keys[ cRoll      ] = KeySet( ikNone,   jkB      );
            ctrl.keys[ cInventory ] = KeySet( ikNone,   jkSelect );
            ctrl.keys[ cStart     ] = KeySet( ikEnter,  jkStart  );
        }

    // use D key for jump in browsers
    #ifdef __EMSCRIPTEN__
        settings.controls[0].keys[cJump].key = ikD;
    #endif

    #ifdef __RPI__
        settings.detail.setShadows(Core::Settings::LOW);
    #endif

    #ifdef FFP
        settings.detail.setFilter   (Core::Settings::MEDIUM);
        settings.detail.setLighting (Core::Settings::LOW);
        settings.detail.setShadows  (Core::Settings::LOW);
        settings.detail.setWater    (Core::Settings::LOW);
        settings.audio.reverb = false;
    #endif

        eyeTex[0] = eyeTex[1] = NULL;

        resetTime();
    }

    void deinit() {
        delete eyeTex[0];
        delete eyeTex[1];
        delete whiteTex;
    #ifdef _PSP
        delete[] cmdBuf;
    #else
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &FBO);

        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        for (int b = 0; b < 2; b++)
            for (int i = 0; i < rtCache[b].count; i++)
                glDeleteRenderbuffers(1, &rtCache[b].items[i].ID);
    #endif
        Sound::deinit();
    }

#ifdef VR_SUPPORT
    void initVR(int width, int height) {
        eyeTex[0] = new Texture(width, height, Texture::RGBA);
        eyeTex[1] = new Texture(width, height, Texture::RGBA);
    }
#endif

#ifndef _PSP
    int cacheRenderTarget(bool depth, int width, int height) {
        RenderTargetCache &cache = rtCache[depth];

        for (int i = 0; i < cache.count; i++)
            if (cache.items[i].width == width && cache.items[i].height == height)
                return i;

        ASSERT(cache.count < MAX_RENDER_BUFFERS);

        RenderTargetCache::Item &item = cache.items[cache.count];
        item.width  = width;
        item.height = height;

        glGenRenderbuffers(1, &item.ID);
        glBindRenderbuffer(GL_RENDERBUFFER, item.ID);
        glRenderbufferStorage(GL_RENDERBUFFER, depth ? GL_RGB565 : GL_DEPTH_COMPONENT16, width, height);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        return cache.count++;
    }
#endif

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
            Texture *target = reqTarget.texture;
            uint8   face    = reqTarget.face;

            if (target != active.target || face != active.targetFace) {
            #ifdef _PSP
/*
                if (!target)
                    sceGuDrawBufferList(GU_PSM_5650, curBackBuffer, 512);
                else
                    sceGuDrawBufferList(GU_PSM_5650, target->offset, target->width);
*/
            #else
                if (!target) { // may be a null
                    glBindFramebuffer(GL_FRAMEBUFFER, defaultFBO);
                } else {
                    GLenum texTarget = GL_TEXTURE_2D;
                    if (target->opt & Texture::CUBEMAP) 
                        texTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X + face;

                    bool depth   = target->format == Texture::DEPTH || target->format == Texture::SHADOW;
                    int  rtIndex = cacheRenderTarget(depth, target->width, target->height);

                    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
                    glFramebufferTexture2D    (GL_FRAMEBUFFER, depth ? GL_DEPTH_ATTACHMENT  : GL_COLOR_ATTACHMENT0, texTarget,       target->ID, 0);
                    glFramebufferRenderbuffer (GL_FRAMEBUFFER, depth ? GL_COLOR_ATTACHMENT0 : GL_DEPTH_ATTACHMENT,  GL_RENDERBUFFER, rtCache[depth].items[rtIndex].ID);
                }
            #endif

                active.target     = target;
                active.targetFace = face;
            }
        }

        if (mask & RS_VIEWPORT) {
            if (viewport != active.viewport) {
                active.viewport = viewport;
            #ifdef _PSP
                sceGuOffset(2048 - int(viewport.z) / 2, 2048 - int(viewport.w) / 2);
                sceGuViewport(2048 + int(viewport.x), 2048 + int(viewport.y), int(viewport.z), int(viewport.w));
            #else
                glViewport(int(viewport.x), int(viewport.y), int(viewport.z), int(viewport.w));
            #endif
            }
            renderState &= ~RS_VIEWPORT;
        }

        if (mask & RS_DEPTH_TEST) {
        #ifdef _PSP
            if (renderState & RS_DEPTH_TEST)
                sceGuEnable(GU_DEPTH_TEST);
            else
                sceGuDisable(GU_DEPTH_TEST);
        #else
            if (renderState & RS_DEPTH_TEST)
                glEnable(GL_DEPTH_TEST);
            else
                glDisable(GL_DEPTH_TEST);
        #endif
        }
        
        if (mask & RS_DEPTH_WRITE) {
        #ifdef _PSP
            sceGuDepthMask((renderState & RS_DEPTH_WRITE) != 0 ? GU_FALSE : GU_TRUE);
        #else
            glDepthMask((renderState & RS_DEPTH_WRITE) != 0 ? GL_TRUE : GL_FALSE);
        #endif
        }

        if (mask & RS_COLOR_WRITE) {
        #ifdef _PSP
            sceGuPixelMask(~(((renderState & RS_COLOR_WRITE_R) != 0 ? 0x000000FF : 0) |
                             ((renderState & RS_COLOR_WRITE_G) != 0 ? 0x0000FF00 : 0) |
                             ((renderState & RS_COLOR_WRITE_B) != 0 ? 0x00FF0000 : 0) |
                             ((renderState & RS_COLOR_WRITE_A) != 0 ? 0xFF000000 : 0)));
        #else
            glColorMask((renderState & RS_COLOR_WRITE_R) != 0,
                        (renderState & RS_COLOR_WRITE_G) != 0,
                        (renderState & RS_COLOR_WRITE_B) != 0,
                        (renderState & RS_COLOR_WRITE_A) != 0);
        #endif
        }

        if (mask & RS_CULL) {
        #ifdef _PSP
            if (!(active.renderState & RS_CULL))
                sceGuEnable(GU_CULL_FACE);
            switch (renderState & RS_CULL) {
                case RS_CULL_BACK  : sceGuFrontFace(GU_CCW);  break;
                case RS_CULL_FRONT : sceGuFrontFace(GU_CW); break;
                default            : sceGuDisable(GU_CULL_FACE);
            }
        #else
            if (!(active.renderState & RS_CULL))
                glEnable(GL_CULL_FACE);
            switch (renderState & RS_CULL) {
                case RS_CULL_BACK  : glCullFace(GL_BACK);  break;
                case RS_CULL_FRONT : glCullFace(GL_FRONT); break;
                default            : glDisable(GL_CULL_FACE);
            }
        #endif
        }

        if (mask & RS_BLEND) {
        #ifdef _PSP
            if (!(active.renderState & RS_BLEND))
                sceGuEnable(GU_BLEND);
            switch (renderState & RS_BLEND) {
                case RS_BLEND_ALPHA    : sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);    break;
                case RS_BLEND_ADD      : sceGuBlendFunc(GU_ADD, GU_FIX, GU_FIX, 0xffffffff, 0xffffffff);        break;
                case RS_BLEND_MULT     : sceGuBlendFunc(GU_ADD, GU_DST_COLOR, GU_FIX, 0, 0);                    break;
                case RS_BLEND_PREMULT  : sceGuBlendFunc(GU_ADD, GU_FIX, GU_ONE_MINUS_SRC_ALPHA, 0xffffffff, 0); break;
                default                : sceGuDisable(GU_BLEND);
            }
        #else
            if (!(active.renderState & RS_BLEND))
                glEnable(GL_BLEND);
            switch (renderState & RS_BLEND) {
                case RS_BLEND_ALPHA    : glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); break;
                case RS_BLEND_ADD      : glBlendFunc(GL_ONE, GL_ONE);                       break;
                case RS_BLEND_MULT     : glBlendFunc(GL_DST_COLOR, GL_ZERO);                break;
                case RS_BLEND_PREMULT  : glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);       break;
                default                : glDisable(GL_BLEND);
            }
        #endif
        }

        if (mask & RS_TARGET) {
            if (reqTarget.clear) {
            #ifdef _PSP
                sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT | GU_FAST_CLEAR_BIT);
            #else
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
            #endif
            }
            renderState &= ~RS_TARGET;
        }

        active.renderState = renderState;
    }

    void setClearColor(const vec4 &color) {
    #ifdef _PSP
        ubyte4 c(clamp(int(color.x * 255), 0, 255),
                 clamp(int(color.y * 255), 0, 255),
                 clamp(int(color.z * 255), 0, 255),
                 clamp(int(color.w * 255), 0, 255));
        sceGuClearColor(*((uint32*)&c));
    #else
        glClearColor(color.x, color.y, color.z, color.w);
    #endif
    }

    void setViewport(int x, int y, int width, int height) {
        viewport = vec4(float(x), float(y), float(width), float(height));
        renderState |= RS_VIEWPORT;
    }

    void setCulling(CullFace mode) {
        renderState &= ~RS_CULL;
        switch (mode) {
            case cfNone  : break;
            case cfBack  : renderState |= RS_CULL_BACK;  break;
            case cfFront : renderState |= RS_CULL_FRONT; break;
        }
    }

    void setBlending(BlendMode mode) {
        renderState &= ~RS_BLEND;
        switch (mode) {
            case bmNone     : break;
            case bmAlpha    : renderState |= RS_BLEND_ALPHA;   break;
            case bmAdd      : renderState |= RS_BLEND_ADD;     break;
            case bmMult     : renderState |= RS_BLEND_MULT;    break;
            case bmPremult  : renderState |= RS_BLEND_PREMULT; break;
        }
    }

    void setColorWrite(bool r, bool g, bool b, bool a) {
        renderState &= ~RS_COLOR_WRITE;
        if (r) renderState |= RS_COLOR_WRITE_R;
        if (g) renderState |= RS_COLOR_WRITE_G;
        if (b) renderState |= RS_COLOR_WRITE_B;
        if (a) renderState |= RS_COLOR_WRITE_A;
    }

    void setDepthWrite(bool write) {
        if (write)
            renderState |= RS_DEPTH_WRITE;
        else
            renderState &= ~RS_DEPTH_WRITE;
    }

    void setDepthTest(bool test) {
        if (test)
            renderState |= RS_DEPTH_TEST;
        else
            renderState &= ~RS_DEPTH_TEST;
    }

    void invalidateTarget(bool color, bool depth) {
    #ifdef MOBILE
        if (support.discardFrame && (color || depth)) {
            int count = 0;
            GLenum discard[2];
            if (color) discard[count++] = active.target ? GL_COLOR_ATTACHMENT0 : GL_COLOR_EXT;
            if (depth) discard[count++] = active.target ? GL_DEPTH_ATTACHMENT  : GL_DEPTH_EXT;
            glDiscardFramebufferEXT(GL_FRAMEBUFFER, count, discard);
        }
    #endif
    }

    void setTarget(Texture *target, bool clear = false, int face = 0) {
        if (!target)
            target = defaultTarget;

        bool color = !target || (target->format != Texture::DEPTH && target->format != Texture::SHADOW);
        setColorWrite(color, color, color, color);

        if (target == defaultTarget) // backbuffer
            setViewport(int(viewportDef.x), int(viewportDef.y), int(viewportDef.z), int(viewportDef.w));
        else
            setViewport(0, 0, target->width, target->height);

        reqTarget.texture = target;
        reqTarget.clear   = clear;
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
        dst->bind(sDiffuse);
    #ifdef _PSP
    
    #else
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, xOffset, yOffset, x, y, width, height); // TODO: too bad for iOS devices!
    #endif
    }

    vec4 copyPixel(int x, int y) { // GPU sync!
        validateRenderState();
    #ifdef _PSP
        return vec4(0.0f);
    #else
        ubyte4 c;
        glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &c);
        return vec4(float(c.x), float(c.y), float(c.z), float(c.w)) * (1.0f / 255.0f);
    #endif
    }

    void beginFrame() {
        //memset(&active, 0, sizeof(active));        
        setViewport(0, 0, Core::width, Core::height);
        viewportDef = viewport;
        setCulling(cfFront);
        setBlending(bmAlpha);
        setDepthTest(true);
        setDepthWrite(true);
        setColorWrite(true, true, true, true);

        Core::stats.start();
    }

    void endFrame() {
    #ifdef __EMSCRIPTEN__
        glColorMask(false, false, false, true);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glColorMask(true, true, true, true);
    #endif
        Core::stats.stop();
    }

    void waitVBlank() {
        if (Core::settings.detail.vsync) {
        #ifdef WIN32
            if (wglSwapIntervalEXT) wglSwapIntervalEXT(1);
        #elif LINUX
            if (glXSwapIntervalSGI) glXSwapIntervalSGI(1);
        #elif _PSP
            sceDisplayWaitVblankStart();
        #endif
        } else {
        #ifdef WIN32
            if (wglSwapIntervalEXT) wglSwapIntervalEXT(0);
        #elif LINUX
            if (glXSwapIntervalSGI) glXSwapIntervalSGI(0);
        #endif
        }
    }

    void setViewProj(const mat4 &mView, const mat4 &mProj) {
        Core::mProj     = mProj;
        Core::mView     = mView;
        Core::mViewProj = mProj * mView;
    #ifdef FFP
        #ifdef _PSP
            sceGumMatrixMode(GU_PROJECTION);
            sceGumLoadMatrix((ScePspFMatrix4*)&mProj);
            sceGumMatrixMode(GU_VIEW);
            sceGumLoadMatrix((ScePspFMatrix4*)&mView);
        #else
            glMatrixMode(GL_PROJECTION);
            glLoadMatrixf((float*)&mProj);
        #endif
    #endif
    }

    void DIP(int iStart, int iCount) {
        validateRenderState();

    #ifdef FFP
        #ifdef _PSP
            mat4 m = mModel;
            m.scale(vec3(32767.0f));
            sceGumMatrixMode(GU_MODEL);
            sceGumLoadMatrix((ScePspFMatrix4*)&m);
        #else
            mat4 m = mView * mModel;
            glMatrixMode(GL_MODELVIEW);
            glLoadMatrixf((GLfloat*)&m);
        #endif
    #endif

    #ifdef _PSP
        sceGumDrawArray(GU_TRIANGLES, GU_TEXTURE_16BIT | GU_COLOR_8888 | GU_NORMAL_16BIT | GU_VERTEX_16BIT | GU_INDEX_16BIT | GU_TRANSFORM_3D, iCount, active.iBuffer + iStart, active.vBuffer);
    #else
        glDrawElements(GL_TRIANGLES, iCount, GL_UNSIGNED_SHORT, (Index*)NULL + iStart);
    #endif

        stats.dips++;
        stats.tris += iCount / 3;
    }
}

#include "mesh.h"

#endif
