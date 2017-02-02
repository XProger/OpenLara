#ifndef H_TEXTURE
#define H_TEXTURE

#include "core.h"

struct Texture {
    enum Format : uint32 { RGBA, RGBA_FLOAT, RGBA_HALF, RED, DEPTH, SHADOW, MAX };

    GLuint  ID;
    int     width, height;
    Format  format;
    bool    cube;

    Texture(int width, int height, Format format, bool cube, void *data = NULL, bool filter = true) : width(width), height(height), cube(cube) {
        glGenTextures(1, &ID);
        bind(0);

        GLenum target = cube ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;

        if (format == SHADOW && !Core::support.shadowSampler) {
            format = DEPTH;
            filter = false;
        }

        if (format == DEPTH) {
            if (Core::support.depthTexture)
                filter = false;
            else
                format = RGBA;
        }

        if (format == RGBA_HALF) {
            if (Core::support.texHalf)
                filter = Core::support.texHalfLinear;
            else
                format = RGBA_FLOAT;
        }

        if (format == RGBA_FLOAT) {
            if (Core::support.texFloat)
                filter = Core::support.texFloatLinear;
            else
                format = RGBA;
        }

        this->format = format;

        if (format == SHADOW) {
            glTexParameteri(target, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
            glTexParameteri(target, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
        }

        glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, filter ? GL_LINEAR : GL_NEAREST);
        glTexParameteri(target, GL_TEXTURE_MIN_FILTER, filter ? GL_LINEAR : GL_NEAREST);
        
        struct FormatDesc {
            GLuint ifmt, fmt;
            GLenum type;
        } formats[MAX] = {
            { GL_RGBA,            GL_RGBA,            GL_UNSIGNED_BYTE  },    // RGBA
            { GL_RGBA32F,         GL_RGBA,            GL_FLOAT          },    // RGBA_FLOAT
            { GL_RGBA16F,         GL_RGBA,            GL_HALF_FLOAT     },    // RGBA_HALF
            { GL_RED,             GL_RED,             GL_UNSIGNED_BYTE  },    // RED
            { GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT },    // DEPTH
            { GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT },    // SHADOW
        };

        FormatDesc &desc = formats[format];
        
        for (int i = 0; i < 6; i++) {
            glTexImage2D(cube ? (GL_TEXTURE_CUBE_MAP_POSITIVE_X + i) : GL_TEXTURE_2D, 0, desc.ifmt, width, height, 0, desc.fmt, desc.type, data);
            if (!cube) break;
        }
    }

    virtual ~Texture() {
        glDeleteTextures(1, &ID);
    }

    void bind(int sampler) {
        glActiveTexture(GL_TEXTURE0 + sampler);
        glBindTexture(cube ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, ID);
    }

    void unbind(int sampler) {
        glActiveTexture(GL_TEXTURE0 + sampler);
        glBindTexture(cube ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, 0);
    }
};

#endif