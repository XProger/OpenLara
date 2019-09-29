#ifndef H_GAPI_SW
#define H_GAPI_SW

#include "core.h"

#define PROFILE_MARKER(title)
#define PROFILE_LABEL(id, name, label)
#define PROFILE_TIMING(time)

#ifdef _OS_LINUX
    #define COLOR_16
#endif

namespace GAPI {

    using namespace Core;

    typedef ::Vertex Vertex;

    #ifdef COLOR_16
        typedef uint16 ColorSW;
    #else
        typedef uint32 ColorSW;
    #endif
    typedef uint16 DepthSW;

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
        int        width, height, origWidth, origHeight;
        TexFormat  fmt;
        uint32     opt;

        Texture(int width, int height, int depth, uint32 opt) : memory(0), width(width), height(height), origWidth(width), origHeight(height), fmt(FMT_RGBA), opt(opt) {}

        void init(void *data) {
            ASSERT((opt & OPT_PROXY) == 0);

            opt &= ~(OPT_CUBEMAP | OPT_MIPMAPS);

            memory = new uint8[width * height * 4];
            if (data) {
                update(data);
            }
        }

        void deinit() {
            if (memory) {
                delete[] memory;
            }
        }

        void generateMipMap() {}

        void update(void *data) {
            memcpy(memory, data, width * height * 4);
        }

        void bind(int sampler) {
            Core::active.textures[sampler] = this;

            if (!this || (opt & OPT_PROXY)) return;
            ASSERT(memory);

//            sceGuTexMode(GU_PSM_8888, 0, 0, getSwizzle());
//            sceGuTexImage(0, width, height, width, memory);
        }

        void bindTileCLUT(void *tile, void *clut) {
            ASSERT(tile);
            ASSERT(clut);

//            sceGuTexMode(GU_PSM_T4, 0, 0, getSwizzle());
//            sceGuClutLoad(1, clut);
//            sceGuTexImage(0, width, height, width, tile);
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

            iBuffer = new Index[iCount];
            vBuffer = new Vertex[vCount];

            update(indices, iCount, vertices, vCount);
        }

        void deinit() {
            delete[] iBuffer;
            delete[] vBuffer;
        }

        void update(Index *indices, int iCount, ::Vertex *vertices, int vCount) {
            if (indices) {
                memcpy(iBuffer, indices, iCount * sizeof(indices[0]));
            }
        
            if (vertices) {
                memcpy(vBuffer, vertices, vCount * sizeof(vertices[0]));
            }
        }

        void bind(const MeshRange &range) const {}

        void initNextRange(MeshRange &range, int &aIndex) const {
            range.aIndex = -1;
        }
    };


    int cullMode, blendMode;

    ColorSW *swColor;
    DepthSW *swDepth;

    mat4    swMatrix;
    vec4    swViewport;

    struct VertexSW {
        float   w;
        short2  coord;
        ubyte4  color;
        short2  uv;
        DepthSW z;
    };

    VertexSW *swVertices;
    int32    swVerticesCount;

    Index  *swIndices;
    int32  swIndicesCount;


    void init() {
        LOG("Renderer : %s\n", "Software");
        LOG("Version  : %s\n", "0.1");
        /*
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
        */
        swDepth    = NULL;        
        swVertices = NULL;
        swIndices  = NULL;

        swVerticesCount = 0;
        swIndicesCount  = 0;
    }

    void deinit() {
        delete[] swDepth;
        delete[] swVertices;
        delete[] swIndices;
    }

    void resize() {
        delete[] swDepth;
        swDepth = new DepthSW[Core::width * Core::height];
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

    void endFrame() {
        //
    }

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
        //sceDisplayWaitVblankStart();
    }

    void clear(bool color, bool depth) {
        if (color) {
            memset(swColor, 0, Core::width * Core::height * sizeof(ColorSW));
        }

        if (depth) {
            memset(swDepth, 0xFF, Core::width * Core::height * sizeof(DepthSW));
        }
    }

    void setClearColor(const vec4 &color) {
        //
    }

    void setViewport(const Viewport &vp) {
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

    void setAlphaTest(bool enable) {
        //
    }

    void setCullMode(int rsMask) {
        cullMode = rsMask;
        //switch (rsMask) {
        //    case RS_CULL_BACK  : sceGuFrontFace(GU_CCW);     break;
        //    case RS_CULL_FRONT : sceGuFrontFace(GU_CW);      break;
        //    default            : sceGuDisable(GU_CULL_FACE); return;
        //}
        //sceGuEnable(GU_CULL_FACE);
    }

    void setBlendMode(int rsMask) {
        blendMode = rsMask;
        //switch (rsMask) {
        //    case RS_BLEND_ALPHA   : sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);    break;
        //    case RS_BLEND_ADD     : sceGuBlendFunc(GU_ADD, GU_FIX, GU_FIX, 0xffffffff, 0xffffffff);        break;
        //    case RS_BLEND_MULT    : sceGuBlendFunc(GU_ADD, GU_DST_COLOR, GU_FIX, 0, 0);                    break;
        //    case RS_BLEND_PREMULT : sceGuBlendFunc(GU_ADD, GU_FIX, GU_ONE_MINUS_SRC_ALPHA, 0xffffffff, 0); break;
        //    default               : sceGuDisable(GU_BLEND); return;
        //}
        //sceGuEnable(GU_BLEND);
    }

    void setViewProj(const mat4 &mView, const mat4 &mProj) {
        //
    }

    void updateLights(vec4 *lightPos, vec4 *lightColor, int count) {
        int lightsCount = 0;

        ubyte4 amb;
        amb.x = amb.y = amb.z = clamp(int(active.material.y * 255), 0, 255);
        amb.w = 255;
    /*
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
        */
    }
    
    struct Edge {
        float A, B, C;

        Edge(const vec3 &a, const vec3 &b) {
            A = a.y - b.y;
            B = b.x - a.x;
            C = -(A * (a.x + b.x) + B * (a.y + b.y)) * 0.5f;
        }

        float evaluate(float x, float y) const {
            return A*x + B*y + C;
        }
    };

    #define VERTEX_CACHE_MAX 4

    Index vertexCache[2][VERTEX_CACHE_MAX];

    void vcacheClear() {
        memset(vertexCache, 0xFF, sizeof(vertexCache));
    }

    void vcacheSet(int in, int out) {
        int i = in % VERTEX_CACHE_MAX;
        vertexCache[0][i] = in;
        vertexCache[1][i] = out;
    }

    Index vcacheGet(int in) {
        int i = in % VERTEX_CACHE_MAX;
        if (vertexCache[0][i] == in) {
            return vertexCache[1][i];
        }
        return 0xFFFF;
    }

    void DrawLine(int x1, int x2, int y, DepthSW z, ColorSW color) {
        if (x2 - x1 == 0) return;

        if (x1 < 0) x1 = 0;
        if (x2 > Core::width - 1) x2 = Core::width - 1;
        if (x2 < x1) return;

        int32 i = y * Core::width;

        for (int x = i + x1; x < i + x2; x++) {
            if (swDepth[x] >= z) {
                swDepth[x] = z;
                swColor[x] = color;
            }
        }
    }

    void DrawTriangle(Index *indices) {
        VertexSW a = swVertices[indices[0]];
        VertexSW b = swVertices[indices[1]];
        VertexSW c = swVertices[indices[2]];

        if (a.w < 0.0f || b.w < 0.0f || c.w < 0.0f) return;

        short2 &ia = a.coord;
        short2 &ib = b.coord;
        short2 &ic = c.coord;

        if ((ib.x - ia.x) * (ic.y - ia.y) -
            (ib.y - ia.y) * (ic.x - ia.x) <= 0) return;

        DepthSW z = DepthSW((uint32(a.z) + uint32(b.z) + uint32(c.z)) / 3.0f); // 16-bit depth range

        if (a.coord.y > b.coord.y) swap(a, b);
        if (a.coord.y > c.coord.y) swap(a, c);
        if (b.coord.y > c.coord.y) swap(b, c);

        ColorSW color = a.color.value;

        int16 minY = ia.y;
        int16 maxY = ic.y;
        if (minY < 0) minY = 0;
        if (maxY > Core::height - 1) maxY = Core::height - 1;
        if (minY > maxY) return; 

        int16 midY = clamp(ib.y, minY, maxY);

        int32 f = ((ic.x - ia.x) << 16) / (ic.y - ia.y);

        if (ia.y != ib.y) {
            int32 f1 = f;
            int32 f2 = ((ib.x - ia.x) << 16) / (ib.y - ia.y);
            int32 x1 = (ia.x << 16) + (minY - ia.y) * f1;
            int32 x2 = (ia.x << 16) + (minY - ia.y) * f2;

            if (f1 > f2) {
                swap(x1, x2);
                swap(f1, f2);
            }

            for (int16 y = minY; y < midY; y++) {
                DrawLine(x1 >> 16, x2 >> 16, y, z, color);
                x1 += f1;
                x2 += f2;
            }
        }

        if (ib.y != ic.y) {
            int32 f1 = f;
            int32 f2 = ((ic.x - ib.x) << 16) / (ic.y - ib.y);
            int32 x1 = (ia.x << 16) + (midY - ia.y) * f1;
            int32 x2 = (ib.x << 16) + (midY - ib.y) * f2;

            if (x1 > x2) {
                swap(x1, x2);
                swap(f1, f2);
            }

            for (int16 y = midY; y < maxY; y++) {
                DrawLine(x1 >> 16, x2 >> 16, y, z, color);
                x1 += f1;
                x2 += f2;
            }
        }
    }

    void DIP(Mesh *mesh, const MeshRange &range) {
        swMatrix   = mViewProj * mModel;
        swViewport = vec4(float(Core::width / 2), float(Core::height / 2), 0, 0);

        if (swVerticesCount < mesh->vCount) {
            delete[] swVertices;
            swVerticesCount = mesh->vCount;
            swVertices = new VertexSW[swVerticesCount];
        }

        if (swIndicesCount < mesh->iCount) {
            delete[] swIndices;
            swIndicesCount = mesh->iCount;
            swIndices = new Index[swIndicesCount];
        }

        vcacheClear();

        uint32 vCount = 0;

        for (int i = 0; i < range.iCount; i++) {
            Index in  = mesh->iBuffer[range.iStart + i];
            Index out = vcacheGet(in);

            if (out == 0xFFFF) {
                out = vCount++;
                VertexSW &result = swVertices[out];
                Vertex   &vertex = mesh->vBuffer[range.vStart + in];

                vec4 c;
                c = swMatrix * vec4(vertex.coord.x, vertex.coord.y, vertex.coord.z, 1.0f); \
                c.x = clamp( swViewport.x * ( c.x / c.w + 1.0f), -16384.0f, 16384.0f); \
                c.y = clamp( swViewport.y * (-c.y / c.w + 1.0f), -16384.0f, 16384.0f); \
                c.z /= c.w;

                result.w     = c.w;
                result.coord = short2( int16(c.x), int16(c.y) );
                result.z     = DepthSW(clamp(c.z, 0.0f, 1.0f) * 0xFFFF);
                result.color = vertex.light; 

                vcacheSet(in, out);
            }

            swIndices[i] = out;
        }

        for (int i = 0; i < range.iCount; i += 3) {
            DrawTriangle(swIndices + i);
        }
    }

    vec4 copyPixel(int x, int y) {
        return vec4(0.0f); // TODO: read from framebuffer
    }
}

#endif
