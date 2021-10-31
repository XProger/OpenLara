#ifndef H_NAV
#define H_NAV

#include "common.h"

#define NAV_INDEX   0x3FFF
#define NAV_WEIGHT  0x7FFF
#define NAV_BLOCKED 0x8000

void Nav::init(uint32 boxIndex)
{
    switch (stepHeight)
    {
        case 256 : zoneType = ZONE_GROUND_1; break;
        case 512 : zoneType = ZONE_GROUND_2; break;
        default  : zoneType = ZONE_FLY;
    }

    weight = 0;
    endBox = NO_BOX;
    nextBox = NO_BOX;

    headBox = NO_BOX;
    tailBox = NO_BOX;

    mask = 0x400;

    for (int32 i = 0; i < level.boxesCount; i++)
    {
        cells[i].end = NO_BOX;
        cells[i].next = NO_BOX;
        cells[i].weight = 0;
    }

    const uint16* defZones = level.zones[0][zoneType];
    const uint16* altZones = level.zones[1][zoneType];

    uint16 defZone = defZones[boxIndex];
    uint16 altZone = altZones[boxIndex];

    cellsCount = 0;
    Nav::Cell* cell = cells;

    for (int32 i = 0; i < level.boxesCount; i++)
    {
        if ((defZone == defZones[i]) || (altZone == altZones[i]))
        {
            (*cell++).boxIndex = i;
            cellsCount++;
        }
    }

    ASSERT(cellsCount > 0);
}

vec3i Nav::getWaypoint(uint32 boxIndex, const vec3i &from)
{
    if (nextBox != NO_BOX && nextBox != endBox)
    {
        endBox = nextBox;

        Nav::Cell &cell = cells[endBox];

        if (cell.next == NO_BOX && tailBox != endBox)
        {
            cell.next = headBox;

            if (headBox == NO_BOX) {
                tailBox = endBox;
            }

            headBox = endBox;
        }

        weight++;
        cell.weight = weight;
        cell.end = NO_BOX;
    }

    if (headBox != NO_BOX)
    {
        const uint16* zones = level.zones[gSaveGame.flipped][zoneType];
        uint16 zone = zones[headBox];

        for (int32 i = 0; (i < NAV_STEPS) && (headBox != NO_BOX); i++)
        {
            search(zone, zones);
        }
    }

    if (boxIndex == endBox)
        return pos;

    vec3i wp = from;

    if (boxIndex == NO_BOX)
        return wp;

    const Box* box = level.boxes + boxIndex;

    int32 bMinX = (box->minX << 10);
    int32 bMaxX = (box->maxX << 10) - 1;
    int32 bMinZ = (box->minZ << 10);
    int32 bMaxZ = (box->maxZ << 10) - 1;

    int32 minX = bMinX;
    int32 maxX = bMaxX;
    int32 minZ = bMinZ;
    int32 maxZ = bMaxZ;

    while ((boxIndex != NO_BOX) && !(level.boxes[boxIndex].overlap & mask))
    {
        box = level.boxes + boxIndex;
    
        bMinX = (box->minX << 10);
        bMaxX = (box->maxX << 10) - 1;
        bMinZ = (box->minZ << 10);
        bMaxZ = (box->maxZ << 10) - 1;

        if (from.x >= bMinX && from.x <= bMaxX && from.z >= bMinZ && from.z <= bMaxZ)
        {
            minX = bMinX;
            maxX = bMaxX;
            minZ = bMinZ;
            maxZ = bMaxZ;
        } else {
            if ((wp.x < bMinX) || (wp.x > bMaxX))
            {
                if ((wp.z < minZ) || (wp.z > maxZ))
                    break;
                wp.x = X_CLAMP(wp.x, bMinX + 512, bMaxX - 512);
                minZ = X_MAX(minZ, bMinZ);
                maxZ = X_MIN(maxZ, bMaxZ);
            }

            if ((wp.z < bMinZ) || (wp.z > bMaxZ))
            {
                if ((wp.x < minX) || (wp.x > maxX))
                    break;
                wp.z = X_CLAMP(wp.z, bMinZ + 512, bMaxZ - 512);
                minX = X_MAX(minX, bMinX);
                maxX = X_MIN(maxX, bMaxX);
            }
        }

        if (boxIndex == endBox)
        {
            wp.x = X_CLAMP(wp.x, bMinX + 512, bMaxX - 512);
            wp.z = X_CLAMP(wp.z, bMinZ + 512, bMaxZ - 512);
            break;
        }

        boxIndex = cells[boxIndex].end;
    }

    wp.y = box->floor - ((zoneType == ZONE_FLY) ? 384 : 0); // TODO check for 320

    return wp;
}

void Nav::search(uint16 zone, const uint16* zones)
{
    Nav::Cell &curr = cells[headBox];
    const Box &b = level.boxes[headBox];

    uint16 overlapIndex = b.overlap & NAV_INDEX;

    bool end = false;

    do {
        uint16 boxIndex = level.overlaps[overlapIndex++];

        end = boxIndex & NAV_BLOCKED;
        if (end) {
            boxIndex &= NAV_INDEX;
        }

        if (zone != zones[boxIndex])
            continue;

        int32 diff = level.boxes[boxIndex].floor - b.floor;
        if (diff > stepHeight || diff < dropHeight)
            continue;

        Nav::Cell &next = cells[boxIndex];

        uint16 cWeight = curr.weight & NAV_WEIGHT;
        uint16 nWeight = next.weight & NAV_WEIGHT;

        if (cWeight < nWeight)
            continue;

        if (curr.weight & NAV_BLOCKED)
        {
            if (cWeight == nWeight)
                continue;

            next.weight = curr.weight;
        }
        else
        {        
            if ((cWeight == nWeight) && !(next.weight & NAV_BLOCKED))
                continue;

            if (level.boxes[boxIndex].overlap & mask)
            {
                next.weight = curr.weight | NAV_BLOCKED;
            }
            else
            {
                next.weight = curr.weight;
                next.end = headBox;
            }
        }

        if (next.next == NO_BOX && boxIndex != tailBox)
        {
            cells[tailBox].next = boxIndex;
            tailBox = boxIndex;
        }
    } while (!end);

    headBox = curr.next;
    curr.next = NO_BOX;
}

#endif
