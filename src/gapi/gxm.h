#ifndef H_GAPI_GXM
#define H_GAPI_GXM

#include "core.h"

#include <psp2/display.h>
#include <psp2/gxm.h>
#include <psp2/gxt.h>

#define PROFILE_MARKER(title)
#define PROFILE_LABEL(id, name, label)
#define PROFILE_TIMING(time)

#define DISPLAY_WIDTH           960
#define DISPLAY_HEIGHT          544
#define DISPLAY_STRIDE          1024
#define DISPLAY_BUFFER_COUNT    2
#define DISPLAY_COLOR_FORMAT    SCE_GXM_COLOR_FORMAT_A8B8G8R8
#define DISPLAY_PIXEL_FORMAT    SCE_DISPLAY_PIXELFORMAT_A8B8G8R8

namespace GAPI {
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

    void displayCallback(const void *callbackData) {
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
                size = ALIGNADDR(size, 256 * 1024);
            } else {
                size = ALIGNADDR(size, 4 * 1024);
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

            size = ALIGNADDR(size, 4096);

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

            size = ALIGNADDR(size, 4096);

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

        uint32 *SWIZZE_TABLE = NULL;
        #define SWIZZLE(x, y) ((SWIZZE_TABLE[(x)] << 1) | (SWIZZE_TABLE[(y)]))

        void initSwizzleTable() {
            SWIZZE_TABLE = new uint32[4096];
            uint32 value = 0;
            for (int i = 0; i < 4096; i++) {
                SWIZZE_TABLE[i] = value;
                value += 0x2AAAAAAB;
                value &= 0x55555555;
            }
        }

        void freeSwizzleTable() {
            delete[] SWIZZE_TABLE;
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

            initSwizzleTable();
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
            freeSwizzleTable();
        }

        void checkPendings() {
            int i = 0;
            while (i < pendings.length) {
                if (pendings[i].frameIndex + DISPLAY_BUFFER_COUNT <= Core::stats.frameIndex) {
                    pendings.removeFast(i);
                } else {
                    i++;
                }
            }
        }

        template <typename T>
        void swizzleTiles(T *dst, T *src, int width, int tilesX, int tilesY) {
            int tileSize = width / tilesX;
            int tileArea = SQR(tileSize);

            for (int j = 0; j < tilesY; j++) {
                for (int i = 0; i < tilesX; i++) {
                    T *tilePtr = dst + ((tileArea * tilesX) * j) + (tileArea * i);

                    for (int y = 0; y < tileSize; y++) {
                        T *ptr = src + (width * (tileSize * j)) + (tileSize * i) + width * y;

                        for (int x = 0; x < tileSize; x++)
                            *(tilePtr + SWIZZLE(x, y)) = *ptr++;
                    }
                }
            }
        }

        void swizzleImage(void *dst, void *src, int width, int height, int bpp) {
            ASSERT(SWIZZLE_TABLE);

            int tilesX, tilesY;

            if (width > height) {
                tilesX = width / height;
                tilesY = 1;
            } else {
                tilesX = 1;
                tilesY = height / width;
            }

            switch (bpp) {
                case  8 : swizzleTiles(  (uint8*) dst,  (uint8*) src, width, tilesX, tilesY ); break;
                case 16 : swizzleTiles( (uint16*) dst, (uint16*) src, width, tilesX, tilesY ); break;
                case 32 : swizzleTiles( (uint32*) dst, (uint32*) src, width, tilesX, tilesY ); break;
            }
        }

        #define TILE_SIZE 32

        void tileImage(void *dst, void *src, int width, int height, int bpp) {
            int tilesX = width  / TILE_SIZE;
            int tilesY = height / TILE_SIZE;

            uint8 *tilePtr = (uint8*)dst;
            for (int y = 0; y < tilesY; y++) {
                for (int x = 0; x < tilesX; x++) {
                    uint8 *ptr = (uint8*)src + (width * y + x) * TILE_SIZE * bpp / 8;

                    for (int i = 0; i < TILE_SIZE; i++) {
                        memcpy(tilePtr, ptr, TILE_SIZE * bpp / 8);
                        ptr += width * bpp / 8;
                        tilePtr += TILE_SIZE * bpp / 8;
                    }
                }
            }
        }

        #undef TILE_SIZE
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

            uint32 dsWidth   = ALIGNADDR(DISPLAY_WIDTH,  SCE_GXM_TILE_SIZEX);
            uint32 dsHeight  = ALIGNADDR(DISPLAY_HEIGHT, SCE_GXM_TILE_SIZEY);

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
    #include "shaders/gxm/shaders.h"

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
        SceGxmShaderPatcherId  vpUID;
        SceGxmProgram          *vpPtr, *fpPtr;

        struct PSO {
            SceGxmFragmentProgram  *fp;
            SceGxmShaderPatcherId  fpUID;
        } pso[2 * bmMAX];

        const SceGxmProgramParameter *vParams[uMAX];
        const SceGxmProgramParameter *fParams[uMAX];

        vec4  cbMem[98 + MAX_CONTACTS];
        int   cbCount[uMAX];

        SceGxmOutputRegisterFormat outputFmt;

        int colorMask, blendMode;
        int psoIndex;

        bool rebind;

        void init(Pass pass, int type, int *def, int defCount) {
			LOG("init shader %d %d ", int(pass), int(type));
            memset(pso, 0, sizeof(pso));

            outputFmt = SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4;

            float *flags = (float*)(cbMem + bindings[uFlags]);

            bool underwater = false;
            bool alphatest  = false;

            for (int i = 0; i < defCount; i++) {
                switch (def[i]) {
                    case SD_UNDERWATER      : underwater = true; break;
                    case SD_ALPHA_TEST      : alphatest  = true; break;
                    case SD_OPT_AMBIENT     : flags[0] = 1.0f; break;
                    case SD_OPT_SHADOW      : flags[1] = 1.0f; break;
                    case SD_OPT_CONTACT     : flags[2] = 1.0f; break;
                    case SD_OPT_CAUSTICS    : flags[3] = 1.0f; break;
                }
            }

            #define SHADER(S,P)    S##_##P
            #define SHADER_A(S,P)  (alphatest  ? SHADER(S##_a,P) : SHADER(S,P))
            #define SHADER_U(S,P)  (underwater ? SHADER(S##_u,P) : SHADER(S,P))
            #define SHADER_AU(S,P) ((underwater && alphatest) ? SHADER(S##_au,P) : (alphatest ? SHADER(S##_a,P) : SHADER_U(S,P)))

            const uint8 *vSrc = NULL, *fSrc = NULL;
            switch (pass) {
                case passCompose :
                    switch (type) {
                        case 0  : vSrc = SHADER_U ( compose_sprite, v );  fSrc = SHADER_AU ( compose_sprite, f ); break;
                        case 1  : vSrc = SHADER   ( compose_flash,  v );  fSrc = SHADER    ( compose_flash,  f ); break;
                        case 2  : vSrc = SHADER_U ( compose_room,   v );  fSrc = SHADER_AU ( compose_room,   f ); break;
                        case 3  : vSrc = SHADER_U ( compose_entity, v );  fSrc = SHADER_AU ( compose_entity, f ); break;
                        case 4  : vSrc = SHADER   ( compose_mirror, v );  fSrc = SHADER    ( compose_mirror, f ); break;
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
                        case 0  : vSrc = SHADER ( ambient_sprite, v );  fSrc = SHADER_A ( ambient_sprite, f ); break;
                        case 1  : vSrc = SHADER ( ambient_room,   v );  fSrc = SHADER   ( ambient_room,   f ); break; // TYPE_FLASH (sky)
                        case 2  : vSrc = SHADER ( ambient_room,   v );  fSrc = SHADER_A ( ambient_room,   f ); break;
                        default : ASSERT(false);
                    }
                    break;
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
                case passFilter :
                    switch (type) {
                        case 0  : vSrc = SHADER ( filter_upscale,    v );  fSrc = SHADER ( filter_upscale,    f ); break;
                        case 1  : vSrc = SHADER ( filter_downsample, v );  fSrc = SHADER ( filter_downsample, f ); break;
                        case 3  : vSrc = SHADER ( filter_grayscale,  v );  fSrc = SHADER ( filter_grayscale,  f ); break;
                        case 4  : vSrc = SHADER ( filter_blur,       v );  fSrc = SHADER ( filter_blur,       f ); break;
                        default : ASSERT(false);
                    }
                    break;
                case passGUI    : vSrc = SHADER ( gui,   v );  fSrc = SHADER ( gui,   f ); break;
				case passSky    : vSrc = SHADER ( gui,   v );  fSrc = SHADER ( gui,   f ); break;
                case PASS_CLEAR : vSrc = SHADER ( clear, v );  fSrc = SHADER ( clear, f ); break;
                default         : ASSERT(false); LOG("! wrong pass id\n"); return;
            }

			LOG("  %s", vSrc != NULL ? "true" : "false");

            #undef SHADER_A
            #undef SHADER_U
            #undef SHADER_AU

            if (pass == passWater && (type == 0 || type == 1)) { // water_simulate & water_drop use half2 render target
                outputFmt = SCE_GXM_OUTPUT_REGISTER_FORMAT_HALF2;
            }

            vpPtr = (SceGxmProgram*)vSrc;
            fpPtr = (SceGxmProgram*)fSrc;

            sceGxmShaderPatcherRegisterProgram(shaderPatcher, vpPtr, &vpUID);

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

            for (int ut = 0; ut < uMAX; ut++) {
                vParams[ut] = sceGxmProgramFindParameterByName(vpPtr, UniformName[ut]);
                fParams[ut] = sceGxmProgramFindParameterByName(fpPtr, UniformName[ut]);
            }

            colorMask = blendMode = -1;

			LOG("done\n");
        }

        void deinit() {
            sceGxmDisplayQueueFinish();

            sceGxmShaderPatcherReleaseVertexProgram(shaderPatcher, vp);
            sceGxmShaderPatcherUnregisterProgram(shaderPatcher, vpUID);

            for (int i = 0; i < COUNT(pso); i++) {
                if (pso[i].fp) {
                    sceGxmShaderPatcherReleaseFragmentProgram(shaderPatcher, pso[i].fp);
                    sceGxmShaderPatcherUnregisterProgram(shaderPatcher, pso[i].fpUID);
                }
            }
        }

        void setBlendInfo(int colorMask, int blendMode) {
            if (this->colorMask == colorMask && this->blendMode == blendMode)
                return;
            this->colorMask = colorMask;
            this->blendMode = blendMode;

            psoIndex = 0;
            switch (blendMode) {
                case RS_BLEND_ALPHA   : psoIndex = bmAlpha;   break;
                case RS_BLEND_ADD     : psoIndex = bmAdd;     break;
                case RS_BLEND_MULT    : psoIndex = bmMult;    break;
                case RS_BLEND_PREMULT : psoIndex = bmPremult; break;
                default               : psoIndex = bmNone;
            }

            if (outputFmt != SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4) {
                psoIndex = 0;
            }

            if (colorMask != SCE_GXM_COLOR_MASK_ALL) {
                psoIndex += bmMAX;
            }

            PSO &p = pso[psoIndex];

            if (!p.fp) {
                SceGxmBlendInfo blendInfo;
                blendInfo.colorMask = SceGxmColorMask(colorMask);
                blendInfo.colorFunc = SCE_GXM_BLEND_FUNC_ADD;
                blendInfo.alphaFunc = SCE_GXM_BLEND_FUNC_ADD;
                blendInfo.alphaSrc  = SCE_GXM_BLEND_FACTOR_ONE;
                blendInfo.alphaDst  = SCE_GXM_BLEND_FACTOR_ZERO;

                switch (blendMode) {
                    case RS_BLEND_ALPHA   :
                        blendInfo.colorSrc = SCE_GXM_BLEND_FACTOR_SRC_ALPHA;
                        blendInfo.colorDst = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                        break;
                    case RS_BLEND_ADD     :
                        blendInfo.colorSrc = SCE_GXM_BLEND_FACTOR_ONE;
                        blendInfo.colorDst = SCE_GXM_BLEND_FACTOR_ONE;
                        break;
                    case RS_BLEND_MULT    :
                        blendInfo.colorSrc = SCE_GXM_BLEND_FACTOR_DST_COLOR;
                        blendInfo.colorDst = SCE_GXM_BLEND_FACTOR_ZERO;
                        break;
                    case RS_BLEND_PREMULT :
                        blendInfo.colorSrc = SCE_GXM_BLEND_FACTOR_ONE;
                        blendInfo.colorDst = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                        break;
                    default               :
                        blendInfo.colorSrc = SCE_GXM_BLEND_FACTOR_ONE;
                        blendInfo.colorDst = SCE_GXM_BLEND_FACTOR_ZERO;
                }
                createFP(p, &blendInfo);
            }

            rebind = true;
        }

        void createFP(PSO &p, SceGxmBlendInfo *blendInfo) {
            sceGxmShaderPatcherRegisterProgram(shaderPatcher, fpPtr, &p.fpUID);
            sceGxmShaderPatcherCreateFragmentProgram(shaderPatcher, p.fpUID, outputFmt, SCE_GXM_MULTISAMPLE_NONE, blendInfo, vpPtr, &p.fp);
        }

        void bind() {
            if (active.shader != this) {
                active.shader = this;

                memset(cbCount, 0, sizeof(cbCount));
                cbCount[uFlags] = 16;

                rebind = true;
            }
        }

        void validate() {
            if (rebind) {
                sceGxmSetVertexProgram(Context::gxmContext, vp);
                sceGxmSetFragmentProgram(Context::gxmContext, pso[psoIndex].fp);
                rebind = false;
            }

            for (int uType = 0; uType < uMAX; uType++) {
                if (!cbCount[uType]) continue;
                void *buff;
                if (vParams[uType]) {
                    sceGxmReserveVertexDefaultUniformBuffer(Context::gxmContext, &buff);
                    sceGxmSetUniformDataF(buff, vParams[uType], 0, cbCount[uType] * 4, (float*)(cbMem + bindings[uType]));
                }
                if (fParams[uType]) {
                    sceGxmReserveFragmentDefaultUniformBuffer(Context::gxmContext, &buff);
                    sceGxmSetUniformDataF(buff, fParams[uType], 0, cbCount[uType] * 4, (float*)(cbMem + bindings[uType]));
                }
            }
        }

        void setParam(UniformType uType, float *value, int count) {
            cbCount[uType] = count;
            memcpy(cbMem + bindings[uType], value, count * 16);
        }

        void setParam(UniformType uType, const vec4 &value, int count = 1) {
            setParam(uType, (float*)&value, count);
        }

        void setParam(UniformType uType, const mat4  &value, int count = 1) {
            setParam(uType, (float*)&value, count * 4);
        }
    };

// Texture
    static const struct FormatDesc {
        uint32 bpp, textureFormat, targetFormat;
    } formats[FMT_MAX] = {
        {  8, SCE_GXM_TEXTURE_FORMAT_U8_1RRR       , SCE_GXM_COLOR_FORMAT_U8_R          }, // LUMINANCE
        { 32, SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_ABGR , SCE_GXM_COLOR_FORMAT_A8B8G8R8      }, // RGBA
        { 16, SCE_GXM_TEXTURE_FORMAT_U5U6U5_BGR    , SCE_GXM_COLOR_FORMAT_U5U6U5_BGR    }, // RGB16
        { 16, SCE_GXM_TEXTURE_FORMAT_U1U5U5U5_ABGR , SCE_GXM_COLOR_FORMAT_U1U5U5U5_ABGR }, // RGBA16
        { 64, SCE_GXM_TEXTURE_FORMAT_F32F32_GR     , SCE_GXM_COLOR_FORMAT_F32F32_GR     }, // RG_FLOAT  // not supported
        { 32, SCE_GXM_TEXTURE_FORMAT_F16F16_GR     , SCE_GXM_COLOR_FORMAT_F16F16_GR     }, // RG_HALF
        { 32, SCE_GXM_TEXTURE_FORMAT_F32M_R        , SCE_GXM_DEPTH_STENCIL_FORMAT_DF32  }, // DEPTH
        { 32, SCE_GXM_TEXTURE_FORMAT_F32M_R        , SCE_GXM_DEPTH_STENCIL_FORMAT_DF32  }, // SHADOW
    };

    struct Texture {
        SceGxmTexture ID;
        uint8         *data;
        SceUID        uid;

        int           width, height, depth, origWidth, origHeight, origDepth, aWidth, aHeight;
        TexFormat     fmt;
        uint32        opt;
        int           mipCount;

        SceGxmColorSurface colorSurface;
        SceGxmRenderTarget *renderTarget;

        SceUID                    depthBufferUID;
        SceGxmDepthStencilSurface depthSurface;
        void                      *depthBufferData;

        Texture(int width, int height, int depth, uint32 opt) : width(width), height(height), depth(depth), origWidth(width), origHeight(height), origDepth(depth), fmt(FMT_RGBA), opt(opt) { opt |= OPT_NEAREST; }

        void init(void *data) {
            ASSERT((opt & OPT_PROXY) == 0);

            bool filter     = (opt & OPT_NEAREST) == 0;
            bool mipmaps    = (opt & OPT_MIPMAPS) != 0;
            bool isCube     = (opt & OPT_CUBEMAP) != 0;
            bool isTarget   = (opt & OPT_TARGET)  != 0;
            bool isShadow   = fmt == FMT_SHADOW;
            bool isTiled    = isTarget;
            bool isSwizzled = !isTiled && filter;

            FormatDesc desc = formats[fmt];

            if (isSwizzled) {
                aWidth  = width  = nextPow2(width);
                aHeight = height = nextPow2(height);
            } else if (isTiled) {
                aWidth  = ALIGNADDR(width,  SCE_GXM_TILE_SIZEX);
                aHeight = ALIGNADDR(height, SCE_GXM_TILE_SIZEY);
            } else {
                aWidth  = ALIGNADDR(width, 8);
                aHeight = height;
            }

            int size = 0;

            if (isCube || isTiled || fmt != FMT_RGBA) {
                mipmaps = false;
            }

            mipCount = 0;
            if (mipmaps) {
                int w = width;
                int h = height;
                while (w > 15 && h > 15 && mipCount < 4) {
                    size += ALIGNADDR(w, 8) * h;
                    w /= 2;
                    h /= 2;
                    mipCount++;
                }
            } else {
                size += aWidth * aHeight;
            }

            if (mipCount > 1) {
                isSwizzled = false;
            }

            size *= desc.bpp / 8;

            if (isCube) {
                size *= 6;
            }

            SceGxmMemoryAttribFlags flags = (isTarget || mipCount > 1) ? SCE_GXM_MEMORY_ATTRIB_RW : SCE_GXM_MEMORY_ATTRIB_READ;
            this->data = (uint8*)Context::allocGPU(SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, size, flags, &uid);

            if (data && this->data) {
                if (isSwizzled || isTiled) {
                    if (aWidth != origWidth || aHeight != origHeight) {
                        uint8 *tmp = new uint8[aWidth * aHeight * desc.bpp / 8];
                        swap(this->data, tmp);
                        updateData(data);
                        swap(this->data, tmp);
                        if (isSwizzled) {
                            Context::swizzleImage(this->data, tmp, aWidth, aHeight, desc.bpp);
                        } else {
                            Context::tileImage(this->data, tmp, aWidth, aHeight, desc.bpp);
                        }
                        delete[] tmp;
                    } else {
                        if (isSwizzled) {
                            Context::swizzleImage(this->data, data, aWidth, aHeight, desc.bpp);
                        } else {
                            Context::tileImage(this->data, data, aWidth, aHeight, desc.bpp);
                        }
                    }
                } else {
                    updateData(data);
                }
            }

            //generateMipMap();

            if (isCube) {
                sceGxmTextureInitCube(&ID, this->data, SceGxmTextureFormat(desc.textureFormat), width, height, mipCount);
            } else if (isSwizzled) {
                sceGxmTextureInitSwizzled(&ID, this->data, SceGxmTextureFormat(desc.textureFormat), width, height, mipCount);
            } else if (isTiled) {
                sceGxmTextureInitTiled(&ID, this->data, SceGxmTextureFormat(desc.textureFormat), width, height, mipCount);
            } else {
                sceGxmTextureInitLinear(&ID, this->data, SceGxmTextureFormat(desc.textureFormat), width, height, mipCount);
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

                } else {
                    sceGxmColorSurfaceInit(&colorSurface,
                        SceGxmColorFormat(desc.targetFormat),
                        isSwizzled ? SCE_GXM_COLOR_SURFACE_SWIZZLED : (isTiled ? SCE_GXM_COLOR_SURFACE_TILED : SCE_GXM_COLOR_SURFACE_LINEAR),
                        SCE_GXM_COLOR_SURFACE_SCALE_NONE,
                        desc.bpp > 32 ? SCE_GXM_OUTPUT_REGISTER_SIZE_64BIT : SCE_GXM_OUTPUT_REGISTER_SIZE_32BIT,
                        aWidth, aHeight, aWidth, this->data);

                    uint32 dsWidth  = ALIGNADDR(width,  SCE_GXM_TILE_SIZEX);
                    uint32 dsHeight = ALIGNADDR(height, SCE_GXM_TILE_SIZEY);

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
                params.width           = aWidth;
                params.height          = aHeight;
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

        void generateMipMap() { // TODO: cubemap
            if (mipCount <= 1) return;

            int w = width;
            int h = height;

            uint8 *src = this->data;
            int srcStride = ALIGNADDR(w, 8) * 4;

            for (int i = 0; i < mipCount - 1; i++) {
                uint8 *dst = src + srcStride * h;
                int dstStride = ALIGNADDR(w / 2, 8) * 4;

                // TODO: check for NPOT
                if (w > 1024 || h > 1024) { // sceGxmTransferDownscale supports blocks less than 1024
                    int blocksX = max(1, w / 1024);
                    int blocksY = max(1, h / 1024);
                    for (int y = 0; y < blocksY; y++) {
                        for (int x = 0; x < blocksX; x++) {
                            int blockWidth  = min(1024, w - x * 1024);
                            int blockHeight = min(1024, h - y * 1024);
                            sceGxmTransferDownscale(
                                SCE_GXM_TRANSFER_FORMAT_U8U8U8U8_ABGR, src, x * 1024, y * 1024, blockWidth, blockHeight, srcStride,
                                SCE_GXM_TRANSFER_FORMAT_U8U8U8U8_ABGR, dst, x * 512,  y * 512,  dstStride,
                                NULL, SCE_GXM_TRANSFER_FRAGMENT_SYNC, NULL);
                        }
                    }
                } else {
                    sceGxmTransferDownscale(
                        SCE_GXM_TRANSFER_FORMAT_U8U8U8U8_ABGR, src, 0, 0, w, h, srcStride,
                        SCE_GXM_TRANSFER_FORMAT_U8U8U8U8_ABGR, dst, 0, 0, dstStride,
                        NULL, SCE_GXM_TRANSFER_FRAGMENT_SYNC, NULL);
                }

                w /= 2;
                h /= 2;
                src = dst;
                srcStride = dstStride;
            }

            sceGxmTextureSetMipFilter(&ID, SCE_GXM_TEXTURE_MIP_FILTER_ENABLED);
        }

        void updateData(void *data) {
            FormatDesc desc = formats[fmt];

            if (aWidth != origWidth || aHeight != origHeight) {
                uint8 *dst = (uint8*)this->data;
                uint8 *src = (uint8*)data;
                for (int y = 0; y < origHeight; y++) {
                    memcpy(dst, src, origWidth * desc.bpp / 8);
                    src += origWidth * desc.bpp / 8;
                    dst += aWidth * desc.bpp / 8;
                }
            } else {
                memcpy(this->data, data, aWidth * aHeight * desc.bpp / 8);
            }
        }

        void update(void *data) {
            if (data) {
                updateData(data);
            }
        }

        void bind(int sampler) {
            if (opt & OPT_PROXY) return;
            ASSERT(ID);

            if (active.textures[sampler] != this) {
                active.textures[sampler] = this;
                sceGxmSetFragmentTexture(Context::gxmContext, sampler, &ID);

                if (opt & OPT_VERTEX) {
                    sceGxmSetVertexTexture(Context::gxmContext, sampler, &ID);
                }
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

        bool         dynamic;

        struct Chunk {
            int frameIndex;
            int iBase, iStart, iCount;
            int vBase, vStart, vCount;
        } chunks[DISPLAY_BUFFER_COUNT];

        Mesh(bool dynamic) : iBuffer(NULL), vBuffer(NULL), dynamic(dynamic) {}

        void init(Index *indices, int iCount, ::Vertex *vertices, int vCount, int aCount) {
            ASSERT(sizeof(GAPI::Vertex) == sizeof(::Vertex));

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

            iBuffer = (Index*)  Context::allocGPU(SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, iCount * sizeof(Index),  SCE_GXM_MEMORY_ATTRIB_READ, &iBufferUID);
            vBuffer = (Vertex*) Context::allocGPU(SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, vCount * sizeof(Vertex), SCE_GXM_MEMORY_ATTRIB_READ, &vBufferUID);

            if (!dynamic) {
                update(indices, iCount, vertices, vCount);
            }
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
                chunk.iStart = chunk.iCount = chunk.iBase;
                chunk.vStart = chunk.vCount = chunk.vBase;
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

        support.shaderBinary   = true;
        support.depthTexture   = true;
        support.shadowSampler  = true;
        support.texNPOT        = true;
        support.texRG          = true;
        support.colorHalf      = true;
        support.texHalfLinear  = true;
        support.texHalf        = true;
        support.clipDist       = true;

        Core::width  = DISPLAY_WIDTH;
        Core::height = DISPLAY_HEIGHT;

        { // gxm
            SceGxmInitializeParams params;
            memset(&params, 0, sizeof(params));
            params.displayQueueMaxPendingCount  = DISPLAY_BUFFER_COUNT - 1;
            params.displayQueueCallback         = displayCallback;
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

        clearColor = vec4(0.0f);

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
    }

    bool hasScene;
    Texture defTarget(DISPLAY_WIDTH, DISPLAY_HEIGHT, 1, OPT_TARGET);

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

            uint32 flags = SCE_GXM_SCENE_VERTEX_TRANSFER_SYNC;
            if (target->opt & OPT_VERTEX) flags |= SCE_GXM_SCENE_FRAGMENT_SET_DEPENDENCY;
            if (target->opt & OPT_DEPEND) flags |= SCE_GXM_SCENE_VERTEX_WAIT_FOR_DEPENDENCY;

            SceGxmColorSurface *colorSurface = (target->fmt == FMT_DEPTH || target->fmt == FMT_SHADOW) ? NULL : &target->colorSurface;

            bool loadDepth  = (Core::reqTarget.op & RT_LOAD_DEPTH);
            bool storeDepth = (Core::reqTarget.op & RT_STORE_DEPTH);

            sceGxmDepthStencilSurfaceSetForceLoadMode  ( &target->depthSurface, loadDepth  ? SCE_GXM_DEPTH_STENCIL_FORCE_LOAD_ENABLED  : SCE_GXM_DEPTH_STENCIL_FORCE_LOAD_DISABLED  );
            sceGxmDepthStencilSurfaceSetForceStoreMode ( &target->depthSurface, storeDepth ? SCE_GXM_DEPTH_STENCIL_FORCE_STORE_ENABLED : SCE_GXM_DEPTH_STENCIL_FORCE_STORE_DISABLED );

            sceGxmBeginScene(Context::gxmContext, flags, target->renderTarget, NULL, NULL, NULL, colorSurface, &target->depthSurface);
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
        active.shader->validate();
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
//        GAPI::Texture *t = Core::active.target;
//        Color32 *color = (Color32*)t->data;
//        return vec4(color->r, color->g, color->b, 255.0f) * (1.0f / 255.0f);
        return vec4(0.0f); // TODO: read from framebuffer
    }
}

#endif