/* OpenLara - SNES / Super FX target
 * The saved game (FX3): src/fixed/game.h's gameSave and gameLoad, and the
 * GBA's osSaveGame/osLoadGame/osCheckSave, on the cartridge's battery RAM.
 *
 * Where. An FX3 board declares its battery with cartridge type $18 (the FX3
 * specification, section 2), and a Super FX battery keeps the game pak RAM
 * -- the GSU's own working memory -- whole. FX3 draws its frame in bank $71,
 * which left the GSU-2 frame buffers' $70:1000-$31FF to nothing: the save is
 * there, $1000-$30FF, 8,448 bytes, of which a level of 128 entities would
 * take 4.8 KB, and the next boot's request above it (Save_boot, main.asm's
 * SelectLevel). The runner holds that nothing else writes them.
 *
 * What. Upstream saves each item's place, angles, animation, flags, timer and
 * states (ItemObj::save), Lara's extras and the creatures' (health, speeds,
 * mood), the inventory's counts and both random seeds -- nothing of the
 * rooms, except that a block, which is a floor, gives its old sector back
 * and takes its new one when it is loaded (Block::load). Here an item is its
 * whole 32-byte record, a superset of ItemSave holding health, speeds and
 * mood too, then the per-item arrays beside it; Lara's extraL is the
 * globals lara.c, weapon.c, weapon_anim.c and contact_fx3.c keep for it;
 * then the counts, secrets, the level's end, the seeds. The fixed cameras'
 * ONCE bits, which upstream keeps in level data and so loses, are kept.
 * Navigation is not: upstream makes a creature's extra again in
 * activate(), and a creature here acquires a Nav context again on its next
 * update.
 *
 * Load. Load Game's Current Position asks the next boot for the save's
 * level and a load (Save_boot) and restarts the cartridge, as Load Game of
 * a level does (startLevel); SelectLevel picks the level and the kind of
 * boot up, Save_check sees it, and the load tick, once olLevelInit has made
 * the level as a boot leaves it, puts the saved state over it (Save_apply)
 * before the camera snaps to her.
 *
 * The header, sixteen bytes: "OLSV", the version, the ROM's own checksum
 * (another build of the cartridge may number its entities differently), the
 * entity count, the payload's length and Fletcher sum (save_fx3.asm), and the
 * level it is a save of -- the cartridge may carry several.
 */

#include "save.h"
#include "item.h"
#include "lara.h"
#include "weapon.h"
#include "enemy.h"
#include "object.h"
#include "room.h"
#include "camera.h"
#include "level.h"
#include "inventory.h"
#include "nav_fx3.h"

#define SAVE_SRAM       0x1000          /* $70:1000 */
#define SAVE_SPACE      0x2100          /* to $70:30FF; the boot request is next */
#define SAVE_HEADER     16
#define SAVE_VERSION    3
#define BOOT_WORDS      ((uint16 *)0x703100LL)  /* main.asm BOOT_REQUEST */

#define SAVE_WORDS      ((uint16 *)0x701000LL)
#define SAVE_BYTES      ((uint8 *)0x701000LL)
#define H_MAGIC0        0               /* header words */
#define H_MAGIC1        1
#define H_VERSION       2
#define H_ROM           3
#define H_ITEMS         4
#define H_LENGTH        5
#define H_SUM           6
#define H_LEVEL         7
#define MAGIC0          0x4C4F          /* "OL" */
#define MAGIC1          0x5653          /* "SV" */
#define MAGIC_BOOT1     0x5242          /* "BR" */

#define ROM_CHECKSUM    romChecksum()

#define IO_LOAD         0
#define IO_SAVE         1
#define IO_COUNT        2

extern void olSramCopy(unsigned int sram, void* wram, unsigned int len, unsigned int toSram);
extern unsigned int olSramSum(unsigned int sram, unsigned int words);
extern uint32 olRandSeed;                   /* math.c */
extern unsigned int olDrawSeed;             /* sound.c */
extern uint8 olCamOnce[32];                 /* camera_fix.c */

unsigned int olSaveValid, olSaveLoad;
static unsigned int ioAt, ioMode;

static void io(void* p, unsigned int len)
{
    if (ioMode != IO_COUNT) olSramCopy(ioAt, p, len, ioMode);
    ioAt += len;
}

/* The one list, both ways and for the length: the layout cannot differ
   between a save and its load. */
static unsigned int fields(unsigned int mode, unsigned int n)
{
    ioMode = mode;
    ioAt = SAVE_SRAM + SAVE_HEADER;
    io(items, n * sizeof(ItemObj));
    io(olEnemyNext, n);
    io(olEnemyTurn, n);
    io(&olFirstActive, sizeof(olFirstActive));
    /* Lara's extraL: LaraSave, and what this port keeps beside it */
    io(&olLaraTurn, sizeof(olLaraTurn));
    io(&olLaraVSpeedHack, sizeof(olLaraVSpeedHack));
    io(&olLaraMoveAngle, sizeof(olLaraMoveAngle));
    io(&olLaraWaterState, sizeof(olLaraWaterState));
    io(&olLaraAngleX, sizeof(olLaraAngleX));
    io(&olLaraAngleZ, sizeof(olLaraAngleZ));
    io(&olLaraSwimTimer, sizeof(olLaraSwimTimer));
    io(&olLaraOxygen, sizeof(olLaraOxygen));
    io(&olLaraWeaponState, sizeof(olLaraWeaponState));
    io(&olLaraLooking, sizeof(olLaraLooking));
    io(&olLaraHitQuadrant, 1);
    io(&olLaraHitTimer, 1);
    io(&olLaraHitFrame, 1);
    io(olWeaponArms, sizeof(olWeaponArms));
    io(&olLaraLook, sizeof(olLaraLook));
    io(&olWeaponTarget, sizeof(olWeaponTarget));
    io(&olWeaponAim, sizeof(olWeaponAim));
    io(&olWeaponAimX, sizeof(olWeaponAimX));
    io(&olWeaponAimY, sizeof(olWeaponAimY));
    io(&olWeaponKit, sizeof(olWeaponKit));  /* the gun, the goal, her meshes */
    /* the inventory's counts, and the rest of the level's state */
    io(olInvCounts, sizeof(olInvCounts));
    io(olInventory, 11);                    /* use.c's INV_SLOTS */
    io(&olPickups, sizeof(olPickups));
    io(&olDartsLive, sizeof(olDartsLive));
    io(&olLevelEnd, sizeof(olLevelEnd));
    io(&olSecrets, sizeof(olSecrets));
    io(olCamOnce, sizeof(olCamOnce));
    io(&olCamLastIndex, sizeof(olCamLastIndex));
    io(&olFlipped, sizeof(olFlipped));
    io(&olRandSeed, sizeof(olRandSeed));
    io(&olDrawSeed, sizeof(olDrawSeed));
    return ioAt - (SAVE_SRAM + SAVE_HEADER);
}

/* The cartridge header's checksum. A function, not the dereference inline:
   816-tcc compiles `x != *(uint16 *)CONSTANT` as a compare against the
   constant's low word, immediate (`sbc #$FFDE`); a load of it is right. */
static unsigned int romChecksum(void)
{
    return *(uint16 *)0x80FFDELL;
}

/* A level's entity count, which a save of it must name: from its host
   directory, so that it can be asked of any level the cartridge carries,
   before one is loaded. */
static unsigned int levelItems(unsigned int level)
{
    union { const unsigned char* p; uint16 w[2]; } u;
    unsigned int n;
    u.w[0] = OL_HOST_ITEMS;
    u.w[1] = olLevelHostBanks[level];
    n = olData16(u.p, 0);
    return n > OL_ITEM_MAX ? OL_ITEM_MAX : n;
}

/* osCheckSave, and the sum and shape upstream trusts the RAM for */
static int Save_valid(void)
{
    uint16* h = SAVE_WORDS;
    unsigned int level = h[H_LEVEL], n, len;
    if (h[H_MAGIC0] != MAGIC0 || h[H_MAGIC1] != MAGIC1) return 0;
    if (h[H_VERSION] != SAVE_VERSION || h[H_ROM] != ROM_CHECKSUM) return 0;
    if (level >= olLevelCount) return 0;
    n = levelItems(level);
    len = (fields(IO_COUNT, n) + 1) & ~1u;
    if (h[H_ITEMS] != n || h[H_LENGTH] != len) return 0;
    return h[H_SUM] == olSramSum(SAVE_SRAM + SAVE_HEADER, len >> 1);
}

void Save_check(void)
{
    uint16* h = SAVE_WORDS;
    olSaveValid = Save_valid();
    olSaveLoad = olSaveValid && olBootKind == BOOT_LOAD && h[H_LEVEL] == olLevelIndex;
}

/* The next boot: which level, and whether to load the save over it. Read
   and spent by main.asm's SelectLevel; the restart follows. */
void Save_boot(unsigned int level, unsigned int kind)
{
    uint16* b = BOOT_WORDS;
    b[2] = level;
    b[3] = kind;
    b[1] = MAGIC_BOOT1;
    b[0] = MAGIC0;
}

/* gameSave: invalid while it is written, so that a save cut short by the
   power is no save rather than half of one; read back for the answer. */
int Save_write(void)
{
    uint16* h = SAVE_WORDS;
    unsigned int n = itemsCount, len;
    h[H_MAGIC0] = 0;
    len = fields(IO_SAVE, n);
    if (len & 1) SAVE_BYTES[SAVE_HEADER + len++] = 0;
    if (len > SAVE_SPACE - SAVE_HEADER) return olSaveValid = 0;
    h[H_VERSION] = SAVE_VERSION;
    h[H_ROM] = ROM_CHECKSUM;
    h[H_ITEMS] = n;
    h[H_LENGTH] = len;
    h[H_SUM] = olSramSum(SAVE_SRAM + SAVE_HEADER, len >> 1);
    h[H_LEVEL] = olLevelIndex;
    h[H_MAGIC1] = MAGIC1;
    h[H_MAGIC0] = MAGIC0;
    return olSaveValid = Save_valid();
}

void Save_requestLoad(void)
{
    Save_boot(SAVE_WORDS[H_LEVEL], BOOT_LOAD);
}

/* A block's sector, raised as olLevelInit raises it (Block's constructor)
   or given back; upstream's Block::load does the same around its load. */
static void blockFloors(int offset)
{
    unsigned int i;
    for (i = 1; i < itemsCount; i++) {
        ItemObj* item = items + i;
        if (ItemObj_class(item->type) == CLASS_BLOCK && ItemObj_status(item) != ITEM_STATUS_INVISIBLE)
            Block_updateFloor(item, offset);
    }
}

/* gameLoad, after startLevel */
void Save_apply(void)
{
    unsigned int i;
    olSaveLoad = 0;
    blockFloors(1024);
    fields(IO_LOAD, itemsCount);
    blockFloors(-1024);
    Nav_setFlipped(olFlipped);
    Weapon_restore();
    for (i = 0; i < itemsCount; i++) olItemDirty[i] = 1;
}
