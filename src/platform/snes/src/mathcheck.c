/* Boot-time checks through 816-tcc's actual argument and return convention.
 * Kept in ROM bank $3C: gameInit calls it before the first GSU start, while
 * the CPU still owns ROM. Never call it from the concurrent gameplay loop.
 */
#include "fixed.h"

extern unsigned int ol_mathFail;

void olMathCheck(void)
{
    int bit = 0;
    ol_mathFail = 0;

    #define OL_CHECK(cond) { if (!(cond)) ol_mathFail |= (1u << bit); bit++; }

    OL_CHECK((int)phd_atan((int32)1000, (int32)0) == 0);
    OL_CHECK((int)(unsigned int)phd_atan((int32)0, (int32)1000) == ANGLE_90);
    OL_CHECK((int)(unsigned int)phd_atan((int32)(-1000), (int32)0)
             == (int)(unsigned int)ANGLE_180);
    OL_CHECK((int)(unsigned int)phd_atan((int32)0, (int32)(-1000))
             == (int)(unsigned int)(0 - ANGLE_90));
    OL_CHECK((int)(unsigned int)phd_atan((int32)1000, (int32)1000) == ANGLE_45);
    OL_CHECK((int)(unsigned int)phd_atan((int32)(-1000), (int32)1000)
             == ANGLE_90 + ANGLE_45);
    OL_CHECK((int)(unsigned int)phd_atan((int32)(-32767 - 1), (int32)0)
             == (int)(unsigned int)ANGLE_180);

    OL_CHECK((int)rand_logic() == 12);
    OL_CHECK((int)rand_logic() == 30469);
    OL_CHECK((int)rand_logic() == 16649);

    OL_CHECK(phd_sin((int32)0) == 0);
    OL_CHECK(phd_cos((int32)0) > FIXED_ONE - 4);
    OL_CHECK(phd_sin((int32)ANGLE_90) > FIXED_ONE - 4);
    OL_CHECK(phd_sin((int32)ANGLE_180) < 4 && phd_sin((int32)ANGLE_180) > -4);
    /* Exercise both words of the native integer return via compiled C.
       r0h is a pointer bank; 816-tcc reads a long long's high word in r1. */
    OL_CHECK(phd_sin((int32)(ANGLE_180 + ANGLE_90)) == -(int32)FIXED_ONE);
    OL_CHECK(phd_sqrt((uint32)0xFFFFFFFF) == (uint32)65535);

    #undef OL_CHECK
}
