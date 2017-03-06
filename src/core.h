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

	#define GL_TEXTURE_COMPARE_MODE		0x884C
	#define GL_TEXTURE_COMPARE_FUNC		0x884D
	#define GL_COMPARE_REF_TO_TEXTURE	0x884E

    #undef  GL_RGBA32F
    #undef  GL_RGBA16F
    #undef  GL_HALF_FLOAT

    #define GL_RGBA32F      GL_RGBA
    #define GL_RGBA16F      GL_RGBA
    #define GL_HALF_FLOAT   GL_HALF_FLOAT_OES

    #define PFNGLGENVERTEXARRAYSPROC     PFNGLGENVERTEXARRAYSOESPROC
    #define PFNGLDELETEVERTEXARRAYSPROC  PFNGLDELETEVERTEXARRAYSOESPROC
    #define PFNGLBINDVERTEXARRAYPROC     PFNGLBINDVERTEXARRAYOESPROC
    #define glGenVertexArrays            glGenVertexArraysOES
    #define glDeleteVertexArrays         glDeleteVertexArraysOES
    #define glBindVertexArray            glBindVertexArrayOES
#elif __linux__
    #define LINUX 1
    #include <GL/gl.h>
    #include <GL/glx.h>
    #include <GL/glext.h>
#elif __APPLE__
    #include <Carbon/Carbon.h>
    #include <AudioToolbox/AudioQueue.h>
    #include <OpenGL/OpenGL.h>
    #include <OpenGL/gl.h>
    #include <OpenGL/glext.h>
    #include <AGL/agl.h>
    #define glGenVertexArrays    glGenVertexArraysAPPLE
    #define glDeleteVertexArrays glDeleteVertexArraysAPPLE
    #define glBindVertexArray    glBindVertexArrayAPPLE
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
#endif

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
    #endif

    PFNGLGENVERTEXARRAYSPROC            glGenVertexArrays;
    PFNGLDELETEVERTEXARRAYSPROC         glDeleteVertexArrays;
    PFNGLBINDVERTEXARRAYPROC            glBindVertexArray;
#endif

#ifdef MOBILE
    PFNGLDISCARDFRAMEBUFFEREXTPROC      glDiscardFramebufferEXT;
#endif

#define MAX_LIGHTS          3
#define MAX_CACHED_LIGHTS   3
#define MAX_RENDER_BUFFERS  32

struct Shader;
struct Texture;

#ifdef PROFILE
    struct Marker {
        Marker(const char *title) {
            if (glPushDebugGroup) glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, title);
        }

        ~Marker() {
            if (glPopDebugGroup) glPopDebugGroup();
        }

        static void setLabel(GLenum id, GLuint name, const char *label) {
            if (glObjectLabel) glObjectLabel(id, name, -1, label);
        }
    };

    #define PROFILE_MARKER(title)               Marker marker(title)
    #define PROFILE_LABEL(id, name, label)      Marker::setLabel(GL_##id, name, label)
#else
    #define PROFILE_MARKER(title)
    #define PROFILE_LABEL(id, name, label)
#endif

namespace Core {
    int width, height;
    int frameIndex;
    float deltaTime;
    mat4 mView, mProj, mViewProj, mViewInv, mLightProj;
    Basis basis;
    vec3 viewPos;
    vec3 lightPos[MAX_LIGHTS];
    vec4 lightColor[MAX_LIGHTS];
    vec4 color;

    Texture *blackTex, *whiteTex;

    enum Pass { passCompose, passShadow, passAmbient, passFilter, passWater, passMAX } pass;

    GLuint FBO;
    struct RenderTargetCache {
        int count;
        struct Item {
            GLuint  ID;
            int     width;
            int     height;
        } items[MAX_RENDER_BUFFERS];
    } rtCache[2];

    struct {
        Shader  *shader;
        Texture *textures[8];
        Texture *target;
        GLuint  VAO;
    } active;

    struct {
        int dips;
        int tris;
    } stats;

    struct {
        bool VAO;
        bool depthTexture;
        bool shadowSampler;
        bool discardFrame;
        bool texNPOT;
        bool texFloat, texFloatLinear;
        bool texHalf,  texHalfLinear;
    } support;
}

#include "texture.h"
#include "shader.h"
#include "mesh.h"

enum CullMode  { cfNone, cfBack, cfFront };
enum BlendMode { bmNone, bmAlpha, bmAdd, bmMultiply, bmScreen };

namespace Core {

    bool extSupport(const char *str, const char *ext) {
        return strstr(str, ext) != NULL;
    }

    void init() {
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
            #endif

            #ifdef MOBILE
                GetProcOGL(glDiscardFramebufferEXT);
            #endif

            GetProcOGL(glGenVertexArrays);
            GetProcOGL(glDeleteVertexArrays);
            GetProcOGL(glBindVertexArray);
        #endif


        char *ext = (char*)glGetString(GL_EXTENSIONS);
        LOG("%s\n", ext);
        support.VAO            = extSupport(ext, "_vertex_array_object");
        support.depthTexture   = extSupport(ext, "_depth_texture");
        support.shadowSampler  = extSupport(ext, "_shadow_samplers") || extSupport(ext, "GL_ARB_shadow");
        support.discardFrame   = extSupport(ext, "_discard_framebuffer");
        support.texNPOT        = extSupport(ext, "_texture_npot") || extSupport(ext, "_texture_non_power_of_two");
        support.texFloatLinear = extSupport(ext, "GL_ARB_texture_float") || extSupport(ext, "_texture_float_linear");
        support.texFloat       = support.texFloatLinear || extSupport(ext, "_texture_float");
        support.texHalfLinear  = extSupport(ext, "GL_ARB_texture_float") || extSupport(ext, "_texture_half_float_linear");
        support.texHalf        = support.texHalfLinear || extSupport(ext, "_texture_half_float");
        
        char *vendor = (char*)glGetString(GL_VENDOR);
        LOG("Vendor   : %s\n", vendor);
        LOG("Renderer : %s\n", glGetString(GL_RENDERER));
        LOG("Version  : %s\n", glGetString(GL_VERSION));

        LOG("supports:\n");
        LOG("  vertex arrays  : %s\n", support.VAO           ? "true" : "false");
        LOG("  depth texture  : %s\n", support.depthTexture  ? "true" : "false");
        LOG("  shadow sampler : %s\n", support.shadowSampler ? "true" : "false");
        LOG("  discard frame  : %s\n", support.discardFrame  ? "true" : "false");        
        LOG("  NPOT textures  : %s\n", support.texNPOT       ? "true" : "false");        
        LOG("  float textures : float = %s, half = %s\n", 
            support.texFloat ? (support.texFloatLinear ? "linear" : "nearest") : "false",
            support.texHalf  ? (support.texHalfLinear  ? "linear" : "nearest") : "false");
        LOG("\n");

        glGenFramebuffers(1, &FBO);
        memset(rtCache, 0, sizeof(rtCache));

        Sound::init();

        for (int i = 0; i < MAX_LIGHTS; i++)
            lightColor[i] = vec4(0, 0, 0, 1);

        frameIndex = 0;
      
        uint32 data = 0xFF000000;
        blackTex = new Texture(1, 1, Texture::RGBA, false, &data);
        data = 0xFFFFFFFF;
        whiteTex = new Texture(1, 1, Texture::RGBA, false, &data);       
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

    void clear(bool clearColor, bool clearDepth, const vec4 &color) {
        glClearColor(color.x, color.y, color.z, color.w);
        if (GLbitfield mask = (clearColor ? GL_COLOR_BUFFER_BIT : 0) | (clearDepth ? GL_DEPTH_BUFFER_BIT : 0))
            glClear(mask);
    }

    void setViewport(int x, int y, int width, int height) {
        glViewport(x, y, width, height);
    }

    void setCulling(CullMode mode) {
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

        if (mode != cfNone)
            glEnable(GL_CULL_FACE);
    }

    void setBlending(BlendMode mode) {
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

        if (mode != bmNone)
            glEnable(GL_BLEND);
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
        if (!target)  {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glColorMask(true, true, true, true);
            setViewport(0, 0, Core::width, Core::height);
        } else {
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
            setViewport(0, 0, target->width, target->height);
        }
        active.target = target;

        if (clear)
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    }

    void copyTarget(Texture *texture, int xOffset, int yOffset, int x, int y, int width, int height) {
        texture->bind(sDiffuse);
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, xOffset, yOffset, x, y, width, height);
    }

    void resetStates() {
        memset(&active, 0, sizeof(active));
        glEnable(GL_DEPTH_TEST);
        setCulling(cfFront);
        setBlending(bmNone);
    }
}

#endif
