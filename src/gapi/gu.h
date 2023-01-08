#ifndef H_GAPI_GU
#define H_GAPI_GU

#include "core.h"

#include <pspgu.h>
#include <pspgum.h>

#define PROFILE_MARKER(title)
#define PROFILE_LABEL(id, name, label)
#define PROFILE_TIMING(time)

namespace GAPI {

    using namespace Core;

    struct Vertex {
        short2 texCoord;
        ubyte4 color;
        short3 normal;
        short3 coord;
    };

// Shader
    struct Shader {
        void init(Pass pass, int type, int *def, int defCount) {}
        void deinit() {}
        void bind() {}
        void setParam(UniformType uType, const vec4  &value, int count = 1) {}
        void setParam(UniformType uType, const mat4  &value, int count = 1) {}
    };

// Texture
    struct Texture {
        uint8      *memory;
        int        width, height, depth, origWidth, origHeight, origDepth;//todo: depth
        TexFormat  fmt;
        uint32     opt;

        /*depth*/
        Texture(int width, int height, int depth, uint32 opt) : memory(0), width(width), height(height), depth(depth), origWidth(width), origHeight(height), origDepth(depth), fmt(FMT_RGBA), opt(opt) {}

        void init(void *data) {
            ASSERT((opt & OPT_PROXY) == 0);

            opt &= ~(OPT_CUBEMAP | OPT_MIPMAPS);

            if (data) {
                memory = new uint8[width * height * 4];
                update(data);
            } else
                memory = NULL;
        }

        void deinit() {
            if (memory)
                delete[] memory;
        }

        void swizzle(uint8 *out, const uint8 *in, uint32 width, uint32 height) {
            int rowblocks = width / 16;

            for (int j = 0; j < height; j++)
                for (int i = 0; i < width; i++) {
                    int blockx = i / 16;
                    int blocky = j / 8;

                    int x = i - blockx * 16;
                    int y = j - blocky * 8;
                    int block_index   = blockx + blocky * rowblocks;
                    int block_address = block_index * 16 * 8;

                    out[block_address + x + y * 16] = in[i + j * width];
                }
        }

        void generateMipMap() {}

        int getSwizzle() {
            #ifdef TEX_SWIZZLE
                return GU_TRUE;
            #else
                return GU_FALSE
            #endif
        }

        void update(void *data) {
        #ifdef TEX_SWIZZLE
            swizzle(memory, (uint8*)data, width * 4, height);
        #else
            memcpy(memory, data, width * height * 4);
        #endif
        }

        void bind(int sampler) {
            if (!this || (opt & OPT_PROXY)) return;
            ASSERT(memory);

            sceGuTexMode(GU_PSM_8888, 0, 0, getSwizzle());
            sceGuTexImage(0, width, height, width, memory);
        }

        void bindTileCLUT(void *tile, void *clut) {
            ASSERT(tile);
            ASSERT(clut);

            sceGuTexMode(GU_PSM_T4, 0, 0, getSwizzle());
            sceGuClutLoad(1, clut);
            sceGuTexImage(0, width, height, width, tile);
        }

        void unbind(int sampler) {}

        void setFilterQuality(int value) {
            if (value > Settings::LOW)
                opt &= ~OPT_NEAREST;
            else
                opt |= OPT_NEAREST;
        }
    };

// Mesh
    struct Mesh {
        Index        *iBuffer;
        GAPI::Vertex *vBuffer;

        int          iCount;
        int          vCount;
        bool         dynamic;

        Mesh(bool dynamic) : iBuffer(NULL), vBuffer(NULL), dynamic(dynamic) {}

        void init(Index *indices, int iCount, ::Vertex *vertices, int vCount, int aCount) {
            this->iCount  = iCount;
            this->vCount  = vCount;

            if (dynamic)
                return;

            #ifdef EDRAM_MESH
                iBuffer =  (Index*)allocEDRAM(iCount * sizeof(Index)); 
                vBuffer = (Vertex*)allocEDRAM(vCount * sizeof(Vertex)); 
            #else
                iBuffer = new Index[iCount];
                vBuffer = new Vertex[vCount];
            #endif

            update(indices, iCount, vertices, vCount);
        }

        void deinit() {
            if (dynamic)
                return;

            #ifndef EDRAM_MESH
                delete[] iBuffer;
                delete[] vBuffer;
            #endif
        }

        void update(Index *indices, int iCount, ::Vertex *vertices, int vCount) {
            if (dynamic) {
                iBuffer =  (Index*)sceGuGetMemory(iCount * sizeof(Index)); 
                vBuffer = (Vertex*)sceGuGetMemory(vCount * sizeof(Vertex)); 
            }

            if (indices)
                memcpy(iBuffer, indices, iCount * sizeof(indices[0]));
        
            if (vertices) {
                ::Vertex *src = vertices;
                Vertex   *dst = vBuffer;

                for (int i = 0; i < vCount; i++) {
                    dst->texCoord = short2(src->texCoord.x, src->texCoord.y);
                    dst->color    = ubyte4(src->light.x, src->light.y, src->light.z, src->light.w); //color;
                    dst->normal   = src->normal;
                    dst->coord    = src->coord;

                    dst++;
                    src++;
                }
            }
        }

        void bind(const MeshRange &range) const {}

        void initNextRange(MeshRange &range, int &aIndex) const {
            range.aIndex = -1;
        }
    };


    int cullMode, blendMode;

    uint32 *cmdBuf = NULL;
    void   *curBackBuffer;

    static int EDRAM_OFFSET;
    static int EDRAM_SIZE;

    void* allocEDRAM(int size) {
        LOG("EDRAM ALLOC: offset: %d size %d (free %d)\n", EDRAM_OFFSET, size, EDRAM_SIZE - (EDRAM_OFFSET + size));
        if (EDRAM_OFFSET + size > EDRAM_SIZE)
            LOG("! EDRAM overflow !\n");

        void *ptr = ((char*)sceGeEdramGetAddr()) + EDRAM_OFFSET;
        EDRAM_OFFSET += (size + 15) / 16 * 16;
        return ptr;
    }

    void freeEDRAM() {
        EDRAM_OFFSET = (512 * 272 * 2 * 2) + (512 * 272 * 2);
        LOG("EDRAM FREE: offset: %d\n", EDRAM_OFFSET);
    }

    void beginCmdBuf() {
        if (!cmdBuf)
            cmdBuf = new uint32[262144];
        sceGuStart(GU_DIRECT, cmdBuf);
    }

    void submitCmdBuf() {
        ASSERT(cmdBuf);
        sceGuFinish();
        sceGuSync(GU_SYNC_WAIT, GU_SYNC_FINISH);
    }

    void init() {
        LOG("Vendor   : %s\n", "Sony");
        LOG("Renderer : %s\n", "SCE GU");
        LOG("Version  : %s\n", "1.0");

        EDRAM_SIZE = sceGeEdramGetSize();
        LOG("VRAM     : %d\n", EDRAM_SIZE);
        freeEDRAM();

        support.maxAniso       = 0;
        support.maxVectors     = 0;
        support.shaderBinary   = false;
        support.VAO            = false;
        support.VBO            = false;
        support.depthTexture   = false;
        support.shadowSampler  = false;
        support.discardFrame   = false;
        support.texNPOT        = false;
        support.tex3D          = false;
        support.texRG          = false;
        support.texBorder      = false;
        support.texMaxLevel    = false;
        support.colorFloat     = false;
        support.colorHalf      = false;
        support.texFloatLinear = false;
        support.texFloat       = false;
        support.texHalfLinear  = false;
        support.texHalf        = false;
        //support.clipDist       = false;

        Core::width  = 480;
        Core::height = 272;

        sceGuEnable(GU_TEXTURE_2D);
        sceGuDepthFunc(GU_LEQUAL);
        sceGuDepthRange(0x0000, 0xFFFF);
        sceGuClearDepth(0xFFFF);

        sceGuShadeModel(GU_SMOOTH);
        sceGuAlphaFunc(GU_GREATER, 127, 255);

        int swizzle = GU_FALSE;
        #ifdef TEX_SWIZZLE
            swizzle = GU_TRUE;
        #endif

        sceGuClutMode(GU_PSM_5551, 0, 0xFF, 0);
        sceGuTexMode(GU_PSM_T4, 0, 0, swizzle);
        sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
        sceGuTexScale(1.0f, 1.0f);
        sceGuTexOffset(0.0f, 0.0f);
        sceGuTexFilter(GU_LINEAR, GU_LINEAR);
        //sceGuTexFilter(GU_NEAREST, GU_NEAREST);
        sceGuEnable(GU_CLIP_PLANES);

        const ScePspIMatrix4 dith =
            { {-4,  0, -3,  1},
            { 2, -2,  3, -1},
            {-3,  1, -4,  0},
            { 3, -1,  2, -2} };
        sceGuSetDither(&dith);
        sceGuEnable(GU_DITHER);

        sceGuAmbientColor(0xFFFFFFFF);
        sceGuColor(0xFFFFFFFF);
        sceGuClearColor(0x00000000);
        sceGuColorMaterial(GU_AMBIENT | GU_DIFFUSE);
    }

    void deinit() {
        delete[] cmdBuf;
    }

    inline mat4::ProjRange getProjRange() {
        return mat4::PROJ_NEG_POS;
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

    void endFrame() {}

    void resetState() {}

    void bindTarget(Texture *texture, int face) {
/*
                if (!target)
                    sceGuDrawBufferList(GU_PSM_5650, GAPI::curBackBuffer, 512);
                else
                    sceGuDrawBufferList(GU_PSM_5650, target->offset, target->width);
*/
    }

    void discardTarget(bool color, bool depth) {}

    void copyTarget(Texture *dst, int xOffset, int yOffset, int x, int y, int width, int height) {}

    void setVSync(bool enable) {}

    void waitVBlank() {
        sceDisplayWaitVblankStart();
    }

    void clear(bool color, bool depth) {
        uint32 mask = (color ? GU_COLOR_BUFFER_BIT : 0) | (depth ? GU_DEPTH_BUFFER_BIT : 0);
        if (mask) sceGuClear(mask | GU_FAST_CLEAR_BIT);
    }

    void setClearColor(const vec4 &color) {
        ubyte4 c(clamp(int(color.x * 255), 0, 255),
                 clamp(int(color.y * 255), 0, 255),
                 clamp(int(color.z * 255), 0, 255),
                 clamp(int(color.w * 255), 0, 255));
        sceGuClearColor(*((uint32*)&c));
    }

    // Previous: struct Viewport
    void setViewport(const short4 &vp) {
        sceGuOffset(2048 - vp.z / 2, 2048 - vp.w / 2);
        sceGuViewport(2048 + vp.x, 2048 + vp.y, vp.z, vp.w);
    }
    
    /*this was giving errors on menu rendering when method was empty, see RT_ flags too*/
    void setFog(const vec4 &params) {
	uint32 color;
	if(params.w > 0.0f){
		sceGuEnable(GU_FOG);
		// FFP TODO
		color = 0xFF000000
                | (uint32(clamp(params.x * 255.0f, 0.0f, 255.0f)) << 0)
                | (uint32(clamp(params.y * 255.0f, 0.0f, 255.0f)) << 8)
                | (uint32(clamp(params.z * 255.0f, 0.0f, 255.0f)) << 16);
                // from 3DS
		sceGuFog(24.0f, 32.0f * 1024.0f, color);
	}
	else{
		sceGuDisable(GU_FOG);
	}
    }
    
    // from other gapi, could be improved??
    void setScissor(const short4 &s) {
        sceGuScissor(s.x, Core::viewportDef.w - (s.y + s.w), s.x + s.z, Core::viewportDef.w - s.y);
    }

    void setDepthTest(bool enable) {
        if (enable)
            sceGuEnable(GU_DEPTH_TEST);
        else
            sceGuDisable(GU_DEPTH_TEST);
    }

    void setDepthWrite(bool enable) {
        sceGuDepthMask(enable ? GU_FALSE : GU_TRUE);
    }

    void setColorWrite(bool r, bool g, bool b, bool a) {
        sceGuPixelMask(~((r ? 0x000000FF : 0) | (g ? 0x0000FF00 : 0) | (b ? 0x00FF0000 : 0) | (a ? 0xFF000000 : 0)));
    }

    void setAlphaTest(bool enable) {
        if (enable)
            sceGuEnable(GU_ALPHA_TEST);
        else
            sceGuDisable(GU_ALPHA_TEST);
    }

    void setCullMode(int rsMask) {
        cullMode = rsMask;
        switch (rsMask) {
            case RS_CULL_BACK  : sceGuFrontFace(GU_CCW);     break;
            case RS_CULL_FRONT : sceGuFrontFace(GU_CW);      break;
            default            : sceGuDisable(GU_CULL_FACE); return;
        }
        sceGuEnable(GU_CULL_FACE);
    }

    void setBlendMode(int rsMask) {
        blendMode = rsMask;
        switch (rsMask) {
            case RS_BLEND_ALPHA   : sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);    break;
            case RS_BLEND_ADD     : sceGuBlendFunc(GU_ADD, GU_FIX, GU_FIX, 0xffffffff, 0xffffffff);        break;
            case RS_BLEND_MULT    : sceGuBlendFunc(GU_ADD, GU_DST_COLOR, GU_FIX, 0, 0);                    break;
            case RS_BLEND_PREMULT : sceGuBlendFunc(GU_ADD, GU_FIX, GU_ONE_MINUS_SRC_ALPHA, 0xffffffff, 0); break;
            default               : sceGuDisable(GU_BLEND); return;
        }
        sceGuEnable(GU_BLEND);
    }

    void setViewProj(const mat4 &mView, const mat4 &mProj) {
        sceGumMatrixMode(GU_PROJECTION);
        sceGumLoadMatrix((ScePspFMatrix4*)&mProj);
        sceGumMatrixMode(GU_VIEW);
        sceGumLoadMatrix((ScePspFMatrix4*)&mView);
    }

    void updateLights(vec4 *lightPos, vec4 *lightColor, int count) {
        int lightsCount = 0;

        ubyte4 amb;
        amb.x = amb.y = amb.z = clamp(int(active.material.y * 255), 0, 255);
        amb.w = 255;
        sceGuAmbient(*(uint32*)&amb);

        for (int i = 0; i < MAX_LIGHTS; i++) {
            if (lightColor[i].w != 1.0f) {
                sceGuEnable(GU_LIGHT0 + i);
                lightsCount++;
            } else {
                sceGuDisable(GU_LIGHT0 + i);
                continue;
            }

            ScePspFVector3 pos;
            pos.x = lightPos[i].x;
            pos.y = lightPos[i].y;
            pos.z = lightPos[i].z;

            sceGuLight(i, GU_POINTLIGHT, GU_DIFFUSE, &pos);

            ubyte4 color;
            color.x = clamp(int(lightColor[i].x * 255), 0, 255);
            color.y = clamp(int(lightColor[i].y * 255), 0, 255);
            color.z = clamp(int(lightColor[i].z * 255), 0, 255);
            color.w = 255;

            sceGuLightColor(i, GU_DIFFUSE, *(uint32*)&color);
            sceGuLightAtt(i, 1.0f, 0.0f, lightColor[i].w * lightColor[i].w);
        }

        if (lightsCount) {
            sceGuEnable(GU_LIGHTING);
        } else {
            sceGuDisable(GU_LIGHTING);
        }
    }

    void DIP(Mesh *mesh, const MeshRange &range) {
        mat4 m = mModel;
        m.scale(vec3(32767.0f));
        sceGumMatrixMode(GU_MODEL);
        sceGumLoadMatrix((ScePspFMatrix4*)&m);

        sceGumDrawArray(GU_TRIANGLES, GU_TEXTURE_16BIT | GU_COLOR_8888 | GU_NORMAL_16BIT | GU_VERTEX_16BIT | GU_INDEX_16BIT | GU_TRANSFORM_3D, range.iCount, mesh->iBuffer + range.iStart, mesh->vBuffer + range.vStart);
    }

    vec4 copyPixel(int x, int y) {
	//pspgu.h, pspdisplay.h ??
        return vec4(0.0f); // TODO: read from framebuffer
    }
}

#endif
