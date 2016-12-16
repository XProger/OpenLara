#ifndef H_SHADER
#define H_SHADER

#include "core.h"

enum AttribType     { aCoord, aTexCoord, aNormal, aColor, aMAX };
enum SamplerType    { sDiffuse, sShadow, sMAX };
enum UniformType    { uViewProj, uViewInv, uModel, uLightProj, uParam, uColor, uViewPos, uLightPos, uLightColor, uLightTarget, uAnimTexRanges, uAnimTexOffsets, uMAX };

const char *AttribName[aMAX]    = { "aCoord", "aTexCoord", "aNormal", "aColor" };
const char *SamplerName[sMAX]   = { "sDiffuse", "sShadow" };
const char *UniformName[uMAX]   = { "uViewProj", "uViewInv", "uModel", "uLightProj", "uParam", "uColor", "uViewPos", "uLightPos", "uLightColor", "uLightTarget", "uAnimTexRanges", "uAnimTexOffsets" };

struct Shader {
    GLuint  ID;
    GLint   uID[uMAX];

    Shader(const char *text, const char *defines = "") {
        #ifdef MOBILE
	        #define GLSL_DEFINE "precision highp float;\n" "#define MOBILE\n"
        #else
	        #define GLSL_DEFINE "#version 120\n"
        #endif

        const int type[2] = { GL_VERTEX_SHADER, GL_FRAGMENT_SHADER };
        const char *code[2][3] = {
                { GLSL_DEFINE "#define VERTEX\n",   defines, text },
                { GLSL_DEFINE "#define FRAGMENT\n", defines, text }
            };

        GLchar info[256];

        ID = glCreateProgram();
        for (int i = 0; i < 2; i++) {
            GLuint obj = glCreateShader(type[i]);
            glShaderSource(obj, 3, code[i], NULL);
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
        if (Core::active.shader != this) {
            Core::active.shader = this;
            glUseProgram(ID);
        }
    }

    void setParam(UniformType uType, const vec2 &value, int count = 1) {
        if (uID[uType] != -1)
            glUniform2fv(uID[uType], count, (GLfloat*)&value);
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