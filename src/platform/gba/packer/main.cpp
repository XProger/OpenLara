#include "common.h"
#include "IMA.h"
#include "TR1_PC.h"
#include "TR1_PSX.h"
#include "out_GBA.h"
#include "out_3DO.h"

TR1_PC* pc[LVL_MAX];
TR1_PSX* psx[LVL_MAX];

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

    for (int32 i = 0; i < LVL_MAX; i++)
    {
        delete pc[i];
    }

    return 0;
}
