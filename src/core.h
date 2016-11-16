#ifndef H_CORE
#define H_CORE

#include <stdio.h>
#ifdef WIN32
    #include <windows.h>
    #include <gl/GL.h>
    #include <gl/glext.h>
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
#elif __EMSCRIPTEN__
    #define MOBILE 1
    #include <emscripten.h>
    #include <html5.h>
    #include <GLES3/gl3.h>
    #include <GLES3/gl2ext.h>
#endif

#include "utils.h"
#include "input.h"
#include "sound.h"

#if defined(WIN32) || defined(LINUX)
    void* GetProc(const char *name) {
        #ifdef WIN32
    	    return (void*)wglGetProcAddress(name);
        #elif LINUX
	        return (void*)glXGetProcAddress((GLubyte*)name);
        #endif
    }

    #define GetProcOGL(x) x=(decltype(x))GetProc(#x);

// Texture
    #ifdef WIN32
    PFNGLACTIVETEXTUREPROC              glActiveTexture;
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
    PFNGLUNIFORM2FVPROC                 glUniform2fv;
    PFNGLUNIFORM3FVPROC                 glUniform3fv;
    PFNGLUNIFORM4FVPROC                 glUniform4fv;
    PFNGLUNIFORMMATRIX4FVPROC           glUniformMatrix4fv;
    PFNGLBINDATTRIBLOCATIONPROC         glBindAttribLocation;
    PFNGLENABLEVERTEXATTRIBARRAYPROC    glEnableVertexAttribArray;
    PFNGLDISABLEVERTEXATTRIBARRAYPROC   glDisableVertexAttribArray;
    PFNGLVERTEXATTRIBPOINTERPROC        glVertexAttribPointer;
// Mesh
    PFNGLGENBUFFERSARBPROC              glGenBuffers;
    PFNGLDELETEBUFFERSARBPROC           glDeleteBuffers;
    PFNGLBINDBUFFERARBPROC              glBindBuffer;
    PFNGLBUFFERDATAARBPROC              glBufferData;
    PFNGLGENVERTEXARRAYSPROC            glGenVertexArrays;
    PFNGLDELETEVERTEXARRAYSPROC         glDeleteVertexArrays;
    PFNGLBINDVERTEXARRAYPROC            glBindVertexArray;
// Profiling
    #ifdef PROFILE
        PFNGLOBJECTLABELPROC                glObjectLabel;
        PFNGLPUSHDEBUGGROUPPROC             glPushDebugGroup;
        PFNGLPOPDEBUGGROUPPROC              glPopDebugGroup;
    #endif
#endif

#define MAX_LIGHTS 3

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
    float deltaTime;
    mat4 mView, mProj, mViewProj, mViewInv, mModel;
    vec3 viewPos;
    vec3 lightPos[MAX_LIGHTS];
    vec4 lightColor[MAX_LIGHTS];
    vec3 ambient;
    vec4 color;

    struct {
        Shader  *shader;
        Texture *testures[8];
    } active;

    struct {
        int dips;
        int tris;
    } stats;

    struct {
        bool VAO;
    } support;
}

#include "texture.h"
#include "shader.h"
#include "mesh.h"

enum CullMode  { cfNone, cfBack, cfFront };
enum BlendMode { bmNone, bmAlpha, bmAdd, bmMultiply, bmScreen };

namespace Core {

    void init() {
    #if defined(WIN32) || defined(LINUX)
        #ifdef WIN32
        GetProcOGL(glActiveTexture);
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
        GetProcOGL(glUniform2fv);
        GetProcOGL(glUniform3fv);
        GetProcOGL(glUniform4fv);
        GetProcOGL(glUniformMatrix4fv);
        GetProcOGL(glBindAttribLocation);
        GetProcOGL(glEnableVertexAttribArray);
        GetProcOGL(glDisableVertexAttribArray);
        GetProcOGL(glVertexAttribPointer);

        GetProcOGL(glGenBuffers);
        GetProcOGL(glDeleteBuffers);
        GetProcOGL(glBindBuffer);
        GetProcOGL(glBufferData);
        GetProcOGL(glGenVertexArrays);
        GetProcOGL(glDeleteVertexArrays);
        GetProcOGL(glBindVertexArray);
        #ifdef PROFILE
            GetProcOGL(glObjectLabel);
            GetProcOGL(glPushDebugGroup);
            GetProcOGL(glPopDebugGroup);
        #endif
    #endif
        support.VAO = (void*)glBindVertexArray != NULL;

        Sound::init();

        for (int i = 0; i < MAX_LIGHTS; i++)
            lightColor[i] = vec4(0, 0, 0, 1);
    }

    void free() {
        Sound::free();
    }

    void clear(const vec4 &color) {
        glClearColor(color.x, color.y, color.z, color.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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
}

#endif
