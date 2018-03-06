#ifndef H_SHADER
#define H_SHADER

#include "core.h"

#define SHADER_ATTRIBS(E) \
    E( aCoord           ) \
    E( aNormal          ) \
    E( aTexCoord        ) \
    E( aColor           ) \
    E( aLight           )

#define SHADER_SAMPLERS(E) \
    E( sDiffuse         ) \
    E( sNormal          ) \
    E( sReflect         ) \
    E( sShadow          ) \
    E( sEnvironment     ) \
    E( sMask            )

#define SHADER_UNIFORMS(E) \
    E( uParam           ) \
    E( uTexParam        ) \
    E( uViewProj        ) \
    E( uBasis           ) \
    E( uLightProj       ) \
    E( uMaterial        ) \
    E( uAmbient         ) \
    E( uFogParams       ) \
    E( uViewPos         ) \
    E( uLightPos        ) \
    E( uLightColor      ) \
    E( uAnimTexRanges   ) \
    E( uAnimTexOffsets  ) \
    E( uRoomSize        ) \
    E( uPosScale        ) \
    E( uContacts        )

enum AttribType  { SHADER_ATTRIBS(DECL_ENUM)  aMAX };
enum SamplerType { SHADER_SAMPLERS(DECL_ENUM) sMAX };
enum UniformType { SHADER_UNIFORMS(DECL_ENUM) uMAX };

const char *AttribName[aMAX]  = { SHADER_ATTRIBS(DECL_STR)  };
const char *SamplerName[sMAX] = { SHADER_SAMPLERS(DECL_STR) };
const char *UniformName[uMAX] = { SHADER_UNIFORMS(DECL_STR) };

#undef SHADER_ATTRIBS
#undef SHADER_SAMPLERS
#undef SHADER_UNIFORMS
#undef ENUM
#undef STR

struct Shader {
    vec4    params[uMAX][4];

    enum Type { 
        DEFAULT = 0,
        /* shader */ SPRITE = 0, FLASH = 1, ROOM = 2, ENTITY = 3, MIRROR = 4, 
        /* filter */ FILTER_DOWNSAMPLE = 1, FILTER_GRAYSCALE = 2, FILTER_BLUR = 3, FILTER_MIXER = 4, FILTER_EQUIRECTANGULAR = 5,
        /* water  */ WATER_DROP = 0, WATER_STEP = 1, WATER_CAUSTICS = 2, WATER_MASK = 3, WATER_COMPOSE = 4,
        MAX = 6
    };
#ifdef FFP
    Shader(const char *source, const char *defines = "") {}
    virtual ~Shader() {}
    bool bind() { return true; }
    void setParam(UniformType uType, const int &value, int count = 1) {}
    void setParam(UniformType uType, const float &value, int count = 1) {}
    void setParam(UniformType uType, const vec2 &value, int count = 1) {}
    void setParam(UniformType uType, const vec3 &value, int count = 1) {}
    void setParam(UniformType uType, const vec4 &value, int count = 1) {}
    void setParam(UniformType uType, const mat4 &value, int count = 1) {}
    void setParam(UniformType uType, const Basis &value, int count = 1) {}
#else
    uint32  ID;
    int32   uID[uMAX];

    Shader(const char *source, const char *defines = "") {
        char fileName[255];
    // generate shader file path
        if (Core::support.shaderBinary) {
            uint32 hash = fnv32(defines, strlen(defines), fnv32(source, strlen(source)));
            sprintf(fileName, "%08X.xsh", hash);
        }

        ID = glCreateProgram();

        if (!(Core::support.shaderBinary && linkBinary(fileName))) // try to load cached shader     
            if (linkSource(source, defines) && Core::support.shaderBinary) { // compile shader from source and dump it into cache
            #ifndef __EMSCRIPTEN__
                GLenum format, size;
                glGetProgramiv(ID, GL_PROGRAM_BINARY_LENGTH, (GLsizei*)&size);
                char *data = new char[8 + size];
                glGetProgramBinary(ID, size, NULL, &format, &data[8]);
                *(int*)(&data[0]) = format;
                *(int*)(&data[4]) = size;
                Stream::cacheWrite(fileName, data, 8 + size);
                delete[] data;
            #endif
            }

        init();
    }

    bool linkSource(const char *text, const char *defines = "") {
        #ifdef MOBILE
            #define GLSL_DEFINE ""
            #define GLSL_VERT   ""
            #define GLSL_FRAG   "#extension GL_OES_standard_derivatives : enable\n"
        #else
            #define GLSL_DEFINE "#version 120\n"
            #define GLSL_VERT   ""
            #define GLSL_FRAG   ""
        #endif

        const int type[2] = { GL_VERTEX_SHADER, GL_FRAGMENT_SHADER };
        const char *code[2][3] = {
                { GLSL_DEFINE GLSL_VERT "#define VERTEX\n",   defines, text },
                { GLSL_DEFINE GLSL_FRAG "#define FRAGMENT\n", defines, text }
            };

        GLchar info[1024];

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

    virtual ~Shader() {
        glDeleteProgram(ID);
    }

    void init() {
        bind();
        for (int st = 0; st < sMAX; st++) {
            GLint idx = glGetUniformLocation(ID, (GLchar*)SamplerName[st]);
            if (idx != -1)
                glUniform1iv(idx, 1, &st);
        }

        for (int ut = 0; ut < uMAX; ut++)
            uID[ut] = glGetUniformLocation(ID, (GLchar*)UniformName[ut]);

        memset(params, 0, sizeof(params));
    }

    bool bind() {
        if (Core::active.shader != this) {
            Core::active.shader = this;
            glUseProgram(ID);
            return true;
        }
        return false;
    }

    void setParam(UniformType uType, const int &value, int count = 1) {
        if (uID[uType] != -1) glUniform1iv(uID[uType], count, (GLint*)&value);
    }

    void setParam(UniformType uType, const float &value, int count = 1) {
        if (uID[uType] != -1) glUniform1fv(uID[uType], count, (GLfloat*)&value);
    }

    void setParam(UniformType uType, const vec2 &value, int count = 1) {
        if (uID[uType] != -1) glUniform2fv(uID[uType], count, (GLfloat*)&value);
    }

    void setParam(UniformType uType, const vec3 &value, int count = 1) {
        if (uID[uType] != -1) glUniform3fv(uID[uType], count, (GLfloat*)&value);
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

#endif
