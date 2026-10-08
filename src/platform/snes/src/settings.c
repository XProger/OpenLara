/* OpenLara - SNES / Super FX target
 * The settings (FX3): upstream's gSettings.
 *
 * gameInit's defaults -- rumble on, A/B as they are, sound and music on,
 * gamma 0, the frame counter shown, no vsync -- then osLoadSettings over
 * them, from the battery RAM after the boot request ($70:3110: "OLST", a
 * version, the seven fields), and osSaveSettings back there whenever an
 * options page changes one (Settings_option, upstream's onDetail, onSound
 * and onControls).
 *
 * What they do here. SFX: Sound_play sends nothing (sound.asm), as the
 * GBA's sndPlaySample returns. Swap A B: game.c trades the two buttons
 * before anything reads them. Gamma: upstream's palGamma over the whole
 * palette, which on this cartridge is CG-RAM -- the level's 128 colours,
 * the HUD's, the font's, greys, borders, the options' bar and a title
 * boot's picture -- composed here from the same ROM the boot uploaded and
 * sent up by main.asm's NMI 64 bytes a field. Show FPS: drawFPS, the
 * frame counter at (2, 16) in TR's font, through the GSU's TextRender in
 * the game's frames (mesh.gsu FrameBody; the text is olInvText's while the
 * ring is shut). Music, rumble and vsync are kept and do nothing: there is
 * no music, no rumble, and the frame is not the GBA's.
 */

#include "settings.h"
#include "inventory.h"
#include "level.h"
#include "save.h"
#include "olhost.h"

#define SETTINGS_BYTES      ((uint8 *)0x703110LL)   /* after the boot request */
#define SETTINGS_VERSION    1
#define OL_FPS              (*(uint16 *)0x7E0000LL) /* main.asm's direct page: fps */

OlSettings olSettings;
uint16 olCgram[256];
uint16 olCgramBase[256];                /* CG-RAM as the boot left it, read at the load */
uint16 olGammaR[32], olGammaG[32], olGammaB[32];   /* palGamma's levels, in place */

void olGammaApply(void);                /* save_fx3.asm: olCgram from olCgramBase */
unsigned int olCgramLeft;
unsigned int olGameTextDirty;
static unsigned int textKey;            /* olInvText's game text: 0 none, fps | $8000 */
unsigned int olGammaDirty;              /* Settings_text composes it, off the tick */

void Inventory_fpsText(unsigned int key);   /* inventory_text.c */

void Settings_load(void)
{
    uint8* s = SETTINGS_BYTES;
    olSettings.vibration = 1;
    olSettings.swap = 0;
    olSettings.sfx = 1;
    olSettings.music = 1;
    olSettings.gamma = 0;
    olSettings.fps = 1;
    olSettings.vsync = 0;
    if (s[0] != 'O' || s[1] != 'L') return;
    if (s[2] != 'S' || s[3] != 'T') return;
    if (s[4] != SETTINGS_VERSION) return;
    olSettings.vibration = s[5] & 1;
    olSettings.swap = s[6] & 1;
    olSettings.sfx = s[7] & 1;
    olSettings.music = s[8] & 1;
    olSettings.gamma = s[9] > 16 ? 16 : s[9];
    olSettings.fps = s[10] & 1;
    olSettings.vsync = s[11] & 1;
}

/* osSaveSettings: not valid while it is written */
static void save(void)
{
    uint8* s = SETTINGS_BYTES;
    s[0] = 0;
    s[4] = SETTINGS_VERSION;
    s[5] = olSettings.vibration;
    s[6] = olSettings.swap;
    s[7] = olSettings.sfx;
    s[8] = olSettings.music;
    s[9] = olSettings.gamma;
    s[10] = olSettings.fps;
    s[11] = olSettings.vsync;
    s[3] = 'T';
    s[2] = 'S';
    s[1] = 'L';
    s[0] = 'O';
}

static const unsigned char* farAt(uint16 at, uint16 bank)
{
    union { const unsigned char* p; uint16 w[2]; } u;
    u.w[0] = at;
    u.w[1] = bank;
    return u.p;
}

static void baseRun(unsigned int first, const unsigned char* p, unsigned int count)
{
    unsigned int i;
    for (i = 0; i < count; i++) olCgramBase[first + i] = olData16(p, i << 1);
}

/* The load tick's: CG-RAM as UploadPalette and InitHud left it, from the
   same ROM -- the far reads are a field's worth in 816-tcc, so once, where
   a field more does not matter -- and the gamma over it if there is one
   (level.h's palSet at the load). */
void Settings_level(void)
{
    unsigned int i;
    for (i = 160; i < 256; i++) olCgramBase[i] = 0;
    baseRun(0, farAt(olLevel.paletteAt, olLevel.paletteBank), 128);
    baseRun(128, farAt(olLevel.hudAt, olLevel.hudBank), 32);
    if (olLevel.fontSize) {
        const unsigned char* f = farAt(olLevel.fontAt, olLevel.fontBank);
        baseRun(160, f, 28);                            /* font, greys, borders */
        baseRun(188, f + 64 + f[57] * 6, 4);            /* the options' bar */
    }
#if OL_TITLE_SIZE
    if (olBootKind == BOOT_TITLE)
        baseRun(192, (const unsigned char *)OL_TITLE_ADDR, 64);
#endif
    if (olSettings.gamma) Settings_gamma();
}

/* palSet(palette, gamma << 4, 0): upstream's palGamma over what the boot
   put in CG-RAM (save_fx3.asm olGammaApply: in 816-tcc it took two fields
   and held the tick back) */
void Settings_gamma(void)
{
    olGammaApply();
    olCgramLeft = 512;
}

/* The render sync's (main.asm, between ticks): a gamma the options page
   changed -- composing 256 colours is a third of a field, which held a
   tick back long enough to lose a key's release -- and drawFPS, while the
   ring is shut: the counter's text in olInvText, for ItemsExchange to hand
   the GSU when it changes. */
void Settings_text(void)
{
    unsigned int key = 0;
    if (olGammaDirty) {
        Settings_gamma();
        olGammaDirty = 0;
    }
    if (olInvState != INV_STATE_NONE) {
        textKey = 0xFFFF;               /* the ring's text is there now */
        return;
    }
    if (olSettings.fps) key = OL_FPS | 0x8000;
    if (key == textKey) return;
    textKey = key;
    Inventory_fpsText(key);
    olGameTextDirty = 1;
}

/* onDetail, onSound, onControls: the chosen option's value, if it moved */
void Settings_option(void)
{
    unsigned int v = olInvOptValue[olInvOptionIndex];
    unsigned int was;
    switch (olInvOptStr[olInvOptionIndex])
    {
        case OSTR_GAMMA:
            v >>= 4;
            if (v == olSettings.gamma) return;
            olSettings.gamma = (uint8)v;
            olGammaDirty = 1;
            break;
        case OSTR_FPS:
            was = olSettings.fps; olSettings.fps = (uint8)v;
            if (was == v) return;
            break;
        case OSTR_VSYNC:
            was = olSettings.vsync; olSettings.vsync = (uint8)v;
            if (was == v) return;
            break;
        case OSTR_SFX:
            was = olSettings.sfx; olSettings.sfx = (uint8)v;
            if (was == v) return;
            break;
        case OSTR_MUSIC:
            was = olSettings.music; olSettings.music = (uint8)v;
            if (was == v) return;
            break;
        case OSTR_RUMBLE:
            was = olSettings.vibration; olSettings.vibration = (uint8)v;
            if (was == v) return;
            break;
        case OSTR_SWAP:
            was = olSettings.swap; olSettings.swap = (uint8)v;
            if (was == v) return;
            break;
        default:
            return;
    }
    save();
}

/* The passport, the options page's first item: upstream's updateVisMask,
   which of its nodes are drawn as its pages turn -- covers and spine, the
   spread it is on and the page turning (frames 14, 19 and 24 are its three
   spreads). Every ring item is drawn under olInvVisMask, as upstream's
   drawSlot draws under itemVisMask, and every choosing and putting back
   resets it to all (inventory.c); inventory_draw.c publishes it. Here
   rather than in inventory.c for the C_MOTION oracle's full $BB. */
uint16 olInvVisMask;

/* the pages' bits per frame, 0..29; 29 and on, the covers and spine alone */
static const uint8 passportPages[30] = {
    0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,   /* <= 14: 2, 6 */
    0x44, 0x44, 0x44, 0x44, 0x44, 0x4C, 0x4C, 0x4C, 0x4C, 0x48,   /* 15-18: 2, 3, 6; 19: 3, 6 */
    0x68, 0x68, 0x68, 0x68, 0x28, 0x28, 0x28, 0x28, 0x28, 0x00    /* 20-23: 3, 5, 6; 24-28: 3, 5 */
};

void Settings_visMask(void)
{
    unsigned int f = (unsigned int)olInvFrameIndex;
    if (olInvItemsList[olInvItemIndex] != SLOT_PASSPORT) return;
    if (f > 29) f = 29;
    olInvVisMask = (uint16)(0x13 | passportPages[f]);   /* 0, 1, 4 always */
}
