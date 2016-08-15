#ifndef H_CORE
#define H_CORE

#include <stdio.h>
#include <windows.h>
#include <gl/GL.h>
#include <gl/glext.h>

#include "utils.h"
#include "input.h"

#ifdef WIN32
	#if defined(_MSC_VER) // Visual Studio
		#define GetProcOGL(x) *(void**)&x=(void*)wglGetProcAddress(#x);
	#else // GCC
		#define GetProcOGL(x) x=(typeof(x))wglGetProcAddress(#x);
	#endif

// Texture
	PFNGLACTIVETEXTUREPROC				glActiveTexture;
	PFNGLCOMPRESSEDTEXIMAGE2DPROC		glCompressedTexImage2D;
// Shader
	PFNGLCREATEPROGRAMPROC				glCreateProgram;
	PFNGLDELETEPROGRAMPROC				glDeleteProgram;
	PFNGLLINKPROGRAMPROC				glLinkProgram;
	PFNGLUSEPROGRAMPROC					glUseProgram;
	PFNGLGETPROGRAMINFOLOGPROC			glGetProgramInfoLog;
	PFNGLCREATESHADERPROC				glCreateShader;
	PFNGLDELETESHADERPROC				glDeleteShader;
	PFNGLSHADERSOURCEPROC				glShaderSource;
	PFNGLATTACHSHADERPROC				glAttachShader;
	PFNGLCOMPILESHADERPROC				glCompileShader;
	PFNGLGETSHADERINFOLOGPROC			glGetShaderInfoLog;
	PFNGLGETUNIFORMLOCATIONPROC			glGetUniformLocation;
	PFNGLUNIFORM1IVPROC					glUniform1iv;
	PFNGLUNIFORM3FVPROC					glUniform3fv;
	PFNGLUNIFORM4FVPROC					glUniform4fv;
	PFNGLUNIFORMMATRIX4FVPROC			glUniformMatrix4fv;
	PFNGLBINDATTRIBLOCATIONPROC			glBindAttribLocation;
	PFNGLENABLEVERTEXATTRIBARRAYPROC	glEnableVertexAttribArray;
	PFNGLDISABLEVERTEXATTRIBARRAYPROC	glDisableVertexAttribArray;
	PFNGLVERTEXATTRIBPOINTERPROC		glVertexAttribPointer;
// Mesh
	PFNGLGENBUFFERSARBPROC				glGenBuffers;
	PFNGLDELETEBUFFERSARBPROC			glDeleteBuffers;
	PFNGLBINDBUFFERARBPROC				glBindBuffer;
	PFNGLBUFFERDATAARBPROC				glBufferData;
#endif

namespace Core {
	int width, height;
	float deltaTime;
	mat4 mView, mProj, mViewProj, mModel;
}

#include "texture.h"
#include "shader.h"
#include "mesh.h"

enum CullMode { cfNone, cfBack, cfFront };
enum BlendMode { bmNone, bmAlpha, bmAdd, bmMultiply, bmScreen };

namespace Core {

	void init() {
		GetProcOGL(glActiveTexture);
		GetProcOGL(glCompressedTexImage2D);

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
	}

	void free() {
		//
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

		if (mode != bmNone)
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