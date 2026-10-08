/* OpenLara - SNES / Super FX target
 * The port of src/fixed/item.h: what a thing in the world is, and what it does
 * about being one.
 *
 * Ported rather than reimplemented. The names are upstream's so the two can be
 * read side by side -- `ItemObj::animProcess` is `ItemObj_animProcess` here,
 * because 816-tcc is a C compiler and a member function becomes a free one
 * taking the object first. Anything that exists only on this target keeps the
 * `ol` prefix, and there is a good deal of it: the level's tables live in game
 * pak RAM rather than in pointers, so where upstream writes
 * `level.anims[animIndex].frameEnd` this writes `olAnimFrames(animIndex)`.
 *
 * ---------------------------------------------------------------------------
 * ItemObj is 32 bytes, and that is not an accident. Every index into an array
 * of structs is a multiply on this compiler; a power of two is a shift. The
 * fields are upstream's, in upstream's names, minus the ones nothing here can
 * use yet -- there is no `extra` pointer: Lara's weapon state is separate,
 * and FX3's Nav pool records the index of each owning creature.
 * ---------------------------------------------------------------------------
 */

#ifndef H_OL_ITEM
#define H_OL_ITEM

#include "fixed.h"

/* The level's own tables come from level.h, which is where a lookup is a
   macro over the blob the GSU stages rather than a field of a `level`. */
#include "level.h"
#include "common.h"

/* Upstream's ITEM_FLAG_*, of which this port uses the four that mean
   something to it. The mask is five bits and an entity is active only when
   every one of them has been set -- see checkTrigger.

   The names are upstream's; the bit positions are not. Upstream's flags are a
   sixteen-bit word with the mask at bit 9 and room in between for things this
   port stores elsewhere or omits -- gravity, injured, animated, reverse,
   shadow. Packing the first four into a byte makes them fit an ItemObj that is
   a power of two long, which is what makes indexing one a shift. */
#define ITEM_FLAG_MASK      0x1F
#define ITEM_FLAG_ONCE      0x20
#define ITEM_FLAG_ACTIVE    0x40
#define ITEM_FLAG_DRIVEN    0x80    /* ol: a trigger somewhere names it */

/* The rest of upstream's word, in a second byte because the first is full.
   STATUS is upstream's two-bit field and GRAVITY is upstream's bit; the
   positions are this port's, as above. */
#define ITEM_FLAG_STATUS        0x03
#define ITEM_STATUS_NONE        0
#define ITEM_STATUS_ACTIVE      1
#define ITEM_STATUS_INACTIVE    2
#define ITEM_STATUS_INVISIBLE   3
#define ITEM_FLAG_GRAVITY       0x04
#define ITEM_FLAG_INJURED       0x08    /* upstream's; shot since it decided */
/* Upstream's ITEM_FLAG_REVERSE: the level said "active from the start" (a
   full mask in TR's item flags), so isActive reads backwards -- a trigger
   that fills the mask switches it *off*. ItemObj::init clears the mask and
   sets this. */
#define ITEM_FLAG_REVERSE       0x10
#define ITEM_FLAG_ANIMATED      0x20    /* wild aggression, currently FX3 */

/* TR's entity type zero. Kept here because animation processing needs the
   same Lara exception as upstream, before the object dispatcher is involved. */
#define ITEM_LARA               0
#define ITEM_CRYSTAL            83      /* upstream hides it (object.h Crystal) */

/* Upstream writes `(flags & ITEM_FLAG_STATUS) == ITEM_FLAG_STATUS_INVISIBLE`;
   this is the same test, over the byte the field ended up in. */
#define ItemObj_status(item)    ((item)->flags2 & ITEM_FLAG_STATUS)
#define ItemObj_setStatus(item, s) \
    ((item)->flags2 = (uint8)(((item)->flags2 & ~ITEM_FLAG_STATUS) | (s)))

typedef struct ItemObj {
    uint8  type;            /* TR's entity type */
    uint8  room;
    uint8  state;
    uint8  goalState;

    int16  x, y, z;         /* upstream's pos, in the world */
    uint16 angleY;          /* upstream's angle.y */

    uint16 animIndex;
    uint8  frameIndex;      /* within the animation, not the level */
    uint8  flags;

    int16  health;
    int16  timer;           /* object activation, or FX3's dead-enemy countdown */

    uint16 trigger;         /* ol: the trigger of the sector it stands in */
    uint8  model;           /* ol: the renderer indexes models, not types */
    uint8  mood;            /* enemies only, as upstream; four of them */

    uint8  flags2;          /* the rest of upstream's flags word */
    uint8  nextActive;      /* upstream's nextActive pointer, as an index */
    int16  roomFloor;       /* upstream's, set by updateRoom */
    int16  vSpeed;          /* upstream's */
    int16  hSpeed;          /* upstream's, from the animation's own speed */
                            /* exactly 32 bytes, so indexing is a shift */
} ItemObj;

#define OL_ITEM_HIDDEN      0xFF    /* the animation index that means "skip" */
#define OL_TIMER_EXPIRED    (-1)

extern ItemObj      items[OL_ITEM_MAX];
extern unsigned int itemsCount;

/* One byte per entity: it changed since the renderer last heard about it.
   gameRenderSync serializes Lara, the active list and the marked, not all
   sixty entities a frame -- which was two fields of the 65816 per rendered
   frame, tolerable at one frame a second and nothing else. An entity off
   the active list cannot move or animate, so what has to be marked is what
   changes one without the list: leaving it (its last tick's motion is not
   yet drawn), a status change, and the level load. */
extern unsigned char olItemDirty[OL_ITEM_MAX];
#define ItemObj_touch(item)     (olItemDirty[(item) - items] = 1)
/* Per-enemy state retained across animation links. Separate arrays keep the
   32-byte item ABI; max turn is stored in degrees (0/2/5 for the wolf). */
extern uint8 olEnemyNext[OL_ITEM_MAX], olEnemyTurn[OL_ITEM_MAX];

/* ---------------------------------------------------------------------------
 * ItemObj, ported.
 * ------------------------------------------------------------------------ */

/* Advance one frame, taking whatever transition the state graph offers and
   running whatever commands the animation carries. Upstream's animProcess. */
void ItemObj_animProcess(ItemObj* item);

/* The transition out of the current animation toward goalState, if this frame
   of it offers one. Upstream's animChange. */
void ItemObj_animChange(ItemObj* item);

/* The commands an animation fires: at the end of it, or on a given frame of
   it. Upstream's animCmd, with its `fx` argument. */
void ItemObj_animCmd(ItemObj* item, int fx);

/* Put it on an animation outright, which is what dying is: there is no
   transition from standing to dead. Upstream's animSet. */
void ItemObj_animSet(ItemObj* item, unsigned int anim);
void ItemObj_animSetFrame(ItemObj* item, unsigned int anim,
                          unsigned int frame);
/* Upstream's animSet(index, resetState, frame) in full: with resetState
   clear the state and goal are left alone, which is how a collision handler
   swaps the walk for its ascending or descending variant mid-stride. */
void ItemObj_animSetState(ItemObj* item, unsigned int anim,
                          unsigned int frame, int resetState);

/* Whether it is active this frame, which is where the timer lives.
   Upstream's Object::isActive. */
int ItemObj_isActive(ItemObj* item);

/* bool ItemObj::animIsEnd(int32 offset): on the frame `offset` before the
   animation's last. What a pickup, a key and a switch wait for. */
int ItemObj_animIsEnd(ItemObj* item, unsigned int offset);

/* void ItemObj::animSkip(stateBefore, stateAfter, advance): walk the graph
   to `before` without moving, optionally take one frame of it, and ask for
   `after` -- how an interaction animation is entered outright. */
void ItemObj_animSkip(ItemObj* item, unsigned int before, unsigned int after,
                      int advance);

/* bool ItemObj::moveTo(point, item, lerp): put `mover` at `point` in
   `item`'s frame, facing as it does. Snapped when lerp is clear; otherwise
   a step toward it, returning whether it has arrived. */
int ItemObj_moveTo(ItemObj* mover, int px, int py, int pz, ItemObj* item,
                   int lerp);

/* ---------------------------------------------------------------------------
 * The active list.
 *
 * Upstream threads a linked list through every entity that wants updating and
 * walks only that: `sFirstActive`, and a `nextActive` pointer per item. This
 * is the same list with an index where the pointer was, because a pointer is
 * four bytes here and ItemObj has to stay 32.
 *
 * What differs is *who* is on it. Upstream puts an entity there when a
 * trigger activates it; this port puts every entity there at load, because
 * TR's activation of creatures is not wired up and a wolf that never gets
 * updated is a wolf that never moves. Taking one off is upstream's, and is
 * what a falling floor does when it has finished falling.
 * ------------------------------------------------------------------------ */
#define OL_NO_ACTIVE    0xFF

extern unsigned int olFirstActive;

void ItemObj_activate(ItemObj* item);
void ItemObj_deactivate(ItemObj* item);

/* void ItemObj::move()
 *
 * Upstream's, both halves. The horizontal speed is the *animation's* -- TR
 * gives every animation a speed and an acceleration and this is where they
 * are read, which is why a walk and a run move differently without anything
 * saying so. The vertical is gravity.
 *
 * Lara uses `olLaraMoveAngle`, the port of `extraL->moveAngle`, when the
 * animation travels sideways or backwards. Other items use their facing. */
void ItemObj_move(ItemObj* item);

extern uint16 olLaraMoveAngle;

/* Upstream's updateRoom(offset): resolve the item's room and the floor under
   y + offset through the host sector table. */
void ItemObj_updateRoom(ItemObj* item, int offset);

/* Whether `lara` (a point with a radius, until she has a box of her own) is
   inside this item's box, and pushing her out of it. Upstream's
   collideBounds and collidePush -- which rotate her offset into the item's
   frame rather than rounding the item to a quadrant, and which test Y. */
/* The box it occupies right now, in its own frame. Upstream's
   getBoundingBox(false), which reads the current frame's; this reads the
   current *animation's*, because per-frame boxes are 47 KB and the host's
   blob has not got them. */
void ItemObj_getBoundingBox(ItemObj* item, AABBi* out);

int  ItemObj_collideBounds(ItemObj* item, ItemObj* lara, int radius, int height);
void ItemObj_collidePush(ItemObj* item, ItemObj* lara, int radius);
#ifdef OL_FX3
/* Immutable current-frame bounds. Legacy union-based callers stay separate
   until their own collision/interaction contracts have been migrated. */
const AABBi* ItemObj_frameBox(ItemObj* item);
int ItemObj_collideFrameBounds(ItemObj* item, ItemObj* lara, int radius);
int ItemObj_pushFrame(ItemObj* item, ItemObj* lara);
/* bit 0: pre-push contact; bit 1: geometric push performed (room guard follows). */
int ItemObj_frameContact(ItemObj* item, ItemObj* lara, int radius, int push);
#endif

/* The floor a bridge puts under a point, and the floor an entity puts under
   one generally. Upstream's getBridgeFloor and getItemFloorCeiling. */
int ItemObj_getBridgeFloor(ItemObj* item, int x, int z);
#ifdef OL_FX3
int ItemObj_getDrawBridgeFloor(ItemObj* item, int x, int z);
void ItemObj_extendPlatformBounds(ItemObj* item, int extent);
#endif
void ItemObj_getItemFloorCeiling(ItemObj* item, int x, int y, int z,
                                 int* floor);

#endif
