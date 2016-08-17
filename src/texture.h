#ifndef H_TEXTURE
#define H_TEXTURE

#include "core.h"

struct Texture {
	GLuint	ID;
	int		width, height;

	Texture(int width, int height, int format, void *data) : width(width), height(height) {
		glGenTextures(1, &ID);
		bind(0);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	}

	virtual ~Texture() {
		glDeleteTextures(1, &ID);
	}

	void bind(int sampler) {
		glActiveTexture(GL_TEXTURE0 + sampler);
		glBindTexture(GL_TEXTURE_2D, ID);
	}
};

#endif