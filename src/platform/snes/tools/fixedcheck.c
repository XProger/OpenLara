/* OpenLara - SNES / Super FX target
 * Differential test for the ported fixed-point code.
 *
 * The renderer has two oracles that must agree; the engine port needs one
 * too, or 529 KB of C++ becomes 529 KB of transcription nobody can check.
 * This is it: src/math.c's C reference is compiled with OL_HOST_BUILD and
 * run against libm. TypeSNES separately exercises the cartridge's native
 * math.asm helpers through the 816-tcc calling convention.
 *
 * That is deliberately an independent model rather than a copy of upstream's
 * algorithm. A transcription bug reproduced faithfully in the test is not a
 * test; a wrong angle is wrong against atan2 whatever it was copied from.
 *
 * Tolerances are the table resolutions, and they are asserted rather than
 * observed: if a table is made smaller the test says so instead of drifting.
 *
 *   cc -DOL_HOST_BUILD -I<build> -I<src> fixedcheck.c ../src/math.c -lm
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "fixed.h"
#include "common.h"

#define TWO_PI          6.283185307179586

/* One sin entry spans 64 angle units, and phd_sin rounds to the nearest, so
   the error is half a step of the curve: sin' is at most 1, and half a step
   is (2*pi/1024)/2 radians, which is 50 of 16384. */
#define SIN_TOLERANCE   52

/* atan is sampled 128 times over a unit ratio and its derivative is at most
   1, so half a step is 1/256 of a radian: 256 angle units, and the octant
   reduction does not add to it. */
#define ATAN_TOLERANCE  260

static int failures;

static void fail(const char* what, double a, double b, long input)
{
    if (failures < 12) {
        printf("  FAIL %-10s input %-12ld got %.0f want %.0f\n",
               what, input, a, b);
    }
    failures++;
}

/* The shortest way round the circle, since 65535 and 0 are neighbours. */
static int angleDelta(int a, int b)
{
    int d = (a - b) & 0xFFFF;
    if (d > 0x8000) d -= 0x10000;
    return d < 0 ? -d : d;
}

static void checkSin(void)
{
    int worst = 0;
    long a;

    for (a = 0; a < 0x10000; a++)
    {
        double want = sin(TWO_PI * a / 65536.0) * FIXED_ONE;
        int got = (int)phd_sin((int32)a);
        int err = (int)fabs(got - want);
        if (err > worst) worst = err;
        if (err > SIN_TOLERANCE) fail("sin", got, want, a);

        want = cos(TWO_PI * a / 65536.0) * FIXED_ONE;
        got = (int)phd_cos((int32)a);
        err = (int)fabs(got - want);
        if (err > worst) worst = err;
        if (err > SIN_TOLERANCE) fail("cos", got, want, a);
    }

    /* Negative and past-a-turn angles have to wrap, not index off the end. */
    for (a = -0x20000; a < 0x20000; a += 97)
    {
        int got = (int)phd_sin((int32)a);
        double want = sin(TWO_PI * (double)(a & 0xFFFF) / 65536.0) * FIXED_ONE;
        if (fabs(got - want) > SIN_TOLERANCE) fail("sin wrap", got, want, a);
    }

    printf("  sin/cos   worst error %d of %d tolerance\n", worst, SIN_TOLERANCE);
}

static void checkAtan(void)
{
    int worst = 0;
    long i;
    /* A spread of magnitudes rather than a grid: the octants and the y > x
       swap are where transcription goes wrong, and both need every sign. */
    static const int mags[] = { 1, 2, 3, 7, 15, 100, 511, 1024, 5000, 32000 };
    const int n = (int)(sizeof(mags) / sizeof(mags[0]));
    int xi, yi, sx, sy;

    if (phd_atan(0, 0) != 0) fail("atan origin", (double)phd_atan(0, 0), 0, 0);

    for (xi = 0; xi < n; xi++)
    for (yi = 0; yi < n; yi++)
    for (sx = -1; sx <= 1; sx += 2)
    for (sy = -1; sy <= 1; sy += 2)
    {
        int x = mags[xi] * sx;
        int y = mags[yi] * sy;
        double want = atan2((double)y, (double)x) / TWO_PI * 65536.0;
        int got = (int)phd_atan((int32)x, (int32)y);
        int err = angleDelta(got, (int)lround(want));
        if (err > worst) worst = err;
        if (err > ATAN_TOLERANCE) fail("atan", got, want, x * 100000L + y);
    }

    /* And the axes, which are the octant boundaries. */
    for (i = 1; i < 30000; i += 313)
    {
        struct { int x, y; double want; } cases[] = {
            { (int)i,  0,      0.0 },
            { 0,  (int)i,  16384.0 },
            { -(int)i, 0,  32768.0 },
            { 0, -(int)i, -16384.0 },
        };
        int k;
        for (k = 0; k < 4; k++) {
            int got = (int)phd_atan((int32)cases[k].x, (int32)cases[k].y);
            int err = angleDelta(got, (int)lround(cases[k].want));
            if (err > worst) worst = err;
            if (err > ATAN_TOLERANCE) fail("atan axis", got, cases[k].want, i);
        }
    }

    printf("  atan      worst error %d of %d tolerance\n", worst, ATAN_TOLERANCE);
}

static void checkSqrt(void)
{
    unsigned long v;
    int worst = 0;

    for (v = 0; v < 70000; v++)
    {
        unsigned want = (unsigned)sqrt((double)v);
        unsigned got = (unsigned)phd_sqrt((uint32)v);
        if (got != want) { fail("sqrt", got, want, (long)v); }
    }

    /* And the top of the range, where the algorithm's leading bit lives. */
    for (v = 0; v < 4000; v++)
    {
        unsigned long x = 0xFFFFFFFFul - v * 1013ul;
        unsigned want = (unsigned)sqrt((double)x);
        unsigned got = (unsigned)phd_sqrt((uint32)x);
        int err = (int)abs((int)(got - want));
        if (err > worst) worst = err;
        if (err > 1) fail("sqrt big", got, want, (long)x);
    }

    printf("  sqrt      exact below 70000, worst error %d near 2^32\n", worst);
}

/* The box helpers are pure functions, which makes them the easiest thing in
   the port to get right and the easiest to get subtly wrong by writing
   something that resembles them. Checked against the identities they have to
   satisfy rather than against a second copy of themselves: a box rotated four
   quadrants is where it started, a box pushed out of another no longer
   overlaps it, and a translated box contains a translated point. */
static void checkBoxes(void)
{
    AABBi a;
    int q, i;
    int bad = 0;

    for (i = 0; i < 4; i++)
    {
        AABBi b;
        b.minX = -500 - i;  b.maxX = 300 + i;
        b.minY = -1023;     b.maxY = 0;
        b.minZ = -561;      b.maxZ = -461 + i;
        a = b;

        /* Four quarter turns is the identity, whatever the convention. */
        for (q = 0; q < 4; q++) {
            /* quadrant 2 is upstream's no-op, so a full turn is 1,1,1,1 read
               as "three rotations and a no-op" -- rotate by 1 four times. */
            boxRotateYQ(&a, 1);
        }
        if (a.minX != b.minX || a.maxX != b.maxX ||
            a.minZ != b.minZ || a.maxZ != b.maxZ) {
            fail("box rotate", a.minX, b.minX, i);
            bad++;
        }

        /* Quadrant 2 changes nothing at all. */
        a = b;
        boxRotateYQ(&a, 2);
        if (a.minX != b.minX || a.maxZ != b.maxZ) {
            fail("box quad 2", a.minX, b.minX, i);
            bad++;
        }
    }

    /* Pushing one box out of another has to end the overlap. */
    for (i = -400; i <= 400; i += 37)
    {
        AABBi wall, her;
        vec3i push;
        int ax, az;

        wall.minX = -200; wall.maxX = 200;
        wall.minY = -100; wall.maxY = 100;
        wall.minZ = -50;  wall.maxZ = 50;

        her.minX = i - 128; her.maxX = i + 128;
        her.minY = -100;    her.maxY = 100;
        her.minZ = -128;    her.maxZ = 128;

        if (!boxIntersect(&her, &wall)) continue;

        boxPushOut(&wall, &her, &push);
        ax = abs(push.x);
        az = abs(push.z);
        if (ax < az) boxTranslate(&her, push.x, 0, 0);
        else         boxTranslate(&her, 0, 0, push.z);

        if (boxIntersect(&her, &wall)) {
            fail("box push", i, 0, i);
            bad++;
        }
    }

    /* And a point inside a box stays inside when both move together. */
    {
        AABBi box;
        vec3i p;
        box.minX = -10; box.maxX = 10;
        box.minY = -10; box.maxY = 10;
        box.minZ = -10; box.maxZ = 10;
        p.x = 5; p.y = -5; p.z = 0;

        if (!boxContains(&box, &p)) { fail("box contains", 0, 1, 0); bad++; }
        boxTranslate(&box, 1000, -2000, 3000);
        p.x += 1000; p.y -= 2000; p.z += 3000;
        if (!boxContains(&box, &p)) { fail("box translate", 0, 1, 0); bad++; }
    }

    printf("  boxes     rotate, push and translate hold their identities\n");
}

int main(void)
{
    printf("fixedcheck: src/math.c against libm\n");
    checkSin();
    checkAtan();
    checkSqrt();
    checkBoxes();

    if (failures) {
        printf("  %d failure(s)\n", failures);
        return 1;
    }
    printf("  all clear\n");
    return 0;
}
