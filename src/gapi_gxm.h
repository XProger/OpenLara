#ifndef H_GAPI_GXM
#define H_GAPI_GXM

#include "core.h"

#include <psp2/display.h>
#include <psp2/gxm.h>
#include <psp2/gxt.h>

#define PROFILE_MARKER(title)
#define PROFILE_LABEL(id, name, label)
#define PROFILE_TIMING(time)

#define ALIGN(x, a) (((x) + ((a) - 1)) & ~((a) - 1))

#define DISPLAY_WIDTH           960
#define DISPLAY_HEIGHT          544
#define DISPLAY_STRIDE          1024
#define DISPLAY_BUFFER_COUNT    2
#define DISPLAY_COLOR_FORMAT    SCE_GXM_COLOR_FORMAT_A8B8G8R8
#define DISPLAY_PIXEL_FORMAT    SCE_DISPLAY_PIXELFORMAT_A8B8G8R8

namespace GAPI {
    #include "shaders/gxm/compose_vp.h"
    #include "shaders/gxm/compose_fp.h"
    #include "shaders/gxm/shadow_vp.h"
    #include "shaders/gxm/shadow_fp.h"
    #include "shaders/gxm/ambient_vp.h"
    #include "shaders/gxm/ambient_fp.h"
    #include "shaders/gxm/water_vp.h"
    #include "shaders/gxm/water_fp.h"
    #include "shaders/gxm/filter_vp.h"
    #include "shaders/gxm/filter_fp.h"
    #include "shaders/gxm/gui_vp.h"
    #include "shaders/gxm/gui_fp.h"
    #include "shaders/gxm/clear_vp.h"
    #include "shaders/gxm/clear_fp.h"

    #define SHADER_BUFF_SIZE    (64 * 1024)
    #define SHADER_VERT_SIZE    (64 * 1024)
    #define SHADER_FRAG_SIZE    (64 * 1024)

    SceGxmShaderPatcher *shaderPatcher;
    SceUID shaderBuffUID, shaderVertUID, shaderFragUID;
    void *shaderBuffPtr, *shaderVertPtr, *shaderFragPtr;

    using namespace Core;

    #define PASS_CLEAR Core::Pass(0xFF)

    typedef ::Vertex Vertex;

    struct DisplayData {
        void *addr;
    };

    void display_queue_callback(const void *callbackData) {
        SceDisplayFrameBuf display_fb;
        const DisplayData *cb_data = (DisplayData*)callbackData;

        memset(&display_fb, 0, sizeof(display_fb));
        display_fb.size        = sizeof(display_fb);
        display_fb.base        = cb_data->addr;
        display_fb.pitch       = DISPLAY_STRIDE;
        display_fb.pixelformat = DISPLAY_PIXEL_FORMAT;
        display_fb.width       = DISPLAY_WIDTH;
        display_fb.height      = DISPLAY_HEIGHT;

        sceDisplaySetFrameBuf(&display_fb, SCE_DISPLAY_SETBUF_NEXTFRAME);

        if (Core::settings.detail.vsync) {
            sceDisplayWaitVblankStart();
        }
    }

    namespace Context {
        SceGxmContext *gxmContext;

        SceUID vdmRingBufferUID;
        SceUID vertexRingBufferUID;
        SceUID fragmentRingBufferUID;
        SceUID fragmentUsseRingBufferUID;

        void *vdmRingBuffer;
        void *vertexRingBuffer;
        void *fragmentRingBuffer;
        void *fragmentUsseRingBuffer;

        struct PendingResource {
            int    frameIndex;
            SceUID uid;
        };

        Array<PendingResource> pendings;

        void* allocCPU(void *user_data, unsigned int size) {
            return malloc(size);
        }

        void freeCPU(void *user_data, void *mem) {
            return free(mem);
        }

        void* allocGPU(SceKernelMemBlockType type, unsigned int size, SceGxmMemoryAttribFlags attribs, SceUID *uid) {
            void *mem;

            if (type == SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW) {
                size = ALIGN(size, 256 * 1024);
            } else {
                size = ALIGN(size, 4 * 1024);
            }

            *uid = sceKernelAllocMemBlock("gpu_mem", type, size, NULL);

            if (sceKernelGetMemBlockBase(*uid, &mem) < 0)
                return NULL;
            if (sceGxmMapMemory(mem, size, attribs) < 0)
                return NULL;

            return mem;
        }

        void freeGPU(SceUID uid, bool pending = false) {
            if (pending) {
                PendingResource res;
                res.frameIndex = Core::stats.frameIndex;
                res.uid        = uid;
                pendings.push(res);
                return;
            }

            void *mem = NULL;
            if (sceKernelGetMemBlockBase(uid, &mem) < 0)
                return;

            SceKernelMemBlockInfo Info;
            Info.size = sizeof(Info);
            sceKernelGetMemBlockInfoByAddr(mem, &Info);

            sceGxmUnmapMemory(mem);
            sceKernelFreeMemBlock(uid);
        }

        void* allocVertexUSSE(unsigned int size, SceUID *uid, unsigned int *usse_offset) {
            void *mem = NULL;

            size = ALIGN(size, 4096);

            *uid = sceKernelAllocMemBlock("vertex_usse", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, size, NULL);

            if (sceKernelGetMemBlockBase(*uid, &mem) < 0)
                return NULL;
            if (sceGxmMapVertexUsseMemory(mem, size, usse_offset) < 0)
                return NULL;

            return mem;
        }

        void freeVertexUSSE(SceUID uid) {
            void *mem = NULL;
            if (sceKernelGetMemBlockBase(uid, &mem) < 0)
                return;

            SceKernelMemBlockInfo Info;
            Info.size = sizeof(Info);
            sceKernelGetMemBlockInfoByAddr(mem, &Info);

            sceGxmUnmapVertexUsseMemory(mem);
            sceKernelFreeMemBlock(uid);
        }

        void* allocFragmentUSSE(unsigned int size, SceUID *uid, unsigned int *usse_offset) {
            void *mem = NULL;

            size = ALIGN(size, 4096);

            *uid = sceKernelAllocMemBlock("fragment_usse", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, size, NULL);

            if (sceKernelGetMemBlockBase(*uid, &mem) < 0)
                return NULL;
            if (sceGxmMapFragmentUsseMemory(mem, size, usse_offset) < 0)
                return NULL;

            return mem;
        }

        void freeFragmentUSSE(SceUID uid) {
            void *mem = NULL;
            if (sceKernelGetMemBlockBase(uid, &mem) < 0)
                return;

            SceKernelMemBlockInfo Info;
            Info.size = sizeof(Info);
            sceKernelGetMemBlockInfoByAddr(mem, &Info);

            sceGxmUnmapFragmentUsseMemory(mem);
            sceKernelFreeMemBlock(uid);
        }

        void init() {
            void *vdmRingBuffer = allocGPU(
                SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, SCE_GXM_DEFAULT_VDM_RING_BUFFER_SIZE,
                SCE_GXM_MEMORY_ATTRIB_READ, &vdmRingBufferUID);

            void *vertexRingBuffer = allocGPU(
                SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, SCE_GXM_DEFAULT_VERTEX_RING_BUFFER_SIZE,
                SCE_GXM_MEMORY_ATTRIB_READ, &vertexRingBufferUID);

            void *fragmentRingBuffer = allocGPU(
                SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, SCE_GXM_DEFAULT_FRAGMENT_RING_BUFFER_SIZE,
                SCE_GXM_MEMORY_ATTRIB_READ, &fragmentRingBufferUID);

            unsigned int offset;
            void *fragmentUsseRingBuffer = allocFragmentUSSE(SCE_GXM_DEFAULT_FRAGMENT_USSE_RING_BUFFER_SIZE,
                &fragmentUsseRingBufferUID, &offset);

            SceGxmContextParams params;
            memset(&params, 0, sizeof(params));
            params.hostMem                       = malloc(SCE_GXM_MINIMUM_CONTEXT_HOST_MEM_SIZE);
            params.hostMemSize                   = SCE_GXM_MINIMUM_CONTEXT_HOST_MEM_SIZE;
            params.vdmRingBufferMem              = vdmRingBuffer;
            params.vdmRingBufferMemSize          = SCE_GXM_DEFAULT_VDM_RING_BUFFER_SIZE;
            params.vertexRingBufferMem           = vertexRingBuffer;
            params.vertexRingBufferMemSize       = SCE_GXM_DEFAULT_VERTEX_RING_BUFFER_SIZE;
            params.fragmentRingBufferMem         = fragmentRingBuffer;
            params.fragmentRingBufferMemSize     = SCE_GXM_DEFAULT_FRAGMENT_RING_BUFFER_SIZE;
            params.fragmentUsseRingBufferMem     = fragmentUsseRingBuffer;
            params.fragmentUsseRingBufferMemSize = SCE_GXM_DEFAULT_FRAGMENT_USSE_RING_BUFFER_SIZE;
            params.fragmentUsseRingBufferOffset  = offset;

            sceGxmCreateContext(&params, &gxmContext);
        }

        void deinit() {
            for (int i = 0; i < pendings.length; i++)
                freeGPU(pendings[i].uid);
            pendings.clear();
            freeGPU(vdmRingBufferUID);
            freeGPU(vertexRingBufferUID);
            freeGPU(fragmentRingBufferUID);
            freeFragmentUSSE(fragmentUsseRingBufferUID);
            sceGxmDestroyContext(gxmContext);
        }

        void checkPendings() {
            int i = 0;
            while (i < pendings.length) {
                if (pendings[i].frameIndex + DISPLAY_BUFFER_COUNT <= Core::stats.frameIndex)
                    pendings.removeFast(i);
                else
                    i++;
            }
        }
    };

    namespace SwapChain {

        struct DepthBuffer {
            SceUID                    uid;
            SceGxmDepthStencilSurface surface;
            void                      *data;
        } depthBuffer;

        struct ColorBuffer {
            SceUID             uid;
            SceGxmColorSurface surface;
            void               *data;
            SceGxmSyncObject   *syncObj;
        } colorBuffers[DISPLAY_BUFFER_COUNT];

        uint32 bufferIndex, bufferIndexLast;
        SceGxmRenderTarget *defaultTarget;

        void init() {
            bufferIndex = bufferIndexLast = 0;

            SceGxmRenderTargetParams params;
            memset(&params, 0, sizeof(params));
            params.width           = DISPLAY_WIDTH;
            params.height          = DISPLAY_HEIGHT;
            params.scenesPerFrame  = 1;
            params.multisampleMode = SCE_GXM_MULTISAMPLE_NONE;
            params.driverMemBlock  = -1;

            sceGxmCreateRenderTarget(&params, &defaultTarget);

            for (int i = 0; i < DISPLAY_BUFFER_COUNT; i++) {
                ColorBuffer &color = colorBuffers[i];

                color.data = Context::allocGPU(
                    SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,
                    4 * DISPLAY_STRIDE * DISPLAY_HEIGHT,
                    SCE_GXM_MEMORY_ATTRIB_RW,
                    &color.uid);

                sceGxmColorSurfaceInit(&color.surface,
                    DISPLAY_COLOR_FORMAT,
                    SCE_GXM_COLOR_SURFACE_LINEAR,
                    SCE_GXM_COLOR_SURFACE_SCALE_NONE,
                    SCE_GXM_OUTPUT_REGISTER_SIZE_32BIT,
                    DISPLAY_WIDTH,
                    DISPLAY_HEIGHT,
                    DISPLAY_STRIDE,
                    color.data);

                sceGxmSyncObjectCreate(&color.syncObj);
            }

            uint32 dsWidth   = ALIGN(DISPLAY_WIDTH,  SCE_GXM_TILE_SIZEX);
            uint32 dsHeight  = ALIGN(DISPLAY_HEIGHT, SCE_GXM_TILE_SIZEY);

            depthBuffer.data = Context::allocGPU(
                SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE,
                4 * dsWidth * dsHeight,
                SCE_GXM_MEMORY_ATTRIB_RW,
                &depthBuffer.uid);

            sceGxmDepthStencilSurfaceInit(&depthBuffer.surface,
                SCE_GXM_DEPTH_STENCIL_FORMAT_S8D24, SCE_GXM_DEPTH_STENCIL_SURFACE_TILED,
                dsWidth, depthBuffer.data, NULL);
        }

        void deinit() {
            Context::freeGPU(depthBuffer.uid);

            for (int i = 0; i < DISPLAY_BUFFER_COUNT; i++) {
                ColorBuffer &color = colorBuffers[i];
                Context::freeGPU(color.uid);
                sceGxmSyncObjectDestroy(color.syncObj);
            }
            sceGxmDestroyRenderTarget(defaultTarget);
        }

        SceGxmSyncObject *getSyncObj() {
            return colorBuffers[bufferIndex].syncObj;
        }

        SceGxmSyncObject *getLastSyncObj() {
            return colorBuffers[bufferIndexLast].syncObj;
        }

        SceGxmColorSurface* getColorSurface() {
            return &colorBuffers[bufferIndex].surface;
        }

        void* getColorSurfaceData() {
            return colorBuffers[bufferIndex].data;
        }

        SceGxmDepthStencilSurface* getDepthSurface() {
            return &depthBuffer.surface;
        }

        void present() {
            sceGxmPadHeartbeat(getColorSurface(), getSyncObj());

            DisplayData displayData;
            displayData.addr = getColorSurfaceData();
            sceGxmDisplayQueueAddEntry(getLastSyncObj(), getSyncObj(), &displayData);

            bufferIndexLast = bufferIndex;
            bufferIndex = (bufferIndex + 1) % DISPLAY_BUFFER_COUNT;
        }
    };

// Shader
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

    struct Shader {
        SceGxmVertexProgram    *vp;
        SceGxmFragmentProgram  *fp;
        SceGxmProgram          *vpPtr, *fpPtr;
        SceGxmShaderPatcherId  vpUID, fpUID;

        const SceGxmProgramParameter *vParams[uMAX];
        const SceGxmProgramParameter *fParams[uMAX];

        vec4  cbMem[98 + MAX_CONTACTS];
        int   cbCount[uMAX];

        int colorMask, blendMode;

        void init(Pass pass, int type, int *def, int defCount) {
            const uint8 *vpSrc, *fpSrc;
            switch (pass) {
                case passCompose : vpSrc = COMPOSE_VP; fpSrc = COMPOSE_FP; break;
                case passShadow  : vpSrc = SHADOW_VP;  fpSrc = SHADOW_FP;  break;
                case passAmbient : vpSrc = AMBIENT_VP; fpSrc = AMBIENT_FP; break;
                case passWater   : vpSrc = WATER_VP;   fpSrc = WATER_FP;   break;
                case passFilter  : vpSrc = FILTER_VP;  fpSrc = FILTER_FP;  break;
                case passGUI     : vpSrc = GUI_VP;     fpSrc = GUI_FP;     break;
                case PASS_CLEAR  : vpSrc = CLEAR_VP;   fpSrc = CLEAR_FP;   break;
                default          : ASSERT(false); LOG("! wrong pass id\n"); return;
            }

            float *flags = (float*)(cbMem + bindings[uFlags]);
            flags[type] = 1.0f;

            for (int i = 0; i < defCount; i++) {
                switch (def[i]) {
                    case SD_UNDERWATER      : flags[ 5] = 1.0f; break;
                    case SD_ALPHA_TEST      : flags[ 6] = 1.0f; break;
                    case SD_CLIP_PLANE      : flags[ 7] = 1.0f; break;
                    case SD_OPT_AMBIENT     : flags[ 8] = 1.0f; break;
                    case SD_OPT_SHADOW      : flags[ 9] = 1.0f; break;
                    case SD_OPT_CONTACT     : flags[10] = 1.0f; break;
                    case SD_OPT_CAUSTICS    : flags[11] = 1.0f; break;
                }
            }

            vpPtr = (SceGxmProgram*)vpSrc;
            fpPtr = (SceGxmProgram*)fpSrc;

            sceGxmShaderPatcherRegisterProgram(shaderPatcher, vpPtr, &vpUID);
            sceGxmShaderPatcherRegisterProgram(shaderPatcher, fpPtr, &fpUID);

            SceGxmVertexStream vStream;
            vStream.stride      = sizeof(Vertex);
            vStream.indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;

            SceGxmVertexAttribute vAttrib[5];
            int vAttribCount = 0;

            Vertex *v = NULL;

            struct AttribDesc {
                int index;
                int offset;
                SceGxmParameterSemantic semantic;
                SceGxmAttributeFormat   format;
            } attribDesc[] = {
                { 0, OFFSETOF(Vertex, coord),    SCE_GXM_PARAMETER_SEMANTIC_POSITION, SCE_GXM_ATTRIBUTE_FORMAT_S16  },
                { 0, OFFSETOF(Vertex, normal),   SCE_GXM_PARAMETER_SEMANTIC_NORMAL,   SCE_GXM_ATTRIBUTE_FORMAT_S16N },
                { 0, OFFSETOF(Vertex, texCoord), SCE_GXM_PARAMETER_SEMANTIC_TEXCOORD, SCE_GXM_ATTRIBUTE_FORMAT_S16  },
                { 0, OFFSETOF(Vertex, color),    SCE_GXM_PARAMETER_SEMANTIC_COLOR,    SCE_GXM_ATTRIBUTE_FORMAT_U8N  },
                { 1, OFFSETOF(Vertex, light),    SCE_GXM_PARAMETER_SEMANTIC_COLOR,    SCE_GXM_ATTRIBUTE_FORMAT_U8N  },
            };

            for (int i = 0; i < COUNT(vAttrib); i++) {
                AttribDesc &desc = attribDesc[i];
                const SceGxmProgramParameter *param = sceGxmProgramFindParameterBySemantic(vpPtr, desc.semantic, desc.index);
                if (!param) continue;
                SceGxmVertexAttribute &attrib = vAttrib[vAttribCount++];
                attrib.streamIndex    = 0;
                attrib.offset         = desc.offset;
                attrib.format         = desc.format;
                attrib.componentCount = 4;
                attrib.regIndex       = sceGxmProgramParameterGetResourceIndex(param);
            }

            sceGxmShaderPatcherCreateVertexProgram(shaderPatcher, vpUID, vAttrib, vAttribCount, &vStream, 1, &vp);
            sceGxmShaderPatcherCreateFragmentProgram(shaderPatcher, fpUID, SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4, SCE_GXM_MULTISAMPLE_NONE, NULL, vpPtr, &fp);

            for (int ut = 0; ut < uMAX; ut++) {
                vParams[ut] = sceGxmProgramFindParameterByName(vpPtr, UniformName[ut]);
                fParams[ut] = sceGxmProgramFindParameterByName(fpPtr, UniformName[ut]);
            }

            colorMask = SCE_GXM_COLOR_MASK_ALL;
            blendMode = 0;
        }

        void deinit() {
            sceGxmShaderPatcherUnregisterProgram(shaderPatcher, vpUID);
            sceGxmShaderPatcherUnregisterProgram(shaderPatcher, fpUID);
        }

        void setBlendInfo(int colorMask, int blendMode) {
            if (this->colorMask == colorMask && this->blendMode == blendMode)
                return;
            this->colorMask = colorMask;
            this->blendMode = blendMode;

            SceGxmBlendInfo blendInfo;
            blendInfo.colorMask = SceGxmColorMask(colorMask);
            blendInfo.colorFunc = SCE_GXM_BLEND_FUNC_ADD;
            blendInfo.alphaFunc = SCE_GXM_BLEND_FUNC_ADD;

            switch (blendMode) {
                case RS_BLEND_ALPHA   :
                    blendInfo.colorSrc = blendInfo.alphaSrc = SCE_GXM_BLEND_FACTOR_SRC_ALPHA;
                    blendInfo.colorDst = blendInfo.alphaDst = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                    break;
                case RS_BLEND_ADD     :
                    blendInfo.colorSrc = blendInfo.alphaSrc = SCE_GXM_BLEND_FACTOR_ONE;
                    blendInfo.colorDst = blendInfo.alphaDst = SCE_GXM_BLEND_FACTOR_ONE;
                    break;
                case RS_BLEND_MULT    :
                    blendInfo.colorSrc = blendInfo.alphaSrc = SCE_GXM_BLEND_FACTOR_DST_COLOR;
                    blendInfo.colorDst = blendInfo.alphaDst = SCE_GXM_BLEND_FACTOR_ZERO;
                    break;
                case RS_BLEND_PREMULT :
                    blendInfo.colorSrc = blendInfo.alphaSrc = SCE_GXM_BLEND_FACTOR_ONE;
                    blendInfo.colorDst = blendInfo.alphaDst = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                    break;
                default               :
                    blendInfo.colorSrc = blendInfo.alphaSrc = SCE_GXM_BLEND_FACTOR_ONE;
                    blendInfo.colorDst = blendInfo.alphaDst = SCE_GXM_BLEND_FACTOR_ZERO;
            }

            sceGxmShaderPatcherCreateFragmentProgram(shaderPatcher, fpUID, SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4, SCE_GXM_MULTISAMPLE_NONE, &blendInfo, vpPtr, &fp);

            rebind = true;
        }

        void bind() {
            if (active.shader != this) {
                active.shader = this;

                memset(cbCount, 0, sizeof(cbCount));
                cbCount[uFlags] = 16;

                rebind = true;
            }
        }

        void setup() {
            if (rebind) {
                sceGxmSetVertexProgram(Context::gxmContext, vp);
                sceGxmSetFragmentProgram(Context::gxmContext, fp);
                rebind = false;
            }

            for (int uType = 0; uType < uMAX; uType++) {
                if (!cbCount[uType]) continue;
                void *buff;
                if (vParams[uType]) {
                    sceGxmReserveVertexDefaultUniformBuffer(Context::gxmContext, &buff);
                    sceGxmSetUniformDataF(buff, vParams[uType], 0, cbCount[uType], (float*)(cbMem + bindings[uType]));
                }
                if (fParams[uType]) {
                    sceGxmReserveFragmentDefaultUniformBuffer(Context::gxmContext, &buff);
                    sceGxmSetUniformDataF(buff, fParams[uType], 0, cbCount[uType], (float*)(cbMem + bindings[uType]));
                }
            }
        }

        void setParam(UniformType uType, float *value, int count) {
            cbCount[uType] = count;
            memcpy(cbMem + bindings[uType], value, count * 4);
        }

        void setParam(UniformType uType, const vec4 &value, int count = 1) {
            setParam(uType, (float*)&value, count * 4);
        }

        void setParam(UniformType uType, const mat4  &value, int count = 1) {
            setParam(uType, (float*)&value, count * 16);
        }

        void setParam(UniformType uType, const Basis &value, int count = 1) {
            setParam(uType, (float*)&value, count * 8);
        }
    };

// Texture
    struct Texture {
        SceGxmTexture ID;
        void          *data;
        SceUID        uid;
        int           size;

        int           width, height, origWidth, origHeight;
        TexFormat     fmt;
        uint32        opt;

        SceGxmColorSurface colorSurface;
        SceGxmRenderTarget *renderTarget;

            SceUID                    depthBufferUID;
            SceGxmDepthStencilSurface depthSurface;
            void                      *depthBufferData;


        Texture(int width, int height, uint32 opt) : width(width), height(height), origWidth(width), origHeight(height), fmt(FMT_RGBA), opt(opt) {}

        void init(void *data) {
            ASSERT((opt & OPT_PROXY) == 0);

            bool filter   = (opt & OPT_NEAREST) == 0;
            bool mipmaps  = (opt & OPT_MIPMAPS) != 0;
            bool cube     = (opt & OPT_CUBEMAP) != 0;
            bool isTarget = (opt & OPT_TARGET)  != 0;
            bool isShadow = fmt == FMT_SHADOW;

            static const struct FormatDesc {
                uint32 bpp, textureFormat, targetFormat;
            } formats[FMT_MAX] = {
                {  8, SCE_GXM_TEXTURE_FORMAT_U8_1RRR           , SCE_GXM_COLOR_FORMAT_U8_R              }, // LUMINANCE
                { 32, SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_ARGB     , SCE_GXM_COLOR_FORMAT_A8R8G8B8          }, // RGBA
                { 16, SCE_GXM_TEXTURE_FORMAT_U5U6U5_RGB        , SCE_GXM_COLOR_FORMAT_U5U6U5_RGB        }, // RGB16
                { 16, SCE_GXM_TEXTURE_FORMAT_U1U5U5U5_ARGB     , SCE_GXM_COLOR_FORMAT_U1U5U5U5_ARGB     }, // RGBA16
                { 64, SCE_GXM_COLOR_FORMAT_A8R8G8B8 , SCE_GXM_COLOR_FORMAT_A8R8G8B8 }, // RGBA_FLOAT  // not supported
                { 64, SCE_GXM_COLOR_FORMAT_A8R8G8B8 , SCE_GXM_COLOR_FORMAT_A8R8G8B8 }, // RGBA_HALF
                { 32, SCE_GXM_TEXTURE_FORMAT_F32M_R            , SCE_GXM_DEPTH_STENCIL_FORMAT_DF32      }, // DEPTH
                { 32, SCE_GXM_TEXTURE_FORMAT_F32M_R            , SCE_GXM_DEPTH_STENCIL_FORMAT_DF32      }, // SHADOW
            };

            FormatDesc desc = formats[fmt];

            if (data && !support.texNPOT && (width != origWidth || height != origHeight))
                data = NULL;

            uint32 aWidth  = width;
            uint32 aHeight = height;

            if (fmt == FMT_DEPTH || fmt == FMT_SHADOW) {
                aWidth  = ALIGN(aWidth,  SCE_GXM_TILE_SIZEX);
                aHeight = ALIGN(aHeight, SCE_GXM_TILE_SIZEY);
            }

            size = aWidth * aHeight * desc.bpp / 8 * (cube ? 6 : 1);

            SceGxmMemoryAttribFlags flags = isTarget ? SCE_GXM_MEMORY_ATTRIB_RW : SCE_GXM_MEMORY_ATTRIB_READ;
            this->data = Context::allocGPU(SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, size, flags, &uid);

            if (data && this->data) {
                memcpy(this->data, data, size);
            }

            if (fmt == FMT_DEPTH || fmt == FMT_SHADOW) {
                sceGxmTextureInitTiled(&ID, this->data, SceGxmTextureFormat(desc.textureFormat), width, height, 1);
            } else {
                if (cube) {
                    sceGxmTextureInitCube(&ID, this->data, SceGxmTextureFormat(desc.textureFormat), width, height, 1);
                } else {
                    sceGxmTextureInitLinear(&ID, this->data, SceGxmTextureFormat(desc.textureFormat), width, height, 1);
                }
            }

            SceGxmTextureAddrMode addrMode;
            if (opt & OPT_REPEAT) {
                addrMode = SCE_GXM_TEXTURE_ADDR_REPEAT;
            } else {
                addrMode = (isShadow && support.texBorder) ? SCE_GXM_TEXTURE_ADDR_CLAMP_FULL_BORDER : SCE_GXM_TEXTURE_ADDR_CLAMP;
            }
            sceGxmTextureSetUAddrMode(&ID, addrMode);
            sceGxmTextureSetUAddrMode(&ID, addrMode);

            sceGxmTextureSetMinFilter(&ID, filter ? SCE_GXM_TEXTURE_FILTER_LINEAR : SCE_GXM_TEXTURE_FILTER_POINT);
            sceGxmTextureSetMagFilter(&ID, filter ? SCE_GXM_TEXTURE_FILTER_LINEAR : SCE_GXM_TEXTURE_FILTER_POINT);

            if (opt & OPT_TARGET) {

                if (fmt == FMT_DEPTH || fmt == FMT_SHADOW) {
                    depthBufferData = this->data;
                    depthBufferUID  = uid;

                    sceGxmDepthStencilSurfaceInit(&depthSurface,
                        SceGxmDepthStencilFormat(desc.targetFormat),
                        SCE_GXM_DEPTH_STENCIL_SURFACE_TILED,
                        aWidth, this->data, NULL);

            sceGxmDepthStencilSurfaceSetForceStoreMode ( &depthSurface, SCE_GXM_DEPTH_STENCIL_FORCE_STORE_ENABLED );

                } else {
                    sceGxmColorSurfaceInit(&colorSurface,
                        SceGxmColorFormat(desc.targetFormat),
                        SCE_GXM_COLOR_SURFACE_LINEAR,
                        SCE_GXM_COLOR_SURFACE_SCALE_NONE,
                        SCE_GXM_OUTPUT_REGISTER_SIZE_32BIT,
                        width, height, width, this->data);

                    uint32 dsWidth  = ALIGN(width,  SCE_GXM_TILE_SIZEX);
                    uint32 dsHeight = ALIGN(height, SCE_GXM_TILE_SIZEY);

                    depthBufferData = Context::allocGPU(
                        SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE,
                        4 * dsWidth * dsHeight,
                        SCE_GXM_MEMORY_ATTRIB_RW,
                        &depthBufferUID);

                    sceGxmDepthStencilSurfaceInit(&depthSurface,
                        SCE_GXM_DEPTH_STENCIL_FORMAT_D16,
                        SCE_GXM_DEPTH_STENCIL_SURFACE_TILED,
                        dsWidth, depthBufferData, NULL);
                }

                SceGxmRenderTargetParams params;
                memset(&params, 0, sizeof(params));
                params.width           = width;
                params.height          = height;
                params.scenesPerFrame  = 1;
                params.multisampleMode = SCE_GXM_MULTISAMPLE_NONE;
                params.driverMemBlock  = -1;

                sceGxmCreateRenderTarget(&params, &renderTarget);
            }
        }

        void deinit() {
            if (opt & OPT_TARGET) {
                sceGxmDestroyRenderTarget(renderTarget);
                if (depthBufferUID != uid) {
                    Context::freeGPU(depthBufferUID, true);
                }
            }
            Context::freeGPU(uid, true);
        }

        void generateMipMap() {
            /*
            bind(0);
            GLenum target = (opt & OPT_CUBEMAP) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;

            glGenerateMipmap(target);
            if (!(opt & OPT_CUBEMAP) && !(opt & OPT_NEAREST) && (support.maxAniso > 0))
                glTexParameteri(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, min(int(support.maxAniso), 8));
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 4);
            */
        }

        void update(void *data) {
            if (!data) return;
            memcpy(this->data, data, size);
        }

        void bind(int sampler) {
            if (opt & OPT_PROXY) return;
            ASSERT(ID);

            if (active.textures[sampler] != this) {
                active.textures[sampler] = this;
                sceGxmSetFragmentTexture(Context::gxmContext, sampler, &ID);
            }
        }

        void unbind(int sampler) {
            if (active.textures[sampler]) {
                active.textures[sampler] = NULL;
                sceGxmSetFragmentTexture(Context::gxmContext, sampler, NULL);
            }
        }

        void setFilterQuality(int value) {
            bool filter  = (opt & OPT_NEAREST) == 0 && (value > Settings::LOW);
            bool mipmaps = (opt & OPT_MIPMAPS) != 0;

            sceGxmTextureSetMinFilter(&ID, filter ? SCE_GXM_TEXTURE_FILTER_LINEAR : SCE_GXM_TEXTURE_FILTER_POINT);
            sceGxmTextureSetMagFilter(&ID, filter ? SCE_GXM_TEXTURE_FILTER_LINEAR : SCE_GXM_TEXTURE_FILTER_POINT);
        }
    };

// Mesh
    struct Mesh {
        Index        *iBuffer;
        GAPI::Vertex *vBuffer;

        SceUID       iBufferUID;
        SceUID       vBufferUID;

        int          iCount;
        int          vCount;

        bool         dynamic;

        struct Chunk {
            int frameIndex;
            int iStart, iCount;
            int vStart, vCount;
        } chunks[DISPLAY_BUFFER_COUNT];

        Mesh(bool dynamic) : iBuffer(NULL), vBuffer(NULL), dynamic(dynamic) {}

        void init(Index *indices, int iCount, ::Vertex *vertices, int vCount, int aCount) {
            ASSERT(sizeof(GAPI::Vertex) == sizeof(::Vertex));
            memset(chunks, 0, sizeof(chunks));

            this->iCount  = iCount;
            this->vCount  = vCount;

            iBuffer = (Index*)  Context::allocGPU(SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, iCount * sizeof(Index),  SCE_GXM_MEMORY_ATTRIB_READ, &iBufferUID);
            vBuffer = (Vertex*) Context::allocGPU(SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, vCount * sizeof(Vertex), SCE_GXM_MEMORY_ATTRIB_READ, &vBufferUID);

            update(indices, iCount, vertices, vCount);
        }

        void deinit() {
            Context::freeGPU(iBufferUID, true);
            Context::freeGPU(vBufferUID, true);
        }

        Chunk& getChunk() {
            return dynamic ? chunks[Core::stats.frameIndex % COUNT(chunks)] : chunks[0];
        }

        void update(Index *indices, int iCount, ::Vertex *vertices, int vCount) {
            Chunk &chunk = getChunk();
            if (chunk.frameIndex != Core::stats.frameIndex) {
                chunk.frameIndex = Core::stats.frameIndex;
                chunk.iStart = chunk.iCount = 0;
                chunk.vStart = chunk.vCount = 0;
            }

            if (indices && iCount) {
                chunk.iStart = chunk.iCount;
                chunk.iCount += iCount;
                memcpy(iBuffer + chunk.iStart, indices, iCount * sizeof(Index));
            }

            if (vertices && vCount) {
                chunk.vStart = chunk.vCount;
                chunk.vCount += vCount;
                memcpy(vBuffer + chunk.vStart, vertices, vCount * sizeof(Vertex));
            }
        }

        void bind(const MeshRange &range) {
            active.vBuffer = vBuffer + getChunk().vStart + range.vStart;
            sceGxmSetVertexStream(Context::gxmContext, 0, active.vBuffer);
        }

        void initNextRange(MeshRange &range, int &aIndex) const {
            range.aIndex = -1;
        }
    };


    int cullMode, blendMode, colorMask;
    bool depthTest, depthWrite;
    Shader clearShader;
    Mesh   clearMesh(false);
    vec4   clearColor;

    void init() {
        LOG("Vendor   : %s\n", "Sony");
        LOG("Renderer : %s\n", "SCE GXM");
        LOG("Version  : %s\n", "1.0");

       // EDRAM_SIZE = sceGeEdramGetSize();
       // LOG("VRAM     : %d\n", EDRAM_SIZE);
       // freeEDRAM();

        support.maxAniso       = 0;
        support.maxVectors     = 0;
        support.shaderBinary   = false;
        support.VAO            = false;
        support.depthTexture   = true;
        support.shadowSampler  = true;
        support.discardFrame   = false;
        support.texNPOT        = true;
        support.texRG          = true;
        support.texBorder      = false;
        support.colorFloat     = false;
        support.colorHalf      = true;
        support.texFloatLinear = false;
        support.texFloat       = false;
        support.texHalfLinear  = true;
        support.texHalf        = true;
        support.clipDist       = true;

        Core::width  = DISPLAY_WIDTH;
        Core::height = DISPLAY_HEIGHT;

        { // gxm
            SceGxmInitializeParams params;
            memset(&params, 0, sizeof(params));
            params.displayQueueMaxPendingCount  = DISPLAY_BUFFER_COUNT - 1;
            params.displayQueueCallback         = display_queue_callback;
            params.displayQueueCallbackDataSize = sizeof(DisplayData);
            params.parameterBufferSize          = SCE_GXM_DEFAULT_PARAMETER_BUFFER_SIZE;

            sceGxmInitialize(&params);
        }

        Context::init();
        SwapChain::init();

        { // shader patcher
            uint32 vertOffset, fragOffset;

            shaderBuffPtr = Context::allocGPU(SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, SHADER_BUFF_SIZE, SCE_GXM_MEMORY_ATTRIB_RW, &shaderBuffUID);
            shaderVertPtr = Context::allocVertexUSSE(SHADER_VERT_SIZE, &shaderVertUID, &vertOffset);
            shaderFragPtr = Context::allocFragmentUSSE(SHADER_FRAG_SIZE, &shaderFragUID, &fragOffset);

            SceGxmShaderPatcherParams params;
            memset(&params, 0, sizeof(params));
            params.hostAllocCallback   = Context::allocCPU;
            params.hostFreeCallback    = Context::freeCPU;
            params.bufferMem           = shaderBuffPtr;
            params.bufferMemSize       = SHADER_BUFF_SIZE;
            params.vertexUsseMem       = shaderVertPtr;
            params.vertexUsseMemSize   = SHADER_VERT_SIZE;
            params.vertexUsseOffset    = vertOffset;
            params.fragmentUsseMem     = shaderFragPtr;
            params.fragmentUsseMemSize = SHADER_FRAG_SIZE;
            params.fragmentUsseOffset  = fragOffset;

            sceGxmShaderPatcherCreate(&params, &shaderPatcher);
        }

        clearShader.init(PASS_CLEAR, 0, NULL, 0);

        Index indices[] = { 0, 1, 2 };
        Vertex vertices[3];
        vertices[0].coord = short4{-1, -1, 1, 1};
        vertices[1].coord = short4{ 3, -1, 1, 1};
        vertices[2].coord = short4{-1,  3, 1, 1};
        clearMesh.init(indices, 3, vertices, 3, 0);

        clearColor = vec4(1.0f, 1.0f, 0.0f, 1.0f);

        colorMask = SCE_GXM_COLOR_MASK_ALL;
        blendMode = 0;
    }

    void deinit() {
        sceGxmFinish(Context::gxmContext);
        sceGxmDisplayQueueFinish();

        clearShader.deinit();
        clearMesh.deinit();

        SwapChain::deinit();
        Context::deinit();

        sceGxmTerminate();
    }

    mat4 ortho(float l, float r, float b, float t, float znear, float zfar) {
        return mat4(mat4::PROJ_ZERO_POS, l, r, b, t, znear, zfar);
    }

    mat4 perspective(float fov, float aspect, float znear, float zfar) {
        return mat4(mat4::PROJ_ZERO_POS, fov, aspect, znear, zfar);
    }

    bool beginFrame() {
        return true;
    }

    void endFrame() {
    }

    bool hasScene;
    Texture defTarget(DISPLAY_WIDTH, DISPLAY_HEIGHT, OPT_TARGET);

    void resetState() {
        hasScene = false;
        Core::defaultTarget = &defTarget;
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
        if (hasScene) {
            sceGxmEndScene(Context::gxmContext, NULL, NULL);
        }
        hasScene = true;

        if (!target || target == &defTarget) {
            sceGxmBeginScene(Context::gxmContext, 0, SwapChain::defaultTarget, NULL, NULL,
                SwapChain::getSyncObj(),
                SwapChain::getColorSurface(),
                SwapChain::getDepthSurface());
        } else {
            ASSERT(target->opt & OPT_TARGET);

            SceGxmColorSurface *colorSurface = (target->fmt == FMT_DEPTH || target->fmt == FMT_SHADOW) ? NULL : &target->colorSurface;

            bool loadDepth  = (Core::reqTarget.op & RT_LOAD_DEPTH);
            bool storeDepth = (Core::reqTarget.op & RT_STORE_DEPTH);

            //sceGxmDepthStencilSurfaceSetForceLoadMode  ( &target->depthSurface, loadDepth  ? SCE_GXM_DEPTH_STENCIL_FORCE_LOAD_ENABLED  : SCE_GXM_DEPTH_STENCIL_FORCE_LOAD_DISABLED  );
            //sceGxmDepthStencilSurfaceSetForceStoreMode ( &target->depthSurface, storeDepth ? SCE_GXM_DEPTH_STENCIL_FORCE_STORE_ENABLED : SCE_GXM_DEPTH_STENCIL_FORCE_STORE_DISABLED );

            sceGxmBeginScene(Context::gxmContext, 0, target->renderTarget, NULL, NULL, NULL, colorSurface, &target->depthSurface);



        /*
            bool depth = target->fmt == FMT_DEPTH || target->fmt == FMT_SHADOW;

            if (target->tex2D) {
                D3DCHECK(target->tex2D->GetSurfaceLevel(0, &surface));
            } else if (target->texCube)
                D3DCHECK(target->texCube->GetCubeMapSurface(D3DCUBEMAP_FACES(D3DCUBEMAP_FACE_POSITIVE_X + face), 0, &surface));

            int rtIndex = cacheRenderTarget(!depth, target->width, target->height);

            if (depth) {
                D3DCHECK(device->SetRenderTarget(0, rtCache[false].items[rtIndex].surface));
                D3DCHECK(device->SetDepthStencilSurface(surface));
            } else {
                D3DCHECK(device->SetRenderTarget(0, surface));
                D3DCHECK(device->SetDepthStencilSurface(rtCache[true].items[rtIndex].surface));
            }

            surface->Release();
        */
        }
        active.viewport = Viewport(0, 0, 0, 0); // forcing viewport reset
    }

    void discardTarget(bool color, bool depth) {}

    void copyTarget(Texture *dst, int xOffset, int yOffset, int x, int y, int width, int height) {
        //
    }

    void setVSync(bool enable) {}

    void present() {
        if (hasScene) {
            sceGxmEndScene(Context::gxmContext, NULL, NULL);
        }

        SwapChain::present();
        Context::checkPendings();
    }

    void setViewport(const Viewport &vp) {
        int vh = active.target ? active.target->height : Core::height;
        int sw = vp.width  / 2;
        int sh = vp.height / 2;
        sceGxmSetViewport(Context::gxmContext, float(vp.x + sw), float(sw), float(vh - vp.y - sh), float(-sh), 0.0f, 1.0f);
        sceGxmSetRegionClip(Context::gxmContext, SCE_GXM_REGION_CLIP_OUTSIDE, vp.x, vh - vp.y - vp.height, vp.x + vp.width, vp.y + vp.height);
    }

    void setDepthTest(bool enable) {
        depthTest = enable;
        sceGxmSetFrontDepthFunc(Context::gxmContext, enable ? SCE_GXM_DEPTH_FUNC_LESS_EQUAL : SCE_GXM_DEPTH_FUNC_ALWAYS);
        sceGxmSetBackDepthFunc(Context::gxmContext,  enable ? SCE_GXM_DEPTH_FUNC_LESS_EQUAL : SCE_GXM_DEPTH_FUNC_ALWAYS);
        sceGxmSetFrontDepthWriteEnable(Context::gxmContext, (depthWrite && depthTest) ? SCE_GXM_DEPTH_WRITE_ENABLED : SCE_GXM_DEPTH_WRITE_DISABLED);
        sceGxmSetBackDepthWriteEnable(Context::gxmContext,  (depthWrite && depthTest) ? SCE_GXM_DEPTH_WRITE_ENABLED : SCE_GXM_DEPTH_WRITE_DISABLED);
    }

    void setDepthWrite(bool enable) {
        depthWrite = enable;
        if (depthTest) {
            sceGxmSetFrontDepthWriteEnable(Context::gxmContext, enable ? SCE_GXM_DEPTH_WRITE_ENABLED : SCE_GXM_DEPTH_WRITE_DISABLED);
            sceGxmSetBackDepthWriteEnable(Context::gxmContext,  enable ? SCE_GXM_DEPTH_WRITE_ENABLED : SCE_GXM_DEPTH_WRITE_DISABLED);
        }
    }

    void setColorWrite(bool r, bool g, bool b, bool a) {
        colorMask = SCE_GXM_COLOR_MASK_NONE;
        if (r) colorMask |= SCE_GXM_COLOR_MASK_R;
        if (g) colorMask |= SCE_GXM_COLOR_MASK_G;
        if (b) colorMask |= SCE_GXM_COLOR_MASK_B;
        if (a) colorMask |= SCE_GXM_COLOR_MASK_A;
    }

    void setAlphaTest(bool enable) {}

    void setCullMode(int rsMask) {
        cullMode = rsMask;
        switch (rsMask) {
            case RS_CULL_BACK  : sceGxmSetCullMode(Context::gxmContext, SCE_GXM_CULL_CW);  break;
            case RS_CULL_FRONT : sceGxmSetCullMode(Context::gxmContext, SCE_GXM_CULL_CCW); break;
            default            : sceGxmSetCullMode(Context::gxmContext, SCE_GXM_CULL_NONE);
        }
    }

    void setBlendMode(int rsMask) {
        blendMode = rsMask;
    }

    void setViewProj(const mat4 &mView, const mat4 &mProj) {}

    void DIP(Mesh *mesh, const MeshRange &range) {
        if (!active.shader) return;

        active.shader->setBlendInfo(colorMask, blendMode);
        active.shader->setup();
        sceGxmDraw(Context::gxmContext, SCE_GXM_PRIMITIVE_TRIANGLES, SCE_GXM_INDEX_FORMAT_U16, mesh->iBuffer + mesh->getChunk().iStart + range.iStart, range.iCount);
    }

    void updateLights(vec4 *lightPos, vec4 *lightColor, int count) {
        if (active.shader) {
            active.shader->setParam(uLightColor, lightColor[0], count);
            active.shader->setParam(uLightPos,   lightPos[0],   count);
        }
    }

    void clear(bool color, bool depth) {
        int  oColorMask  = colorMask;
        int  oBlendMode  = blendMode;
        bool oDepthTest  = depthTest;
        int  oCullMode   = cullMode;
        Vertex *oBuffer  = active.vBuffer;
        Shader *oShader  = Core::active.shader;

        sceGxmSetFrontDepthFunc(Context::gxmContext, SCE_GXM_DEPTH_FUNC_ALWAYS);
        sceGxmSetBackDepthFunc(Context::gxmContext,  SCE_GXM_DEPTH_FUNC_ALWAYS);
        sceGxmSetFrontDepthWriteEnable(Context::gxmContext, depth ? SCE_GXM_DEPTH_WRITE_ENABLED : SCE_GXM_DEPTH_WRITE_DISABLED);
        sceGxmSetBackDepthWriteEnable(Context::gxmContext,  depth ? SCE_GXM_DEPTH_WRITE_ENABLED : SCE_GXM_DEPTH_WRITE_DISABLED);

        setColorWrite(color, color, color, color);
        setBlendMode(0);
        setCullMode(cmNone);

        active.shader = &clearShader;
        active.shader->setParam(uMaterial, clearColor);

        MeshRange range;
        range.iStart =  0;
        range.iCount =  3;
        range.vStart =  0;
        range.aIndex = -1;

        clearMesh.bind(range);
        DIP(&clearMesh, range);

        colorMask = oColorMask;
        setBlendMode(oBlendMode);
        setCullMode(oCullMode);
        setDepthTest(oDepthTest);

        active.shader = oShader;

        sceGxmSetVertexStream(Context::gxmContext, 0, oBuffer);
    }

    void setClearColor(const vec4 &color) {
        clearColor = color;
    }

    vec4 copyPixel(int x, int y) {
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