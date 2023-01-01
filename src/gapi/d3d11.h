#ifndef H_GAPI_D3D11
#define H_GAPI_D3D11

#include "core.h"
#include <d3d11.h>

#define SAFE_RELEASE(P) if(P){P->Release(); P = NULL;}

#if 0 //defined(_DEBUG) || defined(PROFILE)
    #include <d3d9.h>

    struct Marker {
        Marker(const char *title) {
            wchar_t ws[128];
            swprintf(ws, sizeof(ws), L"%hs", title);
            D3DPERF_BeginEvent(0xFFFFFFFF, ws);
        }

        ~Marker() {
            D3DPERF_EndEvent();
        }

        static void setLabel(ID3D11DeviceChild *child, const char *label) {
            // TODO: use Windows 10 SDK
            //child->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(label), label);
        }
    };

    #define PROFILE_MARKER(title) Marker marker(title)
    #define PROFILE_LABEL(id, child, label) Marker::setLabel(child, label)
    #define PROFILE_TIMING(time)
#else
    #define PROFILE_MARKER(title)
    #define PROFILE_LABEL(id, child, label)
    #define PROFILE_TIMING(time)
#endif

#ifdef _OS_WP8
    extern Microsoft::WRL::ComPtr<ID3D11Device1>        osDevice;
    extern Microsoft::WRL::ComPtr<ID3D11DeviceContext1> osContext;
    extern Microsoft::WRL::ComPtr<IDXGISwapChain1>      osSwapChain;
#else
    extern ID3D11Device          *osDevice;
    extern ID3D11DeviceContext   *osContext;
    #ifdef _OS_XB1
        extern IDXGISwapChain1   *osSwapChain;
    #else
        extern IDXGISwapChain    *osSwapChain;
    #endif
#endif

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

    ID3D11BlendState        *BS[2][bmMAX]; // [colorWrite][blendMode] ONLY two colorWrite modes are supported (A and RGBA)
    ID3D11RasterizerState   *RS[cmMAX];    // [cullMode]
    ID3D11DepthStencilState *DS[2][2];     // [depthTest][depthWrite]

    enum {
        smpDefault,
        smpPoint,
        smpPointWrap,
        smpLinear,
        smpLinearWrap,
        smpCmp,
        smpMAX
    };

    ID3D11SamplerState      *samplers[smpMAX];

    ID3D11Texture2D         *stagingPixel;

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
        ID3D11InputLayout  *IL;
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
                    case SD_UNDERWATER : underwater = true; break;
                    case SD_ALPHA_TEST : alphatest  = true; break;
                }
            }

            int vSize, fSize;

            #define SHADER(S,P)    (P##Src = S##_##P, P##Size = sizeof(S##_##P))
            #define SHADER_A(S,P)  (alphatest  ? SHADER(S##_a,P) : SHADER(S,P))
            #define SHADER_U(S,P)  (underwater ? SHADER(S##_u,P) : SHADER(S,P))
            #define SHADER_AU(S,P) ((underwater && alphatest) ? SHADER(S##_au,P) : (alphatest ? SHADER(S##_a,P) : SHADER_U(S,P)))

            const uint8 *vSrc = NULL, *fSrc = NULL;
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
                case passSky   :
                    switch (type) {
                        case 0  : SHADER ( sky,     v );     SHADER ( sky,     f );    break;
                        case 1  : SHADER ( sky_clouds, v );  SHADER ( sky_clouds, f ); break;
                        case 2  : SHADER ( sky_azure, v );   SHADER ( sky_azure, f );  break;
                        default : ASSERT(false);
                    }
                    break;
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
                        case 5  : SHADER ( filter_anaglyph,   v );  SHADER ( filter_anaglyph,   f ); break;
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
            ret = osDevice->CreateVertexShader ((DWORD*)vSrc, vSize, NULL, &VS); ASSERT(ret == S_OK);
            ret = osDevice->CreatePixelShader  ((DWORD*)fSrc, fSize, NULL, &PS); ASSERT(ret == S_OK);

            const D3D11_INPUT_ELEMENT_DESC vertexDecl[] = {
                { "POSITION", 0, DXGI_FORMAT_R16G16B16A16_SINT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // aCoord
                { "NORMAL",   0, DXGI_FORMAT_R16G16B16A16_SINT, 0,  8, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // aNormal
                { "TEXCOORD", 0, DXGI_FORMAT_R16G16B16A16_SINT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // aTexCoord
                { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // aColor
                { "COLOR",    1, DXGI_FORMAT_R8G8B8A8_UNORM,    0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // aLight
            };

            ret = osDevice->CreateInputLayout(vertexDecl, COUNT(vertexDecl), vSrc, vSize, &IL);
            ASSERT(ret == S_OK);

            rebind = true;

            D3D11_BUFFER_DESC      desc;
            memset(&desc, 0, sizeof(desc));
            desc.Usage          = D3D11_USAGE_DYNAMIC;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
            desc.ByteWidth      = sizeof(cbMem);
            osDevice->CreateBuffer(&desc, NULL, &CB);
        }

        void deinit() {
            SAFE_RELEASE(CB);
            SAFE_RELEASE(IL);
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
                osContext->IASetInputLayout(IL);
                osContext->VSSetShader(VS, NULL, 0);
                osContext->PSSetShader(PS, NULL, 0);
                rebind = false;
            }

            D3D11_MAPPED_SUBRESOURCE mapped;
            osContext->Map(CB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
            memcpy(mapped.pData, cbMem, sizeof(cbMem));
            osContext->Unmap(CB, 0);

            osContext->VSSetConstantBuffers(0, 1, &CB);
            osContext->PSSetConstantBuffers(0, 1, &CB);
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
        union {
            ID3D11Resource       *ID;
            ID3D11Texture2D      *tex2D;
            ID3D11Texture3D      *tex3D;
        };
        ID3D11ShaderResourceView *SRV;
        ID3D11RenderTargetView   *RTV[6];
        ID3D11DepthStencilView   *DSV;

        int       width, height, depth, origWidth, origHeight, origDepth;
        TexFormat fmt;
        uint32    opt;

        Texture(int width, int height, int depth, uint32 opt) : ID(NULL), SRV(NULL), DSV(NULL), width(width), height(height), depth(depth), origWidth(width), origHeight(height), origDepth(depth), fmt(FMT_RGBA), opt(opt) {
            memset(RTV, 0, sizeof(RTV));
        }

        void init(void *data) {
            ASSERT((opt & OPT_PROXY) == 0);

            bool mipmaps   = (opt & OPT_MIPMAPS) != 0;
            bool isDepth   = fmt == FMT_DEPTH || fmt == FMT_SHADOW;
            bool isCube    = (opt & OPT_CUBEMAP) != 0;
            bool isVolume  = (opt & OPT_VOLUME)  != 0;
            bool isTarget  = (opt & OPT_TARGET)  != 0;
            bool isDynamic = (opt & OPT_DYNAMIC) != 0;

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
                osDevice->CreateTexture3D(&desc, data ? initialData : NULL, &tex3D);
                ASSERT(tex3D);

                descSRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
                osDevice->CreateShaderResourceView(tex3D, &descSRV, &SRV);
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

                osDevice->CreateTexture2D(&desc, data ? initialData : NULL, &tex2D);
                ASSERT(tex2D);

                if (isTarget) {
                    if (isDepth) {
                        D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
                        memset(&descDSV, 0, sizeof(descDSV));
                        descDSV.Format        = DXGI_FORMAT_D16_UNORM;
                        descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                        osDevice->CreateDepthStencilView(tex2D, &descDSV, &DSV);
                        ASSERT(DSV);
                    } else {
                        D3D11_RENDER_TARGET_VIEW_DESC descRTV;
                        memset(&descRTV, 0, sizeof(descRTV));
                        descRTV.Format                   = desc.Format;
                        descRTV.ViewDimension            = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                        descRTV.Texture2DArray.ArraySize = 1;

                        for (int i = 0; i < 6; i++) {
                            descRTV.Texture2DArray.FirstArraySlice = i;
                            osDevice->CreateRenderTargetView(tex2D, &descRTV, &RTV[i]);
                            ASSERT(RTV[i]);
                            if (!isCube) break;
                        }
                    }
                }

                descSRV.ViewDimension = isCube ? D3D11_SRV_DIMENSION_TEXTURECUBE : D3D11_SRV_DIMENSION_TEXTURE2D;
                osDevice->CreateShaderResourceView(tex2D, &descSRV, &SRV);
            }

            ASSERT(SRV);
        }

        void deinit() {
            SAFE_RELEASE(tex2D);
            SAFE_RELEASE(tex3D);
            SAFE_RELEASE(SRV);
            for (int i = 0; i < 6; i++) {
                SAFE_RELEASE(RTV[i]);
            }
            SAFE_RELEASE(DSV);
        }

        void generateMipMap() {
            ASSERT(SRV && tex2D);
            osContext->GenerateMips(SRV);
        }

        void update(void *data) {
            ASSERT(tex2D);
            ASSERT(opt & OPT_DYNAMIC);
            D3D11_MAPPED_SUBRESOURCE mapped;
            osContext->Map(tex2D, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
            memcpy(mapped.pData, data, mapped.RowPitch * height);
            osContext->Unmap(tex2D, 0);
        }

        void bind(int sampler) {
            if (opt & OPT_PROXY) return;
            ASSERT(tex2D || tex3D);

            if (Core::active.textures[sampler] != this) {
                Core::active.textures[sampler] = this;

            #ifndef _OS_WP8
                if (opt & OPT_VERTEX) {
                    osContext->VSSetShaderResources(sampler, 1, &SRV);
                }
            #endif

                osContext->PSSetShaderResources(sampler, 1, &SRV);
            }
        }

        void unbind(int sampler) {
            if (Core::active.textures[sampler]) {
                Core::active.textures[sampler] = NULL;

                ID3D11ShaderResourceView *none = NULL;

                if (opt & OPT_VERTEX) {
                    osContext->VSSetShaderResources(sampler, 1, &none);
                }
                osContext->PSSetShaderResources(sampler, 1, &none);
            }
        }

        void setFilterQuality(int value) {}
    };

// Mesh
    struct Mesh {
        ID3D11Buffer *ID[2];

        int  iCount;
        int  vCount;
        bool dynamic;

        Mesh(bool dynamic) : dynamic(dynamic) {
            ID[0] = ID[1] = NULL;
        }

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
            osDevice->CreateBuffer(&desc, dynamic ? NULL : &initData, &ID[0]);

            desc.BindFlags   = D3D11_BIND_VERTEX_BUFFER;
            desc.ByteWidth   = vCount * sizeof(Vertex);
            initData.pSysMem = vertices;
            osDevice->CreateBuffer(&desc, dynamic ? NULL : &initData, &ID[1]);
        }

        void deinit() {
            SAFE_RELEASE(ID[0]);
            SAFE_RELEASE(ID[1]);
        }

        void update(Index *indices, int iCount, ::Vertex *vertices, int vCount) {
            ASSERT(sizeof(GAPI::Vertex) == sizeof(::Vertex));

            D3D11_MAPPED_SUBRESOURCE mapped;
         
            if (indices && iCount) {
                osContext->Map(ID[0], 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
                memcpy(mapped.pData, indices, iCount * sizeof(indices[0]));
                osContext->Unmap(ID[0], 0);
            }

            if (vertices && vCount) {
                osContext->Map(ID[1], 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
                memcpy(mapped.pData, vertices, vCount * sizeof(vertices[0]));
                osContext->Unmap(ID[1], 0);
            }
        }

        void bind(const MeshRange &range) const {
            UINT stride = sizeof(Vertex);
            UINT offset = 0;//range.vStart * stride;
            osContext->IASetIndexBuffer(ID[0], sizeof(Index) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT, 0);
            osContext->IASetVertexBuffers(0, 1, &ID[1], &stride, &offset);
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
            desc.Filter         = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
            desc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
            desc.BorderColor[0] =
            desc.BorderColor[1] =
            desc.BorderColor[2] =
            desc.BorderColor[3] = 1.0f;
        } else {
            desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        }

        desc.AddressU =
        desc.AddressV =
        desc.AddressW = cmp ? D3D11_TEXTURE_ADDRESS_BORDER : (wrap ? D3D11_TEXTURE_ADDRESS_WRAP : D3D11_TEXTURE_ADDRESS_CLAMP);
        desc.MinLOD   = 0;
        desc.MaxLOD   = D3D11_FLOAT32_MAX;

        ID3D11SamplerState *sampler;
        osDevice->CreateSamplerState(&desc, &sampler);
        return sampler;
    }

    void initSamplers() {
        deinitSamplers();
        samplers[smpDefault]    = initSampler(true,  true,  false, false); // TODO settings dependent
        samplers[smpPoint]      = initSampler(false, false, false, false);
        samplers[smpPointWrap]  = initSampler(false, false, true,  false);
        samplers[smpLinear]     = initSampler(true,  false, false, false);
        samplers[smpLinearWrap] = initSampler(true,  false, true,  false);
        samplers[smpCmp]        = initSampler(true,  false, false, true);
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

        #ifdef _OS_WP8
            support.depthTexture   = false;
            support.shadowSampler  = false;
            support.colorFloat     = true;
            support.colorHalf      = true;
            support.texFloatLinear = true;
            support.texFloat       = true;
            support.texHalfLinear  = true;
            support.texHalf        = true;
            support.tex3D          = false;
        #endif

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
                osDevice->CreateBlendState(&desc, &BS[i][B])

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
            desc.ScissorEnable         = TRUE;
            desc.FrontCounterClockwise = TRUE;
            desc.DepthClipEnable       = TRUE;
            desc.FillMode = D3D11_FILL_SOLID;
            desc.CullMode = D3D11_CULL_NONE;
            osDevice->CreateRasterizerState(&desc, &RS[cmNone]);
            desc.CullMode = D3D11_CULL_BACK;
            osDevice->CreateRasterizerState(&desc, &RS[cmBack]);
            desc.CullMode = D3D11_CULL_FRONT;
            osDevice->CreateRasterizerState(&desc, &RS[cmFront]);
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
                    osDevice->CreateDepthStencilState(&desc, &DS[i][j]);
                }
            }
        }

    // init samplers
        memset(samplers, 0, sizeof(samplers));
        initSamplers();

    // init staging texture for copyPixel
        D3D11_TEXTURE2D_DESC desc;
        memset(&desc, 0, sizeof(desc));
        desc.Width            = 1;
        desc.Height           = 1;
        desc.MipLevels        = 1;
        desc.ArraySize        = 1;
        desc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage            = D3D11_USAGE_STAGING;
        desc.MiscFlags        = 0;
        desc.CPUAccessFlags   = D3D11_CPU_ACCESS_READ;
        osDevice->CreateTexture2D(&desc, NULL, &stagingPixel);
    }

    void resetDevice() {
        SAFE_RELEASE(defRTV);
        SAFE_RELEASE(defDSV);

        for (int i = 0; i < rtCache.count; i++) {
            SAFE_RELEASE(rtCache.items[i].RTV);
            SAFE_RELEASE(rtCache.items[i].DSV);
        }

        rtCache.count = 0;
    }

    void deinit() {
        resetDevice();

        deinitSamplers();

        SAFE_RELEASE(stagingPixel);

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

    inline mat4::ProjRange getProjRange() {
        return mat4::PROJ_ZERO_POS;
    }

    mat4 ortho(float l, float r, float b, float t, float znear, float zfar) {
        mat4 m;
        m.ortho(getProjRange(), l, r, b, t, znear, zfar);

        #ifdef _OS_WP8
            m.rot90();
        #endif

        return m;
    }

    mat4 perspective(float fov, float aspect, float znear, float zfar, float eye) {
        mat4 m;

        #ifdef _OS_WP8
            aspect = 1.0f / aspect;
        #endif

        m.perspective(getProjRange(), fov, aspect, znear, zfar, eye);

        #ifdef _OS_WP8
            m.rot90();
        #endif

        return m;
    }

    bool beginFrame() {
        if (!defRTV) {
            ID3D11Texture2D *pBackBuffer = NULL;
            osSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
            osDevice->CreateRenderTargetView(pBackBuffer, NULL, &defRTV);
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
            osDevice->CreateTexture2D(&desc, NULL, &dsTex);

            D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
            memset(&descDSV, 0, sizeof(descDSV));
            descDSV.Format             = desc.Format;
            descDSV.ViewDimension      = D3D11_DSV_DIMENSION_TEXTURE2D;
            descDSV.Texture2D.MipSlice = 0;
            osDevice->CreateDepthStencilView(dsTex, &descDSV, &defDSV);

            SAFE_RELEASE(dsTex);
        }

        osContext->OMSetRenderTargets(1, &defRTV, defDSV);

        return true;
    }

    void endFrame() {
        //
    }

    void resetState() {
        osContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        osContext->RSSetState(RS[cmNone]);
        depthTest = depthWrite = dirtyDepthState = true;
        colorWrite = dirtyBlendState = true;

    #ifndef _OS_WP8
        osContext->VSSetSamplers(0, COUNT(samplers), samplers);
    #endif

        osContext->PSSetSamplers(0, COUNT(samplers), samplers);
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
                osDevice->CreateTexture2D(&desc, NULL, &tex2D);
                osDevice->CreateRenderTargetView(tex2D, NULL, &rtCache.items[index].RTV);
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
                osDevice->CreateTexture2D(&desc, NULL, &tex2D);

                D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
                memset(&descDSV, 0, sizeof(descDSV));
                descDSV.Format        = DXGI_FORMAT_D16_UNORM;
                descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                osDevice->CreateDepthStencilView(tex2D, &descDSV, &rtCache.items[index].DSV);
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

            if (target->RTV[face]) {
                RTV = target->RTV[face];
                cacheRenderTarget(NULL, &DSV, target->width, target->height);
            } else if (target->DSV) {
                DSV = target->DSV;
                cacheRenderTarget(&RTV, NULL, target->width, target->height);
            } else {
                ASSERT(false);
            }
        }

        osContext->OMSetRenderTargets(1, &RTV, DSV);

        Core::active.viewport = short4(0, 0, 0, 0); // forcing viewport reset
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

        osContext->OMGetRenderTargets(1, &RTV, NULL);
        RTV->GetResource(&res);

        osContext->CopySubresourceRegion(dst->tex2D, 0, xOffset, yOffset, 0, res, 0, &box);

        SAFE_RELEASE(RTV);
        SAFE_RELEASE(res);
    }

    void setVSync(bool enable) {}
    void waitVBlank() {}

    void clear(bool color, bool depth) {
        ID3D11RenderTargetView *RTV = NULL;
        ID3D11DepthStencilView *DSV = NULL;

        osContext->OMGetRenderTargets(1, &RTV, &DSV);

        if (color && RTV) {
            osContext->ClearRenderTargetView(RTV, (FLOAT*)&clearColor);
        }

        if (depth && DSV) {
            osContext->ClearDepthStencilView(DSV, D3D11_CLEAR_DEPTH, 1.0, 0);
        }

        SAFE_RELEASE(RTV);
        SAFE_RELEASE(DSV);
    }

    void setClearColor(const vec4 &color) {
        clearColor = color;
    }

    void setViewport(const short4 &v) {
        D3D11_VIEWPORT viewport;
        viewport.TopLeftX = (FLOAT)v.x;
        viewport.TopLeftY = (FLOAT)v.y;
        viewport.Width    = (FLOAT)v.z;
        viewport.Height   = (FLOAT)v.w;
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;

        osContext->RSSetViewports(1, &viewport);
    }

    void setScissor(const short4 &s) {
        D3D11_RECT scissor;
        scissor.left   = s.x;
        scissor.top    = active.viewport.w - (s.y + s.w);
        scissor.right  = s.x + s.z;
        scissor.bottom = active.viewport.w - s.y;

        osContext->RSSetScissorRects(1, &scissor);
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
        osContext->RSSetState(RS[cullMode]);
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
            osContext->OMSetDepthStencilState(DS[depthTest][depthWrite], 0);
            dirtyDepthState = false;
        }

        if (dirtyBlendState) {
            osContext->OMSetBlendState(BS[colorWrite][blendMode], NULL, 0xFFFFFFFF);
            dirtyBlendState = false;
        }

        osContext->DrawIndexed(range.iCount, range.iStart, range.vStart);
    }

    vec4 copyPixel(int x, int y) {
        D3D11_BOX srcBox;
        srcBox.left   = x;
        srcBox.top    = y;
        srcBox.right  = x + 1;
        srcBox.bottom = y + 1;
        srcBox.front  = 0;
        srcBox.back   = 1;

        ASSERT(Core::active.target);
        osContext->CopySubresourceRegion(stagingPixel, 0, 0, 0, 0, Core::active.target->tex2D, 0, &srcBox);

        D3D11_MAPPED_SUBRESOURCE res;
        osContext->Map(stagingPixel, 0, D3D11_MAP_READ, 0, &res);
        ASSERT(res.pData);
        Color32 c = *((Color32*)res.pData);
        osContext->Unmap(stagingPixel, 0);

        return vec4(float(c.r), float(c.g), float(c.b), float(c.a)) * (1.0f / 255.0f);
    }
}

#endif