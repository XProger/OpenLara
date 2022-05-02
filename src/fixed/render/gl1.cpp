#ifndef H_GL1
#define H_GL1

#include <common.h>
#include <gl/GL.h>
#include <gl/GLU.h>

#define FIX2FLT (1.0f / 0x4000)

#ifdef __WIN32__
extern HWND hWnd;
extern HDC hDC;
HGLRC hRC;

float FRAME_PERSP;

#define FACE_TEXTURE    0x07FF

#define MAX_TEX_TILES   16

GLuint texTiles[MAX_TEX_TILES];
GLuint texBackground;
/*
struct RoomBufVertex
{
    uint8 x, y, z, _pad1;
    uint8 r, g, b, _pad2;
    int8 nx, ny, nz, _pad3;
    uint16 u, v;
};

struct RoomInst
{
    struct Part
    {
        int32 tile;
        //
    };

    RoomInst(Room* room)
    {
        RoomBufVertex vertices[0xFFFF];
        uint16 indices[1024];
    }
};
*/
void renderInit()
{
    PIXELFORMATDESCRIPTOR pfd;
    memset(&pfd, 0, sizeof(pfd));
    pfd.nSize      = sizeof(pfd);
    pfd.nVersion   = 1;
    pfd.dwFlags    = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.cColorBits = 32;
    pfd.cRedBits   = 8;
    pfd.cGreenBits = 8;
    pfd.cBlueBits  = 8;
    pfd.cAlphaBits = 8;
    pfd.cDepthBits = 24;

    int format = ChoosePixelFormat(hDC, &pfd);
    SetPixelFormat(hDC, format, &pfd);
    hRC = wglCreateContext(hDC);

    wglMakeCurrent(hDC, hRC);

    glClearColor(0, 0, 0, 1);

    glEnable(GL_CULL_FACE);
    //glEnable(GL_SCISSOR_TEST);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_ALPHA_TEST);

    glFrontFace(GL_CW);
    glCullFace(GL_BACK);
    glDepthMask(GL_TRUE);
    glAlphaFunc(GL_GREATER, 0.25f);

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glScalef(1.0f / 255.0f, 1.0f / 255.0f, 1.0f / 255.0f);

    glMatrixMode(GL_MODELVIEW);
}

void renderFree()
{
    wglMakeCurrent(0, 0);
    wglDeleteContext(hRC);
    ReleaseDC(hWnd, hDC);
}

void renderSwap()
{
    SwapBuffers(hDC);

    float aspect = float(FRAME_WIDTH) / float(FRAME_HEIGHT);
    FRAME_PERSP = aspect / tanf(60.0f * 0.5f * 3.1495f / 180.0f);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0f, aspect, 0.1f, 1024 * 16);
    glScalef(1, -1, 1);

    glMatrixMode(GL_MODELVIEW);

    glViewport(0, 0, FRAME_WIDTH, FRAME_HEIGHT);
}
#endif

void renderLevelInit()
{
    uint8 data[256 * 256 * 4];

    glGenTextures(MAX_TEX_TILES, texTiles);

    for (int32 i = 0; i < level.tilesCount; i++)
    {
        const uint8* idx = level.tiles + i * 256 * 256;

        for (int32 j = 0; j < 256 * 256 * 4; j += 4)
        {
            int32 index = *idx++;
            const uint8* c = (uint8*)level.palette + index * 3;
            data[j + 0] = c[0];
            data[j + 1] = c[1];
            data[j + 2] = c[2];
            data[j + 3] = index ? 255 : 0;
        }

        glBindTexture(GL_TEXTURE_2D, texTiles[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
}

void renderLevelFree()
{
    //
}

void setViewport(const RectMinMax &vp)
{
    //glScissor(vp.x0, FRAME_HEIGHT - vp.y1, vp.x1 - vp.x0, vp.y1 - vp.y0);
    viewport = vp;
}

void setPaletteIndex(int32 index)
{

}

void clear()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void updateProjMatrix()
{
    glMatrixMode(GL_PROJECTION);
    //
    glMatrixMode(GL_MODELVIEW);
}

void updateViewMatrix()
{
    const Matrix &m = matrixGet();
    float f[4][4];
    f[0][0] = m.e00 * FIX2FLT;
    f[0][1] = m.e10 * FIX2FLT;
    f[0][2] = -m.e20 * FIX2FLT;
    f[0][3] = 0.0f;

    f[1][0] = m.e01 * FIX2FLT;
    f[1][1] = m.e11 * FIX2FLT;
    f[1][2] = -m.e21 * FIX2FLT;
    f[1][3] = 0.0f;

    f[2][0] = m.e02 * FIX2FLT;
    f[2][1] = m.e12 * FIX2FLT;
    f[2][2] = -m.e22 * FIX2FLT;
    f[2][3] = 0.0f;

    f[3][0] = m.e03 * FIX2FLT;
    f[3][1] = m.e13 * FIX2FLT;
    f[3][2] = -m.e23 * FIX2FLT;
    f[3][3] = 1.0f;

    glLoadMatrixf(&f[0][0]);
}

void renderRoom(const Room* room)
{
    updateViewMatrix();

    const RoomVertex* vertices = room->data.vertices;

    int32 tileIndex = -1;
    int16 uv[4][2];

    glEnable(GL_TEXTURE_2D);

    for (int32 i = 0; i < room->info->quadsCount; i++)
    {
        const RoomQuad* f = room->data.quads + i;
        const Texture* tex = level.textures + (f->flags & FACE_TEXTURE);

        uint32 uv01 = tex->uv01;
        uint32 uv23 = tex->uv23;

        uv[0][0] = (uv01 >> 8) & 0xFF;
        uv[0][1] = (uv01 >> 24) & 0xFF;
        uv[1][0] = (uv01) & 0xFF;
        uv[1][1] = (uv01 >> 16) & 0xFF;
        uv[2][0] = (uv23 >> 8) & 0xFF;
        uv[2][1] = (uv23 >> 24) & 0xFF;
        uv[3][0] = (uv23) & 0xFF;
        uv[3][1] = (uv23 >> 16) & 0xFF;

        if (tileIndex != tex->tile)
        {
            if (tileIndex != -1) {
                glEnd();
            }

            tileIndex = tex->tile;


            glBindTexture(GL_TEXTURE_2D, texTiles[tileIndex]);
            glBegin(GL_QUADS);
        }

        for (int32 j = 0; j < 4; j++)
        {
            const RoomVertex* v = vertices + f->indices[j];
            glColor3ub(v->g, v->g, v->g);
            glTexCoord2sv(uv[j]);
            glVertex3s(v->x << 8, v->y << 8, v->z << 8);
        }
    }

    if (tileIndex != -1)
    {
        glEnd();
        tileIndex = -1;
    }

    for (int32 i = 0; i < room->info->trianglesCount; i++)
    {
        const RoomTriangle* f = room->data.triangles + i;

        const Texture* tex = level.textures + (f->flags & FACE_TEXTURE);
        uint32 uv01 = tex->uv01;
        uint32 uv23 = tex->uv23;

        uv[0][0] = (uv01 >> 8) & 0xFF;
        uv[0][1] = (uv01 >> 24) & 0xFF;
        uv[1][0] = (uv01) & 0xFF;
        uv[1][1] = (uv01 >> 16) & 0xFF;
        uv[2][0] = (uv23 >> 8) & 0xFF;
        uv[2][1] = (uv23 >> 24) & 0xFF;

        if (tileIndex != tex->tile)
        {
            if (tileIndex != -1) {
                glEnd();
            }
            tileIndex = tex->tile;

            glBindTexture(GL_TEXTURE_2D, texTiles[tileIndex]);
            glBegin(GL_TRIANGLES);
        }

        for (int32 j = 0; j < 3; j++)
        {
            const RoomVertex* v = vertices + f->indices[j];
            glColor3ub(v->g, v->g, v->g);
            glTexCoord2sv(uv[j]);
            glVertex3s(v->x << 8, v->y << 8, v->z << 8);
        }
    }

    if (tileIndex != -1)
    {
        glEnd();
    }
}

void renderMesh(const Mesh* mesh)
{
    updateViewMatrix();

    const uint8* ptr = (uint8*)mesh + sizeof(Mesh);

    const MeshVertex* vertices = (MeshVertex*)ptr;
    ptr += mesh->vCount * sizeof(MeshVertex);
    ptr += mesh->vCount * (mesh->hasNormals ? sizeof(vec3s) : sizeof(uint16));

    MeshQuad* r = (MeshQuad*)ptr;
    ptr += mesh->rCount * sizeof(MeshQuad);
    MeshTriangle* t = (MeshTriangle*)ptr;
    ptr += mesh->tCount * sizeof(MeshTriangle);
    MeshQuad* cr = (MeshQuad*)ptr;
    ptr += mesh->crCount * sizeof(MeshQuad);
    MeshTriangle* ct = (MeshTriangle*)ptr;

    glEnable(GL_TEXTURE_2D);
    glColor3ub(255, 255, 255);

    int32 tileIndex = -1;
    int16 uv[4][2];

    for (int32 i = 0; i < mesh->rCount; i++)
    {
        const MeshQuad* f = r + i;

        const Texture* tex = level.textures + (f->flags & FACE_TEXTURE);

        uint32 uv01 = tex->uv01;
        uint32 uv23 = tex->uv23;

        uv[0][0] = (uv01 >> 8) & 0xFF;
        uv[0][1] = (uv01 >> 24) & 0xFF;
        uv[1][0] = (uv01) & 0xFF;
        uv[1][1] = (uv01 >> 16) & 0xFF;
        uv[2][0] = (uv23 >> 8) & 0xFF;
        uv[2][1] = (uv23 >> 24) & 0xFF;
        uv[3][0] = (uv23) & 0xFF;
        uv[3][1] = (uv23 >> 16) & 0xFF;

        if (tileIndex != tex->tile)
        {
            if (tileIndex != -1) {
                glEnd();
            }

            tileIndex = tex->tile;

            glBindTexture(GL_TEXTURE_2D, texTiles[tileIndex]);
            glBegin(GL_QUADS);
        }

        for (int32 j = 0; j < 4; j++)
        {
            const MeshVertex* v = vertices + f->indices[j];
            ASSERT(f->indices[j] < mesh->vCount);
            glTexCoord2sv(uv[j]);
            glVertex3s(v->x, v->y, v->z);
        }
    }

    if (tileIndex != -1)
    {
        glEnd();
        tileIndex = -1;
    }

    for (int32 i = 0; i < mesh->tCount; i++)
    {
        const MeshTriangle* f = t + i;

        const Texture* tex = level.textures + (f->flags & FACE_TEXTURE);

        uint32 uv01 = tex->uv01;
        uint32 uv23 = tex->uv23;

        uv[0][0] = (uv01 >> 8) & 0xFF;
        uv[0][1] = (uv01 >> 24) & 0xFF;
        uv[1][0] = (uv01) & 0xFF;
        uv[1][1] = (uv01 >> 16) & 0xFF;
        uv[2][0] = (uv23 >> 8) & 0xFF;
        uv[2][1] = (uv23 >> 24) & 0xFF;

        if (tileIndex != tex->tile)
        {
            if (tileIndex != -1) {
                glEnd();
            }

            tileIndex = tex->tile;

            glBindTexture(GL_TEXTURE_2D, texTiles[tileIndex]);
            glBegin(GL_TRIANGLES);
        }

        for (int32 j = 0; j < 3; j++)
        {
            const MeshVertex* v = vertices + f->indices[j];
            ASSERT(f->indices[j] < mesh->vCount);
            glTexCoord2sv(uv[j]);
            glVertex3s(v->x, v->y, v->z);
        }
    }

    if (tileIndex != -1)
    {
        glEnd();
    }

    glDisable(GL_TEXTURE_2D);

    glBegin(GL_QUADS);
    for (int32 i = 0; i < mesh->crCount; i++)
    {
        const MeshQuad* f = cr + i;
        uint8* c = (uint8*)level.palette + f->flags * 3;
        for (int32 j = 0; j < 4; j++)
        {
            const MeshVertex* v = vertices + f->indices[j];
            ASSERT(f->indices[j] < mesh->vCount);
            glColor3ub(c[0], c[1], c[2]);
            glVertex3s(v->x, v->y, v->z);
        }
    }
    glEnd();

    glBegin(GL_TRIANGLES);
    for (int32 i = 0; i < mesh->ctCount; i++)
    {
        const MeshTriangle* f = ct + i;
        uint8* c = (uint8*)level.palette + f->flags * 3;
        for (int32 j = 0; j < 3; j++)
        {
            const MeshVertex* v = vertices + f->indices[j];
            ASSERT(f->indices[j] < mesh->vCount);
            glColor3ub(c[0], c[1], c[2]);
            glVertex3s(v->x, v->y, v->z);
        }
    }
    glEnd();
}

void renderShadow(int32 x, int32 z, int32 sx, int32 sz)
{

}

void renderSprite(int32 vx, int32 vy, int32 vz, int32 vg, int32 index)
{

}

void renderGlyph(int32 vx, int32 vy, int32 index)
{

}

void renderBorder(int32 x, int32 y, int32 width, int32 height, int32 shade, int32 color1, int32 color2, int32 z)
{

}

void renderBar(int32 x, int32 y, int32 width, int32 value, BarType type)
{

}

void renderBackground(const void* background)
{

}

void* copyBackground()
{
    return &texBackground;
}

int32 boxIsVisible_c(const AABBs* box)
{
    return 1;
}

int32 sphereIsVisible_c(int32 x, int32 y, int32 z, int32 r)
{
    return 1;
}

void flush_c()
{

}

#endif
