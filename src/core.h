#ifndef H_CORE
#define H_CORE

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
    #include <GL/glx.h>
    #include <GL/glext.h>
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
#endif

#include "utils.h"

enum ControlKey { cLeft, cRight, cUp, cDown, cJump, cWalk, cAction, cWeapon, cLook, cStepLeft, cStepRight, cRoll, cInventory, cMAX };

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
    // gamepad
        ikJoyA, ikJoyB, ikJoyX, ikJoyY, ikJoyLB, ikJoyRB, ikJoySelect, ikJoyStart, ikJoyL, ikJoyR, ikJoyLT, ikJoyRT, ikJoyPOV,
        ikJoyLeft, ikJoyRight, ikJoyUp, ikJoyDown,
        ikMAX };

struct KeySet {
    InputKey key, joy;
};

namespace Core {
    float deltaTime;
    int width, height;

    struct {
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
        enum Quality : uint8 { LOW, MEDIUM, HIGH };

        union {
            struct {
                Quality filter;
                Quality lighting;
                Quality shadows;
                Quality water;
            };
            Quality quality[4];

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

        struct {
            KeySet keys[cMAX];
            bool retarget;
            bool multitarget;
            bool vibration;
        } controls;

        struct {
            float music;
            float sound;
            bool reverb;
        } audio;
    } settings;
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

enum RenderState : int32 {
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
    RS_BLEND_MULTIPLY   = 1 << 12,
    RS_BLEND_SCREEN     = 1 << 13,
    RS_BLEND            = RS_BLEND_ADD | RS_BLEND_ALPHA | RS_BLEND_MULTIPLY | RS_BLEND_SCREEN,
};

typedef unsigned short Index;

struct Vertex {
    short4  coord;      // xyz  - position, w - joint index (for entities only)
    short4  normal;     // xyz  - vertex normal, w - unused
    short4  texCoord;   // xy   - texture coordinates, zw - trapezoid warping
    ubyte4  param;      // xy   - anim tex range and frame index, zw - unused
    ubyte4  color;      // xyz  - color, w - intensity
};

#ifdef PROFILE
   #define USE_CV_MARKERS

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

enum CullMode  { cfNone, cfBack, cfFront };
enum BlendMode { bmNone, bmAlpha, bmAdd, bmMultiply, bmScreen };

extern int getTime();

namespace Core {
    float eye;
    vec4 viewport, viewportDef;
    mat4 mView, mProj, mViewProj, mViewInv, mLightProj;
    Basis basis;
    vec3 viewPos;
    vec3 lightPos[MAX_LIGHTS];
    vec4 lightColor[MAX_LIGHTS];
    vec4 params;
    vec4 contacts[MAX_CONTACTS];

    Texture *blackTex, *whiteTex;

    enum Pass { passCompose, passShadow, passAmbient, passWater, passFilter, passGUI, passMAX } pass;

    GLuint FBO, defaultFBO;
    Texture *defaultTarget;

    struct RenderTargetCache {
        int count;
        struct Item {
            GLuint  ID;
            int     width;
            int     height;
        } items[MAX_RENDER_BUFFERS];
    } rtCache[2];

    int32 renderState;

    struct {
        Shader      *shader;
        Texture     *textures[8];
        Texture     *target;
        int         targetFace;
        vec4        viewport;
        GLuint      VAO;
        GLuint      iBuffer;
        GLuint      vBuffer;
        int32       renderState;
    } active;
    
    struct {
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
            if (fpsTime < getTime()) {
                LOG("FPS: %d DIP: %d TRI: %d\n", fps, dips, tris);
            #ifdef PROFILE
                LOG("frame time: %d mcs\n", tFrame / 1000);
            #endif
                fps     = frame;
                frame   = 0;
                fpsTime = getTime() + 1000;
            } else
                frame++;        
        }
    } stats;
}

#include "texture.h"
#include "shader.h"

namespace Core {

    bool extSupport(const char *str, const char *ext) {
        return strstr(str, ext) != NULL;
    }

    void init() {
        Input::init();
        #ifdef ANDROID
            void *libGL = dlopen("libGLESv2.so", RTLD_LAZY);
        #endif

        #if defined(WIN32) || (defined(LINUX) && !defined(__RPI__)) || defined(ANDROID)
            #ifdef WIN32
                GetProcOGL(glActiveTexture);
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
        glGetIntegerv(GL_MAX_VARYING_VECTORS, &support.maxVectors);

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

    #ifdef PROFILE
        support.profMarker     = extSupport(ext, "_KHR_debug");
        support.profTiming     = extSupport(ext, "_timer_query");
    #endif
        char *vendor = (char*)glGetString(GL_VENDOR);
        LOG("Vendor   : %s\n", vendor);
        LOG("Renderer : %s\n", glGetString(GL_RENDERER));
        LOG("Version  : %s\n", glGetString(GL_VERSION));
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

        glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&defaultFBO);
        glGenFramebuffers(1, &FBO);
        
        memset(rtCache, 0, sizeof(rtCache));
        defaultTarget = NULL;

        Sound::init();

        for (int i = 0; i < MAX_LIGHTS; i++) {
            lightPos[i]   = vec3(0.0);
            lightColor[i] = vec4(0, 0, 0, 1);
        }
        eye = 0.0f;

        uint32 data = 0x00000000;
        blackTex = new Texture(1, 1, Texture::RGBA, false, &data, false);
        data = 0xFFFFFFFF;
        whiteTex = new Texture(1, 1, Texture::RGBA, false, &data, false);

    // init settings
        settings.detail.setFilter   (Core::Settings::HIGH);
        settings.detail.setLighting (Core::Settings::HIGH);
        settings.detail.setShadows  (Core::Settings::MEDIUM);
        settings.detail.setWater    (Core::Settings::HIGH);

        settings.audio.music          = 0.7f;
        settings.audio.sound          = 0.7f;
        settings.audio.reverb         = true;

        settings.controls.retarget    = true;
        settings.controls.multitarget = true;
        settings.controls.vibration   = true;

        settings.controls.keys[ cLeft      ] = { ikLeft,   ikJoyLeft   };
        settings.controls.keys[ cRight     ] = { ikRight,  ikJoyRight  };
        settings.controls.keys[ cUp        ] = { ikUp,     ikJoyUp     };
        settings.controls.keys[ cDown      ] = { ikDown,   ikJoyDown   };
        settings.controls.keys[ cJump      ] = { ikD,      ikJoyX      };
        settings.controls.keys[ cWalk      ] = { ikShift,  ikJoyRB     };
        settings.controls.keys[ cAction    ] = { ikCtrl,   ikJoyA      };
        settings.controls.keys[ cWeapon    ] = { ikSpace,  ikJoyY      };
        settings.controls.keys[ cLook      ] = { ikC,      ikJoyLB     };
        settings.controls.keys[ cStepLeft  ] = { ikZ,      ikJoyLT     };
        settings.controls.keys[ cStepRight ] = { ikX,      ikJoyRT     };
        settings.controls.keys[ cRoll      ] = { ikA,      ikJoyB      };
        settings.controls.keys[ cInventory ] = { ikTab,    ikJoySelect };

#ifdef __RPI__
        settings.detail.setShadows(Core::Settings::LOW);
#endif
    }

    void free() {
        delete blackTex;
        delete whiteTex;
    /*
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFrameBuffers(1, &FBO);

        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        for (int b = 0; b < 2; b++)
            for (int i = 0; i < rtCache[b].count; i++)
                glDeleteRenderBuffers(1, &rtCache[b].items[i].ID);
    */
        Sound::free();
    }

    int cacheRenderTarget(bool depth, int width, int height) {
        RenderTargetCache &cache = rtCache[depth];

        for (int i = 0; i < cache.count; i++)
            if (cache.items[i].width == width && cache.items[i].height == height)
                return i;

        ASSERT(cache.count < MAX_RENDER_BUFFERS);

        RenderTargetCache::Item &item = cache.items[cache.count];

        glGenRenderbuffers(1, &item.ID);
        item.width  = width;
        item.height = height;

        glBindRenderbuffer(GL_RENDERBUFFER, item.ID);
        glRenderbufferStorage(GL_RENDERBUFFER, depth ? GL_RGB565 : GL_DEPTH_COMPONENT16, width, height);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        
        return cache.count++;
    }

    void validateRenderState() {
        int32 mask = renderState ^ active.renderState;
        if (!mask) return;

        if (mask & RS_TARGET) {
            Texture *target = reqTarget.texture;
            uint8   face    = reqTarget.face;

            if (target != active.target || face != active.targetFace) {
                
                if (!target) { // may be a null
                    glBindFramebuffer(GL_FRAMEBUFFER, defaultFBO);
                } else {
                    GLenum texTarget = GL_TEXTURE_2D;
                    if (target->cube) 
                        texTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X + face;

                    bool depth   = target->format == Texture::DEPTH || target->format == Texture::SHADOW;
                    int  rtIndex = cacheRenderTarget(depth, target->width, target->height);

                    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
                    glFramebufferTexture2D    (GL_FRAMEBUFFER, depth ? GL_DEPTH_ATTACHMENT  : GL_COLOR_ATTACHMENT0, texTarget,       target->ID, 0);
                    glFramebufferRenderbuffer (GL_FRAMEBUFFER, depth ? GL_COLOR_ATTACHMENT0 : GL_DEPTH_ATTACHMENT,  GL_RENDERBUFFER, rtCache[depth].items[rtIndex].ID);
                }

                active.target     = target;
                active.targetFace = face;
            }
        }

        if (mask & RS_VIEWPORT) {
            if (viewport != active.viewport) {
                active.viewport = viewport;
                glViewport(int(viewport.x), int(viewport.y), int(viewport.z), int(viewport.w));
            }
            renderState &= ~RS_VIEWPORT;
        }

        if (mask & RS_DEPTH_TEST) {
            if (renderState & RS_DEPTH_TEST)
                glEnable(GL_DEPTH_TEST);
            else
                glDisable(GL_DEPTH_TEST);
        }
        
        if (mask & RS_DEPTH_WRITE) {
            glDepthMask((renderState & RS_DEPTH_WRITE) != 0);
        }

        if (mask & RS_COLOR_WRITE) {
            glColorMask((renderState & RS_COLOR_WRITE_R) != 0,
                        (renderState & RS_COLOR_WRITE_G) != 0,
                        (renderState & RS_COLOR_WRITE_B) != 0,
                        (renderState & RS_COLOR_WRITE_A) != 0);
        }

        if (mask & RS_CULL) {
            if (!(active.renderState & RS_CULL))
                glEnable(GL_CULL_FACE);
            switch (renderState & RS_CULL) {
                case RS_CULL_BACK  : glCullFace(GL_BACK);  break;
                case RS_CULL_FRONT : glCullFace(GL_FRONT); break;
                default            : glDisable(GL_CULL_FACE);
            }
        }

        if (mask & RS_BLEND) {
            if (!(active.renderState & RS_BLEND))
                glEnable(GL_BLEND);
            switch (renderState & RS_BLEND) {
                case RS_BLEND_ALPHA    : glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); break;
                case RS_BLEND_ADD      : glBlendFunc(GL_ONE, GL_ONE);                       break;
                case RS_BLEND_MULTIPLY : glBlendFunc(GL_DST_COLOR, GL_ZERO);                break;
                case RS_BLEND_SCREEN   : glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);       break;
                default                : glDisable(GL_BLEND);
            }
        }

        if (mask & RS_TARGET) { // for cler the RT & reset mask
            if (reqTarget.clear)
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
            renderState &= ~RS_TARGET;
        }

        active.renderState = renderState;
    }

    void setClearColor(const vec4 &color) {
        glClearColor(color.x, color.y, color.z, color.w);
    }

    void setViewport(int x, int y, int width, int height) {
        viewport = vec4(float(x), float(y), float(width), float(height));
        renderState |= RS_VIEWPORT;
    }

    void setCulling(CullMode mode) {
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
            case bmAlpha    : renderState |= RS_BLEND_ALPHA;    break;
            case bmAdd      : renderState |= RS_BLEND_ADD;      break;
            case bmMultiply : renderState |= RS_BLEND_MULTIPLY; break;
            case bmScreen   : renderState |= RS_BLEND_SCREEN;   break;
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

        if (!target) // backbuffer
            setViewport(int(viewportDef.x), int(viewportDef.y), int(viewportDef.z), int(viewportDef.w));
        else
            setViewport(0, 0, target->width, target->height);

        reqTarget.texture = target;
        reqTarget.clear   = clear;
        reqTarget.face    = face;
        renderState |= RS_TARGET;
    }

    void copyTarget(Texture *dst, int xOffset, int yOffset, int x, int y, int width, int height) {
        validateRenderState();
        dst->bind(sDiffuse);
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, xOffset, yOffset, x, y, width, height); // TODO: too bad for iOS devices!
    }

    vec4 copyPixel(int x, int y) { // GPU sync!
        validateRenderState();
        ubyte4 c;
        glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &c);
        return vec4(float(c.x), float(c.y), float(c.z), float(c.w)) * (1.0f / 255.0f);
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

    void DIP(int iStart, int iCount) {
        validateRenderState();
        glDrawElements(GL_TRIANGLES, iCount, GL_UNSIGNED_SHORT, (Index*)NULL + iStart);
        stats.dips++;
        stats.tris += iCount / 3;
    }
}

#include "mesh.h"

#endif
