#ifndef H_TEXTURE
#define H_TEXTURE

#include "core.h"

struct Texture {
    GLuint  ID;    
    int     width, height;
    bool    depth;
    bool    cube;
    Texture *dummy;

    Texture(int width, int height, bool depth, bool cube, void *data = NULL) : width(width), height(height), cube(cube), dummy(NULL) {
        glGenTextures(1, &ID);
        bind(0);

        bool filter = true;
        if (depth && !Core::support.depthTexture)
            depth = false;
        this->depth = depth;

        GLenum target = cube ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
        GLint format  = depth ? GL_DEPTH_COMPONENT : GL_RGBA;

        if (depth) {
            if (Core::support.shadowSampler) {
                glTexParameteri(target, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
                glTexParameteri(target, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
            } else
                filter = false;
        }       

        glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, filter ? GL_LINEAR : GL_NEAREST);
        glTexParameteri(target, GL_TEXTURE_MIN_FILTER, filter ? GL_LINEAR : GL_NEAREST);

        for (int i = 0; i < 6; i++) {
            glTexImage2D(cube ? (GL_TEXTURE_CUBE_MAP_POSITIVE_X + i) : GL_TEXTURE_2D, 0, format, width, height, 0, format, depth ? GL_UNSIGNED_SHORT : GL_UNSIGNED_BYTE, data);
            if (!cube) break;
        }

        if (depth)
            dummy = new Texture(width, height, false, false, NULL); // some drivers can't render to texture without color target, create dummy color target for fix it
    }

    virtual ~Texture() {
        delete dummy;
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