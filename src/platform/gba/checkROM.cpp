#include <tonc.h>

#define MEM_CHECK_MAGIC 14021968

const int ROM_MAGIC[] = {
    MEM_CHECK_MAGIC,
    MEM_CHECK_MAGIC + 1,
    MEM_CHECK_MAGIC + 2,
    MEM_CHECK_MAGIC + 3,
    MEM_CHECK_MAGIC + 4,
    MEM_CHECK_MAGIC + 5,
    MEM_CHECK_MAGIC + 6,
    MEM_CHECK_MAGIC + 7
};

EWRAM_CODE bool checkROM(unsigned int mask)
{
    REG_WSCNT = mask;

    for (int i = 0; i < 8; i++)
    {
        if (*(volatile int*)&ROM_MAGIC[i] != (MEM_CHECK_MAGIC + i))
        {
            REG_WSCNT = WS_ROM0_N4 | WS_ROM0_S2 | WS_PREFETCH;
            return false;
        }
    }
    
    for (int i = 7; i >= 0; i--)
    {
        if (*(volatile int*)&ROM_MAGIC[i] != (MEM_CHECK_MAGIC + i))
        {
            REG_WSCNT = WS_ROM0_N4 | WS_ROM0_S2 | WS_PREFETCH;
            return false;
        }
    }

    return true;
}