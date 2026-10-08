/* OpenLara - SNES / Super FX target
 * The port of src/fixed/item.h's soundPlay and soundStop, and of the GBA's
 * sndPlaySample as far as this machine needs one.
 *
 * On the GBA the mixer is software and the samples are read out of ROM; here
 * the S-DSP mixes and the samples were uploaded to the sound CPU at boot, so
 * what sndPlaySample did -- find a channel, or the one already playing this
 * sample -- is the driver's (src/spc.asm), and this side ends at four port
 * writes. The rest is upstream's: the level's soundMap and SoundInfo, the
 * chance roll, the distance from the camera target, the random gain, pitch
 * and variant, on a PRNG of its own (rand_draw) so that none of it disturbs
 * the logic PRNG the creatures are tested against.
 *
 * The tables are the packer's SNDTAB.BIN (packer/sound_SNES.h): a byte per
 * id, and per info the first source, volume >> 7, chance >> 7 and a flags
 * byte -- mode in bits 0-1, count in 2-5, random pitch 6, random gain 7.
 */

#include "sound.h"
#include "camera.h"
#include "room.h"
#ifdef OL_FX3
#include "settings.h"
#endif

#define APU_PORT0   (*(volatile unsigned char *)0x002140LL)
#define APU_PORT1   (*(volatile unsigned char *)0x002141LL)
#define APU_PORT2   (*(volatile unsigned char *)0x002142LL)
#define APU_PORT3   (*(volatile unsigned char *)0x002143LL)

#define SND_MAX_DIST    8192

unsigned char olSndTable[SND_TABLE_BYTES];
unsigned int ol_sndReady;
unsigned int ol_sndCount;
unsigned int ol_sndLast;

/* The command byte last written: bits 7-2 advance every command so the
   driver sees each one change; zero is what both sides start on. */
unsigned int olSndSeq;
unsigned int olDrawSeed;
/* sound.asm's scratch */
unsigned int olSndWork[12];

extern Location camTarget;

/* The cartridge runs sound.asm; this body is the C reference the C_MOTION
   oracle build executes. It came out at 2,344 bytes, which the GSU-2's
   resident image does not have. */
#if defined(OL_C_MOTION) || defined(OL_HOST_BUILD)

#define sndSeq olSndSeq

/* Upstream's draw PRNG: the same generator as rand_logic, a seed apart.
   (sound.asm uses a 16-bit xorshift instead; nothing observable follows
   the sequence.) */
int32 rand_draw(void)
{
    uint32 seed = ((uint32)olSndWork[11] << 16) | olDrawSeed;
    seed = (uint32)(0x3039 + seed * (uint32)0x41C64E6D);
    olDrawSeed = (unsigned int)seed;
    olSndWork[11] = (unsigned int)(seed >> 16);
    return (int32)((seed >> 10) & 0x7FFF);
}

void Sound_init(void)
{
    sndSeq = 0;
    olDrawSeed = 0;
    olSndWork[11] = 0;
    ol_sndCount = 0;
    ol_sndLast = 0;
}

/* One command: source, volume, pitch, then the command byte. The driver
   echoes the byte when it has acted; the wait for the previous echo is
   bounded so a dead sound CPU costs a few hundred cycles, not the tick. */
static void Sound_send(unsigned int source, unsigned int volume,
                       unsigned int pitch, unsigned int mode)
{
    unsigned int n = 250;
    while (APU_PORT0 != sndSeq && --n) ;
    APU_PORT1 = (unsigned char)source;
    APU_PORT2 = (unsigned char)volume;
    APU_PORT3 = (unsigned char)pitch;
    sndSeq = (sndSeq + 4) & 0xFC;
    if (sndSeq == 0) sndSeq = 4;
    sndSeq |= mode;
    APU_PORT0 = (unsigned char)sndSeq;
    ol_sndCount++;
}

/* void* soundPlay(int16 id, const vec3i* pos) -- with a position always:
   nothing here plays a sound at the camera. Returns 1 when a command went
   to the driver. */
int Sound_play(unsigned int id, int x, int y, int z)
{
    const unsigned char* rec;
    unsigned int a, flags, count, volume, cut, source, pitch, mode;
    int dx, dy, dz;

    if (!ol_sndReady || id >= 256) return 0;
#ifdef OL_FX3
    if (!olSettings.sfx) return 0;      /* settings.c: audio_sfx */
#endif
    a = olSndTable[id];
    if (a == 255) return 0;
    rec = olSndTable + 256 + (a << 2);

    /* chance, stored >> 7 against a 15-bit roll */
    if (rec[2] && rec[2] < (unsigned int)(rand_draw() >> 7)) return 0;

    volume = (unsigned int)rec[1] << 7;

    dx = x - camTarget.x;
    dy = y - camTarget.y;
    dz = z - camTarget.z;
    if (dx >= SND_MAX_DIST || dx <= -SND_MAX_DIST ||
        dy >= SND_MAX_DIST || dy <= -SND_MAX_DIST ||
        dz >= SND_MAX_DIST || dz <= -SND_MAX_DIST) return 0;
    /* volume -= phd_sqrt(dot(d, d)) << 2, which cannot go below zero here:
       the subtraction is unsigned and an inaudible sound leaves early. */
    cut = (unsigned int)phd_sqrt((uint32)((int32)dx * dx) +
                                 (uint32)((int32)dy * dy) +
                                 (uint32)((int32)dz * dz)) << 2;
    if (cut >= volume) return 0;
    volume -= cut;

    flags = rec[3];
    if (flags & 0x80) {
        cut = (unsigned int)(rand_draw() >> 2);
        if (cut >= volume) return 0;
        volume -= cut;
    }

    volume >>= 9;               /* 0..63 */
    if (volume == 0) return 0;
    volume += 1;                /* 1..64: 64 is a full-volume sample */

    pitch = 128;
    if (flags & 0x40) {
        /* upstream: ((rand * 13) >> 14) - 13, in sixteen bits */
        pitch += (((unsigned int)rand_draw() >> 3) * 13 >> 11);
        pitch -= 13;
    }

    source = rec[0];
    count = (flags >> 2) & 15;
    if (count > 1) {
        source += ((unsigned int)rand_draw() >> 3) * count >> 12;
    }

    /* UNIQUE and REPLAY are the driver's modes 0 and 1; LOOP and the
       fourth value take a new voice, as sndPlaySample would. */
    mode = flags & 3;
    if (mode > 2) mode = 2;

    /* the driver's VOL is a signed byte: 64 -> 127 */
    volume += volume;
    if (volume > 127) volume = 127;

    ol_sndLast = (id << 8) | source;
    Sound_send(source, volume, pitch, mode);
    return 1;
}

void Sound_playItem(unsigned int id, ItemObj* item)
{
    Sound_play(id, item->x, item->y, item->z);
}

/* void soundStop(int16 id): every variant of the sound, keyed off. */
void Sound_stop(unsigned int id)
{
    const unsigned char* rec;
    unsigned int a, count, source;

    if (!ol_sndReady || id >= 256) return;
    a = olSndTable[id];
    if (a == 255) return;
    rec = olSndTable + 256 + (a << 2);
    source = rec[0];
    count = (rec[3] >> 2) & 15;
    if (count == 0) count = 1;
    while (count--) {
        Sound_send(source++, 0, 0, 3);
    }
}

#endif /* OL_C_MOTION || OL_HOST_BUILD */
