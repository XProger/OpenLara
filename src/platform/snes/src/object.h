/* OpenLara - SNES / Super FX target
 * The port of src/fixed/object.h, and of the dispatch in ItemObj::init that
 * decides which of it applies.
 *
 * Upstream writes `new (this) Door(room)` and gets a vtable. There is no
 * vtable here -- 816-tcc is a C compiler -- so `ItemObj::init`'s switch
 * becomes a table from type to class, and `update` becomes a switch on it.
 * That is the same dispatch with the indirection spelled out, and it is what
 * every object behaviour hangs off: without it, "what does this thing do when
 * it is activated" has nowhere to be asked.
 */

#ifndef H_OL_OBJECT
#define H_OL_OBJECT

#include "item.h"

/* Door, Trapdoor, Bridge and the dart emitter all use these two. */
#define STATE_CLOSE     0
#define STATE_OPEN      1

/* Switch::STATE_UP and STATE_DOWN, which are its own. */
#define SWITCH_STATE_UP     0
#define SWITCH_STATE_DOWN   1

/* One per class upstream has that this port has anything to say about. The
   rest of TR's thirty-odd -- keyholes, the moving block, the boulder, Natla --
   fall to CLASS_NONE and stand still. */
enum {
    CLASS_NONE,
    CLASS_LARA,
    CLASS_ENEMY,
    CLASS_DOOR,     /* and trapdoors: shut when inactive, open when not */
    CLASS_SWITCH,
    CLASS_BRIDGE,   /* no update of its own; it is a floor, not an actor */
    CLASS_PICKUP,   /* a sprite in TR, and a billboard quad here */
    CLASS_TRAP_FLOOR,/* the floor that shakes, drops and lies there */
    CLASS_TRAP_DOOR,/* a door's update and a bridge's floor: shut, she
                       stands on it; open, she falls through */
    CLASS_HOLE,     /* keyholes and puzzle holes */
    CLASS_BLOCK,    /* the pushable cube */
    CLASS_DART_EMITTER, /* FX3: fires from the dart pool (trap.c) */
    CLASS_DART,
    CLASS_SWING_BLADE,  /* FX3: trap.c */
    CLASS_FX,           /* FX3: blood, ricochet, splash, bubble, smoke (fx.c) */
    CLASS_DRAWBRIDGE    /* FX3: a lowered bridge covers two forward sectors */
};

/* TrapFloor's own states, upstream's enum. LEVEL1's two are model 9 with
   animations 223 to 226, one per state and in this order. */
#define TRAP_FLOOR_STATE_STATIC 0
#define TRAP_FLOOR_STATE_SHAKE  1
#define TRAP_FLOOR_STATE_FALL   2
#define TRAP_FLOOR_STATE_DOWN   3

/* How far above a falling floor Lara stands when she is standing on it.
   Upstream's `pos.y - 512`, which is the height of the block's own top. */
#define TRAP_FLOOR_HEIGHT   512

unsigned int ItemObj_class(unsigned int type);

/* ItemObj::update, dispatched. `lara` is item 0, which every class that has
   an opinion about her needs. */
void ItemObj_update(ItemObj* item, ItemObj* lara);

/* bool Switch::use(int32 t): the switch's own trigger asking whether the
   lever has been pulled -- the pull's animation ends in a KILL, which is
   what makes the status INACTIVE. A timer keeps a pulled switch counting
   down to spring back. */
int Switch_use(ItemObj* item, int timer);

/* bool Key::use(): the keyhole's trigger asking whether the key is in it. */
int Key_use(ItemObj* item);

/* Pickup::use -- whether the thing has been taken, which is what a pickup
   trigger asks before it fires. Upstream's returns true once and flips the
   status, so the second asking is false. */
int Pickup_use(ItemObj* item);

/* Upstream's gSaveGame.pickups, and the counts a keyhole reads. use.c. */
extern unsigned int olPickups;
extern uint8 olInventory[];
void Inventory_add(unsigned int type);

/* The collide() methods -- use.c and block.c -- and the loop that calls
   them, upstream's Lara::updateObjectsCollision. `input` is hers this
   tick; Action is what every one of them waits for. */
int  Object_checkLimit(ItemObj* item, ItemObj* L, const int* lim, int pitch);
void Lara_collideObjects(ItemObj* L, unsigned int input);
void Block_collide(ItemObj* item, ItemObj* L, unsigned int input);
void Block_update(ItemObj* item);
void Block_updateFloor(ItemObj* item, int offset);
#ifdef OL_FX3
extern unsigned int olDartsLive;
void DartEmitter_update(ItemObj* item);
void Dart_update(ItemObj* dart);
void Dart_collideLara(ItemObj* lara);
#define OL_BLADE_MAX 8
extern uint8 olBlades[OL_BLADE_MAX];
extern unsigned int olBladeCount;
void SwingBlade_update(ItemObj* item);
void SwingBlade_collideLara(ItemObj* lara);
/* door.c: a shut door's sectors are walls, as upstream's Door::action
   makes them. Door_tick follows Door_update in the update. */
extern uint8 olDoorOpen[];
void Door_action(ItemObj* item, int close);
void Door_tick(ItemObj* item);
#endif

#endif
