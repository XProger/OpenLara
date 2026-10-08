/* OpenLara - SNES / Super FX target
 * The settings (FX3): upstream's gSettings and its options pages. See settings.c.
 */

#ifndef H_OL_SETTINGS
#define H_OL_SETTINGS

#include "fixed.h"

/* upstream's Settings, a byte a field */
typedef struct {
    uint8 vibration;    /* controls_vibration: no rumble on this pad */
    uint8 swap;         /* controls_swap: A and B trade */
    uint8 sfx;          /* audio_sfx */
    uint8 music;        /* audio_music: no music on this cartridge */
    uint8 gamma;        /* video_gamma, 0..16 */
    uint8 fps;          /* video_fps: drawFPS in the game's frames */
    uint8 vsync;        /* video_vsync: the GBA's; nothing here */
    uint8 pad;
} OlSettings;

extern OlSettings olSettings;
extern uint16 olCgram[256];             /* palSet's output, for main.asm's NMI */
extern unsigned int olCgramLeft;        /* bytes of it still to go up */
extern unsigned int olGameTextDirty;    /* ItemsExchange: olInvText to GS_TEXT */
extern unsigned int olGammaDirty;       /* a gamma for the render sync to compose */

void Settings_load(void);               /* gameInit: the defaults, osLoadSettings */
void Settings_level(void);              /* the load tick: CG-RAM's colours, the gamma */
void Settings_gamma(void);              /* palSet(palette, gamma << 4, 0) */
void Settings_text(void);               /* the render sync: drawFPS's text */
void Settings_option(void);             /* onDetail/onSound/onControls */
void Settings_visMask(void);            /* the passport's pages: updateVisMask */

#endif
