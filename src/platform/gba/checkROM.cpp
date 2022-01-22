#include <tonc.h>

#define MEM_CHECK_MAGIC 14021968
#define MEM_CHECK_SIZE  16

const int ROM_MAGIC[MEM_CHECK_SIZE] = {
    MEM_CHECK_MAGIC + 0,
    MEM_CHECK_MAGIC + 1,
    MEM_CHECK_MAGIC + 2,
    MEM_CHECK_MAGIC + 3,
    MEM_CHECK_MAGIC + 4,
    MEM_CHECK_MAGIC + 5,
    MEM_CHECK_MAGIC + 6,
    MEM_CHECK_MAGIC + 7,
    MEM_CHECK_MAGIC + 8,
    MEM_CHECK_MAGIC + 9,
    MEM_CHECK_MAGIC + 10,
    MEM_CHECK_MAGIC + 11,
    MEM_CHECK_MAGIC + 12,
    MEM_CHECK_MAGIC + 13,
    MEM_CHECK_MAGIC + 14,
    MEM_CHECK_MAGIC + 15,
};

EWRAM_CODE bool checkROM(unsigned int mask)
{
    REG_WSCNT = mask;

    // check sequential read (S)
    for (int i = 0; i < MEM_CHECK_SIZE; i++)
    {
        if (*(volatile int*)&ROM_MAGIC[i] != (MEM_CHECK_MAGIC + i))
        {
            REG_WSCNT = WS_ROM0_N4 | WS_ROM0_S2 | WS_PREFETCH;
            return false;
        }
    }

    // check non-sequential read (N)
    for (int i = 0, j = MEM_CHECK_SIZE - 1; i < MEM_CHECK_SIZE; i++, j--)
    {
        bool L = *(volatile int*)&ROM_MAGIC[i] == (MEM_CHECK_MAGIC + i);
        bool R = *(volatile int*)&ROM_MAGIC[j] == (MEM_CHECK_MAGIC + j);

        if (L && R) continue;

        REG_WSCNT = WS_ROM0_N4 | WS_ROM0_S2 | WS_PREFETCH;
        return false;
    }

    return true;
}