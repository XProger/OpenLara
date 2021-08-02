#ifndef H_ENEMY
#define H_ENEMY

#include "common.h"
#include "item.h"

EWRAM_DATA ExtraInfoEnemy enemiesExtra[MAX_ENEMIES];

enum AggressionLevel
{
    AGGRESSION_LVL_1   = 0x400,
    AGGRESSION_LVL_2   = 0x2000,
    AGGRESSION_LVL_3   = 0x4000,
    AGGRESSION_LVL_MAX = 0x7FFF,
};

struct Enemy : Item
{
    Enemy(Room* room, int32 _health, int32 _radius, int32 _headOffset, int32 _aggression) : Item(room)
    {
        flags.shadow = true;

    #ifndef STATIC_ITEMS
        angle.y += (rand_logic() - 0x4000) >> 1;
    #endif

        health = _health;
        radius = _radius;
        headOffset = _headOffset;
        aggression = _aggression;
    }

    void setExtra(ExtraInfoEnemy* extra)
    {
        ASSERT(!extraE);

        extraE = extra;

        if (!extraE)
            return;

        if (extra->enemy)
        {
            extra->enemy->flags.status = ITEM_FLAGS_STATUS_INVISIBLE;
            extra->enemy->disable();
        }

        extra->enemy = this;
        extra->rotHead = extra->rotNeck = 0;

        // TODO initialize enemy and navigation
    }

    bool enable(bool forced)
    {
        //return false;

        if (extraE)
            return true;

        for (int32 i = 0; i < MAX_ENEMIES; i++)
        {
            if (!enemiesExtra[i].enemy)
            {
                setExtra(enemiesExtra + i);
                return true;
            }
        }

        for (int32 i = 0; i < MAX_PLAYERS; i++)
        {
            if (!players[i])
                continue;

            vec3i &viewPos = players[i]->extraL->camera->view.pos;

            int32 index = -1;
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

            if (index != -1)
            {
                setExtra(enemiesExtra + index);
                return true;
            }
        }

        return false;
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

        if (health <= 0)
        {
            tinfo.angle = 0;
            tinfo.rotHead = 0;
            tinfo.aim = false;
            return;
        }

        int32 dx = tinfo.target->pos.x - pos.x;
        int32 dz = tinfo.target->pos.z - pos.z;

        if (headOffset) {
            dx -= phd_sin(angle.y) * headOffset >> FIXED_SHIFT;
            dz -= phd_cos(angle.y) * headOffset >> FIXED_SHIFT;
        }

        tinfo.angle = phd_atan(dz, dx) - angle.y;
        tinfo.aim = (tinfo.angle > -ANGLE_90) && (tinfo.angle < ANGLE_90);
        tinfo.rotHead = tinfo.aim ? tinfo.angle : 0;
    }

    virtual void activate()
    {
        Item::activate();

        if (!enable(flags.status == ITEM_FLAGS_STATUS_NONE)) {
            flags.status = ITEM_FLAGS_STATUS_INVISIBLE;
        }
    }

    virtual void hit(int32 damage, const vec3i &point, int32 soundId)
    {
        ASSERT(health > 0);

        health -= damage;

        if (health > 0)
        {
            if (soundId) {
                soundPlay(soundId, pos);
            }
        } else {
            gSaveGame.kills++;
        }

        if (type != ITEM_MUMMY) {
            fxBlood(point, 0, 0);
        }
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
        if (flags.status == ITEM_FLAGS_STATUS_INVISIBLE)
        {
            if (!enable(false))
                return;
            flags.status = ITEM_FLAGS_STATUS_ACTIVE;
        }

        if (!extraE)
            return;

        updateTargetInfo();

        logic();

        extraE->rotHead = angleLerp(extraE->rotHead, tinfo.rotHead, ANGLE(5));

        animProcess();

        if (flags.status == ITEM_FLAGS_STATUS_INACTIVE)
        {
            flags.collision = false;
            health = NOT_ENEMY;
            disable();
            deactivate();
            return;
        }

        // TODO collision, pathfinding

        updateRoom();
    }

    virtual void logic() {}
};


struct Doppelganger : Enemy
{
    Doppelganger(Room* room) : Enemy(room, LARA_MAX_HEALTH, 10, 0, 0) {}
};


struct Wolf : Enemy
{
    enum {
        HIT_MASK = 0x774F,  // body, head, front legs
    };

    enum {
        ANIM_DEATH = 20,
        ANIM_DEATH_RUN,
        ANIM_DEATH_JUMP,
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
        STATE_BITE,
    };

    Wolf(Room* room) : Enemy(room, 6, 341, 375, AGGRESSION_LVL_2)
    {
        frameIndex = level.anims[animIndex].frameEnd;
    }

    virtual void hit(int32 damage, const vec3i &point, int32 soundId)
    {
        Enemy::hit(damage, point, SND_HIT_WOLF);

        if (health <= 0) {
            animSet(models[type].animIndex + ANIM_DEATH + (rand_logic() % 3), true);
        }
    }

    virtual void logic()
    {
        if (health <= 0)
            return;

        // TODO

        if (state == STATE_SLEEP) {
            goalState = STATE_STOP;
        }
    }
};


struct Bear : Enemy
{
    enum {
        HIT_MASK = 0x2406C, // front legs and head
    };

    enum {
        STATE_WALK,
        STATE_STOP,
        STATE_HIND,
        STATE_RUN,
        STATE_STAND,
        STATE_GROWL,
        STATE_BITE,
        STATE_ATTACK,
        STATE_EAT,
        STATE_DEATH,
    };

    Bear(Room* room) : Enemy(room, 20, 341, 500, AGGRESSION_LVL_3) {}

    virtual void hit(int32 damage, const vec3i &point, int32 soundId)
    {
        Enemy::hit(damage, point, SND_HIT_BEAR);
    }

    virtual void logic()
    {
        if (health <= 0)
        {
            switch (state)
            {
                case STATE_HIND:
                    goalState = STATE_STAND;
                    break;
                case STATE_WALK:
                case STATE_RUN:
                    goalState = STATE_STOP;
                    break;
                case STATE_STOP:
                case STATE_STAND:
                    goalState = STATE_DEATH;
                    break;
            }
            return;
        }

        // TODO
    }
};


struct Bat : Enemy
{
    enum {
        STATE_NONE,
        STATE_AWAKE,
        STATE_FLY,
        STATE_ATTACK,
        STATE_CIRCLING,
        STATE_DEATH,
    };

    Bat(Room* room) : Enemy(room, 1, 102, 0, AGGRESSION_LVL_1) {}

    virtual void logic()
    {
        if (health <= 0)
        {
            hSpeed = 0;
            flags.gravity = (pos.y < roomFloor);

            if (flags.gravity) {
                goalState = STATE_CIRCLING;
            } else {
                goalState = STATE_DEATH;
                pos.y = roomFloor;
            }

            return;
        }

        // TODO
    }
};


struct Crocodile : Enemy
{
    Crocodile(Room* room) : Enemy(room, 20, 341, 600, AGGRESSION_LVL_2) {}
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
};


struct Gorilla : Enemy
{
    Gorilla(Room* room) : Enemy(room, 22, 341, 250, AGGRESSION_LVL_MAX) {}
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
    Rex(Room* room) : Enemy(room, 100, 341, 2000, AGGRESSION_LVL_MAX) {}
};


struct Raptor : Enemy
{
    Raptor(Room* room) : Enemy(room, 20, 341, 400, AGGRESSION_LVL_3) {}
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
