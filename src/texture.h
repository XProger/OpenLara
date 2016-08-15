#ifndef H_TEXTURE
#define H_TEXTURE

#include "core.h"

#define PVR_RGBA8	0x61626772
#define PVR_ALPHA	0x00000061
#define PVR_PVRTC4	0x00000002	// iOS
#define PVR_ETC1	0x00000006	// Android
#define PVR_BC1		0x00000007	// Desktop

#define GL_COMPRESSED_RGB_S3TC_DXT1		0x83F0
#define GL_ETC1_RGB8_OES				0x8D64
#define GL_COMPRESSED_RGB_PVRTC_4BPPV1	0x8C00

struct Texture {
	GLuint	ID;
	int		width, height;

	Texture(const char *name) {
		Stream stream(name);

		struct {
			int		version;
			int		flags;
			long	format;
			long	ext;
			int		color;
			int		channel;
			int		height;
			int		width;
			int		depth;
			int		surfaces;
			int		faces;
			int		mipCount;
			int		metaSize;
		} header;

		stream.read(header);
		stream.seek(header.metaSize);

		GLenum fmt;
		int minSize;
		switch (header.format) {
			case PVR_ALPHA  : minSize = 1; fmt = GL_ALPHA; break;
			case PVR_RGBA8	: minSize = 1; fmt = GL_RGBA; break;
			case PVR_BC1	: minSize = 4; fmt = GL_COMPRESSED_RGB_S3TC_DXT1; break;
			case PVR_ETC1	: minSize = 4; fmt = GL_ETC1_RGB8_OES; break;
			case PVR_PVRTC4	: minSize = 8; fmt = GL_COMPRESSED_RGB_PVRTC_4BPPV1; break;
			default			: 
				LOG("! unsupported texture format\n");
				ID = 0;
				return;
		}

		glGenTextures(1, &ID);
		bind(0);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, header.mipCount == 1 ? GL_LINEAR : GL_LINEAR_MIPMAP_LINEAR);

		int sizeX = width  = header.width;
		int sizeY = height = header.height;

		char *data = new char[width * height * 4];
		for (int i = 0; i < header.mipCount; i++) {
			if (minSize == 1) {
				int size = sizeX * sizeY * (header.format == PVR_RGBA8 ? 4 : 1);
				stream.read(data, size);
				glTexImage2D(GL_TEXTURE_2D, i, fmt, sizeX, sizeY, 0, fmt, GL_UNSIGNED_BYTE, data);
			} else {
				int size = (max(sizeX, minSize) * max(sizeY, minSize) * 4 + 7) / 8;
				stream.read(data, size);
				glCompressedTexImage2D(GL_TEXTURE_2D, i, fmt, sizeX, sizeY, 0, size, data);
			}
			sizeX /= 2;
			sizeY /= 2;
		}

		delete[] data;
	}

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