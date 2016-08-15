#ifndef H_SHADER
#define H_SHADER

#include "core.h"

enum AttribType		{ aCoord, aTexCoord, aNormal, aMAX };
enum SamplerType	{ sTex0, sMAX };
enum UniformType	{ uViewProj, uModel, uLightVec, uMAX };

const char *AttribName[aMAX]	= { "aCoord", "aTexCoord", "aNormal" };
const char *SamplerName[sMAX]	= { "sTex0" };
const char *UniformName[uMAX]	= { "uViewProj", "uModel", "uLightVec" };

struct Shader {
	GLuint	ID;
	GLint	uID[uMAX];

	Shader(const char *name, int param) {
		Stream stream(name);

		char *text = new char[stream.size + 1];
		stream.read(text, stream.size);
		text[stream.size] = '\0';

		#define GLSL_DEFINE	"#version 110\n"

		const int type[2] = { GL_VERTEX_SHADER, GL_FRAGMENT_SHADER };
		const char *code[2][2] = { 
				{ GLSL_DEFINE "#define VERTEX\n",   text }, 
				{ GLSL_DEFINE "#define FRAGMENT\n", text } 
			};

		GLchar info[256];

		ID = glCreateProgram();
		for (int i = 0; i < 2; i++) {
			GLuint obj = glCreateShader(type[i]);
			glShaderSource(obj, 2, code[i], NULL);
			glCompileShader(obj);
		
			glGetShaderInfoLog(obj, sizeof(info), NULL, info);
			if (info[0]) LOG("! shader: %s\n", info);

			glAttachShader(ID, obj);
			glDeleteShader(obj);
		}
		delete[] text;

		for (int at = 0; at < aMAX; at++)
			glBindAttribLocation(ID, at, AttribName[at]);

		glLinkProgram(ID);

		glGetProgramInfoLog(ID, sizeof(info), NULL, info);	
		if (info[0]) LOG("! program: %s\n", info);

		bind();
		for (int st = 0; st < sMAX; st++)
			glUniform1iv(glGetUniformLocation(ID, (GLchar*)SamplerName[st]), 1, &st);
	
		for (int ut = 0; ut < uMAX; ut++)
			uID[ut] = glGetUniformLocation(ID, (GLchar*)UniformName[ut]);
	}
	
	virtual ~Shader() {
		glDeleteProgram(ID);
	}

	void bind() {
		glUseProgram(ID);
		setParam(uViewProj, Core::mViewProj);
		setParam(uModel, Core::mModel);
	}

	void setParam(UniformType uType, const vec3 &value, int count = 1) {
		if (uID[uType] != -1)
			glUniform3fv(uID[uType], count, (GLfloat*)&value);
	}

	void setParam(UniformType uType, const vec4 &value, int count = 1) {
		if (uID[uType] != -1)
			glUniform4fv(uID[uType], count, (GLfloat*)&value);
	}

	void setParam(UniformType uType, const mat4 &value, int count = 1) {
		if (uID[uType] != -1)
			glUniformMatrix4fv(uID[uType], count, false, (GLfloat*)&value);
	}
};

#endif