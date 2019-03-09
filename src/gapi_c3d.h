#ifndef H_GAPI_C3D
#define H_GAPI_C3D

#include <3ds.h>
#include <citro3d.h>

#include "core.h"

#define PROFILE_MARKER(title)
#define PROFILE_LABEL(id, name, label)
#define PROFILE_TIMING(time)

#define DISPLAY_BUFFER_COUNT    2

#define DISPLAY_TRANSFER_FLAGS (\
      GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_XY) \
    | GX_TRANSFER_FLIP_VERT(0) \
    | GX_TRANSFER_OUT_TILED(0) \
    | GX_TRANSFER_RAW_COPY(0)  \
    | GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) \
    | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) )

namespace GAPI {

    using namespace Core;

    typedef ::Vertex Vertex;

// Shader
    extern "C" {
        #include "compose_shbin.h"
        #include "gui_shbin.h"
        #include "filter_upscale_shbin.h"
    }

    static const int bindings[uMAX] = {
        94, // uFlags
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
        98, // uContacts
    };

    DVLB_s* compose_dvlb;
    DVLB_s* filter_upscale_dvlb;
    DVLB_s* gui_dvlb;

    struct Shader {
        shaderProgram_s program;
        C3D_TexEnv      env;

        int32   uID[uMAX];

        vec4  cbMem[98 + MAX_CONTACTS];
        int   cbCount[uMAX];

        bool  rebind;

        void init(Pass pass, int type, int *def, int defCount) {
	        shaderProgramInit(&program);

            DVLB_s* src = NULL;

            switch (pass) {
                case Core::passFilter : src = filter_upscale_dvlb; break;
                case Core::passGUI    : src = gui_dvlb;            break;
                default               : src = compose_dvlb;
            }

	        shaderProgramSetVsh(&program, &src->DVLE[0]);

            for (int ut = 0; ut < uMAX; ut++) {
                uID[ut] = shaderInstanceGetUniformLocation(program.vertexShader, UniformName[ut]);
            }

            rebind = true;

            C3D_TexEnvInit(&env);
            C3D_TexEnvSrc(&env, C3D_Both, GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
            C3D_TexEnvFunc(&env, C3D_Both, GPU_MODULATE);
            if (Core::pass == Core::passCompose && type == 2) { // rooms
                C3D_TexEnvScale(&env, C3D_RGB, GPU_TEVSCALE_2);
            }
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
                C3D_SetTexEnv(0, &env);
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
            cbCount[uType] = count;
            memcpy(cbMem + bindings[uType], &value, count * 16);
        }

        void setParam(UniformType uType, const mat4 &value, int count = 1) {
            if (uID[uType] == -1) return;
            cbCount[uType] = count * 4;

            ASSERT(count == 1);
            mat4 m = value.transpose();
            memcpy(cbMem + bindings[uType], &m, sizeof(m));
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

    static const uint8 tileSwizzle[64] = {
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

        C3D_Tex       tex;
        C3D_TexCube   texCube;

        void convertImage(uint32 *dst, uint32 *src, int dstWidth, int dstHeight, int srcWidth, int srcHeight, int bpp) {
            // 8x8 tiles swizzling
            // vertical flip
            // swap RGBA channels to ABGR
            dst += dstWidth * (dstHeight - srcHeight);
            for (int y = 0; y < srcHeight; y += 8) {
                for (int x = 0; x < srcWidth; x += 8) {
                    for (int i = 0; i < COUNT(tileSwizzle); i++) {
                        int sx = tileSwizzle[i] % 8;
                        int sy = (tileSwizzle[i] - sx) / 8;
                        int index = (srcHeight - (y + sy) - 1) * srcWidth + (x + sx);

                        *dst++ = swap32(src[index]);
                    }
                }
                dst += (dstWidth - srcWidth) * 8;
            }
        }

        Texture(int width, int height, int depth, uint32 opt) : width(width), height(height), origWidth(width), origHeight(height), fmt(FMT_RGBA), opt(opt) { opt |= OPT_NEAREST; }

        void init(void *data) {
            ASSERT((opt & OPT_PROXY) == 0);
            
            FormatDesc desc = formats[fmt];

            if (width < 8 || height < 8) {
                LOG("texture too small %dx%d [%d %d]!\n", width, height, fmt, opt);
                width  = 8;
                height = 8;
                data   = NULL;
            }

            if (width > 1024 || height > 1024) {
                LOG("texture too large %dx%d [%d %d]!\n", width, height, fmt, opt);
                width  = 8;
                height = 8;
                data   = NULL;
            }

            int size = width * height * desc.bpp / 8;

            bool isCube   = (opt & OPT_CUBEMAP) != 0;
            bool isShadow = fmt == FMT_SHADOW;

            C3D_TexInitParams params;
            memset(&params, 0, sizeof(params));
            params.width    = width;
            params.height   = height;
            params.maxLevel = ((opt & OPT_MIPMAPS) != 0) ? min(3, C3D_TexCalcMaxLevel(width, height)) : 0;
            params.format   = desc.format;
            params.onVram   = false;

            if (isCube && isShadow)
                params.type = GPU_TEX_SHADOW_CUBE;
            else if (isCube)
                params.type = GPU_TEX_CUBE_MAP;
            else if (isShadow)
                params.type = GPU_TEX_SHADOW_2D;
            else
                params.type = GPU_TEX_2D;

            C3D_TexInitWithParams(&tex, &texCube, params);

            if (data && !isCube) {
                update(data);
            }

            GPU_TEXTURE_FILTER_PARAM filter = ((opt & OPT_NEAREST) == 0) ? GPU_LINEAR : GPU_NEAREST;
            C3D_TexSetFilter(&tex, filter, filter);
            C3D_TexSetFilterMipmap(&tex, filter);
        }

        void deinit() {
            C3D_TexDelete(&tex);
        }

        void generateMipMap() {
            C3D_TexGenerateMipmap(&tex, GPU_TEXFACE_2D);
        }

        void update(void *data) {
            if (!data) return;
            FormatDesc desc = formats[fmt];
            convertImage((uint32*)tex.data, (uint32*)data, width, height, origWidth, origHeight, desc.bpp);
            C3D_TexFlush(&tex);
        }

        void bind(int sampler) {
            if (opt & OPT_PROXY) return;

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
            ASSERT(sizeof(GAPI::Vertex) == sizeof(::Vertex));
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

            VAO = new C3D_BufInfo[aCount + 1];

            iBuffer = (Index*)  linearAlloc(iCount * sizeof(Index));
            vBuffer = (Vertex*) linearAlloc(vCount * sizeof(Vertex));

            if (!dynamic) {
                update(indices, iCount, vertices, vCount);
            }
        }

        void deinit() {
            delete[] VAO;
            linearFree(iBuffer);
            linearFree(vBuffer);
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
                memcpy(iBuffer + chunk.iStart, indices, iCount * sizeof(Index));
                GSPGPU_FlushDataCache(iBuffer + chunk.iStart, iCount * sizeof(Index));
            }

            if (vertices && vCount) {
                chunk.vStart = chunk.vCount;
                chunk.vCount += vCount;
                memcpy(vBuffer + chunk.vStart, vertices, vCount * sizeof(Vertex));
                GSPGPU_FlushDataCache(vBuffer + chunk.vStart, vCount * sizeof(Vertex));
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
            C3D_BufInfo *vao = VAO;

            if (range.aIndex == -1) {
                vao += aCount - 1;
                initVAO(vao, vBuffer + range.vStart + getChunk().vStart);
            // workaround for passing "info != &ctx->bufInfo" check inside C3D_SetBufInfo for the same VAO pointers
                C3D_BufInfo dummyBufInfo;
                C3D_SetBufInfo(&dummyBufInfo);
            } else {
                vao += range.aIndex;
                if (Core::active.VAO == vao) {
                    return;
                }
            }
            
            C3D_SetBufInfo(vao);
            Core::active.VAO = vao;
        }
    };


    bool depthTest;
    uint32 colorMask, depthMask;

    Shader clearShader;
    Mesh   clearMesh(false);
    uint32 clearColor;

    C3D_RenderTarget *curTarget;
    C3D_RenderTarget *defTarget[2];
    C3D_AttrInfo      vertexAttribs;

    void init() {
        LOG("Vendor   : %s\n", "Nintendo");
        LOG("Renderer : %s\n", "PICA200 citro3D");
        LOG("Version  : %s\n", "1.0");

        gfxInitDefault();
        gfxSet3D(true);
        consoleInit(GFX_BOTTOM, NULL);

        C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
        defTarget[0] = C3D_RenderTargetCreate(240 * 2, 400 * 2, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
        defTarget[1] = C3D_RenderTargetCreate(240 * 2, 400 * 2, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
        C3D_RenderTargetSetOutput(defTarget[0], GFX_TOP, GFX_LEFT,  DISPLAY_TRANSFER_FLAGS);
        C3D_RenderTargetSetOutput(defTarget[1], GFX_TOP, GFX_RIGHT, DISPLAY_TRANSFER_FLAGS);
        curTarget = defTarget[0];

        support.shaderBinary   = true;
        support.VAO            = true;
        support.texMinSize     = 8;

        Core::width  = 400;
        Core::height = 240;

    // init default vertex declaration
        AttrInfo_Init(&vertexAttribs);
        AttrInfo_AddLoader(&vertexAttribs, aCoord    , GPU_SHORT         , 4);
        AttrInfo_AddLoader(&vertexAttribs, aNormal   , GPU_SHORT         , 4);
        AttrInfo_AddLoader(&vertexAttribs, aTexCoord , GPU_SHORT         , 4);
        AttrInfo_AddLoader(&vertexAttribs, aColor    , GPU_UNSIGNED_BYTE , 4);
        AttrInfo_AddLoader(&vertexAttribs, aLight    , GPU_UNSIGNED_BYTE , 4);
        
    // default texture env params
        C3D_TexEnv* env = C3D_GetTexEnv(0);
        C3D_TexEnvInit(env);
        C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
        C3D_TexEnvFunc(env, C3D_Both, GPU_MODULATE);
        C3D_TexEnvScale(env, C3D_RGB, GPU_TEVSCALE_2);

        //C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR);
        //C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);

        clearColor = 0x68B0D8FF;
        colorMask  = GPU_WRITE_COLOR;
        depthMask  = GPU_WRITE_DEPTH;

    // init shaders
        compose_dvlb        = DVLB_ParseFile((u32*)compose_shbin,        compose_shbin_size);
        filter_upscale_dvlb = DVLB_ParseFile((u32*)filter_upscale_shbin, filter_upscale_shbin_size);
        gui_dvlb            = DVLB_ParseFile((u32*)gui_shbin,            gui_shbin_size);
    }

    void deinit() {
        DVLB_Free(compose_dvlb);
        DVLB_Free(filter_upscale_dvlb);
        DVLB_Free(gui_dvlb);

        C3D_Fini();
        gfxExit();
    }
    
    mat4 ortho(float l, float r, float b, float t, float znear, float zfar) {
        mat4 m;
        Mtx_OrthoTilt((C3D_Mtx*)&m, l, r, b, t, znear, zfar, false);

        mat4 res;
        res.e00 = m.e30; res.e10 = m.e31; res.e20 = m.e32; res.e30 = m.e33;
        res.e01 = m.e20; res.e11 = m.e21; res.e21 = m.e22; res.e31 = m.e23;
        res.e02 = m.e10; res.e12 = m.e11; res.e22 = m.e12; res.e32 = m.e13;
        res.e03 = m.e00; res.e13 = m.e01; res.e23 = m.e02; res.e33 = m.e03;
        return res;
    }

    mat4 perspective(float fov, float aspect, float znear, float zfar) {
        mat4 m;
        Mtx_PerspTilt((C3D_Mtx*)&m, fov * DEG2RAD, aspect, znear, zfar, false);

        mat4 res;
        res.e00 = m.e30; res.e10 = m.e31; res.e20 = m.e32; res.e30 = m.e33;
        res.e01 = m.e20; res.e11 = m.e21; res.e21 = m.e22; res.e31 = m.e23;
        res.e02 = m.e10; res.e12 = m.e11; res.e22 = m.e12; res.e32 = m.e13;
        res.e03 = m.e00; res.e13 = m.e01; res.e23 = m.e02; res.e33 = m.e03;
        return res;
    }

    bool beginFrame() {
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        curTarget = Core::eye <= 0.0f ? defTarget[0] : defTarget[1];
        C3D_FrameDrawOn(curTarget);

        return true;
    }

    void endFrame() {
        C3D_FrameEnd(0);
    }

    void resetState() {
        C3D_SetAttrInfo(&vertexAttribs);
    }

    int cacheRenderTarget(bool depth, int width, int height) {
        /*
        RenderTargetCache &cache = rtCache[depth];

        for (int i = 0; i < cache.count; i++)
            if (cache.items[i].width == width && cache.items[i].height == height)
                return i;

        ASSERT(cache.count < MAX_RENDER_BUFFERS);

        RenderTargetCache::Item &item = cache.items[cache.count];
        item.width  = width;
        item.height = height;

        if (depth)
            device->CreateDepthStencilSurface(width, height, D3DFMT_D16, D3DMULTISAMPLE_NONE, 0, true, &item.surface, NULL);
        else
            device->CreateRenderTarget(width, height, D3DFMT_R5G6B5, D3DMULTISAMPLE_NONE, 0, false, &item.surface, NULL);

        return cache.count++;
        */
        return 0;
    }

    void bindTarget(Texture *target, int face) {
        active.viewport = Viewport(0, 0, 0, 0); // forcing viewport reset
        //curTarget = Core::eye <= 0.0f ? defTarget[0] : defTarget[1];
        //C3D_FrameDrawOn(curTarget);
    }

    void discardTarget(bool color, bool depth) {}

    void copyTarget(Texture *dst, int xOffset, int yOffset, int x, int y, int width, int height) {
        //
    }

    void setVSync(bool enable) {}

    void present() {

    }

    void setViewport(const Viewport &vp) {
        //int vh = active.target ? active.target->height : Core::height;

    }

    void updateWriteMask() {
        C3D_DepthTest(depthTest, GPU_GREATER, GPU_WRITEMASK(colorMask | depthMask));
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

    void clear(bool color, bool depth) {
        uint32 mask = 0;
        if (color) mask |= C3D_CLEAR_COLOR;
        if (depth) mask |= C3D_CLEAR_DEPTH;
        if (!mask) return;

        C3D_RenderTargetClear(curTarget, C3D_ClearBits(mask), clearColor, 0);
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

    void initPSO(PSO *pso) {
        ASSERT(pso);
        ASSERT(pso && pso->data == NULL);
        pso->data = &pso;
    }

    void deinitPSO(PSO *pso) {
        ASSERT(pso);
        ASSERT(pso->data != NULL);
        pso->data = NULL;
    }

    void bindPSO(const PSO *pso) {
        //
    }
}

#endif