/* OpenLara - SNES / Super FX target
 * The port of src/fixed/room.h's interface. TR's FloorData never reaches the
 * cartridge -- the packer resolves it -- so a trigger is an index here.
 */

#ifndef H_OL_ROOM
#define H_OL_ROOM

#include "fixed.h"

typedef struct Location {
    int x, y, z;
    unsigned int room;
} Location;

typedef struct RoomQuery {
    unsigned int room;
    int floor, ceiling;
    int sectorX, sectorZ;
    unsigned int trigger;
    int floorSlopeX, floorSlopeZ;
    int ceilingSlopeX, ceilingSlopeZ;
} RoomQuery;

/* Upstream's WALL: a floor byte of NO_FLOOR, in world units. The host table
   reports that floor as OL_NO_FLOOR and a missing ceiling as this; the
   collision code translates so its handlers read as upstream's. */
#define WALL                (-127 * 256)

/* Resolve a world point from a room hint through horizontal and vertical
   portals. Floors and ceilings come from the bottom/top linked sectors and
   include TR's slants. Returns zero only for an invalid table or room hint. */
int Room_query(unsigned int roomHint, int x, int y, int z, RoomQuery* out);

/* Shared by the camera and weapon targeting. Clips `to` at a room wall,
   floor or ceiling and returns whether the whole segment was clear.
   The sector walker lives in camera.c (camera_fx3.asm on native FX3);
   endpoints must be inside the world. */
int Room_trace(const Location* from, Location* to);
/* Validates an endpoint and resolves its room in place. */
int Room_pointOpen(Location* p);

/* Upstream's ItemObj::getWaterLevel and getWaterDepth, in one walk: from
   `room`, up through the vertical portals to the last water room or down to
   the first, then the surface of that column and how far the floor lies
   below it. Both are WALL when no water is in the column. */
void Room_water(unsigned int room, int x, int z, int* level, int* depth);

/* Forget the room record Room_sector remembers; at level load, and after
   a block has rewritten a sector. */
void Room_reset(void);
#ifdef OL_FX3
/* Uses the existing renderer portal list through FX3's CPU ROM mapping. */
int Room_isAdjacent(unsigned int room, unsigned int other);
#endif

/* Upstream's Room::getSector, as the host table's sector index: what a
   block rewrites. */
unsigned int Room_sectorAt(unsigned int r, int x, int z);

/* The ACTIVATE_OBJECT arm of checkTrigger over trigger t's entities from
   the `first`th: a switch, key or pickup trigger names the thing used as
   its first entity, and that one is asked rather than activated. */
void fireTrigger(unsigned int t, unsigned int first);
void checkTrigger(unsigned int t);
#ifdef OL_FX3
/* A landed block: only OBJECT triggers pass, with no Lara weapon/pad gate. */
void checkTriggerObject(unsigned int t);
#endif

/* An END trigger has fired (upstream's nextLevel; nothing loads the next
   one yet), and the secrets found so far as bits (gSaveGame.secrets). */
extern unsigned int olLevelEnd;
extern unsigned int olSecrets;

#endif
