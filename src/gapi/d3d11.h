#ifndef H_GAPI_D3D11
#define H_GAPI_D3D11

#include "core.h"
#include <d3d11.h>

#define SAFE_RELEASE(P) if(P){P->Release(); P = NULL;}

#define PROFILE_MARKER(title)
#define PROFILE_LABEL(id, name, label)
#define PROFILE_TIMING(time)

extern ID3D11Device          *device;
extern ID3D11DeviceContext   *deviceContext;
extern IDXGISwapChain        *swapChain;

#ifdef _DEBUG
void D3DCHECK(HRESULT res) {
    if (res == S_OK) return;
    /*
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
        case D3DERR_DRIVERINVALIDCALL         : LOG("D3DERR_DRIVERINVALIDCALL");         break;
        case D3DERR_WASSTILLDRAWING           : LOG("D3DERR_WASSTILLDRAWING");           break;
        default                               : LOG("D3DERR_UNKNOWN");                   break;
    }
    LOG("\n");
    */
    ASSERT(false);
}
#else
    #define D3DCHECK(res) res
#endif

#define MAX_SAMPLERS 5

namespace GAPI {
    using namespace Core;

    typedef ::Vertex Vertex;

    CullMode  cullMode;
    BlendMode blendMode;
    vec4 clearColor;

    bool depthTest, depthWrite, colorWrite;
    bool dirtyDepthState;
    bool dirtyBlendState;

    ID3D11RenderTargetView  *defRTV;
    ID3D11DepthStencilView  *defDSV;
    ID3D11InputLayout       *inputLayout;

    ID3D11BlendState        *BS[2][bmMAX]; // [colorWrite][blendMode] ONLY two colorWrite modes are supported (A and RGBA)
    ID3D11RasterizerState   *RS[cmMAX];    // [cullMode]
    ID3D11DepthStencilState *DS[2][2];     // [depthTest][depthWrite]

    ID3D11SamplerState      *samplers[MAX_SAMPLERS];

// Shader
    #include "shaders/d3d11/shaders.h"

    enum {
        USAGE_VS,
        USAGE_PS,
    };

    static const struct Binding {
        int reg;
        int usage;
    } bindings[uMAX] = {
        {   0, USAGE_VS | USAGE_PS }, // uFlags
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
        ID3D11VertexShader *VS;
        ID3D11PixelShader  *PS;
        ID3D11Buffer       *CB;

        vec4  cbMem[98 + MAX_CONTACTS];
        int   cbCount[uMAX];

        bool  rebind;

        Shader() : VS(NULL), PS(NULL) {}

        void init(Core::Pass pass, int type, int *def, int defCount) {
            memset(cbMem, 0, sizeof(cbMem));

            bool underwater = false;
            bool alphatest  = false;

            for (int i = 0; i < defCount; i++) {
                switch (def[i]) {
                    case SD_UNDERWATER   : underwater = true; break;
                    case SD_ALPHA_TEST   : alphatest  = true; break;
                    case SD_OPT_AMBIENT  : cbMem[uFlags].x = 1.0f; break;
                    case SD_OPT_SHADOW   : cbMem[uFlags].y = 1.0f; break;
                    case SD_OPT_CONTACT  : cbMem[uFlags].z = 1.0f; break;
                    case SD_OPT_CAUSTICS : cbMem[uFlags].w = 1.0f; break;
                }
            }

            int vSize, fSize;

            #define SHADER(S,P)    (P##Src = S##_##P, P##Size = sizeof(S##_##P))
            #define SHADER_A(S,P)  (alphatest  ? SHADER(S##_a,P) : SHADER(S,P))
            #define SHADER_U(S,P)  (underwater ? SHADER(S##_u,P) : SHADER(S,P))
            #define SHADER_AU(S,P) ((underwater && alphatest) ? SHADER(S##_au,P) : (alphatest ? SHADER(S##_a,P) : SHADER_U(S,P)))

            const uint8 *vSrc, *fSrc;
            switch (pass) {
                case passCompose :
                    switch (type) {
                        case 0  : SHADER_U ( compose_sprite, v );  SHADER_AU ( compose_sprite, f ); break;
                        case 1  : SHADER   ( compose_flash,  v );  SHADER    ( compose_flash,  f ); break;
                        case 2  : SHADER_U ( compose_room,   v );  SHADER_AU ( compose_room,   f ); break;
                        case 3  : SHADER_U ( compose_entity, v );  SHADER_AU ( compose_entity, f ); break;
                        case 4  : SHADER   ( compose_mirror, v );  SHADER    ( compose_mirror, f ); break;
                        default : ASSERT(false);
                    }
                    break;
                case passShadow : 
                    switch (type) {
                        case 3  :
                        case 4  : SHADER ( shadow_entity, v );  SHADER ( shadow_entity, f ); break;
                        default : ASSERT(false);
                    }
                    break;
                case passAmbient :
                    switch (type) {
                        case 0  : SHADER ( ambient_sprite, v );  SHADER_A ( ambient_sprite, f ); break;
                        case 1  : SHADER ( ambient_room,   v );  SHADER   ( ambient_room,   f ); break; // TYPE_FLASH (sky)
                        case 2  : SHADER ( ambient_room,   v );  SHADER_A ( ambient_room,   f ); break;
                        default : ASSERT(false);
                    }
                    break;
                case passSky   : SHADER ( gui, v );  SHADER ( gui, f ); break; // TODO
                case passWater : 
                    switch (type) {
                        case 0  : SHADER ( water_drop,     v );  SHADER ( water_drop,     f ); break;
                        case 1  : SHADER ( water_simulate, v );  SHADER ( water_simulate, f ); break;
                        case 2  : SHADER ( water_caustics, v );  SHADER ( water_caustics, f ); break;
                        case 3  : SHADER ( water_rays,     v );  SHADER ( water_rays,     f ); break;
                        case 4  : SHADER ( water_mask,     v );  SHADER ( water_mask,     f ); break;
                        case 5  : SHADER ( water_compose,  v );  SHADER ( water_compose,  f ); break;
                        default : ASSERT(false);
                    }
                    break;
                case passFilter :
                    switch (type) {
                        case 0  : SHADER ( filter_upscale,    v );  SHADER ( filter_upscale,    f ); break;
                        case 1  : SHADER ( filter_downsample, v );  SHADER ( filter_downsample, f ); break;
                        case 3  : SHADER ( filter_grayscale,  v );  SHADER ( filter_grayscale,  f ); break;
                        case 4  : SHADER ( filter_blur,       v );  SHADER ( filter_blur,       f ); break;
                        default : ASSERT(false);
                    }
                    break;
                case passGUI    : SHADER ( gui, v );  SHADER ( gui, f ); break;
                default         : ASSERT(false); LOG("! wrong pass id\n"); return;
            }

            #undef SHADER_A
            #undef SHADER_U
            #undef SHADER_AU

            HRESULT ret;
            ret = device->CreateVertexShader ((DWORD*)vSrc, vSize, NULL, &VS); ASSERT(ret == S_OK);
            ret = device->CreatePixelShader  ((DWORD*)fSrc, fSize, NULL, &PS); ASSERT(ret == S_OK);

            const D3D11_INPUT_ELEMENT_DESC vertexDecl[] = {
                { "POSITION", 0, DXGI_FORMAT_R16G16B16A16_SINT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // aCoord
                { "NORMAL",   0, DXGI_FORMAT_R16G16B16A16_SINT, 0,  8, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // aNormal
                { "TEXCOORD", 0, DXGI_FORMAT_R16G16B16A16_SINT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // aTexCoord
                { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // aColor
                { "COLOR",    1, DXGI_FORMAT_R8G8B8A8_UNORM,    0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // aLight
            };

            ret = device->CreateInputLayout(vertexDecl, COUNT(vertexDecl), vSrc, vSize, &inputLayout);
            ASSERT(ret == S_OK);

            rebind = true;

            D3D11_BUFFER_DESC      desc;
            memset(&desc, 0, sizeof(desc));
            desc.Usage          = D3D11_USAGE_DYNAMIC;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
            desc.ByteWidth      = sizeof(cbMem);
            device->CreateBuffer(&desc, NULL, &CB);
        }

        void deinit() {
            SAFE_RELEASE(VS);
            SAFE_RELEASE(PS);
        }

        void bind() {
            if (Core::active.shader != this) {
                Core::active.shader = this;
                memset(cbCount, 0, sizeof(cbCount));
                rebind = true;
            }
        }

        void validate() {
            if (rebind) {
                deviceContext->IASetInputLayout(inputLayout);
                deviceContext->VSSetShader(VS, NULL, 0);
                deviceContext->PSSetShader(PS, NULL, 0);
                rebind = false;
            }

            D3D11_MAPPED_SUBRESOURCE mapped;
            deviceContext->Map(CB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
            memcpy(mapped.pData, cbMem, sizeof(cbMem));
            deviceContext->Unmap(CB, 0);

            deviceContext->VSSetConstantBuffers(0, 1, &CB);
            deviceContext->PSSetConstantBuffers(0, 1, &CB);
            Core::stats.cb++;
        }

        void setParam(UniformType uType, float *value, int count) {
            cbCount[uType] = count;
            memcpy(cbMem + bindings[uType].reg, value, count * 16);
        }

        void setParam(UniformType uType, const vec4 &value, int count = 1) {
            setParam(uType, (float*)&value, count);
        }

        void setParam(UniformType uType, const mat4 &value, int count = 1) {
            setParam(uType, (float*)&value, count * 4);
        }
    };

// Texture
    struct Texture {
        ID3D11Texture2D          *tex2D;
        ID3D11Texture3D          *tex3D;
        ID3D11ShaderResourceView *SRV;
        ID3D11RenderTargetView   *RTV;
        ID3D11DepthStencilView   *DSV;

        int       width, height, depth, origWidth, origHeight, origDepth;
        TexFormat fmt;
        uint32    opt;

        Texture(int width, int height, int depth, uint32 opt) : tex2D(NULL), tex3D(NULL), SRV(NULL), RTV(NULL), DSV(NULL), width(width), height(height), depth(depth), origWidth(width), origHeight(height), origDepth(depth), fmt(FMT_RGBA), opt(opt) {}

        void init(void *data) {
            ASSERT((opt & OPT_PROXY) == 0);

            bool mipmaps   = (opt & OPT_MIPMAPS) != 0;
            bool isDepth   = fmt == FMT_DEPTH || fmt == FMT_SHADOW;
            bool isCube    = (opt & OPT_CUBEMAP) != 0;
            bool isVolume  = (opt & OPT_VOLUME)  != 0;
            bool isTarget  = (opt & OPT_TARGET);
            bool isDynamic = (opt & OPT_DYNAMIC);

            static const struct FormatDesc {
                int         bpp;
                DXGI_FORMAT format;
            } formats[FMT_MAX] = {
                {   8, DXGI_FORMAT_R8_UNORM       },
                {  32, DXGI_FORMAT_R8G8B8A8_UNORM },
                {  16, DXGI_FORMAT_B5G6R5_UNORM   },
                {  16, DXGI_FORMAT_B5G5R5A1_UNORM },
                {  64, DXGI_FORMAT_R32G32_FLOAT   },
                {  32, DXGI_FORMAT_R16G16_FLOAT   },
                {  16, DXGI_FORMAT_R16_TYPELESS   },
                {  16, DXGI_FORMAT_R16_TYPELESS   },
            };

            D3D11_SUBRESOURCE_DATA initialData[6];
            initialData[0].pSysMem          = data;
            initialData[0].SysMemPitch      = origWidth * formats[fmt].bpp / 8;
            initialData[0].SysMemSlicePitch = origHeight * initialData[0].SysMemPitch;

            if (isCube && data) {
                for (int i = 1; i < 6; i++) {
                    initialData[i] = initialData[i - 1];
                    *(uint8*)initialData[i].pSysMem += initialData[i].SysMemSlicePitch;
                }
            }

            D3D11_SHADER_RESOURCE_VIEW_DESC descSRV;
            memset(&descSRV, 0, sizeof(descSRV));
            descSRV.Format = isDepth ? DXGI_FORMAT_R16_UNORM : formats[fmt].format;
            descSRV.Texture2D.MipLevels = 1; // for cube, 3d and 2d

            if (isVolume) {
                D3D11_TEXTURE3D_DESC desc;
                memset(&desc, 0, sizeof(desc));
                desc.Width              = width;
                desc.Height             = height;
                desc.Depth              = origDepth;
                desc.MipLevels          = 1;
                desc.Format             = formats[fmt].format;
                desc.Usage              = D3D11_USAGE_DEFAULT;
                desc.BindFlags          = D3D11_BIND_SHADER_RESOURCE;
                device->CreateTexture3D(&desc, data ? initialData : NULL, &tex3D);
                ASSERT(tex3D);

                descSRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
                device->CreateShaderResourceView(tex3D, &descSRV, &SRV);
            } else {
                D3D11_TEXTURE2D_DESC desc;
                memset(&desc, 0, sizeof(desc));
                desc.Width              = width;
                desc.Height             = height;
                desc.MipLevels          = 1;//mipmaps ? 0 : 1;
                desc.ArraySize          = isCube ? 6 : 1;
                desc.Format             = formats[fmt].format;
                desc.SampleDesc.Count   = 1;
                desc.Usage              = D3D11_USAGE_DEFAULT;
                desc.BindFlags          = D3D11_BIND_SHADER_RESOURCE;
                desc.MiscFlags          = (isCube ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0);

                if (isTarget) {
                    if (isDepth) {
                        desc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
                    } else {
                        desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
                    }
                }

                if (isDynamic) {
                    desc.Usage          = D3D11_USAGE_DYNAMIC;
                    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
                }

                if (mipmaps) {
                    desc.BindFlags |= D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
                    desc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
                }

                device->CreateTexture2D(&desc, data ? initialData : NULL, &tex2D);
                ASSERT(tex2D);

                if (isTarget) {
                    if (isDepth) {
                        D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
                        memset(&descDSV, 0, sizeof(descDSV));
                        descDSV.Format        = DXGI_FORMAT_D16_UNORM;
                        descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                        device->CreateDepthStencilView(tex2D, &descDSV, &DSV);
                        ASSERT(DSV);
                    } else {
                        device->CreateRenderTargetView(tex2D, NULL, &RTV);
                        ASSERT(RTV);
                    }
                }

                descSRV.ViewDimension = isCube ? D3D11_SRV_DIMENSION_TEXTURECUBE : D3D11_SRV_DIMENSION_TEXTURE2D;
                device->CreateShaderResourceView(tex2D, &descSRV, &SRV);
            }

            ASSERT(SRV);
        }

        void deinit() {
            SAFE_RELEASE(tex2D);
            SAFE_RELEASE(tex3D);
            SAFE_RELEASE(SRV);
            SAFE_RELEASE(RTV);
            SAFE_RELEASE(DSV);
        }

        void generateMipMap() {
            ASSERT(SRV && tex2D);
            deviceContext->GenerateMips(SRV);
        }

        void update(void *data) {
            ASSERT(tex2D);
            ASSERT(opt & OPT_DYNAMIC);
            D3D11_MAPPED_SUBRESOURCE mapped;
            D3DCHECK(deviceContext->Map(tex2D, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped));
            memcpy(mapped.pData, data, mapped.RowPitch * height);
            deviceContext->Unmap(tex2D, 0);
        }

        void bind(int sampler) {
            if (opt & OPT_PROXY) return;
            ASSERT(tex2D || tex3D);

            if (Core::active.textures[sampler] != this) {
                Core::active.textures[sampler] = this;

                if (opt & OPT_VERTEX) {
                    deviceContext->VSSetShaderResources(sampler, 1, &SRV);
                }
                deviceContext->PSSetShaderResources(sampler, 1, &SRV);
            }
        }

        void unbind(int sampler) {
            if (Core::active.textures[sampler]) {
                Core::active.textures[sampler] = NULL;
            }
        }

        void setFilterQuality(int value) {}
    };

// Mesh
    struct Mesh {
        ID3D11Buffer *IB, *VB;

        int  iCount;
        int  vCount;
        bool dynamic;

        Mesh(bool dynamic) : IB(NULL), VB(NULL), dynamic(dynamic) {}

        void init(Index *indices, int iCount, ::Vertex *vertices, int vCount, int aCount) {
            this->iCount = iCount;
            this->vCount = vCount;
            ASSERT(sizeof(GAPI::Vertex) == sizeof(::Vertex));

            D3D11_BUFFER_DESC      desc;
            D3D11_SUBRESOURCE_DATA initData;

            memset(&desc,     0, sizeof(desc));
            memset(&initData, 0, sizeof(initData));
            desc.Usage          = dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
            desc.CPUAccessFlags = dynamic ? D3D11_CPU_ACCESS_WRITE : 0;
            
            desc.BindFlags   = D3D11_BIND_INDEX_BUFFER;
            desc.ByteWidth   = iCount * sizeof(Index);
            initData.pSysMem = indices;
            D3DCHECK(device->CreateBuffer(&desc, dynamic ? NULL : &initData, &IB));

            desc.BindFlags   = D3D11_BIND_VERTEX_BUFFER;
            desc.ByteWidth   = vCount * sizeof(Vertex);
            initData.pSysMem = vertices;
            D3DCHECK(device->CreateBuffer(&desc, dynamic ? NULL : &initData, &VB));
        }

        void deinit() {
            SAFE_RELEASE(IB);
            SAFE_RELEASE(VB);
        }

        void update(Index *indices, int iCount, ::Vertex *vertices, int vCount) {
            ASSERT(sizeof(GAPI::Vertex) == sizeof(::Vertex));

            D3D11_MAPPED_SUBRESOURCE mapped;
         
            if (indices && iCount) {
                deviceContext->Map(IB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
                memcpy(mapped.pData, indices, iCount * sizeof(indices[0]));
                deviceContext->Unmap(IB, 0);
            }

            if (vertices && vCount) {
                deviceContext->Map(VB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
                memcpy(mapped.pData, vertices, vCount * sizeof(vertices[0]));
                deviceContext->Unmap(VB, 0);
            }
        }

        void bind(const MeshRange &range) const {
            UINT stride = sizeof(Vertex);
            UINT offset = 0;//range.vStart * stride;
            deviceContext->IASetIndexBuffer(IB, DXGI_FORMAT_R16_UINT, 0);
            deviceContext->IASetVertexBuffers(0, 1, &VB, &stride, &offset);
        }

        void initNextRange(MeshRange &range, int &aIndex) const {
            range.aIndex = -1;
        }
    };

//    GLuint FBO, defaultFBO;
    struct RenderTargetCache {
        int count;
        struct Item {
            ID3D11RenderTargetView *RTV;
            ID3D11DepthStencilView *DSV;
            int                    width;
            int                    height;
        } items[MAX_RENDER_BUFFERS];
    } rtCache;

    void deinitSamplers() {
        for (int i = 0; i < COUNT(samplers); i++) {
            SAFE_RELEASE(samplers[i]);
        }
    }

    ID3D11SamplerState* initSampler(bool filter, bool aniso, bool wrap, bool cmp) {
        D3D11_SAMPLER_DESC desc;
        memset(&desc, 0, sizeof(desc));

        if (aniso && (Core::support.maxAniso > 0)) {
            desc.MaxAnisotropy = min(int(Core::support.maxAniso), 8);
        } else {
            desc.MaxAnisotropy = 1;
        }

        if (desc.MaxAnisotropy > 1) {
            desc.Filter = D3D11_FILTER_ANISOTROPIC;
        } else {
            if (filter) {
                desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            } else {
                desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
            }
        }

        if (cmp) {
            desc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
            desc.ComparisonFunc = D3D11_COMPARISON_LESS;
        } else {
            desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        }

        desc.AddressU =
        desc.AddressV =
        desc.AddressW = wrap ? D3D11_TEXTURE_ADDRESS_WRAP : D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.MinLOD   = 0;
        desc.MaxLOD   = D3D11_FLOAT32_MAX;

        ID3D11SamplerState *sampler;
        device->CreateSamplerState(&desc, &sampler);
        return sampler;
    }

    void initSamplers() {
        deinitSamplers();
    /*
        0 - smpDefault
        1 - smpPoint
        2 - smpPointWrap
        3 - smpLinear
        4 - smpCmp
    */
        samplers[0] = initSampler(true,  true,  false, false); // TODO settings dependent
        samplers[1] = initSampler(false, false, false, false);
        samplers[2] = initSampler(false, false, true,  false);
        samplers[3] = initSampler(true,  false, false, false);
        samplers[4] = initSampler(true,  false, false, true);
    }

    void init() {
        memset(&rtCache, 0, sizeof(rtCache));
        
        /* TODO
        D3DADAPTER_IDENTIFIER9 adapterInfo;
        D3D->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &adapterInfo);
        LOG("Vendor   : %s\n", adapterInfo.Description);
        LOG("Renderer : Direct3D 9.0c\n");
        */

        support.maxAniso       = 8;
        support.maxVectors     = 16;
        support.shaderBinary   = true;
        support.VAO            = false; // SHADOW_COLOR
        support.depthTexture   = true;
        support.shadowSampler  = true;
        support.discardFrame   = false;
        support.texNPOT        = true;
        support.texRG          = true;
        support.texBorder      = true;
        support.colorFloat     = true;
        support.colorHalf      = true;
        support.texFloatLinear = true;
        support.texFloat       = true;
        support.texHalfLinear  = true;
        support.texHalf        = true;
        support.tex3D          = true;
        support.clipDist       = true;

        #ifdef PROFILE
            support.profMarker = false;
            support.profTiming = false;
        #endif

        defRTV = NULL;
        defDSV = NULL;

    // init blend modes
        {
            #define BLEND_FUNC(B,S,D)\
                desc.RenderTarget[0].SrcBlend  = S;\
                desc.RenderTarget[0].DestBlend = D;\
                device->CreateBlendState(&desc, &BS[i][B])

            D3D11_BLEND_DESC desc;
            memset(&desc, 0, sizeof(desc));
            desc.IndependentBlendEnable         = FALSE;
            desc.RenderTarget[0].BlendOp        = D3D11_BLEND_OP_ADD;
            desc.RenderTarget[0].BlendOpAlpha   = D3D11_BLEND_OP_ADD;
            desc.RenderTarget[0].SrcBlendAlpha  = D3D11_BLEND_ONE;
            desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;

            for (int i = 0; i < 2; i++) {
                desc.RenderTarget[0].RenderTargetWriteMask = i ? D3D11_COLOR_WRITE_ENABLE_ALL : D3D11_COLOR_WRITE_ENABLE_ALPHA;
                desc.RenderTarget[0].BlendEnable = FALSE;
                BLEND_FUNC(bmNone,    D3D11_BLEND_ONE,  D3D11_BLEND_ZERO);
                desc.RenderTarget[0].BlendEnable = TRUE;
                BLEND_FUNC(bmAlpha,   D3D11_BLEND_SRC_ALPHA,  D3D11_BLEND_INV_SRC_ALPHA);
                BLEND_FUNC(bmAdd,     D3D11_BLEND_ONE,        D3D11_BLEND_ONE);
                BLEND_FUNC(bmMult,    D3D11_BLEND_DEST_COLOR, D3D11_BLEND_ZERO);
                BLEND_FUNC(bmPremult, D3D11_BLEND_ONE,        D3D11_BLEND_INV_SRC_ALPHA);
            }

            #undef BLEND_FUNC
        }

    // init raster state
        {
            D3D11_RASTERIZER_DESC desc;
            memset(&desc, 0, sizeof(desc));
            desc.FrontCounterClockwise = TRUE;
            desc.FillMode = D3D11_FILL_SOLID;
            desc.CullMode = D3D11_CULL_NONE;
            device->CreateRasterizerState(&desc, &RS[cmNone]);
            desc.CullMode = D3D11_CULL_BACK;
            device->CreateRasterizerState(&desc, &RS[cmBack]);
            desc.CullMode = D3D11_CULL_FRONT;
            device->CreateRasterizerState(&desc, &RS[cmFront]);
        }

    // init depth stencil states
        {
            D3D11_DEPTH_STENCIL_DESC desc;
            memset(&desc, 0, sizeof(desc));
            desc.StencilEnable  = FALSE;
            desc.DepthEnable    = TRUE;
            desc.DepthFunc      = D3D11_COMPARISON_LESS_EQUAL;

            for (int i = 0; i < 2; i++) {
                desc.DepthEnable = i ? TRUE : FALSE;
                for (int j = 0; j < 2; j++) {
                    desc.DepthWriteMask = j ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
                    device->CreateDepthStencilState(&desc, &DS[i][j]);
                }
            }
        }

    // init samplers
        memset(samplers, 0, sizeof(samplers));
        initSamplers();
    }

    void resetDevice() {
        SAFE_RELEASE(defRTV);
        SAFE_RELEASE(defDSV);

        for (int i = 0; i < rtCache.count; i++) {
            SAFE_RELEASE(rtCache.items[i].RTV);
            SAFE_RELEASE(rtCache.items[i].DSV);
        }

        deinitSamplers();

        rtCache.count = 0;
    }


    void deinit() {
        resetDevice();

        for (int i = 0; i < COUNT(RS); i++) {
            SAFE_RELEASE(RS[i]);
        }

        for (int j = 0; j < COUNT(DS); j++) {
            for (int i = 0; i < COUNT(DS[0]); i++) {
                SAFE_RELEASE(DS[j][i]);
            }
        }


        for (int j = 0; j < COUNT(BS); j++) {
            for (int i = 0; i < COUNT(BS[0]); i++) {
                SAFE_RELEASE(BS[j][i]);
            }
        }
    }

    mat4 ortho(float l, float r, float b, float t, float znear, float zfar) {
        mat4 m;
        m.ortho(mat4::PROJ_ZERO_POS, l, r, b, t, znear, zfar);
        return m;
    }

    mat4 perspective(float fov, float aspect, float znear, float zfar, float eye) {
        mat4 m;
        m.perspective(mat4::PROJ_ZERO_POS, fov, aspect, znear, zfar, eye);
        return m;
    }

    bool beginFrame() {
        if (!defRTV) {
            ID3D11Texture2D *pBackBuffer = NULL;
            D3DCHECK(swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer));
            D3DCHECK(device->CreateRenderTargetView(pBackBuffer, NULL, &defRTV));
            SAFE_RELEASE(pBackBuffer);
        }

        if (!defDSV) {
            D3D11_TEXTURE2D_DESC desc;
            memset(&desc, 0, sizeof(desc));
            desc.Width            = Core::width;
            desc.Height           = Core::height;
            desc.MipLevels        = 1;
            desc.ArraySize        = 1;
            desc.Format           = DXGI_FORMAT_D24_UNORM_S8_UINT;
            desc.SampleDesc.Count = 1;
            desc.Usage            = D3D11_USAGE_DEFAULT;
            desc.BindFlags        = D3D11_BIND_DEPTH_STENCIL;

            ID3D11Texture2D *dsTex;
            D3DCHECK(device->CreateTexture2D(&desc, NULL, &dsTex));

            D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	        memset(&descDSV, 0, sizeof(descDSV));
            descDSV.Format             = desc.Format;
            descDSV.ViewDimension      = D3D11_DSV_DIMENSION_TEXTURE2D;
            descDSV.Texture2D.MipSlice = 0;
            D3DCHECK(device->CreateDepthStencilView(dsTex, &descDSV, &defDSV));

            SAFE_RELEASE(dsTex);
        }

        deviceContext->OMSetRenderTargets(1, &defRTV, defDSV);

        return true;
    }

    void endFrame() {
        //
    }

    void resetState() {
        deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        deviceContext->RSSetState(RS[cmNone]);
        depthTest = depthWrite = dirtyDepthState = true;
        colorWrite = dirtyBlendState = true;

        deviceContext->VSGetSamplers(0, COUNT(samplers), samplers);
        deviceContext->PSGetSamplers(0, COUNT(samplers), samplers);
    }

    void cacheRenderTarget(ID3D11RenderTargetView **RTV, ID3D11DepthStencilView **DSV, int width, int height) {
        ASSERT((RTV != NULL) ^ (DSV != NULL));

        int index = -1;
        for (int i = 0; i < rtCache.count; i++)
            if (rtCache.items[i].width == width && rtCache.items[i].height == height) {
                index = i;
                break;
            }

        ASSERT(rtCache.count < MAX_RENDER_BUFFERS);

        if (index == -1) {
            index = rtCache.count++;
            rtCache.items[index].width  = width;
            rtCache.items[index].height = height;
        }

        if (RTV) {
            if (!rtCache.items[index].RTV) {
                ID3D11Texture2D *tex2D = NULL;

                D3D11_TEXTURE2D_DESC desc;
                memset(&desc, 0, sizeof(desc));
                desc.Width            = width;
                desc.Height           = height;
                desc.MipLevels        = 1;
                desc.ArraySize        = 1;
                desc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
                desc.SampleDesc.Count = 1;
                desc.Usage            = D3D11_USAGE_DEFAULT;
                desc.BindFlags        = D3D11_BIND_RENDER_TARGET;
                device->CreateTexture2D(&desc, NULL, &tex2D);
                device->CreateRenderTargetView(tex2D, NULL, &rtCache.items[index].RTV);
                SAFE_RELEASE(tex2D);
            }
            *RTV = rtCache.items[index].RTV;
            ASSERT(*RTV);
        }

        if (DSV) {
            if (!rtCache.items[index].DSV) {
                ID3D11Texture2D *tex2D = NULL;

                D3D11_TEXTURE2D_DESC desc;
                memset(&desc, 0, sizeof(desc));
                desc.Width            = width;
                desc.Height           = height;
                desc.MipLevels        = 1;
                desc.ArraySize        = 1;
                desc.Format           = DXGI_FORMAT_R16_TYPELESS;
                desc.SampleDesc.Count = 1;
                desc.Usage            = D3D11_USAGE_DEFAULT;
                desc.BindFlags        = D3D11_BIND_DEPTH_STENCIL;
                device->CreateTexture2D(&desc, NULL, &tex2D);

                D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
                memset(&descDSV, 0, sizeof(descDSV));
                descDSV.Format        = DXGI_FORMAT_D16_UNORM;
                descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                device->CreateDepthStencilView(tex2D, &descDSV, &rtCache.items[index].DSV);
                SAFE_RELEASE(tex2D);
            }
            *DSV = rtCache.items[index].DSV;
            ASSERT(*DSV);
        }
    }

    void bindTarget(Texture *target, int face) {
        ID3D11RenderTargetView *RTV = defRTV;
        ID3D11DepthStencilView *DSV = defDSV;

        if (target) {
            ASSERT(target->opt & OPT_TARGET);

            if (target->RTV) {
                RTV = target->RTV;
                cacheRenderTarget(NULL, &DSV, target->width, target->height);
            } else if (target->DSV) {
                DSV = target->DSV;
                cacheRenderTarget(&RTV, NULL, target->width, target->height);
            } else {
                ASSERT(false);
            }
        }

        deviceContext->OMSetRenderTargets(1, &RTV, DSV);

        Core::active.viewport = Viewport(0, 0, 0, 0); // forcing viewport reset
    }

    void discardTarget(bool color, bool depth) {}

    void copyTarget(Texture *dst, int xOffset, int yOffset, int x, int y, int width, int height) {
        D3D11_BOX box;

        box.left   = x;
        box.top    = y;
        box.right  = x + width;
        box.bottom = y + height;
        box.front  = 0;
        box.back   = 1;

        ID3D11RenderTargetView *RTV;
        ID3D11Resource         *res;

        deviceContext->OMGetRenderTargets(1, &RTV, NULL);
        RTV->GetResource(&res);

        deviceContext->CopySubresourceRegion(dst->tex2D, 0, xOffset, yOffset, 0, res, 0, &box);

        SAFE_RELEASE(RTV);
        SAFE_RELEASE(res);
    }

    void setVSync(bool enable) {}
    void waitVBlank() {}

    void clear(bool color, bool depth) {
        ID3D11RenderTargetView *RTV = NULL;
        ID3D11DepthStencilView *DSV = NULL;

        deviceContext->OMGetRenderTargets(1, &RTV, &DSV);

        if (color && RTV) {
            deviceContext->ClearRenderTargetView(RTV, (FLOAT*)&clearColor);
        }

        if (depth && DSV) {
            deviceContext->ClearDepthStencilView(DSV, D3D11_CLEAR_DEPTH, 1.0, 0);
        }

        SAFE_RELEASE(RTV);
        SAFE_RELEASE(DSV);
    }

    void setClearColor(const vec4 &color) {
        clearColor = color;
    }

    void setViewport(const Viewport &vp) {
        D3D11_VIEWPORT viewport;
        viewport.TopLeftX = (FLOAT)x;
        viewport.TopLeftY = (FLOAT)y;
        viewport.Width    = (FLOAT)width;
        viewport.Height   = (FLOAT)height;
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        deviceContext->RSSetViewports(1, &viewport);
    }

    void setDepthTest(bool enable) {
        depthTest = enable;
        dirtyDepthState = true;
    }

    void setDepthWrite(bool enable) {
        depthWrite = enable;
        dirtyDepthState = true;
    }

    void setColorWrite(bool r, bool g, bool b, bool a) {
        colorWrite = r && g && b && a;
        dirtyBlendState = true;
    }

    void setAlphaTest(bool enable) {}

    void setCullMode(int rsMask) {
        cullMode = cmNone;
        switch (rsMask) {
            case RS_CULL_BACK  : cullMode = cmBack;  break;
            case RS_CULL_FRONT : cullMode = cmFront; break;
        }
        deviceContext->RSSetState(RS[cullMode]);
    }

    void setBlendMode(int rsMask) {
        blendMode = bmNone;
        switch (rsMask) {
            case RS_BLEND_ALPHA   : blendMode = bmAlpha;   break;
            case RS_BLEND_ADD     : blendMode = bmAdd;     break;
            case RS_BLEND_MULT    : blendMode = bmMult;    break;
            case RS_BLEND_PREMULT : blendMode = bmPremult; break;
        }
        dirtyBlendState = true;
    }

    void setViewProj(const mat4 &mView, const mat4 &mProj) {}

    void updateLights(vec4 *lightPos, vec4 *lightColor, int count) {
        if (active.shader) {
            active.shader->setParam(uLightColor, lightColor[0], count);
            active.shader->setParam(uLightPos,   lightPos[0],   count);
        }
    }

    void DIP(Mesh *mesh, const MeshRange &range) {
        if (Core::active.shader) {
            Core::active.shader->validate();
        }

        if (dirtyDepthState) {
            deviceContext->OMSetDepthStencilState(DS[depthTest][depthWrite], 0);
            dirtyDepthState = false;
        }

        if (dirtyBlendState) {
            deviceContext->OMSetBlendState(BS[colorWrite][blendMode], NULL, 0xFFFFFFFF);
            dirtyBlendState = false;
        }

        deviceContext->DrawIndexed(range.iCount, range.iStart, range.vStart);
    }

    vec4 copyPixel(int x, int y) {
        /* TODO
        GAPI::Texture *t = Core::active.target;
        ASSERT(t && t->tex2D);

        LPDIRECT3DSURFACE9 surface, texSurface;
        D3DCHECK(t->tex2D->GetSurfaceLevel(0, &texSurface));
        D3DCHECK(device->CreateOffscreenPlainSurface(t->width, t->height, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &surface, NULL));
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
        return vec4(0.0f);
    }
}

#endif