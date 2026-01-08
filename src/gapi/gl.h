#ifndef H_GAPI_GL
#define H_GAPI_GL

#include "core.h"

#if defined(_DEBUG) || defined(PROFILE)
    //#define _DEBUG_SHADERS "../../OpenLara/src/shaders/"
#endif

#ifdef __LIBRETRO__
#include <libretro.h>
extern struct retro_hw_render_callback hw_render;
#endif

#ifdef _OS_WIN
    #include <GL/gl.h>
    #include <GL/glext.h>
    #include <GL/wglext.h>
#elif __LIBRETRO_GLES__
    #include <EGL/egl.h>
    #include <GLES2/gl2.h>
    #include <GLES2/gl2ext.h>

    #define GL_CLAMP_TO_BORDER          0x812D
    #define GL_TEXTURE_BORDER_COLOR     0x1004

    #define GL_TEXTURE_COMPARE_MODE     0x884C
    #define GL_TEXTURE_COMPARE_FUNC     0x884D
    #define GL_COMPARE_REF_TO_TEXTURE   0x884E

    #undef  GL_RG
    #undef  GL_RG32F
    #undef  GL_RG16F
    #undef  GL_RGBA32F
    #undef  GL_RGBA16F
    #undef  GL_HALF_FLOAT

    #define GL_RG           GL_RGBA
    #define GL_RGBA32F      GL_RGBA
    #define GL_RGBA16F      GL_RGBA
    #define GL_RG32F        GL_RGBA
    #define GL_RG16F        GL_RGBA
    #define GL_HALF_FLOAT   GL_HALF_FLOAT_OES

    #ifndef GL_TEXTURE_3D
    #define GL_TEXTURE_3D           0
    #endif
    #define GL_TEXTURE_WRAP_R       0
    #define GL_DEPTH_STENCIL        GL_DEPTH_STENCIL_OES
    #define GL_UNSIGNED_INT_24_8    GL_UNSIGNED_INT_24_8_OES

    #define glTexImage3D(...) 0

    #define glGenVertexArrays(...)
    #define glDeleteVertexArrays(...)
    #define glBindVertexArray(...)
    
    #define GL_PROGRAM_BINARY_LENGTH     GL_PROGRAM_BINARY_LENGTH_OES
    #define glGetProgramBinary(...)
    #define glProgramBinary(...)

    #define glInvalidateFramebuffer(...)

    #define GL_R8           GL_R8_EXT
    #define GL_RED          GL_RED_EXT

    #ifndef GL_TEXTURE_MAX_LEVEL
    #define GL_TEXTURE_MAX_LEVEL 0x813D
    #endif

#ifndef HAVE_OPENGLES3
    #define PFNGLGENVERTEXARRAYSPROC    PFNGLGENVERTEXARRAYSOESPROC
    #define PFNGLDELETEVERTEXARRAYSPROC PFNGLDELETEVERTEXARRAYSOESPROC
    #define PFNGLBINDVERTEXARRAYPROC    PFNGLBINDVERTEXARRAYOESPROC
    #define PFNGLGETPROGRAMBINARYPROC   PFNGLGETPROGRAMBINARYOESPROC
    #define PFNGLPROGRAMBINARYPROC      PFNGLPROGRAMBINARYOESPROC
#endif

#elif _OS_ANDROID
    #include <dlfcn.h>

//    #define _GAPI_GLES2 // for old devices

    #ifdef _GAPI_GLES2
        #include <GLES2/gl2.h>
        #include <GLES2/gl2ext.h>

        #define GL_CLAMP_TO_BORDER          0x812D
        #define GL_TEXTURE_BORDER_COLOR     0x1004

        #define GL_TEXTURE_COMPARE_MODE     0x884C
        #define GL_TEXTURE_COMPARE_FUNC     0x884D
        #define GL_COMPARE_REF_TO_TEXTURE   0x884E

        #undef  GL_RG
        #undef  GL_RG32F
        #undef  GL_RG16F
        #undef  GL_RGBA32F
        #undef  GL_RGBA16F
        #undef  GL_HALF_FLOAT

        #define GL_RG           GL_RGBA
        #define GL_RGBA32F      GL_RGBA
        #define GL_RGBA16F      GL_RGBA
        #define GL_RG32F        GL_RGBA
        #define GL_RG16F        GL_RGBA
        #define GL_HALF_FLOAT   GL_HALF_FLOAT_OES

        #define GL_TEXTURE_3D           0
        #define GL_TEXTURE_WRAP_R       0
        #define GL_DEPTH_STENCIL        GL_DEPTH_STENCIL_OES
        #define GL_UNSIGNED_INT_24_8    GL_UNSIGNED_INT_24_8_OES

        #define glTexImage3D(...) 0

        #define glGenVertexArrays(...)
        #define glDeleteVertexArrays(...)
        #define glBindVertexArray(...)
    
        #define GL_PROGRAM_BINARY_LENGTH     GL_PROGRAM_BINARY_LENGTH_OES
        #define glGetProgramBinary(...)
        #define glProgramBinary(...)

        #define glInvalidateFramebuffer(...)
    #else
        #include <GLES3/gl3.h>
        #include <GLES3/gl3ext.h>
        #include <GLES2/gl2ext.h>
    #endif

#elif defined(__SDL2__) 
    #include <SDL2/SDL.h>

    #if defined(_GAPI_GLES) // Default in SDL2 is GLES3. If we want GLES2, pass -D_GAPI_GLES2.
    #if defined (_GAPI_GLES2) // We want GLES2 on SDL2
        #include <SDL2/SDL_opengles2.h>
        #include <SDL2/SDL_opengles2_gl2ext.h>

        #define GL_CLAMP_TO_BORDER          0x812D
        #define GL_TEXTURE_BORDER_COLOR     0x1004

        #define GL_TEXTURE_COMPARE_MODE     0x884C
        #define GL_TEXTURE_COMPARE_FUNC     0x884D
        #define GL_COMPARE_REF_TO_TEXTURE   0x884E

        #define GL_NUM_EXTENSIONS           0x821D

        #undef  GL_RG
        #undef  GL_RG32F
        #undef  GL_RG16F
        #undef  GL_RGBA32F
        #undef  GL_RGBA16F
        #undef  GL_HALF_FLOAT

        #define GL_RG           GL_RGBA
        #define GL_RGBA32F      GL_RGBA
        #define GL_RGBA16F      GL_RGBA
        #define GL_RG32F        GL_RGBA
        #define GL_RG16F        GL_RGBA
        #define GL_HALF_FLOAT   GL_HALF_FLOAT_OES

        #define GL_R8           GL_R8_EXT
        #define GL_RED          GL_RED_EXT

        #define GL_TEXTURE_WRAP_R       0
        #define GL_DEPTH_STENCIL        GL_DEPTH_STENCIL_OES
        #define GL_UNSIGNED_INT_24_8    GL_UNSIGNED_INT_24_8_OES
        //We need this on GLES2, too.
        #define GL_TEXTURE_MAX_LEVEL     GL_TEXTURE_MAX_LEVEL_APPLE

        #define glTexImage3D(...) 0
        #ifndef GL_TEXTURE_3D // WUUUUUT!?
        #define GL_TEXTURE_3D GL_TEXTURE_3D_OES
        #endif

        #define GL_PROGRAM_BINARY_LENGTH     GL_PROGRAM_BINARY_LENGTH_OES
    #else // We want GLES3 on SDL2
        #include <GLES3/gl3.h>
        #include <GLES3/gl3ext.h>
        #include <GLES2/gl2ext.h>
    #endif //GAPI_GLES2

    // These are needed for both GLES2 and GLES3 on SDL2
    #define glGenVertexArrays(...)
    #define glDeleteVertexArrays(...)
    #define glBindVertexArray(...)
    #define glGetProgramBinary(...)
    #define glProgramBinary(...)

    #define PFNGLGENVERTEXARRAYSPROC    PFNGLGENVERTEXARRAYSOESPROC
    #define PFNGLDELETEVERTEXARRAYSPROC PFNGLDELETEVERTEXARRAYSOESPROC
    #define PFNGLBINDVERTEXARRAYPROC    PFNGLBINDVERTEXARRAYOESPROC
    #define PFNGLGETPROGRAMBINARYPROC   PFNGLGETPROGRAMBINARYOESPROC
    #define PFNGLPROGRAMBINARYPROC      PFNGLPROGRAMBINARYOESPROC

    #else // We want OpenGL on SDL2, not GLES
        #include <SDL2/SDL_opengl.h>
        #include <SDL2/SDL_opengl_glext.h>
    #endif

#elif defined(_OS_PSC)
    #include <GLES3/gl3.h>
    #include <GLES2/gl2ext.h>
    extern EGLDisplay display;
#elif defined(_OS_GCW0)
    #include <GLES2/gl2.h>
    #include <GLES2/gl2ext.h>
    #include <EGL/egl.h>
    #include <EGL/eglext.h>

    #define GL_TEXTURE_MAX_LEVEL        GL_TEXTURE_MAX_LEVEL_APPLE
    #define GL_TEXTURE_3D               0 
    #define GL_TEXTURE_WRAP_R           0 
    #define GL_TEXTURE_COMPARE_MODE     0x884C
    #define GL_TEXTURE_COMPARE_FUNC     0x884D
    #define GL_COMPARE_REF_TO_TEXTURE   0x884E

    #define GL_RG                       0x8227
    #define GL_RG16F                    0x822F
    #define GL_RG32F                    0x8230
    #define GL_RGBA16F                  0x881A
    #define GL_RGBA32F                  0x8814
    #define GL_HALF_FLOAT               0x140B

    #define GL_RG           GL_RGBA
    #define GL_RGBA32F      GL_RGBA
    #define GL_RGBA16F      GL_RGBA
    #define GL_RG32F        GL_RGBA
    #define GL_RG16F        GL_RGBA
    #define GL_HALF_FLOAT   GL_HALF_FLOAT_OES

    #define glTexImage3D(...) 0

    #define GL_PROGRAM_BINARY_LENGTH     GL_PROGRAM_BINARY_LENGTH_OES

    #define PFNGLGENVERTEXARRAYSPROC    PFNGLGENVERTEXARRAYSOESPROC
    #define PFNGLDELETEVERTEXARRAYSPROC PFNGLDELETEVERTEXARRAYSOESPROC
    #define PFNGLBINDVERTEXARRAYPROC    PFNGLBINDVERTEXARRAYOESPROC
    #define PFNGLGETPROGRAMBINARYPROC   PFNGLGETPROGRAMBINARYOESPROC
    #define PFNGLPROGRAMBINARYPROC      PFNGLPROGRAMBINARYOESPROC

    #define glGenVertexArrays    glGenVertexArraysOES
    #define glDeleteVertexArrays glDeleteVertexArraysOES
    #define glBindVertexArray    glBindVertexArrayOES
    #define glGetProgramBinary   glGetProgramBinaryOES
    #define glProgramBinary      glProgramBinaryOES

    extern EGLDisplay display;
#elif defined(_OS_RPI) || defined(_OS_CLOVER)
    #include <GLES2/gl2.h>
    #include <GLES2/gl2ext.h>
    #include <EGL/egl.h>
    #include <EGL/eglext.h>

    #define GL_CLAMP_TO_BORDER          0x812D
    #define GL_TEXTURE_BORDER_COLOR     0x1004
    #define GL_TEXTURE_MAX_LEVEL        GL_TEXTURE_MAX_LEVEL_APPLE
    #define GL_TEXTURE_COMPARE_MODE     0x884C
    #define GL_TEXTURE_COMPARE_FUNC     0x884D
    #define GL_COMPARE_REF_TO_TEXTURE   0x884E

    #undef  GL_RG
    #undef  GL_RG32F
    #undef  GL_RG16F
    #undef  GL_RGBA32F
    #undef  GL_RGBA16F
    #undef  GL_HALF_FLOAT

    #define GL_RG           GL_RGBA
    #define GL_RGBA32F      GL_RGBA
    #define GL_RGBA16F      GL_RGBA
    #define GL_RG32F        GL_RGBA
    #define GL_RG16F        GL_RGBA
    #define GL_HALF_FLOAT   GL_HALF_FLOAT_OES

    #define GL_TEXTURE_3D           0
    #define GL_TEXTURE_WRAP_R       0
    #define GL_DEPTH_STENCIL        GL_DEPTH_STENCIL_OES
    #define GL_UNSIGNED_INT_24_8    GL_UNSIGNED_INT_24_8_OES

    #define glTexImage3D(...) 0

    #define glGenVertexArrays(...)
    #define glDeleteVertexArrays(...)
    #define glBindVertexArray(...)
    
    #define GL_PROGRAM_BINARY_LENGTH     GL_PROGRAM_BINARY_LENGTH_OES
    #define glGetProgramBinary(...)
    #define glProgramBinary(...)

    extern EGLDisplay display;
#elif _OS_SWITCH
    #define GL_GLEXT_PROTOTYPES
    #include <EGL/egl.h>
    #include <EGL/eglext.h>
    #include <GL/gl.h>
    #include <GL/glext.h>

    extern EGLDisplay display;
#elif _OS_LINUX && !defined(__LIBRETRO_GLES__)
    #include <GL/gl.h>
    #include <GL/glext.h>
    #include <GL/glx.h>
#elif __APPLE__
    #ifdef _OS_IOS
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

        // TODO: WTF?
        #undef  GL_RG
        #undef  GL_RGBA32F
        #undef  GL_RGBA16F
        #undef  GL_RG32F
        #undef  GL_RG16F
        #undef  GL_HALF_FLOAT

        #define GL_RG           GL_RGBA
        #define GL_RG16F        GL_RGBA
        #define GL_RG32F        GL_RGBA
        #define GL_RGBA32F      GL_RGBA
        #define GL_RGBA16F      GL_RGBA
        #define GL_HALF_FLOAT   GL_HALF_FLOAT_OES

        //#define GL_TEXTURE_COMPARE_MODE     GL_TEXTURE_COMPARE_MODE_EXT
        //#define GL_TEXTURE_COMPARE_FUNC     GL_TEXTURE_COMPARE_FUNC_EXT
        //#define GL_COMPARE_REF_TO_TEXTURE   GL_COMPARE_REF_TO_TEXTURE_EXT
    #else
        #include <AudioToolbox/AudioQueue.h>
        #include <OpenGL/gl3.h>
        #include <OpenGL/gl3ext.h>
        #include <OpenGL/glext.h>

        // In OpenGL 2.1, the 16-bit RGB565 sized internal format is unavailable
        // (because macOS doesn't provide the GL_ARB_ES2_compatibility extension),
        // so use a 32-bit format for renderbuffer storage on macOS. See issue 360.
        #undef GL_RGB565
        #define GL_RGB565 GL_RGBA

        // In compatibility mode, macOS only provides OpenGL 2.1 (no VAO), but it does
        // support the Apple-specific VAO extension which is older and in all relevant
        // parts 100% compatible. So use those functions instead.
        #define glGenVertexArrays    glGenVertexArraysAPPLE
        #define glDeleteVertexArrays glDeleteVertexArraysAPPLE
        #define glBindVertexArray    glBindVertexArrayAPPLE

        #define GL_LUMINANCE 0x1909
    #endif
#elif _OS_WEB
    #include <emscripten/emscripten.h>
    #include <emscripten/html5.h>
    #include <GLES/gl.h>
    #include <GLES3/gl3.h>
    #include <GLES3/gl2ext.h>

    #define GL_CLAMP_TO_BORDER          GL_CLAMP_TO_BORDER_EXT
    #define GL_TEXTURE_BORDER_COLOR     GL_TEXTURE_BORDER_COLOR_EXT

    #define glGetProgramBinary(...)
    #define glProgramBinary(...)
#endif

#if defined(_OS_WIN) || defined(_OS_LINUX) || defined(_OS_GCW0) || (defined(__SDL2__) && (defined(_GAPI_GLES2) || defined(_SDL2_OPENGL)))

    void* GetProc(const char *name) {
        #ifdef _OS_WIN
            return (void*)wglGetProcAddress(name);
        #elif _OS_LINUX && !(__LIBRETRO_GLES__)
            return (void*)glXGetProcAddress((GLubyte*)name);
        #elif __SDL2__
            return (void*)SDL_GL_GetProcAddress(name);
        #else // EGL
            return (void*)eglGetProcAddress(name);
        #endif
    }

    #define GetProcOGL(x) x=(decltype(x))GetProc(#x)

    // TODO: different systems, different headers, different extension suffixes... fuck this shit and make your own OGL header!

// Texture
    #ifdef _OS_WIN
        PFNGLACTIVETEXTUREPROC              glActiveTexture;
    #endif

// VSync
    #ifdef _OS_WIN
        typedef BOOL (WINAPI * PFNWGLSWAPINTERVALEXTPROC) (int interval);
        PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;
    #elif _OS_LINUX
        typedef int (*PFNGLXSWAPINTERVALSGIPROC) (int interval);
        PFNGLXSWAPINTERVALSGIPROC glXSwapIntervalSGI;
    #endif

    #if defined(_OS_WIN) || defined(_OS_LINUX) && !(__LIBRETRO__) || (defined(__SDL2__) && !defined(_GAPI_GLES2))
        PFNGLGENERATEMIPMAPPROC             glGenerateMipmap;
        #ifdef _OS_WIN
            PFNGLTEXIMAGE3DPROC             glTexImage3D;
        #endif
        PFNGLGETSTRINGIPROC                 glGetStringi;
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
        PFNGLGENFRAMEBUFFERSPROC                     glGenFramebuffers;
        PFNGLBINDFRAMEBUFFERPROC                     glBindFramebuffer;
        PFNGLGENRENDERBUFFERSPROC                    glGenRenderbuffers;
        PFNGLBINDRENDERBUFFERPROC                    glBindRenderbuffer;
        PFNGLFRAMEBUFFERTEXTURE2DPROC                glFramebufferTexture2D;
        PFNGLFRAMEBUFFERRENDERBUFFERPROC             glFramebufferRenderbuffer;
        PFNGLRENDERBUFFERSTORAGEPROC                 glRenderbufferStorage;
        PFNGLCHECKFRAMEBUFFERSTATUSPROC              glCheckFramebufferStatus;
        PFNGLDELETEFRAMEBUFFERSPROC                  glDeleteFramebuffers;
        PFNGLDELETERENDERBUFFERSPROC                 glDeleteRenderbuffers;
        PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC glGetFramebufferAttachmentParameteriv;
    // Mesh
        PFNGLGENBUFFERSARBPROC              glGenBuffers;
        PFNGLDELETEBUFFERSARBPROC           glDeleteBuffers;
        PFNGLBINDBUFFERARBPROC              glBindBuffer;
        PFNGLBUFFERDATAARBPROC              glBufferData;
        PFNGLBUFFERSUBDATAARBPROC           glBufferSubData;
    #endif

// Vertex Arrays
    #ifndef glGenVertexArrays
        PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
    #endif

    #ifndef glDeleteVertexArrays
        PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays;
    #endif

    #ifndef glBindVertexArray
        PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
    #endif

// Binary shaders
    #ifndef glGetProgramBinary
        PFNGLGETPROGRAMBINARYPROC glGetProgramBinary;
    #endif

    #ifndef glProgramBinary
        PFNGLPROGRAMBINARYPROC glProgramBinary;
    #endif

    #if defined(_GAPI_GLES)
        PFNGLDISCARDFRAMEBUFFEREXTPROC      glDiscardFramebufferEXT;
    #endif

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


namespace GAPI {

    using namespace Core;

    typedef ::Vertex Vertex;

    int cullMode, blendMode;
    bool depthWrite;

    char GLSL_HEADER_VERT[512];
    char GLSL_HEADER_FRAG[512];

    bool GL_VER_3 = false;

// Shader
    #ifndef FFP
        const char SHADER_COMPOSE[] =
            #include "../shaders/compose.glsl"
        ;

        const char SHADER_SHADOW[] =
            #include "../shaders/shadow.glsl"
        ;

        const char SHADER_AMBIENT[] =
            #include "../shaders/ambient.glsl"
        ;

        const char SHADER_SKY[] =
            #include "../shaders/sky.glsl"
        ;

        const char SHADER_WATER[] =
            #include "../shaders/water.glsl"
        ;

        const char SHADER_FILTER[] =
            #include "../shaders/filter.glsl"
        ;

        const char SHADER_GUI[] =
            #include "../shaders/gui.glsl"
        ;
        
        const char *DefineName[SD_MAX]  = { SHADER_DEFINES(DECL_STR) };
    #endif

    static const struct Binding {
        bool vec; // true - vec4, false - mat4
        int  reg;
    } bindings[uMAX] = {
        { true,   0 }, // uParam
        { true,   1 }, // uTexParam
        { false,  2 }, // uViewProj
        { true,   6 }, // uBasis
        { false, 70 }, // uLightProj
        { true,  74 }, // uMaterial
        { true,  75 }, // uAmbient
        { true,  81 }, // uFogParams
        { true,  82 }, // uViewPos
        { true,  83 }, // uLightPos
        { true,  87 }, // uLightColor
        { true,  91 }, // uRoomSize
        { true,  92 }, // uPosScale
        { true,  98 }, // uContacts
    };

    struct Shader {
    #ifdef FFP
        void init(Core::Pass pass, int type, int *def, int defCount) {}
        void deinit() {}
        void bind() {}
        void validate() {}
        void setParam(UniformType uType, const vec4  &value, int count = 1) {}
        void setParam(UniformType uType, const mat4  &value, int count = 1) {}
    #else
        GLuint  ID;
        int32   uID[uMAX];

        vec4  cbMem[98 + MAX_CONTACTS];
        int   cbCount[uMAX];

        bool  rebind;

        void init(Pass pass, int type, int *def, int defCount) {
            const char *source;
            switch (pass) {
                case Core::passCompose : source = SHADER_COMPOSE; break;
                case Core::passShadow  : source = SHADER_SHADOW;  break;
                case Core::passAmbient : source = SHADER_AMBIENT; break;
                case Core::passSky     : source = SHADER_SKY;     break;
                case Core::passWater   : source = SHADER_WATER;   break;
                case Core::passFilter  : source = SHADER_FILTER;  break;
                case Core::passGUI     : source = SHADER_GUI;     break;
                default                : ASSERT(false); LOG("! wrong pass id\n"); return;
            }

            #ifdef _DEBUG_SHADERS
                Stream *stream = NULL;
                switch (pass) {
                    case Core::passCompose : stream = new Stream(_DEBUG_SHADERS "compose.glsl"); break;
                    case Core::passShadow  : stream = new Stream(_DEBUG_SHADERS "shadow.glsl");  break;
                    case Core::passAmbient : stream = new Stream(_DEBUG_SHADERS "ambient.glsl"); break;
                    case Core::passSky     : stream = new Stream(_DEBUG_SHADERS "sky.glsl");     break;
                    case Core::passWater   : stream = new Stream(_DEBUG_SHADERS "water.glsl");   break;
                    case Core::passFilter  : stream = new Stream(_DEBUG_SHADERS "filter.glsl");  break;
                    case Core::passGUI     : stream = new Stream(_DEBUG_SHADERS "gui.glsl");     break;
                    default                : ASSERT(false);  return;
                }
                
                char *sourceData = new char[stream->size + 1];
                stream->raw(sourceData, stream->size);
                sourceData[stream->size] = 0;

                source = sourceData;
                for (int i = 0; i < stream->size; i++) // trim string resource begin tag
                    if (sourceData[i] == '(') {
                        source = sourceData + i + 1;
                        break;
                    }

                for (int i = stream->size - 1; i >= 0; i--) // trim string resource end tag
                    if (sourceData[i] == ')') {
                        sourceData[i] = 0;
                        break;
                    }

                delete stream;
            #endif

            char defines[1024];
            defines[0] = 0;
            strcat(defines, "#define VER3\n");

            for (int i = 0; i < defCount; i++) {
                sprintf(defines + strlen(defines), "#define %s\n", DefineName[def[i]]);
            }

            sprintf(defines + strlen(defines), "#define SHADOW_SIZE %d.0\n", SHADOW_TEX_SIZE);

            #ifdef MERGE_MODELS
                strcat(defines, "#define MESH_SKINNING\n");
            #endif

            // etnaviv driver has no sin/cos/abs implementation
            #if !defined(_OS_GCW0) 
                strcat(defines, "#define VERT_CAUSTICS\n");
            #endif

            #if defined(_OS_RPI) || defined(_OS_CLOVER) || defined(_OS_GCW0) || (defined (__SDL2__) && defined(_GAPI_GLES))
                strcat(defines, "#define OPT_VLIGHTPROJ\n");
                strcat(defines, "#define OPT_VLIGHTVEC\n");
                strcat(defines, "#define OPT_SHADOW_ONETAP\n");
            #endif

            if (support.tex3D) {
                strcat(defines, "#define OPT_TEXTURE_3D\n");
            }

            #ifndef _OS_CLOVER
                // TODO: only for non Mali-400?
                strcat(defines, "#define OPT_TRAPEZOID\n");
                if (Core::settings.detail.water > Core::Settings::LOW)
                    strcat(defines, "#define OPT_UNDERWATER_FOG\n");
            #endif

            char fileName[255];
        // generate shader file path
            if (Core::support.shaderBinary) {
                uint32 hash = fnv32(defines, (int32)strlen(defines), fnv32(source, (int32)strlen(source)));
                sprintf(fileName, "%08X.xsh", hash);
            }

            ID = glCreateProgram();

            if (!(Core::support.shaderBinary && linkBinary(fileName))) { // try to load cached shader     
                if (linkSource(source, defines) && Core::support.shaderBinary) { // compile shader from source and dump it into cache
                #ifndef _OS_WEB
                    GLenum format = 0, size;
                    glGetProgramiv(ID, GL_PROGRAM_BINARY_LENGTH, (GLsizei*)&size);
                    char *data = new char[8 + size];
                    glGetProgramBinary(ID, size, NULL, &format, &data[8]);
                    *(int*)(&data[0]) = format;
                    *(int*)(&data[4]) = size;
                    Stream::cacheWrite(fileName, data, 8 + size);
                    delete[] data;
                #endif
                }
            }

            #ifdef _DEBUG_SHADERS
                delete[] sourceData;
            #endif

            Core::active.shader = this;
            glUseProgram(ID);

            for (int st = 0; st < sMAX; st++) {
                GLint idx = glGetUniformLocation(ID, (GLchar*)SamplerName[st]);
                if (idx != -1)
                    glUniform1iv(idx, 1, &st);
            }

            for (int ut = 0; ut < uMAX; ut++)
                uID[ut] = glGetUniformLocation(ID, (GLchar*)UniformName[ut]);

            rebind = true;
        }

        void deinit() {
            glDeleteProgram(ID);
        }

        bool linkSource(const char *text, const char *defines = "") {
            const int type[2] = { GL_VERTEX_SHADER, GL_FRAGMENT_SHADER };
            const char *code[2][4] = {
                    { GLSL_HEADER_VERT, defines, "#line 0\n", text },
                    { GLSL_HEADER_FRAG, defines, "#line 0\n", text }
                };

            GLchar info[1024];

            for (int i = 0; i < 2; i++) {
                GLuint obj = glCreateShader(type[i]);
                glShaderSource(obj, 4, code[i], NULL);
                glCompileShader(obj);

                glGetShaderInfoLog(obj, sizeof(info), NULL, info);
                if (info[0] && strlen(info) > 8) LOG("! shader: %s\n", info);

                glAttachShader(ID, obj);
                glDeleteShader(obj);
            }

            for (int at = 0; at < aMAX; at++)
                glBindAttribLocation(ID, at, AttribName[at]);

            glLinkProgram(ID);

            glGetProgramInfoLog(ID, sizeof(info), NULL, info);
            if (info[0] && strlen(info) > 8) LOG("! program: %s\n", info);

            return checkLink();
        }
    
        bool linkBinary(const char *name) {
            // non-async code!
            char path[255];
            strcpy(path, cacheDir);
            strcat(path, name);

            if (!Stream::exists(path))
                return false;

            Stream *stream = new Stream(path);

            if (!stream || stream->size <= 0) {
                delete stream;
                return false;
            }

            GLenum size, format;
            stream->read(format);
            stream->read(size);

            if (size == 0 || size > (GLenum)stream->size) {
                delete stream;
                return false;
            }

            char *data = new char[size];
            stream->raw(data, size);
            glProgramBinary(ID, format, data, size);
            delete[] data;
            delete stream;

            return checkLink();
        }

        bool checkLink() {
            GLint success;
            glGetProgramiv(ID, GL_LINK_STATUS, &success);
            return success != 0;
        }

        void bind() {
            if (Core::active.shader != this) {
                Core::active.shader = this;
                memset(cbCount, 0, sizeof(cbCount));
                rebind = true;
            }
        }

        void validate() {
            if (rebind) {
                glUseProgram(ID);
                rebind = false;
            }

            for (int uType = 0; uType < uMAX; uType++) {
                if (!cbCount[uType]) continue;

                const Binding &b = bindings[uType];

                if (b.vec)
                    glUniform4fv(uID[uType], cbCount[uType], (GLfloat*)(cbMem + b.reg));
                else
                    glUniformMatrix4fv(uID[uType], cbCount[uType] / 4, false, (GLfloat*)(cbMem + b.reg));

                cbCount[uType] = 0;
                Core::stats.cb++;
            }
        }
        
        void setParam(UniformType uType, float *value, int count) {
            cbCount[uType] = count;
            memcpy(cbMem + bindings[uType].reg, value, count * 16);
        }

        void setParam(UniformType uType, const vec4 &value, int count = 1) {
            if (uID[uType] != -1) setParam(uType, (float*)&value, count);
        }

        void setParam(UniformType uType, const mat4 &value, int count = 1) {
            if (uID[uType] != -1) setParam(uType, (float*)&value, count * 4);
        }
    #endif
    };


// Texture
    static const struct FormatDesc {
        GLuint ifmt, fmt;
        GLenum type;
    } formats[FMT_MAX] = {
        { GL_LUMINANCE,       GL_LUMINANCE,       GL_UNSIGNED_BYTE          }, // LUMINANCE
        { GL_RGBA,            GL_RGBA,            GL_UNSIGNED_BYTE          }, // RGBA
        { GL_RGB,             GL_RGB,             GL_UNSIGNED_SHORT_5_6_5   }, // RGB16
        { GL_RGBA,            GL_RGBA,            GL_UNSIGNED_SHORT_5_5_5_1 }, // RGBA16
        { GL_RG32F,           GL_RG,              GL_FLOAT                  }, // RG_FLOAT
        { GL_RG16F,           GL_RG,              GL_HALF_FLOAT             }, // RG_HALF
        { GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT         }, // DEPTH
        { GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT         }, // SHADOW
    };

    struct Texture {
        uint32     ID;
        int        width, height, depth, origWidth, origHeight, origDepth;
        TexFormat  fmt;
        uint32     opt;
        GLenum     target;

        Texture(int width, int height, int depth, uint32 opt) : ID(0), width(width), height(height), depth(depth), origWidth(width), origHeight(height), origDepth(depth), fmt(FMT_RGBA), opt(opt) {}

        void init(void *data) {
            ASSERT((opt & OPT_PROXY) == 0);

            bool filter   = (opt & OPT_NEAREST) == 0;
            bool mipmaps  = (opt & OPT_MIPMAPS) != 0;
            bool isCube   = (opt & OPT_CUBEMAP) != 0;
            bool isVolume = (opt & OPT_VOLUME)  != 0;
            bool isShadow = fmt == FMT_SHADOW;

            target = isVolume ? GL_TEXTURE_3D : (isCube ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D);

            glGenTextures(1, &ID);

            Core::active.textures[0] = NULL;
            bind(0);

            if (isShadow) {
                glTexParameteri(target, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
                glTexParameteri(target, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
            }

            glTexParameteri(target, GL_TEXTURE_WRAP_S, (opt & OPT_REPEAT) ? GL_REPEAT : GL_CLAMP_TO_EDGE);
            glTexParameteri(target, GL_TEXTURE_WRAP_T, (opt & OPT_REPEAT) ? GL_REPEAT : GL_CLAMP_TO_EDGE);
            if (isVolume) {
                glTexParameteri(target, GL_TEXTURE_WRAP_R, (opt & OPT_REPEAT) ? GL_REPEAT : GL_CLAMP_TO_EDGE);
            }

            glTexParameteri(target, GL_TEXTURE_MIN_FILTER, filter ? (mipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR) : (mipmaps ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST));
            glTexParameteri(target, GL_TEXTURE_MAG_FILTER, filter ? GL_LINEAR : GL_NEAREST);

            FormatDesc desc = getFormat();

            void *pix = (width == origWidth && height == origHeight && depth == origDepth) ? data : NULL;

            if (isVolume) {
                glTexImage3D(target, 0, desc.ifmt, width, height, depth, 0, desc.fmt, desc.type, pix);
            } else if (isCube) {
                for (int i = 0; i < 6; i++) {
                    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, desc.ifmt, width, height, 0, desc.fmt, desc.type, pix);
                }
            } else {
                glTexImage2D(target, 0, desc.ifmt, width, height, 0, desc.fmt, desc.type, pix);
            }

            if (pix != data) {
                update(data);
            }
        }

        void deinit() {
            if (ID) {
                glDeleteTextures(1, &ID);
            }
        }

        FormatDesc getFormat() {
            FormatDesc desc = formats[fmt];

            if ((fmt == FMT_RG_FLOAT || fmt == FMT_RG_HALF) && !Core::support.texRG) {
                desc.ifmt = (fmt == FMT_RG_FLOAT) ? GL_RGBA32F : GL_RGBA16F;
                desc.fmt  = GL_RGBA;
            }

            if (fmt == FMT_LUMINANCE && Core::support.texRG) {
                desc.ifmt = GL_R8;
                desc.fmt  = GL_RED;
            }

            #ifdef _OS_WEB // fucking firefox!
                if (WEBGL_VERSION == 1) {
                    if (fmt == FMT_RG_FLOAT) {
                        if (Core::support.texFloat) {
                            desc.ifmt = GL_RGBA;
                            desc.type = GL_FLOAT;
                        }
                    }

                    if (fmt == FMT_RG_HALF) {
                        if (Core::support.texHalf) {
                            desc.ifmt = GL_RGBA;
                            desc.type = GL_HALF_FLOAT_OES;
                        }
                    }
                } else {
                    if (fmt == FMT_DEPTH || fmt == FMT_SHADOW) {
                        desc.ifmt = GL_DEPTH_COMPONENT16;
                    }
                }
            #else
                if ((fmt == FMT_RG_FLOAT && !Core::support.colorFloat) || (fmt == FMT_RG_HALF && !Core::support.colorHalf)) {
                    desc.ifmt = GL_RGBA;
                    #ifdef _GAPI_GLES
                        if (fmt == FMT_RG_HALF) {
                            desc.type = GL_HALF_FLOAT_OES;
                        }
                    #endif
                }
            #endif
            return desc;
        }

        void generateMipMap() {
            bind(0);
            if (glGenerateMipmap) {
                glGenerateMipmap(target);
            }
            if ((opt & (OPT_VOLUME | OPT_CUBEMAP | OPT_NEAREST)) == 0 && (Core::support.maxAniso > 0)) {
                glTexParameteri(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, min(int(Core::support.maxAniso), 8));
                if (Core::support.texMaxLevel) {
                    glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, 3);
                }
            }
        }

        void update(void *data) {
            ASSERT((opt & (OPT_VOLUME | OPT_CUBEMAP)) == 0);
            bind(0);
            FormatDesc desc = getFormat();
            glTexSubImage2D(target, 0, 0, 0, origWidth, origHeight, desc.fmt, desc.type, data);
        }

        void bind(int sampler) {
            if (opt & OPT_PROXY) return;
            ASSERT(ID);

            if (Core::active.textures[sampler] != this) {
                Core::active.textures[sampler] = this;
            #ifdef FFP
                if (sampler != sDiffuse) {
                    return;
                }
            #else
                glActiveTexture(GL_TEXTURE0 + sampler);
            #endif
                glBindTexture(target, ID);
            }
        }

        void unbind(int sampler) {
            if (Core::active.textures[sampler]) {
                Core::active.textures[sampler] = NULL;
            #ifdef FFP
                if (sampler != sDiffuse) {
                    return;
                }
            #else
                glActiveTexture(GL_TEXTURE0 + sampler);
            #endif
                glBindTexture(target, 0);
            }
        }

        void setFilterQuality(int value) {
            bool filter  = (opt & OPT_NEAREST) == 0 && (value > Core::Settings::LOW);
            bool mipmaps = (opt & OPT_MIPMAPS) != 0;

            Core::active.textures[0] = NULL;
            bind(0);
            if (Core::support.maxAniso > 0) {
                glTexParameteri(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, value > Core::Settings::MEDIUM ? min(int(Core::support.maxAniso), 8) : 1);
            }
            glTexParameteri(target, GL_TEXTURE_MIN_FILTER, filter ? (mipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR ) : ( mipmaps ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST ));
            glTexParameteri(target, GL_TEXTURE_MAG_FILTER, filter ? GL_LINEAR : GL_NEAREST);
        }
    };

// Mesh
    struct Mesh {
        Index  *iBuffer;
        Vertex *vBuffer;
        GLuint *VAO;
        GLuint ID[2];

        int    iCount;
        int    vCount;
        int    aCount;
        bool   dynamic;

        Mesh(bool dynamic) : iBuffer(NULL), vBuffer(NULL), VAO(NULL), dynamic(dynamic) {
            ID[0] = ID[1] = 0;
        }

        void init(Index *indices, int iCount, ::Vertex *vertices, int vCount, int aCount) {
            this->iCount = iCount;
            this->vCount = vCount;
            this->aCount = aCount;

            if (Core::support.VAO)
                glBindVertexArray(Core::active.VAO = 0);

            bool useVBO = Core::support.VBO;

            #ifdef DYNGEOM_NO_VBO
                if (!vertices && !indices) {
                    useVBO = false;
                }
            #endif

            ASSERT(sizeof(GAPI::Vertex) == sizeof(::Vertex));

            if (useVBO) {
                glGenBuffers(2, ID);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID[0]);
                glBindBuffer(GL_ARRAY_BUFFER,         ID[1]);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, iCount * sizeof(Index),  indices,  dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
                glBufferData(GL_ARRAY_BUFFER,         vCount * sizeof(Vertex), vertices, dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
            } else {
                iBuffer = new Index[iCount];
                vBuffer = new GAPI::Vertex[vCount];
                update(indices, iCount, vertices, vCount);
            }
            
            if (Core::support.VAO && aCount) {
                VAO = new GLuint[aCount];
                glGenVertexArrays(aCount, VAO);
            }
        }

        void deinit() {
            if (iBuffer || vBuffer) {
                delete[] iBuffer;
                delete[] vBuffer;
            } else {
                if (VAO) {
                    glDeleteVertexArrays(aCount, VAO);
                    delete[] VAO;
                }
                glDeleteBuffers(2, ID);
            }
        }

        void update(Index *indices, int iCount, ::Vertex *vertices, int vCount) {
            ASSERT(sizeof(GAPI::Vertex) == sizeof(::Vertex));

            if (Core::support.VAO && Core::active.VAO != 0)
                glBindVertexArray(Core::active.VAO = 0);

            if (indices && iCount) {
                if (iBuffer) {
                    memcpy(iBuffer, indices, iCount * sizeof(Index));
                } else {
                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Core::active.iBuffer = ID[0]);
                    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, iCount * sizeof(Index), indices);
                }
            }
            if (vertices && vCount) {
                if (vBuffer) {
                    memcpy(vBuffer, vertices, vCount * sizeof(GAPI::Vertex));
                } else {
                    glBindBuffer(GL_ARRAY_BUFFER, Core::active.vBuffer = ID[1]);
                    glBufferSubData(GL_ARRAY_BUFFER, 0, vCount * sizeof(GAPI::Vertex), vertices);
                }
            }
        }

        void setupFVF(GAPI::Vertex *v) const {
            #ifdef FFP
                glTexCoordPointer (2, GL_SHORT,         sizeof(*v), &v->texCoord);
                glColorPointer    (4, GL_UNSIGNED_BYTE, sizeof(*v), &v->light);
                glNormalPointer   (   GL_SHORT,         sizeof(*v), &v->normal);
                glVertexPointer   (3, GL_SHORT,         sizeof(*v), &v->coord);
            #else
                glEnableVertexAttribArray(aCoord);
                glEnableVertexAttribArray(aNormal);
                glEnableVertexAttribArray(aTexCoord);
                glEnableVertexAttribArray(aColor);
                glEnableVertexAttribArray(aLight);

                glVertexAttribPointer(aCoord,    4, GL_SHORT,         false, sizeof(*v), &v->coord);
                glVertexAttribPointer(aNormal,   4, GL_SHORT,         true,  sizeof(*v), &v->normal);
                glVertexAttribPointer(aTexCoord, 4, GL_SHORT,         true,  sizeof(*v), &v->texCoord);
                glVertexAttribPointer(aColor,    4, GL_UNSIGNED_BYTE, true,  sizeof(*v), &v->color);
                glVertexAttribPointer(aLight,    4, GL_UNSIGNED_BYTE, true,  sizeof(*v), &v->light);
            #endif
        }

        void bind(const MeshRange &range) const {
            if (range.aIndex == -1) {
                if (Core::support.VBO && Core::active.iBuffer != ID[0])
                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Core::active.iBuffer = ID[0]);
                if (Core::support.VBO && Core::active.vBuffer != ID[1])
                    glBindBuffer(GL_ARRAY_BUFFER, Core::active.vBuffer = ID[1]);
                setupFVF(vBuffer + range.vStart);
            } else {
                ASSERT(Core::support.VAO);
                GLuint vao = VAO[range.aIndex];
                if (Core::active.VAO != vao)
                    glBindVertexArray(Core::active.VAO = vao);
            }
        }

        void initNextRange(MeshRange &range, int &aIndex) const {
            if (Core::support.VAO && VAO) {
                ASSERT(aIndex < aCount);

                Core::active.iBuffer = 0;
                Core::active.vBuffer = 0;
                Core::active.VAO     = 0;

                range.aIndex = aIndex++;    // get new VAO index
                bind(range);                // bind VAO
                range.aIndex = -1;          // reset VAO to -1
                bind(range);                // bind buffers and setup vertex format
                range.aIndex = aIndex - 1;  // set VAO index back to the new

                glBindVertexArray(Core::active.VAO = 0);
            } else
                range.aIndex = -1;
        }
    };


    GLuint FBO, defaultFBO;
    struct RenderTargetCacheItem {
        GLuint  ID;
        int     width;
        int     height;
    };
    Array<RenderTargetCacheItem> rtCache[2];

#ifdef __LIBRETRO__
    void updateFBO()
    {
        defaultFBO = hw_render.get_current_framebuffer();
    }
#endif

    bool extSupport(const char *str) {
        #if !defined(_GAPI_GLES2) && !_OS_MAC
        if (glGetStringi != NULL) {
            GLint count = 0;
            glGetIntegerv(GL_NUM_EXTENSIONS, &count); 
            for (int i = 0; i < count; i++) {
                const char *ext = (const char*)glGetStringi(GL_EXTENSIONS, i);
                if (strstr(ext, str) != NULL) {
                    return true;
                }
            }
        } else
        #endif
        {
            const char *ext =  (const char*)glGetString(GL_EXTENSIONS);
            if (!ext) {
                return false;
            }
            return strstr(ext, str) != NULL;
        }

        return false;
    }

    void init() {

        #ifdef _OS_ANDROID
            //void *libGL = dlopen("libGLESv2.so", RTLD_LAZY);
        #endif

        #if (defined(__SDL2__) && defined(_GAPI_GLES2))
                GetProcOGL(glDiscardFramebufferEXT);
        #endif

        #if defined(_OS_WIN) || defined(_OS_LINUX) || defined(_OS_GCW0) || (defined(__SDL2__) && !defined(_GAPI_GLES)) 
            #ifdef _OS_WIN
                GetProcOGL(glActiveTexture);
            #endif

#ifndef __LIBRETRO__
            #ifdef _OS_WIN
                GetProcOGL(wglSwapIntervalEXT);
            #elif _OS_LINUX
                GetProcOGL(glXSwapIntervalSGI);
            #endif
#endif

            #if defined(_OS_WIN) || defined(_OS_LINUX) && !(__LIBRETRO__) || (defined(__SDL2__) && (defined(_GAPI_GLES2) || defined(_SDL2_OPENGL)))
                GetProcOGL(glGenerateMipmap);
                #ifdef _OS_WIN
                    GetProcOGL(glTexImage3D);
                #endif

                GetProcOGL(glGetStringi);

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
                GetProcOGL(glGetFramebufferAttachmentParameteriv);

                GetProcOGL(glGenBuffers);
                GetProcOGL(glDeleteBuffers);
                GetProcOGL(glBindBuffer);
                GetProcOGL(glBufferData);
                GetProcOGL(glBufferSubData);
            #endif

            #ifndef __LIBRETRO_GLES__
                GetProcOGL(glGenVertexArrays);
                GetProcOGL(glDeleteVertexArrays);
                GetProcOGL(glBindVertexArray);

                GetProcOGL(glGetProgramBinary);
                GetProcOGL(glProgramBinary);
            #endif

            #if defined(_GAPI_GLES)
                GetProcOGL(glDiscardFramebufferEXT);
            #endif
        #endif

        LOG("Vendor   : %s\n", (char*)glGetString(GL_VENDOR));
        LOG("Renderer : %s\n", (char*)glGetString(GL_RENDERER));
        LOG("Version  : %s\n", (char*)glGetString(GL_VERSION));

    #ifndef FFP
        bool GLES3 = false;
        #ifdef _OS_WEB
            GLES3 = WEBGL_VERSION != 1;
        #else
            #if defined(_GAPI_GLES) && !defined(_GAPI_GLES2)
                int GLES_VERSION = 1;
                #if defined(__SDL2__)
                    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &GLES_VERSION);
                #else
                    #if defined(_OS_RPI) || defined(_OS_CLOVER) || defined(_OS_GCW0) || defined(HAVE_OPENGLES2)
                        GLES_VERSION = 2;
                    #else
                        glGetIntegerv(GL_MAJOR_VERSION, &GLES_VERSION);
                    #endif
                #endif 
                GLES3 = GLES_VERSION > 2; 
            #endif
        #endif

        bool _GL_EXT_shadow_samplers      = extSupport("GL_EXT_shadow_samplers");
        bool _GL_ARB_shadow               = extSupport("GL_ARB_shadow");
        bool _GL_OES_standard_derivatives = extSupport("GL_OES_standard_derivatives");

        support.shaderBinary   = extSupport("_program_binary");
        support.VAO            = GLES3 || extSupport("_vertex_array_object");
        support.VBO            = glGenBuffers != NULL;
        support.depthTexture   = GLES3 || extSupport("_depth_texture");
        support.shadowSampler  = _GL_EXT_shadow_samplers || _GL_ARB_shadow;
        support.discardFrame   = extSupport("_discard_framebuffer");
        support.texNPOT        = GLES3 || extSupport("_texture_npot") || extSupport("_texture_non_power_of_two");
        support.texRG          = GLES3 || extSupport("_texture_rg");
        support.texMaxLevel    = GLES3 || extSupport("_texture_max_level");

        #ifdef _GAPI_GLES2 // TODO
            support.shaderBinary = false;
            support.VAO = false;
            support.texRG = false;
            support.discardFrame = false;
        #endif
        #ifdef _GAPI_GLES
            support.derivatives = GLES3 || _GL_OES_standard_derivatives; 
            support.tex3D       = GLES3;
        #else
            support.derivatives = true;
#ifdef _GAPI_GLES2
            support.tex3D       = false;
#else
            support.tex3D       = glTexImage3D != NULL;
#endif
        #endif
        support.texBorder      = extSupport("_texture_border_clamp");
        support.maxAniso       = extSupport("_texture_filter_anisotropic");
        support.colorFloat     = extSupport("_color_buffer_float");
        support.colorHalf      = extSupport("_color_buffer_half_float") || extSupport("GL_ARB_half_float_pixel");
        support.texFloatLinear = support.colorFloat || extSupport("GL_ARB_texture_float") || extSupport("_texture_float_linear");
        support.texFloat       = support.texFloatLinear || extSupport("_texture_float");
        support.texHalfLinear  = support.colorHalf || extSupport("GL_ARB_texture_float") || extSupport("_texture_half_float_linear") || extSupport("_color_buffer_half_float");
 
        support.texHalf        = support.texHalfLinear || extSupport("_texture_half_float");

        #ifdef SDL2_GLES
            support.shaderBinary  = false; // TODO
            support.VAO           = false; // TODO
            support.shadowSampler = false; // TODO
        #endif

        #ifdef PROFILE
            support.profMarker = extSupport("_KHR_debug");
            support.profTiming = extSupport("_timer_query");
        #endif

        if (support.maxAniso)
            glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &support.maxAniso);
        #ifdef _GAPI_GLES
            glGetIntegerv(GL_MAX_VARYING_VECTORS, &support.maxVectors);
        #else
            support.maxVectors = 16;
        #endif
    #endif

        glEnable(GL_SCISSOR_TEST);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glDepthFunc(GL_LEQUAL);

    #ifdef FFP
        glEnable(GL_TEXTURE_2D);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);
        glEnableClientState(GL_VERTEX_ARRAY);
            
        glAlphaFunc(GL_GREATER, 0.5f);

        glMatrixMode(GL_TEXTURE);
        glLoadIdentity();
        glScalef(1.0f / 32767.0f, 1.0f / 32767.0f, 1.0f / 32767.0f);

        glClearColor(0, 0, 0, 0);
    #else
#ifdef __LIBRETRO__
        updateFBO();
#else
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&defaultFBO);
#endif
        glGenFramebuffers(1, &FBO);

        char extHeader[256];
        GLSL_HEADER_VERT[0] = GLSL_HEADER_FRAG[0] = extHeader[0] = 0;
        if (_GL_OES_standard_derivatives) {
            if (!GLES3) {
                strcat(extHeader, "#extension GL_OES_standard_derivatives : enable\n");
            }
        }

        if (_GL_EXT_shadow_samplers && !_GL_ARB_shadow) {
            if (!GLES3) {
                strcat(extHeader, "#extension GL_EXT_shadow_samplers : enable\n");
            }
        }
        
    #ifdef _GAPI_GLES
        if (GLES3) {
            // vertex
            strcat(GLSL_HEADER_VERT, "#version 300 es\n"
                                     "precision lowp  int;\n"
                                     "precision highp float;\n"
                                     "#define VERTEX\n"
                                     "#define varying   out\n"
                                     "#define attribute in\n"
                                     "#define texture2D texture\n");
            // fragment
            strcat(GLSL_HEADER_FRAG, "#version 300 es\n");
            strcat(GLSL_HEADER_FRAG, extHeader);
            strcat(GLSL_HEADER_FRAG, "precision lowp  int;\n"
                                     "precision highp float;\n"
                                     "precision lowp  sampler3D;\n"
                                     "#define FRAGMENT\n"
                                     "#define varying     in\n"
                                     "#define texture2D   texture\n"
                                     "#define texture3D   texture\n"
                                     "#define textureCube texture\n"
                                     "#define FETCH_SHADOW2D(a,b) texture(a,b)\n"
                                     "out vec4 fragColor;\n");
        } else {
            // vertex
            strcat(GLSL_HEADER_VERT, "#version 100\n"
                                     "precision lowp  int;\n"
                                     "precision highp float;\n"
                                     "#define VERTEX\n");
            // fragment
            strcat(GLSL_HEADER_FRAG, "#version 100\n");
            strcat(GLSL_HEADER_FRAG, extHeader);
            strcat(GLSL_HEADER_FRAG, "precision lowp  int;\n"
                                     "precision highp float;\n"
                                     "#define FRAGMENT\n"
                                     "#define FETCH_SHADOW2D(a,b) shadow2DEXT(a,b)\n"
                                     "#define fragColor gl_FragColor\n");
        }

        if (support.shadowSampler) {
            strcat(GLSL_HEADER_FRAG, "#define sampler2DShadow lowp sampler2DShadow\n");
        }
    #else
        if (GL_VER_3) {
            strcat(GLSL_HEADER_VERT, "#version 150\n"
                                     "#define VERTEX\n"
                                     "#define varying   out\n"
                                     "#define attribute in\n"
                                     "#define texture2D texture\n");
            // fragment
            strcat(GLSL_HEADER_FRAG, "#version 150\n");
            strcat(GLSL_HEADER_FRAG, extHeader);
            strcat(GLSL_HEADER_FRAG, "#define FRAGMENT\n"
                                     "#define varying     in\n"
                                     "#define texture2D   texture\n"
                                     "#define texture3D   texture\n"
                                     "#define textureCube texture\n"
                                     "#define FETCH_SHADOW2D(a,b) texture(a,b)\n"
                                     "out vec4 fragColor;\n");
        } else {
            // vertex
            strcat(GLSL_HEADER_VERT, "#version 110\n"
                                     "#define VERTEX\n");
            // fragment
            strcat(GLSL_HEADER_FRAG, "#version 110\n");
            strcat(GLSL_HEADER_FRAG, extHeader);
            strcat(GLSL_HEADER_FRAG, "#define FRAGMENT\n"
                                     "#define FETCH_SHADOW2D(a,b) shadow2D(a,b).x\n"
                                     "#define fragColor gl_FragColor\n");
        }
    #endif
        ASSERT(strlen(GLSL_HEADER_VERT) < COUNT(GLSL_HEADER_VERT));
        ASSERT(strlen(GLSL_HEADER_FRAG) < COUNT(GLSL_HEADER_FRAG));
    #endif // FFP
    }

    void deinit() {
        #ifdef FFP
            return;
        #endif
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &FBO);

        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        for (int b = 0; b < 2; b++) {
            for (int i = 0; i < rtCache[b].length; i++) {
                glDeleteRenderbuffers(1, &rtCache[b][i].ID);
            }
            rtCache[b].clear();
        }
    }

    inline mat4::ProjRange getProjRange() {
        return mat4::PROJ_NEG_POS;
    }

    mat4 ortho(float l, float r, float b, float t, float znear, float zfar) {
        mat4 m;
        m.ortho(getProjRange(), l, r, b, t, znear, zfar);
        return m;
    }

    mat4 perspective(float fov, float aspect, float znear, float zfar, float eye) {
        mat4 m;
        m.perspective(getProjRange(), fov, aspect, znear, zfar, eye);
        return m;
    }

    bool beginFrame() {
        return true;
    }

    void endFrame() {}

    void resetState() {
        if (Core::support.VAO)
            glBindVertexArray(0);

        #ifndef FFP
            glActiveTexture(GL_TEXTURE0);
            glUseProgram(0);
        #endif

        if (Core::support.VBO) {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
    }

    int cacheRenderTarget(bool depth, int width, int height) {
        Array<RenderTargetCacheItem> &items = rtCache[depth];

        for (int i = 0; i < items.length; i++)
            if (items[i].width == width && items[i].height == height) {
                RenderTargetCacheItem item = items[i];
                items.remove(i);
                return items.push(item);
            }

        if (items.length >= MAX_RENDER_BUFFERS) {
            glDeleteRenderbuffers(1, &items[0].ID);
            items.remove(0);
        }

        RenderTargetCacheItem item;
        item.width  = width;
        item.height = height;

        glGenRenderbuffers(1, &item.ID);
        glBindRenderbuffer(GL_RENDERBUFFER, item.ID);
        glRenderbufferStorage(GL_RENDERBUFFER, depth ? GL_DEPTH_COMPONENT16 : GL_RGB565, width, height);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        return items.push(item);
    }

    void bindTarget(Texture *target, int face) {
        if (!target) { // may be a null
            glBindFramebuffer(GL_FRAMEBUFFER, defaultFBO);
        } else {
            GLenum texTarget = GL_TEXTURE_2D;
            if (target->opt & OPT_CUBEMAP) {
                texTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X + face;
            }

            bool depth = target->fmt == FMT_DEPTH || target->fmt == FMT_SHADOW;

            int rtIndex = cacheRenderTarget(!depth, target->width, target->height);

            glBindFramebuffer(GL_FRAMEBUFFER, FBO);
            glFramebufferTexture2D    (GL_FRAMEBUFFER, depth ? GL_DEPTH_ATTACHMENT  : GL_COLOR_ATTACHMENT0, texTarget,       target->ID, 0);
            glFramebufferRenderbuffer (GL_FRAMEBUFFER, depth ? GL_COLOR_ATTACHMENT0 : GL_DEPTH_ATTACHMENT,  GL_RENDERBUFFER, rtCache[!depth].items[rtIndex].ID);
            GLuint status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if (status != GL_FRAMEBUFFER_COMPLETE) {
                LOG("status: 0x%04X\n", (int)status);
            }
        }
    }

    void discardTarget(bool color, bool depth) {
    #ifdef _GAPI_GLES
        if (Core::support.discardFrame) {
            int count = 0;
            GLenum discard[2];
            if (color) discard[count++] = Core::active.target ? GL_COLOR_ATTACHMENT0 : GL_COLOR_EXT;
            if (depth) discard[count++] = Core::active.target ? GL_DEPTH_ATTACHMENT  : GL_DEPTH_EXT;
            if (count) {
                #if defined(_OS_ANDROID) || (defined(__SDL2__) && !defined(_GAPI_GLES2))
                   /* glInvalidateBuffer() is the GLES3 version of glDiscardFramebufferEXT(), also
                      available on Android. Not available in any GLES2 implementation, this is GLES3 stuff.*/
                    glInvalidateFramebuffer(GL_FRAMEBUFFER, count, discard);
                #elif !defined(_OS_WEB) || (defined(__SDL2__) && defined(_GAPI_GLES2))
                    /* glDiscardFramebufferEXT() is available even in GLES2 MESA implementations,
                       but we have to get the extension function pointer address to use it.
                       Not available in GLES3, which is SDL2 default GLES version. */
                    glDiscardFramebufferEXT(GL_FRAMEBUFFER, count, discard);
                #endif
            }
        }
    #endif
    }

    void copyTarget(Texture *dst, int xOffset, int yOffset, int x, int y, int width, int height) {
        Core::active.textures[0] = NULL;
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, dst->ID);
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, xOffset, yOffset, x, y, width, height);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void setVSync(bool enable) {
        #ifdef _OS_WIN
            if (wglSwapIntervalEXT) wglSwapIntervalEXT(enable ? 1 : 0);
        #elif _OS_LINUX
            if (glXSwapIntervalSGI) glXSwapIntervalSGI(enable ? 1 : 0);
        #elif defined(__SDL2__)
            SDL_GL_SetSwapInterval(enable ? 1 : 0);
        #elif defined(_OS_RPI) || defined(_OS_CLOVER) || defined(_OS_SWITCH)
            eglSwapInterval(display, enable ? 1 : 0);
        #endif
    }

    void waitVBlank() {}

    void clear(bool color, bool depth) {
        uint32 mask = (color ? GL_COLOR_BUFFER_BIT : 0) | (depth ? GL_DEPTH_BUFFER_BIT : 0);
        if (mask) {
            if (depth && !depthWrite) {
                glDepthMask(GL_TRUE);
                glClear(mask);
                glDepthMask(GL_FALSE);
            } else {
                glClear(mask);
            }
        }
    }

    void setClearColor(const vec4 &color) {
        glClearColor(color.x, color.y, color.z, color.w);
    }

    void setViewport(const short4 &v) {
        glViewport(v.x, v.y, v.z, v.w);
    }

    void setScissor(const short4 &s) {
        glScissor(s.x, s.y, s.z, s.w);
    }

    void setDepthTest(bool enable) {
        if (enable)
            glEnable(GL_DEPTH_TEST);
        else
            glDisable(GL_DEPTH_TEST);
    }

    void setDepthWrite(bool enable) {
        depthWrite = enable;
        glDepthMask(enable ? GL_TRUE : GL_FALSE);
    }

    void setColorWrite(bool r, bool g, bool b, bool a) {
        glColorMask(r, g, b, a);
    }

    void setAlphaTest(bool enable) {
    #ifdef FFP
        if (enable)
            glEnable(GL_ALPHA_TEST);
        else
            glDisable(GL_ALPHA_TEST);
    #endif
    }

    void setCullMode(int rsMask) {
        cullMode = rsMask;
        switch (rsMask) {
            case RS_CULL_BACK  : glCullFace(GL_BACK);     break;
            case RS_CULL_FRONT : glCullFace(GL_FRONT);    break;
            default            : glDisable(GL_CULL_FACE); return;
        }
        glEnable(GL_CULL_FACE);
    }

    void setBlendMode(int rsMask) {
        blendMode = rsMask;
        switch (rsMask) {
            case RS_BLEND_ALPHA   : glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); break;
            case RS_BLEND_ADD     : glBlendFunc(GL_ONE, GL_ONE);                       break;
            case RS_BLEND_MULT    : glBlendFunc(GL_DST_COLOR, GL_ZERO);                break;
            case RS_BLEND_PREMULT : glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);       break;
            default               : glDisable(GL_BLEND); return;
        }
        glEnable(GL_BLEND);
    }

    void setViewProj(const mat4 &mView, const mat4 &mProj) {
    #ifdef FFP
        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf((float*)&mProj);
    #endif
    }

    void updateLights(vec4 *lightPos, vec4 *lightColor, int count) {
    #ifdef FFP
        int lightsCount = 0;

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        vec4 amb(vec3(Core::active.material.y), 1.0f);
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, (GLfloat*)&amb);

        for (int i = 0; i < count; i++) {
            GLenum light = GL_LIGHT0 + i;

            if (lightColor[i].w != 1.0f) {
                glEnable(light);
                lightsCount++;
            } else {
                glDisable(light);
                continue;
            }

            vec4 pos(lightPos[i].xyz(), 1.0f);
            vec4 color(lightColor[i].xyz(), 1.0f);
            float att = lightColor[i].w * lightColor[i].w;

            glLightfv(light, GL_POSITION, (GLfloat*)&pos);
            glLightfv(light, GL_DIFFUSE,  (GLfloat*)&color);
            glLightfv(light, GL_QUADRATIC_ATTENUATION, (GLfloat*)&att);
        }

        glPopMatrix();

        if (lightsCount) {
            glEnable(GL_COLOR_MATERIAL);
            glEnable(GL_LIGHTING);        
        } else {
            glDisable(GL_COLOR_MATERIAL);
            glDisable(GL_LIGHTING);
        }
    #else
        if (Core::active.shader) {
            Core::active.shader->setParam(uLightColor, lightColor[0], count);
            Core::active.shader->setParam(uLightPos,   lightPos[0],   count);
        }
    #endif
    }

    void setFog(const vec4 &params) {
        // FFP TODO
    }

    void DIP(Mesh *mesh, const MeshRange &range) {
    #ifdef FFP
        mat4 m = mView * mModel;
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf((GLfloat*)&m);
    #endif
        if (Core::active.shader) {
            Core::active.shader->validate();
        }

        glDrawElements(GL_TRIANGLES, range.iCount, sizeof(Index) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, mesh->iBuffer + range.iStart);
    }

    vec4 copyPixel(int x, int y) {
        ubyte4 c;
        glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &c);
        return vec4(float(c.x), float(c.y), float(c.z), float(c.w)) * (1.0f / 255.0f);
    }
}

#endif
