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

void renderInit()
{
    PIXELFORMATDESCRIPTOR pfd;
    memset(&pfd, 0, sizeof(pfd));
    pfd.nSize        = sizeof(pfd);
    pfd.nVersion     = 1;
    pfd.dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.cColorBits   = 32;
    pfd.cRedBits     = 8;
    pfd.cGreenBits   = 8;
    pfd.cBlueBits    = 8;
    pfd.cAlphaBits   = 8;
    pfd.cDepthBits   = 24;

    int format = ChoosePixelFormat(hDC, &pfd);
    SetPixelFormat(hDC, format, &pfd);
    hRC = wglCreateContext(hDC);

    wglMakeCurrent(hDC, hRC);

    glClearColor(0, 0, 0, 1);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);


    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.25f);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0f, float(FRAME_WIDTH) / float(FRAME_HEIGHT), 0.1f, VIEW_DIST);
    glScalef(1, -1, 1);
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
}
#endif

void renderLevelInit()
{
    //
}

void renderLevelFree()
{
    //
}

void setViewport(const RectMinMax &vp)
{
    //glViewport(vp.x0, vp.y0, vp.x1 - vp.x0, vp.y1 - vp.y0);
    //glScissor()
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

    glBegin(GL_QUADS);
    for (int32 i = 0; i < room->info->quadsCount; i++)
    {
        const RoomQuad* f = room->data.quads + i;
        for (int32 j = 0; j < 4; j++)
        {
            const RoomVertex* v = vertices + f->indices[j];
            glColor3ub(v->cR, v->cG, v->cB);
            glVertex3s(v->x << 10, v->y << 8, v->z << 10);
        }
    }
    glEnd();

    glBegin(GL_TRIANGLES);
    for (int32 i = 0; i < room->info->trianglesCount; i++)
    {
        const RoomTriangle* f = room->data.triangles + i;
        for (int32 j = 0; j < 3; j++)
        {
            const RoomVertex* v = vertices + f->indices[j];
            glColor3ub(v->cR, v->cG, v->cB);
            glVertex3s(v->x << 10, v->y << 8, v->z << 10);
        }
    }
    glEnd();
}

void renderMesh(const Mesh* mesh)
{
    updateViewMatrix();

    const uint8* ptr = (uint8*)mesh + sizeof(Mesh);

    const MeshVertex* vertices = (MeshVertex*)ptr;
    ptr += mesh->vCount * sizeof(MeshVertex);

    MeshQuad* q = (MeshQuad*)ptr;
    ptr += mesh->rCount * sizeof(MeshQuad);

    MeshTriangle* t = (MeshTriangle*)ptr;

    glBegin(GL_QUADS);
    for (int32 i = 0; i < mesh->rCount + mesh->crCount; i++)
    {
        const MeshQuad* f = q + i;
        for (int32 j = 0; j < 4; j++)
        {
            const MeshVertex* v = vertices + f->indices[j];
            glColor3ub(255, 255, 255);
            glVertex3s(v->x, v->y, v->z);
        }
    }
    glEnd();

    glBegin(GL_TRIANGLES);
    for (int32 i = 0; i < mesh->tCount + mesh->ctCount; i++)
    {
        const MeshTriangle* f = t + i;
        for (int32 j = 0; j < 3; j++)
        {
            const MeshVertex* v = vertices + f->indices[j];
            glColor3ub(255, 255, 255);
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
    return NULL;
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
