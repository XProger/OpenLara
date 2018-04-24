#ifndef H_TEXTURE
#define H_TEXTURE

#include "core.h"
#include "format.h"

struct Texture {
    enum Option { CUBEMAP = 1, MIPMAPS = 2, NEAREST = 4 };

    int     width, height, origWidth, origHeight;
    Format  format;
    uint32  opt;

#ifdef _OS_PSP
    TR::Tile4 *tiles;
    TR::CLUT  *cluts;
    uint8     *memory;

    void swizzle(uint8* out, const uint8* in, uint32 width, uint32 height) {
        int rowblocks = width / 16;

        for (int j = 0; j < height; j++)
            for (int i = 0; i < width; i++) {
                int blockx = i / 16;
                int blocky = j / 8;

                int x = i - blockx * 16;
                int y = j - blocky * 8;
                int block_index   = blockx + blocky * rowblocks;
                int block_address = block_index * 16 * 8;

                out[block_address + x + y * 16] = in[i + j * width];
            }
    }

#else
    uint32  ID;
    Texture *tiles[32];
#endif

    #ifdef SPLIT_BY_TILE

        #ifdef _OS_PSP
            Texture(TR::Tile4 *tiles, int tilesCount, TR::CLUT *cluts, int clutsCount) : width(256), height(256), memory(NULL) {
                #ifdef EDRAM_TEX
                    this->tiles = (TR::Tile4*)Core::allocEDRAM(tilesCount * sizeof(tiles[0]));
                    this->cluts =  (TR::CLUT*)Core::allocEDRAM(clutsCount * sizeof(cluts[0]));
                    memcpy(this->cluts, cluts, clutsCount * sizeof(cluts[0]));
                    #ifdef TEX_SWIZZLE
                        for (int i = 0; i < tilesCount; i++)
                            swizzle((uint8*)&this->tiles[i], (uint8*)&tiles[i], width / 2, height);
                    #else
                        memcpy(this->tiles, tiles, tilesCount * sizeof(tiles[0]));
                    #endif
                #else
                    this->tiles = tiles;
                    this->cluts = cluts;
                #endif
            }
        #else
            Texture(TR::Tile32 *tiles, int tilesCount) : width(256), height(256), ID(0) {
                memset(this->tiles, 0, sizeof(this->tiles));

                ASSERT(tilesCount < COUNT(this->tiles));
                for (int i = 0; i < tilesCount; i++)
                    this->tiles[i] = new Texture(width, height, RGBA, 0, tiles + i);
            }
        #endif

        void bind(uint16 tile, uint16 clut) {
        #ifdef _OS_PSP
            int swizzle = GU_FALSE;
            #ifdef TEX_SWIZZLE
                swizzle = GU_TRUE;
            #endif
            sceGuTexMode(GU_PSM_T4, 0, 0, swizzle);
            sceGuClutLoad(1, cluts + clut);
            sceGuTexImage(0, width, height, width, tiles + tile);
        #else
            tiles[tile]->bind(0);
        #endif
        }
    #endif

    Texture(int width, int height, Format format, uint32 opt = 0, void *data = NULL) : opt(opt) {
//        LOG("create texture %d x %d (%d)\n", width, height, format);

        #ifndef _OS_PSP
            #ifdef SPLIT_BY_TILE
                memset(this->tiles, 0, sizeof(tiles));
            #endif
        #endif

        origWidth  = width;
        origHeight = height;

        if (!Core::support.texNPOT) {
            width  = nextPow2(width);
            height = nextPow2(height);
        }
        this->width  = width;
        this->height = height;

        bool   filter   = (opt & NEAREST) == 0;
        bool   cube     = (opt & CUBEMAP) != 0;
        bool   mipmaps  = (opt & MIPMAPS) != 0;
        bool   isShadow = format == FMT_SHADOW;

        if (format == FMT_SHADOW && !Core::support.shadowSampler) {
            format = FMT_DEPTH;
            filter = false;
        }

        if (format == FMT_DEPTH) {
            if (Core::support.depthTexture)
                filter = false;
            else
                format = FMT_RGBA;
        }

        if (format == FMT_RGBA_HALF) {
            if (Core::support.texHalf)
                filter = filter && Core::support.texHalfLinear;
            else
                format = FMT_RGBA_FLOAT;
        }

        if (format == FMT_RGBA_FLOAT) {
            if (Core::support.texFloat)
                filter = filter && Core::support.texFloatLinear;
            else
                format = FMT_RGBA;
        }

        this->format = format;

    #ifdef _OS_PSP
        if (data) {
            memory = new uint8[width * height * 4];
        #ifdef TEX_SWIZZLE
            swizzle(memory, (uint8*)data, width * 4, height);
        #else
            memcpy(memory, data, width * height * 4);
        #endif
        } else
            memory = NULL;
    #else
        glGenTextures(1, &ID);
        bind(0);

        GLenum target = (opt & CUBEMAP) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;

        if (format == FMT_SHADOW) {
            glTexParameteri(target, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
            glTexParameteri(target, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
        }

        bool border = isShadow && Core::support.texBorder;
        glTexParameteri(target, GL_TEXTURE_WRAP_S, border ? GL_CLAMP_TO_BORDER : GL_CLAMP_TO_EDGE);
        glTexParameteri(target, GL_TEXTURE_WRAP_T, border ? GL_CLAMP_TO_BORDER : GL_CLAMP_TO_EDGE);
        if (border) {
            float color[] = { 1.0f, 1.0f, 1.0f, 1.0f };
            glTexParameterfv(target, GL_TEXTURE_BORDER_COLOR, color);
        }

        glTexParameteri(target, GL_TEXTURE_MIN_FILTER, filter ? (mipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR ) : ( mipmaps ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST ));
        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, filter ? GL_LINEAR : GL_NEAREST);

        struct FormatDesc {
            GLuint ifmt, fmt;
            GLenum type;
        } formats[FMT_MAX] = {            
            { GL_LUMINANCE,       GL_LUMINANCE,       GL_UNSIGNED_BYTE          }, // LUMINANCE
            { GL_RGBA,            GL_RGBA,            GL_UNSIGNED_BYTE          }, // RGBA
            { GL_RGB,             GL_RGB,             GL_UNSIGNED_SHORT_5_6_5   }, // RGB16
            { GL_RGBA,            GL_RGBA,            GL_UNSIGNED_SHORT_5_5_5_1 }, // RGBA16
            { GL_RGBA32F,         GL_RGBA,            GL_FLOAT                  }, // RGBA_FLOAT
            { GL_RGBA16F,         GL_RGBA,            GL_HALF_FLOAT             }, // RGBA_HALF
            { GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT         }, // DEPTH
            { GL_DEPTH_STENCIL,   GL_DEPTH_STENCIL,   GL_UNSIGNED_INT_24_8      }, // DEPTH_STENCIL
            { GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT         }, // SHADOW
        };

        FormatDesc desc = formats[format];

        #ifdef _OS_WEB // fucking firefox!
            if (format == FMT_RGBA_FLOAT) {
                if (Core::support.texFloat) {
                    desc.ifmt = GL_RGBA;
                    desc.type = GL_FLOAT;
                }
            }

            if (format == FMT_RGBA_HALF) {
                if (Core::support.texHalf) {
                    desc.ifmt = GL_RGBA;
                    desc.type = GL_HALF_FLOAT_OES;
                }
            }
        #else
            if ((format == FMT_RGBA_FLOAT && !Core::support.colorFloat) || (format == FMT_RGBA_HALF && !Core::support.colorHalf)) {
                desc.ifmt = GL_RGBA;
                #ifdef _GAPI_GLES
                    if (format == FMT_RGBA_HALF)
                        desc.type = GL_HALF_FLOAT_OES;
                #endif
            }
        #endif

        for (int i = 0; i < 6; i++) {
            glTexImage2D(cube ? (GL_TEXTURE_CUBE_MAP_POSITIVE_X + i) : GL_TEXTURE_2D, 0, desc.ifmt, width, height, 0, desc.fmt, desc.type, data);
            if (!cube) break;
        }
    #endif

        if (mipmaps)
            generateMipMap();

        if (filter)
            this->opt &= ~NEAREST;
        else
            this->opt |= NEAREST;
    }

    void generateMipMap() {
    #ifdef _OS_PSP
        // TODO
    #else
        bind(0);
        GLenum target = (opt & CUBEMAP) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;

        glGenerateMipmap(target);
        if (!(opt & CUBEMAP) && !(opt & NEAREST) && (Core::support.maxAniso > 0))
            glTexParameteri(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, min(int(Core::support.maxAniso), 8));
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 4);
    #endif
    }

    virtual ~Texture() {
    #ifdef _OS_PSP
        delete[] memory;
    #else
        #ifdef SPLIT_BY_TILE
            if (!ID) {
                for (int i = 0; i < COUNT(tiles); i++)
                    delete[] tiles[i];
                return;
            }
        #endif
        glDeleteTextures(1, &ID);
    #endif
    }

    void setFilterQuality(int value) {
        bool filter  = value > Core::Settings::LOW;
        bool mipmaps = value > Core::Settings::MEDIUM;

    #ifdef _OS_PSP
        if (filter)
            opt &= ~NEAREST;
        else
            opt |= NEAREST;

        if (mipmaps)
            opt |= MIPMAPS;
        else
            opt &= ~MIPMAPS;
    #else
        Core::active.textures[0] = NULL;
        bind(0);
        if (Core::support.maxAniso > 0)
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, value > Core::Settings::MEDIUM ? min(int(Core::support.maxAniso), 8) : 1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter ? (mipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR ) : ( mipmaps ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST ));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter ? GL_LINEAR : GL_NEAREST);
    #endif
    }

    void bind(int sampler) {
    #ifdef _OS_PSP
        if (this && !sampler && memory) {
            int swizzle = GU_FALSE;
            #ifdef TEX_SWIZZLE
                swizzle = GU_TRUE;
            #endif
            sceGuTexMode(GU_PSM_8888, 0, 0, swizzle);
            sceGuTexImage(0, width, height, width, memory);
        }
    #else
        #ifdef SPLIT_BY_TILE
            if (!this || sampler || !ID) return;
        #endif

        if (Core::active.textures[sampler] != this) {
            Core::active.textures[sampler] = this;
            glActiveTexture(GL_TEXTURE0 + sampler);
            glBindTexture((opt & CUBEMAP) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, ID);
        }
    #endif
    }

    void unbind(int sampler) {
    #ifdef _OS_PSP
        //
    #else
        if (Core::active.textures[sampler]) {
            Core::active.textures[sampler] = NULL;
            glActiveTexture(GL_TEXTURE0 + sampler);
            glBindTexture((opt & CUBEMAP) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, 0);
        }
    #endif
    }

    struct Color24 {
        uint8 r, g, b;
    };

    struct Color32 {
        uint8 r, g, b, a;
    };
/*
    static void SaveBMP(const char *name, const char *data32, int width, int height) {
        BITMAPINFOHEADER BMIH;
        BMIH.biSize = sizeof(BITMAPINFOHEADER);
        BMIH.biSizeImage = width * height * 4;

        BMIH.biSize = sizeof(BITMAPINFOHEADER);
        BMIH.biWidth = width;
        BMIH.biHeight = height;
        BMIH.biPlanes = 1;
        BMIH.biBitCount = 32;
        BMIH.biCompression = BI_RGB;
        BMIH.biSizeImage = width * height * 4;

        BITMAPFILEHEADER bmfh;
        int nBitsOffset = sizeof(BITMAPFILEHEADER) + BMIH.biSize;
        LONG lImageSize = BMIH.biSizeImage;
        LONG lFileSize = nBitsOffset + lImageSize;
        bmfh.bfType = 'B' + ('M' << 8);
        bmfh.bfOffBits = nBitsOffset;
        bmfh.bfSize = lFileSize;
        bmfh.bfReserved1 = bmfh.bfReserved2 = 0;

        char buf[256];
        strcpy(buf, name);
        strcat(buf, ".bmp");

        FILE *f = fopen(buf, "wb");
        if (f) {
            fwrite(&bmfh, sizeof(bmfh), 1, f);
            fwrite(&BMIH, sizeof(BMIH), 1, f);
            int res = fwrite(data32, width * height * 4, 1, f);
            LOG("Res %d\n", res);
            fclose(f);
        } else
            ASSERT(false);
    }
*/
    static uint8* LoadPCX(Stream &stream, uint32 &width, uint32 &height) {
        struct PCX {
            uint8  magic;
            uint8  version;
            uint8  compression;
            uint8  bpp;
            uint16 rect[4];
            uint16 width;
            uint16 height;
            uint8  other[48 + 64];
        } pcx;

        stream.raw(&pcx, sizeof(PCX));

        ASSERT(pcx.bpp == 8);
        ASSERT(pcx.compression == 1);

        int i = 0;
        int size = pcx.width * pcx.height;

        uint8 *indices = new uint8[pcx.width * pcx.height];

        while (i < size) {
            uint8 n;
            stream.read(n);
            if ((n & 0xC0) == 0xC0) {
                uint8 count = n & 0x3F;
                stream.read(n);
                memset(&indices[i], n, count);
                i += count;
            } else
                indices[i++] = n;
        }

        uint8 flag;
        stream.read(flag);
        ASSERT(flag == 0x0C);

        Color24 palette[256];
        stream.raw(palette, sizeof(palette));

        Color32 *data = new Color32[pcx.width * pcx.height];
        Color32 *dst = data;
        uint8   *src = indices;

        for (int y = 0; y < pcx.height; y++)
            for (int x = 0; x < pcx.width; x++) {
                Color24 &c = palette[*src++];
                dst->r = c.r;
                dst->g = c.g;
                dst->b = c.b;
                dst->a = 255;
                dst++;
            }
        delete[] indices;

        width  = pcx.width;
        height = pcx.height;
        return (uint8*)data;
    }

    static uint8* LoadBMP(Stream &stream, uint32 &width, uint32 &height) {
        int32 offset;
        stream.seek(10);
        stream.read(offset);
        stream.seek(4);
        stream.read(width);
        stream.read(height);
        stream.seek(offset - stream.pos);

        Color24 *data24 = new Color24[width * height];
        Color32 *data32 = new Color32[width * height];
        stream.raw(data24, width * height * sizeof(Color24));

        Color32 *dst = data32;
        for (uint32 y = 0; y < height; y++) {
            Color24 *src = data24 + (height - y - 1) * width;
            for (uint32 x = 0; x < width; x++) {
                dst->r = src->b;
                dst->g = src->g;
                dst->b = src->r;
                dst->a = 255;
                src++;
                dst++;
            }
        }
        delete[] data24;

        return (uint8*)data32;
    }

#ifdef USE_INFLATE
    static int pngPaeth(int a, int b, int c) {
        int p = a + b - c;
        int pa = abs(p - a);
        int pb = abs(p - b);
        int pc = abs(p - c);
        if (pa <= pb && pa <= pc)
            return a;
        if (pb <= pc)
            return b;
        return c;
    }

    static void pngFilter(int id, int BPP, int BPL, const uint8 *src, uint8 *dst, const uint8 *prev) {
        enum { NONE, SUB, UP, AVRG, PAETH };

        switch (id) {
            case NONE : 
                memcpy(dst, src, BPL); 
                break;
            case SUB  : 
                memcpy(dst, src, BPP);
                for (int i = BPP; i < BPL; i++)
                    dst[i] = src[i] + dst[i - BPP];
                break;
            case UP :
                for (int i = 0; i < BPL; i++)
                    dst[i] = src[i] + prev[i];
                break;
            case AVRG :
                for (int i = 0; i < BPP; i++)
                    dst[i] = src[i] + (prev[i] >> 1);
                for (int i = BPP; i < BPL; i++)
                    dst[i] = src[i] + ((dst[i - BPP] + prev[i]) >> 1);
                break;
            case PAETH :
                for (int i = 0; i < BPP; i++)
                    dst[i] = src[i] + pngPaeth(0, prev[i], 0);
                for (int i = BPP; i < BPL; i++)
                    dst[i] = src[i] + pngPaeth(dst[i - BPP], prev[i], prev[i - BPP]);
                break;
        };
    }

    static uint8* LoadPNG(Stream &stream, uint32 &width, uint32 &height) {
        stream.seek(8);

        uint8 bits, colorType, interlace;
        int BPP, BPL;

        uint8 palette[256 * 3];
        uint8 trans[256];

        uint8 *cdata = NULL;
        int offset = 0;

    // read chunks
        while (stream.pos < stream.size) {
            uint32 chunkSize, chunkName;
            chunkSize = swap32(stream.read(chunkSize));
            stream.read(chunkName);
            if (chunkName == FOURCC("IHDR")) { // Image Header
                width  = swap32(stream.read(width));
                height = swap32(stream.read(height));
                stream.read(bits);
                stream.read(colorType);
                stream.seek(2);
                stream.read(interlace);

                ASSERT(interlace == 0);
                ASSERT(bits <= 8);

                int components;
                switch (colorType) {
                    case 2  : components = 3; break;
                    case 4  : components = 2; break;
                    case 6  : components = 4; break;
                    default : components = 1;
                }

                BPP = (bits + 7) / 8 * components;
                BPL = (width * bits + 7) / 8 * components;

                cdata = new uint8[stream.size];
                memset(trans, 0xFF, sizeof(trans));
            } else if (chunkName == FOURCC("PLTE")) { // Palette
                stream.raw(palette, chunkSize);
            } else if (chunkName == FOURCC("tRNS")) { // Transparency info
                stream.raw(trans, chunkSize);
            } else if (chunkName == FOURCC("IDAT")) { // Compressed image data part
                ASSERT(cdata);
                stream.raw(cdata + offset, chunkSize);
                offset += chunkSize;
            } else if (chunkName == FOURCC("IEND")) {
                break;
            } else
                stream.seek(chunkSize);
            stream.seek(4); // skip chunk CRC
        }

        ASSERT(cdata);

        uint8 *buffer = new uint8[(BPL + 1) * height];

        uint32 cbytes;
        tinf_uncompress(buffer, &cbytes, cdata + 2, 0);
        delete[] cdata;

        ASSERT(cbytes > 0);

    // apply line filters to image
        uint8 *data = new uint8[BPL * height];

        uint8 *prev = new uint8[BPL];
        memset(prev, 0, BPL);
        for (uint32 i = 0; i < height; i++) {
            pngFilter(buffer[(BPL + 1) * i], BPP, BPL, &buffer[(BPL + 1) * i + 1], &data[BPL * i], prev);
            if (i == 0)
                delete[] prev;
            prev = &data[BPL * i];
        }
        delete[] buffer;


    // convert to 32-bit image
        uint8 *data32 = new uint8[width * height * 4];

        uint8 *src = data;
        uint8 *dst = data32;
        uint8 *end = data32 + width * height * 4;

        switch (colorType) {
            case 0 : // grayscale
                while (dst < end) {
                    dst[0] =
                    dst[1] =
                    dst[2] = src[0];
                    dst[3] = trans[0];
                    dst += 4;
                    src++;
                }
                break;
            case 2 : // RGB
                while (dst < end) {
                    dst[0] = src[0];
                    dst[1] = src[1];
                    dst[2] = src[2];
                    dst[3] = trans[0];
                    dst += 4;
                    src += 3;
                }
                break;
            case 3 : // indexed color with alpha
                for (uint32 j = 0, palWord = 0; j < height; j++) {
                    for (uint32 i = 0, curBit = 8; i < width; i++) {
                        if (curBit > 7) {
                            curBit -= 8;
                            palWord = *src++;
                            if (i < width - 1)
                                palWord |= *src << 8;
                        }
                        uint16 palIdx = (palWord >> (8 - bits - curBit)) & ~(0xFFFF << bits);
                        curBit += bits;

                        dst[0] = palette[palIdx * 3 + 0];
                        dst[1] = palette[palIdx * 3 + 1];
                        dst[2] = palette[palIdx * 3 + 2];
                        dst[3] = trans[palIdx];
                        dst += 4;
                    }
                }
                break;
            case 4 : // grayscale with alpha
                while (dst < end) {
                    dst[0] =
                    dst[1] =
                    dst[2] = src[0];
                    dst[3] = src[1];
                    dst += 4;
                    src += 2;
                }
                break;
            case 6 : // RGBA
                memcpy(dst, src, width * height * 4);
                break;
        }
        delete[] data;

        return data32;
    }
#endif

    static void rncGetOffset(BitStream &bs, uint16 &offset) {
        offset = 0;
        if (bs.readBit()) {
            offset = bs.readBit();

            if (bs.readBit()) {
                offset = ((offset << 1) | bs.readBit()) | 4;

                if (!bs.readBit())
                    offset = (offset << 1) | bs.readBit();
            } else if (!offset)
                offset = bs.readBit() + 2;
        }
        offset = ((offset << 8) | bs.readByte()) + 1;
    }

    static uint8* LoadRNC(Stream &stream, uint32 &width, uint32 &height) { // https://github.com/lab313ru/rnc_propack_source
        uint32 magic, size, csize;
        stream.read(magic);

        if (magic == FOURCC("RNC\002")) {
            size  = swap32(stream.read(size));
            csize = swap32(stream.read(csize));
            stream.seek(6);
        } else {
            stream.seek(-4);
            size  = 384 * 256 * 2;
            csize = stream.size;
        }

        uint8 *data  = new uint8[size];
        uint8 *cdata = new uint8[csize];

        stream.raw(cdata, csize);

        BitStream bs(cdata, csize);
        uint8 *dst = data;
        uint8 *end = data + size;

        uint32 length = 0;
        uint16 offset = 0;

        bs.readBits(2);
        while (bs.data < bs.end && dst < end) {
            if (!bs.readBit()) {
                *dst++ = bs.readByte();
            } else {
                if (bs.readBit()) {
                    if (bs.readBit()) {
                        if (bs.readBit()) {
                            length = bs.readByte() + 8;
                            if (length == 8) {
                                bs.readBit();
                                continue;
                            }
                        } else
                            length = 3;

                        rncGetOffset(bs, offset);
                    } else {
                        length = 2;
                        offset = bs.readByte() + 1;
                    }

                    while (length--) {
                        *dst = dst[-offset];
                        dst++;
                    }
                } else {
                    length = bs.readBit() + 4;
                    if (bs.readBit())
                        length = ((length - 1) << 1) + bs.readBit();

                    if (length != 9) {
                        rncGetOffset(bs, offset);

                        while (length--) {
                            *dst = dst[-offset];
                            dst++;
                        }
                    } else {
                        length = (bs.readBits(4) << 2) + 12;
                        while (length--)
                            *dst++ = bs.readByte();
                    }
                }
            }
        }
        delete[] cdata;

        width  = 384;
        height = size / width / 2;

        uint32 *data32 = new uint32[width * height];
        {
            uint32 *dst = data32;
            uint16 *src = (uint16*)data;
            uint16 *end = src + width * height;

            while (src < end) {
                uint16 c = *src++;
                *dst++ = ((c & 0x001F) << 3) | ((c & 0x03E0) << 6) | (((c & 0x7C00) << 9)) | 0xFF000000;
            }
        }
        
        delete[] data;

        return (uint8*)data32;
    }

    static uint8* LoadBIN(Stream &stream, uint32 &width, uint32 &height) {
        height = 224;
        width  = stream.size / height / 2;

        uint8 *data = new uint8[stream.size];
        stream.raw(data, stream.size);

        uint32 *data32 = new uint32[width * height];
        uint32 *dst = data32;
        uint16 *src = (uint16*)data;
        uint16 *end = src + width * height;

        while (src < end) {
            uint16 c = swap16(*src++);
            *dst++ = ((c & 0x001F) << 3) | ((c & 0x03E0) << 6) | (((c & 0x7C00) << 9)) | 0xFF000000;
        }

        delete[] data;

        return (uint8*)data32;
    }

    static uint8* LoadDATA(Stream &stream, uint32 &width, uint32 &height) {
        uint32 magic;
        stream.read(magic);
        stream.seek(-4);

        #ifdef USE_INFLATE
            if (magic == 0x474E5089)
                return LoadPNG(stream, width, height);
        #endif

        if (stream.name && strstr(stream.name, ".RAW"))
            return LoadRNC(stream, width, height);

        if (stream.name && strstr(stream.name, ".BIN"))
            return LoadBIN(stream, width, height);

        if ((magic & 0xFFFF) == 0x4D42)
            return LoadBMP(stream, width, height);

        return LoadPCX(stream, width, height);
    }


    static Texture* Load(Stream &stream, bool border = true) {
        uint32 width, height;
        uint8 *data = LoadDATA(stream, width, height);

    // convert to POT size if NPOT isn't supported
        uint32 dw = Core::support.texNPOT ? width  : nextPow2(width);
        uint32 dh = Core::support.texNPOT ? height : nextPow2(height);
        if (dw != width || dh != height) {
            uint32 *dataPOT = new uint32[dw * dh];
            uint32 *dst = (uint32*)dataPOT;
            uint32 *src = (uint32*)data;

            for (uint32 j = 0; j < dh; j++)
                for (uint32 i = 0; i < dw; i++)
                    *dst++ = (i < width && j < height) ? *src++ : 0xFF000000;

            delete[] data;
            data = (uint8*)dataPOT;
        }

        if (border) {
            for (uint32 y = 0; y < height; y++)
                ((uint32*)data)[y * dw] = ((uint32*)data)[y * dw + dw - 1] = 0xFF000000;
        }

        Texture *tex = new Texture(dw, dh, FMT_RGBA, 0, data);
        tex->origWidth  = width;
        tex->origHeight = height;

        delete[] data;

        return tex;
    }
};

#define ATLAS_BORDER 8

struct Atlas {
    typedef void (Callback)(int id, int width, int height, int x, int y, void *userData, void *data);

    struct Node {
        Node   *child[2];
        short4 rect;
        int    id;

        Node(short l, short t, short r, short b) : rect(l, t, r, b), id(-1) { 
            child[0] = child[1] = NULL; 
        }

        ~Node() {
            delete child[0];
            delete child[1];
        }

        Node* insert(const short4 &tile, int id) {
            ASSERT(tile.x != 0x7FFF);

            if (child[0] != NULL && child[1] != NULL) {
                Node *node = child[0]->insert(tile, id);
                if (node != NULL) return node;
                return child[1]->insert(tile, id);
            } else {
                if (this->id != -1)
                    return NULL;

                int16 w  = rect.z - rect.x;
                int16 h  = rect.w - rect.y;
                int16 tx = (tile.z - tile.x) + ATLAS_BORDER * 2;
                int16 ty = (tile.w - tile.y) + ATLAS_BORDER * 2;

                if (w < tx || h < ty)
                    return NULL;

                if (w == tx && h == ty) {
                    this->id = id;
                    return this;
                }

                int16 dx = w - tx;
                int16 dy = h - ty;

                if (dx > dy) {
                    child[0] = new Node(rect.x, rect.y, rect.x + tx, rect.w);
                    child[1] = new Node(rect.x + tx, rect.y, rect.z, rect.w);
                } else {
                    child[0] = new Node(rect.x, rect.y, rect.z, rect.y + ty);
                    child[1] = new Node(rect.x, rect.y + ty, rect.z, rect.w);
                }

                return child[0]->insert(tile, id);
            }
        }
    } *root;

    struct Tile {
        int    id;
        short4 uv;
    } *tiles;

    int      tilesCount;
    int      size;
    int      width, height;
    void     *userData;
    Callback *callback;

    Atlas(int maxTiles, void *userData, Callback *callback) : root(NULL), tilesCount(0), size(0), userData(userData), callback(callback) {
        tiles = new Tile[maxTiles];
    }

    ~Atlas() {
        delete root;
        delete[] tiles;
    }

    void add(short4 uv, int id) {
        for (int i = 0; i < tilesCount; i++)
            if (tiles[i].uv == uv) {
                uv.x = 0x7FFF;
                uv.y = tiles[i].id;
                uv.z = uv.w = 0;
                break;
            }
        
        tiles[tilesCount].id = id;
        tiles[tilesCount].uv = uv;
        tilesCount++;

        if (uv.x != 0x7FFF)
            size += (uv.z - uv.x + ATLAS_BORDER * 2) * (uv.w - uv.y + ATLAS_BORDER * 2);
    }

    bool insertAll(int *indices) {
        for (int i = 0; i < tilesCount; i++) {
            int idx = indices[i];
            if (tiles[idx].uv.x != 0x7FFF && !root->insert(tiles[idx].uv, tiles[idx].id))
                return false;
        }
        return true;
    }

    Texture* pack() {
    // TODO TR2 fix CUT2 AV
//        width  = 4096;//nextPow2(int(sqrtf(float(size))));
//        height = 2048;//(width * width / 2 > size) ? (width / 2) : width;
        width  = nextPow2(int(sqrtf(float(size))));
        height = (width * width / 2 > size) ? (width / 2) : width;
    // sort
        int *indices = new int[tilesCount];
        for (int i = 0; i < tilesCount; i++)
            indices[i] = i;

        int n = tilesCount;
        bool swapped;
        do {
            swapped = false;
            for (int i = 1; i < n; i++) {
                short4 &a = tiles[indices[i - 1]].uv;
                short4 &b = tiles[indices[i]].uv;
                //if ((a.z - a.x + ATLAS_BORDER * 2) * (a.w - a.y + ATLAS_BORDER * 2) < (b.z - b.x + ATLAS_BORDER * 2) * (b.w - b.y + ATLAS_BORDER * 2)) {
                if ((a.z - a.x + ATLAS_BORDER * 2) < (b.z - b.x + ATLAS_BORDER * 2)) {
                    swap(indices[i - 1], indices[i]);
                    swapped = true;
                }
            }
            n--;
        } while (swapped);
    // pack
        while (1) {
            delete root;
            root = new Node(0, 0, width, height);

            if (insertAll(indices)) 
                break;

            if (width < height)
                width  *= 2;
            else
                height *= 2;
        }

        delete[] indices;

        uint32 *data = new uint32[width * height];
        memset(data, 0, width * height * sizeof(data[0]));
        fill(root, data);
        fillInstances();

        Texture *atlas = new Texture(width, height, FMT_RGBA, Texture::MIPMAPS, data);

        //Texture::SaveBMP("atlas.bmp", (char*)data, width, height);

        delete[] data;
        return atlas;
    };

    void fill(Node *node, void *data) {
        if (!node) return;

        if (node->id == -1) {
            fill(node->child[0], data);
            fill(node->child[1], data);
        } else
            callback(node->id, width, height, node->rect.x, node->rect.y, userData, data);
    }

    void fillInstances() {
        for (int i = 0; i < tilesCount; i++)
            if (tiles[i].uv.x == 0x7FFF)
                callback(tiles[i].id, width, height, tiles[i].uv.y, 0, userData, NULL);
    }
};

#endif