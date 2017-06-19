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

    #define GL_TEXTURE_COMPARE_MODE		0x884C
    #define GL_TEXTURE_COMPARE_FUNC		0x884D
    #define GL_COMPARE_REF_TO_TEXTURE	0x884E

    #undef  GL_RGBA32F
    #undef  GL_RGBA16F
    #undef  GL_HALF_FLOAT

    #define GL_RGBA32F      GL_RGBA
    #define GL_RGBA16F      GL_RGBA
    #define GL_HALF_FLOAT   GL_HALF_FLOAT_OES

    #define GL_DEPTH_STENCIL        GL_DEPTH_STENCIL_OES
    #define GL_UNSIGNED_INT_24_8    GL_UNSIGNED_INT_24_8_OES

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
    #define GL_STENCIL_TEST_TWO_SIDE_EXT 0
    #define glActiveStencilFaceEXT(...)
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

        #define GL_CLAMP_TO_BORDER          GL_CLAMP_TO_BORDER_EXT
        #define GL_TEXTURE_BORDER_COLOR     GL_TEXTURE_BORDER_COLOR_EXT

        #define GL_TEXTURE_COMPARE_MODE		GL_TEXTURE_COMPARE_MODE_EXT
        #define GL_TEXTURE_COMPARE_FUNC		GL_TEXTURE_COMPARE_FUNC_EXT
        #define GL_COMPARE_REF_TO_TEXTURE	GL_COMPARE_REF_TO_TEXTURE_EXT

        #define GL_STENCIL_TEST_TWO_SIDE_EXT 0
        #define glActiveStencilFaceEXT(...)
    #else
        #include <Carbon/Carbon.h>
        #include <AudioToolbox/AudioQueue.h>
        #include <OpenGL/OpenGL.h>
        #include <OpenGL/gl.h>
        #include <OpenGL/glext.h>
        #include <AGL/agl.h>

        #define GL_RGBA32F                  GL_RGBA
        #define GL_RGBA16F                  GL_RGBA

        #define GL_RGB565                   GL_RGBA
        #define GL_TEXTURE_COMPARE_MODE		0x884C
        #define GL_TEXTURE_COMPARE_FUNC		0x884D
        #define GL_COMPARE_REF_TO_TEXTURE	0x884E

        #define glGenVertexArrays    glGenVertexArraysAPPLE
        #define glDeleteVertexArrays glDeleteVertexArraysAPPLE
        #define glBindVertexArray    glBindVertexArrayAPPLE

        #define GL_PROGRAM_BINARY_LENGTH 0
        #define glGetProgramBinary(...)  0
        #define glProgramBinary(...)     0
    #endif
#elif __EMSCRIPTEN__
    #define MOBILE
    #include <emscripten.h>
    #include <html5.h>
    #include <GLES3/gl3.h>
    #include <GLES3/gl2ext.h>
    
    #undef GL_RGBA32F
    #undef GL_RGBA16F
    #undef GL_HALF_FLOAT

    #define GL_RGBA32F      GL_RGBA
    #define GL_RGBA16F      GL_RGBA
    #define GL_HALF_FLOAT   GL_HALF_FLOAT_OES

    #define GL_CLAMP_TO_BORDER          GL_CLAMP_TO_BORDER_EXT
    #define GL_TEXTURE_BORDER_COLOR     GL_TEXTURE_BORDER_COLOR_EXT

    #define GL_STENCIL_TEST_TWO_SIDE_EXT 0
    #define glGetProgramBinary(...)
    #define glProgramBinary(...)
    #define glActiveStencilFaceEXT(...)
#endif

namespace Core {
    float deltaTime;
    int width, height;
}

#include "utils.h"
#include "input.h"
#include "sound.h"

#if defined(WIN32) || defined(LINUX) || defined(ANDROID)

    #ifdef ANDROID
        #define GetProc(x) dlsym(libGL, x);
    #else
        void* GetProc(const char *name) {
            #ifdef WIN32
                return (void*)wglGetProcAddress(name);
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
    // Stencil
        PFNGLACTIVESTENCILFACEEXTPROC       glActiveStencilFaceEXT;
        PFNGLSTENCILFUNCSEPARATEPROC        glStencilFuncSeparate;
        PFNGLSTENCILOPSEPARATEPROC          glStencilOpSeparate;
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

#define MAX_LIGHTS          4
#define MAX_CACHED_LIGHTS   3
#define MAX_RENDER_BUFFERS  32

struct Shader;
struct Texture;

namespace Core {
    struct {
        bool shaderBinary;
        bool VAO;
        bool depthTexture;
        bool shadowSampler;
        bool discardFrame;
        bool texNPOT;
        bool texRG;
        bool texBorder;
        bool texFloat, texFloatLinear;
        bool texHalf,  texHalfLinear;
        char stencil;
    #ifdef PROFILE
        bool profMarker;
        bool profTiming;
    #endif
    } support;
}

#ifdef PROFILE
    struct Marker {
        Marker(const char *title) {
            if (Core::support.profMarker) glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, title);
        }

        ~Marker() {
            if (Core::support.profMarker) glPopDebugGroup();
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
    vec4 scissor;
    mat4 mView, mProj, mViewProj, mViewInv, mLightProj;
    Basis basis;
    vec3 viewPos;
    vec3 lightPos[MAX_LIGHTS];
    vec4 lightColor[MAX_LIGHTS];
    vec4 params;

    Texture *blackTex, *whiteTex;

    enum Pass { passCompose, passShadow, passAmbient, passWater, passFilter, passVolume, passGUI, passMAX } pass;

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

    struct {
        Shader      *shader;
        Texture     *textures[8];
        Texture     *target;
        int         targetFace;
        GLuint      VAO;
        GLuint      iBuffer;
        GLuint      vBuffer;
        BlendMode   blendMode;
        CullMode    cullMode;
        bool        stencilTwoSide;
    } active;

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

    struct {
        bool ambient;
        bool lighting;
        bool shadows;
        bool water;
    } settings;
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

        #if defined(WIN32) || defined(LINUX) || defined(ANDROID)
            #ifdef WIN32
                GetProcOGL(glActiveTexture);
            #endif

            #if defined(WIN32) || defined(LINUX)
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
                
                GetProcOGL(glActiveStencilFaceEXT);
                GetProcOGL(glStencilFuncSeparate);
                GetProcOGL(glStencilOpSeparate);
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
        //LOG("%s\n", ext);

        support.shaderBinary   = extSupport(ext, "_program_binary");
        support.VAO            = extSupport(ext, "_vertex_array_object");
        support.depthTexture   = extSupport(ext, "_depth_texture");
        support.shadowSampler  = support.depthTexture && (extSupport(ext, "_shadow_samplers") || extSupport(ext, "GL_ARB_shadow"));
        support.discardFrame   = extSupport(ext, "_discard_framebuffer");
        support.texNPOT        = extSupport(ext, "_texture_npot") || extSupport(ext, "_texture_non_power_of_two");
        support.texRG          = extSupport(ext, "_texture_rg ");   // hope that isn't last extension in string ;)
        support.texBorder      = extSupport(ext, "_texture_border_clamp");
        support.texFloatLinear = extSupport(ext, "GL_ARB_texture_float") || extSupport(ext, "_texture_float_linear");
        support.texFloat       = support.texFloatLinear || extSupport(ext, "_texture_float");
        support.texHalfLinear  = extSupport(ext, "GL_ARB_texture_float") || extSupport(ext, "_texture_half_float_linear");
        support.texHalf        = support.texHalfLinear || extSupport(ext, "_texture_half_float");

        if (extSupport(ext, "_ATI_separate_stencil"))
            support.stencil = 2;
        else
            if (extSupport(ext, "_stencil_two_side"))
                support.stencil = 1;
            else
                support.stencil = 0;
        
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
        LOG("  binary shaders : %s\n", support.shaderBinary  ? "true" : "false");
        LOG("  vertex arrays  : %s\n", support.VAO           ? "true" : "false");
        LOG("  depth texture  : %s\n", support.depthTexture  ? "true" : "false");
        LOG("  shadow sampler : %s\n", support.shadowSampler ? "true" : "false");
        LOG("  discard frame  : %s\n", support.discardFrame  ? "true" : "false");
        LOG("  NPOT textures  : %s\n", support.texNPOT       ? "true" : "false");
        LOG("  RG   textures  : %s\n", support.texRG         ? "true" : "false");
        LOG("  border color   : %s\n", support.texBorder     ? "true" : "false");
        LOG("  float textures : float = %s, half = %s\n", 
            support.texFloat ? (support.texFloatLinear ? "linear" : "nearest") : "false",
            support.texHalf  ? (support.texHalfLinear  ? "linear" : "nearest") : "false");
        LOG("  stencil        : %s\n", support.stencil == 2 ? "separate" : (support.stencil == 1 ? "two_side" : "false"));
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

    void clear(bool clearColor, bool clearDepth, bool clearStencil = false) {        
        if (GLbitfield mask = (clearColor ? GL_COLOR_BUFFER_BIT : 0) | (clearDepth ? GL_DEPTH_BUFFER_BIT : 0) | (clearStencil ? GL_STENCIL_BUFFER_BIT : 0))
            glClear(mask);
    }

    void setClearColor(const vec4 &color) {
        glClearColor(color.x, color.y, color.z, color.w);
    }

    void setClearStencil(int value) {
        glClearStencil(value);
    }

    void setViewport(const vec4 &vp) {
        glViewport(int(vp.x), int(vp.y), int(vp.z), int(vp.w));
        viewport = vp;
    }

    void setViewport(int x, int y, int width, int height) {
        setViewport(vec4(float(x), float(y), float(width), float(height)));
    }

    void setScissor(int x, int y, int width, int height) {
        glScissor(x, y, width, height);
        scissor = vec4(float(x), float(y), float(width), float(height));
    }

    void setCulling(CullMode mode) {
        if (active.cullMode == mode)
            return;

        switch (mode) {
            case cfNone :
                glDisable(GL_CULL_FACE);
            case cfBack :
                glCullFace(GL_BACK);
                break;
            case cfFront :
                glCullFace(GL_FRONT);
                break;
        }

        if (mode != cfNone && active.cullMode == cfNone)
            glEnable(GL_CULL_FACE);

        active.cullMode = mode;
    }

    void setBlending(BlendMode mode) {
        if (active.blendMode == mode)
            return;

        switch (mode) {
            case bmNone :
                glDisable(GL_BLEND);
                break;
            case bmAlpha :
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                break;
            case bmAdd :
                glBlendFunc(GL_ONE, GL_ONE);
                break;
            case bmMultiply :
                glBlendFunc(GL_DST_COLOR, GL_ZERO);
                break;
            case bmScreen :
                glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);
                break;
        }

        if (mode != bmNone && active.blendMode == bmNone)
            glEnable(GL_BLEND);

        active.blendMode = mode;
    }

    void setColorWrite(bool r, bool g, bool b, bool a) {
        glColorMask(r, g, b, a);
    }

    void setDepthWrite(bool write) {
        glDepthMask(write);
    }

    void setDepthTest(bool test) {
        if (test)
            glEnable(GL_DEPTH_TEST);
        else
            glDisable(GL_DEPTH_TEST);
    }

    void setStencilTest(bool test) {
        if (test)
            glEnable(GL_STENCIL_TEST);
        else
            glDisable(GL_STENCIL_TEST);
    }

    void setScissorTest(bool test) {
        if (test)
            glEnable(GL_SCISSOR_TEST);
        else
            glDisable(GL_SCISSOR_TEST);
    }

    void setStencilTwoSide(int ref, bool test) { // preset for z-fail shadow volumes
        active.stencilTwoSide = test;
        if (test) {
            switch (Core::support.stencil) {
                case 0 :
                    glStencilFunc(GL_ALWAYS, ref, ~0);
                    break;
                case 1 :
                    setCulling(cfNone);
                    glEnable(GL_STENCIL_TEST_TWO_SIDE_EXT);
                    glActiveStencilFaceEXT(GL_BACK);
                    glStencilOp(GL_KEEP, GL_DECR, GL_KEEP);
                    glStencilFunc(GL_ALWAYS, ref, ~0);
                    glActiveStencilFaceEXT(GL_FRONT);
                    glStencilOp(GL_KEEP, GL_INCR, GL_KEEP);
                    glStencilFunc(GL_ALWAYS, ref, ~0);
                    break;
                case 2 :
                    setCulling(cfNone);
                    glStencilFuncSeparate(GL_FRONT, GL_ALWAYS, ref, ~0);
                    glStencilFuncSeparate(GL_BACK,  GL_ALWAYS, ref, ~0);
                    glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_INCR, GL_KEEP);
                    glStencilOpSeparate(GL_BACK,  GL_KEEP, GL_DECR, GL_KEEP);
                    break;
            }
        } else {
            if (Core::support.stencil == 1)
                glDisable(GL_STENCIL_TEST_TWO_SIDE_EXT);
            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
            glStencilFunc(GL_NOTEQUAL, ref, ~0);
            setCulling(cfFront);
        }
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
        if (!target && defaultTarget)
            target = defaultTarget;

        if (target != active.target || face != active.targetFace) {
            if (!target)  {
                glBindFramebuffer(GL_FRAMEBUFFER, defaultFBO);
                glColorMask(true, true, true, true);

                setViewport(int(viewportDef.x), int(viewportDef.y), int(viewportDef.z), int(viewportDef.w));
            } else {
                if (active.target == NULL || active.target == defaultTarget)
                    viewportDef = viewport;
                GLenum texTarget = GL_TEXTURE_2D;
                if (target->cube) 
                    texTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X + face;

                bool depth   = target->format == Texture::DEPTH || target->format == Texture::SHADOW;
                int  rtIndex = cacheRenderTarget(depth, target->width, target->height);

                glBindFramebuffer(GL_FRAMEBUFFER, FBO);
                glFramebufferTexture2D    (GL_FRAMEBUFFER, depth ? GL_DEPTH_ATTACHMENT  : GL_COLOR_ATTACHMENT0, texTarget,       target->ID, 0);
                glFramebufferRenderbuffer (GL_FRAMEBUFFER, depth ? GL_COLOR_ATTACHMENT0 : GL_DEPTH_ATTACHMENT,  GL_RENDERBUFFER, rtCache[depth].items[rtIndex].ID);

                if (depth)
                    glColorMask(false, false, false, false);
                else
                    glColorMask(true, true, true, true);
                setViewport(0, 0, target->width, target->height);
            }
        }

        if (clear)
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        active.target     = target;
        active.targetFace = face;
    }

    void copyTarget(Texture *texture, int xOffset, int yOffset, int x, int y, int width, int height) {
        texture->bind(sDiffuse);
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, xOffset, yOffset, x, y, width, height);
    }

    void beginFrame() {
        memset(&active, 0, sizeof(active));
        setViewport(0, 0, Core::width, Core::height);
        viewportDef = viewport;
        setDepthTest(true);
        active.blendMode = bmAlpha;
        active.cullMode  = cfNone;
        setCulling(cfFront);
        setBlending(bmNone);
        Core::stats.start();
    }

    void endFrame() {
        Core::stats.stop();
    }
}

#include "mesh.h"

#endif
