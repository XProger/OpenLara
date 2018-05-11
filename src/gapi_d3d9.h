#ifndef H_GAPI_D3D9
#define H_GAPI_D3D9

#include "core.h"
#include <d3d9.h>

#define PROFILE_MARKER(title)
#define PROFILE_LABEL(id, name, label)
#define PROFILE_TIMING(time)
    
extern LPDIRECT3D9       D3D;
extern LPDIRECT3DDEVICE9 device;

namespace GAPI {
    #include "shaders/base_vs.h"
    #include "shaders/base_ps.h"
    #include "shaders/water_vs.h"
    #include "shaders/water_ps.h"
    #include "shaders/filter_vs.h"
    #include "shaders/filter_ps.h"
    #include "shaders/gui_vs.h"
    #include "shaders/gui_ps.h"

    using namespace Core;

    typedef ::Vertex Vertex;

    int cullMode, blendMode;
    uint32 clearColor;

    LPDIRECT3DVERTEXDECLARATION9 vertexDecl;

// Shader
    enum {
        USAGE_VS,
        USAGE_PS,
    };

    static const struct Binding {
        int reg;
        int usage;
    } bindings[uMAX] = {
        {   1, USAGE_VS | USAGE_PS }, // uParam
        {   2, USAGE_VS | USAGE_PS }, // uTexParam
        {   3, USAGE_VS | USAGE_PS }, // uViewProj
        {   7, USAGE_VS | USAGE_PS }, // uBasis
        {  71, USAGE_VS | USAGE_PS }, // uLightProj
        { 103, USAGE_VS | USAGE_PS }, // uMaterial
        { 104, USAGE_VS | USAGE_PS }, // uAmbient
        { 110, USAGE_VS | USAGE_PS }, // uFogParams
        { 111, USAGE_VS | USAGE_PS }, // uViewPos
        { 112, USAGE_VS | USAGE_PS }, // uLightPos
        { 116, USAGE_VS | USAGE_PS }, // uLightColor
        { 120, USAGE_VS | USAGE_PS }, // uRoomSize
        { 121, USAGE_VS | USAGE_PS }, // uPosScale
        { 123, USAGE_VS | USAGE_PS }, // uContacts
    };

    struct Shader {
        LPDIRECT3DVERTEXSHADER9 VS;
        LPDIRECT3DPIXELSHADER9  PS;

        Shader() : VS(NULL), PS(NULL) {}

        void init(Core::Pass pass, int *def, int defCount) {
            const BYTE *vSrc, *pSrc;
            switch (pass) {
                case Core::passCompose :
                case Core::passShadow  :
                case Core::passAmbient : vSrc = BASE_VS;   pSrc = BASE_PS;   break;
                case Core::passWater   : vSrc = WATER_VS;  pSrc = WATER_PS;  break;
                case Core::passFilter  : vSrc = FILTER_VS; pSrc = FILTER_PS; break;
                case Core::passGUI     : vSrc = GUI_VS;    pSrc = GUI_PS;    break;
                default                : ASSERT(false); LOG("! wrong pass id\n"); return;
            }

            device->CreateVertexShader ((DWORD*)vSrc, &VS);
            device->CreatePixelShader  ((DWORD*)pSrc, &PS);
        }

        void deinit() {
            if (VS) VS->Release();
            if (PS) PS->Release();
        }

        void bind() {
            if (Core::active.shader != this) {
                Core::active.shader = this;
                device->SetVertexShader(VS);
                device->SetPixelShader(PS);
            }
        }

        void setConstant(UniformType uType, const float *value, int vectors) {
            const Binding &b = bindings[uType];
            if (b.usage | USAGE_VS) device->SetVertexShaderConstantF (b.reg, value, vectors);
            if (b.usage | USAGE_PS) device->SetPixelShaderConstantF  (b.reg, value, vectors);
        }

        void setParam(UniformType uType, const vec4 &value, int count = 1) {
            setConstant(uType, (float*)&value, count);
        }

        void setParam(UniformType uType, const mat4 &value, int count = 1) {
            setConstant(uType, (float*)&value, count * 4);
        }

        void setParam(UniformType uType, const Basis &value, int count = 1) {
            setConstant(uType, (float*)&value, count * 2);
        }
    };

// Texture
    struct Texture {
        LPDIRECT3DTEXTURE9     tex2D;
        LPDIRECT3DCUBETEXTURE9 texCube;

        int       width, height, origWidth, origHeight;
        TexFormat fmt;
        uint32    opt;

        Texture(int width, int height, uint32 opt) : tex2D(NULL), texCube(NULL), width(width), height(height), origWidth(width), origHeight(height), fmt(FMT_RGBA), opt(opt) {}

        void init(void *data) {
            ASSERT((opt & OPT_PROXY) == 0);

            bool filter   = (opt & OPT_NEAREST) == 0;
            bool mipmaps  = (opt & OPT_MIPMAPS) != 0;
            bool cube     = (opt & OPT_CUBEMAP) != 0;
            bool isShadow = fmt == FMT_SHADOW;

            static const struct FormatDesc {
                int       bpp;
                D3DFORMAT format;
            } formats[FMT_MAX] = {
                {  8, D3DFMT_L8           },
                { 32, D3DFMT_A8R8G8B8     },
                { 16, D3DFMT_R5G6B5       },
                { 16, D3DFMT_A1R5G5B5     },
                { 64, D3DFMT_A16B16G16R16 },
                { 64, D3DFMT_A16B16G16R16 },
                { 16, D3DFMT_D16          },
                { 32, D3DFMT_D24S8        },
                { 16, D3DFMT_D16          },
            };

            FormatDesc desc = formats[fmt];

            if (cube) {
                device->CreateCubeTexture(width, 1, 0, desc.format, D3DPOOL_MANAGED, &texCube, NULL);
            } else {
                device->CreateTexture(width, height, 1, 0, desc.format, D3DPOOL_MANAGED, &tex2D, NULL);
                if (data) {
                    D3DLOCKED_RECT rect;
                    tex2D->LockRect(0, &rect, NULL, 0);
                    memcpy(rect.pBits, data, width * height * (desc.bpp / 8));
                    tex2D->UnlockRect(0);
                }
            }
        }

        void deinit() {
            if (tex2D)   tex2D->Release();
            if (texCube) texCube->Release();
        }

        void generateMipMap() {
            // TODO
        }

        void bind(int sampler) {
            if (opt & OPT_PROXY) return;
            ASSERT(tex2D || texCube);

            if (Core::active.textures[sampler] != this) {
                Core::active.textures[sampler] = this;
                if (tex2D)
                    device->SetTexture(sampler, tex2D);
                else if (texCube)
                    device->SetTexture(sampler, texCube);
            }
        }

        void unbind(int sampler) {
            if (Core::active.textures[sampler]) {
                Core::active.textures[sampler] = NULL;
                device->SetTexture(sampler, NULL);
            }
        }

        void setFilterQuality(int value) {
            bool filter  = value > Core::Settings::LOW;
            bool mipmaps = value > Core::Settings::MEDIUM;

            for (int i = 0; i < sMAX; i++) {
                device->SetSamplerState(i, D3DSAMP_ADDRESSU,  D3DTADDRESS_CLAMP);
                device->SetSamplerState(i, D3DSAMP_ADDRESSV,  D3DTADDRESS_CLAMP);
                device->SetSamplerState(i, D3DSAMP_ADDRESSW,  D3DTADDRESS_CLAMP);
                device->SetSamplerState(i, D3DSAMP_MAGFILTER, filter ? D3DTEXF_LINEAR : D3DTEXF_POINT);
                device->SetSamplerState(i, D3DSAMP_MINFILTER, filter ? D3DTEXF_LINEAR : D3DTEXF_POINT);
            }
        }
    };

// Mesh
    struct Mesh {
        LPDIRECT3DINDEXBUFFER9  IB;
        LPDIRECT3DVERTEXBUFFER9 VB;

        int  iCount;
        int  vCount;
        bool dynamic;

        Mesh(bool dynamic) : IB(NULL), VB(NULL), dynamic(dynamic) {}

        void init(Index *indices, int iCount, ::Vertex *vertices, int vCount, int aCount) {
            this->iCount = iCount;
            this->vCount = vCount;

            ASSERT(sizeof(GAPI::Vertex) == sizeof(::Vertex));

            device->CreateIndexBuffer  (iCount * sizeof(indices[0]),  D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_MANAGED, &IB, NULL);
            device->CreateVertexBuffer (vCount * sizeof(vertices[0]), D3DUSAGE_WRITEONLY,              0, D3DPOOL_MANAGED, &VB, NULL);
            update(indices, iCount, vertices, vCount);
        }

        void deinit() {
            IB->Release();
            VB->Release();
        }

        void update(Index *indices, int iCount, ::Vertex *vertices, int vCount) {
            ASSERT(sizeof(GAPI::Vertex) == sizeof(::Vertex));

            void* ptr;
            int size;

            if (indices && iCount) {
                IB->Lock(0, size = iCount * sizeof(indices[0]), &ptr, 0);
                memcpy(ptr, indices, size);
                IB->Unlock();
            }

            if (vertices && vCount) {
                VB->Lock(0, size = vCount * sizeof(vertices[0]), &ptr, 0);
                memcpy(ptr, vertices, size);
                VB->Unlock();
            }
        }

        void bind(const MeshRange &range) const {
            device->SetIndices(IB);
            device->SetStreamSource(0, VB, 0, sizeof(Vertex));
        }

        void initNextRange(MeshRange &range, int &aIndex) const {
            range.aIndex = -1;
        }
    };

//    GLuint FBO, defaultFBO;
    struct RenderTargetCache {
        int count;
        struct Item {
//            GLuint  ID;
            int     width;
            int     height;
        } items[MAX_RENDER_BUFFERS];
    } rtCache[2];

    void init() {
        memset(rtCache, 0, sizeof(rtCache));
        
        D3DADAPTER_IDENTIFIER9 adapterInfo;
        D3D->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &adapterInfo);
        LOG("Vendor   : %s\n", adapterInfo.Description);
        LOG("Renderer : Direct3D 9.0c\n");

        support.maxAniso       = 16;
        support.maxVectors     = 16;
        support.shaderBinary   = false;
        support.VAO            = false;
        support.depthTexture   = false;
        support.shadowSampler  = false;
        support.discardFrame   = false;
        support.texNPOT        = false;
        support.texRG          = false;
        support.texBorder      = false;
        support.colorFloat     = false;
        support.colorHalf      = false;
        support.texFloatLinear = false;
        support.texFloat       = false;
        support.texHalfLinear  = false;
        support.texHalf        = false;

        #ifdef PROFILE
            support.profMarker = false;
            support.profTiming = false;
        #endif

        const D3DVERTEXELEMENT9 VERTEX_DECL[] = {
            {0, 0,  D3DDECLTYPE_SHORT4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0}, // aCoord
            {0, 8,  D3DDECLTYPE_SHORT4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,   0}, // aNormal
            {0, 16, D3DDECLTYPE_SHORT4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0}, // aTexCoord
            {0, 24, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,    0}, // aColor
            {0, 28, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,    1}, // aLight
            D3DDECL_END()
        };

        device->CreateVertexDeclaration(VERTEX_DECL, &vertexDecl);
/*
        if (support.maxAniso)
            glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &support.maxAniso);
        #ifdef _GAPI_GLES
            glGetIntegerv(GL_MAX_VARYING_VECTORS, &support.maxVectors);
        #else
            support.maxVectors = 16;
        #endif
        glEnable(GL_SCISSOR_TEST);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&defaultFBO);
        glGenFramebuffers(1, &FBO);
*/
    }

    void deinit() {
        /*
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &FBO);

        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        for (int b = 0; b < 2; b++)
            for (int i = 0; i < rtCache[b].count; i++)
                glDeleteRenderbuffers(1, &rtCache[b].items[i].ID);
                */
    }

    void beginFrame() {
        device->BeginScene();
    }

    void endFrame() {
        device->EndScene();
    }

    void resetState() {
        device->SetVertexDeclaration(vertexDecl);
        device->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
        device->SetRenderState(D3DRS_LIGHTING, FALSE);
        device->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
    }

    int cacheRenderTarget(bool depth, int width, int height) {
        RenderTargetCache &cache = rtCache[depth];

        for (int i = 0; i < cache.count; i++)
            if (cache.items[i].width == width && cache.items[i].height == height)
                return i;

        ASSERT(cache.count < MAX_RENDER_BUFFERS);

        RenderTargetCache::Item &item = cache.items[cache.count];
        item.width  = width;
        item.height = height;
        /*
        glGenRenderbuffers(1, &item.ID);
        glBindRenderbuffer(GL_RENDERBUFFER, item.ID);
        glRenderbufferStorage(GL_RENDERBUFFER, depth ? GL_RGB565 : GL_DEPTH_COMPONENT16, width, height);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        */
        return cache.count++;
    }

    void bindTarget(Texture *target, int face) {
        /*
        if (!target) { // may be a null
            glBindFramebuffer(GL_FRAMEBUFFER, defaultFBO);
        } else {
            GLenum texTarget = GL_TEXTURE_2D;
            if (target->opt & OPT_CUBEMAP) 
                texTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X + face;

            bool depth = target->fmt == FMT_DEPTH || target->fmt == FMT_SHADOW;

            int rtIndex = cacheRenderTarget(depth, target->width, target->height);

            glBindFramebuffer(GL_FRAMEBUFFER, FBO);
            glFramebufferTexture2D    (GL_FRAMEBUFFER, depth ? GL_DEPTH_ATTACHMENT  : GL_COLOR_ATTACHMENT0, texTarget,       target->ID, 0);
            glFramebufferRenderbuffer (GL_FRAMEBUFFER, depth ? GL_COLOR_ATTACHMENT0 : GL_DEPTH_ATTACHMENT,  GL_RENDERBUFFER, rtCache[depth].items[rtIndex].ID);
        }
        */
    }

    void discardTarget(bool color, bool depth) {}

    void copyTarget(Texture *dst, int xOffset, int yOffset, int x, int y, int width, int height) {
        Core::active.textures[0] = NULL;
        /*
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, dst->ID);
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, xOffset, yOffset, x, y, width, height);
        glBindTexture(GL_TEXTURE_2D, 0);
        */
    }

    void setVSync(bool enable) {
        //
    }

    void waitVBlank() {}

    void clear(bool color, bool depth) {
        uint32 flags = (color ? D3DCLEAR_TARGET : 0) | (depth ? D3DCLEAR_ZBUFFER : 0);
        device->Clear(0, NULL, flags, clearColor, 1.0f, 0);
    }

    void setClearColor(const vec4 &color) {
        clearColor = (int(color.x * 255) << 16) |
                     (int(color.y * 255) << 8 ) |
                     (int(color.z * 255)      ) |
                     (int(color.w * 255) << 24);
    }

    void setViewport(const Viewport &vp) {
        D3DVIEWPORT9 dv;
        dv.X      = vp.x;
        dv.Y      = vp.y;
        dv.Width  = vp.width;
        dv.Height = vp.height;
        dv.MinZ   = 0.0f;
        dv.MaxZ   = 1.0f;

        RECT ds;
        ds.left   = vp.x;
        ds.top    = vp.y;
        ds.right  = vp.x + vp.width;
        ds.bottom = vp.y + vp.height;

        device->SetViewport(&dv);
        device->SetScissorRect(&ds);
    }

    void setDepthTest(bool enable) {
        device->SetRenderState(D3DRS_ZENABLE, enable);
    }

    void setDepthWrite(bool enable) {
        device->SetRenderState(D3DRS_ZWRITEENABLE, enable);
    }

    void setColorWrite(bool r, bool g, bool b, bool a) {
        device->SetRenderState(D3DRS_COLORWRITEENABLE,
            (r ? D3DCOLORWRITEENABLE_RED   : 0) |
            (g ? D3DCOLORWRITEENABLE_GREEN : 0) |
            (b ? D3DCOLORWRITEENABLE_BLUE  : 0) |
            (a ? D3DCOLORWRITEENABLE_ALPHA : 0));
    }

    void setAlphaTest(bool enable) {}

    void setCullMode(int rsMask) {
        cullMode = rsMask;
        switch (rsMask) {
            case RS_CULL_BACK  : device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);  break;
            case RS_CULL_FRONT : device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW); break;
            default            : device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        }
    }

    void setBlendMode(int rsMask) {
        blendMode = rsMask;
        switch (rsMask) {
            case RS_BLEND_ALPHA   : 
                device->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA);
                device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
                break;
            case RS_BLEND_ADD     :
                device->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_ONE);
                device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
                break;
            case RS_BLEND_MULT    :
                device->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_DESTCOLOR);
                device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
                break;
            case RS_BLEND_PREMULT :
                device->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_ONE);
                device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
                break;
            default               :
                device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
                return;
        }
        device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    }

    void setViewProj(const mat4 &mView, const mat4 &mProj) {
    }

    void DIP(Mesh *mesh, const MeshRange &range) {
        device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, range.vStart, 0, mesh->vCount, range.iStart, range.iCount / 3);
    }

    vec4 copyPixel(int x, int y) {
        ubyte4 c;
//        glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &c);
        return vec4(float(c.x), float(c.y), float(c.z), float(c.w)) * (1.0f / 255.0f);
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