/* OpenLara - SNES / Super FX target
 * Sound effects: upstream's soundPlay/soundStop, and the four-port protocol
 * to the SPC700 driver in src/spc.asm.
 */

#ifndef H_OL_SOUND
#define H_OL_SOUND

#include "fixed.h"
#include "item.h"

/* TR1's sound ids, the ones the game calls by number (the rest arrive
   through SOUND animation commands). */
#define SND_NO              2
#define SND_LANDING         4
#define SND_DRAW            6
#define SND_HOLSTER         7
#define SND_PISTOLS_SHOT    8
#define SND_SHOTGUN_RELOAD  9
#define SND_UZIS_SHOT       43
#define SND_MAGNUMS_SHOT    44
#define SND_SHOTGUN_SHOT    45
#define SND_HIT_BEAR        16
#define SND_HIT_WOLF        20
#define SND_HIT_LION        85
#define SND_HIT             27
#define SND_SCREAM          30

/* SNDTAB.BIN, copied into WRAM by main.asm at boot: soundMap[256] (info
   slot, 255 for none) and up to 96 four-byte infos. */
#define SND_INFO_MAX        96
#define SND_TABLE_BYTES     (256 + SND_INFO_MAX * 4)
extern unsigned char olSndTable[SND_TABLE_BYTES];

/* 1 once the boot upload's driver answered; 0 plays silently. */
extern unsigned int ol_sndReady;
/* Commands sent, and the last one: (id << 8) | source. The runner's. */
extern unsigned int ol_sndCount;
extern unsigned int ol_sndLast;

int  Sound_play(unsigned int id, int x, int y, int z);
void Sound_playItem(unsigned int id, ItemObj* item);
void Sound_stop(unsigned int id);
int32 rand_draw(void);

#endif
