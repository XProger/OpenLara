#ifndef H_GAPI_C3D
#define H_GAPI_C3D

#include <citro3d.h>
#include "core.h"

#define PROFILE_MARKER(title)
#define PROFILE_LABEL(id, name, label)
#define PROFILE_TIMING(time)

#define DISPLAY_WIDTH           240
#define DISPLAY_HEIGHT          400
#define DISPLAY_BUFFER_COUNT    2

#define DISPLAY_TRANSFER_FLAGS (\
      GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO) \
    | GX_TRANSFER_FLIP_VERT(0) \
    | GX_TRANSFER_OUT_TILED(0) \
    | GX_TRANSFER_RAW_COPY(0)  \
    | GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGB8) \
    | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) )

namespace GAPI {

    using namespace Core;

    struct Vertex {
        short4 coord;
        ubyte4 normal;
        short4 texCoord;
        ubyte4 color;
        ubyte4 light;
    };

    int VRAM_TOTAL = 0;

    void mmLogVRAM() {
        LOG("VRAM: %d / %d kb\n", (VRAM_TOTAL - vramSpaceFree()) / 1024, VRAM_TOTAL / 1024);
    }

    bool mmIsVRAM(void *addr) {
        uint32 vaddr = (uint32)addr;
        return vaddr >= 0x1F000000 && vaddr < 0x1F600000;
    }

    void* mmAlloc(size_t size) {
        void *addr = vramAlloc(size);
        if (!addr) {
            LOG("! OUT OF VRAM %d < %d\n", vramSpaceFree() / 1024, size / 1024);
            addr = linearAlloc(size);
            ASSERT(addr);
        } else {
            mmLogVRAM();
        }
        return addr;
    }

    void mmFree(void *addr) {
        if (!addr) return;
        if (mmIsVRAM(addr)) {
            vramFree(addr);
            mmLogVRAM();
        } else {
            linearFree(addr);
        }
    }

    void mmCopy(void *dst, void *src, size_t size) {
        if (mmIsVRAM(dst)) {
            GSPGPU_FlushDataCache(src, size);
            GX_RequestDma((u32*)src, (u32*)dst, size);
            gspWaitForDMA();
        } else {
            memcpy(dst, src, size);
            GSPGPU_FlushDataCache(dst, size);
        }
    }


// Shader
    extern "C" {
        #include "compose_sprite_shbin.h"
        #include "compose_flash_shbin.h"
        #include "compose_room_shbin.h"
        #include "compose_entity_shbin.h"
        #include "compose_mirror_shbin.h"
        #include "compose_sprite_u_shbin.h"
        #include "compose_room_u_shbin.h"
        #include "compose_entity_u_shbin.h"
        #include "ambient_sprite_shbin.h"
        #include "ambient_room_shbin.h"
        #include "shadow_entity_shbin.h"
        #include "filter_upscale_shbin.h"
        #include "gui_shbin.h"
        #include "dummy_shbin.h"
    }

    #define SHADERS_LIST(E) \
        E( compose_sprite   ) \
        E( compose_flash    ) \
        E( compose_room     ) \
        E( compose_entity   ) \
        E( compose_mirror   ) \
        E( compose_sprite_u ) \
        E( compose_room_u   ) \
        E( compose_entity_u ) \
        E( ambient_sprite   ) \
        E( ambient_room     ) \
        E( shadow_entity    ) \
        E( filter_upscale   ) \
        E( gui              ) \
        E( dummy            )

    #define SHADER_DECL(v) DVLB_s* v;
    #define SHADER_INIT(v) v = DVLB_ParseFile((u32*)v##_shbin, v##_shbin_size);
    #define SHADER_FREE(v) DVLB_Free(v);

    static const int bindings[uMAX] = {
         0, // uParam
         1, // uTexParam
         2, // uViewProj
         6, // uBasis
        70, // uLightProj
        74, // uMaterial
        75, // uAmbient
        81, // uFogParams
        82, // uViewPos
        83, // uLightPos
        87, // uLightColor
        91, // uRoomSize
        92, // uPosScale
         0, // uContacts (unused)
    };

    SHADERS_LIST(SHADER_DECL);

    struct FogLUT {
        vec4       params;
        uint32     color;
        C3D_FogLut table;
    } fogLUT[3];

    vec4 fogParams;

    struct Shader {
        shaderProgram_s program;
        C3D_TexEnv      env[4];
        int             envCount;

        int32   uID[uMAX];

        vec4  cbMem[98 + MAX_CONTACTS];
        int   cbCount[uMAX];

        bool  rebind;

        void init(Pass pass, int type, int *def, int defCount) {
            shaderProgramInit(&program);

            DVLB_s* src = NULL;

            bool underwater = false;
            bool grayscale  = false;

            for (int i = 0; i < defCount; i++) {
                if (def[i] == SD_UNDERWATER) {
                    underwater = true;
                }
                if (def[i] == SD_FILTER_GRAYSCALE) {
                    grayscale = true;
                }
            }

            switch (pass) {
                case Core::passCompose :
                    if (underwater) {
                        switch (type) {
                            case 0  : src = compose_sprite_u; break;
                            case 1  : src = compose_flash;    break;
                            case 2  : src = compose_room_u;   break;
                            case 3  : src = compose_entity_u; break;
                            case 4  : src = compose_mirror;   break;
                            default : src = dummy;
                        }
                    } else {
                        switch (type) {
                            case 0  : src = compose_sprite; break;
                            case 1  : src = compose_flash;  break;
                            case 2  : src = compose_room;   break;
                            case 3  : src = compose_entity; break;
                            case 4  : src = compose_mirror; break;
                            default : src = dummy;
                        }
                    }
                    break;
                case Core::passAmbient :
                    switch (type) {
                        case 0  : src = ambient_sprite; break;
                        case 1  : src = ambient_room;   break;
                        case 2  : src = ambient_room;   break;
                        default : src = dummy;
                    }
                    break;
                case Core::passShadow : src = shadow_entity;  break;
                case Core::passFilter : src = filter_upscale; break;
                case Core::passGUI    : src = gui;            break;
                default               : src = dummy;
            }

            shaderProgramSetVsh(&program, &src->DVLE[0]);

            for (int ut = 0; ut < uMAX; ut++) {
                uID[ut] = shaderInstanceGetUniformLocation(program.vertexShader, UniformName[ut]);
            }

            rebind = true;
            envCount = 0;

            for (int i = 0; i < COUNT(env); i++) {
                C3D_TexEnvInit(env + i);
            }

            C3D_TexEnv *e = env;

            { // texture * vertex color
                C3D_TexEnvSrc(e, C3D_Both, GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
                C3D_TexEnvFunc(e, C3D_Both, GPU_MODULATE);
                if (pass == Core::passCompose || pass == Core::passAmbient) {
                    C3D_TexEnvScale(e, C3D_Both, GPU_TEVSCALE_4);
                }
                e++;
            }

            if (pass == Core::passAmbient && underwater) { // multiply by underwater color only for ambient pass
                C3D_TexEnvSrc(e, C3D_Both, GPU_PREVIOUS, GPU_CONSTANT, GPU_PRIMARY_COLOR);
                C3D_TexEnvFunc(e, C3D_Both, GPU_MODULATE);
                C3D_TexEnvColor(e, 0xFFE5E599);
                e++;
            }

            if (grayscale) { // grayscale * blue tint
                C3D_TexEnvSrc(e, C3D_RGB, GPU_PREVIOUS, GPU_CONSTANT, GPU_CONSTANT);
                C3D_TexEnvFunc(e, C3D_RGB, GPU_MULTIPLY_ADD);
                C3D_TexEnvColor(e, 0x00808080);
                e++;

                C3D_TexEnvSrc(e, C3D_RGB, GPU_PREVIOUS, GPU_CONSTANT, GPU_PRIMARY_COLOR);
                C3D_TexEnvFunc(e, C3D_RGB, GPU_DOT3_RGB);
                C3D_TexEnvColor(e, 0x008ECAA6);
                e++;

                C3D_TexEnvSrc(e, C3D_RGB, GPU_PREVIOUS, GPU_CONSTANT, GPU_PRIMARY_COLOR);
                C3D_TexEnvFunc(e, C3D_RGB, GPU_MODULATE);
                C3D_TexEnvColor(e, 0x00FFC0C0);
                e++;
            }

            envCount = e - env;
        }

        void deinit() {
            shaderProgramFree(&program);
        }

        void bind() {
            if (active.shader != this) {
                active.shader = this;
                memset(cbCount, 0, sizeof(cbCount));
                rebind = true;
            }
        }

        void validate() {
            if (rebind) {
                C3D_BindProgram(&program);
                for (int i = 0; i < COUNT(env); i++) {
                    C3D_SetTexEnv(i, env + i);
                }
                rebind = false;
            }

            for (int uType = 0; uType < uMAX; uType++) {
                if (!cbCount[uType]) continue;

                vec4 *src = cbMem + bindings[uType];
                vec4 *dst = (vec4*)C3D_FVUnifWritePtr(GPU_VERTEX_SHADER, uID[uType], cbCount[uType]);

                for (int i = 0; i < cbCount[uType]; i++) {
                    dst->x = src->w;
                    dst->y = src->z;
                    dst->z = src->y;
                    dst->w = src->x;
                    dst++;
                    src++;
                }

                cbCount[uType] = 0;
                Core::stats.cb++;
            }
        }

        void setParam(UniformType uType, const vec4 &value, int count = 1) {
            if (uID[uType] == -1) return;
            cbCount[uType] = max(cbCount[uType], count);
            memcpy(cbMem + bindings[uType], &value, count * sizeof(value));
        }

        void setParam(UniformType uType, const mat4 &value, int count = 1) {
            if (uID[uType] == -1) return;
            cbCount[uType] = max(cbCount[uType], count * 4);

            ASSERT(count == 1);
            memcpy(cbMem + bindings[uType], &value, count * sizeof(value));
        }
    };

// Texture
    static const struct FormatDesc {
        uint8        bpp;
        GPU_TEXCOLOR format;
    } formats[FMT_MAX] = {
        {  8 , GPU_L8       }, // LUMINANCE
        { 32 , GPU_RGBA8    }, // RGBA
        { 16 , GPU_RGB565   }, // RGB16
        { 16 , GPU_RGBA5551 }, // RGBA16
        { 32 , GPU_RGBA8    }, // RG_FLOAT  // not supported
        { 32 , GPU_RGBA8    }, // RG_HALF   // not supported
        { 16 , GPU_RGBA8    }, // DEPTH
        { 16 , GPU_RGBA8    }, // SHADOW
    };

    #define TILE_SIZE 8

    static const uint8 tileSwizzle[TILE_SIZE * TILE_SIZE] = {
         0,  1,  8,  9,  2,  3, 10, 11, 
        16, 17, 24, 25, 18, 19, 26, 27,
         4,  5, 12, 13,  6,  7, 14, 15,
        20, 21, 28, 29, 22, 23, 30, 31,
        32, 33, 40, 41, 34, 35, 42, 43,
        48, 49, 56, 57, 50, 51, 58, 59,
        36, 37, 44, 45, 38, 39, 46, 47, 
        52, 53, 60, 61, 54, 55, 62, 63
    };

    struct Texture {
        int           width, height, origWidth, origHeight;
        TexFormat     fmt;
        uint32        opt;
        int           mipCount;

        C3D_Tex          tex;
        C3D_TexCube      texCube;
        C3D_RenderTarget *target;

        void convertImage32(uint32 *dst, uint32 *src, int dstWidth, int dstHeight, int srcWidth, int srcHeight) {
            // 8x8 tiles swizzling
            // vertical flip
            // swap RGBA channels to ABGR
            dst += dstWidth * (dstHeight - srcHeight);
            for (int y = 0; y < srcHeight; y += TILE_SIZE) {
                for (int x = 0; x < srcWidth; x += TILE_SIZE) {
                    for (int i = 0; i < COUNT(tileSwizzle); i++) {
                        int sx = tileSwizzle[i] % TILE_SIZE;
                        int sy = (tileSwizzle[i] - sx) / TILE_SIZE;
                        int index = (srcHeight - (y + sy) - 1) * srcWidth + (x + sx);

                        *dst++ = swap32(src[index]);
                    }
                }
                dst += (dstWidth - srcWidth) * TILE_SIZE;
            }
        }

        void convertImage16(uint16 *dst, uint16 *src, int dstWidth, int dstHeight, int srcWidth, int srcHeight) {
            // 8x8 tiles swizzling
            // vertical flip
            // swap RGBA channels to ABGR
            dst += dstWidth * (dstHeight - srcHeight);
            for (int y = 0; y < srcHeight; y += TILE_SIZE) {
                for (int x = 0; x < srcWidth; x += TILE_SIZE) {

                    for (int i = 0; i < COUNT(tileSwizzle); i++) {
                        int sx = tileSwizzle[i] % TILE_SIZE;
                        int sy = (tileSwizzle[i] - sx) / TILE_SIZE;
                        int index = (srcHeight - (y + sy) - 1) * srcWidth + (x + sx);

                        *dst++ = src[index];
                    }
                }
                dst += (dstWidth - srcWidth) * TILE_SIZE;
            }
        }

        void downsampleImage32(uint32 *dst, uint32 *src, int width, int height) {
            for (int y = 0; y < height; y += 2) {
                for (int x = 0; x < width; x += 2) {
                    Color32 a(src[0]);
                    Color32 b(src[1]);
                    Color32 c(src[width]);
                    Color32 d(src[width + 1]);
                    Color32 &p = *(Color32*)dst;
                    p.r = (uint16(a.r) + uint16(b.r) + uint16(c.r) + uint16(d.r)) >> 2;
                    p.g = (uint16(a.g) + uint16(b.g) + uint16(c.g) + uint16(d.g)) >> 2;
                    p.b = (uint16(a.b) + uint16(b.b) + uint16(c.b) + uint16(d.b)) >> 2;
                    p.a = (uint16(a.a) + uint16(b.a) + uint16(c.a) + uint16(d.a)) >> 2;
                    dst++;
                    src += 2;
                }
                src += width;
            }
        }

        void downsampleImage16(uint16 *dst, uint16 *src, int width, int height) {
            for (int y = 0; y < height; y += 2) {
                for (int x = 0; x < width; x += 2) {
                    AtlasColor a(src[0]);
                    AtlasColor b(src[1]);
                    AtlasColor c(src[width]);
                    AtlasColor d(src[width + 1]);
                    AtlasColor &p = *(AtlasColor*)dst;
                    p.r = (uint16(a.r) + uint16(b.r) + uint16(c.r) + uint16(d.r)) >> 2;
                    p.g = (uint16(a.g) + uint16(b.g) + uint16(c.g) + uint16(d.g)) >> 2;
                    p.b = (uint16(a.b) + uint16(b.b) + uint16(c.b) + uint16(d.b)) >> 2;
                    p.a = (uint16(a.a) + uint16(b.a) + uint16(c.a) + uint16(d.a)) >> 2;
                    dst++;
                    src += 2;
                }
                src += width;
            }
        }

        void convertImage(void *dst, void *src, int dstWidth, int dstHeight, int srcWidth, int srcHeight) {
            FormatDesc desc = formats[fmt];

            if (fmt == FMT_RGBA) {
                convertImage32((uint32*)dst, (uint32*)src, dstWidth, dstHeight, srcWidth, srcHeight);
            } else {
                convertImage16((uint16*)dst, (uint16*)src, dstWidth, dstHeight, srcWidth, srcHeight);
            }
        }

        void downsampleImage(void *dst, void *src, int width, int height) {
            FormatDesc desc = formats[fmt];

            if (fmt == FMT_RGBA) {
                downsampleImage32((uint32*)dst, (uint32*)src, width, height);
            } else {
                downsampleImage16((uint16*)dst, (uint16*)src, width, height);
            }
        }

        Texture(int width, int height, int depth, uint32 opt) : width(width), height(height), origWidth(width), origHeight(height), fmt(FMT_RGBA), opt(opt) {
            opt |= OPT_NEAREST;
            target = (C3D_RenderTarget*)malloc(sizeof(C3D_RenderTarget) * 6);
            memset(target, 0, sizeof(C3D_RenderTarget) * 6);
        }

        void init(void *data) {
            ASSERT((opt & OPT_PROXY) == 0);
            
            FormatDesc desc = formats[fmt];

            if (width < 8 || height < 8) {
                LOG("\ntexture too small %dx%d [%d %d]!\n\n", width, height, fmt, opt);
                width  = 8;
                height = 8;
                data   = NULL;
            }

            void* tmpData = NULL;

            if (width > 1024 || height > 1024) {
                LOG("\ntexture too large %dx%d [%d %d]!\n", width, height, fmt, opt);

                origWidth  >>= 1;
                origHeight >>= 1;
                width      >>= 1;
                height     >>= 1;

                LOG("downsample to %dx%d\n\n", width, height);

                tmpData = linearAlloc(width * height * desc.bpp / 8);
                downsampleImage(tmpData, data, width << 1, height << 1);

                data = tmpData;
            }

            bool isCube   = (opt & OPT_CUBEMAP) != 0;
            bool isShadow = fmt == FMT_SHADOW;

            C3D_TexInitParams params;
            memset(&params, 0, sizeof(params));
            params.width    = width;
            params.height   = height;
            params.maxLevel = ((opt & OPT_MIPMAPS) != 0) ? min(3, C3D_TexCalcMaxLevel(width, height)) : 0;
            params.format   = desc.format;
            params.onVram   = (opt & OPT_VRAM_3DS) != 0;

            if (isCube && isShadow)
                params.type = GPU_TEX_SHADOW_CUBE;
            else if (isCube)
                params.type = GPU_TEX_CUBE_MAP;
            else if (isShadow)
                params.type = GPU_TEX_SHADOW_2D;
            else
                params.type = GPU_TEX_2D;

            bool ret = C3D_TexInitWithParams(&tex, &texCube, params);
            if (!ret && params.onVram) {
                params.onVram = false;
                ret = C3D_TexInitWithParams(&tex, &texCube, params);
            }

            if (width != origWidth || height != origHeight) {
                uint32 texSize = C3D_TexCalcTotalSize(tex.size, tex.maxLevel);
                memset(tex.data, 0, texSize);
            }

            ASSERT(ret);

            mmLogVRAM();

            if (data && !isCube) {
                update(data);
            }

            if (tmpData) {
                linearFree(tmpData);
            }

            GPU_TEXTURE_FILTER_PARAM filter = (opt & OPT_NEAREST) ? GPU_NEAREST : GPU_LINEAR;
            C3D_TexSetFilter(&tex, filter, filter);
            C3D_TexSetFilterMipmap(&tex, filter);
        }

        void deinit() {
            C3D_TexDelete(&tex);
            mmLogVRAM();

            free(target);
        }

        void generateMipMap() {
            /*
            if (opt & OPT_MIPMAPS) {
                for (int i = 0; i < 6; i++) {
                    C3D_TexGenerateMipmap(&tex, GPU_TEXFACE(i));
                    if (!(opt & OPT_CUBEMAP)) {
                        break;
                    }
                }
            }
            */
        }

        void update(void *data) {
            if (!data) return;

            FormatDesc desc = formats[fmt];

            bool isVRAM = mmIsVRAM(tex.data);

            uint32 texSize = C3D_TexCalcTotalSize(tex.size, tex.maxLevel);
            void* texData  = isVRAM ? linearAlloc(texSize) : tex.data;
            void* mipData  = tex.maxLevel ? linearAlloc(texSize >> 2) : NULL;

            uint8* ptr = (uint8*)mipData;
            uint8* mip = (uint8*)data;
            uint32 w   = width;
            uint32 h   = height;

            for (int i = 1; i <= tex.maxLevel; i++) {
                downsampleImage(ptr, mip, w, h);

                mip = ptr;
                w >>= 1;
                h >>= 1;
                ptr += w * h * desc.bpp / 8;
            }

            convertImage(texData, data, width, height, origWidth, origHeight);

            ptr = (uint8*)texData;
            mip = (uint8*)mipData;
            w   = width;
            h   = height;

            for (int i = 1; i <= tex.maxLevel; i++) {
                ptr += w * h * desc.bpp / 8;
                w >>= 1;
                h >>= 1;

                convertImage(ptr, mip, w, h, w, h);

                mip += w * h * desc.bpp / 8;
            }

            GSPGPU_FlushDataCache(texData, texSize);

            if (isVRAM) {
                C3D_SyncTextureCopy((u32*)texData, 0, (u32*)tex.data, 0, texSize, 8);
                linearFree(texData);
            }

            if (mipData) {
                linearFree(mipData);
            }
        }

        void bind(int sampler) {
            if (opt & OPT_PROXY) return;

            if (sampler > 3) {
                return;
            }

            if (active.textures[sampler] != this) {
                active.textures[sampler] = this;
                C3D_TexBind(sampler, &tex);
            }
        }

        void unbind(int sampler) {
            active.textures[sampler] = NULL;
        }

        void setFilterQuality(int value) {
            GPU_TEXTURE_FILTER_PARAM filter = ((opt & OPT_NEAREST) == 0 && (value > Settings::LOW)) ? GPU_LINEAR : GPU_NEAREST;

            C3D_TexSetFilter(&tex, filter, filter);
            C3D_TexSetFilterMipmap(&tex, filter);
        }
    };

    #undef TILE_SIZE

// Mesh
    struct Mesh {
        C3D_BufInfo *VAO;

        Index       *iBuffer;
        Vertex      *vBuffer;

        int         aCount;
        bool        dynamic;

        struct Chunk {
            int frameIndex;
            int iBase, iStart, iCount;
            int vBase, vStart, vCount;
        } chunks[DISPLAY_BUFFER_COUNT];

        Mesh(bool dynamic) : iBuffer(NULL), vBuffer(NULL), dynamic(dynamic) {}

        void init(Index *indices, int iCount, ::Vertex *vertices, int vCount, int aCount) {
            ASSERT(Core::support.VAO && aCount);

            this->aCount = aCount;

            memset(chunks, 0, sizeof(chunks));

            for (int i = 0; i < COUNT(chunks); i++) {
                chunks[i].frameIndex = -1;
                chunks[i].iBase = i * iCount;
                chunks[i].vBase = i * vCount;
            }

            if (dynamic) {
                iCount *= COUNT(chunks);
                vCount *= COUNT(chunks);
            }

            VAO = new C3D_BufInfo[aCount];

            iBuffer = (Index*)  (dynamic ? linearAlloc(iCount * sizeof(Index))  : mmAlloc(iCount * sizeof(Index)));
            vBuffer = (Vertex*) (dynamic ? linearAlloc(vCount * sizeof(Vertex)) : mmAlloc(vCount * sizeof(Vertex)));

            if (!dynamic) {
                update(indices, iCount, vertices, vCount);
            }
        }

        void deinit() {
            delete[] VAO;
            mmFree(iBuffer);
            mmFree(vBuffer);
        }

        Chunk& getChunk() {
            return dynamic ? chunks[Core::stats.frameIndex % COUNT(chunks)] : chunks[0];
        }

        void update(Index *indices, int iCount, ::Vertex *vertices, int vCount) {
            Chunk &chunk = getChunk();
            if (chunk.frameIndex != Core::stats.frameIndex) {
                chunk.frameIndex = Core::stats.frameIndex;
                chunk.iStart = chunk.iCount = chunk.iBase;
                chunk.vStart = chunk.vCount = chunk.vBase;
            }

            if (indices && iCount) {
                chunk.iStart = chunk.iCount;
                chunk.iCount += iCount;
                mmCopy(iBuffer + chunk.iStart, indices, iCount * sizeof(Index));
            }

            if (vertices && vCount) {
                chunk.vStart = chunk.vCount;
                chunk.vCount += vCount;

                Vertex *vert = (Vertex*)linearAlloc(sizeof(Vertex) * vCount);

                for (int i = 0; i < vCount; i++) {
                    ::Vertex &vIn  = vertices[i];
                    Vertex   &vOut = vert[i];

                    vOut.coord    = vIn.coord;
                    vOut.normal   = ubyte4(127 + vIn.normal.x / 256, 127 + vIn.normal.y / 256, 127 + vIn.normal.z / 256, 0);
                    vOut.texCoord = vIn.texCoord;
                    vOut.color    = vIn.color;
                    vOut.light    = vIn.light;
                }

                mmCopy(vBuffer + chunk.vStart, vert, vCount * sizeof(Vertex));

                linearFree(vert);
            }
        }

        void initVAO(C3D_BufInfo *vao, Vertex *offset) {
            BufInfo_Init(vao);
            BufInfo_Add(vao, offset, sizeof(Vertex), 5, 0x43210);
        }

        void initNextRange(MeshRange &range, int &aIndex) {
            range.aIndex = aIndex++;
            initVAO(VAO + range.aIndex, vBuffer + range.vStart);
        }

        void bind(const MeshRange &range) {
            ASSERT(range.aIndex > -1);
            C3D_BufInfo *vao = VAO + range.aIndex;

            if (dynamic) {
                initVAO(vao, vBuffer + getChunk().vStart + range.vStart);
            } else {
                if (Core::active.VAO == vao) {
                    return;
                }
            }
            
            C3D_SetBufInfo(vao);
            Core::active.VAO = vao;
        }
    };

    bool   rotate90;
    bool   depthTest;
    uint32 colorMask, depthMask;
    uint32 clearColor;

    C3D_RenderTarget *curTarget;
    C3D_RenderTarget *defTarget[2];
    C3D_AttrInfo      vertexAttribs;

    // depth buffer memory aliasing
    #define MAX_DEPTH_ALIAS_GROUPS 1

    struct DepthBuffer {
        void *data;
        int   size;
    } depthBuffers[MAX_DEPTH_ALIAS_GROUPS];

    void* getDepthBuffer(int width, int height, int group, GPU_DEPTHBUF format) {
        ASSERT(group < MAX_DEPTH_ALIAS_GROUPS);

        DepthBuffer &db = depthBuffers[group];

        int size = C3D_CalcDepthBufSize(width, height, format);

        if (!db.data) {
            LOG("alloc depth alias group %d (size: %d %dx%d)\n", group, size / 1024, width, height);

            db.data = mmAlloc(size);
            db.size = size;
            return db.data;
        }

        if (db.size >= size) {
            return db.data;
        }

        LOG("! can't fit depth %dx%d %d ([%d] = %d)\n", width, height, size / 1024, group, db.size / 1024);

        ASSERT(false);
        return NULL;
    }

    C3D_RenderTarget* checkRenderTarget(Texture *texture, int face, int group, GPU_DEPTHBUF depthFmt) {
        if (!texture->target[face].frameBuf.colorBuf) {
            C3D_FrameBuf &fb = texture->target[face].frameBuf;
            fb.colorBuf  = (texture->opt & OPT_CUBEMAP) ? texture->tex.cube->data[face] : texture->tex.data;
            fb.depthBuf  = getDepthBuffer(texture->width, texture->height, group, depthFmt);
            fb.colorFmt  = GPU_COLORBUF(formats[texture->fmt].format);
            fb.depthFmt  = depthFmt;
            fb.colorMask = 0x0F;
            fb.depthMask = 0x02; // no stencil
            fb.width     = texture->width;
            fb.height    = texture->height;
            fb.block32   = false;
        }

        return &texture->target[face];
    }

    void init() {
        memset(depthBuffers, 0, sizeof(depthBuffers));

        for (int i = 0; i < COUNT(fogLUT); i++) {
            fogLUT[i].params = vec4(-2.0f); // initialize with some unique value
        }

        gfxInitDefault();

        vramFree(vramAlloc(0)); // vramInit()
        VRAM_TOTAL = vramSpaceFree();

        consoleInit(GFX_BOTTOM, NULL);

        LOG("Vendor   : %s\n", "DMP");
        LOG("Renderer : %s\n", "PICA200 citro3D");

        C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);

        support.shaderBinary   = true;
        support.VAO            = true;
        support.texMinSize     = 8;

    // rotated
        Core::width  = DISPLAY_HEIGHT;
        Core::height = DISPLAY_WIDTH;

    // init default vertex declaration
        AttrInfo_Init(&vertexAttribs);
        AttrInfo_AddLoader(&vertexAttribs, aCoord    , GPU_SHORT         , 4);
        AttrInfo_AddLoader(&vertexAttribs, aNormal   , GPU_UNSIGNED_BYTE , 4);
        AttrInfo_AddLoader(&vertexAttribs, aTexCoord , GPU_SHORT         , 4);
        AttrInfo_AddLoader(&vertexAttribs, aColor    , GPU_UNSIGNED_BYTE , 4);
        AttrInfo_AddLoader(&vertexAttribs, aLight    , GPU_UNSIGNED_BYTE , 4);

        rotate90   = true;
        depthTest  = false;
        clearColor = 0; //0x68B0D8FF;
        colorMask  = GPU_WRITE_COLOR;
        depthMask  = GPU_WRITE_DEPTH;

    // init shaders
        SHADERS_LIST(SHADER_INIT);
    }

    void deinit() {
        SHADERS_LIST(SHADER_FREE);

        C3D_Fini();
        gfxExit();
    }

    void initOutput(Texture **outputTex) {
        // VRAM +562k (2 x Color) +192k (1 x Depth)
        for (int i = 0; i < COUNT(GAPI::defTarget); i++) {
            Texture *tex = outputTex[i];
            tex->width  = tex->origWidth;
            tex->height = tex->origHeight;

            // output target has portrait orientation (rotate90)
            int width  = tex->height;
            int height = tex->width;

            C3D_RenderTarget *target = C3D_RenderTargetCreate(width, height, GPU_RB_RGB8, C3D_DEPTHTYPE(-1));
            mmLogVRAM();
            void *depthBuf = getDepthBuffer(width, height, 0, GPU_RB_DEPTH16);
            C3D_FrameBufDepth(&target->frameBuf, depthBuf, GPU_RB_DEPTH16);

            tex->target[0] = *target;

            GAPI::defTarget[i] = &tex->target[0];
        }

        C3D_RenderTargetSetOutput(defTarget[0], GFX_TOP, GFX_LEFT,  DISPLAY_TRANSFER_FLAGS);
        C3D_RenderTargetSetOutput(defTarget[1], GFX_TOP, GFX_RIGHT, DISPLAY_TRANSFER_FLAGS);

        active.target = outputTex[1]; // ????
        curTarget = defTarget[0];
        C3D_FrameDrawOn(curTarget); // ????
    }

    inline mat4::ProjRange getProjRange() {
        return mat4::PROJ_NEG_ZERO;
    }

    mat4 ortho(float l, float r, float b, float t, float znear, float zfar) {
        mat4 m;
        m.ortho(getProjRange(), l, r, b, t, znear, zfar, rotate90);
        return m;
    }

    mat4 perspective(float fov, float aspect, float znear, float zfar, float eye) {
        mat4 m;
        m.perspective(getProjRange(), fov, aspect, znear, zfar, eye, rotate90);
        return m;
    }

    bool beginFrame() {
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        return true;
    }

    void endFrame() {
        C3D_FrameEnd(0);
    }

    void resetState() {
        fogParams = vec4(-1.0f);
        C3D_SetAttrInfo(&vertexAttribs);
    }

    void bindTarget(Texture *target, int face) {
        if (target) {
            curTarget = checkRenderTarget(target, face, 0, GPU_RB_DEPTH16);
        } else {
            curTarget = defTarget[0];
        }

        C3D_FrameDrawOn(curTarget);
    }

    void discardTarget(bool color, bool depth) {}

    void copyTarget(Texture *dst, int xOffset, int yOffset, int x, int y, int width, int height) {
        //
    }

    void setVSync(bool enable) {}

    void present() {}

    bool isRotate90() {
        return (curTarget == defTarget[0]) || (curTarget == defTarget[1]);
    }

    void setViewport(const short4 &v) {
        if (isRotate90()) {
            C3D_SetViewport(v.y, Core::viewportDef.z - (v.x + v.z), v.w, v.z);
        } else {
            C3D_SetViewport(v.x, v.y, v.z, v.w);
        }
    }

    void setScissor(const short4 &s) {
        if (isRotate90()) {
            C3D_SetScissor(GPU_SCISSOR_NORMAL, s.y, Core::viewportDef.z - (s.x + s.z), s.y + s.w, Core::viewportDef.z - s.x);
        } else {
            C3D_SetScissor(GPU_SCISSOR_NORMAL, s.x, s.y, s.x + s.z, s.y + s.w);
        }
    }

    void updateWriteMask() {
        C3D_DepthTest(depthTest, GPU_GREATER, GPU_WRITEMASK(colorMask | depthMask));
        //C3D_EarlyDepthTest(depthTest, GPU_EARLYDEPTH_GREATER, 0); // TODO block32 256x416 input -> 240x400 output
    }

    void setDepthTest(bool enable) {
        depthTest = enable;
        updateWriteMask();
    }

    void setDepthWrite(bool enable) {
        depthMask = enable ? GPU_WRITE_DEPTH : 0;
        updateWriteMask();
    }

    void setColorWrite(bool r, bool g, bool b, bool a) {
        colorMask = 0;
        if (r) colorMask |= GPU_WRITE_RED;
        if (g) colorMask |= GPU_WRITE_GREEN;
        if (b) colorMask |= GPU_WRITE_BLUE;
        if (a) colorMask |= GPU_WRITE_ALPHA;
        updateWriteMask();
    }

    void setAlphaTest(bool enable) {
        C3D_AlphaTest(enable, GPU_GREATER, 128);
    }

    void setCullMode(int rsMask) {
        switch (rsMask) {
            case RS_CULL_BACK  : C3D_CullFace(GPU_CULL_BACK_CCW);  break;
            case RS_CULL_FRONT : C3D_CullFace(GPU_CULL_FRONT_CCW); break;
            default            : C3D_CullFace(GPU_CULL_NONE);
        }
    }

    void setBlendMode(int rsMask) {
        switch (rsMask) {
            case RS_BLEND_ALPHA   : C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA); break;
            case RS_BLEND_ADD     : C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ONE, GPU_ONE, GPU_ONE);  break;
            case RS_BLEND_MULT    : C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_DST_COLOR, GPU_ZERO, GPU_DST_COLOR, GPU_ZERO); break;
            case RS_BLEND_PREMULT : C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ONE_MINUS_SRC_ALPHA, GPU_ONE, GPU_ONE_MINUS_SRC_ALPHA); break;
            default               : C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ZERO, GPU_ONE, GPU_ZERO); break;
        }
    }

    void setViewProj(const mat4 &mView, const mat4 &mProj) {}

    void DIP(Mesh *mesh, const MeshRange &range) {
        if (!active.shader) return;

        active.shader->validate();

        C3D_DrawElements(GPU_TRIANGLES, range.iCount, C3D_UNSIGNED_SHORT, mesh->iBuffer + mesh->getChunk().iStart + range.iStart);
    }

    void updateLights(vec4 *lightPos, vec4 *lightColor, int count) {
        if (active.shader) {
            active.shader->setParam(uLightColor, lightColor[0], count);
            active.shader->setParam(uLightPos,   lightPos[0],   count);
        }
    }

    void setFog(const vec4 &params) {
        if (fogParams == params) return;

        fogParams = params;

        int32 index;

        if (fogLUT[0].params == params) {
            index = 0;
        } else if (fogLUT[1].params == params) {
            index = 1;
        } else if (fogLUT[2].params == params) {
            index = 2;
        } else {
            index = 2;
            fogLUT[0] = fogLUT[1];
            fogLUT[1] = fogLUT[2];

            // for some reason GPU_NO_FOG breaks depth, blend or texEnv states in some cases, so we use low density fog table (0.0f) as NO_FOG
            FogLut_Exp(&fogLUT[index].table, params.w, 1.0f, 32.0f, 45.0f * 1024.0f);
            fogLUT[index].params = params;
            fogLUT[index].color = 0xFF000000
                | (uint32(clamp(params.x * 255.0f, 0.0f, 255.0f)) << 0)
                | (uint32(clamp(params.y * 255.0f, 0.0f, 255.0f)) << 8)
                | (uint32(clamp(params.z * 255.0f, 0.0f, 255.0f)) << 16);
        }


        C3D_FogGasMode(GPU_FOG, GPU_PLAIN_DENSITY, false);
        C3D_FogColor(fogLUT[index].color);
        C3D_FogLutBind(&fogLUT[index].table);
    }

    void clear(bool color, bool depth) {
        uint32 mask = 0;
        if (color) mask |= C3D_CLEAR_COLOR;
        if (depth) mask |= C3D_CLEAR_DEPTH;
        if (!mask) return;

        C3D_FrameSplit(0);
        C3D_FrameBufClear(&curTarget->frameBuf, C3D_ClearBits(mask), clearColor, 0);
    }

    void setClearColor(const vec4 &color) {
        clearColor  = (uint32(color.w * 255))
                    | (uint32(color.z * 255) <<  8)
                    | (uint32(color.y * 255) << 16)
                    | (uint32(color.x * 255) << 24);
    }

    vec4 copyPixel(int x, int y) {
//        GAPI::Texture *t = Core::active.target;
//        Color32 *color = (Color32*)t->data;
//        return vec4(color->r, color->g, color->b, 255.0f) * (1.0f / 255.0f);
        return vec4(0.0f); // TODO: read from framebuffer
    }
}

#endif