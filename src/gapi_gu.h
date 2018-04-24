#ifndef H_GAPI_GU
#define H_GAPI_GU

#include "core.h"

namespace GAPI {

    using namespace Core;

    struct Vertex {
        short2 texCoord;
        ubyte4 color;
        short3 normal;
        short3 coord;
    };

    int cullMode, blendMode;

    uint32 *cmdBuf = NULL;

    static int EDRAM_OFFSET;
    static int EDRAM_SIZE;

    void* allocEDRAM(int size) {
        LOG("EDRAM ALLOC: offset: %d size %d (free %d)\n", Core::EDRAM_OFFSET, size, EDRAM_SIZE - (Core::EDRAM_OFFSET + size));
        if (Core::EDRAM_OFFSET + size > EDRAM_SIZE)
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


        support.maxAniso       = 1;
        support.maxVectors     = 0;
        support.shaderBinary   = false;
        support.VAO            = false;
        support.depthTexture   = false;
        support.shadowSampler  = false;
        support.discardFrame   = false;
        support.texNPOT        = false;
        support.texRG          = false;
        support.texBorder      = false;
        support.maxAniso       = false;
        support.colorFloat     = false;
        support.colorHalf      = false;
        support.texFloatLinear = false;
        support.texFloat       = false;
        support.texHalfLinear  = false;
        support.texHalf        = false;

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

    void resetState() {}

    void discardTarget(bool color, bool depth) {}

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

    void setViewport(int x, int y, int w, int h) {
        sceGuOffset(2048 - w / 2, 2048 - h / 2);
        sceGuViewport(2048 + x, 2048 + y, w, h);
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

    void DIP(int iStart, int iCount, Index *iBuffer) {
        mat4 m = mModel;
        m.scale(vec3(32767.0f));
        sceGumMatrixMode(GU_MODEL);
        sceGumLoadMatrix((ScePspFMatrix4*)&m);

        sceGumDrawArray(GU_TRIANGLES, GU_TEXTURE_16BIT | GU_COLOR_8888 | GU_NORMAL_16BIT | GU_VERTEX_16BIT | GU_INDEX_16BIT | GU_TRANSFORM_3D, iCount, active.iBuffer + iStart, active.vBuffer);
    }

    vec4 copyPixel(int x, int y) {
        return vec4(0.0f); // TODO: read from framebuffer
    }
}

#endif