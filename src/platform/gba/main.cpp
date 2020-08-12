#ifndef _WIN32
#include <gba_console.h>
#include <gba_video.h>
#include <gba_timers.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_input.h>
#include <gba_dma.h>
#include <gba_affine.h>
#include <fade.h>

#include "LEVEL1_PHD.h"
#endif

#include "common.h"

//#define PROFILE

#if defined(USE_MODE_5) || defined(_WIN32)
extern uint16 palette[256];
#endif

extern uint8 lightmap[256 * 32];

extern Vertex gVertices[MAX_VERTICES];
extern uint32 gVerticesCount;

extern Face gFaces[MAX_FACES];
extern int32 gFacesCount;

extern Rect clip;

extern const uint8* tiles[15];

extern const uint8* curTile;
uint32 tilesCount;

extern int32 fps;

uint16 camRotY = 16 << 8;

extern int32 camSinY;
extern int32 camCosY;

extern int32 camX;
extern int32 camY;
extern int32 camZ;

extern uint16       roomsCount;
extern const Room*  rooms;

extern uint32         texturesCount;
extern const Texture* textures;

extern const Sprite*  sprites;

extern uint32                spriteSequencesCount;
extern const SpriteSequence* spriteSequences;

extern int32 seqGlyphs;

extern const uint8*  meshData;
extern const uint32* meshOffsets;

extern const int32*  nodes;
extern const Model*  models;

extern void transform(int32 vx, int32 vy, int32 vz, int32 vg, int32 x, int32 y, int32 z);
extern void faceAddTriangle(uint16 flags, const Index* indices, int32 startVertex);
extern void faceAddQuad(uint16 flags, const Index* indices, int32 startVertex);
extern void flush();
extern void initRender();

void drawRoom(int16 roomIndex) {
    const Room *room = rooms;

    //Room::Portal *portals;
    uint16 portalsCount;

    //Room::Sector* sectors;
    uint16 zSectors, xSectors;

    //uint16 ambient;

    //Room::Light* lights;
    uint16 lightsCount;

    //Room::Mesh* meshes;
    uint16 meshesCount;

    uint8 *ptr = (uint8*)room;

    while (roomIndex--) {
        uint32 dataSize;
        memcpy(&dataSize, &room->dataSize, sizeof(dataSize));
        ptr += sizeof(Room) + dataSize * 2;
        
        portalsCount = *((uint16*)ptr);
        ptr += 2;
        //portals = (Room::Portal*)ptr;
        ptr += sizeof(Room::Portal) * portalsCount;

        zSectors = *((uint16*)ptr);
        ptr += 2;
        xSectors = *((uint16*)ptr);
        ptr += 2;
        //sectors = (Room::Sector*)sectors;
        ptr += sizeof(Room::Sector) * zSectors * xSectors;

        //ambient = *((uint16*)ptr);
        ptr += 2;

        lightsCount = *((uint16*)ptr);
        ptr += 2;
        //lights = (Room::Light*)ptr;
        ptr += sizeof(Room::Light) * lightsCount;

        meshesCount = *((uint16*)ptr);
        ptr += 2;
        //meshes = (Room::Mesh*)ptr;
        ptr += sizeof(Room::Mesh) * meshesCount;

        ptr += 2 + 2; // skip alternateRoom and flags

        room = (Room*)ptr;
    }

    ptr += sizeof(Room);

    uint16 vCount = *((uint16*)ptr);
    ptr += 2;
    Room::Vertex* vertices = (Room::Vertex*)ptr;
    ptr += sizeof(Room::Vertex) * vCount;

    // non-aligned data
    int32 roomX;
    int32 roomZ;
    memcpy(&roomX, &room->info.x, sizeof(roomX));
    memcpy(&roomZ, &room->info.z, sizeof(roomZ));

    int32 dx = -camX + roomX;
    int32 dy = -camY;
    int32 dz = -camZ + roomZ;

    int32 startVertex = gVerticesCount;

    for (uint16 i = 0; i < vCount; i++) {
        const Room::Vertex &v = vertices[i];
        transform(v.x, v.y, v.z, v.lighting, dx, dy, dz);
    }

    uint16 qCount = *((uint16*)ptr);
    ptr += 2;
    Quad* quads = (Quad*)ptr;
    ptr += sizeof(Quad) * qCount;

    for (uint16 i = 0; i < qCount; i++) {
        faceAddQuad(quads[i].flags, quads[i].indices, startVertex);
    }

    uint16 tCount = *((uint16*)ptr);
    ptr += 2;
    Triangle* triangles = (Triangle*)ptr;
    ptr += sizeof(Triangle) * tCount;

    for (uint16 i = 0; i < tCount; i++) {
        faceAddTriangle(triangles[i].flags, triangles[i].indices, startVertex);
    }
}

void drawMesh(int16 meshIndex, int32 x, int32 y, int32 z) {
    uint32 offset = meshOffsets[meshIndex];
    const uint8* ptr = meshData + offset;

    //int16 cx = *(int16*)ptr; ptr += 2;
    //int16 cy = *(int16*)ptr; ptr += 2;
    //int16 cz = *(int16*)ptr; ptr += 2;
    //int16 r  = *(int16*)ptr; ptr += 2;
    //ptr += 2; // skip flags
    ptr += 2 * 5;

    int16 vCount = *(int16*)ptr; ptr += 2;
    const int16* vertices = (int16*)ptr;
    ptr += vCount * 3 * sizeof(int16);

    int16 nCount = *(int16*)ptr; ptr += 2;
    //const int16* normals = (int16*)ptr;
    if (nCount > 0) { // normals
        ptr += nCount * 3 * sizeof(int16);
    } else { // intensity
        ptr += vCount * sizeof(int16);
    }

    int16     rCount = *(int16*)ptr; ptr += 2;
    Quad*     rFaces = (Quad*)ptr; ptr += rCount * sizeof(Quad);

    int16     tCount = *(int16*)ptr; ptr += 2;
    Triangle* tFaces = (Triangle*)ptr; ptr += tCount * sizeof(Triangle);

    int16     crCount = *(int16*)ptr; ptr += 2;
    Quad*     crFaces = (Quad*)ptr; ptr += crCount * sizeof(Quad);

    int16     ctCount = *(int16*)ptr; ptr += 2;
    Triangle* ctFaces = (Triangle*)ptr; ptr += ctCount * sizeof(Triangle);

    int32 startVertex = gVerticesCount;

    int32 dx = x - camX;
    int32 dy = y - camY;
    int32 dz = z - camZ;

    const int16* v = vertices;
    for (uint16 i = 0; i < vCount; i++) {
        transform(v[0], v[1], v[2], 4096, dx, dy, dz);
        v += 3;
    }

    for (int i = 0; i < rCount; i++) {
        faceAddQuad(rFaces[i].flags, rFaces[i].indices, startVertex);
    }

    for (int i = 0; i < crCount; i++) {
        faceAddQuad(crFaces[i].flags | FACE_COLORED, crFaces[i].indices, startVertex);
    }

    for (int i = 0; i < tCount; i++) {
        faceAddTriangle(tFaces[i].flags, tFaces[i].indices, startVertex);
    }

    for (int i = 0; i < ctCount; i++) {
        faceAddTriangle(ctFaces[i].flags | FACE_COLORED, ctFaces[i].indices, startVertex);
    }
}

void drawModel(int32 modelIndex, int32 x, int32 y, int32 z) {
    const Model* model = models + modelIndex;

    // non-aligned access
    uint32 node, frame;
    memcpy(&node,  &model->node,  sizeof(node));
    memcpy(&frame, &model->frame, sizeof(frame));

    Node bones[32];
    memcpy(bones, nodes + node, (model->mCount - 1) * sizeof(Node));

    const Node* n = bones;

    struct StackItem {
        int32 x, y, z;
    } stack[4];
    StackItem *s = stack;

    drawMesh(model->mStart, x, y, z);

    for (int i = 1; i < model->mCount; i++) {
        if (n->flags & 1) {
            s--;
            x = s->x;
            y = s->y;
            z = s->z;
        }

        if (n->flags & 2) {
            s->x = x;
            s->y = y;
            s->z = z;
            s++;
        }

        x += n->x;
        y += n->y;
        z += n->z;
        n++;

        drawMesh(model->mStart + i, x, y, z);
    }
}

void readLevel(const uint8 *data) {
    tilesCount = *((uint32*)(data + 4));
    for (uint32 i = 0; i < tilesCount; i++) {
        tiles[i] = data + 8 + 256 * 256 * i;
    }

    roomsCount = *((uint16*)(data + 720908));
    rooms = (Room*)(data + 720908 + 2);

    texturesCount = *((uint32*)(data + 1271686));
    textures = (Texture*)(data + 1271686 + 4);

    sprites = (Sprite*)(data + 1289634);

    spriteSequencesCount = *((uint32*)(data + 1292130));
    spriteSequences = (SpriteSequence*)(data + 1292130 + 4);

    for (uint32 i = 0; i < spriteSequencesCount; i++) {
        if (spriteSequences[i].type == 190) {
            seqGlyphs = i;
            break;
        }
    }

    meshData = data + 908172 + 4;
    meshOffsets = (uint32*)(data + 975724 + 4);

    nodes = (int32*)(data + 990318);

    models = (Model*)(data + 1270670);

    const uint8* f_lightmap = data + 1320576;
    memcpy(lightmap, f_lightmap, sizeof(lightmap));

#if !(defined(USE_MODE_5) || defined(_WIN32))
    uint16 palette[256];
#endif

    const uint8* f_palette = data + 1328768;

    const uint8* p = f_palette;

    for (int i = 0; i < 256; i++) {
        palette[i] = (p[0] >> 1) | ((p[1] >> 1) << 5) | ((p[2] >> 1) << 10);
        p += 3;
    }
   
#ifndef _WIN32
    palette[1] = RGB8(0, 255, 0);

    #ifndef USE_MODE_5
        SetPalette(palette);
    #endif
#endif
}

#ifdef _WIN32
    #define CAM_SPEED     (1 << 3)
    #define CAM_ROT_SPEED (1 << 3)
#else
    #define CAM_SPEED     (1 << 6)
    #define CAM_ROT_SPEED (1 << 8)
#endif


void updateCamera() {
    if (keys[IK_LEFT])  camRotY -= CAM_ROT_SPEED;
    if (keys[IK_RIGHT]) camRotY += CAM_ROT_SPEED;

    {
        ALIGN4 ObjAffineSource src;
        ALIGN4 ObjAffineDest   dst;

        src.sX    = 0x0100;
        src.sY    = 0x0100;
        src.theta = camRotY;

        ObjAffineSet(&src, &dst, 1, 2);

        camCosY = dst.pd << 8;
        camSinY = dst.pc << 8;
    }

    int32 dx = camSinY;
    int32 dz = camCosY;

    dx *= CAM_SPEED;
    dz *= CAM_SPEED;

    dx >>= 16;
    dz >>= 16;

    if (keys[IK_UP]) {
        camX += int32(dx);
        camZ += int32(dz);
    }

    if (keys[IK_DOWN]) {
        camX -= int32(dx);
        camZ -= int32(dz);
    }

    if (keys[IK_L]) {
        camX -= int32(dz);
        camZ += int32(dx);
    }

    if (keys[IK_R]) {
        camX += int32(dz);
        camZ -= int32(dx);
    }

    if (keys[IK_A]) camY -= CAM_SPEED;
    if (keys[IK_B]) camY += CAM_SPEED;

    clip = { 0, 0, FRAME_WIDTH, FRAME_HEIGHT };
}

void drawNumber(int32 number, int32 x, int32 y) {
    const int32 widths[] = { 12, 8, 10, 10, 10, 10, 10, 10, 10, 10 };

    while (number > 0) {
        x -= widths[number % 10];
        drawGlyph(52 + (number % 10), x, y);
        number /= 10;
    }
}

void update(int32 frames) {
    for (int32 i = 0; i < frames; i++) {
        updateCamera();
    }
}

void render() {
    clear();

    drawRoom(6);
    flush();

    drawRoom(0);
    drawModel(0, 75162, 3072 - 512, 5000 + 1024);
    flush();

    drawNumber(fps, WIDTH, 16);
}

#ifdef _WIN32
HDC hDC;

void VBlankIntrWait() {
    #ifdef USE_MODE_5
        for (int i = 0; i < WIDTH * HEIGHT; i++) {
            uint16 c = fb[i];
            VRAM[i] = (((c << 3) & 0xFF) << 16) | ((((c >> 5) << 3) & 0xFF) << 8) | ((c >> 10 << 3) & 0xFF) | 0xFF000000;
        }
    #else
        for (int i = 0; i < WIDTH * HEIGHT; i++) {
            uint16 c = palette[fb[i]];
            VRAM[i] = (((c << 3) & 0xFF) << 16) | ((((c >> 5) << 3) & 0xFF) << 8) | ((c >> 10 << 3) & 0xFF) | 0xFF000000;
        }
    #endif

    const BITMAPINFO bmi = { sizeof(BITMAPINFOHEADER), WIDTH, -HEIGHT, 1, 32, BI_RGB, 0, 0, 0, 0, 0 };
    StretchDIBits(hDC, 0, 0, 240 * WND_SCALE, 160 * WND_SCALE, 0, 0, WIDTH, HEIGHT, VRAM, &bmi, DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_DESTROY :
            PostQuitMessage(0);
            break;
        case WM_KEYDOWN :
        case WM_KEYUP   : {
            InputKey key = IK_MAX;
            switch (wParam) {
                case VK_UP    : key = IK_UP;    break;
                case VK_RIGHT : key = IK_RIGHT; break;
                case VK_DOWN  : key = IK_DOWN;  break;
                case VK_LEFT  : key = IK_LEFT;  break;
                case 'Z'      : key = IK_A;     break;
                case 'X'      : key = IK_B;     break;
                case 'A'      : key = IK_L;     break;
                case 'S'      : key = IK_R;     break;
            }
            if (key != IK_MAX) {
                keys[key] = msg != WM_KEYUP;
            }
            break;
        }
        default :
            return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}
#endif

int32 frameIndex = 0;
int32 fpsCounter = 0;

void vblank() {
    frameIndex++;
}

int main(void) {
#ifdef _WIN32
    {
        FILE *f = fopen("C:/Projects/TR/TR1_ANDROID/LEVEL1.PHD", "rb");
        fseek(f, 0, SEEK_END);
        int32 size = ftell(f);
        fseek(f, 0, SEEK_SET);
        LEVEL1_PHD = new uint8[size];
        fread(LEVEL1_PHD, 1, size, f);
        fclose(f);
    }
#else
    // set low latency mode via WAITCNT register (thanks to GValiente)
    #define BIT_SET(y, flag)    (y |= (flag))
    #define REG_WAITCNT_NV      *(u16*)(REG_BASE + 0x0204)

    BIT_SET(REG_WAITCNT_NV, 0x0008 | 0x0010 | 0x4000);
#endif

    initRender();

    readLevel(LEVEL1_PHD);

#ifdef _WIN32
    RECT r = { 0, 0, 240 * WND_SCALE, 160 * WND_SCALE };

    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, false);
    int wx = (GetSystemMetrics(SM_CXSCREEN) - (r.right - r.left)) / 2;
    int wy = (GetSystemMetrics(SM_CYSCREEN) - (r.bottom - r.top)) / 2;

    HWND hWnd = CreateWindow("static", "OpenLara GBA", WS_OVERLAPPEDWINDOW, wx + r.left, wy + r.top, r.right - r.left, r.bottom - r.top, 0, 0, 0, 0);
    hDC = GetDC(hWnd);

    SetWindowLong(hWnd, GWL_WNDPROC, (LONG)&wndProc);
    ShowWindow(hWnd, SW_SHOWDEFAULT);

    MSG msg;

    do {
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {

            update(1);
            render();

            VBlankIntrWait();
        }
    } while (msg.message != WM_QUIT);

#else
    irqInit();
    irqSet(IRQ_VBLANK, vblank);
    irqEnable(IRQ_VBLANK);

    uint16 mode = BG2_ON | BACKBUFFER;

    #ifdef USE_MODE_5
        mode |= MODE_5;

        REG_BG2PA = 256 - 64 - 16 - 4 - 1;
        REG_BG2PD = 256 - 48 - 2;
    #else
        mode |= MODE_4;
    #endif

    int32 lastFrameIndex = -1;

    #ifdef PROFILE
        int counter = 0;
    #endif

    while (1) {
        //VBlankIntrWait();

    #ifdef PROFILE
        if (counter++ >= 10) return 0;
    #endif

        SetMode(mode ^= BACKBUFFER);
        fb ^= 0xA000;

        scanKeys();
        uint16 key = keysDown() | keysHeld();
        keys[IK_UP]    = (key & KEY_UP);
        keys[IK_RIGHT] = (key & KEY_RIGHT);
        keys[IK_DOWN]  = (key & KEY_DOWN);
        keys[IK_LEFT]  = (key & KEY_LEFT);
        keys[IK_A]     = (key & KEY_A);
        keys[IK_B]     = (key & KEY_B);
        keys[IK_L]     = (key & KEY_L);
        keys[IK_R]     = (key & KEY_R);

        int32 frame = frameIndex;
        update(frame - lastFrameIndex);
        lastFrameIndex = frame;

        render();

        fpsCounter++;
        if (frameIndex >= 60) {
            frameIndex -= 60;
            lastFrameIndex -= 60;

            fps = fpsCounter;

            fpsCounter = 0;
        }

    }
#endif
}
