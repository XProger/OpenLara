#ifndef H_GAPI_GL
#define H_GAPI_GL

#include "core.h"

#ifdef _OS_WIN
    #include <gl/GL.h>
    #include <gl/glext.h>
#elif _OS_ANDROID
    #include <dlfcn.h>

    #include <GLES2/gl2.h>
    #include <GLES2/gl2ext.h>
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
#elif defined(_OS_RPI) || defined(_OS_CLOVER)
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
    
    extern EGLDisplay display;
#elif _OS_LINUX
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

        #undef  GL_RGBA32F
        #undef  GL_RGBA16F
        #undef  GL_HALF_FLOAT

        #define GL_RGBA32F      GL_RGBA
        #define GL_RGBA16F      GL_RGBA
        #define GL_HALF_FLOAT   GL_HALF_FLOAT_OES

        //#define GL_TEXTURE_COMPARE_MODE     GL_TEXTURE_COMPARE_MODE_EXT
        //#define GL_TEXTURE_COMPARE_FUNC     GL_TEXTURE_COMPARE_FUNC_EXT
        //#define GL_COMPARE_REF_TO_TEXTURE   GL_COMPARE_REF_TO_TEXTURE_EXT
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

#if defined(_OS_WIN) || defined(_OS_LINUX) || defined(_OS_ANDROID)

    #ifdef _OS_ANDROID
        #define GetProc(x) dlsym(libGL, x);
    #else
        void* GetProc(const char *name) {
            #ifdef _OS_WIN
                return (void*)wglGetProcAddress(name);
            #elif _OS_LINUX
                return (void*)glXGetProcAddress((GLubyte*)name);
            #else // EGL
                return (void*)eglGetProcAddress(name);
            #endif
        }
    #endif

    #define GetProcOGL(x) x=(decltype(x))GetProc(#x);

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

    #if defined(_OS_WIN) || defined(_OS_LINUX)
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

#if defined(_GAPI_GLES) && !defined(_OS_RPI) && !defined(_OS_CLOVER) && !defined(_OS_IOS)
    PFNGLDISCARDFRAMEBUFFEREXTPROC      glDiscardFramebufferEXT;
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

// Shader
    #ifndef FFP
        const char SHADER_BASE[] =
            #include "shaders/shader.glsl"
        ;

        const char SHADER_WATER[] =
            #include "shaders/water.glsl"
        ;

        const char SHADER_FILTER[] =
            #include "shaders/filter.glsl"
        ;

        const char SHADER_GUI[] =
            #include "shaders/gui.glsl"
        ;
        
        const char *DefineName[SD_MAX]  = { SHADER_DEFINES(DECL_STR) };
    #endif

    struct Shader {
    #ifdef FFP
        void init(Core::Pass pass, int type, int *def, int defCount) {}
        void deinit() {}
        void bind() {}
        void setParam(UniformType uType, const vec4  &value, int count = 1) {}
        void setParam(UniformType uType, const mat4  &value, int count = 1) {}
        void setParam(UniformType uType, const Basis &value, int count = 1) {}
    #else
        GLuint  ID;
        int32   uID[uMAX];

        void init(Core::Pass pass, int type, int *def, int defCount) {
            const char *source;
            switch (pass) {
                case Core::passCompose :
                case Core::passShadow  :
                case Core::passAmbient : source = SHADER_BASE;   break;
                case Core::passWater   : source = SHADER_WATER;  break;
                case Core::passFilter  : source = SHADER_FILTER; break;
                case Core::passGUI     : source = SHADER_GUI;    break;
                default                : ASSERT(false); LOG("! wrong pass id\n"); return;
            }

            char defines[1024];
            defines[0] = 0;

            for (int i = 0; i < defCount; i++) {
                #ifdef _GAPI_GLES
                    if (def[i] == SD_SHADOW_SAMPLER)
                        strcat(defines, "#extension GL_EXT_shadow_samplers : require\n"); // ACHTUNG! must be first in the list
                #endif
                sprintf(defines + strlen(defines), "#define %s\n", DefineName[def[i]]);
            }
            sprintf(defines + strlen(defines), "#define PASS_%s\n", passNames[pass]);

            #if defined(_OS_RPI) || defined(_OS_CLOVER)
                strcat(defines, "#define OPT_VLIGHTPROJ\n");
                strcat(defines, "#define OPT_VLIGHTVEC\n");
                strcat(defines, "#define OPT_SHADOW_ONETAP\n");
            #endif

            #ifndef _OS_CLOVER
                // TODO: only for non Mali-400?
                strcat(defines, "#define OPT_TRAPEZOID\n");
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

            Core::active.shader = this;
            glUseProgram(ID);

            for (int st = 0; st < sMAX; st++) {
                GLint idx = glGetUniformLocation(ID, (GLchar*)SamplerName[st]);
                if (idx != -1)
                    glUniform1iv(idx, 1, &st);
            }

            for (int ut = 0; ut < uMAX; ut++)
                uID[ut] = glGetUniformLocation(ID, (GLchar*)UniformName[ut]);
        }

        void deinit() {
            glDeleteProgram(ID);
        }

        bool linkSource(const char *text, const char *defines = "") {
            #ifdef _GAPI_GLES
                #define GLSL_DEFINE ""
                #define GLSL_VERT   ""
                #define GLSL_FRAG   "#extension GL_OES_standard_derivatives : enable\n"
            #else
                #define GLSL_DEFINE "#version 120\n"
                #define GLSL_VERT   ""
                #define GLSL_FRAG   ""
            #endif

            const int type[2] = { GL_VERTEX_SHADER, GL_FRAGMENT_SHADER };
            const char *code[2][4] = {
                    { GLSL_DEFINE GLSL_VERT "#define VERTEX\n",   defines, "#line 0\n", text },
                    { GLSL_DEFINE GLSL_FRAG "#define FRAGMENT\n", defines, "#line 0\n", text }
                };

            GLchar info[1024];

            for (int i = 0; i < 2; i++) {
                GLuint obj = glCreateShader(type[i]);
                glShaderSource(obj, 4, code[i], NULL);
                glCompileShader(obj);

                glGetShaderInfoLog(obj, sizeof(info), NULL, info);
                if (info[0]) LOG("! shader: %s\n", info);

                glAttachShader(ID, obj);
                glDeleteShader(obj);
            }

            for (int at = 0; at < aMAX; at++)
                glBindAttribLocation(ID, at, AttribName[at]);

            glLinkProgram(ID);

            glGetProgramInfoLog(ID, sizeof(info), NULL, info);
            if (info[0]) LOG("! program: %s\n", info);

            return checkLink();
        }
    
        bool linkBinary(const char *name) {
            // non-async code!
            char path[255];
            strcpy(path, Stream::cacheDir);
            strcat(path, name);

            if (!Stream::exists(path))
                return false;

            Stream *stream = new Stream(path);
            if (!stream)
                return false;

            GLenum size, format;
            stream->read(format);
            stream->read(size);
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
                glUseProgram(ID);
            }
        }

        void setParam(UniformType uType, const vec4 &value, int count = 1) {
            if (uID[uType] != -1) glUniform4fv(uID[uType], count, (GLfloat*)&value);
        }

        void setParam(UniformType uType, const mat4 &value, int count = 1) {
            if (uID[uType] != -1) glUniformMatrix4fv(uID[uType], count, false, (GLfloat*)&value);
        }

        void setParam(UniformType uType, const Basis &value, int count = 1) {
            if (uID[uType] != -1) glUniform4fv(uID[uType], count * 2, (GLfloat*)&value);
        }
    #endif
    };


// Texture
    struct Texture {
        uint32     ID;
        int        width, height, origWidth, origHeight;
        TexFormat  fmt;
        uint32     opt;

        Texture(int width, int height, uint32 opt) : ID(0), width(width), height(height), origWidth(width), origHeight(height), fmt(FMT_RGBA), opt(opt) {}

        void init(void *data) {
            ASSERT((opt & OPT_PROXY) == 0);

            bool filter   = (opt & OPT_NEAREST) == 0;
            bool mipmaps  = (opt & OPT_MIPMAPS) != 0;
            bool cube     = (opt & OPT_CUBEMAP) != 0;
            bool isShadow = fmt == FMT_SHADOW;

            glGenTextures(1, &ID);
            bind(0);

            GLenum target = cube ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;

            if (fmt == FMT_SHADOW) {
                glTexParameteri(target, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
                glTexParameteri(target, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
            }

            bool border = isShadow && Core::support.texBorder;
            glTexParameteri(target, GL_TEXTURE_WRAP_S, border ? GL_CLAMP_TO_BORDER : GL_CLAMP_TO_EDGE);
            glTexParameteri(target, GL_TEXTURE_WRAP_T, border ? GL_CLAMP_TO_BORDER : GL_CLAMP_TO_EDGE);
            if (border) {
                float color[] = { 1.0f, 1.0f, 1.0f, 1.0f };
                glTexParameterfv(target, GL_TEXTURE_BORDER_COLOR, color);
            }

            glTexParameteri(target, GL_TEXTURE_MIN_FILTER, filter ? (mipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR ) : ( mipmaps ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST ));
            glTexParameteri(target, GL_TEXTURE_MAG_FILTER, filter ? GL_LINEAR : GL_NEAREST);

            static const struct FormatDesc {
                GLuint ifmt, fmt;
                GLenum type;
            } formats[FMT_MAX] = {            
                { GL_RGBA,            GL_RGBA,            GL_UNSIGNED_BYTE          }, // RGBA
                { GL_RGB,             GL_RGB,             GL_UNSIGNED_SHORT_5_6_5   }, // RGB16
                { GL_RGBA,            GL_RGBA,            GL_UNSIGNED_SHORT_5_5_5_1 }, // RGBA16
                { GL_RGBA32F,         GL_RGBA,            GL_FLOAT                  }, // RGBA_FLOAT
                { GL_RGBA16F,         GL_RGBA,            GL_HALF_FLOAT             }, // RGBA_HALF
                { GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT         }, // DEPTH
                { GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT         }, // SHADOW
            };

            FormatDesc desc = formats[fmt];

            #ifdef _OS_WEB // fucking firefox!
                if (fmt == FMT_RGBA_FLOAT) {
                    if (Core::support.texFloat) {
                        desc.ifmt = GL_RGBA;
                        desc.type = GL_FLOAT;
                    }
                }

                if (fmt == FMT_RGBA_HALF) {
                    if (Core::support.texHalf) {
                        desc.ifmt = GL_RGBA;
                        desc.type = GL_HALF_FLOAT_OES;
                    }
                }
            #else
                if ((fmt == FMT_RGBA_FLOAT && !Core::support.colorFloat) || (fmt == FMT_RGBA_HALF && !Core::support.colorHalf)) {
                    desc.ifmt = GL_RGBA;
                    #ifdef _GAPI_GLES
                        if (fmt == FMT_RGBA_HALF)
                            desc.type = GL_HALF_FLOAT_OES;
                    #endif
                }
            #endif


            void *pix = data;
            if (data && !Core::support.texNPOT && (width != origWidth || height != origHeight))
                pix = NULL;

            for (int i = 0; i < 6; i++) {
                glTexImage2D(cube ? (GL_TEXTURE_CUBE_MAP_POSITIVE_X + i) : GL_TEXTURE_2D, 0, desc.ifmt, width, height, 0, desc.fmt, desc.type, pix);
                if (!cube) break;
            }

            if (pix != data)
                update(data);
        }

        void deinit() {
            if (ID)
                glDeleteTextures(1, &ID);
        }

        void generateMipMap() {
            bind(0);
            GLenum target = (opt & OPT_CUBEMAP) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;

            glGenerateMipmap(target);
            if (!(opt & OPT_CUBEMAP) && !(opt & OPT_NEAREST) && (Core::support.maxAniso > 0))
                glTexParameteri(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, min(int(Core::support.maxAniso), 8));
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 4);
        }

        void update(void *data) {
            bind(0);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, origWidth, origHeight, GL_RGBA, GL_UNSIGNED_BYTE, data);
        }

        void bind(int sampler) {
            if (opt & OPT_PROXY) return;
            ASSERT(ID);

            if (Core::active.textures[sampler] != this) {
                Core::active.textures[sampler] = this;
                glActiveTexture(GL_TEXTURE0 + sampler);
                glBindTexture((opt & OPT_CUBEMAP) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, ID);
            }
        }

        void unbind(int sampler) {
            if (Core::active.textures[sampler]) {
                Core::active.textures[sampler] = NULL;
                glActiveTexture(GL_TEXTURE0 + sampler);
                glBindTexture((opt & OPT_CUBEMAP) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, 0);
            }
        }

        void setFilterQuality(int value) {
            bool filter  = (opt & OPT_NEAREST) == 0 && (value > Core::Settings::LOW);
            bool mipmaps = (opt & OPT_MIPMAPS) != 0;

            Core::active.textures[0] = NULL;
            bind(0);
            if (Core::support.maxAniso > 0)
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, value > Core::Settings::MEDIUM ? min(int(Core::support.maxAniso), 8) : 1);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter ? (mipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR ) : ( mipmaps ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST ));
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter ? GL_LINEAR : GL_NEAREST);
        }
    };

// Mesh
    struct Mesh {
        Index        *iBuffer;
        GAPI::Vertex *vBuffer;
        GLuint       *VAO;
        GLuint       ID[2];

        int          iCount;
        int          vCount;
        int          aCount;
        bool         dynamic;

        Mesh(bool dynamic) : iBuffer(NULL), vBuffer(NULL), VAO(NULL), dynamic(dynamic) {
            ID[0] = ID[1] = 0;
        }

        void init(Index *indices, int iCount, ::Vertex *vertices, int vCount, int aCount) {
            this->iCount = iCount;
            this->vCount = vCount;
            this->aCount = aCount;

            if (Core::support.VAO)
                glBindVertexArray(Core::active.VAO = 0);

            #ifdef DYNGEOM_NO_VBO
                if (!vertices && !indices) {
                    iBuffer = new Index[iCount];
                    vBuffer = new GAPI::Vertex[vCount];
                    return;
                }
            #endif 

            ASSERT(sizeof(GAPI::Vertex) == sizeof(::Vertex));

            glGenBuffers(2, ID);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID[0]);
            glBindBuffer(GL_ARRAY_BUFFER,         ID[1]);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, iCount * sizeof(Index),  indices,  dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
            glBufferData(GL_ARRAY_BUFFER,         vCount * sizeof(Vertex), vertices, dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
            
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
                if (Core::active.iBuffer != ID[0])
                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Core::active.iBuffer = ID[0]);
                if (Core::active.vBuffer != ID[1])
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
    struct RenderTargetCache {
        int count;
        struct Item {
            GLuint  ID;
            int     width;
            int     height;
        } items[MAX_RENDER_BUFFERS];
    } rtCache[2];

    bool extSupport(const char *str, const char *ext) {
        if (!str) return false;
        return strstr(str, ext) != NULL;
    }

    void init() {
        memset(rtCache, 0, sizeof(rtCache));

        #ifdef _OS_ANDROID
            void *libGL = dlopen("libGLESv2.so", RTLD_LAZY);
        #endif

        #if defined(_OS_WIN) || defined(_OS_LINUX) || defined(_OS_ANDROID)
            #ifdef _OS_WIN
                GetProcOGL(glActiveTexture);
            #endif

            #ifdef _OS_WIN
                GetProcOGL(wglSwapIntervalEXT);
            #elif _OS_LINUX
                GetProcOGL(glXSwapIntervalSGI);
            #endif

            #if defined(_OS_WIN) || defined(_OS_LINUX)
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

            #ifdef _GAPI_GLES
                GetProcOGL(glDiscardFramebufferEXT);
            #endif

            GetProcOGL(glGenVertexArrays);
            GetProcOGL(glDeleteVertexArrays);
            GetProcOGL(glBindVertexArray);
            GetProcOGL(glGetProgramBinary);
            GetProcOGL(glProgramBinary);
        #endif

        LOG("Vendor   : %s\n", (char*)glGetString(GL_VENDOR));
        LOG("Renderer : %s\n", (char*)glGetString(GL_RENDERER));
        LOG("Version  : %s\n", (char*)glGetString(GL_VERSION));

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

    #ifdef FFP
        support.maxAniso       = 0;
        support.maxVectors     = 0;
        support.shaderBinary   = false;
        support.VAO            = false;
        support.depthTexture   = false;
        support.shadowSampler  = false;
        support.discardFrame   = false;
        support.texNPOT        = false;
        support.texRG          = false;
        support.texBorder      = false;
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

        #ifdef PROFILE
            support.profMarker = extSupport(ext, "_KHR_debug");
            support.profTiming = extSupport(ext, "_timer_query");
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

        glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&defaultFBO);
        glGenFramebuffers(1, &FBO);
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
    #endif
    }

    void deinit() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &FBO);

        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        for (int b = 0; b < 2; b++)
            for (int i = 0; i < rtCache[b].count; i++)
                glDeleteRenderbuffers(1, &rtCache[b].items[i].ID);
    }

    mat4 ortho(float l, float r, float b, float t, float znear, float zfar) {
        return mat4(mat4::PROJ_NEG_POS, l, r, b, t, znear, zfar);
    }

    mat4 perspective(float fov, float aspect, float znear, float zfar) {
        return mat4(mat4::PROJ_NEG_POS, fov, aspect, znear, zfar);
    }

    bool beginFrame() {
        return true;
    }

    void endFrame() {}

    void resetState() {
        if (Core::support.VAO)
            glBindVertexArray(0);
        glActiveTexture(GL_TEXTURE0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glUseProgram(0);
    }

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
        glRenderbufferStorage(GL_RENDERBUFFER, depth ? GL_DEPTH_COMPONENT16 : GL_RGB565, width, height);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        return cache.count++;
    }

    void bindTarget(Texture *target, int face) {
        if (!target) { // may be a null
            glBindFramebuffer(GL_FRAMEBUFFER, defaultFBO);
        } else {
            GLenum texTarget = GL_TEXTURE_2D;
            if (target->opt & OPT_CUBEMAP) 
                texTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X + face;

            bool depth = target->fmt == FMT_DEPTH || target->fmt == FMT_SHADOW;

            int rtIndex = cacheRenderTarget(!depth, target->width, target->height);

            glBindFramebuffer(GL_FRAMEBUFFER, FBO);
            glFramebufferTexture2D    (GL_FRAMEBUFFER, depth ? GL_DEPTH_ATTACHMENT  : GL_COLOR_ATTACHMENT0, texTarget,       target->ID, 0);
            glFramebufferRenderbuffer (GL_FRAMEBUFFER, depth ? GL_COLOR_ATTACHMENT0 : GL_DEPTH_ATTACHMENT,  GL_RENDERBUFFER, rtCache[!depth].items[rtIndex].ID);
        }
    }

    void discardTarget(bool color, bool depth) {
    #ifdef _GAPI_GLES
        if (Core::support.discardFrame) {
            int count = 0;
            GLenum discard[2];
            if (color) discard[count++] = Core::active.target ? GL_COLOR_ATTACHMENT0 : GL_COLOR_EXT;
            if (depth) discard[count++] = Core::active.target ? GL_DEPTH_ATTACHMENT  : GL_DEPTH_EXT;
            if (count)
                glDiscardFramebufferEXT(GL_FRAMEBUFFER, count, discard);
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
        #elif defined(_OS_RPI) || defined(_OS_CLOVER)
            eglSwapInterval(display, enable ? 1 : 0);
        #endif
    }

    void waitVBlank() {}

    void clear(bool color, bool depth) {
        uint32 mask = (color ? GL_COLOR_BUFFER_BIT : 0) | (depth ? GL_DEPTH_BUFFER_BIT : 0);
        if (mask) glClear(mask);
    }

    void setClearColor(const vec4 &color) {
        glClearColor(color.x, color.y, color.z, color.w);
    }

    void setViewport(const Viewport &vp) {
        glViewport(vp.x, vp.y, vp.width, vp.height);
        glScissor(vp.x, vp.y, vp.width, vp.height);
    }

    void setDepthTest(bool enable) {
        if (enable)
            glEnable(GL_DEPTH_TEST);
        else
            glDisable(GL_DEPTH_TEST);
    }

    void setDepthWrite(bool enable) {
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

    void DIP(Mesh *mesh, const MeshRange &range) {
    #ifdef FFP
        mat4 m = mView * mModel;
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf((GLfloat*)&m);
    #endif

        glDrawElements(GL_TRIANGLES, range.iCount, GL_UNSIGNED_SHORT, mesh->iBuffer + range.iStart);
    }

    vec4 copyPixel(int x, int y) {
        ubyte4 c;
        glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &c);
        return vec4(float(c.x), float(c.y), float(c.z), float(c.w)) * (1.0f / 255.0f);
    }

    void initPSO(PSO *pso) {
        ASSERT(pso);
        ASSERT(pso && pso->data == NULL);
        pso->data = &pso;
    }

    void deinitPSO(PSO *pso) {
        ASSERT(pso);
        ASSERT(pso->data != NULL);
        pso->data = NULL;
    }

    void bindPSO(const PSO *pso) {
        ASSERT(pso);
        ASSERT(pso->data != NULL);

        uint32 state = pso->renderState;
        uint32 mask  = 0;//mask;

        if (Core::active.pso)
            mask ^= Core::active.pso->renderState;

        if (!Core::active.pso || Core::active.pso->clearColor != pso->clearColor)
            setClearColor(pso->clearColor);

        if (mask & RS_DEPTH_TEST)
            setDepthTest((state & RS_DEPTH_TEST) != 0);
        
        if (mask & RS_DEPTH_WRITE)
            setDepthWrite((state & RS_DEPTH_WRITE) != 0);

        if (mask & RS_COLOR_WRITE)
            setColorWrite((state & RS_COLOR_WRITE_R) != 0, (state & RS_COLOR_WRITE_G) != 0, (state & RS_COLOR_WRITE_B) != 0, (state & RS_COLOR_WRITE_A) != 0);

        if (mask & RS_CULL)
            setCullMode(state & RS_CULL);

        if (mask & RS_BLEND)
            setBlendMode(state & RS_BLEND);

        if (mask & RS_DISCARD)
            setAlphaTest((state & RS_DISCARD) != 0);
    }
}

#endif
