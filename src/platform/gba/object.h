#ifndef H_OBJECT
#define H_OBJECT

#include "item.h"
#include "lara.h"
#include "inventory.h"

vec3i getBlockOffset(int16 angleY, int32 offset)
{
    if (angleY == ANGLE_0)
        return vec3i(0, 0, -offset);
    if (angleY == ANGLE_180)
        return vec3i(0, 0, offset);
    if (angleY == ANGLE_90)
        return vec3i(-offset, 0, 0);
    return vec3i(offset, 0, 0);
}

struct Limit
{
    Bounds box;
    vec3s angle;
};

namespace Limits
{
    static const Limit SWITCH = {
        Bounds( -200, 200, 0, 0, 312, 512 ),
        vec3s( ANGLE(10), ANGLE(30), ANGLE(10) )
    };

    static const Limit SWITCH_UW = {
        Bounds( -1024, 1024, -1024, 1024, -1024, 1024 ),
        vec3s( ANGLE(80), ANGLE(80), ANGLE(80) )
    };

    static const Limit BLOCK = {
        Bounds( -300, 300, 0, 0, -692, -512 ),
        vec3s( ANGLE(10), ANGLE(30), ANGLE(10) )
    };

    static const Limit PICKUP = {
        Bounds( -256, 256, -100, 100, -256, 100 ),
        vec3s( ANGLE(10), 0, 0 )
    };

    static const Limit PICKUP_UW = {
        Bounds( -512, 512, -512, 512, -512, 512 ),
        vec3s( ANGLE(45), ANGLE(45), ANGLE(45) )
    };

    static const Limit HOLE = {
        Bounds( -200, 200, 0, 0, 312, 512 ),
        vec3s( ANGLE(10), ANGLE(30), ANGLE(10) )
    };
};


struct Object : Item
{
    Object(Room* room) : Item(room) {}

    virtual void update()
    {
        animProcess();
    }

    bool isActive()
    {
        if ((flags.mask != ITEM_FLAGS_MASK_ALL) || (timer == -1))
            return flags.reverse == 1;

        if (timer == 0)
            return flags.reverse == 0;
        
        timer--;

        if (timer == 0)
            timer = -1;

        return flags.reverse == 0;
    }

    bool checkLimit(Lara* lara, const Limit& limit)
    {
        int16 ax = abs(lara->angle.x - angle.x);
        int16 ay = abs(lara->angle.y - angle.y);
        int16 az = abs(lara->angle.z - angle.z);

        if (ax > limit.angle.x || ay > limit.angle.y || az > limit.angle.z)
            return false;

        vec3i d = lara->pos - pos;

        matrixSetIdentity();
        matrixRotateZXY(-angle.x, -angle.y, -angle.z);
        const Matrix &m = matrixGet();

        vec3i p;
        p.x = DP33(m[0], d) >> FIXED_SHIFT;
        p.y = DP33(m[1], d) >> FIXED_SHIFT;
        p.z = DP33(m[2], d) >> FIXED_SHIFT;

        return boxContains(limit.box, p);
    }

    void collideDefault(Lara* lara, CollisionInfo* cinfo)
    {
        if (!updateHitMask(lara, cinfo))
            return;

        if (!cinfo->enemyPush)
            return;

        collidePush(lara, cinfo, false);
    }
};


struct SpriteEffect : Item
{
    SpriteEffect(Room* room) : Item(room)
    {
        tick = 0;
        timer = 0;
        hSpeed = 0;
        frameIndex = 0;
        activate();
    }

    virtual void update()
    {
        tick++;
        if (tick >= timer)
        {
            tick = 0;

            if (flags.animated)
            {
                frameIndex++;
                if (frameIndex >= -models[type].count)
                {
                    remove();
                    return;
                }
            } else {
                remove();
                return;
            }
        }

        if (hSpeed)
        {
            pos.x += phd_sin(angle.y) * hSpeed >> FIXED_SHIFT;
            pos.z += phd_cos(angle.y) * hSpeed >> FIXED_SHIFT;
        }
    }
};


struct Bubble : Item
{
    Bubble(Room* room) : Item(room)
    {
        soundPlay(SND_BUBBLE, pos);
        frameIndex = rand_draw() % (-models[type].count);
        vSpeed = -(10 + (rand_draw() % 6));
        angle = vec3s(0, 0, ANGLE_90);
        activate();

        roomFloor = getWaterLevel();
    }

    virtual void update()
    {
        pos.y += vSpeed;
        if (roomFloor > pos.y)
        {
            remove();
            return;
        }

        angle.x += ANGLE(9);
        angle.z += ANGLE(13);

        int32 dx = phd_sin(angle.x);
        int32 dz = phd_sin(angle.z);

        pos.x += dx * 11 >> FIXED_SHIFT;
        pos.z += dz * 8 >> FIXED_SHIFT;

        Room* nextRoom = room->getRoom(pos.x, pos.y, pos.z);
        if (nextRoom != room)
        {
            room->remove(this);
            nextRoom->add(this);
        }
    }
};


struct ViewTarget : Object
{
    ViewTarget(Room* room) : Object(room) {}

    virtual void draw() {}
};


struct Waterfall : Object
{
    Waterfall(Room* room) : Object(room) {}

    virtual void draw() {}
};


struct LavaEmitter : Object
{
    LavaEmitter(Room* room) : Object(room) {}

    virtual void draw() {}
};


struct Door : Object
{
    enum {
        STATE_CLOSE,
        STATE_OPEN,
    };

    Door(Room* room) : Object(room)
    {
        flags.collision = true;
        action(true);
    }

    virtual void update()
    {
        if (isActive()) {
            if (state == STATE_CLOSE) {
                goalState = STATE_OPEN;
            } else {
                action(false);
            }
        } else {
            if (state == STATE_OPEN) {
                goalState = STATE_CLOSE;
            } else {
                action(true);
            }
        }

        animProcess();
    }

    virtual void collide(Lara* lara, CollisionInfo* cinfo)
    {
        // TODO door collision
    }

    void action(bool close)
    {
        vec3i nextPos = getBlockOffset(angle.y, 1);
        nextPos.x = pos.x + (nextPos.x << 10);
        nextPos.z = pos.z + (nextPos.z << 10);

        activate(close, false, room, nextPos.x, nextPos.z); // use the sector behind the door

        // TODO flip rooms
    }

    void activate(bool close, bool behind, Room* room, int32 x, int32 z)
    {
        room->modify(); // make room->sectors dynamic (non ROM)

        Sector* sector = (Sector*)room->getSector(x, z); // now we can modify room sectors

        Room* nextRoom;

        if (close) {
            nextRoom = sector->getNextRoom();

            sector->floorIndex = 0;
            sector->boxIndex   = NO_BOX;
            sector->roomBelow  = NO_ROOM;
            sector->floor      = NO_FLOOR;
            sector->roomAbove  = NO_ROOM;
            sector->ceiling    = NO_FLOOR;
        } else {
            *sector = room->data.sectors[sector - room->sectors];

            nextRoom = sector->getNextRoom();
        }

        // TODO modify level.boxes

        if (!behind && nextRoom) {
            activate(close, true, nextRoom, pos.x, pos.z); // use sector from item pos
        }
    }
};


struct TrapDoor : Object
{
    enum {
        STATE_CLOSE,
        STATE_OPEN,
    };

    TrapDoor(Room* room) : Object(room) {}

    virtual void update()
    {
        if (isActive()) {
            if (state == STATE_CLOSE) {
                goalState = STATE_OPEN;
            }
        } else {
            if (state == STATE_OPEN) {
                goalState = STATE_CLOSE;
            }
        }

        animProcess();
    }
};


struct Crystal : Object
{
    Crystal(Room* room) : Object(room)
    {
        activate();
    }

    virtual void collide(Lara* lara, CollisionInfo* cinfo)
    {
        collideDefault(lara, cinfo);
        // TODO
    }
};


struct Switch : Object
{
    enum {
        STATE_UP,
        STATE_DOWN,
    };

    Switch(Room* room) : Object(room) {}

    virtual void update()
    {
        flags.mask |= ITEM_FLAGS_MASK_ALL;
        if (!isActive())
        {
            goalState = STATE_DOWN;
            timer = 0;
        }
        Object::update();
    }

    virtual void collide(Lara* lara, CollisionInfo* cinfo)
    {
        if (lara->extraL->weaponState != WEAPON_STATE_FREE)
            return;

        if (!(lara->input & IN_ACTION))
            return;

        if (lara->state != Lara::STATE_STOP)
            return;

        if (flags.status != ITEM_FLAGS_STATUS_NONE)
            return;

        if (!checkLimit(lara, Limits::SWITCH))
            return;

        lara->angle.y = angle.y;

        ASSERT(state == STATE_DOWN || state == STATE_UP);

        bool isDown = (state == STATE_DOWN);

        goalState = isDown ? STATE_UP : STATE_DOWN;
        lara->animSkip(isDown ? Lara::STATE_SWITCH_DOWN : Lara::STATE_SWITCH_UP, Lara::STATE_STOP, true);
        activate();
        flags.status = ITEM_FLAGS_STATUS_ACTIVE;
        lara->extraL->weaponState = WEAPON_STATE_BUSY;
    }

    bool use(int32 t)
    {
        if (flags.status == ITEM_FLAGS_STATUS_INACTIVE)
        {
            if (t > 0 && state == Switch::STATE_UP)
            {
                if (t != 1) {
                    t *= 30;
                }
                timer = t;
                flags.status = ITEM_FLAGS_STATUS_ACTIVE;
            } else {
                deactivate();
                flags.status = ITEM_FLAGS_STATUS_NONE;
            }
            return true;
        }

        return false;
    }
};


struct SwitchWater : Switch
{
    SwitchWater(Room* room) : Switch(room) {}

    virtual void collide(Lara* lara, CollisionInfo* cinfo)
    {
        if (lara->extraL->weaponState != WEAPON_STATE_FREE)
            return;

        if (!(lara->input & IN_ACTION))
            return;

        if (lara->state != Lara::STATE_UW_TREAD)
            return;

        if (!checkLimit(lara, Limits::SWITCH_UW))
            return;

        if (!lara->moveTo(vec3i(0, 0, 108), this, true))
            return;

        lara->vSpeed = 0; // underwater speed
        goalState = (state == STATE_UP) ? STATE_DOWN : STATE_UP;

        lara->animSkip(Lara::STATE_SWITCH_DOWN, Lara::STATE_UW_TREAD, true);
        activate();
        flags.status = ITEM_FLAGS_STATUS_ACTIVE;

        //TODO TR2+
        //lara->weaponState = WEAPON_STATE_BUSY;
    }
};


struct Key : Object
{
    Key(Room* room) : Object(room) {}

    bool use(Item* lara)
    {
        if (flags.status == ITEM_FLAGS_STATUS_ACTIVE && lara->extraL->weaponState == WEAPON_STATE_FREE) // TODO check weapons
        {
            flags.status = ITEM_FLAGS_STATUS_INACTIVE;
            return true;
        }
        return false;
    }
};


struct Pickup : Object
{
    Pickup(Room* room) : Object(room)
    {
        frameIndex = 0;
    }

    bool use()
    {
        if (flags.status == ITEM_FLAGS_STATUS_INVISIBLE)
        {
            flags.status = ITEM_FLAGS_STATUS_INACTIVE;
            return true;
        }
        return false;
    }

    virtual void collide(Lara* lara, CollisionInfo* cinfo)
    {
        angle.y = lara->angle.y;
        angle.z = 0;

        if (lara->waterState == WATER_STATE_ABOVE || lara->waterState == WATER_STATE_WADE)
        {
            angle.x = 0;

            if (!checkLimit(lara, Limits::PICKUP))
                return;

            if (lara->state == Lara::STATE_PICKUP)
            {
                if (!lara->animIsEnd(23))
                    return;

                inventory.add((ItemType)type);
                gSaveGame.pickups++;
                room->remove(this);
                flags.status = ITEM_FLAGS_STATUS_INVISIBLE;
            }
            else if (lara->state == Lara::STATE_STOP)
            {
                if (lara->extraL->weaponState != WEAPON_STATE_FREE)
                    return;

                if (!(lara->input & IN_ACTION))
                    return;

                if (!lara->moveTo(vec3i(0, 0, -100), this, false))
                    return;

                lara->animSkip(Lara::STATE_PICKUP, Lara::STATE_STOP);
                lara->extraL->weaponState = WEAPON_STATE_BUSY;
            }
        }

        if (lara->waterState == WATER_STATE_UNDER)
        {
            angle.x = ANGLE(-25);

            if (!checkLimit(lara, Limits::PICKUP_UW))
                return;

            if (lara->state == Lara::STATE_PICKUP)
            {
                if (!lara->animIsEnd(14))
                    return;

                inventory.add((ItemType)type);
                gSaveGame.pickups++;
                room->remove(this);
                flags.status = ITEM_FLAGS_STATUS_INVISIBLE;
            }
            else if (lara->state == Lara::STATE_UW_TREAD)
            {
                // TODO TR2+
                //if (lara->weaponState != WEAPON_STATE_FREE)
                //    return;

                if (!(lara->input & IN_ACTION))
                    return;

                if (!lara->moveTo(vec3i(0, -200, -350), this, true))
                    return;

                lara->animSkip(Lara::STATE_PICKUP, Lara::STATE_UW_TREAD);

                // TODO TR2+
                //lara->weaponState = WEAPON_STATE_BUSY; // TODO check CMD_EMPTY event
            }
        }
    }
};


bool useSwitch(Item* item, int32 timer)
{
    return ((Switch*)item)->use(timer);
}

bool useKey(Item* item, Item* lara)
{
    return ((Key*)item)->use(lara);
}

bool usePickup(Item* item)
{
    return ((Pickup*)item)->use();
}


vec3i tmpPos;

struct Hole : Object // parent class for KeyHole and PuzzleHole
{
    Hole(Room* room) : Object(room) {}

    void apply(int32 offset, Lara* lara, Lara::State stateUse)
    {
        if (lara->extraL->weaponState != WEAPON_STATE_FREE)
            return;

        if (flags.status != ITEM_FLAGS_STATUS_NONE)
            return;

        if (lara->state != Lara::STATE_STOP || lara->animIndex != Lara::ANIM_STAND_NORMAL)
            return;

        if (!(lara->input & IN_ACTION) && (inventory.useSlot == SLOT_MAX))
            return;

        if (!checkLimit(lara, Limits::HOLE))
            return;

        if (inventory.useSlot == SLOT_MAX)
        {
            if (inventory.numKeys > 0) {
                inventory.show(lara, this);
                return;
            }
        } else {
            if (inventory.applyItem(this))
            {
                lara->moveTo(vec3i(0, 0, offset), this, false);
                lara->animSkip(stateUse, Lara::STATE_STOP);
                lara->extraL->weaponState = WEAPON_STATE_BUSY;
                flags.status = ITEM_FLAGS_STATUS_ACTIVE;
                return;
            }

            tmpPos.x = ~lara->pos.x;
            inventory.useSlot = SLOT_MAX;
        }

        if (tmpPos != lara->pos)
        {
            tmpPos = lara->pos;
            soundPlay(SND_NO, lara->pos);
        }
    }
};


struct KeyHole : Hole
{
    KeyHole(Room* room) : Hole(room) {}

    virtual void collide(Lara* lara, CollisionInfo* cinfo)
    {
        apply(362, lara, Lara::STATE_USE_KEY);
    }
};


struct PuzzleHole : Hole
{
    PuzzleHole(Room* room) : Hole(room) {}

    virtual void collide(Lara* lara, CollisionInfo* cinfo)
    {
        if (lara->state == Lara::STATE_USE_PUZZLE)
        {
            if (!checkLimit(lara, Limits::HOLE))
                return;

            if (!lara->animIsEnd(28))
                return;

            switch (type) {
                case ITEM_PUZZLEHOLE_1 : type = ITEM_PUZZLEHOLE_DONE_1; break;
                case ITEM_PUZZLEHOLE_2 : type = ITEM_PUZZLEHOLE_DONE_2; break;
                case ITEM_PUZZLEHOLE_3 : type = ITEM_PUZZLEHOLE_DONE_3; break;
                case ITEM_PUZZLEHOLE_4 : type = ITEM_PUZZLEHOLE_DONE_4; break;
            }

            return;
        }

        apply(327, lara, Lara::STATE_USE_PUZZLE);
    }
};


struct TrapFloor : Object
{
    enum {
        STATE_STATIC,
        STATE_SHAKE,
        STATE_FALL,
        STATE_DOWN,
    };

    TrapFloor(Room* room) : Object(room) {}

    virtual void update()
    {
        switch (state)
        {
            case STATE_STATIC:
                if (getLara(pos)->pos.y != pos.y - 512)
                {
                    flags.status = ITEM_FLAGS_STATUS_NONE;
                    deactivate();
                    return;
                }
                goalState = STATE_SHAKE;
                break;
            case STATE_SHAKE:
                goalState = STATE_FALL;
                break;
            case STATE_FALL:
                if (goalState != STATE_DOWN)
                {
                    flags.gravity = true;
                }
                break;
        }

        animProcess();

        if (flags.status == ITEM_FLAGS_STATUS_INACTIVE)
        {
            deactivate();
            return;
        }

        updateRoom();

        if (state == STATE_FALL && pos.y >= roomFloor)
        {
            pos.y = roomFloor;
            vSpeed = 0;
            flags.gravity = false;
            goalState = STATE_DOWN;
        }
    }
};


struct TrapSwingBlade : Object
{
    enum {
        STATE_STATIC,
        STATE_BEGIN,
        STATE_SWING,
        STATE_END,
    };

    TrapSwingBlade(Room* room) : Object(room)
    {
        flags.shadow = true;
    }

    virtual void collide(Lara* lara, CollisionInfo* cinfo)
    {
        if (flags.status != ITEM_FLAGS_STATUS_ACTIVE)
            return;

        if (state != STATE_SWING)
            return;

        if (!updateHitMask(lara, cinfo))
            return;

        vec3i offsetPos = vec3i((rand_logic() - 0x4000) >> 8, -256 - (rand_logic() >> 6), (rand_logic() - 0x4000) >> 8);
        int32 offsetAngle = (rand_logic() - 0x4000) >> 3;
        lara->fxBlood(lara->pos + offsetPos, lara->angle.y + offsetAngle, lara->hSpeed);

        lara->health -= 100; // TODO TR2 50?
    }

    virtual void update()
    {
        if (isActive()) {
            if (state == STATE_STATIC) {
                goalState = STATE_SWING;
            }
        } else {
            if (state == STATE_SWING) {
                goalState = STATE_STATIC;
            }
        }

        animProcess();
    }
};


struct Dart : Object
{
    Dart(Room* room) : Object(room)
    {
        flags.shadow = true;
    }

    virtual void collide(Lara* lara, CollisionInfo* cinfo)
    {
        collideDefault(lara, cinfo);

        if (hitMask)
        {
            lara->fxBlood(pos, lara->angle.y, lara->hSpeed);
            lara->health -= 50;
        }
    }

    virtual void update()
    {
        animProcess();
        updateRoom();

        if (pos.y < roomFloor)
            return;

        remove();

        fxRicochet(room, pos, false);
    }
};


struct TrapDartEmitter : Object
{
    enum {
        STATE_IDLE,
        STATE_FIRE
    };

    TrapDartEmitter(Room* room) : Object(room) {}

    virtual void update()
    {
        goalState = isActive() ? STATE_FIRE : STATE_IDLE;

        if (state == STATE_IDLE && state == goalState)
        {
            deactivate();
            return;
        }

        if (state == STATE_FIRE && frameIndex == level.anims[animIndex].frameBegin)
        {
            vec3i p = getBlockOffset(angle.y, 412);
            p.y = -512;
            p += pos;

            Item* dart = Item::add(ITEM_DART, room, p, angle.y);

            if (dart)
            {
                soundPlay(SND_DART, p);

                dart->intensity = 0;
                dart->flags.status = ITEM_FLAGS_STATUS_ACTIVE;
                dart->activate();

                fxSmoke(p);
            }
        }

        animProcess();
    }
};


struct Block : Object
{
    enum {
        STATE_NONE,
        STATE_READY,
        STATE_PUSH,
        STATE_PULL,
    };

    Block(Room* room) : Object(room)
    {
        if (flags.status != ITEM_FLAGS_STATUS_INVISIBLE) {
            updateFloor(-1024);
        }
    }

    void updateFloor(int32 offset)
    {
        room->modify();
        
        Sector* sector = (Sector*)room->getSector(pos.x, pos.z);

        if (sector->floor == NO_FLOOR) {
            sector->floor = sector->ceiling + (offset >> 8); 
        } else {
            sector->floor += (offset >> 8);
            if (sector->floor == sector->ceiling) {
                sector->floor = NO_FLOOR;
            }
        }

        // TODO modify level.boxes
    }

    bool checkBlocking()
    {
        const Sector* sector = room->getSector(pos.x, pos.z);

        return (sector->floor == NO_FLOOR) || ((sector->floor << 8) + 1024 == pos.y);
    }

    bool checkObstacles(int32 x, int32 z, int32 height)
    {
        Room* nextRoom = room->getRoom(x, pos.y, z);
        const Sector* sector = nextRoom->getSector(x, z);

        int32 floor = pos.y;
        int32 ceiling = pos.y - height;

        if ((sector->floor << 8) != floor)
            return false;

        nextRoom = nextRoom->getRoom(x, ceiling, z);
        sector = nextRoom->getSector(x, z);

        if ((sector->ceiling << 8) > ceiling)
            return false;

        return true;
    }

    bool checkPush()
    {
        if (!checkBlocking())
            return false;

        vec3i offset = getBlockOffset(angle.y, -1024);

        return checkObstacles(pos.x + offset.x, pos.z + offset.z, 1024);
    }

    bool checkPull(Item* lara)
    {
        if (!checkBlocking())
            return false;

        vec3i offset = getBlockOffset(angle.y, 1024);

        if (!checkObstacles(pos.x + offset.x, pos.z + offset.z, 1024))
            return false;

        return checkObstacles(lara->pos.x + offset.x, lara->pos.z + offset.z, LARA_HEIGHT);
    }

    virtual void collide(Lara* lara, CollisionInfo* cinfo)
    {
        if (lara->extraL->weaponState != WEAPON_STATE_FREE)
            return;

        if (!(lara->input & IN_ACTION))
            return;

        if (flags.status == ITEM_FLAGS_STATUS_ACTIVE)
            return;

        if (lara->pos.y != pos.y)
            return;

        uint16 quadrant = uint16(lara->angle.y + ANGLE_45) / ANGLE_90;

        if (lara->state == Lara::STATE_BLOCK_READY)
        {
            if (!lara->animIsEnd(0))
                return;

            if (!checkLimit(lara, Limits::BLOCK))
                return;

            if (lara->input & IN_UP)
            {
                if (!checkPush())
                    return;

                lara->goalState = Lara::STATE_BLOCK_PUSH;
                goalState = STATE_PUSH;
            }
            else if (lara->input & IN_DOWN)
            {
                if (!checkPull(lara))
                    return;

                lara->goalState = Lara::STATE_BLOCK_PULL;
                goalState = STATE_PULL;
            }
            else
            {
                return;
            }

            updateFloor(1024);

            activate();
            flags.status = ITEM_FLAGS_STATUS_ACTIVE;

            animProcess();
            lara->animProcess();
        }

        if (lara->state == Lara::STATE_STOP)
        {
            if (lara->input & (IN_UP | IN_DOWN))
                return;

            angle.y = quadrant * ANGLE_90;

            if (!checkLimit(lara, Limits::BLOCK))
                return;

            lara->goalState = Lara::STATE_BLOCK_READY;
            lara->angle.y = angle.y;
            lara->alignWall(LARA_RADIUS);
            lara->animProcess();
        }
    }

    virtual void update()
    {
        if (flags.once)
        {
            updateFloor(1024);
            remove();
            return;
        }

        animProcess();

        updateRoom(); // it'll get roomFloor and gLastFloorData

        if (pos.y > roomFloor)
        {
            flags.gravity = true;
        }
        else if (flags.gravity)
        {
            flags.gravity = false;
            flags.status = ITEM_FLAGS_STATUS_INACTIVE;
            pos.y = roomFloor;
            // TODO EarthQuake + playSound 70 (Thor room)
        }

        if (flags.status == ITEM_FLAGS_STATUS_INACTIVE)
        {
            deactivate();
            flags.status = ITEM_FLAGS_STATUS_NONE;

            updateFloor(-1024);

            checkTrigger(gLastFloorData, NULL);
        }
    }
};

#endif
