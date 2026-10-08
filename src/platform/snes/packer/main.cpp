// OpenLara - SNES / Super FX target
// Level packer.
//
// Deliberately separate from src/platform/gba/packer/main.cpp rather than
// another branch inside it: that one pulls in out_GBA.h, which uses
// FindFirstFile, and there is no reason to drag Win32 into this build. The
// level reader (TR1_PC.h) and the shared helpers are reused as they are.
//
// Usage:
//   packer <TR1 data directory> <output directory> [level]
//
// e.g. packer ../../../../tomb_raider_dos/TOMBENG/data build/data LEVEL1

#include "../../gba/packer/common.h"
#include "../../gba/packer/TR1_PC.h"

#include "out_SNES.h"

#include <string.h>
#include <strings.h>

int main(int argc, char** argv)
{
    if (argc < 3 || argc > 5 || (argc == 5 && strcmp(argv[4], "--fx3"))) {
        printf("usage: packer <TR1 data dir> <output dir> [level] [--fx3]\n");
        return 1;
    }

    const char* dataDir = argv[1];
    const char* outDir  = argv[2];
    const char* want    = (argc > 3) ? argv[3] : "LEVEL1";
    const bool fx3 = argc == 5;

    // The DOS release ships lower case names; the packer's table is upper.
    int32 levelIndex = -1;
    for (int32 i = 0; i < LVL_MAX; i++)
    {
        if (strcasecmp(levelNames[i], want) == 0) {
            levelIndex = i;
            break;
        }
    }

    if (levelIndex < 0) {
        printf("unknown level \"%s\"\n", want);
        return 1;
    }

    char fileName[512];
    sprintf(fileName, "%s/%s.PHD", dataDir, levelNames[levelIndex]);

    FileStream f(fileName, false);
    if (!f.isValid())
    {
        // try the lower case spelling the DOS discs actually use
        char lower[64];
        strcpy(lower, levelNames[levelIndex]);
        for (char* c = lower; *c; c++) {
            if (*c >= 'A' && *c <= 'Z') *c += 32;
        }
        sprintf(fileName, "%s/%s.phd", dataDir, lower);
        FileStream f2(fileName, false);
        if (!f2.isValid()) {
            printf("can't open \"%s\"\n", fileName);
            return 1;
        }
        TR1_PC* level = new TR1_PC(f2, LevelID(levelIndex));
        out_SNES out;
        out.trDir = dataDir;
        out.process(outDir, level, fx3);
        delete level;
        return 0;
    }

    TR1_PC* level = new TR1_PC(f, LevelID(levelIndex));
    out_SNES out;
    out.trDir = dataDir;
    out.process(outDir, level, fx3);
    delete level;
    return 0;
}
