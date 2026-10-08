/* GBA floor oracle. The TypeSNES floor runner extracts the four ItemObj
 * floor methods verbatim from src/fixed/item.h into floorupstream.inc.
 * This supplies only their types, constants and an input/output driver.
 * stdin: type state itemX itemY itemZ angleY queryX queryY queryZ floor
 * stdout: platform height (WALL when absent), resulting floor
 */
#include <cstdint>
#include <cstdio>

using int32 = std::int32_t;
enum {
    ITEM_TRAP_FLOOR = 35, ITEM_DRAWBRIDGE = 41,
    ITEM_TRAP_DOOR_1 = 65, ITEM_TRAP_DOOR_2 = 66,
    ITEM_BRIDGE_FLAT = 68, ITEM_BRIDGE_TILT_1 = 69, ITEM_BRIDGE_TILT_2 = 70,
    ANGLE_0 = 0, ANGLE_90 = 0x4000, ANGLE_180 = -0x8000,
    WALL = -127 * 256
};
struct ItemObj {
    struct { int32 x, y, z; } pos;
    struct { std::int16_t x, y, z; } angle;
    int32 type, state;
    int32 getBridgeFloor(int32 x, int32 z) const;
    int32 getTrapDoorFloor(int32 x, int32 z) const;
    int32 getDrawBridgeFloor(int32 x, int32 z) const;
    void getItemFloorCeiling(int32 x, int32 y, int32 z, int32* floor, int32* ceiling) const;
};
#include "floorupstream.inc"

int main()
{
    ItemObj item{};
    int angle, x, y, z, initial;
    while (std::scanf("%d %d %d %d %d %d %d %d %d %d", &item.type, &item.state,
                      &item.pos.x, &item.pos.y, &item.pos.z, &angle, &x, &y, &z, &initial) == 10) {
        item.angle.y = static_cast<std::int16_t>(angle);
        int32 h = WALL, floor = initial;
        if (item.type == ITEM_DRAWBRIDGE) {
            if (item.state == 1) h = item.getDrawBridgeFloor(x, z);
        } else if (item.type == ITEM_TRAP_DOOR_1 || item.type == ITEM_TRAP_DOOR_2) {
            if (item.state == 0) h = item.getTrapDoorFloor(x, z);
        } else {
            h = item.getBridgeFloor(x, z);
        }
        item.getItemFloorCeiling(x, y, z, &floor, nullptr);
        std::printf("%d %d\n", h, floor);
    }
    return std::ferror(stdin) ? 1 : 0;
}
