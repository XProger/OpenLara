#ifndef H_ENEMY
#define H_ENEMY

#include "common.h"
#include "item.h"

#define ENEMY_TURN_1    ANGLE(1)
#define ENEMY_TURN_2    ANGLE(2)
#define ENEMY_TURN_4    ANGLE(4)
#define ENEMY_TURN_5    ANGLE(5)

EWRAM_DATA ExtraInfoEnemy enemiesExtra[MAX_ENEMIES];

enum AggressionLevel
{
    AGGRESSION_LVL_1   = 0x400,
    AGGRESSION_LVL_2   = 0x2000,
    AGGRESSION_LVL_3   = 0x4000,
    AGGRESSION_LVL_MAX = 0x7FFF
};

struct Enemy : ItemObj
{
    Enemy(Room* room, int32 _health, int32 _radius, int32 _headOffset, int32 _aggression) : ItemObj(room)
    {
        flags |= ITEM_FLAG_SHADOW;

    #ifndef STATIC_ITEMS
        angle.y += (rand_logic() - 0x4000) >> 1;
    #endif

        ASSERT(_radius < 512);

        health = _health;
        radius = _radius >> 1;
        headOffset = _headOffset;
        aggression = _aggression;
    }

    void setExtra(ExtraInfoEnemy* extra)
    {
        ASSERT(!extraE);
        ASSERT(extra);

        if (extra->enemy)
        {
            extra->enemy->flags &= ~ITEM_FLAG_STATUS;
            extra->enemy->flags |= ITEM_FLAG_STATUS_INVISIBLE;
            extra->enemy->disable();
        }

        extra->enemy = this;
        extra->rotHead = extra->rotNeck = 0;
        extra->maxTurn = 0;
        extra->nav.stepHeight = 256;
        extra->nav.dropHeight = -256;
        extra->nav.vSpeed = 0;
        extraE = extra;

        initExtra();

        const Sector* sector = room->getSector(pos.x, pos.z);
        ASSERT(sector->boxIndex != NO_BOX);
        extraE->nav.init(sector->boxIndex);
    }

    bool enable(bool forced)
    {
        if (extraE)
            return true;

        int32 index = -1;

        for (int32 i = 0; i < MAX_ENEMIES; i++)
        {
            if (!enemiesExtra[i].enemy)
            {
                index = i;
                break;
            }
        }

        if (index == -1)
        {
            for (int32 i = 0; i < MAX_PLAYERS; i++)
            {
                if (!players[i])
                    continue;

                vec3i &viewPos = players[i]->extraL->camera.view.pos;

                int32 maxDistQ = 0;

                if (!forced)
                {
                    vec3i d = pos - viewPos;
                    maxDistQ = X_SQR(d.x >> 8) + X_SQR(d.y >> 8) + X_SQR(d.z >> 8);
                }

                for (int32 j = 0; j < MAX_ENEMIES; j++)
                {
                    vec3i d = enemiesExtra[j].enemy->pos - viewPos;
                    int32 distQ = X_SQR(d.x >> 8) + X_SQR(d.y >> 8) + X_SQR(d.z >> 8);
                    if (distQ > maxDistQ)
                    {
                        maxDistQ = distQ;
                        index = j;
                    }
                }
            }
        }

        if (index == -1)
            return false;

        setExtra(enemiesExtra + index);
        return true;
    }

    void disable()
    {
        ASSERT(extraE);
        extraE->enemy = NULL;
        extraE = NULL;
    }

    void updateTargetInfo()
    {
        tinfo.target = getLara(pos);

        tinfo.tilt = 0;
        tinfo.turn = 0;
        tinfo.pos = pos;

        tinfo.dist = 16 * 1024;
        tinfo.angle = 0;
        tinfo.front = false;
        tinfo.behind = false;
        tinfo.canAttack = false;
        tinfo.rotHead = 0;

        if (health <= 0)
            return;

        // update navigation target
        const uint16* zones = getZones();

        tinfo.boxIndex = room->getSector(pos.x, pos.z)->boxIndex;
        tinfo.boxIndexTarget = tinfo.target->room->getSector(tinfo.target->pos.x, tinfo.target->pos.z)->boxIndex;
        tinfo.zoneIndex = zones[tinfo.boxIndex];
        tinfo.zoneIndexTarget = zones[tinfo.boxIndexTarget];

        //@TODO blocking

        if (tinfo.target->health <= 0)
        {
            hitMask = 0;
            return;
        }

        int32 dx = tinfo.target->pos.x - pos.x;
        int32 dz = tinfo.target->pos.z - pos.z;

        if (headOffset)
        {
            int32 s, c;
            sincos(angle.y, s, c);
            dx -= s * headOffset >> FIXED_SHIFT;
            dz -= c * headOffset >> FIXED_SHIFT;
        }

        int16 rot = phd_atan(dz, dx);

        tinfo.dist = fastLength(dx, dz);
        tinfo.angle = rot - angle.y;
        tinfo.front = abs(tinfo.angle) < ANGLE_90;
        tinfo.behind = abs(rot - tinfo.target->angle.y - ANGLE_180) < ANGLE_90;
        tinfo.canAttack = tinfo.front && (abs(tinfo.target->pos.y - pos.y) < 256);
        tinfo.rotHead = (tinfo.front && mood != MOOD_SLEEP) ? tinfo.angle : 0;
    }

    void updateMood()
    {
        //if (extraE->nav.cells[extraE->boxIndex].weight == ) //@TODO blocking

        if (mood != MOOD_ATTACK && extraE->nav.nextBox != NO_BOX && !checkZone(extraE->nav.endBox))
        {
            bool inZone = checkZone();
            if (!inZone) {
                mood = MOOD_SLEEP;
            }
            extraE->nav.nextBox = NO_BOX;
        }

        uint32 nextMood = tinfo.target->health <= 0 ? MOOD_SLEEP : ((flags & ITEM_FLAG_ANIMATED) ? getMoodWild() : getMoodNormal());

        flags &= ~ITEM_FLAG_INJURED;

        if (mood != nextMood)
        {
            if (mood == MOOD_ATTACK) {
                setNextBox(extraE->nav.endBox);
            }
            extraE->nav.nextBox = NO_BOX;
            mood = nextMood;
        }

        switch (mood)
        {
            case MOOD_SLEEP  : moodSleep();  break;
            case MOOD_STALK  : moodStalk();  break;
            case MOOD_ATTACK : moodAttack(); break;
            case MOOD_ESCAPE : moodEscape(); break;
        }

        if (extraE->nav.endBox == NO_BOX) {
            setNextBox(tinfo.boxIndex);
        }
    }

    void moodSleep()
    {
        int32 boxIndex = getRandomBox();

        if (!checkZone(boxIndex))
            return;

        if (checkStalk(boxIndex))
        {
            mood = MOOD_STALK;
            setNextBox(boxIndex);
            return;
        }

        if (extraE->nav.nextBox != NO_BOX)
            return;

        setNextBox(boxIndex);
    }

    void moodStalk()
    {
        if (extraE->nav.nextBox != NO_BOX && checkStalk(extraE->nav.nextBox))
            return;

        int32 boxIndex = getRandomBox();

        if (!checkZone(boxIndex))
            return;

        if (checkStalk(boxIndex))
        {
            setNextBox(boxIndex);
            return;
        }

        if (extraE->nav.nextBox != NO_BOX)
            return;

        setNextBox(boxIndex);

        if (tinfo.zoneIndex != tinfo.zoneIndexTarget) {
            mood = MOOD_SLEEP;
        }
    }

    void moodAttack()
    {
        if (rand_logic() >= aggression)
            return;

        extraE->nav.pos = tinfo.target->pos;
        extraE->nav.nextBox = tinfo.boxIndexTarget;

        if (extraE->nav.zoneType == ZONE_FLY) {
            extraE->nav.pos.y += tinfo.target->getFrame()->box.minY; // attack Laras head
        }
    }

    void moodEscape()
    {
        int32 boxIndex = getRandomBox();

        if (!checkZone(boxIndex) || (extraE->nav.nextBox != NO_BOX))
            return;

        if (checkEscape(boxIndex))
        {
            setNextBox(boxIndex);
            return;
        }

        if ((tinfo.zoneIndex != tinfo.zoneIndexTarget) || !checkStalk(boxIndex))
            return;

        mood = MOOD_STALK;
        setNextBox(boxIndex);
    }

    void updateNavigation()
    {
        tinfo.waypoint = extraE->nav.getWaypoint(tinfo.boxIndex, pos);

        int16 maxTurn = extraE->maxTurn;

        if (health <= 0 || !hSpeed || !maxTurn)
            return;

        int32 dx = tinfo.waypoint.x - pos.x;
        int32 dz = tinfo.waypoint.z - pos.z;
        int16 turn = phd_atan(dz, dx) - angle.y;
        int32 r = (hSpeed << FIXED_SHIFT) / maxTurn; //@DIV
        int32 d = fastLength(dx, dz);
        bool aim = (turn > -ANGLE_90) && (turn < ANGLE_90);

        if (!aim && (d < r))
            maxTurn >>= 1;

        tinfo.turn = X_CLAMP(turn, -maxTurn, maxTurn);
    }

    void setNextBox(int16 boxIndex)
    {
        boxIndex &= 0x7FFF;
        const Box* box = level.boxes + boxIndex;

        int32 bMinX = (box->minX << 10) + 512;
        int32 bMaxX = (box->maxX << 10) - 512;
        int32 bMinZ = (box->minZ << 10) + 512;
        int32 bMaxZ = (box->maxZ << 10) - 512;
    
        extraE->nav.nextBox = boxIndex;
        extraE->nav.pos.x = bMinX + RAND_LOGIC(bMaxX - bMinX);
        extraE->nav.pos.z = bMinZ + RAND_LOGIC(bMaxZ - bMinZ);
        extraE->nav.pos.y = box->floor;
        if (extraE->nav.zoneType != ZONE_FLY)
            extraE->nav.pos.y -= 384;
    }

    uint32 getMoodWild()
    {
        bool inZone = checkZone();

        if (mood == MOOD_SLEEP || mood == MOOD_STALK)
            return inZone ? MOOD_ATTACK : ((flags & ITEM_FLAG_INJURED) ? MOOD_ESCAPE : mood);

        if (mood == MOOD_ATTACK)
            return inZone ? mood : MOOD_SLEEP;

        return inZone ? MOOD_ATTACK : mood;
    }

    uint32 getMoodNormal()
    {
        bool inZone = checkZone();

        if (mood == MOOD_SLEEP || mood == MOOD_STALK)
        {
            if ((flags & ITEM_FLAG_INJURED) && (rand_logic() < 0x800 || !inZone))
                return MOOD_ESCAPE;
            if (inZone)
                return ((tinfo.dist < 3072) || (mood == MOOD_STALK && extraE->nav.nextBox == NO_BOX)) ? MOOD_ATTACK : MOOD_STALK;
            return mood;
        }
         
        if (mood == MOOD_ATTACK)
            return ((flags & ITEM_FLAG_INJURED) && rand_logic() < 0x800) ? MOOD_ESCAPE : (!inZone ? MOOD_SLEEP : mood);

        return (inZone && rand_logic() < 0x100) ? MOOD_STALK : mood;
    }

    X_INLINE bool checkZone()
    {
        return tinfo.zoneIndex == tinfo.zoneIndexTarget;
    }

    X_INLINE int32 getRandomBox()
    {
        return extraE->nav.cells[RAND_LOGIC(extraE->nav.cellsCount)].boxIndex;
    }

    bool checkZone(int16 boxIndex)
    {
        if (getZones()[boxIndex] != tinfo.zoneIndex)
            return false;

        const Box &b = level.boxes[boxIndex];

        //@TODO blocking

        return (pos.x <= (b.minX << 10)) || (pos.x >= ((b.maxX << 10) - 1)) || 
               (pos.z <= (b.minZ << 10)) || (pos.z >= ((b.maxZ << 10) - 1));
    }

    bool checkStalk(int16 boxIndex)
    {
        const Box &b = level.boxes[boxIndex];

        int32 dx = (((b.minX + b.maxX) << 10) >> 1) - tinfo.target->pos.x;
        int32 dz = (((b.minZ + b.maxZ) << 10) >> 1) - tinfo.target->pos.z;

        if (X_MAX(abs(dx), abs(dz)) > 3072)
            return false;

        int32 qLara = (tinfo.target->angle.y >> 14) + 2;
        int32 qBox = (dz > 0) ? ((dx > 0) ? 2 : 1) : ((dx > 0) ? 3 : 0);

        if (qLara == qBox) // Lara looks at this box
            return false;

        if (abs(qLara - qBox) != 2) // Lara looks in the opposite direction
            return true;

        dx = pos.x - tinfo.target->pos.x;
        dz = pos.z - tinfo.target->pos.z;

        int32 qEnemy = (dz > 0) ? ((dx > 0) ? 2 : 1) : ((dx > 0) ? 3 : 0);

        return qLara != qEnemy; // Lara looks at enemy
    }

    bool checkEscape(int16 boxIndex)
    {
        const Box &b = level.boxes[boxIndex];

        int32 dx = (((b.minX + b.maxX) << 10) >> 1) - tinfo.target->pos.x;
        int32 dz = (((b.minZ + b.maxZ) << 10) >> 1) - tinfo.target->pos.z;

        if (X_MAX(abs(dx), abs(dz)) < 5120)
            return false;

        int32 px = pos.x - tinfo.target->pos.x;
        int32 pz = pos.z - tinfo.target->pos.z;

        return !(((dx ^ px) & 0x80000000) && ((dz ^ pz) & 0x80000000));
    }

    const uint16* getZones()
    {
        return level.zones[gSaveGame.flipped][extraE->nav.zoneType];
    }

    void bite(int32 damage, const vec3i &offset, int32 joint)
    {
        if (!tinfo.target)
            return;
        
        vec3i fxPos = pos + getJoint(joint, offset);
        fxBlood(fxPos, 0, 0);
        
        tinfo.target->hit(damage, offset, 0);
    }

    void updateLocation()
    {
        Room* oldRoom = room;

        updateRoom();

        int32 h = pos.y - roomFloor; 
        bool badPos = (h > extraE->nav.stepHeight) || (h < extraE->nav.dropHeight);

        if (!badPos)
        {
            if (extraE->nav.zoneType == ZONE_FLY)
            {
                int32 dy = X_CLAMP(tinfo.waypoint.y - pos.y, -extraE->nav.vSpeed, extraE->nav.vSpeed);
                int32 y = pos.y + dy;

                const Sector* sector = room->getSector(pos.x, pos.z);

                int32 floor = sector->getFloor(pos.x, y, pos.z);
                if (y > floor)
                {
                    if (pos.y > floor)
                    {
                        pos.x = tinfo.pos.x;
                        pos.z = tinfo.pos.z;
                        dy = -extraE->nav.vSpeed; // go up
                    } else {
                        pos.y = floor;
                        dy = 0;
                    }
                }

                int32 ceiling = sector->getCeiling(pos.x, y, pos.z);
                if (y < ceiling)
                {
                    if (pos.y < ceiling)
                    {
                        pos.x = tinfo.pos.x;
                        pos.z = tinfo.pos.z;
                        dy = extraE->nav.vSpeed; // go down
                    } else {
                        pos.y = ceiling;
                        dy = 0;
                    }
                }

                // update vertical position
                pos.y += dy;

                // update pitch
                int32 pitch = 0;
                if (hSpeed) {
                    pitch = phd_atan(hSpeed, -dy);
                }
                angle.x = angleLerp(angle.x, pitch, ANGLE_1);

                updateRoom();
            } else {
                if (pos.y > roomFloor) {
                    pos.y = roomFloor;
                } else if (pos.y < roomFloor - 64) {
                    pos.y += 64;
                } else {
                    pos.y = roomFloor;
                }
            }
        }

        if (!badPos) // check for enemy vs enemy collision
        {
            ItemObj* item = room->firstItem;
            while (item)
            {
                if (item->type != ITEM_LARA && item != this && item->health > 0 && ((item->flags & ITEM_FLAG_STATUS) == ITEM_FLAG_STATUS_ACTIVE))
                {
                    int32 dx = item->pos.x - pos.x;
                    int32 dz = item->pos.z - pos.z;
                    int32 d = fastLength(dx, dz);

                    if ((d < (radius << 1)) && hSpeed)
                    {
                        tinfo.turn = ANGLE(8);
                        badPos = true;
                        break;
                    }
                }
                item = item->nextItem;
            }
        }

        if (!badPos) // check for wrong zone or wall
        {
            const uint16* zones = getZones();
            uint32 boxIndex = room->getSector(pos.x, pos.z)->boxIndex;
            badPos = (boxIndex == NO_BOX) || (tinfo.zoneIndex != zones[boxIndex]);
        }

        if (badPos)
        {
            angle.y += tinfo.turn; // apply turn again for double rotation speed if blocked by something
            pos = tinfo.pos;

            if (room != oldRoom)
            {
                room->remove(this);
                oldRoom->add(this);
            }
        }

    #ifdef _DEBUG
        {// TODO investigate enemies respawn
            const Sector* sector = room->getSector(pos.x, pos.z);
            ASSERT(sector->boxIndex != NO_BOX);
        }
    #endif
    }

    virtual void activate()
    {
        if (health <= 0)
            return;

        ItemObj::activate();

        if (!enable(!(flags & ITEM_FLAG_STATUS))) {
            flags &= ~ITEM_FLAG_STATUS;
            flags |= ITEM_FLAG_STATUS_INVISIBLE;
        }
    }

    virtual void hit(int32 damage, const vec3i &point, int32 soundId)
    {
        ASSERT(health > 0);

        health -= damage;

        if (health > 0)
        {
            if (soundId) {
                soundPlay(soundId, &pos);
            }
        } else {
            gSaveGame.kills++;
        #ifdef HIDE_CORPSES
            corpseTimer = HIDE_CORPSES;
        #endif
        }

        if (type != ITEM_MUMMY) {
            fxBlood(point, 0, 0);
        }

        flags |= ITEM_FLAG_INJURED;
    }

    virtual void collide(Lara* lara, CollisionInfo* cinfo)
    {
        if (!extraE) // disabled
            return;

        if (!updateHitMask(lara, cinfo))
            return;

        if (!cinfo->enemyPush)
            return;
        
        int32 r = cinfo->radius;
        cinfo->radius = 0;
        collidePush(lara, cinfo, true);
        cinfo->radius = r;
    }

    virtual void update()
    {
        if ((flags & ITEM_FLAG_STATUS) == ITEM_FLAG_STATUS_INVISIBLE)
        {
            if (!enable(false))
                return;
            flags &= ~ITEM_FLAG_STATUS;
            flags |= ITEM_FLAG_STATUS_ACTIVE;
        }

    #ifdef HIDE_CORPSES
        if ((health <= 0) && !corpseTimer--)
        {
            deactivate();
            flags &= ~ITEM_FLAG_STATUS;
            flags |= ITEM_FLAG_STATUS_INVISIBLE;
        }
    #endif

        if (!extraE)
            return;

        updateTargetInfo();

        if (health > 0)
        {
            updateMood();
            updateNavigation();
        }

        goalState = updateState();

        animProcess();

        updateLocation();

        extraE->rotHead = angleLerp(extraE->rotHead, tinfo.rotHead, ANGLE(5));
        angle.z += X_CLAMP((tinfo.tilt << 2) - angle.z, -ANGLE(3), ANGLE(3));
        angle.y += tinfo.turn;

        if ((flags & ITEM_FLAG_STATUS) == ITEM_FLAG_STATUS_INACTIVE)
        {
            flags &= ~ITEM_FLAG_COLLISION;
            health = NOT_ENEMY;
            disable();
        #ifndef HIDE_CORPSES
            deactivate();
        #endif
        }
    }

    virtual int32 updateState()
    {
        return goalState;
    }

    virtual void initExtra()
    {
        // empty
    }

    struct EnemySave {
        int16 vSpeed;
        int16 hSpeed;
        int16 health;
        uint8 mood;
        uint8 waterState;
    };

    virtual uint8* save(uint8* data)
    {
        data = ItemObj::save(data);
        
        EnemySave* sg = (EnemySave*)data;

        sg->vSpeed = vSpeed;
        sg->hSpeed = hSpeed;
        sg->health = health;
        sg->mood = uint8(mood);
        sg->waterState = waterState;

        return data + sizeof(EnemySave);
    }

    virtual uint8* load(uint8* data)
    {
        data = ItemObj::load(data);

        EnemySave* sg = (EnemySave*)data;

        vSpeed = sg->vSpeed;
        hSpeed = sg->hSpeed;
        health = sg->health;
        mood = sg->mood;
        waterState = sg->waterState;

        return data + sizeof(EnemySave);
    }
};


struct Doppelganger : Enemy
{
    Doppelganger(Room* room) : Enemy(room, LARA_MAX_HEALTH, 10, 0, 0) {}
};


struct Wolf : Enemy
{
    enum {
        HIT_MASK = 0x774F,  // body, head, front legs

        DIST_STALK = 1023 * 3,
        DIST_BITE = 345,
        DIST_ATTACK = 1024 + 512,

        DAMAGE_BITE = 100,
        DAMAGE_ATTACK = 50,

        ANIM_DEATH = 20,
        ANIM_DEATH_RUN = 21,
        ANIM_DEATH_JUMP = 22
    };

    enum {
        STATE_NONE,
        STATE_STOP,
        STATE_WALK,
        STATE_RUN,
        STATE_JUMP, // unused
        STATE_STALK,
        STATE_ATTACK,
        STATE_HOWL,
        STATE_SLEEP,
        STATE_GROWL,
        STATE_TURN, // unused
        STATE_DEATH,
        STATE_BITE
    };

    Wolf(Room* room) : Enemy(room, 6, 341, 375, AGGRESSION_LVL_2)
    {
        frameIndex = level.anims[animIndex].frameEnd - 1;
    }

    virtual void hit(int32 damage, const vec3i &point, int32 soundId)
    {
        Enemy::hit(damage, point, SND_HIT_WOLF);

        if (health <= 0) {
            animSet(level.models[type].animIndex + ANIM_DEATH + RAND_LOGIC(3), true);
        }
    }

    virtual int32 updateState()
    {
        if (health <= 0)
            return goalState;

        switch (state)
        {
            case STATE_STOP:
            {
                if (nextState)
                    return nextState;
                return STATE_WALK;
            }

            case STATE_WALK:
            {
                extraE->maxTurn = ENEMY_TURN_2;
                tinfo.tilt = tinfo.turn >> 1;

                if (mood != MOOD_SLEEP) {
                    nextState = STATE_NONE;
                    return STATE_STALK;
                }
                if (rand_logic() < 0x20) {
                    nextState = STATE_SLEEP;
                    return STATE_STOP;
                }
                break;
            }

            case STATE_RUN:
            {
                extraE->maxTurn = ENEMY_TURN_5;
                tinfo.tilt = tinfo.turn;

                if (tinfo.front && tinfo.dist < DIST_ATTACK)
                {
                    if (tinfo.dist <= (DIST_ATTACK >> 1) && !tinfo.behind)
                    {
                        nextState = STATE_NONE;
                        return STATE_ATTACK;
                    }
                    nextState = STATE_STALK;
                    return STATE_GROWL;
                }
                if (mood == MOOD_STALK && tinfo.dist < DIST_STALK)
                {
                    nextState = STATE_STALK;
                    return STATE_GROWL;
                }
                if (mood == MOOD_SLEEP)
                    return STATE_GROWL;
                break;
            }

            case STATE_STALK:
            {
                extraE->maxTurn = ENEMY_TURN_2;

                if (mood == MOOD_ESCAPE)
                    return STATE_RUN;
                if ((tinfo.dist < DIST_BITE) && tinfo.canAttack && (tinfo.target->health > 0))
                    return STATE_BITE;
                if (tinfo.dist > DIST_STALK)
                    return STATE_RUN;
                if (mood == MOOD_ATTACK)
                {
                    if (!tinfo.front || (tinfo.dist > DIST_ATTACK) || tinfo.behind)
                        return STATE_RUN;
                    break;
                }
                if (rand_logic() < 0x180)
                {
                    nextState = STATE_HOWL;
                    return STATE_GROWL;
                }
                if (mood == MOOD_SLEEP)
                    return STATE_GROWL;
                break;
            }

            case STATE_SLEEP:
            {
                if ((mood == MOOD_ESCAPE) || checkZone())
                    nextState = STATE_GROWL;
                else if (rand_logic() < 0x20)
                    nextState = STATE_WALK;
                else
                    break;
                return STATE_STOP;
            }

            case STATE_GROWL:
            {
                if (nextState)
                    return nextState;
                if (mood == MOOD_ESCAPE)
                    return STATE_RUN;
                if (tinfo.dist < DIST_BITE && tinfo.canAttack && (tinfo.target->health > 0))
                    return STATE_BITE;
                if (mood == MOOD_STALK)
                    return STATE_STALK;
                if (mood == MOOD_SLEEP)
                    return STATE_STOP;
                return STATE_RUN;
            }

            case STATE_ATTACK:
            {
                tinfo.tilt = tinfo.turn;

                if (!nextState && (hitMask & HIT_MASK)) {
                    bite(DAMAGE_ATTACK, _vec3i(0, -14, 174), 6);
                    nextState = STATE_RUN;
                }
                return STATE_RUN;
            }

            case STATE_BITE:
            {
                if (!nextState && (hitMask & HIT_MASK)) {
                    bite(DAMAGE_BITE, _vec3i(0, -14, 174), 6);
                    nextState = STATE_GROWL;
                }
                return STATE_RUN;
            }
        }

        return goalState;
    }

    virtual void initExtra()
    {
        extraE->nav.stepHeight = 256;
        extraE->nav.dropHeight = -1024;
    }
};


struct Bear : Enemy
{
    enum {
        HIT_MASK = 0x2406C, // front legs and head

        DAMAGE_FALL = 200,
        DAMAGE_BITE = 200,
        DAMAGE_ATTACK = 400
    };

    enum {
        STATE_NONE   = 0, // WTF?
        STATE_WALK   = 0,
        STATE_STOP   ,
        STATE_HIND   ,
        STATE_RUN    ,
        STATE_HOWL   ,
        STATE_GROWL  ,
        STATE_BITE   ,
        STATE_ATTACK ,
        STATE_EAT    ,
        STATE_DEATH
    };

    Bear(Room* room) : Enemy(room, 20, 341, 500, AGGRESSION_LVL_3)
    {
        flags |= ITEM_FLAG_ANIMATED; // TODO WILD flag
    }

    virtual void hit(int32 damage, const vec3i &point, int32 soundId)
    {
        Enemy::hit(damage, point, SND_HIT_BEAR);
    }

    virtual int32 updateState()
    {
        if (health <= 0)
        {
            switch (state)
            {
                case STATE_HIND:
                    return STATE_HOWL;
                case STATE_WALK:
                case STATE_RUN:
                    return STATE_STOP;
                case STATE_STOP:
                    flags &= ~ITEM_FLAG_REVERSE; // TODO special flag
                    return STATE_DEATH;
                case STATE_HOWL:
                    flags |= ITEM_FLAG_REVERSE;
                    return STATE_DEATH;
                case STATE_DEATH:
                    if ((flags & ITEM_FLAG_REVERSE) && (hitMask & HIT_MASK) && tinfo.target) // fall on Lara 
                    {
                        tinfo.target->hit(DAMAGE_FALL, _vec3i(0, 0, 0), 0);
                        flags &= ~ITEM_FLAG_REVERSE;
                    }
                    return STATE_DEATH;
                default:
                    return goalState;
            }
        }

        // hold the injured flag until death
        if (flags & ITEM_FLAG_INJURED) {
            flags |= ITEM_FLAG_REVERSE;
        } else {
            if (flags & ITEM_FLAG_REVERSE) {
                flags |= ITEM_FLAG_INJURED;
            }
        }

        switch (state)
        {
            case STATE_WALK:
            {
                extraE->maxTurn = ENEMY_TURN_2;

                if (tinfo.target->health <= 0 && tinfo.front && (hitMask & HIT_MASK))
                    return nextState = STATE_STOP; // eat lara! >:E

                if (mood != MOOD_SLEEP)
                {
                    if (mood == MOOD_ESCAPE) {
                        nextState = STATE_NONE;
                    }
                    return STATE_STOP;
                }
                
                if (rand_logic() < 0x50)
                {
                    nextState = STATE_GROWL;
                    return STATE_STOP;
                }
                break;
            }

            case STATE_STOP:
            {
                if (tinfo.target->health <= 0)
                    return (tinfo.canAttack && tinfo.dist < 768) ? STATE_EAT : STATE_WALK;
                if (nextState)
                    return nextState;
                return (mood == MOOD_SLEEP) ? STATE_WALK : STATE_RUN;
            }

            case STATE_HIND:
            {
                if (flags & ITEM_FLAG_INJURED) {
                    nextState = STATE_NONE;
                    return STATE_HOWL;
                }
                if (tinfo.front && (hitMask & HIT_MASK))
                    return STATE_HOWL;
                if (mood == MOOD_ESCAPE) {
                    nextState = STATE_NONE;
                    return STATE_HOWL;
                }
                if (mood == MOOD_SLEEP || rand_logic() < 0x50) {
                    nextState = STATE_GROWL;
                    return STATE_HOWL;
                }
                if (tinfo.dist > 2048 || rand_logic() < 0x600) {
                    nextState = STATE_STOP;
                    return STATE_HOWL;
                }
                return STATE_HOWL;
            }

            case STATE_RUN:
            {
                extraE->maxTurn = ENEMY_TURN_5;

                if (hitMask & HIT_MASK) {
                    tinfo.target->hit(3, pos, 0);
                    if (health >= 0) {
                        nextState = STATE_NONE;
                    }
                }

                if (tinfo.target->health <= 0 || mood == MOOD_SLEEP)
                    return STATE_STOP;

                if (!nextState && tinfo.front)
                {
                    if (tinfo.dist < 2048)
                    {
                        if (!(flags & ITEM_FLAG_INJURED) && (rand_logic() < 0x300))
                        {
                            nextState = STATE_HOWL;
                            return STATE_STOP;
                        }
                        if (tinfo.dist < 1024)
                            return STATE_BITE;
                    }
                }
                break;
            }

            case STATE_HOWL:
            {
                if (flags & ITEM_FLAG_INJURED)
                {
                    nextState = STATE_NONE;
                    return STATE_STOP;
                }

                if (nextState)
                    return nextState;

                if (mood == MOOD_SLEEP || mood == MOOD_ESCAPE)
                    return STATE_STOP;

                if (tinfo.canAttack && tinfo.dist < 600)
                    return STATE_ATTACK;

                return STATE_HIND;
            }

            case STATE_BITE:
            {
                if (!nextState && (hitMask & HIT_MASK)) {
                    bite(DAMAGE_BITE, _vec3i(0, 96, 335), 14);
                    nextState = STATE_STOP;
                }
                break;
            }

            case STATE_ATTACK:
            {
                if (!nextState && (hitMask & HIT_MASK) && tinfo.target) {
                    tinfo.target->hit(DAMAGE_ATTACK, _vec3i(0, 0, 0), 0);
                    nextState = STATE_HOWL;
                }
                break;
            }
        }

        return goalState;
    }
};


struct Bat : Enemy
{
    enum {
        DAMAGE_ATTACK = 2
    };

    enum {
        STATE_NONE,
        STATE_AWAKE,
        STATE_FLY,
        STATE_ATTACK,
        STATE_CIRCLING,
        STATE_DEATH
    };

    Bat(Room* room) : Enemy(room, 1, 102, 0, AGGRESSION_LVL_1) {}

    virtual int32 updateState()
    {
        if (health <= 0)
        {
            hSpeed = 0;
            if (pos.y < roomFloor) {
                flags |= ITEM_FLAG_GRAVITY;
            } else {
                flags &= ~ITEM_FLAG_GRAVITY;
            }

            if (flags & ITEM_FLAG_GRAVITY)
                return STATE_CIRCLING;

            pos.y = roomFloor;
            return STATE_DEATH;
        }

        switch (state)
        {
            case STATE_AWAKE:
            {
                return STATE_FLY;
            }

            case STATE_FLY:
            {
                if (hitMask)
                    return STATE_ATTACK;
                break;
            }

            case STATE_ATTACK:
            {
                if (!hitMask) {
                    mood = MOOD_SLEEP;
                    return STATE_FLY;
                }
                bite(DAMAGE_ATTACK, _vec3i(0, 16, 45), 4);
                break;
            }
        }

        return goalState;
    }

    virtual void initExtra()
    {
        extraE->nav.stepHeight = 20 * 1024;
        extraE->nav.dropHeight = -20 * 1024;
        extraE->nav.vSpeed = 16;
        extraE->maxTurn = ANGLE(20);
    }
};


struct Crocodile : Enemy
{
    Crocodile(Room* room) : Enemy(room, 20, 341, 600, AGGRESSION_LVL_2) {}

    virtual void initExtra()
    {
        if (type == ITEM_CROCODILE_LAND) {
            extraE->nav.stepHeight = 256;
            extraE->nav.dropHeight = -256;
        } else {
            extraE->nav.stepHeight = 20 * 1024;
            extraE->nav.dropHeight = -20 * 1024;
        }
    }
};


struct Lion : Enemy
{
    Lion(Room* room) : Enemy(room, 1, 341, 400, AGGRESSION_LVL_2)
    {
        switch (type)
        {
            case ITEM_LION_MALE   : health = 30; aggression = AGGRESSION_LVL_MAX; break;
            case ITEM_LION_FEMALE : health = 25; break;
            case ITEM_PUMA        : health = 40; break;
        }
    }

    virtual void hit(int32 damage, const vec3i &point, int32 soundId)
    {
        Enemy::hit(damage, point, (type == ITEM_PUMA) ? 0 : SND_HIT_LION);
    }

    virtual void initExtra()
    {
        extraE->nav.stepHeight = 256;
        extraE->nav.dropHeight = -1024;
    }
};


struct Gorilla : Enemy
{
    Gorilla(Room* room) : Enemy(room, 22, 341, 250, AGGRESSION_LVL_MAX) {}

    virtual void initExtra()
    {
        extraE->nav.stepHeight = 512;
        extraE->nav.dropHeight = -1024;
    }
};


struct Rat : Enemy
{
    Rat(Room* room) : Enemy(room, 5, 204, 200, AGGRESSION_LVL_2) {}

    virtual void hit(int32 damage, const vec3i &point, int32 soundId)
    {
        Enemy::hit(damage, point, SND_HIT_RAT);
    }
};


struct Rex : Enemy
{
    enum {
        HIT_MASK = 0x00003000,  // head
        DIST_BITE = 1500,
        DIST_WALK = 4096,
        DIST_RUN = 5120,
        DAMAGE_RUN = 10,
        DAMAGE_WALK = 1,
        DAMAGE_FATAL = 1000
    };

    enum {
        STATE_NONE,
        STATE_STOP,
        STATE_WALK,
        STATE_RUN,
        STATE_UNUSED,
        STATE_DEATH,
        STATE_ROAR,
        STATE_BITE,
        STATE_FATAL
    };

    Rex(Room* room) : Enemy(room, 100, 341, 2000, AGGRESSION_LVL_MAX) {}

    virtual int32 updateState()
    {
        if (health <= 0) {
            return (state == STATE_STOP) ? STATE_DEATH : STATE_STOP;
        }

        extraE->rotNeck = (extraE->rotHead >>= 1);

        if (hitMask)
        {
            tinfo.target->hit((state == STATE_RUN) ? DAMAGE_RUN : DAMAGE_WALK, _vec3i(0, 0, 0), 0);
        }

        bool walk = (tinfo.canAttack && (tinfo.dist > DIST_BITE) && (tinfo.dist < DIST_WALK)) ||
                    (tinfo.behind && !tinfo.front && (mood != MOOD_ESCAPE));

        switch (state)
        {
            case STATE_STOP:
            {
                if (nextState)
                    return nextState;
                if (tinfo.canAttack && (tinfo.dist < DIST_BITE))
                    return STATE_BITE;
                if ((mood == MOOD_SLEEP) || walk)
                    return STATE_WALK;
                return STATE_RUN;
            }

            case STATE_WALK:
            {
                extraE->maxTurn = ENEMY_TURN_2;
                if ((mood != MOOD_SLEEP) && !walk)
                    return STATE_STOP;
                if (tinfo.front && (rand_logic() < 0x200))
                {
                    nextState = STATE_ROAR;
                    return STATE_STOP;
                }
                break;
            }

            case STATE_RUN:
            {
                extraE->maxTurn = ENEMY_TURN_4;
                if (((tinfo.dist < DIST_RUN) && tinfo.canAttack) || walk)
                    return STATE_STOP;
                if ((mood != MOOD_ESCAPE) && tinfo.front && (rand_logic() < 0x200))
                {
                    nextState = STATE_ROAR;
                    return STATE_STOP;
                }
                if (mood == MOOD_SLEEP)
                    return STATE_STOP;
                break;
            }

            case STATE_BITE:
            {
                nextState = STATE_WALK;
                if (hitMask & HIT_MASK)
                {
                    angle.x = 0;
                    angle.z = 0;

                    tinfo.target->pos = pos;
                    tinfo.target->angle = angle;
                    tinfo.target->flags &= ~ITEM_FLAG_GRAVITY;
                    tinfo.target->hit(DAMAGE_FATAL, _vec3i(0, 0, 0), 0);
                    tinfo.target->meshSwap(ITEM_LARA_SPEC, 0xFFFFFFFF);
                    tinfo.target->animSet(level.models[ITEM_LARA_SPEC].animIndex + 1, true);

                    if (tinfo.target->room != room)
                    {
                        tinfo.target->room->remove(tinfo.target);
                        room->add(tinfo.target);
                    }

                    return STATE_FATAL;
                }
                break;
            }
        }

        return goalState;
    }
};


struct Raptor : Enemy
{
    enum {
        HIT_MASK = 0xFF7C00,  // hands and head

        DIST_BITE = 680,
        DIST_ATTACK = 1536,

        DAMAGE_ATTACK = 100,

        ANIM_DEATH_1 = 9,
        ANIM_DEATH_2 = 10
    };

    enum {
        STATE_NONE = 0,
        STATE_DEATH = 0,
        STATE_STOP,
        STATE_WALK,
        STATE_RUN,
        STATE_ATTACK_1,
        STATE_UNUSED,
        STATE_ROAR,
        STATE_ATTACK_2,
        STATE_BITE
    };

    Raptor(Room* room) : Enemy(room, 20, 341, 400, AGGRESSION_LVL_3) {}

    virtual void hit(int32 damage, const vec3i& point, int32 soundId)
    {
        Enemy::hit(damage, point, 0);

        if (health <= 0) {
            animSet(level.models[type].animIndex + ANIM_DEATH_1 + RAND_LOGIC(2), true);
        }
    }

    virtual int32 updateState()
    {
        if (health <= 0)
            return goalState;

        switch (state)
        {
            case STATE_STOP: {
                if (nextState)
                    return nextState;
                if ((hitMask & HIT_MASK) || (tinfo.canAttack && tinfo.dist < DIST_BITE))
                    return STATE_BITE;
                if (tinfo.canAttack && tinfo.dist < DIST_ATTACK)
                    return STATE_ATTACK_1;
                if (mood == MOOD_SLEEP)
                    return STATE_WALK;
                return STATE_RUN;
            }

            case STATE_WALK: {
                extraE->maxTurn = ENEMY_TURN_1;
                tinfo.tilt = tinfo.turn >> 1;

                if (mood != MOOD_SLEEP)
                    return STATE_STOP;

                if (rand_logic() < 0x100) {
                    nextState = STATE_ROAR;
                    return STATE_STOP;
                }
                break;
            }

            case STATE_RUN: {
                extraE->maxTurn = ENEMY_TURN_4;
                tinfo.tilt = tinfo.turn;

                if (hitMask & HIT_MASK)
                    return STATE_STOP;

                if (tinfo.canAttack && tinfo.dist < DIST_ATTACK)
                {
                    if (goalState != STATE_RUN)
                        break;
                    return (rand_logic() < 0x2000) ? STATE_STOP : STATE_ATTACK_2;
                }

                if (tinfo.front && (mood != MOOD_ESCAPE) && (rand_logic() < 0x100))
                {
                    nextState = STATE_ROAR;
                    return STATE_STOP;
                }

                if (mood == MOOD_SLEEP)
                    return STATE_STOP;
                break;
            }

            case STATE_ATTACK_1:
            case STATE_ATTACK_2:
            case STATE_BITE:
            {
                tinfo.tilt = tinfo.turn;
                if (!nextState && tinfo.front && (hitMask & HIT_MASK)) {
                    bite(DAMAGE_ATTACK, _vec3i(0, 66, 318), 22);
                    nextState = (state == STATE_ATTACK_2) ? STATE_RUN : STATE_STOP;
                }
                break;
            }
        }

        return goalState;
    }
};


struct Mutant : Enemy
{
    Mutant(Room* room) : Enemy(room, 50, 341, 150, AGGRESSION_LVL_MAX) {}
};


struct Centaur : Enemy
{
    Centaur(Room* room) : Enemy(room, 120, 341, 400, AGGRESSION_LVL_MAX) {}
};


struct Mummy : Enemy
{
    Mummy(Room* room) : Enemy(room, 18, 10, 0, AGGRESSION_LVL_MAX) {}
};


struct Larson : Enemy
{
    Larson(Room* room) : Enemy(room, 50, 102, 0, AGGRESSION_LVL_MAX) {}
};


struct Pierre : Enemy
{
    Pierre(Room* room) : Enemy(room, 70, 102, 0, AGGRESSION_LVL_MAX) {}
};


struct Skater : Enemy
{
    Skater(Room* room) : Enemy(room, 125, 204, 0, AGGRESSION_LVL_MAX) {}

    virtual void hit(int32 damage, const vec3i &point, int32 soundId)
    {
        Enemy::hit(damage, point, SND_HIT_SKATER);
    }
};


struct Cowboy : Enemy
{
    Cowboy(Room* room) : Enemy(room, 150, 102, 0, AGGRESSION_LVL_MAX) {}
};


struct MrT : Enemy
{
    MrT(Room* room) : Enemy(room, 200, 102, 0, AGGRESSION_LVL_MAX) {}
};


struct Natla : Enemy
{
    Natla(Room* room) : Enemy(room, 400, 204, 0, AGGRESSION_LVL_MAX) {}
};


struct Adam : Enemy
{
    Adam(Room* room) : Enemy(room, 500, 341, 0, AGGRESSION_LVL_MAX) {}

    virtual void hit(int32 damage, const vec3i &point, int32 soundId)
    {
        Enemy::hit(damage, point, SND_HIT_ADAM);
    }
};

#endif
