#ifndef H_TEXTURE
#define H_TEXTURE

#include "core.h"

struct Texture {
    enum Format : uint32 { LUMINANCE, RGBA, RGB16, RGBA16, RGBA_FLOAT, RGBA_HALF, DEPTH, DEPTH_STENCIL, SHADOW, MAX };

    GLuint  ID;
    int     width, height, origWidth, origHeight;
    Format  format;
    bool    cube;
    bool    filter;

    Texture(int width, int height, Format format, bool cube = false, void *data = NULL, bool filter = true, bool mips = false) : cube(cube), filter(filter) {
        if (!Core::support.texNPOT) {
            width  = nextPow2(width);
            height = nextPow2(height);
        }
        this->width  = origWidth  = width;
        this->height = origHeight = height;

        glGenTextures(1, &ID);
        bind(0);

        GLenum target = cube ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
        bool   isShadow = format == SHADOW;

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
                filter = filter && Core::support.texHalfLinear;
            else
                format = RGBA_FLOAT;
        }

        if (format == RGBA_FLOAT) {
            if (Core::support.texFloat)
                filter = filter && Core::support.texFloatLinear;
            else
                format = RGBA;
        }

        this->format = format;

        if (format == SHADOW) {
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

        glTexParameteri(target, GL_TEXTURE_MIN_FILTER, filter ? (mips ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR ) : ( mips ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST ));
        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, filter ? GL_LINEAR : GL_NEAREST);

        struct FormatDesc {
            GLuint ifmt, fmt;
            GLenum type;
        } formats[MAX] = {            
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

#ifdef __EMSCRIPTEN__ // fucking firefox!
        if (format == RGBA_FLOAT) {
            if (Core::support.texFloat) {
                desc.ifmt = GL_RGBA;
                desc.type = GL_FLOAT;
            }
        }

        if (format == RGBA_HALF) {
            if (Core::support.texHalf) {
                desc.ifmt = GL_RGBA;
                #ifdef MOBILE
                    desc.type = GL_HALF_FLOAT_OES;
                #endif
            }
        }
#else
        if ((format == RGBA_FLOAT && !Core::support.colorFloat) || (format == RGBA_HALF && !Core::support.colorHalf)) {
            desc.ifmt = GL_RGBA;
            #ifdef MOBILE
                if (format == RGBA_HALF)
                    desc.type = GL_HALF_FLOAT_OES;
            #endif
        }
#endif

        for (int i = 0; i < 6; i++) {
            glTexImage2D(cube ? (GL_TEXTURE_CUBE_MAP_POSITIVE_X + i) : GL_TEXTURE_2D, 0, desc.ifmt, width, height, 0, desc.fmt, desc.type, data);
            if (!cube) break;
        }

        if (mips)
            generateMipMap();

        this->filter = filter;
    }

    void generateMipMap() {
        bind(0);
        GLenum target = cube ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;

        glGenerateMipmap(target);
        if (!cube && filter && (Core::support.maxAniso > 0))
            glTexParameteri(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, min(int(Core::support.maxAniso), 8));
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 4);
    }

    virtual ~Texture() {
        glDeleteTextures(1, &ID);
    }

    void setFilterQuality(Core::Settings::Quality value) {
        bool filter = value > Core::Settings::LOW;
        bool mips   = value > Core::Settings::MEDIUM;

        Core::active.textures[0] = NULL;
        bind(0);
        if (Core::support.maxAniso > 0)
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, value > Core::Settings::MEDIUM ? min(int(Core::support.maxAniso), 8) : 1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter ? (mips ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR ) : ( mips ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST ));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter ? GL_LINEAR : GL_NEAREST);
    }

    void bind(int sampler) {
        if (Core::active.textures[sampler] != this) {
            Core::active.textures[sampler] = this;
            glActiveTexture(GL_TEXTURE0 + sampler);
            glBindTexture(cube ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, ID);
        }
    }

    void unbind(int sampler) {
        if (Core::active.textures[sampler]) {
            Core::active.textures[sampler] = NULL;
            glActiveTexture(GL_TEXTURE0 + sampler);
            glBindTexture(cube ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, 0);
        }
    }

    struct Color24 {
        uint8 r, g, b;
    };

    struct Color32 {
        uint8 r, g, b, a;
    };

    static Texture* LoadPCX(Stream &stream) {
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
        int dw = Core::support.texNPOT ? pcx.width  : nextPow2(pcx.width);
        int dh = Core::support.texNPOT ? pcx.height : nextPow2(pcx.height);

        uint8 *buffer = new uint8[size + 256 * 3 + dw * dh * 4];

        while (i < size) {
            uint8 n;
            stream.read(n);
            if ((n & 0xC0) == 0xC0) {
                uint8 count = n & 0x3F;
                stream.read(n);
                memset(&buffer[i], n, count);
                i += count;
            } else
                buffer[i++] = n;
        }

        uint8 flag;
        stream.read(flag);
        ASSERT(flag == 0x0C);

        Color24 *palette = (Color24*)&buffer[size];
        stream.raw(palette, 256 * 3);

        Color32 *data = (Color32*)&palette[256];
        memset(data, 0, dw * dh * 4);
        
        // TODO: color bleeding

        Color32 *ptr = data;
        i = 0;
        for (int y = 0; y < pcx.height; y++) {
            for (int x = 0; x < pcx.width; x++) {
                Color24 &c = palette[buffer[i++]];
                ptr[x].r = c.r;
                ptr[x].g = c.g;
                ptr[x].b = c.b;
                ptr[x].a = 255;
            }
            ptr += dw;
        }

        Texture *tex = new Texture(dw, dh, Texture::RGBA, false, data);
        tex->origWidth  = pcx.width;
        tex->origHeight = pcx.height;
        delete[] buffer;

        return tex;
    }

    static Texture* LoadBMP(Stream &stream) {
        int32 offset, width, height;
        stream.seek(10);
        stream.read(offset);
        stream.seek(4);
        stream.read(width);
        stream.read(height);
        stream.seek(offset - stream.pos);

        int dw = Core::support.texNPOT ? width  : nextPow2(width);
        int dh = Core::support.texNPOT ? height : nextPow2(height);
        Color24 *data24 = new Color24[width * height];
        Color32 *data32 = new Color32[dw * dh];
        stream.raw(data24, width * height * sizeof(Color24));

        Color32 *dst = data32;
        for (int y = 0; y < dh; y++) {
            Color24 *src = data24 + (height - y - 1) * width;
            for (int x = 0; x < dw; x++) {
                
                if (x < width && y < height) {
                    dst->r = src->b;
                    dst->g = src->g;
                    dst->b = src->r;
                    src++;
                } else
                    dst->r = dst->g = dst->b = 0;

                dst->a = 255;
                dst++;
            }
        }

        Texture *tex = new Texture(dw, dh, Texture::RGBA, false, data32);
        tex->origWidth  = width;
        tex->origHeight = height;

        delete[] data24;
        delete[] data32;
        return tex;
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


    static Texture* LoadPNG(Stream &stream) {
        stream.seek(8);

        uint8 bits, colorType, interlace;
        int BPP, BPL;
        uint32 width, height;

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

        switch (colorType) {
            case 0 : // grayscale
                for (uint32 i = 0; i < width * height; i += 4, dst += 4, src++) {
                    dst[0] =
                    dst[1] =
                    dst[2] = src[0];
                    dst[3] = trans[0];
                }
                break;
            case 2 : // RGB
                for (uint32 i = 0; i < width * height; i++, dst += 4, src += 3) {
                    dst[0] = src[0];
                    dst[1] = src[1];
                    dst[2] = src[2];
                    dst[3] = trans[0];
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
                for (uint32 i = 0; i < width * height; i++, dst += 4, src += 2) {
                    dst[0] =
                    dst[1] =
                    dst[2] = src[0];
                    dst[3] = src[1];
                }
                break;
            case 6 : // RGBA
                memcpy(dst, src, width * height * 4);
                break;
        }

    // convert to POT size if NPOT isn't supported
        uint32 dw = Core::support.texNPOT ? width  : nextPow2(width);
        uint32 dh = Core::support.texNPOT ? height : nextPow2(height);
        if (dw != width || dh != height) {
            uint8  *dataPOT = new uint8[dw * dh * 4];
            uint32 *dst = (uint32*)dataPOT;
            uint32 *src = (uint32*)data32;

            for (uint32 j = 0; j < dh; j++)
                for (uint32 i = 0; i < dw; i++)
                    *dst++ = (i < width && j < height) ? *src++ : 0xFF000000;

            delete[] data32;
            data32 = dataPOT;
        }

        Texture *tex = new Texture(dw, dh, Texture::RGBA, false, data32);
        tex->origWidth  = width;
        tex->origHeight = height;

        delete[] data32;
        delete[] data;

        return tex;
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

    static Texture* LoadRNC(Stream &stream) { // https://github.com/lab313ru/rnc_propack_source
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

        uint32 length = 0;
        uint16 offset = 0;

        bs.readBits(2);
        while (bs.data < bs.end || bs.index > 0) {
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

                    while (length--)
                        *dst++ = dst[-offset];
                } else {
                    length = bs.readBit() + 4;
                    if (bs.readBit())
                        length = ((length - 1) << 1) + bs.readBit();

                    if (length != 9) {
                        rncGetOffset(bs, offset);

                        while (length--)
                            *dst++ = dst[-offset];
                    } else {
                        length = (bs.readBits(4) << 2) + 12;
                        while (length--)
                            *dst++ = bs.readByte();
                    }
                }
            }
        }
        delete[] cdata;

        int width  = 384;
        int height = size / width / 2;
        int dw = Core::support.texNPOT ? width  : nextPow2(width);
        int dh = Core::support.texNPOT ? height : nextPow2(height);

        uint32 *data32 = new uint32[dw * dh];
        {
            uint32 *dst = data32;
            uint16 *src = (uint16*)data;

            for (int j = 0; j < dh; j++)
                for (int i = 0; i < dw; i++) {
                    if (i < width && j < height) {
                        uint16 c = *src++;
                        *dst++ = ((c & 0x001F) << 3) | ((c & 0x03E0) << 6) | (((c & 0x7C00) << 9)) | 0xFF000000;
                    } else 
                        *dst++ = 0xFF000000;
                }

            delete[] data;
        }
        Texture *tex = new Texture(dw, dh, Texture::RGBA, false, data32);
        tex->origWidth  = width;
        tex->origHeight = height;

        delete[] data32;

        return tex;
    }

    static Texture* Load(Stream &stream) {
        uint32 magic;
        stream.read(magic);
        stream.seek(-4);

        #ifdef USE_INFLATE
            if (magic == 0x474E5089)
                return LoadPNG(stream);
        #endif

        if (stream.name && strstr(stream.name, ".RAW"))
            return LoadRNC(stream);

        if ((magic & 0xFFFF) == 0x4D42)
            return LoadBMP(stream);
        return LoadPCX(stream);
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
        width  = 4096;//nextPow2(int(sqrtf(float(size))));
        height = 2048;//(width * width / 2 > size) ? (width / 2) : width;
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

        Texture *atlas = new Texture(width, height, Texture::RGBA, false, data, true, true);
        
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
            if (tiles[i].uv.x == 0x7FFF) {
                callback(tiles[i].id, width, height, tiles[i].uv.y, 0, userData, NULL);
                /*
                TR::ObjectTexture &r = level.objectTextures[ref];
                int minXr = min(min(r.texCoord[0].x, r.texCoord[1].x), r.texCoord[2].x);
                int minYr = min(min(r.texCoord[0].y, r.texCoord[1].y), r.texCoord[2].y);

                TR::ObjectTexture &t = level.objectTextures[tiles[i].id];
                int minX = min(min(t.texCoord[0].x, t.texCoord[1].x), t.texCoord[2].x);
                int maxX = max(max(t.texCoord[0].x, t.texCoord[1].x), t.texCoord[2].x);
                int minY = min(min(t.texCoord[0].y, t.texCoord[1].y), t.texCoord[2].y);
                int maxY = max(max(t.texCoord[0].y, t.texCoord[1].y), t.texCoord[2].y);

                int cx = minXr - minX;
                int cy = minYr - minY;

                for (int i = 0; i < 4; i++) {
                    if (t.texCoord[i].x == maxX) t.texCoord[i].x++;
                    if (t.texCoord[i].y == maxY) t.texCoord[i].y++;
                    t.texCoord[i].x += cx;
                    t.texCoord[i].y += cy;
                }
                */
            }
    }
};

#endif