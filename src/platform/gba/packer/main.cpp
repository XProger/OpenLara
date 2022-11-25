#include "common.h"
#include "IMA.h"
#include "TR1_PC.h"
#include "TR1_PSX.h"
#include "out_GBA.h"
#include "out_3DO.h"
#include "out_32X.h"

TR1_PC* pc[LVL_MAX];
TR1_PSX* psx[LVL_MAX];

void dumpLightmap(const char* fileName, TR1_PC* level)
{
    uint32 data[34 * 256];

    uint32 *ptr = data;

    for (int32 i = 0; i < 256; i++)
    {
        int32 idx = i * 3;

        uint8 r = level->palette.colors[idx + 0] << 2;
        uint8 g = level->palette.colors[idx + 1] << 2;
        uint8 b = level->palette.colors[idx + 2] << 2;

        *ptr++ = (b | (g << 8) | (r << 16) | 0xFF000000);
    }

    for (int32 i = 0; i < 256; i++) {
        *ptr++ = 0;
    }

    for (int32 j = 0; j < 32; j++)
    {
        for (int32 i = 0; i < 256; i++)
        {
            int32 idx = level->lightmap[j * 256 + i] * 3;

            uint8 r = level->palette.colors[idx + 0] << 2;
            uint8 g = level->palette.colors[idx + 1] << 2;
            uint8 b = level->palette.colors[idx + 2] << 2;

            *ptr++ = (b | (g << 8) | (r << 16) | 0xFF000000);
        }
    }

    saveBitmap(fileName, (uint8*)data, 256, 32 + 2, 32);
}

int main(int argc, char** argv)
{
    if (argc < 3) {
        printf("usage: packer.exe [gba|3do] directory\n");
        return 0;
    }

    for (int32 i = 0; i < LVL_MAX; i++)
    {
        char fileName[64];

        { // load PC level
            sprintf(fileName, "TR1_PC/DATA/%s.PHD", levelNames[i]);

            FileStream f(fileName, false);
        
            if (f.isValid()) {
                pc[i] = new TR1_PC(f, LevelID(i));
                pc[i]->generateLODs();
                pc[i]->cutData();
            } else {
                printf("can't open \"%s\"", fileName);
            }
        }
    }

    dumpLightmap("lightmap.bmp", pc[LVL_TR1_1]);

    if (strcmp(argv[1], "gba") == 0)
    {
        out_GBA* out = new out_GBA();
        out->process(argv[2], pc, NULL);
        delete out;
    }

    if (strcmp(argv[1], "3do") == 0)
    {
        out_3DO* out = new out_3DO();
        out->process(argv[2], pc, NULL);
        delete out;
    }

    if (strcmp(argv[1], "32x") == 0)
    {
        out_32X* out = new out_32X();
        out->process(argv[2], pc, NULL);
        delete out;
    }

    for (int32 i = 0; i < LVL_MAX; i++)
    {
        delete pc[i];
    }

    return 0;
}
