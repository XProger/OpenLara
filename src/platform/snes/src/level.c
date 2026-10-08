/* OpenLara - SNES / Super FX target
 * Where the level's tables are, and reading the entity list out of them.
 * See level.h for why a table lookup here is a function rather than a field.
 */

#include "level.h"
#include "item.h"
#include "lara.h"
#include "object.h"
#include "room.h"
#ifdef OL_FX3
#include "nav_fx3.h"
#include "fx.h"
#include "enemy.h"
#define ITEM_BYTE(o) olItemData[o]
#define ITEM_WORD(o) olData16(olItemData, o)
#define ITEM_SIGNED(o) olDataS16(olItemData, o)
#else
#define ITEM_BYTE(o) olBlob[o]
#define ITEM_WORD(o) olBlob16(o)
#define ITEM_SIGNED(o) olBlobS16(o)
#endif

#ifdef OL_FX3
OlLevelDesc   olLevel;                      /* main.asm SelectLevel fills these */
unsigned int  olLevelIndex, olLevelCount, olBootKind;
uint16        olLevelHostBanks[OL_LEVEL_MAX], olLevelIds[OL_LEVEL_MAX];
unsigned char olHostDirectory[256];         /* host_tables.asm olHostBind */
#endif

uint8        olDoors[OL_LIST_MAX], olBridges[OL_LIST_MAX], olCreatures[OL_LIST_MAX];
uint8        olUsables[OL_LIST_MAX];
unsigned int olDoorCount, olBridgeCount, olCreatureCount, olUsableCount;
int          olBridgeBox[4];

#ifndef OL_FX3
unsigned int olRoomBase, olSectorBase, olLinkBase, olModelBase;
unsigned int olAnimBoxBase, olAnimSpeedBase;
unsigned int olAnimAimBase;
unsigned int olAnimBase, olTransBase;
unsigned int olCmdBase, olTriggerBase, olItemBase;
unsigned int olBoxBase, olOverlapBase, olZoneBase;
unsigned int olStaticBase, olStaticFirstBase;
#endif
unsigned int olRoomCount, olSectorCount;
unsigned int olBoxCount, olTriggerCount, olStaticCount;

/* Read the level's header and the entity list, once. Everything else is left
   where it lies and reached through the accessors in level.h. */
void olLevelInit(void)
{
    unsigned int models, anims, trans, cmds, rooms, links, i, o;
#ifdef OL_FX3
    itemsCount     = olHeader16(OL_HOST_ITEMS);
    olTriggerCount = olHeader16(OL_HOST_TRIGGERS);
    olBoxCount     = olHeader16(OL_HOST_BOXES);
    olRoomCount    = olHeader16(OL_HOST_ROOMS);
    olSectorCount  = olHeader16(OL_HOST_SECTORS);
    olStaticCount  = olHeader16(OL_HOST_STATICS);
    if (itemsCount > OL_ITEM_MAX) itemsCount = OL_ITEM_MAX;
    Nav_reset();
#else
    itemsCount     = olBlob16(2);
    models         = olBlob16(4);
    anims          = olBlob16(6);
    trans          = olBlob16(8);
    olTriggerCount = olBlob16(10);
    cmds           = olBlob16(12);
    olBoxCount     = olBlob16(14);
    rooms          = olBlob16(18);
    olRoomCount    = rooms;
    olSectorCount  = olBlob16(22);
    links          = olBlob16(24);
    olStaticCount  = olBlob16(26);

    if (itemsCount > OL_ITEM_MAX) itemsCount = OL_ITEM_MAX;

    olRoomBase    = 28;
    olSectorBase  = olRoomBase + rooms * 10;
    olLinkBase    = olSectorBase + (olSectorCount << 2);
    olModelBase   = olLinkBase + (links << 3);
    olAnimBoxBase = olModelBase + models * 16;
    olAnimSpeedBase = olAnimBoxBase + anims * 12;
    olAnimBase    = olAnimSpeedBase + anims * 8;
    olTransBase   = olAnimBase + anims * 14;
    olCmdBase     = olTransBase + trans * 6;
    olTriggerBase = olCmdBase + cmds * 8;
    /* The packer says where this is rather than the host adding up the tables
       in front of it: three pieces of code were doing that sum and every table
       added broke the two not being edited. */
    olItemBase    = olBlob16(20);
    olBoxBase     = olItemBase + itemsCount * 16;
    olOverlapBase = olBoxBase + olBoxCount * 8;
    olZoneBase    = olOverlapBase + olBlob16(16) * 2;
    olStaticBase  = olZoneBase + (olBoxCount << 2);
    olStaticFirstBase = olStaticBase + olMul20(olStaticCount);
    olAnimAimBase = olStaticFirstBase + ((rooms + 1) << 1);
#endif

    /* Decode initial entity records into mutable ItemObjs: where an entity
       stands changes, and what it is doing changes with it. The source item
       table itself stays read-only. */
#ifdef OL_FX3
    o = 0;
#else
    o = olItemBase;
#endif
    for (i = 0; i < itemsCount; i++, o += 16)
    {
        ItemObj* item = items + i;

        item->type      = ITEM_BYTE(o);
        item->room      = ITEM_BYTE(o + 1);
        item->model     = ITEM_BYTE(o + 2);
        item->x         = ITEM_SIGNED(o + 4);
        item->y         = ITEM_SIGNED(o + 6);
        item->z         = ITEM_SIGNED(o + 8);
        item->angleY    = ITEM_WORD(o + 10);
        item->trigger   = ITEM_WORD(o + 12);

        item->animIndex = (uint16)olModelFirstAnim(item->model);
        item->state     = (uint8)olAnimState(item->animIndex);
        item->goalState = item->state;
        /* TR's own two facts about activation, as the packer laid them
           out: the five-bit mask, and the hidden bit in ITEM_FLAG_ONCE's
           slot. ItemObj::init's rules for them come after the list is
           read, below, since one of them activates. */
        item->flags     = (uint8)(ITEM_BYTE(o + 3) & (ITEM_FLAG_MASK | ITEM_FLAG_ONCE));
#ifdef OL_FX3
        item->flags2    = (uint8)((ITEM_BYTE(o + 3) >> 1) & ITEM_FLAG_ANIMATED);
#else
        item->flags2    = 0;
#endif
        item->nextActive = OL_NO_ACTIVE;
        olItemDirty[i]  = 1;
        item->roomFloor = OL_NO_FLOOR;
        item->vSpeed    = 0;
        item->timer     = 0;
        item->mood      = 0;
        item->hSpeed    = 0;
#ifdef OL_FX3
        /* Constructors precede activation upstream. A full-mask creature
           must already be alive when ItemObj_activate checks its health. */
        item->health    = (int16)Enemy_health(item->type);
        if (i == 0) item->health = LARA_MAX_HEALTH;
#else
        item->health    = 0;
#endif
        olEnemyNext[i] = item->state;
        olEnemyTurn[i] = 0;
#ifdef OL_FX3
        olEnemyTouch[i] = 0;
#endif

        /* Start them out of step, so a room full of one model does not move
           as one thing. */
        item->frameIndex = (uint8)
            ((i * OL_PHASE_SPREAD) % olAnimFrames(item->animIndex));
        /* Wolf's constructor starts one frame before the last frame. */
        if (item->type == 7) item->frameIndex = (uint8)(olAnimFrames(item->animIndex) - 2);
    }

    /* Lara stands. Her model's animation zero is ANIM_RUN -- upstream's own
       enum says so -- so an entity that starts on its first animation starts
       her running on the spot. Upstream sets ANIM_STAND at level load and so
       does this. */
    if (itemsCount > 0) {
        ItemObj_animSet(items, olModelFirstAnim(items[0].model) + LARA_ANIM_STAND);
    }

    /* Upstream updates only active entities. Triggers put doors, traps
       and creatures on this list, as TR does; the level can also load
       something active (below). Static scenery, pickups and sleeping
       creatures must not consume a 30 Hz CPU budget. */
    olFirstActive = OL_NO_ACTIVE;

    /* Which entities a trigger somewhere names. Only those are driven by
       standing on one: forcing a goal state on a creature whose states are
       not a door's sends it somewhere TR never would. */
    for (i = 0; i < olTriggerCount; i++)
    {
        unsigned int j;
        for (j = 0; j < olTrigItemCount(i) && j < 8; j++)
        {
            unsigned int k = olTrigItem(i, j);
            /* Lara is item 0 of every level and is not a door. */
            if (k > 0 && k < itemsCount) items[k].flags |= ITEM_FLAG_DRIVEN;
        }
    }

    /* The handfuls the per-tick loops want, found once, and on the same
       pass ItemObj::init's two rules for the level's flags. Hidden means
       invisible until a trigger shows it. A full mask means active from
       the start: the mask is cleared, REVERSE set so that filling it
       switches the thing *off*, and it goes on the active list now. One
       pass, because the load tick is already the longest tick there is
       and each pass over the list is a field's worth of it. A trapdoor is
       a floor as a bridge is, two sectors long, so it joins that list and
       widens its box by the sector it opens toward. */
    olDoorCount = olBridgeCount = olCreatureCount = olUsableCount = 0;
#ifdef OL_FX3
    olBladeCount = 0;
#endif
    olBridgeBox[0] = olBridgeBox[2] = 0x7FFF;
    olBridgeBox[1] = olBridgeBox[3] = -0x7FFF;
    Room_reset();
    for (i = 1; i < itemsCount; i++)
    {
        ItemObj* item = items + i;
        unsigned int cls = ItemObj_class(item->type);

        if (item->flags & ITEM_FLAG_ONCE) {
            item->flags &= (uint8)~ITEM_FLAG_ONCE;
            ItemObj_setStatus(item, ITEM_STATUS_INVISIBLE);
        }
        if ((item->flags & ITEM_FLAG_MASK) == ITEM_FLAG_MASK) {
            item->flags &= (uint8)~ITEM_FLAG_MASK;
            item->flags2 |= ITEM_FLAG_REVERSE;
            ItemObj_activate(item);
            if (ItemObj_status(item) == ITEM_STATUS_NONE) {
                ItemObj_setStatus(item, ITEM_STATUS_ACTIVE);
            }
        }
        /* upstream's Crystal constructor, after ItemObj's: "disable
           crystals for now" -- the PlayStation's save points, which the PC
           levels carry (LEVEL1 one, LEVEL2 two) and upstream never shows */
        if (item->type == ITEM_CRYSTAL) ItemObj_setStatus(item, ITEM_STATUS_INVISIBLE);

        switch (cls)
        {
            case CLASS_DOOR:
                if (olDoorCount < OL_LIST_MAX) olDoors[olDoorCount++] = (uint8)i;
#ifdef OL_FX3
                /* Door's constructor: action(true), every door shut */
                Door_action(item, 1);
#endif
                break;
            case CLASS_BRIDGE:
            case CLASS_TRAP_DOOR:
#ifdef OL_FX3
            case CLASS_DRAWBRIDGE:
#endif
                if (olBridgeCount < OL_LIST_MAX) olBridges[olBridgeCount++] = (uint8)i;
#ifdef OL_FX3
                ItemObj_extendPlatformBounds(item, cls == CLASS_DRAWBRIDGE ? 2 :
                    cls == CLASS_TRAP_DOOR ? 1 : 0);
#else
                {
                    int sx = items[i].x >> 10, sz = items[i].z >> 10;
                    int w = (cls == CLASS_TRAP_DOOR) ? 1 : 0;
                    if (sx - w < olBridgeBox[0]) olBridgeBox[0] = sx - w;
                    if (sx + w > olBridgeBox[1]) olBridgeBox[1] = sx + w;
                    if (sz - w < olBridgeBox[2]) olBridgeBox[2] = sz - w;
                    if (sz + w > olBridgeBox[3]) olBridgeBox[3] = sz + w;
                }
#endif
                break;
            case CLASS_ENEMY:
                if (olCreatureCount < OL_LIST_MAX) olCreatures[olCreatureCount++] = (uint8)i;
                break;
#ifdef OL_FX3
            case CLASS_SWING_BLADE:
                if (olBladeCount < OL_BLADE_MAX) olBlades[olBladeCount++] = (uint8)i;
                break;
#endif
            /* The ones with a collide() to call: use.c's list. A block is
               also a floor from the start -- Block's constructor raises
               its sector to its top -- unless the level hid it. */
            case CLASS_BLOCK:
                if (ItemObj_status(items + i) != ITEM_STATUS_INVISIBLE) {
                    Block_updateFloor(items + i, -1024);
                }
                /* fall through */
            case CLASS_SWITCH:
            case CLASS_PICKUP:
            case CLASS_HOLE:
                if (olUsableCount < OL_LIST_MAX) olUsables[olUsableCount++] = (uint8)i;
                break;
            default:
                break;
        }
    }
#ifdef OL_FX3
    Fx_levelInit();                     /* the effects pool, FX.BIN's */
#endif
}
