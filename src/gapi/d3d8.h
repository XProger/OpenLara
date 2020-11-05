#ifndef H_GAPI_D3D8
#define H_GAPI_D3D8

#include "core.h"

#define PROFILE_MARKER(title)
#define PROFILE_LABEL(id, name, label)
#define PROFILE_TIMING(time)

extern LPDIRECT3D8           D3D;
extern LPDIRECT3DDEVICE8     device;
extern D3DPRESENT_PARAMETERS d3dpp;

#ifdef _DEBUG
void D3DCHECK(HRESULT res) {
    if (!FAILED(res)) return;

    LOG("! ");
    switch (res) {
        case D3DERR_WRONGTEXTUREFORMAT        : LOG("D3DERR_WRONGTEXTUREFORMAT");        break;
        case D3DERR_UNSUPPORTEDCOLOROPERATION : LOG("D3DERR_UNSUPPORTEDCOLOROPERATION"); break;
        case D3DERR_UNSUPPORTEDCOLORARG       : LOG("D3DERR_UNSUPPORTEDCOLORARG");       break;
        case D3DERR_UNSUPPORTEDALPHAOPERATION : LOG("D3DERR_UNSUPPORTEDALPHAOPERATION"); break;
        case D3DERR_UNSUPPORTEDALPHAARG       : LOG("D3DERR_UNSUPPORTEDALPHAARG");       break;
        case D3DERR_TOOMANYOPERATIONS         : LOG("D3DERR_TOOMANYOPERATIONS");         break;
        case D3DERR_CONFLICTINGTEXTUREFILTER  : LOG("D3DERR_CONFLICTINGTEXTUREFILTER");  break;
        case D3DERR_UNSUPPORTEDFACTORVALUE    : LOG("D3DERR_UNSUPPORTEDFACTORVALUE");    break;
        case D3DERR_CONFLICTINGRENDERSTATE    : LOG("D3DERR_CONFLICTINGRENDERSTATE");    break;
        case D3DERR_UNSUPPORTEDTEXTUREFILTER  : LOG("D3DERR_UNSUPPORTEDTEXTUREFILTER");  break;
        case D3DERR_CONFLICTINGTEXTUREPALETTE : LOG("D3DERR_CONFLICTINGTEXTUREPALETTE"); break;
        case D3DERR_DRIVERINTERNALERROR       : LOG("D3DERR_DRIVERINTERNALERROR");       break;
        case D3DERR_NOTFOUND                  : LOG("D3DERR_NOTFOUND");                  break;
        case D3DERR_MOREDATA                  : LOG("D3DERR_MOREDATA");                  break;
        case D3DERR_DEVICELOST                : LOG("D3DERR_DEVICELOST");                break;
        case D3DERR_DEVICENOTRESET            : LOG("D3DERR_DEVICENOTRESET");            break;
        case D3DERR_NOTAVAILABLE              : LOG("D3DERR_NOTAVAILABLE");              break;
        case D3DERR_OUTOFVIDEOMEMORY          : LOG("D3DERR_OUTOFVIDEOMEMORY");          break;
        case D3DERR_INVALIDDEVICE             : LOG("D3DERR_INVALIDDEVICE");             break;
        case D3DERR_INVALIDCALL               : LOG("D3DERR_INVALIDCALL");               break;
        default                               : LOG("D3DERR_UNKNOWN");                   break;
    }
    LOG("\n");
    ASSERT(false);
}
#else
    #define D3DCHECK(res) res
#endif

namespace GAPI {
    using namespace Core;

    typedef ::Vertex Vertex;

    int cullMode, blendMode;
    uint32 clearColor;
    bool isFrontCW;

    LPDIRECT3DSURFACE8 defRT, defDS;

    const DWORD vertexDecl[] =
    {
        D3DVSD_STREAM(0),
        D3DVSD_REG( aCoord,    D3DVSDT_SHORT4     ),
        D3DVSD_REG( aNormal,   D3DVSDT_NORMSHORT4 ),
        D3DVSD_REG( aTexCoord, D3DVSDT_NORMSHORT4 ),
        D3DVSD_REG( aColor,    D3DVSDT_PBYTE4     ),
        D3DVSD_REG( aLight,    D3DVSDT_PBYTE4     ),
        D3DVSD_END()
    };

    struct Texture;
    struct Mesh;

    struct Resource {
        Texture *texture;
        Mesh    *mesh;
    } resList[256];
    int resCount;

    void registerResource(Mesh *mesh) {
        resList[resCount].mesh    = mesh;
        resList[resCount].texture = NULL;
        resCount++;
    }

    void registerResource(Texture *texture) {
        resList[resCount].mesh    = NULL;
        resList[resCount].texture = texture;
        resCount++;
    }

    void unregisterResource(void *res) {
        for (int i = 0; i < resCount; i++)
            if (resList[i].mesh == res || resList[i].texture == res) {
                resList[i] = resList[--resCount];
                break;
            }
    }

// Shader
    #include "shaders/d3d8/shaders.h"

    enum {
        USAGE_VS,
        USAGE_PS,
    };

    static const struct Binding {
        int reg;
        int usage;
    } bindings[uMAX] = {
        {   0, USAGE_VS | USAGE_PS }, // uParam
        {   1, USAGE_VS | USAGE_PS }, // uTexParam
        {   2, USAGE_VS | USAGE_PS }, // uViewProj
        {   6, USAGE_VS | USAGE_PS }, // uBasis
        {  70, USAGE_VS | USAGE_PS }, // uLightProj
        {  74, USAGE_VS | USAGE_PS }, // uMaterial
        {  75, USAGE_VS | USAGE_PS }, // uAmbient
        {  81, USAGE_VS | USAGE_PS }, // uFogParams
        {  82, USAGE_VS | USAGE_PS }, // uViewPos
        {  83, USAGE_VS | USAGE_PS }, // uLightPos
        {  87, USAGE_VS | USAGE_PS }, // uLightColor
        {  91, USAGE_VS | USAGE_PS }, // uRoomSize
        {  92, USAGE_VS | USAGE_PS }, // uPosScale
        {  98, USAGE_VS | USAGE_PS }, // uContacts
    };

    struct Shader {
        DWORD VS;
        DWORD PS;

        Shader() : VS(NULL), PS(NULL) {}

        void init(Core::Pass pass, int type, int *def, int defCount) {
            bool underwater = false;

            for (int i = 0; i < defCount; i++) {
                if (def[i] == SD_UNDERWATER) {
                    underwater = true;
                }
            }
 
            #define SHADER(S,P)    S##_##P
            #define SHADER_U(S,P)  (underwater ? SHADER(S##_u,P) : SHADER(S,P))

            const uint8 *vSrc, *fSrc;
            switch (pass) {
                case passCompose :
                    switch (type) {
                        case 0  : vSrc = SHADER_U ( compose_sprite, v );  fSrc = SHADER_U ( compose_sprite, f ); break;
                        case 1  : vSrc = SHADER   ( compose_flash,  v );  fSrc = SHADER   ( compose_flash,  f ); break;
                        case 2  : vSrc = SHADER_U ( compose_room,   v );  fSrc = SHADER_U ( compose_room,   f ); break;
                        case 3  : vSrc = SHADER_U ( compose_entity, v );  fSrc = SHADER_U ( compose_entity, f ); break;
                        case 4  : vSrc = SHADER   ( compose_mirror, v );  fSrc = SHADER   ( compose_mirror, f ); break;
                        default : ASSERT(false);
                    }
                    break;
                case passShadow : 
                    switch (type) {
                        case 3  :
                        case 4  : vSrc = SHADER ( shadow_entity, v );  fSrc = SHADER ( shadow_entity, f ); break;
                        default : ASSERT(false);
                    }
                    break;
                case passAmbient :
                    switch (type) {
                        case 0  : vSrc = SHADER ( ambient_sprite, v );  fSrc = SHADER ( ambient_sprite, f ); break;
                        case 1  : vSrc = SHADER ( ambient_room,   v );  fSrc = SHADER ( ambient_room,   f ); break; // TYPE_FLASH (sky)
                        case 2  : vSrc = SHADER ( ambient_room,   v );  fSrc = SHADER ( ambient_room,   f ); break;
                        default : ASSERT(false);
                    }
                    break;
                case passSky   : vSrc = SHADER ( gui, v );  fSrc = SHADER ( gui, f ); break; // TODO
            /*
                case passWater : 
                    switch (type) {
                        case 0  : vSrc = SHADER ( water_drop,     v );  fSrc = SHADER ( water_drop,     f ); break;
                        case 1  : vSrc = SHADER ( water_simulate, v );  fSrc = SHADER ( water_simulate, f ); break;
                        case 2  : vSrc = SHADER ( water_caustics, v );  fSrc = SHADER ( water_caustics, f ); break;
                        case 3  : vSrc = SHADER ( water_rays,     v );  fSrc = SHADER ( water_rays,     f ); break;
                        case 4  : vSrc = SHADER ( water_mask,     v );  fSrc = SHADER ( water_mask,     f ); break;
                        case 5  : vSrc = SHADER ( water_compose,  v );  fSrc = SHADER ( water_compose,  f ); break;
                        default : ASSERT(false);
                    }
                    break;
            */
                case passFilter :
                    switch (type) {
                        case 0  : vSrc = SHADER ( filter_upscale,    v );  fSrc = SHADER ( filter_upscale,    f ); break;
                        case 1  : vSrc = SHADER ( filter_downsample, v );  fSrc = SHADER ( filter_downsample, f ); break;
                        case 3  : vSrc = SHADER ( filter_grayscale,  v );  fSrc = SHADER ( filter_grayscale,  f ); break;
                        case 4  : vSrc = SHADER ( filter_blur,       v );  fSrc = SHADER ( filter_blur,       f ); break;
                        case 5  : vSrc = SHADER ( filter_blur,       v );  fSrc = SHADER ( filter_blur,       f ); break; // TODO anaglyph
                        default : ASSERT(false);
                    }
                    break;
                case passGUI    : vSrc = SHADER ( gui, v );  fSrc = SHADER ( gui, f ); break;
                default         : ASSERT(false); LOG("! wrong pass id\n"); return;
            }

            #undef SHADER
            #undef SHADER_U

            device->CreateVertexShader(vertexDecl, (DWORD*)vSrc, &VS, 0);
            device->CreatePixelShader(&((D3DPIXELSHADERDEF_FILE*)fSrc)->Psd, &PS);
        }

        void deinit() {
            device->SetVertexShader(0);
            device->SetPixelShader(0);
            if (VS) device->DeleteVertexShader(VS);
            if (PS) device->DeletePixelShader(PS);
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
            if (b.usage | USAGE_VS) device->SetVertexShaderConstant (b.reg, value, vectors);
        }

        void setParam(UniformType uType, const vec4 &value, int count = 1) {
            setConstant(uType, (float*)&value, count);
        }

        void setParam(UniformType uType, const mat4 &value, int count = 1) {
            setConstant(uType, (float*)&value, count * 4);
        }
    };

// Texture
    struct Texture {
        LPDIRECT3DTEXTURE8     tex2D;
        LPDIRECT3DCUBETEXTURE8 texCube;

        int       width, height, depth, origWidth, origHeight, origDepth;
        TexFormat fmt;
        uint32    opt;
        D3DFORMAT d3dformat;

        Texture(int width, int height, int depth, uint32 opt) : tex2D(NULL), texCube(NULL), width(width), height(height), depth(depth), origWidth(width), origHeight(height), origDepth(depth), fmt(FMT_RGBA), opt(opt) {}

        void init(void *data) {
            ASSERT((opt & OPT_PROXY) == 0);

            if (origWidth < 8 || origHeight < 8) {
                opt &= ~OPT_MIPMAPS;
            }

            bool isDepth  = fmt == FMT_DEPTH || fmt == FMT_SHADOW;
            bool mipmaps  = (opt & OPT_MIPMAPS) != 0;
            bool cube     = (opt & OPT_CUBEMAP) != 0;
            bool dynamic  = (opt & OPT_DYNAMIC) != 0;
            bool isTarget = (opt & OPT_TARGET)  != 0 && !isDepth;

            static const struct FormatDesc {
                int       bpp;
                D3DFORMAT format;
            } formats[FMT_MAX] = {
                {   8, D3DFMT_L8       },
                {  32, D3DFMT_A8R8G8B8 },
                {  16, D3DFMT_R5G6B5   },
                {  16, D3DFMT_A1R5G5B5 },
                {  64, D3DFMT_A8R8G8B8 }, // TODO
                {  32, D3DFMT_A8R8G8B8 }, // TODO
                {  16, D3DFMT_LIN_D16  },
                {  16, D3DFMT_LIN_D16  },
            };

            FormatDesc desc = formats[fmt];

            uint32 usage = 0;
            if (isDepth)  usage |= D3DUSAGE_DEPTHSTENCIL;
            if (isTarget) usage |= D3DUSAGE_RENDERTARGET;

            D3DPOOL pool = (isTarget || isDepth) ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED;

            d3dformat = desc.format;

            if (cube) {
                D3DCHECK(device->CreateCubeTexture(width, 1, usage, desc.format, pool, &texCube));
            } else {
                D3DCHECK(device->CreateTexture(width, height, mipmaps ? 4 : 1, usage, desc.format, pool, &tex2D));
                if (data && !isTarget) {
                    update(data);
                }
            }

            if (pool != D3DPOOL_MANAGED)
                registerResource(this);
        }

        void deinit() {
            unregisterResource(this);
            if (tex2D)   tex2D->Release();
            if (texCube) texCube->Release();
        }

        void updateLevel(int32 level, void *data) {
            int32 bpp;
            switch (fmt) {
                case FMT_LUMINANCE : bpp = 1; break;
                case FMT_RGBA      : bpp = 4; break;
                default            : ASSERT(false);
            }

            int32 w = width >> level;
            int32 h = height >> level;
            int32 ow = origWidth >> level;
            int32 oh = origHeight >> level;

            uint8 *buffer = new uint8[w * h * bpp];

            if (fmt == FMT_RGBA) {
                uint8 *src = (uint8*)data;
                uint8 *dst = buffer;

                for (int y = 0; y < oh; y++) {
                    uint8 *ptr = dst;
                    for (int x = 0; x < ow; x++) {
                        ptr[0] = src[2];
                        ptr[1] = src[1];
                        ptr[2] = src[0];
                        ptr[3] = src[3];
                        ptr += 4;
                        src += 4;
                    }
                    dst += w * 4;
                }
            } else {
                if (w == ow && h == oh) {
                    memcpy(buffer, data, w * h * bpp);
                } else {
                    uint8 *src = (uint8*)data;
                    uint8 *dst = buffer;
                    for (int y = 0; y < oh; y++) {
                        memcpy(dst, src, ow * bpp);
                        src += ow * bpp;
                        dst += w * bpp;
                    }
                }
            }

            D3DLOCKED_RECT rect;
            D3DCHECK(tex2D->LockRect(level, &rect, NULL, 0));
            XGSwizzleRect(buffer, 0, NULL, rect.pBits, w, h, NULL, 4);
            D3DCHECK(tex2D->UnlockRect(level));

            delete[] buffer;

            if ((opt & OPT_MIPMAPS) && (level < 3)) {
                ASSERT(fmt == FMT_RGBA);

                uint8 *mip = new uint8[(ow >> 1) * (oh >> 1) * 4];
                uint8 *dst = mip;
                uint8 *src = (uint8*)data;

                for (int32 y = 0; y < oh; y += 2) {
                    for (int32 x = 0; x < ow; x += 2) {
                        *dst++ = (src[0] + src[4] + src[ow * 4] + src[ow * 4 + 4]) >> 2; src++; // R
                        *dst++ = (src[0] + src[4] + src[ow * 4] + src[ow * 4 + 4]) >> 2; src++; // G
                        *dst++ = (src[0] + src[4] + src[ow * 4] + src[ow * 4 + 4]) >> 2; src++; // B
                        *dst++ = (src[0] + src[4] + src[ow * 4] + src[ow * 4 + 4]) >> 2; src++; // A
                        src += 4;
                    }
                    src += ow * 4;
                }
                
                updateLevel(level + 1, mip);

                delete[] mip;
            }
        }

        void update(void *data) {
            updateLevel(0, data);
        }

        void bind(int sampler) {
            if (sampler > 3) return; // TODO

            if (opt & OPT_PROXY) return;
            ASSERT(tex2D || texCube);

            if (Core::active.textures[sampler] != this) {
                Core::active.textures[sampler] = this;
                if (tex2D) {
                    device->SetTexture(sampler, tex2D);
                    /* TODO unsupported
                    if (opt & OPT_VERTEX) {
                        device->SetTexture(D3DVERTEXTEXTURESAMPLER0 + sampler, tex2D);
                    }*/
                } else if (texCube) {
                    device->SetTexture(sampler, texCube);
                }

                bool filter  = (Core::settings.detail.filter > Core::Settings::LOW) && !(opt & OPT_NEAREST);
                bool mipmaps = (Core::settings.detail.filter > Core::Settings::LOW) &&  (opt & OPT_MIPMAPS);
                bool aniso   = (Core::settings.detail.filter > Core::Settings::MEDIUM) && mipmaps && (Core::support.maxAniso > 0);

                device->SetTextureStageState(sampler, D3DTSS_ADDRESSU, (opt & OPT_REPEAT) ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP);
                device->SetTextureStageState(sampler, D3DTSS_ADDRESSV, (opt & OPT_REPEAT) ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP);

                if (aniso) {
                    device->SetTextureStageState(sampler, D3DTSS_MINFILTER, D3DTEXF_ANISOTROPIC);
                    device->SetTextureStageState(sampler, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
                    device->SetTextureStageState(sampler, D3DTSS_MIPFILTER, D3DTEXF_NONE);
                    device->SetTextureStageState(sampler, D3DTSS_MAXANISOTROPY, support.maxAniso);
                } else {
                    device->SetTextureStageState(sampler, D3DTSS_MINFILTER, filter ? D3DTEXF_LINEAR : D3DTEXF_POINT);
                    device->SetTextureStageState(sampler, D3DTSS_MAGFILTER, filter ? D3DTEXF_LINEAR : D3DTEXF_POINT);
                    device->SetTextureStageState(sampler, D3DTSS_MIPFILTER, mipmaps ? (filter ? D3DTEXF_LINEAR : D3DTEXF_POINT) : D3DTEXF_NONE);
                    device->SetTextureStageState(sampler, D3DTSS_MAXANISOTROPY, 1);
                }
            }
        }

        void unbind(int sampler) {
            if (Core::active.textures[sampler]) {
                Core::active.textures[sampler] = NULL;
                device->SetTexture(sampler, NULL);
            }
        }

        void generateMipMap() {}

        void setFilterQuality(int value) {}
    };

// Mesh
    struct Mesh {
        LPDIRECT3DINDEXBUFFER8  IB;
        LPDIRECT3DVERTEXBUFFER8 VB;

        int  iCount;
        int  vCount;
        bool dynamic;

        Mesh(bool dynamic) : IB(NULL), VB(NULL), dynamic(dynamic) {}

        void init(Index *indices, int iCount, ::Vertex *vertices, int vCount, int aCount) {
            this->iCount = iCount;
            this->vCount = vCount;
            ASSERT(sizeof(GAPI::Vertex) == sizeof(::Vertex));

            uint32 usage = D3DUSAGE_WRITEONLY | (dynamic ? D3DUSAGE_DYNAMIC : 0);
            D3DPOOL pool = dynamic ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED;

            D3DCHECK(device->CreateIndexBuffer  (iCount * sizeof(Index),  usage, D3DFMT_INDEX16, pool, &IB));
            D3DCHECK(device->CreateVertexBuffer (vCount * sizeof(Vertex), usage, D3DFMT_UNKNOWN, pool, &VB));

            update(indices, iCount, vertices, vCount);

            if (pool != D3DPOOL_MANAGED)
                registerResource(this);
        }

        void deinit() {
            unregisterResource(this);
            IB->Release();
            VB->Release();
        }

        void update(Index *indices, int iCount, ::Vertex *vertices, int vCount) {
            ASSERT(sizeof(GAPI::Vertex) == sizeof(::Vertex));

            void* ptr;
            int size;

            if (indices && iCount) {
                size = iCount * sizeof(indices[0]);
                D3DCHECK(IB->Lock(0, 0, (BYTE**)&ptr, 0));
                memcpy(ptr, indices, size);
                D3DCHECK(IB->Unlock());
            }

            if (vertices && vCount) {
                size = vCount * sizeof(vertices[0]);
                D3DCHECK(VB->Lock(0, 0, (BYTE**)&ptr, 0));
                memcpy(ptr, vertices, size);
                D3DCHECK(VB->Unlock());
            }
        }

        void bind(const MeshRange &range) const {
            device->SetIndices(IB, range.vStart);
            device->SetStreamSource(0, VB, sizeof(Vertex));
        }

        void initNextRange(MeshRange &range, int &aIndex) const {
            range.aIndex = -1;
        }
    };

//    GLuint FBO, defaultFBO;
    struct RenderTargetCache {
        int count;
        struct Item {
            LPDIRECT3DSURFACE8 surface;
            int     width;
            int     height;
        } items[MAX_RENDER_BUFFERS];
    } rtCache[2];

    void init() {
        memset(rtCache, 0, sizeof(rtCache));
        isFrontCW = true;
        
        D3DADAPTER_IDENTIFIER8 adapterInfo;
        D3D->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &adapterInfo);
        LOG("Vendor   : %s\n", adapterInfo.Description);
        LOG("Renderer : Direct3D 8\n");

        support.maxAniso       = 4;
        support.maxVectors     = 16;
        support.shaderBinary   = false;
        support.VAO            = false; // SHADOW_COLOR
        support.depthTexture   = false; // SHADOW_DEPTH
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

        defRT = defDS = NULL;

        const float factors[] = {
            1.0f, -19.555555555556f, 60.444444444444f, -56.888888888889f,   // uCosCoeff
            0.5f, 0.5f/PI, 0.75f, 1.0f/1024.0f,                             // uAngles
            0.0f, 0.5f, 1.0f, 2.0f,
            0.6f, 0.9f, 0.9f, 32767.0f
        };
        device->SetVertexShaderConstant(92, factors, 4);

        device->SetRenderState(D3DRS_FOGTABLEMODE, D3DFOG_EXP);
    }

    void deinit() {
        if (defRT) defRT->Release();
        if (defDS) defDS->Release();
    }

    void resetDevice() {
    // release dummy RTs
        for (int i = 0; i < 2; i++) {
            RenderTargetCache &cache = rtCache[i];
            for (int j = 0; j < cache.count; j++)
                cache.items[j].surface->Release();
            cache.count = 0;
        }

    // release texture RTs
        int tmpCount = resCount;
        Resource tmpList[256];
        memcpy(tmpList, resList, sizeof(Resource) * tmpCount);

        for (int i = 0; i < tmpCount; i++) {
            Resource &res = tmpList[i];
            if (res.mesh)
                res.mesh->deinit();
            else
                res.texture->deinit();
        }

        if (defRT) defRT->Release();
        if (defDS) defDS->Release();

        D3DCHECK(device->Reset(&d3dpp));
        device->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &defRT);
        device->GetDepthStencilSurface(&defDS);

    // reinit texture RTs
        for (int i = 0; i < tmpCount; i++) {
            Resource &res = tmpList[i];
            if (res.mesh)
                res.mesh->init(NULL, res.mesh->iCount, NULL, res.mesh->vCount, 0);
            else
                res.texture->init(NULL);
        }
    }

    inline mat4::ProjRange getProjRange() {
        return mat4::PROJ_ZERO_POS;
    }

    mat4 ortho(float l, float r, float b, float t, float znear, float zfar) {
        mat4 m;
        m.ortho(getProjRange(), l, r, b, t, znear, zfar);
        return m;
    }

    mat4 perspective(float fov, float aspect, float znear, float zfar, float eye) {
        mat4 m;
        m.perspective(getProjRange(), fov, aspect, znear, zfar, eye);
        return m;
    }

    bool beginFrame() {
        if (defRT == NULL) device->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &defRT);
        if (defDS == NULL) device->GetDepthStencilSurface(&defDS);

        device->BeginScene();
        return true;
    }

    void endFrame() {
        device->EndScene();
    }

    void resetState() {
        device->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
        device->SetRenderState(D3DRS_LIGHTING, FALSE);
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

        if (depth)
            device->CreateDepthStencilSurface(width, height, D3DFMT_LIN_D16, D3DMULTISAMPLE_NONE, &item.surface);
        else
            device->CreateRenderTarget(width, height, D3DFMT_LIN_R5G6B5, D3DMULTISAMPLE_NONE, false, &item.surface);

        return cache.count++;
    }

    void bindTarget(Texture *target, int face) {
        if (!target) { // may be a null
            D3DCHECK(device->SetRenderTarget(defRT, defDS));
        } else {
            ASSERT(target->opt & OPT_TARGET);

            LPDIRECT3DSURFACE8 surface;

            bool depth = target->fmt == FMT_DEPTH || target->fmt == FMT_SHADOW;

            if (target->tex2D) {
                D3DCHECK(target->tex2D->GetSurfaceLevel(0, &surface));
            } else if (target->texCube) {
                D3DCHECK(target->texCube->GetCubeMapSurface(D3DCUBEMAP_FACES(D3DCUBEMAP_FACE_POSITIVE_X + face), 0, &surface));
            }

            int rtIndex = cacheRenderTarget(!depth, target->width, target->height);

            if (depth) {
                D3DCHECK(device->SetRenderTarget(rtCache[false].items[rtIndex].surface, surface));
            } else {
                D3DCHECK(device->SetRenderTarget(surface, rtCache[true].items[rtIndex].surface));
            }

            surface->Release();
        }

        Core::active.viewport = short4(0, 0, 0, 0); // forcing viewport reset
        Core::active.scissor  = short4(0, 0, 0, 0);
    }

    void discardTarget(bool color, bool depth) {}

    void copyTarget(Texture *dst, int xOffset, int yOffset, int x, int y, int width, int height) {
        ASSERT(dst && dst->tex2D);

        LPDIRECT3DSURFACE8 surface;
        dst->tex2D->GetSurfaceLevel(0, &surface);

        RECT srcRect = { x, y, x + width, y + height },
             dstRect = { xOffset, yOffset, xOffset + width, yOffset + height };

        //device->StretchRect(defRT, &srcRect, surface, &dstRect, D3DTEXF_POINT); TODO

        surface->Release();
    }

    void setVSync(bool enable) {
    //    d3dpp.FullScreen_PresentationInterval = enable ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;
    //    GAPI::resetDevice();
    }

    void waitVBlank() {}

    void clear(bool color, bool depth) {
        uint32 flags = (color ? D3DCLEAR_TARGET : 0) | (depth ? (D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL) : 0);
        if (flags) {
            device->Clear(0, NULL, flags, clearColor, 1.0f, 0);
        }
    }

    void setClearColor(const vec4 &color) {
        clearColor = (int(color.x * 255) << 16) |
                     (int(color.y * 255) << 8 ) |
                     (int(color.z * 255)      ) |
                     (int(color.w * 255) << 24);
    }

    void setViewport(const short4 &v) {
        D3DVIEWPORT8 viewport;
        viewport.X      = v.x;
        viewport.Y      = v.y;
        viewport.Width  = v.z;
        viewport.Height = v.w;
        viewport.MinZ   = 0.0f;
        viewport.MaxZ   = 1.0f;

        device->SetViewport(&viewport);
    }

    void setScissor(const short4 &s) {
        D3DRECT scissor;
        scissor.x1 = s.x;
        scissor.y1 = active.viewport.w - (s.y + s.w);
        scissor.x2 = s.x + s.z;
        scissor.y2 = active.viewport.w - s.y;

        device->SetScissors(1, FALSE, &scissor);
    }

    void setDepthTest(bool enable) {
        device->SetRenderState(D3DRS_ZENABLE, enable ? D3DZB_TRUE : D3DZB_FALSE);
    }

    void setDepthWrite(bool enable) {
        device->SetRenderState(D3DRS_ZWRITEENABLE, enable ? TRUE : FALSE);
    }

    void setColorWrite(bool r, bool g, bool b, bool a) {
        device->SetRenderState(D3DRS_COLORWRITEENABLE,
            (r ? D3DCOLORWRITEENABLE_RED   : 0) |
            (g ? D3DCOLORWRITEENABLE_GREEN : 0) |
            (b ? D3DCOLORWRITEENABLE_BLUE  : 0) |
            (a ? D3DCOLORWRITEENABLE_ALPHA : 0));
    }

    void setAlphaTest(bool enable) {
        device->SetTextureStageState(0, D3DTSS_ALPHAKILL, enable ? D3DTALPHAKILL_ENABLE : D3DTALPHAKILL_DISABLE);
    }

    void setCullMode(int rsMask) {
        cullMode = rsMask;
        switch (rsMask) {
            case RS_CULL_BACK  : device->SetRenderState(D3DRS_CULLMODE, isFrontCW ? D3DCULL_CW : D3DCULL_CCW);  break;
            case RS_CULL_FRONT : device->SetRenderState(D3DRS_CULLMODE, isFrontCW ? D3DCULL_CCW : D3DCULL_CW); break;
            default            : device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        }
    }

    void setFrontFace(bool cw) {
        isFrontCW = cw;
        setCullMode(cullMode);
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
        device->SetTransform(D3DTS_PROJECTION, (D3DXMATRIX*)&mProj);
    }

    void updateLights(vec4 *lightPos, vec4 *lightColor, int count) {
        if (active.shader) {
            active.shader->setParam(uLightColor, lightColor[0], count);
            active.shader->setParam(uLightPos,   lightPos[0],   count);
        }
    }

    void setFog(const vec4 &params) {
        if (params.w > 0.0f) {
            device->SetRenderState(D3DRS_FOGENABLE, TRUE);

            DWORD fogColor = 0xFF000000
                | (DWORD(clamp(params.z * 255.0f, 0.0f, 255.0f)) << 0)
                | (DWORD(clamp(params.y * 255.0f, 0.0f, 255.0f)) << 8)
                | (DWORD(clamp(params.x * 255.0f, 0.0f, 255.0f)) << 16);
            device->SetRenderState(D3DRS_FOGCOLOR, fogColor);

            ASSERT(Core::active.shader);
            Core::active.shader->setParam(uFogParams, params); // color.rgb, factor
        } else {
            device->SetRenderState(D3DRS_FOGENABLE, FALSE);
        }
    }

    void DIP(Mesh *mesh, const MeshRange &range) {
        device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, mesh->vCount, range.iStart, range.iCount / 3);
    }

    vec4 copyPixel(int x, int y) {
        GAPI::Texture *t = Core::active.target;
        ASSERT(t && t->tex2D);
/*
        LPDIRECT3DSURFACE8 surface, texSurface;
        D3DCHECK(t->tex2D->GetSurfaceLevel(0, &texSurface));
        D3DCHECK(device->CreateOffscreenPlainSurface(t->width, t->height, t->d3dformat, D3DPOOL_SYSTEMMEM, &surface, NULL));
        D3DCHECK(device->GetRenderTargetData(texSurface, surface));

        RECT r = { x, y, x + 1, y + 1 };
        D3DLOCKED_RECT rect;
        surface->LockRect(&rect, &r, D3DLOCK_READONLY);
        ubyte4 c = *((ubyte4*)rect.pBits);
        surface->UnlockRect();

        texSurface->Release();
        surface->Release();

        return vec4(float(c.z), float(c.y), float(c.x), float(c.w)) * (1.0f / 255.0f);
        */
        return vec4(1.0f);
    }
}

#endif