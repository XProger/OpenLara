/* OpenLara - SNES / Super FX target
 * The saved game (FX3): src/fixed/game.h's gameSave/gameLoad. See save.c.
 */

#ifndef H_OL_SAVE
#define H_OL_SAVE

extern unsigned int olSaveValid;    /* osCheckSave: a save of one of this cartridge's levels */
extern unsigned int olSaveLoad;     /* this boot is its Load Game: Save_apply at load */

#define BOOT_START      0           /* the level from its start */
#define BOOT_LOAD       1           /* and the save over it */
#define BOOT_TITLE      2           /* the title page, over the first level */

void Save_check(void);              /* gameInit, before the level */
void Save_boot(unsigned int level, unsigned int kind); /* the next boot (main.asm SelectLevel) */
int  Save_write(void);              /* gameSave: 1 once it reads back */
void Save_requestLoad(void);        /* Load Game, Current Position: before the restart */
void Save_apply(void);              /* gameLoad: the load tick, after olLevelInit */

#endif
