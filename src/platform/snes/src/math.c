/* OpenLara - SNES / Super FX target
 * The fixed-point floor everything else in the port stands on.
 *
 * Ported from src/fixed/common.cpp. The algorithms are upstream's; the table
 * sizes are not, and the difference is the point -- see tools/mktables.py.
 *
 * The cartridge uses math.asm for trig and square root, with these same
 * tables. The C versions remain the host reference checked against libm;
 * the emulator checks the assembly's results and 816-tcc calling convention.
 */

#include "fixed.h"
#include "oltables.h"

#ifdef OL_HOST_BUILD
/* src/fixed indexes a 4096-entry table directly. This one is a quarter of
   that, so the angle shifts further, and the cast to uint16 is what makes a
   negative or past-a-turn angle wrap instead of running off the end.

   Half a step is added before the shift: truncating the index costs a whole
   step of error where rounding costs half, and the whole step is what the
   tolerance in tools/fixedcheck would otherwise have to be. */
#define OL_SIN_ROUND    (1 << (OL_SIN_SHIFT - 1))

int32 phd_sin(int32 angle)
{
    return olSinTable[((uint16)(angle + OL_SIN_ROUND) >> OL_SIN_SHIFT)
                      & (OL_SIN_ENTRIES - 1)];
}

int32 phd_cos(int32 angle)
{
    return olSinTable[((uint16)(angle + ANGLE_90 + OL_SIN_ROUND) >> OL_SIN_SHIFT)
                      & (OL_SIN_ENTRIES - 1)];
}

/* The angle of a vector.
 *
 * Upstream folds the octant with a table of eight 32-bit offsets and an abs()
 * at the end. That does not survive this compiler: int32 is `long long` here,
 * the table's entries need the full range, and the result came back as zero
 * for every vector pointing along -X -- which is to say the port's own
 * differential oracle could not see it, because tools/fixedcheck builds this
 * same file with a real 32-bit int and gets the right answer.
 *
 * So the reduction is written in explicit sixteen-bit angle arithmetic
 * instead, which is the same identity a different way round:
 *
 *   a = atan(y/x) for the eighth of the circle where 0 <= y <= x
 *   swapped   a = 90 - a
 *   y < 0     a = -a
 *   x < 0     a = 180 - a
 *
 * Sixteen bits is all a TR angle is, and world deltas fit it, so nothing is
 * lost by narrowing the arguments at the door. See the note in fixed.h.
 */
int32 phd_atan(int32 x, int32 y)
{
    int xi = (int)x;
    int yi = (int)y;
    uint16 ax, ay;
    int a;
    int negY = 0, negX = 0, swapped = 0;

    if (xi == 0 && yi == 0)
        return 0;

    /* Do the magnitudes unsigned. On the cartridge `int` is sixteen bits,
       so -(-32768) is still -32768 and used to turn the table index into an
       out-of-bounds read for a perfectly valid world delta. */
    if (xi < 0) {
        negX = 1;
        ax = (uint16)(0u - (uint16)xi);
    } else {
        ax = (uint16)xi;
    }
    if (yi < 0) {
        negY = 1;
        ay = (uint16)(0u - (uint16)yi);
    } else {
        ay = (uint16)yi;
    }

    if (ay > ax) {
        uint16 t = ax;
        swapped = 1;
        ax = ay;
        ay = t;
    }

    /* ay <= ax now, so the ratio is in [0, 1] and the index in range. The
       multiply is 16-bit and ay can be 32000, so it is done as a long
       divide -- the one place this function needs more than sixteen bits. */
    a = (int)olAtanTable[(int)(((int32)ay * OL_ATAN_ENTRIES) / (int32)ax)];

    if (swapped) a = ANGLE_90 - a;
    if (negY)    a = -a;
    if (negX)    a = ANGLE_180 - a;

    return (int32)(unsigned int)a;
}

#endif

/* Upstream's logic PRNG, seed and constants unchanged. The sequence matters
   as much as the distribution: everything that consults it -- which creature
   flinches, which of three death animations plays -- has to be the same on
   every run or nothing about a creature can be tested. It is a 32-bit
   multiply. FX3 uses constant products in FastROM from math.asm; C_MOTION keeps this
   complete C implementation as an executable sequence oracle. */
#ifdef OL_FX3
uint32 olRandSeed;
#else
static uint32 olRandSeed;
#endif

/* WRAM comes up undefined and there is no crt0 to zero .bss: the seed is
   set here, at gameInit, so the sequence is the same on hardware as in an
   emulator that zero-fills. */
void rand_reset(void)
{
    olRandSeed = 0;
}

#if !defined(OL_FX3) || defined(OL_C_MOTION)
int32 rand_logic(void)
{
    olRandSeed = (uint32)(0x3039 + olRandSeed * (uint32)0x41C64E6D);
    return (int32)((olRandSeed >> 10) & 0x7FFF);
}
#endif

/* Upstream's integer square root, unchanged: sixteen rounds of shift and
   compare, no table and no divide. */
#ifdef OL_HOST_BUILD
uint32 phd_sqrt(uint32 x)
{
    uint32 m = 0x40000000;
    uint32 y = 0;
    uint32 z = 0;

    do {
        y += m;

        if (y > x) {
            y = z;
        } else {
            x -= y;
            y = z + m;
        }

        z = y >> 1;
        m >>= 2;
    } while (m);

    return y;
}
#endif
