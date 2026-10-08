/* Execute the ORIGINAL shared engine's Nav, not a second transcription.
 * A minimal data adapter keeps platform/renderer headers out of the oracle.
 * stdin: box count, overlap count; six fields per box; overlaps; six zone
 * arrays (normal then alternate, ground-1/ground-2/fly); then commands.
 * stdout: little-endian snapshots
 * after each waypoint (1024 original cells, 11 words, target and waypoint).
 * Called by typesnes_dump.ts against the compiled 65816 cartridge. */
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#define H_COMMON
#define MAX_BOXES 1024
#define NO_BOX 0xFFFF
#define NAV_STEPS 5
#define ASSERT assert
#define X_MIN(a,b) ((a) < (b) ? (a) : (b))
#define X_MAX(a,b) ((a) > (b) ? (a) : (b))
#define X_CLAMP(v,a,b) X_MIN(X_MAX(v,a),b)
using uint8 = uint8_t;
using int16 = int16_t;
using uint16 = uint16_t;
using int32 = int32_t;
using uint32 = uint32_t;
enum { ZONE_GROUND_1, ZONE_GROUND_2, ZONE_FLY };
struct vec3i { int32 x, y, z; };
struct Box { uint8 minZ, maxZ, minX, maxX; int16 floor; uint16 overlap; };
struct Nav {
    struct Cell { uint16 boxIndex, weight, end, next; };
    Cell cells[MAX_BOXES];
    uint32 cellsCount, zoneType, weight, endBox, nextBox, headBox, tailBox;
    int32 stepHeight, dropHeight, vSpeed;
    uint32 mask;
    vec3i pos;
    void init(uint32);
    void search(uint16, const uint16*);
    vec3i getWaypoint(uint32, const vec3i&);
};
static struct {
    uint32 boxesCount;
    Box* boxes;
    uint16* overlaps;
    const uint16* zones[2][3];
} level;
static struct { int flipped; } gSaveGame;

#include "../../../fixed/nav.h"

static uint32 logicSeed;
static int32 rand_logic()
{
    logicSeed = logicSeed * 0x41C64E6Du + 0x3039;
    return (logicSeed >> 10) & 32767;
}
#define RAND_LOGIC(r) ((rand_logic() * (r)) >> 15)
#define X_INLINE inline
enum { MOOD_SLEEP, MOOD_STALK, MOOD_ATTACK, MOOD_ESCAPE };
enum { ITEM_FLAG_INJURED = 0x40, ITEM_FLAG_ANIMATED = 0x80 };
struct MoodTarget {
    vec3i pos;
    int32 health;
    struct { int16 y; } angle;
    struct Frame { struct { int32 minY; } box; } frame;
    Frame* getFrame() { return &frame; }
};
struct MoodExtra { Nav nav; };
struct EnemyMood {
    MoodExtra* extraE;
    vec3i pos;
    uint32 mood, flags;
    int32 aggression;
    struct {
        MoodTarget* target;
        uint32 boxIndex, boxIndexTarget, zoneIndex, zoneIndexTarget;
        int32 dist;
    } tinfo;
#include "navmood.inc"
};

static void word(uint32 v) { std::putchar(v & 255); std::putchar(v >> 8 & 255); }
static void point(const vec3i& p)
{
    for (int32 v : {p.x, p.y, p.z}) { word(uint32(v)); word(uint32(v) >> 16); }
}
static void snapshot(const Nav& n, const vec3i& wp)
{
    for (const auto& c : n.cells) {
        word(c.boxIndex); word(c.weight); word(c.end); word(c.next);
    }
    word(n.cellsCount); word(n.zoneType); word(n.weight);
    word(n.endBox); word(n.nextBox); word(n.headBox); word(n.tailBox);
    word(n.stepHeight); word(n.dropHeight); word(n.vSpeed); word(n.mask);
    point(n.pos); point(wp);
}

int main()
{
    unsigned count, links;
    if (!(std::cin >> count >> links) || !count || count > MAX_BOXES || !links) return 1;
    std::vector<Box> boxes(count);
    std::vector<uint16> overlaps(links), maps[6];
    for (auto& b : boxes) {
        int a, c, d, e, f, g;
        std::cin >> a >> c >> d >> e >> f >> g;
        b = {uint8(a), uint8(c), uint8(d), uint8(e), int16(f), uint16(g)};
    }
    unsigned v;
    for (auto& w : overlaps) { std::cin >> v; w = v; }
    for (auto& map : maps) {
        map.resize(count);
        for (auto& w : map) { std::cin >> v; w = v; }
    }
    level.boxesCount = count; level.boxes = boxes.data(); level.overlaps = overlaps.data();
    for (unsigned flip = 0; flip < 2; flip++)
        for (unsigned kind = 0; kind < 3; kind++)
            level.zones[flip][kind] = maps[flip * 3 + kind].data();
    Nav nav[3] = {};
    std::string op;
    unsigned slot, box;
    while (std::cin >> op >> slot >> box) {
        if (slot >= 3) return 2;
        Nav& n = nav[slot];
        if (op == "flip") {
            if (box > 1) return 3;
            gSaveGame.flipped = box;
        } else if (op == "init") {
            std::cin >> n.stepHeight >> n.dropHeight >> n.vSpeed;
            if (box >= count) return 3;
            n.init(box);
        } else if (op == "target") {
            n.nextBox = box;
            std::cin >> n.pos.x >> n.pos.y >> n.pos.z;
        } else if (op == "waypoint") {
            vec3i from;
            std::cin >> from.x >> from.y >> from.z;
            snapshot(n, n.getWaypoint(box, from));
        } else if (op == "mood") {
            MoodExtra extra = {};
            EnemyMood e = {};
            MoodTarget target;
            unsigned dest, next, hurt, animated;
            int angle;
            e.extraE = &extra; e.tinfo.target = &target;
            e.tinfo.boxIndex = box;
            std::cin >> dest >> next >> e.mood >> hurt >> animated >> e.aggression
                >> e.pos.x >> e.pos.y >> e.pos.z
                >> target.pos.x >> target.pos.y >> target.pos.z
                >> angle >> target.health >> target.frame.box.minY >> e.tinfo.dist >> logicSeed
                >> extra.nav.stepHeight >> extra.nav.dropHeight >> extra.nav.vSpeed;
            extra.nav.init(box);
            extra.nav.endBox = box; extra.nav.nextBox = next; extra.nav.pos = e.pos;
            target.angle.y = angle;
            e.flags = (hurt ? ITEM_FLAG_INJURED : 0) | (animated ? ITEM_FLAG_ANIMATED : 0);
            e.tinfo.boxIndexTarget = dest;
            e.tinfo.zoneIndex = e.getZones()[box]; e.tinfo.zoneIndexTarget = e.getZones()[dest];
            e.updateMood();
            const vec3i wp = extra.nav.getWaypoint(box, e.pos);
            word(e.mood); word(logicSeed); word(logicSeed >> 16);
            snapshot(extra.nav, wp);
        } else return 4;
        if (!std::cin) return 5;
    }
    return 0;
}
