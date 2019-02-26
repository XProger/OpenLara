 #ifndef H_TEXTURE
#define H_TEXTURE

#include "core.h"
#include "format.h"

struct Texture : GAPI::Texture {

    #ifdef SPLIT_BY_TILE

        #ifdef _OS_PSP
            TR::Tile4 *tiles;
            TR::CLUT  *cluts;

            Texture(TR::Tile4 *tiles, int tilesCount, TR::CLUT *cluts, int clutsCount) : GAPI::Texture(256, 256, OPT_PROXY) {
                #ifdef EDRAM_TEX
                    this->tiles = (TR::Tile4*)GAPI::allocEDRAM(tilesCount * sizeof(tiles[0]));
                    this->cluts =  (TR::CLUT*)GAPI::allocEDRAM(clutsCount * sizeof(cluts[0]));
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
            Texture *tiles[32];

            struct Tile {
                uint32 width;
                uint32 height;
                uint32 *data;
            };

            Texture(Tile *tiles, int tilesCount) : GAPI::Texture(256, 256, OPT_PROXY) {
                memset(this->tiles, 0, sizeof(this->tiles));

                ASSERT(tilesCount < COUNT(this->tiles));
                for (int i = 0; i < tilesCount; i++)
                    this->tiles[i] = new Texture(tiles[i].width, tiles[i].height, 1, FMT_RGBA, OPT_MIPMAPS, tiles[i].data);
            }
        #endif

        void bindTile(uint16 tile, uint16 clut) {
        #ifdef _OS_PSP
            bindTileCLUT(tiles + tile, cluts + clut);
        #else
            tiles[tile]->bind(0);
        #endif
        }
    #endif

#if defined(_DEBUG) && defined(_OS_WIN) && defined(_GAPI_GL)
    void dump(const char *fileName) {
        bind(0);
        int size = width *height * 4;
        char *data = new char[size];
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        Texture::SaveBMP(fileName, data, width, height);
        delete[] data;
    }
#endif

    Texture(int width, int height, int depth, TexFormat format, uint32 opt = 0, void *data = NULL) : GAPI::Texture(width, height, depth, opt) {
        #ifdef SPLIT_BY_TILE
            #ifndef _OS_PSP
                memset(this->tiles, 0, sizeof(tiles));
            #endif
        #endif

        if (!Core::support.texNPOT) {
            this->width  = nextPow2(width);
            this->height = nextPow2(height);
        }

        bool filter   = (opt & OPT_NEAREST) == 0;
        bool mipmaps  = (opt & OPT_MIPMAPS) != 0;
        bool isVolume = (opt & OPT_VOLUME)  != 0;

        if (format == FMT_SHADOW && !Core::support.shadowSampler) {
            format = FMT_DEPTH;
        }

        if (format == FMT_DEPTH) {
            if (!Core::support.depthTexture)
                format = FMT_RGBA;
            filter = false;
        }

        if (format == FMT_RG_HALF) {
            if (Core::support.texHalf)
                filter = filter && Core::support.texHalfLinear;
            else
                format = FMT_RG_FLOAT;
        }

        if (format == FMT_RG_FLOAT) {
            if (Core::support.texFloat)
                filter = filter && Core::support.texFloatLinear;
            else
                format = FMT_RGBA;
        }

        this->fmt = format;
        if (filter)
            this->opt &= ~OPT_NEAREST;
        else
            this->opt |= OPT_NEAREST;

        if (isVolume && !Core::support.tex3D) {
            this->opt &= ~OPT_VOLUME;
        }

        init(data);

        if (mipmaps)
            generateMipMap();
    }

    virtual ~Texture() {
        #ifndef _OS_PSP
            #ifdef SPLIT_BY_TILE
                for (int i = 0; i < COUNT(tiles); i++)
                    delete tiles[i];
            #endif
        #endif
        deinit();
    }

    struct Color24 {
        uint8 r, g, b;
    };

    struct Color32 {
        uint8 r, g, b, a;
    };

    static void SaveBMP(const char *name, const char *data32, int width, int height) {
        struct BITMAPFILEHEADER {
            uint32  bfSize;
            uint16  bfReserved1;
            uint16  bfReserved2;
            uint32  bfOffBits;
        } fhdr;

        struct BITMAPINFOHEADER{
            uint32 biSize;
            uint32 biWidth;
            uint32 biHeight;
            uint16 biPlanes;
            uint16 biBitCount;
            uint32 biCompression;
            uint32 biSizeImage;
            uint32 biXPelsPerMeter;
            uint32 biYPelsPerMeter;
            uint32 biClrUsed;
            uint32 biClrImportant;
        } ihdr;

        memset(&fhdr, 0, sizeof(fhdr));
        memset(&ihdr, 0, sizeof(ihdr));

        ihdr.biSize      = sizeof(ihdr);
        ihdr.biWidth     = width;
        ihdr.biHeight    = height;
        ihdr.biPlanes    = 1;
        ihdr.biBitCount  = 32;
        ihdr.biSizeImage = width * height * 4;

        fhdr.bfOffBits   = 2 + sizeof(fhdr) + ihdr.biSize;
        fhdr.bfSize      = fhdr.bfOffBits + ihdr.biSizeImage;

        Color32 *data = new Color32[width * height];

        Color32 *dst = data;
        Color32 *src = (Color32*)data32 + width * (height - 1);
        for (int j = 0; j < height; j++) {
            for (int i = 0; i < width; i++) {
                dst->r = src->b;
                dst->g = src->g;
                dst->b = src->r;
                dst->a = src->a;
                dst++;
                src++;
            }
            src -= width * 2;
        }

        char buf[256];
        strcpy(buf, name);
        strcat(buf, ".bmp");

        FILE *f = fopen(buf, "wb");
        if (f) {
            uint16 type = 'B' + ('M' << 8);
            fwrite(&type, sizeof(type), 1, f);
            fwrite(&fhdr, sizeof(fhdr), 1, f);
            fwrite(&ihdr, sizeof(ihdr), 1, f);
            fwrite(data, ihdr.biSizeImage, 1, f);
            LOG("save %s\n", buf);
            fclose(f);
        } else
            ASSERT(false);

        delete[] data;
    }

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
        int32  offset, size;
        uint16 bpp;
        stream.seek(10);
        stream.read(offset);
        stream.seek(4);
        stream.read(width);
        stream.read(height);
        stream.seek(2);
        stream.read(bpp);
        stream.seek(4);
        stream.read(size);
        stream.seek(offset - stream.pos);

        uint8 *data = new uint8[size];
        stream.raw(data, size);

        uint8 *data32 = new uint8[width * height * 8];
        Color32 *dst = (Color32*)data32;

        switch (bpp) {
            case 1  : { // monochrome (alpha)
                for (uint32 y = 0; y < height; y++) {
                    uint8 *src = data + (height - y - 1) * (width / 8);
                    for (uint32 x = 0; x < width / 8; x++) {
                        for (int i = 7; i >= 0; i--) {
                            dst->r = dst->g = dst->b = 255;
                            dst->a = (*src & (1 << i)) != 0 ? 255 : 0;
                            dst++;
                        }
                        src++;
                    }
                }
                break;
            }
            case 24 : { // true color
                for (uint32 y = 0; y < height; y++) {
                    Color24 *src = (Color24*)data + (height - y - 1) * width;
                    for (uint32 x = 0; x < width; x++) {
                        dst->r = src->b;
                        dst->g = src->g;
                        dst->b = src->r;
                        dst->a = 255;
                        src++;
                        dst++;
                    }
                }
                break;
            }
            default : ASSERT(false);
        }
        delete[] data;

        return data32;
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
        int BPP = 0, BPL = 0;

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
        if (bs.readBitBE()) {
            offset = bs.readBitBE();

            if (bs.readBitBE()) {
                offset = ((offset << 1) | bs.readBitBE()) | 4;

                if (!bs.readBitBE())
                    offset = (offset << 1) | bs.readBitBE();
            } else if (!offset)
                offset = bs.readBitBE() + 2;
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
            size  = 512 * 256 * 2;
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

        bs.readBE(2);
        while (bs.data < bs.end && dst < end) {
            if (!bs.readBitBE()) {
                *dst++ = bs.readByte();
            } else {
                if (bs.readBitBE()) {
                    if (bs.readBitBE()) {
                        if (bs.readBitBE()) {
                            length = bs.readByte() + 8;
                            if (length == 8) {
                                bs.readBitBE();
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
                    length = bs.readBitBE() + 4;
                    if (bs.readBitBE())
                        length = ((length - 1) << 1) + bs.readBitBE();

                    if (length != 9) {
                        rncGetOffset(bs, offset);

                        while (length--) {
                            *dst = dst[-offset];
                            dst++;
                        }
                    } else {
                        length = (bs.readBE(4) << 2) + 12;
                        while (length--)
                            *dst++ = bs.readByte();
                    }
                }
            }
        }
        delete[] cdata;

        height = 256;
        width  = uint32(dst - data) / height / 2;

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

        Texture *tex = new Texture(dw, dh, 1, FMT_RGBA, 0, data);
        tex->origWidth  = width;
        tex->origHeight = height;

        delete[] data;

        return tex;
    }
};


struct Atlas {
    struct Tile {
        uint16          id;
        TR::TextureInfo *tex;
        short4          uv;
    } *tiles;

    typedef void (Callback)(Atlas *atlas, int id, int tileX, int tileY, int atalsWidth, int atlasHeight, Tile &tile, void *userData, void *data);

    struct Node {
        Node   *child[2];
        short4 rect;
        int    tileIndex;

        Node(short l, short t, short r, short b) : rect(l, t, r, b), tileIndex(-1) { 
            child[0] = child[1] = NULL; 
        }

        ~Node() {
            delete child[0];
            delete child[1];
        }

        Node* insert(Atlas *atlas, const short4 &tile, int tileIndex) {
            ASSERT(tile.x != 0x7FFF);

            if (child[0] != NULL && child[1] != NULL) {
                Node *node = child[0]->insert(atlas, tile, tileIndex);
                if (node != NULL) return node;
                return child[1]->insert(atlas, tile, tileIndex);
            } else {
                if (this->tileIndex != -1)
                    return NULL;

                int16 w  = rect.z - rect.x;
                int16 h  = rect.w - rect.y;
                int16 tx = (tile.z - tile.x) + atlas->border.x + atlas->border.z;
                int16 ty = (tile.w - tile.y) + atlas->border.y + atlas->border.w;

                if (w < tx || h < ty)
                    return NULL;

                if (w == tx && h == ty) {
                    this->tileIndex = tileIndex;
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

                return child[0]->insert(atlas, tile, tileIndex);
            }
        }
    } *root;

    int      tilesCount;
    int      size;
    int      width, height;
    short4   border;
    void     *userData;
    Callback *callback;

    Atlas(int maxTiles, short4 border, void *userData, Callback *callback) : root(NULL), tilesCount(0), size(0), border(border), userData(userData), callback(callback) {
        tiles = new Tile[maxTiles];
    }

    ~Atlas() {
        delete root;
        delete[] tiles;
    }

    void add(uint16 id, short4 uv, TR::TextureInfo *tex) {
        for (int i = 0; i < tilesCount; i++)
            if (tiles[i].uv == uv && tiles[i].tex->type == tex->type && tiles[i].tex->tile == tex->tile && tiles[i].tex->clut == tex->clut) {
                uv.x = 0x7FFF;
                uv.y = tiles[i].id;
                uv.z = uv.w = 0;
                break;
            }

        tiles[tilesCount].id  = id;
        tiles[tilesCount].tex = tex;
        tiles[tilesCount].uv  = uv;
        tilesCount++;

        if (uv.x != 0x7FFF)
            size += (uv.z - uv.x + border.x + border.z) * (uv.w - uv.y + border.y + border.w);
    }

    bool insertAll(int *indices) {
        for (int i = 0; i < tilesCount; i++) {
            int idx = indices[i];
            if (tiles[idx].uv.x != 0x7FFF && !root->insert(this, tiles[idx].uv, idx))
                return false;
        }
        return true;
    }

    Texture* pack(bool mipmaps) {
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
                if ((a.z - a.x) < (b.z - b.x)) {
                    swap(indices[i - 1], indices[i]);
                    swapped = true;
                }
            }
            n--;
        } while (swapped);
    // pack
        while (1) {
            delete root;
            root = new Node(0, 0, width - 1, height - 1);

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

        Texture *atlas = new Texture(width, height, 1, FMT_RGBA, mipmaps ? OPT_MIPMAPS : 0, data);

        //Texture::SaveBMP("atlas", (char*)data, width, height);

        delete[] data;
        return atlas;
    };

    void fill(Node *node, void *data) {
        if (!node) return;

        if (node->tileIndex == -1) {
            fill(node->child[0], data);
            fill(node->child[1], data);
        } else
            callback(this, tiles[node->tileIndex].id, node->rect.x, node->rect.y, width, height, tiles[node->tileIndex], userData, data);
    }

    void fillInstances() {
        for (int i = 0; i < tilesCount; i++)
            if (tiles[i].uv.x == 0x7FFF)
                callback(this, tiles[i].id, tiles[i].uv.y, 0, width, height, tiles[i], userData, NULL);
    }
};

#endif
