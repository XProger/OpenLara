#ifndef H_TEXTURE
#define H_TEXTURE

#include "core.h"

struct Texture {
    GLuint  ID;    
    int     width, height;
    bool    depth;
    Texture *dummy;

    Texture(int width, int height, bool depth, void *data = NULL) : width(width), height(height), dummy(NULL) {
        glGenTextures(1, &ID);
        bind(0);

        int filter = 1;
        if (depth && !Core::support.depthTexture)
            depth = false;
        this->depth = depth;

        if (depth) {
            if (Core::support.shadowSampler) {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
            } else
                filter = 0;
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter ? GL_LINEAR : GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter ? GL_LINEAR : GL_NEAREST);

        GLint format = depth ? GL_DEPTH_COMPONENT : GL_RGBA;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, depth ? GL_UNSIGNED_SHORT : GL_UNSIGNED_BYTE, data);

        if (depth)
            dummy = new Texture(width, height, false, NULL); // some drivers can't render to texture without color target, create dummy color target for fix it
    }

    virtual ~Texture() {
        delete dummy;
        glDeleteTextures(1, &ID);
    }

    void bind(int sampler) {
        glActiveTexture(GL_TEXTURE0 + sampler);
        glBindTexture(GL_TEXTURE_2D, ID);
    }

    static void unbind(int sampler) {
        glActiveTexture(GL_TEXTURE0 + sampler);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
};

#endif