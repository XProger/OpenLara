#ifndef H_OBJECT
#define H_OBJECT

#include "item.h"
#include "lara.h"

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
    Box   box;
    vec3s angle;
};

namespace Limits
{
    static const Limit SWITCH = {
        Box( -200, 200, 0, 0, 312, 512 ),
        vec3s( 10 * DEG2SHORT, 30 * DEG2SHORT, 10 * DEG2SHORT )
    };
};


struct Object : Item
{
    Object(Room* room) : Item(room) {}

    virtual void update()
    {
        updateAnim();
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
        int16 ax = abs(lara->angleX - angleX);
        int16 ay = abs(lara->angleY - angleY);
        int16 az = abs(lara->angleZ - angleZ);

        if (ax > limit.angle.x || ay > limit.angle.y || az > limit.angle.z)
            return false;

        vec3i d = lara->pos - pos;

        matrixSetIdentity();
        matrixRotateZXY(-angleX, -angleY, -angleZ);
        const Matrix &m = matrixGet();

        vec3i p;
        p.x = DP33(m[0], d) >> FIXED_SHIFT;
        p.y = DP33(m[1], d) >> FIXED_SHIFT;
        p.z = DP33(m[2], d) >> FIXED_SHIFT;

        return boxContains(limit.box, p);
    }
};


struct ViewTarget : Object
{
    ViewTarget(Room* room) : Object(room) {}

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

        updateAnim();
    }

    virtual void collide(Lara* lara, CollisionInfo* cinfo)
    {
        UNUSED(lara);
        UNUSED(cinfo);
    }

    void action(bool close)
    {
        vec3i nextPos = getBlockOffset(angleY, 1);
        nextPos.x = pos.x + (nextPos.x << 10);
        nextPos.z = pos.z + (nextPos.z << 10);

        activate(close, false, room, nextPos.x, nextPos.z); // use the sector behind the door

        // TODO flip rooms
    }

    void activate(bool close, bool behind, Room* room, int32 x, int32 z)
    {
        room->modify(); // make room->sectors dynamic (non ROM)

        RoomInfo::Sector* sector = (RoomInfo::Sector*)room->getSector(x, z); // now we can modify room sectors

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
            *sector = room->sectorsOrig[sector - room->sectors];

            nextRoom = sector->getNextRoom();
        }

        if (!behind && nextRoom) {
            activate(close, true, nextRoom, pos.x, pos.z); // use sector from item pos
        }
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
        UNUSED(cinfo);

        if (!(lara->input & IN_ACTION))
            return;

        if (lara->state != Lara::STOP)
            return;

        if (flags.status != ITEM_FLAGS_STATUS_NONE)
            return;

        if (!checkLimit(lara, Limits::SWITCH))
            return;

        lara->angleY = angleY;

        ASSERT(state == STATE_DOWN || state == STATE_UP);

        if (state == STATE_DOWN) {
            lara->goalState = Lara::SWITCH_DOWN;
            goalState = STATE_UP;
        } else {
            lara->goalState = Lara::SWITCH_UP;
            goalState = STATE_DOWN;
        }

        flags.status = ITEM_FLAGS_STATUS_ACTIVE;
        activate();

        lara->skipAnim();

        updateAnim();

        lara->goalState = Lara::STOP;
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
        UNUSED(lara);
        UNUSED(cinfo);
        // TODO
    }
};


struct Key : Object
{
    Key(Room* room) : Object(room) {}

    bool use()
    {
        if (flags.status == ITEM_FLAGS_STATUS_ACTIVE) // TODO check weapons
        {
            flags.status = ITEM_FLAGS_STATUS_INACTIVE;
            return true;
        }
        return false;
    }
};


struct Pickup : Object
{
    Pickup(Room* room) : Object(room) {}

    bool use()
    {
        if (flags.status == ITEM_FLAGS_STATUS_INVISIBLE)
        {
            flags.status = ITEM_FLAGS_STATUS_INACTIVE;
            return true;
        }
        return false;
    }
};


bool useSwitch(Item* item, int32 timer)
{
    return ((Switch*)item)->use(timer);
}

bool useKey(Item* item)
{
    return ((Key*)item)->use();
}

bool usePickup(Item* item)
{
    return ((Pickup*)item)->use();
}


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

        updateAnim();

        if (flags.status == ITEM_FLAGS_STATUS_INACTIVE)
        {
            deactivate();
            return;
        }

        updateRoom();

        if (state == STATE_FALL && pos.y >= floor)
        {
            pos.y = floor;
            vSpeed = 0;
            flags.gravity = false;
            goalState = STATE_DOWN;
        }
    }
};


struct Dart : Object
{
    Dart(Room* room) : Object(room)
    {
        flags.shadow = true;

        soundPlay(SND_DART, pos);
        // TODO create smoke
    }

    virtual void update()
    {
        // TODO collide with Lara

        updateAnim();
        updateRoom();

        if (pos.y >= floor)
        {
            // TODO create spark
            remove();
        }
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

        if (state == STATE_FIRE && frameIndex == anims[animIndex].frameBegin)
        {
            vec3i p = getBlockOffset(angleY, 412);
            p.y = -512;
            p += pos;

            Item* dart = Item::add(ITEM_DART, room, p, angleY);

            if (dart)
            {
                dart->flags.status = ITEM_FLAGS_STATUS_ACTIVE;
                dart->activate();
            }
        }

        updateAnim();
    }
};

#endif
