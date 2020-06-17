#ifndef H_GAPI_VK
#define H_GAPI_VK

#include "core.h"
#include <vulkan/vulkan.h>

#if defined(_DEBUG) || defined(PROFILE)
    struct Marker {
        Marker(const char *title) {
            //
        }

        ~Marker() {
            //
        }

        static void setLabel(void *object, const char *label) {
            // TODO: use Windows 10 SDK
        }
    };

    #define PROFILE_MARKER(title) Marker marker##__LINE__(title)
    #define PROFILE_LABEL(id, child, label) Marker::setLabel(child, label)
    #define PROFILE_TIMING(time)
#else
    #define PROFILE_MARKER(title)
    #define PROFILE_LABEL(id, child, label)
    #define PROFILE_TIMING(time)
#endif

#define VK_INIT(s)          memset(&s, 0, sizeof(s))
#define VK_INIT_STRUCT(s,t) VK_INIT(s); s.sType = t
#define VK_CHECK(x)  { VkResult r = (x); if (r != VK_SUCCESS) { LOG("! %s = %d\n", #x, (int)r); ASSERT(r != VK_SUCCESS); } }

namespace GAPI {
    using namespace Core;

    typedef ::Vertex Vertex;

    enum {
        smpDefault,
        smpPoint,
        smpPointWrap,
        smpLinear,
        smpLinearWrap,
        smpCmp,
        smpMAX
    };

    VkDevice    device;
    VkInstance  instance;

    CullMode    cullMode;
    BlendMode   blendMode;
    vec4        clearColor;
    VkSampler   samplers[smpMAX];

// Shader
    //#include "shaders/vulkan/shaders.h"

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
        98, // uContacts
    };

    struct Shader {
        VkShaderModule VS;
        VkShaderModule PS;

        Shader() : VS(VK_NULL_HANDLE), PS(VK_NULL_HANDLE) {}

        void init(Core::Pass pass, int type, int *def, int defCount) {
            //
        }

        void deinit() {
            vkDestroyShaderModule(device, VS, NULL);
            vkDestroyShaderModule(device, PS, NULL);
        }

        void bind() {
            //
        }

        void setParam(UniformType uType, float *value, int count) {
            //
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
        VkImage     image;
        VkImageView imageView;

        int       width, height, depth, origWidth, origHeight, origDepth;
        TexFormat fmt;
        uint32    opt;

        Texture(int width, int height, int depth, uint32 opt) : image(VK_NULL_HANDLE), imageView(VK_NULL_HANDLE), width(width), height(height), depth(depth), origWidth(width), origHeight(height), origDepth(depth), fmt(FMT_RGBA), opt(opt) {
            //
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
                VkFormat    format;
            } formats[FMT_MAX] = {
                {   8, VK_FORMAT_R8_UNORM              },
                {  32, VK_FORMAT_R8G8B8A8_UNORM        },
                {  16, VK_FORMAT_B5G6R5_UNORM_PACK16   },
                {  16, VK_FORMAT_B5G5R5A1_UNORM_PACK16 },
                {  64, VK_FORMAT_R32G32_SFLOAT         },
                {  32, VK_FORMAT_R16G16_SFLOAT         },
                {  16, VK_FORMAT_R16_UNORM             },
                {  16, VK_FORMAT_R16_UNORM             },
            };

            //
        }

        void deinit() {
            vkDestroyImage(device, image, NULL);
            vkDestroyImageView(device, imageView, NULL);
        }

        void generateMipMap() {
            //
        }

        void update(void *data) {
            //
        }

        void bind(int sampler) {
            if (opt & OPT_PROXY) return;
            //
        }

        void unbind(int sampler) {
            //
        }

        void setFilterQuality(int value) {
            //
        }
    };

// Mesh
    struct Mesh {
        VkBuffer IB;
        VkBuffer VB;

        int  iCount;
        int  vCount;
        bool dynamic;

        Mesh(bool dynamic) : IB(VK_NULL_HANDLE), VB(VK_NULL_HANDLE), dynamic(dynamic) {
            //
        }

        void init(Index *indices, int iCount, ::Vertex *vertices, int vCount, int aCount) {
            this->iCount = iCount;
            this->vCount = vCount;
            ASSERT(sizeof(GAPI::Vertex) == sizeof(::Vertex));
            //
        }

        void deinit() {
            vkDestroyBuffer(device, IB, NULL);
            vkDestroyBuffer(device, VB, NULL);
        }

        void update(Index *indices, int iCount, ::Vertex *vertices, int vCount) {
            ASSERT(sizeof(GAPI::Vertex) == sizeof(::Vertex));
         
            if (indices && iCount) {
                //
            }

            if (vertices && vCount) {
                //
            }
        }

        void bind(const MeshRange &range) const {
            //
        }

        void initNextRange(MeshRange &range, int &aIndex) const {
            range.aIndex = -1;
        }
    };

//    GLuint FBO, defaultFBO;
    struct RenderTargetCache {
        int count;
        struct Item {
            int width;
            int height;
        } items[MAX_RENDER_BUFFERS];
    } rtCache;

    void deinitSamplers() {
        for (int i = 0; i < COUNT(samplers); i++) {
            //vkDestroySampler(device, samplers[i], NULL);
        }
    }

    VkSampler initSampler(bool filter, bool aniso, bool wrap, bool cmp) {
        VkSamplerCreateInfo createInfo;
        VK_INIT_STRUCT(createInfo, VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO);

        if (aniso && (Core::support.maxAniso > 0)) {
            createInfo.maxAnisotropy = min(int(Core::support.maxAniso), 8);
        } else {
            createInfo.maxAnisotropy = 1;
        }

        createInfo.anisotropyEnable = (createInfo.maxAnisotropy > 1) ? VK_TRUE : VK_FALSE;

        if (filter) {
            createInfo.minFilter  = VK_FILTER_LINEAR;
            createInfo.magFilter  = VK_FILTER_LINEAR;
            createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        } else {
            createInfo.minFilter  = VK_FILTER_LINEAR;
            createInfo.magFilter  = VK_FILTER_LINEAR;
            createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        }

        if (cmp) {
            createInfo.compareEnable = VK_TRUE;
            createInfo.compareOp     = VK_COMPARE_OP_LESS;
            createInfo.borderColor   = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        }

        createInfo.addressModeU =
        createInfo.addressModeV =
        createInfo.addressModeW = cmp ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER : (wrap ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
        createInfo.minLod = 0.0f;
        createInfo.maxLod = 0.0f;

        VkSampler sampler = VK_NULL_HANDLE;
        //VK_CHECK(vkCreateSampler(device, &createInfo, NULL, &sampler));
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
        memset(samplers, 0, sizeof(samplers));
        
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

    // init samplers
        memset(samplers, 0, sizeof(samplers));
        initSamplers();
    }

    void resetDevice() {
        rtCache.count = 0;
    }

    void deinit() {
        resetDevice();

        deinitSamplers();
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
        return true;
    }

    void endFrame() {
        //
    }

    void resetState() {
        //
    }

    void cacheRenderTarget(VkImageView **RTV, VkImageView **DSV, int width, int height) {
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

        //
    }

    void bindTarget(Texture *target, int face) {
        //
    }

    void discardTarget(bool color, bool depth) {}

    void copyTarget(Texture *dst, int xOffset, int yOffset, int x, int y, int width, int height) {
        //
    }

    void setVSync(bool enable) {}
    void waitVBlank() {}

    void clear(bool color, bool depth) {
        //
    }

    void setClearColor(const vec4 &color) {
        clearColor = color;
    }

    void setViewport(const short4 &v) {
        //
    }

    void setScissor(const short4 &s) {
        //
    }

    void setDepthTest(bool enable) {
        //
    }

    void setDepthWrite(bool enable) {
        //
    }

    void setColorWrite(bool r, bool g, bool b, bool a) {
        //
    }

    void setAlphaTest(bool enable) {}

    void setCullMode(int rsMask) {
        //
    }

    void setBlendMode(int rsMask) {
        //
    }

    void setViewProj(const mat4 &mView, const mat4 &mProj) {}

    void updateLights(vec4 *lightPos, vec4 *lightColor, int count) {
        //if (active.shader) {
        //    active.shader->setParam(uLightColor, lightColor[0], count);
        //    active.shader->setParam(uLightPos,   lightPos[0],   count);
        //}
    }

    void DIP(Mesh *mesh, const MeshRange &range) {
        //
    }

    vec4 copyPixel(int x, int y) {
        return vec4(0.0f);
    }
}

#endif