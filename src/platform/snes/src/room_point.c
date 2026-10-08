/* This small room helper lives beside the navigation users in WRAM, freeing
   contiguous space in camera.c's bank for creature implementations. */
#include "room.h"

/* The trace expects valid endpoints. Checking both also rejects a target
   outside the world when the segment crosses no sector boundary. */
int Room_pointOpen(Location* p)
{
    RoomQuery q;
    if (!Room_query(p->room, p->x, p->y, p->z, &q)) return 0;
    if (q.floor == OL_NO_FLOOR || q.ceiling == WALL) return 0;
    if (q.ceiling >= q.floor || p->y < q.ceiling || p->y > q.floor) return 0;
    p->room = q.room;
    return 1;
}
