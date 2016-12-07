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
/*
 * In compatibility mode, Mac OS X only supports OpenGL 2 (no VAO), but it does 
 * support the Apple-specific VAO extension which is older and in all relevant 
 * parts 100% compatible. So use those functions instead.
 */
#define glBindVertexArray glBindVertexArrayAPPLE
#define glGenVertexArrays glGenVertexArraysAPPLE
#define glDeleteVertexArrays glDeleteVertexArraysAPPLE
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
// Render to texture
	PFNGLGENFRAMEBUFFERSPROC			glGenFramebuffers;
	PFNGLBINDFRAMEBUFFERPROC			glBindFramebuffer;
	PFNGLFRAMEBUFFERTEXTURE2DPROC		glFramebufferTexture2D;
	PFNGLGENRENDERBUFFERSPROC			glGenRenderbuffers;
	PFNGLBINDRENDERBUFFERPROC			glBindRenderbuffer;
	PFNGLFRAMEBUFFERRENDERBUFFERPROC	glFramebufferRenderbuffer;
	PFNGLRENDERBUFFERSTORAGEPROC		glRenderbufferStorage;
	PFNGLCHECKFRAMEBUFFERSTATUSPROC		glCheckFramebufferStatus;
	PFNGLDRAWBUFFERSPROC				glDrawBuffers;
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
    mat4 mView, mProj, mViewProj, mViewInv, mModel, mLightProj;
    vec3 viewPos;
    vec3 lightPos[MAX_LIGHTS];
    vec4 lightColor[MAX_LIGHTS];
    vec4 color;

    GLuint RT;

    struct {
        Shader  *shader;
        Texture *testures[8];
    } active;

    struct {
        int dips;
        int tris;
    } stats;

    struct {
        bool depthTexture;
        bool shadowSampler;
        bool VAO;
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

        GetProcOGL(glGenFramebuffers);
        GetProcOGL(glBindFramebuffer);
        GetProcOGL(glFramebufferTexture2D);
        GetProcOGL(glGenRenderbuffers);
        GetProcOGL(glBindRenderbuffer);
        GetProcOGL(glFramebufferRenderbuffer);
        GetProcOGL(glRenderbufferStorage);
        GetProcOGL(glCheckFramebufferStatus);
        GetProcOGL(glDrawBuffers);

        #ifdef PROFILE
            GetProcOGL(glObjectLabel);
            GetProcOGL(glPushDebugGroup);
            GetProcOGL(glPopDebugGroup);
        #endif
    #endif
		char *ext = (char*)glGetString(GL_EXTENSIONS);
		//LOG("%s\n", ext);
		support.depthTexture  = extSupport(ext, "_depth_texture");
		support.shadowSampler = extSupport(ext, "EXT_shadow_samplers") || extSupport(ext, "GL_ARB_shadow");
        support.VAO           = (void*)glBindVertexArray != NULL;
		
		LOG("Vendor   : %s\n", glGetString(GL_VENDOR));
		LOG("Renderer : %s\n", glGetString(GL_RENDERER));
		LOG("Version  : %s\n", glGetString(GL_VERSION));

		LOG("supports:\n");
		LOG("  depth texture  : %s\n", support.depthTexture ? "true" : "false");
		LOG("  shadow sampler : %s\n", support.shadowSampler ? "true" : "false");
		LOG("  vertex arrays  : %s\n", support.VAO ? "true" : "false");
        LOG("\n");

        glGenFramebuffers(1, &RT);

        Sound::init();

        for (int i = 0; i < MAX_LIGHTS; i++)
            lightColor[i] = vec4(0, 0, 0, 1);
    }

    void free() {
    //    glDeleteFrameBuffers(1, &RT);
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

    void setTarget(Texture *target) {
		if (!target)  {
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			return;
		}

		glBindFramebuffer(GL_FRAMEBUFFER, RT);
		glFramebufferTexture2D(GL_FRAMEBUFFER, target->depth ? GL_DEPTH_ATTACHMENT  : GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target->ID, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, target->depth ? GL_COLOR_ATTACHMENT0 : GL_DEPTH_ATTACHMENT,  GL_TEXTURE_2D, 0, 0);

		GLenum  buffers[] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(target->depth ? 0 : 1, buffers);
	}
}

#endif
