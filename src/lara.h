#ifndef H_LARA
#define H_LARA
/*****************************************/
/*     Desine sperare qui hic intras     */
/*****************************************/
#include "character.h"
#include "trigger.h"
#include "sprite.h"

#define TURN_FAST           PI
#define TURN_FAST_BACK      PI * 3.0f / 4.0f
#define TURN_NORMAL         PI / 2.0f
#define TURN_SLOW           PI / 3.0f
#define TURN_WATER_FAST     (DEG2RAD * 150.0f)
#define TURN_WATER_SLOW     (DEG2RAD * 60.0f)
#define TURN_WALL_Y         (DEG2RAD * 150.0f)
#define TURN_WALL_X         (DEG2RAD * 60.0f)
#define TURN_WALL_X_CLAMP   (DEG2RAD * 35.0f)

#define LARA_TILT_SPEED     (DEG2RAD * 37.5f)
#define LARA_TILT_MAX       (DEG2RAD * 10.0f)

#define LARA_MAX_HEALTH     1000.0f
#define LARA_MAX_OXYGEN     60.0f

#define LARA_HANG_OFFSET    724
#define LARA_HEIGHT         762
#define LARA_HEIGHT_WATER   400
#define LARA_RADIUS         100.0f
#define LARA_RADIUS_WATER   300.0f

#define LARA_WATER_ACCEL    2.0f
#define LARA_SURF_SPEED     15.0f
#define LARA_SWIM_SPEED     50.0f
#define LARA_SWIM_FRICTION  1.0f

#define LARA_WET_SPECULAR   0.5f
#define LARA_WET_TIMER      (LARA_WET_SPECULAR / 16.0f)   // 4 sec

#define LARA_DAMAGE_TIME    (40.0f / 30.0f)

#define PICKUP_FRAME_GROUND     40
#define PICKUP_FRAME_UNDERWATER 18
#define PUZZLE_FRAME            80

#define MAX_TRIGGER_ACTIONS 64

#define DESCENT_SPEED       2048.0f
#define MUZZLE_FLASH_TIME   0.1f
#define FLASH_LIGHT_COLOR   vec4(0.6f, 0.5f, 0.1f, 1.0f / 3072.0f)
#define TARGET_MAX_DIST     (8.0f * 1024.0f)

struct Lara : Character {

    // http://www.tombraiderforums.com/showthread.php?t=148859
    enum {
        ANIM_STAND_LEFT         = 2,
        ANIM_STAND_RIGHT        = 3,

        ANIM_RUN_START          = 6,

        ANIM_STAND              = 11,

        ANIM_CLIMB_JUMP         = 26,

        ANIM_HANG_FALL          = 28,

        ANIM_FALL               = 34,
        ANIM_SMASH_JUMP         = 32,

        ANIM_CLIMB_3            = 42,

        ANIM_CLIMB_2            = 50,

        ANIM_SMASH_RUN_LEFT     = 53,
        ANIM_SMASH_RUN_RIGHT    = 54,
        ANIM_RUN_ASCEND_LEFT    = 55,
        ANIM_RUN_ASCEND_RIGHT   = 56,
        ANIM_WALK_ASCEND_LEFT   = 57,
        ANIM_WALK_ASCEND_RIGHT  = 58,
        ANIM_WALK_DESCEND_RIGHT = 59,
        ANIM_WALK_DESCEND_LEFT  = 60,
        ANIM_BACK_DESCEND_LEFT  = 61,
        ANIM_BACK_DESCEND_RIGHT = 62,

        ANIM_SLIDE_FORTH        = 70,

        ANIM_HANG               = 96,

        ANIM_STAND_NORMAL       = 103,

        ANIM_SLIDE_BACK         = 105,

        ANIM_UNDERWATER         = 108,

        ANIM_WATER_FALL         = 112,
        ANIM_TO_ONWATER         = 114,
        ANIM_TO_UNDERWATER      = 119,
        ANIM_HIT_FRONT          = 125,
        ANIM_HIT_BACK           = 126,
        ANIM_HIT_LEFT           = 127,
        ANIM_HIT_RIGHT          = 128,
        ANIM_STAND_ROLL_BEGIN   = 146,
        ANIM_STAND_ROLL_END     = 147,

        ANIM_HANG_SWING         = 150,
    };

    // http://www.tombraiderforums.com/showthread.php?t=211681
    enum {
        STATE_WALK,
        STATE_RUN,
        STATE_STOP,
        STATE_FORWARD_JUMP,
        STATE_POSE,
        STATE_FAST_BACK,
        STATE_TURN_RIGHT,
        STATE_TURN_LEFT,
        STATE_DEATH,
        STATE_FALL,
        STATE_HANG,
        STATE_REACH,
        STATE_SPLAT,
        STATE_TREAD,
        STATE_FAST_TURN_14,
        STATE_COMPRESS,
        STATE_BACK,
        STATE_SWIM,
        STATE_GLIDE,
        STATE_HANG_UP,
        STATE_FAST_TURN,
        STATE_STEP_RIGHT,
        STATE_STEP_LEFT,
        STATE_ROLL_1,
        STATE_SLIDE,
        STATE_BACK_JUMP,
        STATE_RIGHT_JUMP,
        STATE_LEFT_JUMP,
        STATE_UP_JUMP,
        STATE_FALL_BACK,
        STATE_HANG_LEFT,
        STATE_HANG_RIGHT,
        STATE_SLIDE_BACK,
        STATE_SURF_TREAD,
        STATE_SURF_SWIM,
        STATE_DIVE,
        STATE_PUSH_BLOCK,
        STATE_PULL_BLOCK,
        STATE_PUSH_PULL_READY,
        STATE_PICK_UP,
        STATE_SWITCH_DOWN,
        STATE_SWITCH_UP,
        STATE_USE_KEY,
        STATE_USE_PUZZLE,
        STATE_UNDERWATER_DEATH,
        STATE_ROLL_2,
        STATE_SPECIAL,
        STATE_SURF_BACK,
        STATE_SURF_LEFT,
        STATE_SURF_RIGHT,
        STATE_USE_MIDAS,
        STATE_DIE_MIDAS,
        STATE_SWAN_DIVE,
        STATE_FAST_DIVE,
        STATE_HANDSTAND,
        STATE_WATER_OUT,
        STATE_MAX };

    enum : int {
        BODY_HIP        = 0x0001,
        BODY_LEG_L1     = 0x0002,
        BODY_LEG_L2     = 0x0004,
        BODY_LEG_L3     = 0x0008,
        BODY_LEG_R1     = 0x0010,
        BODY_LEG_R2     = 0x0020,
        BODY_LEG_R3     = 0x0040,
        BODY_CHEST      = 0x0080,
        BODY_ARM_R1     = 0x0100,
        BODY_ARM_R2     = 0x0200,
        BODY_ARM_R3     = 0x0400,
        BODY_ARM_L1     = 0x0800,
        BODY_ARM_L2     = 0x1000,
        BODY_ARM_L3     = 0x2000,
        BODY_HEAD       = 0x4000,
        BODY_ARM_L      = BODY_ARM_L1 | BODY_ARM_L2 | BODY_ARM_L3,
        BODY_ARM_R      = BODY_ARM_R1 | BODY_ARM_R2 | BODY_ARM_R3,
        BODY_LEG_L      = BODY_LEG_L1 | BODY_LEG_L2 | BODY_LEG_L3,
        BODY_LEG_R      = BODY_LEG_R1 | BODY_LEG_R2 | BODY_LEG_R3,
        BODY_UPPER      = BODY_CHEST  | BODY_ARM_L  | BODY_ARM_R,       // without head
        BODY_LOWER      = BODY_HIP    | BODY_LEG_L  | BODY_LEG_R,
        BODY_BRAID_MASK = BODY_HEAD   | BODY_CHEST  | BODY_ARM_L1 | BODY_ARM_L2 | BODY_ARM_R1 | BODY_ARM_R2,
    };

    bool home;
    bool dozy;

    struct Weapon {
        enum Type  { EMPTY = -1, PISTOLS, SHOTGUN, MAGNUMS, UZIS, MAX };
        enum State { IS_HIDDEN, IS_ARMED, IS_FIRING };
        enum Anim  { NONE, PREPARE, UNHOLSTER, HOLSTER, HOLD, AIM, FIRE };
    };

    Weapon::Type    wpnCurrent;
    Weapon::Type    wpnNext;
    Weapon::State   wpnState;
    int             *wpnAmmo;
    vec3            chestOffset;

    struct Arm {
        int             tracking;       // tracking target (main target)
        int             target;         // target for shooting
        float           shotTimer;
        quat            rot, rotAbs;
        Weapon::Anim    anim;
        Animation       animation;
    } arms[2];

    ActionCommand actionList[MAX_TRIGGER_ACTIONS];

    TR::Entity::Type  usedKey;
    TR::Entity        *puzzleEntity;
    TR::Entity        *pickupEntity;

    int viewTarget;
    int roomPrev; // water out from room
    vec2 rotFactor;

    float       oxygen;
    float       damageTime;
    float       hitTime;
    int         hitDir;
    vec3        collisionOffset;

    struct Braid {
        Lara *lara;
        vec3 offset;

        Basis *basis;
        struct Joint {
            vec3 posPrev, pos;
            float length;
        } *joints;
        int jointsCount;
        float time;

        Braid(Lara *lara, const vec3 &offset) : lara(lara), offset(offset), time(0.0f) {
            TR::Level *level = lara->level;
            TR::Model *model = getModel();
            jointsCount = model->mCount + 1;
            joints      = new Joint[jointsCount];
            basis       = new Basis[jointsCount - 1];

            Basis basis = getBasis();
            basis.translate(offset);

            TR::Node *node = (int)model->node < level->nodesDataSize ? (TR::Node*)&level->nodesData[model->node] : NULL;
            for (int i = 0; i < jointsCount - 1; i++) {
                TR::Node &t = node[min(i, model->mCount - 2)];
                joints[i].posPrev = joints[i].pos = basis.pos;
                joints[i].length  = float(t.z);
                basis.translate(vec3(0.0f, 0.0f, -joints[i].length));
            }
            joints[jointsCount - 1].posPrev = joints[jointsCount - 1].pos = basis.pos;
            joints[jointsCount - 1].length  = 1.0f;
        }

        ~Braid() {
            delete[] joints;
            delete[] basis;
        }

        TR::Model* getModel() const {
            return &lara->level->models[lara->level->extra.braid];
        }

        Basis getBasis() {
            return lara->animation.getJoints(lara->getMatrix(), 14, true);
        }

        vec3 getPos() {
            return getBasis() * offset;
        }

        void integrate() {
            float TIMESTEP = Core::deltaTime;
            float ACCEL    = 6.0f * GRAVITY * TIMESTEP * TIMESTEP;
            float DAMPING  = 1.5f;

            if (lara->getRoom().flags.water) {
                ACCEL *= -0.5f;
                DAMPING = 4.0f;
            }

            DAMPING = 1.0f / (1.0f + DAMPING * TIMESTEP); // Pade approximation

            for (int i = 1; i < jointsCount; i++) {
                Joint &j = joints[i];
                vec3 delta = j.pos - j.posPrev;
                delta = delta.normal() * (min(delta.length(), 2048.0f * Core::deltaTime) * DAMPING); // speed limit
                j.posPrev  = j.pos;
                j.pos     += delta;
                if (lara->stand == STAND_ONWATER) {
                    if (j.pos.y > lara->pos.y)
                        j.pos.y += ACCEL;
                    else
                        j.pos.y -= ACCEL;
                } else
                    j.pos.y += ACCEL;
            }
        }

        void collide() {
            TR::Level *level = lara->level;
            TR::Model *model = lara->getModel();

            #define BRAID_RADIUS 16.0f

            for (int i = 0; i < model->mCount; i++) {
                if (!(BODY_BRAID_MASK & (1 << i))) continue;

                int offset = level->meshOffsets[model->mStart + i];
                TR::Mesh *mesh = (TR::Mesh*)&level->meshes[offset];

                vec3 center    = lara->animation.getJoints(lara->getMatrix(), i, true) * mesh->center;
                float radiusSq = mesh->radius + BRAID_RADIUS;
                radiusSq *= radiusSq;

                for (int j = 1; j < jointsCount; j++) {
                    vec3 dir = joints[j].pos - center;
                    float len = dir.length2() + EPS;
                    if (len < radiusSq) {
                        len = sqrtf(len);
                        dir *= (mesh->radius + BRAID_RADIUS- len) / len;
                        joints[j].pos += dir * 0.9f;
                    }
                }
            }

            #undef BRAID_RADIUS
        }

        void solve() {
            for (int i = 0; i < jointsCount - 1; i++) {
                Joint &a = joints[i];
                Joint &b = joints[i + 1];

                vec3 dir = b.pos - a.pos;
                float len = dir.length() + EPS;
                dir *= 1.0f / len;

                float d = a.length - len;

                if (i > 0) {
                    dir *= d * (0.5f * 1.0f);
                    a.pos -= dir;
                    b.pos += dir;
                } else
                    b.pos += dir * (d * 1.0f);
            }
        }

        void update() {
            joints[0].pos = getPos();
            integrate(); // Verlet integration step
            collide();   // check collision with Lara's mesh
            for (int i = 0; i < jointsCount; i++) // solve connections (springs)
                solve();

            vec3 headDir = getBasis().rot * vec3(0.0f, 0.0f, -1.0f);

            for (int i = 0; i < jointsCount - 1; i++) {
                vec3 d = (joints[i + 1].pos - joints[i].pos).normal();
                vec3 r = d.cross(headDir).normal();
                vec3 u = d.cross(r).normal();

                mat4 m;
                m.up     = vec4(u, 0.0f);
                m.dir    = vec4(d, 0.0f);
                m.right  = vec4(r, 0.0f);
                m.offset = vec4(0.0f, 0.0f, 0.0f, 1.0f);

                basis[i].identity();
                basis[i].translate(joints[i].pos);
                basis[i].rotate(m.getRot());
            }
        }

        void render(MeshBuilder *mesh) {
            Core::active.shader->setParam(uBasis, basis[0], jointsCount);
            mesh->renderModel(lara->level->extra.braid);
        }

    } *braid;

    Lara(IGame *game, int entity, bool home) : Character(game, entity, LARA_MAX_HEALTH), home(home), dozy(false), wpnCurrent(Weapon::EMPTY), wpnNext(Weapon::EMPTY), chestOffset(pos), viewTarget(-1), braid(NULL) {

        if (getEntity().type == TR::Entity::LARA) {
            if (getRoom().flags.water)
                animation.setAnim(ANIM_UNDERWATER);
            else
                animation.setAnim(ANIM_STAND);
        }

        oxygen     = LARA_MAX_OXYGEN;
        hitDir     = -1;
        damageTime = LARA_DAMAGE_TIME;
        hitTime    = 0.0f;

        getEntity().flags.active = 1;
        initMeshOverrides();

        if (!home)
            wpnSet(Weapon::PISTOLS);
        else
            meshSwap(1, TR::MODEL_LARA_SPEC, BODY_UPPER | BODY_LOWER);

        for (int i = 0; i < 2; i++) {
            arms[i].shotTimer = MUZZLE_FLASH_TIME + 1.0f;
            arms[i].rot       = quat(0, 0, 0, 1);
            arms[i].rotAbs    = quat(0, 0, 0, 1);
        }

        if (level->extra.braid > -1)
            braid = new Braid(this, vec3(-4.0f, 24.0f, -48.0f));
//reset(15, vec3(70067, -256, 29104), -0.68f);     // level 2 (pool)
    #ifdef _DEBUG
        //reset(14, vec3(40448, 3584, 60928), PI * 0.5f, true);  // gym (pool)

        //reset(14, vec3(20215, 6656, 52942), PI);         // level 1 (bridge)
        //reset(15, vec3(70067, -256, 29104), -0.68f);     // level 2 (pool)
        //reset(61, vec3(27221, -1024, 29205), PI * 0.5f); // level 2 (blade)
        //reset(43, vec3(31400, -2560, 25200), PI);        // level 2 (reach)
        //reset(16, vec3(60907, 0, 39642), PI * 3 / 2);    // level 2 (hang & climb)
        //reset(19, vec3(60843, 1024, 30557), PI);         // level 2 (block)
        reset(1,  vec3(62630, -1280, 19633), 0);         // level 2 (dark medikit)
        //reset(7,  vec3(64108, -512, 16514), -PI * 0.5f); // level 2 (bat trigger)
        //reset(15, vec3(70082, -512, 26935), PI * 0.5f);  // level 2 (bear)
        //reset(63, vec3(31390, -2048, 33472), 0.0f);      // level 2 (trap floor)
        //reset(61, vec3(21987, -1024, 29144), PI * 3.0f * 0.5f); // level 2 (trap door)
        //reset(51, vec3(41015, 3584, 34494), -PI);        // level 3a (t-rex)
        //reset(5,  vec3(38643, -3072, 92370), PI * 0.5f); // level 3a (gears)
        //reset(43, vec3(64037, 6656, 48229), PI);         // level 3b (movingblock)
        //reset(0,  vec3(40913, -1012, 42252), PI);        // level 8c
        //reset(10, vec3(90443, 11264 - 256, 114614), PI, STAND_ONWATER);   // villa mortal 2
    #endif
        chestOffset = animation.getJoints(getMatrix(), 7).pos;
    }

    virtual ~Lara() {
        delete braid;
    }

    int getRoomByPos(const vec3 &pos) {
        int x = int(pos.x),
            y = int(pos.y),
            z = int(pos.z);

        for (int i = 0; i < level->roomsCount; i++) {
            TR::Room &r = level->rooms[i];
            int mx = r.info.x + r.xSectors * 1024;
            int mz = r.info.z + r.zSectors * 1024;
            if (x >= r.info.x && x < mx && z >= r.info.z && z < mz && y >= r.info.yTop && y < r.info.yBottom)
                return i;
        }
        return TR::NO_ROOM;
    }

    void reset(int room, const vec3 &pos, float angle, Stand forceStand = STAND_GROUND) {
        if (room == TR::NO_ROOM) {
            stand = STAND_AIR;
            room  = getRoomByPos(pos);
        }

        if (room == TR::NO_ROOM)
            return;

        if (level->rooms[room].flags.water) {
            stand = STAND_UNDERWATER;
            animation.setAnim(ANIM_UNDERWATER);
        }

        velocity = vec3(0.0f);

        getEntity().room = room;
        this->pos        = pos;
        this->angle      = vec3(0.0f, angle, 0.0f);

        if (forceStand != STAND_GROUND) {
            stand = forceStand;
            switch (stand) {
                case STAND_ONWATER    : animation.setAnim(ANIM_TO_ONWATER); break;
                case STAND_UNDERWATER : animation.setAnim(ANIM_UNDERWATER); break;
                default               : ;
            }
        }

        updateEntity();
        updateLights(false);
    }

    TR::Entity::Type getCurrentWeaponInv() {
        switch (wpnCurrent) {
            case Weapon::Type::PISTOLS : return TR::Entity::PISTOLS;
            case Weapon::Type::SHOTGUN : return TR::Entity::SHOTGUN;
            case Weapon::Type::MAGNUMS : return TR::Entity::MAGNUMS;
            case Weapon::Type::UZIS    : return TR::Entity::UZIS;
            default                    : return TR::Entity::NONE;
        }
    }

    void wpnSet(Weapon::Type wType) {
        wpnCurrent = wType;
        wpnState   = Weapon::IS_FIRING;

        TR::Entity::Type invType = getCurrentWeaponInv();

        wpnAmmo = game->invCount(invType);

        arms[0].animation = arms[1].animation = Animation(level, &level->models[wType == Weapon::SHOTGUN ? TR::MODEL_SHOTGUN : TR::MODEL_PISTOLS]);

        wpnSetAnim(arms[0], Weapon::IS_HIDDEN, Weapon::Anim::NONE, 0.0f, 0.0f);
        wpnSetAnim(arms[1], Weapon::IS_HIDDEN, Weapon::Anim::NONE, 0.0f, 0.0f);
    }

    void wpnSetAnim(Arm &arm, Weapon::State wState, Weapon::Anim wAnim, float wAnimTime, float wAnimDir, bool playing = true) {
        arm.animation.setAnim(wpnGetAnimIndex(wAnim), 0, wAnim == Weapon::Anim::FIRE);
        arm.animation.dir = playing ? wAnimDir : 0.0f;

        if (arm.anim != wAnim)
            arm.animation.frameIndex = 0xFFFF;

        arm.anim = wAnim;

        if (wAnimDir > 0.0f)
            arm.animation.time = wAnimTime;
        else
            if (wAnimDir < 0.0f)
                arm.animation.time = arm.animation.timeMax + wAnimTime;
        arm.animation.updateInfo();

        wpnSetState(wState);
    }

    float wpnGetDamage() {
        switch (wpnCurrent) {
            case Weapon::PISTOLS : return 1;
            case Weapon::SHOTGUN : return 1;
            case Weapon::MAGNUMS : return 2;
            case Weapon::UZIS    : return 1;
            default : ;
        }
        return 0;
    }

    void wpnSetState(Weapon::State wState) {
        if (wpnState == wState || !layers) return;

        int mask = 0;
        switch (wpnCurrent) {
            case Weapon::EMPTY   : break;
            case Weapon::PISTOLS :
            case Weapon::MAGNUMS :
            case Weapon::UZIS    :
                switch (wState) {
                    case Weapon::IS_HIDDEN : mask = BODY_LEG_L1 | BODY_LEG_R1;              break;
                    case Weapon::IS_ARMED  : mask = BODY_ARM_L3 | BODY_ARM_R3;              break;
                    case Weapon::IS_FIRING : mask = BODY_ARM_L3 | BODY_ARM_R3 | BODY_HEAD;  break;
                }
                break;
            case Weapon::SHOTGUN :
                switch (wState) {
                    case Weapon::IS_HIDDEN : mask = BODY_CHEST;                             break;
                    case Weapon::IS_ARMED  : mask = BODY_ARM_L3 | BODY_ARM_R3;              break;
                    case Weapon::IS_FIRING : mask = BODY_ARM_L3 | BODY_ARM_R3 | BODY_HEAD;  break;
                }
                break;
            default : ;
        }

        if (wpnState == Weapon::IS_HIDDEN && wState == Weapon::IS_ARMED)  playSound(TR::SND_UNHOLSTER, pos, Sound::Flags::PAN);
        if (wpnState == Weapon::IS_ARMED  && wState == Weapon::IS_HIDDEN) playSound(TR::SND_HOLSTER,   pos, Sound::Flags::PAN);

    // swap layers
    // 0 - body (full)
    // 1 - legs (hands, legs)
    // 2 - shotgun (hands, chest)
    // 3 - angry (head)

        // swap weapon parts
        if (wpnCurrent != Weapon::SHOTGUN) {
            meshSwap(1, level->extra.weapons[wpnCurrent], mask);
            // have a shotgun in inventory place it on the back if another weapon is in use
            meshSwap(2, level->extra.weapons[Weapon::SHOTGUN], game->invCount(TR::Entity::INV_SHOTGUN) ? BODY_CHEST : 0);
        } else {
            meshSwap(2, level->extra.weapons[wpnCurrent], mask);
        }

        // mesh swap to angry Lara's head while firing (from uzis model)
        meshSwap(3, level->extra.weapons[Weapon::UZIS], (wState == Weapon::IS_FIRING) ? BODY_HEAD : 0);

        wpnState = wState;
    }

    bool emptyHands() {
        return wpnCurrent == Weapon::EMPTY || arms[0].anim == Weapon::Anim::NONE;
    }

    bool canLookAt() {
        return (stand == STAND_GROUND || stand == STAND_SLIDE)
               && state != STATE_REACH
               && state != STATE_PUSH_BLOCK
               && state != STATE_PULL_BLOCK
               && state != STATE_PUSH_PULL_READY
               && state != STATE_PICK_UP;
    }

    bool canDrawWeapon() {
        if (dozy) return true;

        return wpnCurrent != Weapon::EMPTY
               && emptyHands()
               && animation.index != ANIM_CLIMB_3
               && animation.index != ANIM_CLIMB_2
               && state != STATE_DEATH
               && state != STATE_HANG
               && state != STATE_REACH
               && state != STATE_TREAD
               && state != STATE_SWIM
               && state != STATE_GLIDE
               && state != STATE_HANG_UP
               && state != STATE_FALL_BACK
               && state != STATE_HANG_LEFT
               && state != STATE_HANG_RIGHT
               && state != STATE_SURF_TREAD
               && state != STATE_SURF_SWIM
               && state != STATE_DIVE
               && state != STATE_PUSH_BLOCK
               && state != STATE_PULL_BLOCK
               && state != STATE_PUSH_PULL_READY
               && state != STATE_PICK_UP
               && state != STATE_SWITCH_DOWN
               && state != STATE_SWITCH_UP
               && state != STATE_USE_KEY
               && state != STATE_USE_PUZZLE
               && state != STATE_UNDERWATER_DEATH
               && state != STATE_SPECIAL
               && state != STATE_SURF_BACK
               && state != STATE_SURF_LEFT
               && state != STATE_SURF_RIGHT
               && state != STATE_SWAN_DIVE
               && state != STATE_FAST_DIVE
               && state != STATE_HANDSTAND
               && state != STATE_WATER_OUT;
    }

    bool canHitAnim() {
        return    state == STATE_WALK
               || state == STATE_RUN
               || state == STATE_STOP
               || state == STATE_FAST_BACK
               || state == STATE_TURN_RIGHT
               || state == STATE_TURN_LEFT
               || state == STATE_BACK
               || state == STATE_FAST_TURN
               || state == STATE_STEP_RIGHT
               || state == STATE_STEP_LEFT;
    }

    bool wpnReady() {
        return arms[0].anim != Weapon::Anim::PREPARE && arms[0].anim != Weapon::Anim::UNHOLSTER && arms[0].anim != Weapon::Anim::HOLSTER;
    }

    void wpnDraw() {
        if (!canDrawWeapon()) return;

        if (wpnReady() && emptyHands()) {
            if (wpnCurrent != Weapon::SHOTGUN) {
                wpnSetAnim(arms[0], wpnState, Weapon::Anim::PREPARE, 0.0f, 1.0f);
                wpnSetAnim(arms[1], wpnState, Weapon::Anim::PREPARE, 0.0f, 1.0f);
            } else
                wpnSetAnim(arms[0], wpnState, Weapon::Anim::UNHOLSTER, 0.0f, 1.0f);
        }
    }

    void wpnHide() {
        if (wpnReady() && !emptyHands()) {
            if (wpnCurrent != Weapon::SHOTGUN) {
                wpnSetAnim(arms[0], wpnState, Weapon::Anim::UNHOLSTER, 0.0f, -1.0f);
                wpnSetAnim(arms[1], wpnState, Weapon::Anim::UNHOLSTER, 0.0f, -1.0f);
            } else
                wpnSetAnim(arms[0], wpnState, Weapon::Anim::HOLSTER, 0.0f, 1.0f);
        }
    }

    void wpnChange(Weapon::Type wType) {
        if (wpnCurrent == wType || home) {
            if (emptyHands())
                wpnDraw();
            return;
        }
        wpnNext = wType;
        wpnHide();
    }

    int wpnGetAnimIndex(Weapon::Anim wAnim) {
        if (wpnCurrent == Weapon::SHOTGUN) {
            switch (wAnim) {
                case Weapon::Anim::PREPARE   : ASSERT(false); break;    // rifle has no prepare animation
                case Weapon::Anim::UNHOLSTER : return 1;
                case Weapon::Anim::HOLSTER   : return 3;
                case Weapon::Anim::HOLD      :
                case Weapon::Anim::AIM       : return 0;
                case Weapon::Anim::FIRE      : return 2;
                default : ;
            }
        } else
            switch (wAnim) {
                case Weapon::Anim::PREPARE   : return 1;
                case Weapon::Anim::UNHOLSTER : return 2;
                case Weapon::Anim::HOLSTER   : ASSERT(false); break;    // pistols has no holster animation (it's reversed unholster)
                case Weapon::Anim::HOLD      :
                case Weapon::Anim::AIM       : return 0;
                case Weapon::Anim::FIRE      : return 3;
                default : ;
            }
        return 0;
    }

    int wpnGetSound() {
        switch (wpnCurrent) {
            case Weapon::PISTOLS : return TR::SND_PISTOLS_SHOT;
            case Weapon::SHOTGUN : return TR::SND_SHOTGUN_SHOT;
            case Weapon::MAGNUMS : return TR::SND_MAGNUMS_SHOT;
            case Weapon::UZIS    : return TR::SND_UZIS_SHOT;
            default              : return TR::SND_NO;
        }
    }

    void wpnFire() {
        bool armShot[2] = { false, false };
        for (int i = 0; i < 2; i++) {
            Arm &arm = arms[i];
            if (arm.anim == Weapon::Anim::FIRE) {
                Animation &anim = arm.animation;
                //int realFrameIndex = int(arms[i].animation.time * 30.0f / anim->frameRate) % ((anim->frameEnd - anim->frameStart) / anim->frameRate + 1);
                if (anim.frameIndex != anim.framePrev) {
                    if (anim.frameIndex == 0) { //realFrameIndex < arms[i].animation.framePrev) {
                        if ((input & ACTION) && (arm.tracking == -1 || arm.target > -1)) {
                            armShot[i] = true;
                        } else
                            wpnSetAnim(arm, Weapon::IS_ARMED, Weapon::Anim::AIM, 0.0f, -1.0f, arm.target == -1);
                    }
                // shotgun reload sound
                    if (wpnCurrent == Weapon::SHOTGUN) {
                        if (anim.frameIndex == 10)
                            playSound(TR::SND_SHOTGUN_RELOAD, pos, Sound::Flags::PAN);
                    }
                }
            }
            arm.animation.framePrev = arm.animation.frameIndex;

            if (wpnCurrent == Weapon::SHOTGUN) break;
        }

        if (armShot[0] || armShot[1])
            doShot(armShot[0], armShot[1]);
    }

    void doShot(bool rightHand, bool leftHand) {
        int count = wpnCurrent == Weapon::SHOTGUN ? 6 : 2;

        float nearDist = 32.0f * 1024.0f;
        vec3  nearPos;
        int   shots = 0;

        for (int i = 0; i < count; i++) {
            int armIndex;
            if (wpnCurrent == Weapon::SHOTGUN) {
                if (!rightHand) continue;
                armIndex = 0;
            } else {
                if (!(i ? leftHand : rightHand)) continue;
                armIndex = i;
            }
            Arm *arm = &arms[armIndex];

            if (wpnAmmo && *wpnAmmo != UNLIMITED_AMMO) {
                if (*wpnAmmo <= 0)
                    continue;
                if (wpnCurrent != Weapon::SHOTGUN)
                    *wpnAmmo -= 1;
            }

            arm->shotTimer = 0.0f;
            shots++;

            int joint = wpnCurrent == Weapon::SHOTGUN ? 8 : (i ? 11 : 8);

            vec3 p = animation.getJoints(getMatrix(), joint, false).pos;
            vec3 d = arm->rotAbs * vec3(0, 0, 1);
            vec3 t = p + d * (24.0f * 1024.0f) + ((vec3(randf(), randf(), randf()) * 2.0f) - vec3(1.0f)) * 1024.0f;

            int room;
            vec3 hit = trace(getRoomIndex(), p, t, room, false);
            if (arm->target > -1 && checkHit(arm->target, p, hit, hit)) {
                ((Character*)level->entities[arm->target].controller)->hit(wpnGetDamage());
                hit -= d * 64.0f;
                Sprite::add(game, TR::Entity::BLOOD, room, (int)hit.x, (int)hit.y, (int)hit.z, Sprite::FRAME_ANIMATED);
            } else {
                hit -= d * 64.0f;
                Sprite::add(game, TR::Entity::RICOCHET, room, (int)hit.x, (int)hit.y, (int)hit.z, Sprite::FRAME_RANDOM);

                float dist = (hit - p).length();
                if (dist < nearDist) {
                    nearPos  = hit;
                    nearDist = dist;
                }
            }

            Core::lightPos[1 + armIndex]   = animation.getJoints(getMatrix(), armIndex == 0 ? 10 : 13, false).pos;
            Core::lightColor[1 + armIndex] = FLASH_LIGHT_COLOR;
        }

        if (shots) {
            playSound(wpnGetSound(), pos, Sound::Flags::PAN);
            playSound(TR::SND_RICOCHET, nearPos, Sound::Flags::PAN);

             if (wpnAmmo && *wpnAmmo != UNLIMITED_AMMO && wpnCurrent == Weapon::SHOTGUN)
                *wpnAmmo -= 1;
        }

        if (wpnAmmo && *wpnAmmo != UNLIMITED_AMMO && *wpnAmmo <= 0) {
            wpnChange(Weapon::PISTOLS);
        }
    }

    void updateWeapon() {
        if (level->cutEntity > -1) return;

        if (input & DEATH) {
            arms[0].shotTimer = arms[1].shotTimer = MUZZLE_FLASH_TIME + 1.0f;
            arms[0].tracking  = arms[1].tracking  = -1;
            arms[0].target    = arms[1].target    = -1;
            animation.overrideMask = 0;
            return;
        }

        updateTargets();
        updateOverrides();

        if (wpnNext != Weapon::EMPTY && emptyHands()) {
            wpnSet(wpnNext);
            wpnDraw();
            wpnNext = Weapon::EMPTY;
        }

    // apply weapon state changes
        if (input & WEAPON) {
            if (emptyHands())
                wpnDraw();
            else
                wpnHide();
        }

        if (!emptyHands()) {
            bool isRifle = wpnCurrent == Weapon::SHOTGUN;

            for (int i = 0; i < 2; i++) {
                Arm &arm = arms[i];

                if (arm.target > -1 || ((input & ACTION) && arm.tracking == -1)) {
                    if (arm.anim == Weapon::Anim::HOLD)
                        wpnSetAnim(arm, wpnState, Weapon::Anim::AIM, 0.0f, 1.0f);
                } else
                    if (arm.anim == Weapon::Anim::AIM)
                        arm.animation.dir = -1.0f;

                if (isRifle) break;
            }

            for (int i = 0; i < 2; i++) {
                arms[i].animation.update();
                arms[i].shotTimer += Core::deltaTime;

                float intensity = clamp((0.1f - arms[i].shotTimer) * 20.0f, EPS, 1.0f);
                Core::lightColor[1 + i] = FLASH_LIGHT_COLOR * vec4(intensity, intensity, intensity, 1.0f / sqrtf(intensity));
            }

            if (isRifle)
                animateShotgun();
            else
                animatePistols();

            wpnFire(); // make a shot
        }
    }

    void animatePistols() {
        for (int i = 0; i < 2; i++) {
            Arm &arm = arms[i];

            if (!arm.animation.isEnded) continue;

            if (arm.animation.dir >= 0.0f)
                switch (arm.anim) {
                    case Weapon::Anim::PREPARE   : wpnSetAnim(arm, Weapon::IS_ARMED, Weapon::Anim::UNHOLSTER, arm.animation.time - arm.animation.timeMax, 1.0f); break;
                    case Weapon::Anim::UNHOLSTER : wpnSetAnim(arm, Weapon::IS_ARMED, Weapon::Anim::HOLD, 0.0f, 1.0f, false); break;
                    case Weapon::Anim::AIM       :
                    case Weapon::Anim::FIRE      :
                        if (input & ACTION)
                            wpnSetAnim(arm, Weapon::IS_FIRING, Weapon::Anim::FIRE, arm.animation.time - arm.animation.timeMax, wpnCurrent == Weapon::UZIS ? 2.0f : 1.0f);
                        else
                            wpnSetAnim(arm, Weapon::IS_ARMED, Weapon::Anim::AIM, 0.0f, -1.0f, false);
                        break;
                    default : ;
                };

            if (arm.animation.dir < 0.0f)
                switch (arm.anim) {
                    case Weapon::Anim::PREPARE   : wpnSetAnim(arm, Weapon::IS_HIDDEN, Weapon::Anim::NONE,    0.0f, 1.0f, false);    break;
                    case Weapon::Anim::UNHOLSTER : wpnSetAnim(arm, Weapon::IS_HIDDEN, Weapon::Anim::PREPARE, arm.animation.time, -1.0f);  break;
                    case Weapon::Anim::AIM       : wpnSetAnim(arm, Weapon::IS_ARMED,  Weapon::Anim::HOLD,    0.0f, 1.0f, false);    break;
                    default : ;
                };
        }
    }

    void animateShotgun() {
        Arm &arm = arms[0];
        if (arm.animation.dir >= 0.0f) {
            if (arm.animation.isEnded) {
                switch (arm.anim) {
                    case Weapon::Anim::UNHOLSTER : wpnSetAnim(arm, Weapon::IS_ARMED,  Weapon::Anim::HOLD, 0.0f, 1.0f, false); break;
                    case Weapon::Anim::HOLSTER   : wpnSetAnim(arm, Weapon::IS_HIDDEN, Weapon::Anim::NONE, 0.0f, 1.0f, false); break;
                    case Weapon::Anim::AIM       :
                    case Weapon::Anim::FIRE      :
                        if (input & ACTION)
                            wpnSetAnim(arm, Weapon::IS_FIRING, Weapon::Anim::FIRE, arm.animation.time - arm.animation.timeMax, 1.0f);
                        else
                            wpnSetAnim(arm, Weapon::IS_ARMED,  Weapon::Anim::AIM, 0.0f, -1.0f, false);
                        break;
                    default : ;
                }
            } else
                if (arm.animation.frameIndex != arm.animation.framePrev) {
                    float delta = arm.animation.time / arm.animation.timeMax;
                    switch (arm.anim) {
                        case Weapon::Anim::UNHOLSTER : if (delta >= 0.3f) wpnSetAnim(arm, Weapon::IS_ARMED,  arm.anim, arm.animation.time, 1.0f); break;
                        case Weapon::Anim::HOLSTER   : if (delta >= 0.7f) wpnSetAnim(arm, Weapon::IS_HIDDEN, arm.anim, arm.animation.time, 1.0f); break;
                        default : ;
                    }
                }
        } else
            if (arm.animation.isEnded && arm.anim == Weapon::Anim::AIM)
                wpnSetAnim(arm, Weapon::IS_ARMED, Weapon::Anim::HOLD, 0.0f, 1.0f, false);
    }

    void updateOverrides() {
        int overrideMask = 0;
        // head & chest
        overrideMask |= BODY_CHEST | BODY_HEAD;

        animation.overrides[ 7] = animation.getJointRot( 7);
        animation.overrides[14] = animation.getJointRot(14);

    /* TODO: shotgun full body animation
        if (wpnCurrent == Weapon::SHOTGUN) {
            animation.frameA = arms[0].animation.frameA;
            animation.frameB = arms[0].animation.frameB;
            animation.delta  = arms[0].animation.delta;
        }
    */

    // arms
        if (!emptyHands()) {
            // right arm
            Arm *arm = &arms[0];
            animation.overrides[ 8] = arm->animation.getJointRot( 8);
            animation.overrides[ 9] = arm->animation.getJointRot( 9);
            animation.overrides[10] = arm->animation.getJointRot(10);
            // left arm
            if (wpnCurrent != Weapon::SHOTGUN) arm = &arms[1];
            animation.overrides[11] = arm->animation.getJointRot(11);
            animation.overrides[12] = arm->animation.getJointRot(12);
            animation.overrides[13] = arm->animation.getJointRot(13);

            overrideMask |=  (BODY_ARM_R | BODY_ARM_L);
        } else
            overrideMask &= ~(BODY_ARM_R | BODY_ARM_L);

    // update hit anim
        if (hitDir >= 0) {
            Animation hitAnim = Animation(level, getModel());
            switch (hitDir) {
                case 0 : hitAnim.setAnim(ANIM_HIT_FRONT, 0, false); break;
                case 1 : hitAnim.setAnim(ANIM_HIT_LEFT,  0, false); break;
                case 2 : hitAnim.setAnim(ANIM_HIT_BACK , 0, false); break;
                case 3 : hitAnim.setAnim(ANIM_HIT_RIGHT, 0, false); break;
            }
            hitTime = min(hitTime, hitAnim.timeMax - EPS);
            hitAnim.time = hitTime;
            hitAnim.updateInfo();
            
            overrideMask &= ~(BODY_CHEST | BODY_HEAD);
            int hitMask = (BODY_UPPER | BODY_LOWER | BODY_HEAD) & ~overrideMask;
            int index    = 0;
            while (hitMask) {
                if (hitMask & 1)
                    animation.overrides[index] = hitAnim.getJointRot(index);
                index++;
                hitMask >>= 1;
            }

            hitTime += Core::deltaTime;
            overrideMask = BODY_UPPER | BODY_LOWER | BODY_HEAD;
        }

        animation.overrideMask = overrideMask;

        lookAt(viewTarget);

        if (wpnCurrent == Weapon::SHOTGUN)
            aimShotgun();
        else
            aimPistols();
    }

    void lookAt(int target) { // TODO: character lookAt
        float speed = 8.0f * Core::deltaTime;
        quat rot;

        bool can = canLookAt();
        // chest
        if (can && aim(target, 7, vec4(-PI * 0.4f, PI * 0.4f, -PI * 0.9f, PI * 0.9f), rot))
            rotChest = rotChest.slerp(quat(0, 0, 0, 1).slerp(rot, 0.5f), speed);
        else
            rotChest = rotChest.slerp(quat(0, 0, 0, 1), speed);
        animation.overrides[7] = rotChest * animation.overrides[7];

        // head
        if (can && aim(target, 14, vec4(-PI * 0.25f, PI * 0.25f, -PI * 0.5f, PI * 0.5f), rot))
            rotHead = rotHead.slerp(rot, speed);
        else
            rotHead = rotHead.slerp(quat(0, 0, 0, 1), speed);
        animation.overrides[14] = rotHead * animation.overrides[14];
    }

    void aimShotgun() {
        quat rot;

        Arm &arm = arms[0];
        arm.target = aim(arm.target, 14, vec4(-PI * 0.4f, PI * 0.4f, -PI * 0.25f, PI * 0.25f), rot, &arm.rotAbs) ? arm.target : -1;
    }

    void aimPistols() {
        float speed = 8.0f * Core::deltaTime;

        int joints[2] = { 8, 11 };

        vec4 ranges[2] = {
            vec4(-PI * 0.4f, PI * 0.4f, -PI * 0.2f, PI * 0.5f),
            vec4(-PI * 0.4f, PI * 0.4f, -PI * 0.5f, PI * 0.2f),
        };
        
        for (int i = 0; i < 2; i++) {
            quat rot;
            Arm &arm = arms[i];
            int j = joints[i];

            if (!aim(arm.target, j, ranges[i], rot, &arm.rotAbs)) {                
                arm.target = arms[i^1].target;
                if (!aim(arm.target, j, ranges[i], rot, &arm.rotAbs)) {
                    rot = quat(0, 0, 0, 1);
                    arm.target = -1;
                }
            }

            float t;
            if (arm.anim == Weapon::Anim::FIRE)
                t = 1.0f;
            else if (arm.anim == Weapon::Anim::AIM)
                t = arm.animation.time / arm.animation.timeMax;
            else
                t = 0.0f;

            arm.rot = arm.rot.slerp(rot, speed);
            animation.overrides[j] = animation.overrides[j].slerp(arm.rot * animation.overrides[j], t);
        }
    }

    void updateTargets() {
        arms[0].target = arms[1].target = -1;

        if (emptyHands() || !wpnReady()) {
            arms[0].tracking = arms[1].tracking = -1;
            return;
        }

        // auto retarget 
        bool retarget = false;
        if (Core::settings.controls.retarget) {
            for (int i = 0; i < 2; i++)
                if (arms[i].tracking == -1 || ((Character*)level->entities[arms[i].tracking].controller)->health <= 0.0f) {
                    retarget = true;
                    break;
                }
        }

        int count = wpnCurrent != Weapon::SHOTGUN ? 2 : 1;
        if (!(input & ACTION) || retarget) {
            getTargets(arms[0].tracking, arms[1].tracking);
            if (count == 1)
                arms[1].tracking = -1;
            else if (arms[0].tracking == -1 && arms[1].tracking != -1)
                arms[0].tracking = arms[1].tracking;
            else if (arms[1].tracking == -1 && arms[0].tracking != -1)
                arms[1].tracking = arms[0].tracking;
            arms[0].target = arms[0].tracking;
            arms[1].target = arms[1].tracking;
        } else {
            if (arms[0].tracking == -1 && arms[1].tracking == -1)
                return;

        // flip left and right by relative target direction
            if (count > 1) {
                int side[2] = { 0, 0 };
                vec3 dir = getDir();
                dir.y = 0.0f;

                for (int i = 0; i < count; i++)
                    if (arms[i].tracking != -1) {
                        vec3 v = ((Controller*)level->entities[arms[i].tracking].controller)->pos - pos;
                        v.y = 0;
                        side[i] = sign(v.cross(dir).y);
                    }

                if (side[0] > 0 && side[1] < 0)
                    swap(arms[0].tracking, arms[1].tracking);
            }

        // check occlusion for tracking targets
            for (int i = 0; i < count; i++)
                if (arms[i].tracking > -1) {
                    TR::Entity &e = level->entities[arms[i].tracking];
                    Controller *enemy = (Controller*)e.controller;

                    Box box = enemy->getBoundingBox();
                    vec3 to = box.center();
                    to.y = box.min.y + (box.max.y - box.min.y) / 3.0f;

                    vec3 from = pos - vec3(0, 650, 0);
                    arms[i].target = checkOcclusion(from, to, (to - from).length()) ? arms[i].tracking : -1;
                }

            if (count == 1)
                arms[1].target = -1;
            else if (arms[0].target == -1 && arms[1].target != -1)
                arms[0].target = arms[1].target;
            else if (arms[1].target == -1 && arms[0].target != -1)
                arms[1].target = arms[0].target;
        }
    }

    void getTargets(int &target1, int &target2) {
        vec3 dir = getDir().normal();
        float dist[2]  = { TARGET_MAX_DIST, TARGET_MAX_DIST };

        target1 = target2 = -1;

        vec3 from = pos - vec3(0, 650, 0);

        for (int i = 0; i < level->entitiesCount; i++) {
            TR::Entity &e = level->entities[i];
            if (!e.flags.active || !e.isEnemy()) continue;
            Character *enemy = (Character*)e.controller;
            if (enemy->health <= 0) continue;

            Box box = enemy->getBoundingBox();
            vec3 p = box.center();
            p.y = box.min.y + (box.max.y - box.min.y) / 3.0f;
            
            vec3 v = p - pos;
            if (dir.dot(v.normal()) <= 0.5f) continue; // target is out of sight -60..+60 degrees

            float d = v.length();

            if ((d > dist[0] && d > dist[1]) || !checkOcclusion(from, p, d)) 
                continue;

            if (d < dist[0]) {
                target2 = target1;
                dist[1] = dist[0];
                target1 = i;
                dist[0] = d;
            } else if (d < dist[1]) {
                target2 = i;
                dist[1] = d;
            }
        }

        if (target2 == -1 || dist[1] > dist[0] * 4)
            target2 = target1;
    }

    bool checkOcclusion(const vec3 &from, const vec3 &to, float dist) {
        int room;
        vec3 d = trace(getRoomIndex(), from, to, room, false); // check occlusion
        return ((d - from).length() > (dist - 512.0f));
    }

    bool checkHit(int target, const vec3 &from, const vec3 &to, vec3 &point) {
        TR::Entity &e = level->entities[target];
        Controller *controller = (Controller*)e.controller;

        Box box = controller->getBoundingBoxLocal();
        mat4 m  = controller->getMatrix();

        float t;
        vec3 v = to - from;
        
        if (box.intersect(m, from, v, t)) {
            v = v.normal();
            Sphere spheres[34];
            int count;
            controller->getSpheres(spheres, count);
            for (int i = 0; i < count; i++) 
                if (spheres[i].intersect(from, v, t)) {
                    point = from + v * t;
                    return true;
                }
        }
        return false;
    }

    virtual void cmdEmpty() {
        wpnHide();
    }

    virtual void cmdOffset(const vec3 &offset) {
        Character::cmdOffset(offset);
        move();
    }

    virtual void cmdJump(const vec3 &vel) {
        vec3 v = vel;
        if (state == STATE_HANG_UP)
            v.y = (3.0f - sqrtf(-2.0f * GRAVITY / 30.0f * (collision.info[Collision::FRONT].floor - pos.y + 800.0f - 128.0f)));
        Character::cmdJump(v);
    }

    void drawGun(int right) {
        int mask = right ? BODY_ARM_R3 : BODY_ARM_L3; // unholster
        if (layers[1].mask & mask)
            mask = right ? BODY_LEG_R1 : BODY_LEG_L1; // holster
        meshSwap(1, level->extra.weapons[wpnCurrent], mask);
    }

    virtual void cmdEffect(int fx) {

        switch (fx) {
            case TR::EFFECT_FLIP_MAP       : break; // TODO
            case TR::EFFECT_LARA_HANDSFREE : break;//meshSwap(1, level->extra.weapons[wpnCurrent], BODY_LEG_L1 | BODY_LEG_R1); break;
            case TR::EFFECT_DRAW_RIGHTGUN  :
            case TR::EFFECT_DRAW_LEFTGUN   : drawGun(fx == TR::EFFECT_DRAW_RIGHTGUN); break;
            default : LOG("unknown effect command %d (anim %d)\n", fx, animation.index);
        }
    }

    virtual void hit(float damage, Controller *enemy = NULL) {
        if (dozy) return;

        damageTime = LARA_DAMAGE_TIME;

        Character::hit(damage, enemy);
        if (damage == 10000) { // T-Rex attack (fatal)
            pos   = enemy->pos;
            angle = enemy->angle;

            meshSwap(1, TR::MODEL_LARA_SPEC, BODY_UPPER | BODY_LOWER);
            meshSwap(2, level->extra.weapons[Weapon::SHOTGUN], 0);
            meshSwap(3, level->extra.weapons[Weapon::UZIS],    0);

            animation.setAnim(level->models[TR::MODEL_LARA_SPEC].animation + 1);
        }

        if (health <= 0)
            Core::lightColor[1 + 0] = Core::lightColor[1 + 1] = vec4(0, 0, 0, 1);
    };

    bool useItem(TR::Entity::Type item) {
        switch (item) {
            case TR::Entity::INV_PISTOLS       : wpnChange(Lara::Weapon::PISTOLS); break;
            case TR::Entity::INV_SHOTGUN       : wpnChange(Lara::Weapon::SHOTGUN); break;
            case TR::Entity::INV_MAGNUMS       : wpnChange(Lara::Weapon::MAGNUMS); break;
            case TR::Entity::INV_UZIS          : wpnChange(Lara::Weapon::UZIS);    break;
            case TR::Entity::INV_MEDIKIT_SMALL :
            case TR::Entity::INV_MEDIKIT_BIG   :
                damageTime = LARA_DAMAGE_TIME;
                health = min(LARA_MAX_HEALTH, health + (item == TR::Entity::INV_MEDIKIT_SMALL ? LARA_MAX_HEALTH / 2 : LARA_MAX_HEALTH));
                playSound(TR::SND_HEALTH, pos, Sound::PAN);
                break;
            case TR::Entity::INV_PUZZLE_1 :
            case TR::Entity::INV_PUZZLE_2 :
            case TR::Entity::INV_PUZZLE_3 :
            case TR::Entity::INV_PUZZLE_4 :
            case TR::Entity::INV_KEY_1    :
            case TR::Entity::INV_KEY_2    :
            case TR::Entity::INV_KEY_3    :
            case TR::Entity::INV_KEY_4    :
                if (usedKey == item)
                    return false;
                usedKey = item;
                break;
            default : return false;
        }
        return true;
    }

    bool waterOut() {
        // TODO: playSound 36
        if (collision.side != Collision::FRONT || pos.y - collision.info[Collision::FRONT].floor > 256 + 128)
            return false;

        vec3 dst = pos + getDir() * (LARA_RADIUS + 32.0f);

        TR::Level::FloorInfo info;
        level->getFloorInfo(getRoomIndex(), int(pos.x), int(pos.y), int(pos.z), info);
        int roomAbove = info.roomAbove;
        if (roomAbove == TR::NO_ROOM)
            return false;

        level->getFloorInfo(roomAbove, int(dst.x), int(dst.y), int(dst.z), info);

        int h = int(pos.y - info.floor);

        if (h >= 0 && h <= (256 + 128) && (state == STATE_SURF_TREAD || animation.setState(STATE_SURF_TREAD)) && animation.setState(STATE_STOP)) {
            alignToWall(LARA_RADIUS);
            roomPrev = getRoomIndex();
            getEntity().room = roomAbove;
            pos.y    = float(info.floor);
            specular = LARA_WET_SPECULAR;
            updateEntity();
            move();
            return true;
        }

        return false;
    }

    bool doPickUp() {
        if ((state != STATE_STOP && state != STATE_TREAD) || !animation.canSetState(STATE_PICK_UP))
            return false;

        int room = getRoomIndex();
        TR::Limits::Limit limit = state == STATE_STOP ? TR::Limits::PICKUP : TR::Limits::PICKUP_UNDERWATER;

        for (int i = 0; i < level->entitiesCount; i++) {
            TR::Entity &item = level->entities[i];
            if (item.room == room && !item.flags.invisible) {
                if (!item.isItem())
                    continue;
                
                Controller *controller = (Controller*)item.controller;
                
                if (stand == STAND_UNDERWATER)
                    controller->angle.x = -25 * DEG2RAD;
                controller->angle.y = angle.y;

                if (!checkInteraction(controller, limit, (input & ACTION) != 0))
                    continue;

                alignByItem(controller, limit, true, false);

                if (stand == STAND_UNDERWATER)
                    angle.x = -25 * DEG2RAD;

                pickupEntity = &item;
                return true;
            }
        }
        return false;
    }

    void alignByItem(Controller *item, const TR::Limits::Limit &limit, bool dx, bool ay) {
        if (ay)
            angle = item->angle;
        else
            angle.x = angle.z = 0.0f;

        mat4 m = item->getMatrix();

        float fx = 0.0f;
        if (!dx)
            fx = (m.transpose() * vec4(pos - item->pos, 0.0f)).x;

        pos      = item->pos + (m * vec4(fx, limit.dy, limit.dz, 0.0f)).xyz;
        velocity = vec3(0.0f);
        speed    = 0.0f;
        updateEntity();
    }

    bool checkInteraction(Controller *controller, const TR::Limits::Limit &limit, bool action) {
        if ((state != STATE_STOP && state != STATE_TREAD && state != STATE_PUSH_PULL_READY) || !action || !emptyHands())
            return false;

        vec3 delta = (controller->getMatrix().transpose() * vec4(pos - controller->pos, 0.0f)).xyz; // inverse transform
        
        return limit.box.contains(delta) && fabsf(shortAngle(angle.y, controller->angle.y)) <= limit.ay * DEG2RAD;
    }

    void checkTrigger() {
        if (actionCommand) return;

        TR::Entity &e = getEntity();
        TR::Level::FloorInfo info;
        level->getFloorInfo(e.room, e.x, e.y, e.z, info);

        if (!info.trigCmdCount) return; // has no trigger

        TR::FloorData::TriggerCommand &cmd = info.trigCmd[0];
        bool isActive = false;
        switch (cmd.action) {
            case TR::Action::SECRET   : isActive = level->secrets[cmd.args]; break;
            case TR::Action::ACTIVATE : isActive = level->entities[cmd.args].flags.active != 0; break;
            default : isActive = false;
        }

        if (info.trigInfo.once == 1 && isActive) return; // once trigger is already activated

        TR::Limits::Limit *limit = NULL;

        int actionState = state;
        switch (info.trigger) {
            case TR::Level::Trigger::ACTIVATE :
                if (isActive) return;
                break;
            case TR::Level::Trigger::PAD :
                if (stand != STAND_GROUND || isActive) return;
                break;
            case TR::Level::Trigger::SWITCH :
                actionState = (isActive && stand == STAND_GROUND) ? STATE_SWITCH_UP : STATE_SWITCH_DOWN;
                if (!animation.canSetState(actionState))
                    return;
                limit = state == STATE_STOP ? &TR::Limits::SWITCH : &TR::Limits::SWITCH_UNDERWATER;
                {
                    Trigger *controller = (Trigger*)level->entities[info.trigCmd[0].args].controller;
                    if (!controller->inState() || !checkInteraction(controller, *limit, isPressed(ACTION)))
                        return;
                }
                break;
            case TR::Level::Trigger::KEY :
                if (level->entities[info.trigCmd[0].args].flags.active || state != STATE_STOP)
                    return;

                actionState = level->entities[info.trigCmd[0].args].isKeyHole() ? STATE_USE_KEY : STATE_USE_PUZZLE;
                if (!animation.canSetState(actionState))
                    return;

                limit = actionState == STATE_USE_PUZZLE ? &TR::Limits::PUZZLE_HOLE : &TR::Limits::KEY_HOLE;
                if (!checkInteraction((Controller*)level->entities[info.trigCmd[0].args].controller, *limit, isPressed(ACTION) || usedKey != TR::Entity::NONE))
                    return;

                if (!animation.canSetState(actionState))
                    return;

                if (usedKey == TR::Entity::NONE) {
                    if (isPressed(ACTION) && !game->invChooseKey(level->entities[info.trigCmd[0].args].type))
                        playSound(TR::SND_NO, pos, Sound::PAN); // no compatible items in inventory
                    return;
                }

                if (TR::Entity::convToInv(TR::Entity::getKeyForHole(level->entities[info.trigCmd[0].args].type)) != usedKey) { // check compatibility if user select other
                    playSound(TR::SND_NO, pos, Sound::PAN); // uncompatible item
                    return;
                }

                puzzleEntity = actionState == STATE_USE_PUZZLE ? &level->entities[info.trigCmd[0].args] : NULL;
                game->invUse(usedKey);
                break;
            case TR::Level::Trigger::PICKUP :
                if (!isActive)  // check if item is not picked up
                    return;
                break;
            default :
                LOG("unsupported trigger type %d\n", info.trigger);
                return;
        }

        // try to activate Lara state
        if (!animation.setState(actionState)) return;

        if (info.trigger == TR::Level::Trigger::KEY)
            level->entities[info.trigCmd[0].args].flags.active = true;

        if (limit)
            alignByItem((Controller*)level->entities[info.trigCmd[0].args].controller, *limit, stand != STAND_GROUND || info.trigger != TR::Level::Trigger::SWITCH, true);

        // build trigger activation chain
        ActionCommand *actionItem = &actionList[1];

        Controller *controller = this;
        for (int i = 0; i < info.trigCmdCount; i++) {
            if (!controller) {
                LOG("! next activation entity %d has no controller\n", level->entities[info.trigCmd[i].args].type);
                playSound(TR::SND_NO, pos, 0);
                return;
            }

            if (info.trigger == TR::Level::Trigger::KEY && i == 0) continue; // skip key שך puzzle hole

            TR::FloorData::TriggerCommand &cmd = info.trigCmd[i];
            switch (cmd.action) {
                case TR::Action::CAMERA_SWITCH :
                    *actionItem = ActionCommand(entity, cmd.action, cmd.args, (float)info.trigCmd[++i].delay);    // camera switch uses next command for delay timer
                    break;
                default :
                    *actionItem = ActionCommand(entity, cmd.action, cmd.args, info.trigInfo.timer);
            }

            actionItem->next = (i < info.trigCmdCount - 1) ? actionItem + 1 : NULL;
            actionItem++;
        }

        actionList[0].next = &actionList[1];
        actionCommand = &actionList[0];

        if (info.trigger != TR::Level::Trigger::KEY)
            activateNext();
    }

    vec3 getViewPoint() {
        vec3 offset = chestOffset = animation.getJoints(getMatrix(), 7).pos;
        if (stand != STAND_UNDERWATER)
            offset.y -= 256.0f;
        if (!emptyHands())
            offset.y -= 256.0f;

        return offset;
    }

    virtual Stand getStand() {
        if (dozy) return STAND_UNDERWATER;

        if (state == STATE_HANG || state == STATE_HANG_LEFT || state == STATE_HANG_RIGHT) {
            if (input & ACTION)
                return STAND_HANG;
            animation.setAnim(ANIM_HANG_FALL);
            velocity = vec3(0.0f);
            pos.y += 128.0f;
            updateEntity();
            return STAND_AIR;
        }

        if (state == STATE_HANDSTAND || state == STATE_HANG_UP)
            return STAND_HANG;

        if (stand == STAND_ONWATER && state != STATE_DIVE && state != STATE_STOP)
            return stand;

        if (getRoom().flags.water) {
            wpnHide();
            return STAND_UNDERWATER;
        }

        TR::Entity &e = getEntity();
        TR::Level::FloorInfo info;
        level->getFloorInfo(e.room, e.x, e.y, e.z, info);

        if (stand == STAND_SLIDE || (stand == STAND_AIR && velocity.y > 0) || stand == STAND_GROUND) {
            if (e.y + 8 >= info.floor && (abs(info.slantX) > 2 || abs(info.slantZ) > 2)) {
                if (stand == STAND_AIR)
                    playSound(TR::SND_LANDING, pos, Sound::Flags::PAN);
                pos.y = float(info.floor);
                updateEntity();

                if (stand == STAND_GROUND || stand == STAND_AIR)
                    slideStart();

                return STAND_SLIDE;
            }
        }

        int extra = stand != STAND_AIR ? 256 : 0;

        if (e.y + extra >= info.floor && !(stand == STAND_AIR && velocity.y < 0)) {
            if (stand != STAND_GROUND) {
                pos.y = float(info.floor);
                updateEntity();
            }
            return STAND_GROUND;
        }

        return STAND_AIR;
    }

    virtual int getHeight() {
        if (stand == STAND_GROUND || stand == STAND_AIR)
            return 768;
        return 0;
    }

    virtual int getStateAir() {
        angle.x = 0.0f;

        if (state == STATE_REACH && getDir().dot(vec3(velocity.x, 0.0f, velocity.z)) < 0)
            velocity.x = velocity.z = 0.0f;

        if ((state == STATE_REACH || state == STATE_UP_JUMP) && (input & ACTION) && emptyHands()) {
            if (state == STATE_REACH && velocity.y < 0.0f)
                return state;

            Box bounds = animation.getBoundingBox(pos, 0);

            vec3 p = vec3(pos.x, bounds.min.y, pos.z);

            Collision c = Collision(level, getRoomIndex(), p, getDir() * 32.0f, vec3(0.0f), LARA_RADIUS, angleExt, 0, 0, 0, 0);

            if (c.side != Collision::FRONT)
                return state;

            int floor   = c.info[Collision::FRONT].floor;
            int ceiling = c.info[Collision::FRONT].ceiling;
            int hands = int(bounds.min.y);

            if (abs(floor - hands) < 64 && floor != ceiling) {
                alignToWall(-LARA_RADIUS);
                pos.y = float(floor + LARA_HANG_OFFSET);
                stand = STAND_HANG;
                updateEntity();

                if (state == STATE_REACH) {
                    vec3 p = pos + getDir() * 256.0f;
                    TR::Level::FloorInfo info;
                    level->getFloorInfo(getRoomIndex(), int(p.x), int(p.y), int(p.z), info);
                    int h = info.ceiling - floor;
                    return animation.setAnim((h > 0 && h < 400) ? ANIM_HANG_SWING : ANIM_HANG);
                } else
                    return animation.setAnim(ANIM_HANG, -15);
            }
        }

        if (state == STATE_FORWARD_JUMP) {
            if (emptyHands()) {
                if (input & ACTION) return STATE_REACH;
                if ((input & (FORTH | WALK)) == (FORTH | WALK)) return STATE_SWAN_DIVE;
            }
        } else
            if (state != STATE_SWAN_DIVE && state != STATE_REACH && state != STATE_FALL && state != STATE_UP_JUMP && state != STATE_BACK_JUMP && state != STATE_LEFT_JUMP && state != STATE_RIGHT_JUMP)
                return animation.setAnim(ANIM_FALL);

        return state;
    }

    int entityQuadrant(const TR::Entity &entity) {
        int ix = int(pos.x) / 1024;
        int iz = int(pos.z) / 1024;

        int bx = entity.x / 1024;
        int bz = entity.z / 1024;

        int q = -1;
        if (abs(bx - ix) ^ abs(bz - iz)) {
            if (bx > ix) q = 1;
            if (bx < ix) q = 3;
            if (bz > iz) q = 0;
            if (bz < iz) q = 2;
        }

        return q;
    }

    Block* getBlock() {
        for (int i = 0; i < level->entitiesCount; i++) {
            TR::Entity &e = level->entities[i];
            if (!e.isBlock())
                continue;

            Block *block = (Block*)e.controller;
            float oldAngle = block->angle.y;
            block->angle.y = angleQuadrant(angle.y) * (PI * 0.5f);

            if (!checkInteraction(block, TR::Limits::BLOCK, (input & ACTION) != 0)) {
                block->angle.y = oldAngle;
                continue;
            }

            alignByItem(block, TR::Limits::BLOCK, false, true);

            return block;
        }
        return NULL;
    }

    virtual int getStateGround() {
        int res = STATE_STOP;
        angle.x = 0.0f;

        if ((state == STATE_STOP || state == STATE_TREAD) && (input & ACTION) && emptyHands() && doPickUp())
            return STATE_PICK_UP;

        if ((input & (FORTH | ACTION)) == (FORTH | ACTION) && (animation.index == ANIM_STAND || animation.index == ANIM_STAND_NORMAL) && emptyHands() && collision.side == Collision::FRONT) { // TODO: get rid of animation.index
            int floor   = collision.info[Collision::FRONT].floor;
            int ceiling = collision.info[Collision::FRONT].ceiling; 

            int h = (int)pos.y - floor;

            int aIndex = animation.index;
            if (floor == ceiling || h < 256)
                ;// do nothing
            else if (h <= 2 * 256 + 128) {
                aIndex = ANIM_CLIMB_2;
                pos.y  = floor + 512.0f;
            } else if (h <= 3 * 256 + 128) {
                aIndex = ANIM_CLIMB_3;
                pos.y  = floor + 768.0f;
            } else if (h <= 7 * 256 + 128)
                aIndex = ANIM_CLIMB_JUMP;

            if (aIndex != animation.index) {
                alignToWall(-LARA_RADIUS);
                return animation.setAnim(aIndex);
            }
        }

        if ( (input & (FORTH | BACK)) == (FORTH | BACK) && (animation.index == ANIM_STAND_NORMAL || state == STATE_RUN) )
            return animation.setAnim(ANIM_STAND_ROLL_BEGIN);

        // ready to jump
        if (state == STATE_COMPRESS) {
            float   ext = angle.y;
            switch (input & (RIGHT | LEFT | FORTH | BACK)) {
                case RIGHT         : res = STATE_RIGHT_JUMP;    ext +=  PI * 0.5f; break;
                case LEFT          : res = STATE_LEFT_JUMP;     ext -=  PI * 0.5f; break;
                case FORTH | LEFT  :
                case FORTH | RIGHT :
                case FORTH         : res = STATE_FORWARD_JUMP;  break;
                case BACK          : res = STATE_BACK_JUMP;     ext +=  PI;        break;
                default            : res = STATE_UP_JUMP;       break;
            }

            if (res != STATE_UP_JUMP) {
                vec3 p = pos;
                collision  = Collision(level, getRoomIndex(), p, vec3(0.0f), vec3(0.0f), LARA_RADIUS * 2.5f, ext, 0, LARA_HEIGHT, 256 + 128, 0xFFFFFF);
                if (collision.side == Collision::FRONT)
                    res = STATE_UP_JUMP;
            }

            return res;
        }

        // jump button is pressed
        if (input & JUMP) {
            if ((input & FORTH) && state == STATE_FORWARD_JUMP)
                return STATE_RUN;
            if (state == STATE_RUN)
                return STATE_FORWARD_JUMP;
            if (animation.index == ANIM_SLIDE_BACK) // TODO: animation index? %)
                return STATE_SLIDE_BACK;
            return STATE_COMPRESS;
        }

        // walk button is pressed
        if ((input & WALK) && animation.index != ANIM_RUN_START) {
            float ext = angle.y;

            if (input & FORTH) { 
                if (state == STATE_BACK)
                    res = STATE_STOP;
                else
                    res = STATE_WALK;
            } else if (input & BACK) {
                res = STATE_BACK;
                ext += PI;
            } else if (input & LEFT) {
                res = STATE_STEP_LEFT;
                ext -= PI * 0.5f;
            } else if (input & RIGHT) {
                res = STATE_STEP_RIGHT;
                ext += PI * 0.5f;
            }

            int maxAscent  = 256 + 128;
            int maxDescent = maxAscent;

            if (state == STATE_STEP_LEFT || state == STATE_STEP_RIGHT)
                maxAscent = maxDescent = 64;

            if (state == STATE_STOP && res != STATE_STOP) {
                vec3 p = pos;
                collision  = Collision(level, getRoomIndex(), p, vec3(0.0f), vec3(0.0f), LARA_RADIUS * 1.1f, ext, 0, LARA_HEIGHT, maxAscent, maxDescent);
                if (collision.side == Collision::FRONT)
                    res = STATE_STOP;
            }

            return res;
        }

        if ((input & ACTION) && emptyHands()) {
            if (state == STATE_PUSH_PULL_READY && (input & (FORTH | BACK))) {
                int pushState = (input & FORTH) ? STATE_PUSH_BLOCK : STATE_PULL_BLOCK;
                Block *block = getBlock();
                if (block && animation.canSetState(pushState) && block->doMove((input & FORTH) != 0))
                    return pushState;
            }

            if (state == STATE_PUSH_PULL_READY || getBlock())
                return STATE_PUSH_PULL_READY;
        }

        // only dpad buttons pressed
        if (input & FORTH)
            res = STATE_RUN;
        else if (input & BACK)
            res = STATE_FAST_BACK;
        else if (input & (LEFT | RIGHT)) {
            if (state == STATE_FAST_TURN)
                return state;

            if (input & LEFT)  return (state == STATE_TURN_LEFT  && animation.prev == animation.index) ? STATE_FAST_TURN : STATE_TURN_LEFT;
            if (input & RIGHT) return (state == STATE_TURN_RIGHT && animation.prev == animation.index) ? STATE_FAST_TURN : STATE_TURN_RIGHT;
        }

        if (state == STATE_STOP && res != STATE_STOP) {
            float ext = angle.y + (res == STATE_RUN ? 0.0f : PI);
            vec3 p = pos;
            collision  = Collision(level, getRoomIndex(), p, vec3(0.0f), vec3(0.0f), LARA_RADIUS * 1.1f, ext, 0, LARA_HEIGHT, 256 + 128, 0xFFFFFF);
            if (collision.side == Collision::FRONT)
                res = STATE_STOP;
        }

        return res;
    }

    void slideStart() {
        TR::Entity &e = getEntity();

        if (state != STATE_SLIDE && state != STATE_SLIDE_BACK) {
            TR::Level::FloorInfo info;
            level->getFloorInfo(e.room, e.x, e.y, e.z, info);

            int sx = abs(info.slantX), sz = abs(info.slantZ);
            // get direction
            float dir;
            if (sx > sz)
                dir = info.slantX > 0 ? 3.0f : 1.0f;
            else
                dir = info.slantZ > 0 ? 2.0f : 0.0f;
            dir *= PI * 0.5f;

            int aIndex = ANIM_SLIDE_FORTH;
            if (fabsf(shortAngle(dir, angle.y)) > PI * 0.5f) {
                aIndex = ANIM_SLIDE_BACK;
                dir += PI;
            }

            angle.y = dir;
            updateEntity();
            animation.setAnim(aIndex);
        }
    }

    virtual int getStateSlide() {
        if (input & JUMP)
            return state == STATE_SLIDE ? STATE_FORWARD_JUMP : STATE_BACK_JUMP;
        return state;
    }

    virtual int getStateHang() {
        if (input & LEFT)  return STATE_HANG_LEFT;
        if (input & RIGHT) return STATE_HANG_RIGHT;
        if (input & FORTH) {
            // possibility check
            TR::Level::FloorInfo info;
            vec3 p = pos + getDir() * (LARA_RADIUS + 2.0f);
            level->getFloorInfo(getRoomIndex(), (int)p.x, (int)p.y, (int)p.z, info);
            if (info.floor - info.ceiling >= LARA_HEIGHT)
                return (input & WALK) ? STATE_HANDSTAND : STATE_HANG_UP;            
        }
        return STATE_HANG;
    }

    virtual int getStateUnderwater() {
        if (input == ACTION && doPickUp())
            return STATE_PICK_UP;

        if (state == STATE_FORWARD_JUMP || state == STATE_UP_JUMP || state == STATE_BACK_JUMP || state == STATE_LEFT_JUMP || state == STATE_RIGHT_JUMP || state == STATE_FALL || state == STATE_REACH || state == STATE_SLIDE || state == STATE_SLIDE_BACK) {
            game->waterDrop(pos, 256.0f, 0.2f);
            Sprite::add(game, TR::Entity::WATER_SPLASH, getRoomIndex(), (int)pos.x, (int)pos.y, (int)pos.z);
            pos.y += 100.0f;
            angle.x = -45.0f * DEG2RAD;
            return animation.setAnim(ANIM_WATER_FALL); // TODO: wronng animation
        }

        if (state == STATE_SWAN_DIVE) {
            angle.x = -PI * 0.5f;
            game->waterDrop(pos, 128.0f, 0.2f);
            Sprite::add(game, TR::Entity::WATER_SPLASH, getRoomIndex(), (int)pos.x, (int)pos.y, (int)pos.z);
            return STATE_DIVE;
        }

        if (input & JUMP) return STATE_SWIM;

        if (state == STATE_GLIDE && speed < LARA_SWIM_SPEED * 2.0f / 3.0f)
            return STATE_TREAD;

        return STATE_GLIDE;
    }

    virtual int getStateOnwater() {
        angle.x = 0.0f;

        if (state == STATE_WATER_OUT) return state;

        if (state != STATE_SURF_TREAD && state != STATE_SURF_LEFT && state != STATE_SURF_RIGHT && state != STATE_SURF_SWIM && state != STATE_SURF_BACK && state != STATE_STOP) {
            game->waterDrop(pos, 128.0f, 0.2f);
            specular = LARA_WET_SPECULAR;
            return animation.setAnim(ANIM_TO_ONWATER);
        }

        if (state == STATE_SURF_TREAD) {
            if (animation.isFrameActive(0))
                game->waterDrop(animation.getJoints(getMatrix(), 14).pos, 96.0f, 0.03f);
        } else {
            if (animation.frameIndex % 4 == 0)
                game->waterDrop(animation.getJoints(getMatrix(), 14).pos, 96.0f, 0.02f);
        }

        if (input & FORTH) {
            if (input & JUMP) {
                angle.x = -PI * 0.25f;
                game->waterDrop(pos, 256.0f, 0.2f);
                return animation.setAnim(ANIM_TO_UNDERWATER);
            }

            if ((input & ACTION) && waterOut()) {
                game->waterDrop(pos, 128.0f, 0.2f);
                return state;
            }

            return STATE_SURF_SWIM;
        }

        if (input & BACK)  return STATE_SURF_BACK;
        if (input & WALK) {
            if (input & LEFT)  return STATE_SURF_LEFT;
            if (input & RIGHT) return STATE_SURF_RIGHT;
        }
        return STATE_SURF_TREAD;
    }

    virtual int getStateDeath() {
        velocity = vec3(0.0f);
        return (stand == STAND_UNDERWATER || stand == STAND_ONWATER) ? STATE_UNDERWATER_DEATH : STATE_DEATH;
    }

    virtual int getStateDefault() {
        if (state == STATE_DIVE || (state == STATE_RUN && (input & JUMP)) ) return state;
        switch (stand) {
            case STAND_GROUND     : return STATE_STOP;
            case STAND_HANG       : return STATE_HANG;
            case STAND_ONWATER    : return STATE_SURF_TREAD;
            case STAND_UNDERWATER : return STATE_TREAD;
            default : ;
        }
        return STATE_FALL;
    }

    virtual int getInput() { // TODO: updateInput
        if (level->cutEntity > -1) return 0;
        input = 0;

        if (!dozy && Input::state[cAction] && Input::state[cJump] && Input::state[cLook] && Input::state[cStepRight]) {
            dozy = true;
            reset(getRoomIndex(), pos - vec3(0, 512, 0), angle.y, STAND_UNDERWATER);
            return input;
        }

        if (dozy && Input::state[cWalk]) {
            dozy = false;
            return input;
        }

        input = Character::getInput();
        if (input & DEATH) return input;

        if (Input::state[cUp])        input |= FORTH;
        if (Input::state[cRight])     input |= RIGHT;
        if (Input::state[cDown])      input |= BACK;
        if (Input::state[cLeft])      input |= LEFT;
        if (Input::state[cRoll])      input  = FORTH | BACK;
        if (Input::state[cStepRight]) input  = WALK  | RIGHT;
        if (Input::state[cStepLeft])  input  = WALK  | LEFT;
        if (Input::state[cJump])      input |= JUMP;
        if (Input::state[cWalk])      input |= WALK;
        if (Input::state[cAction])    input |= ACTION;
        if (Input::state[cWeapon])    input |= WEAPON;

    // analog control
        rotFactor = vec2(1.0f);

        if ((state == STATE_STOP || state == STATE_SURF_TREAD || state == STATE_HANG) && fabsf(Input::joy.L.x) < 0.5f && fabsf(Input::joy.L.y) < 0.5f)
            return input;

        bool moving = state == STATE_RUN || state == STATE_WALK || state == STATE_BACK || state == STATE_FAST_BACK || state == STATE_SURF_SWIM || state == STATE_SURF_BACK || state == STATE_FORWARD_JUMP;

        if (!moving) {
            if (fabsf(Input::joy.L.x) < fabsf(Input::joy.L.y))
                Input::joy.L.x = 0.0f;
            else
                Input::joy.L.y = 0.0f;
        }

        if (Input::joy.L.x != 0.0f) {
            input |= (Input::joy.L.x < 0.0f) ? LEFT : RIGHT;
            if (moving || stand == STAND_UNDERWATER || stand == STAND_ONWATER)
                rotFactor.y = min(fabsf(Input::joy.L.x) / 0.9f, 1.0f);
        }

        if (Input::joy.L.y != 0.0f) {
            input |= (Input::joy.L.y < 0.0f) ? FORTH : BACK;
            if (stand == STAND_UNDERWATER)
                rotFactor.x = min(fabsf(Input::joy.L.y) / 0.9f, 1.0f);
        }

        return input;
    }

    virtual void doCustomCommand(int curFrame, int prevFrame) {
        switch (state) {
            case STATE_PICK_UP : {
                if (pickupEntity && !pickupEntity->flags.invisible) {
                    int pickupFrame = stand == STAND_GROUND ? PICKUP_FRAME_GROUND : PICKUP_FRAME_UNDERWATER;
                    if (animation.isFrameActive(pickupFrame)) {
                        pickupEntity->flags.invisible = true;
                        game->invAdd(pickupEntity->type, 1);
                        pickupEntity = NULL;
                    }
                }
                break;
            }
            case STATE_USE_PUZZLE : {
                if (puzzleEntity && animation.isFrameActive(PUZZLE_FRAME)) {
                    int doneIdx = TR::Entity::convToInv(TR::Entity::getKeyForHole(puzzleEntity->type)) - TR::Entity::INV_PUZZLE_1;
                    ((Controller*)puzzleEntity->controller)->meshSwap(0, level->extra.puzzleDone[doneIdx]);
                    puzzleEntity = NULL;
                }
                break;
            }
        }
    }

    virtual void update() {
        Character::update();

        if (damageTime > 0.0f)
            damageTime = max(0.0f, damageTime - Core::deltaTime);

        if (stand == STAND_UNDERWATER && !dozy) {
            if (oxygen > 0.0f)
                oxygen -= Core::deltaTime;
            else
                hit(Core::deltaTime * 150.0f);
        } else
            if (oxygen < LARA_MAX_OXYGEN)
                oxygen = min(LARA_MAX_OXYGEN, oxygen += Core::deltaTime * 10.0f);

        usedKey = TR::Entity::NONE;
    }

    virtual void updateAnimation(bool commands) {
        Controller::updateAnimation(commands);
        updateWeapon();
        if (stand == STAND_UNDERWATER)
            specular = 0.0f;
        else
            if (specular > 0.0f)
                specular = max(0.0f, specular - LARA_WET_TIMER * Core::deltaTime);
    }

    virtual void updateVelocity() {
        checkTrigger();

    // get turning angle
        float w = (input & LEFT) ? -1.0f : ((input & RIGHT) ? 1.0f : 0.0f);

        if (state == STATE_SWIM || state == STATE_GLIDE)
            w *= TURN_WATER_FAST;
        else if (state == STATE_TREAD || state == STATE_SURF_TREAD || state == STATE_SURF_SWIM || state == STATE_SURF_BACK)
            w *= TURN_WATER_FAST;
        else if (state == STATE_RUN)
            w  *= sign(w) != sign(tilt) ? 0.0f : w * TURN_FAST * tilt / LARA_TILT_MAX;
        else if (state == STATE_FAST_TURN)
            w *= TURN_FAST;
        else if (state == STATE_FAST_BACK)
            w *= TURN_FAST_BACK;
        else if (state == STATE_TURN_LEFT || state == STATE_TURN_RIGHT || state == STATE_WALK)
            w *= TURN_NORMAL;
        else if (state == STATE_FORWARD_JUMP || state == STATE_BACK)
            w *= TURN_SLOW;
        else
            w = 0.0f;

        if (w != 0.0f)
            rotateY(w * rotFactor.y * Core::deltaTime);
    // pitch (underwater only)
        if (stand == STAND_UNDERWATER && (input & (FORTH | BACK)))
            rotateX(((input & FORTH) ? -TURN_WATER_SLOW : TURN_WATER_SLOW) * rotFactor.x * Core::deltaTime);

    // get animation direction
        angleExt = angle.y;
        switch (state) {
            case STATE_BACK :
            case STATE_SURF_BACK  :
            case STATE_BACK_JUMP  :
            case STATE_FAST_BACK  :
            case STATE_SLIDE_BACK :
            case STATE_ROLL_1     :
                angleExt += PI;
                break;
            case STATE_LEFT_JUMP  :
            case STATE_STEP_LEFT  :
            case STATE_SURF_LEFT  :
            case STATE_HANG_LEFT  :
                angleExt -= PI * 0.5f;
                break;
            case STATE_RIGHT_JUMP :
            case STATE_STEP_RIGHT :
            case STATE_SURF_RIGHT :
            case STATE_HANG_RIGHT :
                angleExt +=  PI * 0.5f;
                break;
        }

        switch (stand) {
            case STAND_AIR :
                velocity.y += GRAVITY * Core::deltaTime;
                break;
            case STAND_GROUND  :
            case STAND_SLIDE   :
            case STAND_HANG    :
            case STAND_ONWATER : {

                switch (state) {
                    case STATE_SURF_SWIM  :
                    case STATE_SURF_BACK  :
                    case STATE_SURF_LEFT  :
                    case STATE_SURF_RIGHT :
                        speed = min(speed + 30.0f * LARA_WATER_ACCEL * Core::deltaTime, LARA_SURF_SPEED);
                        break;
                    default :
                        speed = animation.getSpeed();
                }

                if (stand == STAND_ONWATER) {
                    velocity.x = sinf(angleExt) * speed;
                    velocity.z = cosf(angleExt) * speed;
                    velocity.y = 0.0f;
                } else {
                    TR::Entity &e = getEntity();
                    TR::Level::FloorInfo info;
                    if (stand == STAND_HANG) {
                        vec3 p = pos + getDir() * (LARA_RADIUS + 2.0f);
                        level->getFloorInfo(e.room, (int)p.x, (int)p.y, (int)p.z, info);
                        if (info.roomAbove != TR::NO_ROOM && info.floor >= e.y - LARA_HANG_OFFSET)
                            level->getFloorInfo(info.roomAbove, (int)p.x, (int)p.y, (int)p.z, info);
                    } else
                        level->getFloorInfo(e.room, e.x, e.y, e.z, info);

                    vec3 v(sinf(angleExt), 0.0f, cosf(angleExt));
                    velocity = info.getSlant(v) * speed;
                }
                break;
            }
            case STAND_UNDERWATER : {
                if (animation.index == ANIM_TO_UNDERWATER)
                    speed = 15.0f;
                if (state == STATE_SWIM)
                    speed = min(speed + 30.0f * LARA_WATER_ACCEL * Core::deltaTime, LARA_SWIM_SPEED);
                if (state == STATE_TREAD || state == STATE_GLIDE)
                    speed = max(speed - 30.0f * LARA_SWIM_FRICTION * Core::deltaTime, 0.0f);
                velocity = vec3(angle.x, angle.y) * speed;
                // TODO: apply flow velocity
                break;
            }
        }

        if (state == STATE_DEATH || state == STATE_UNDERWATER_DEATH)
            velocity.x = velocity.z = 0.0f;
    }

    virtual void updatePosition() { // TODO: sphere / bbox collision
        // tilt control
        vec2 vTilt(LARA_TILT_SPEED * Core::deltaTime, LARA_TILT_MAX);
        if (stand == STAND_UNDERWATER)
            vTilt *= 2.0f;
        vTilt *= rotFactor.y;
        updateTilt(state == STATE_RUN || stand == STAND_UNDERWATER, vTilt.x, vTilt.y);

        collisionOffset = vec3(0.0f);

        if (checkCollisions() || (velocity + collisionOffset).length2() >= 1.0f)
            move();

        if (getEntity().type != TR::Entity::LARA) {
            TR::Entity &e = getEntity();
            vec3 &p = getPos();
            e.x = int(p.x);
            e.y = int(p.y);
            e.z = int(p.z);
            checkRoom();
            updateEntity();
        }

        if (braid)
            braid->update();
    }

    virtual vec3& getPos() {
        return getEntity().type == TR::Entity::LARA ? pos : chestOffset;
    }

    bool checkCollisions() {
    // check static objects (TODO: check linked rooms?)
        TR::Room &room = getRoom();
        Box box(pos - vec3(LARA_RADIUS, LARA_HEIGHT, LARA_RADIUS), pos + vec3(LARA_RADIUS, 0.0f, LARA_RADIUS));

        for (int i = 0; i < room.meshesCount; i++) {
            TR::Room::Mesh &m  = room.meshes[i];
            TR::StaticMesh &sm = level->staticMeshes[m.meshIndex];
            if (sm.flags != 2) continue;
            Box meshBox;
            sm.getBox(true, m.rotation, meshBox);
            meshBox.translate(vec3(float(m.x), float(m.y), float(m.z)));
            if (!box.intersect(meshBox)) continue;

            collisionOffset += meshBox.pushOut2D(box);
        }

        if (!canHitAnim()) {
            hitDir = -1;
            return false;
        }

    // check enemies & doors
        for (int i = 0; i < level->entitiesCount; i++) {
            TR::Entity &e = level->entities[i];
            Controller *controller = (Controller*)e.controller;

            if (e.isEnemy()) {
                if (e.type != TR::Entity::ENEMY_REX && (!e.flags.active || ((Character*)e.controller)->health <= 0)) continue;
            } else 
                if (!e.isDoor()) continue;

            vec3 dir = pos - vec3(0.0f, 128.0f, 0.0f) - controller->pos;
            vec3 p   = dir.rotateY(controller->angle.y);

            Box box = controller->getBoundingBoxLocal();
            if (!box.contains(p))
                continue;

            if (e.isEnemy()) { // enemy collision
                if (!collide(controller, false))
                    continue;
                velocity.x = velocity.y = 0.0f;
            } else { // door collision
                p += box.pushOut2D(p);
                p = (p.rotateY(-controller->angle.y) + controller->pos) - pos;
                collisionOffset += vec3(p.x, 0.0f, p.z);
            }

            if (e.type == TR::Entity::ENEMY_REX && ((Character*)e.controller)->health <= 0)
                return true;
            if (e.isDoor())
                return true;

        // get hit dir
            if (hitDir == -1) {
                if (health > 0)
                    playSound(TR::SND_HIT, pos, Sound::PAN);
                hitTime = 0.0f;
            }

            hitDir = angleQuadrant(dir.rotateY(angle.y + PI * 0.5f).angleY());
            return true;
        }

        hitDir = -1;
        return false;
    }

    void move() {
        vec3 vel = velocity * Core::deltaTime * 30.0f + collisionOffset;
        vec3 opos(pos), offset(0.0f);

        float radius   = stand == STAND_UNDERWATER ? LARA_RADIUS_WATER : LARA_RADIUS;
        int maxHeight  = stand == STAND_UNDERWATER ? LARA_HEIGHT_WATER : LARA_HEIGHT;
        int minHeight  = 0;
        int maxAscent  = 256 + 128;
        int maxDescent = 0xFFFFFF;

        int room = getRoomIndex();

        if (state == STATE_WALK || state == STATE_BACK)
            maxDescent = maxAscent;
        if (state == STATE_STEP_LEFT || state == STATE_STEP_RIGHT)
            maxAscent = maxDescent = 64;
        if (stand == STAND_ONWATER) {
            maxAscent = -1;
            maxHeight =  0;
            offset.y  = -1;
        }

        bool standHang = stand == STAND_HANG && state != STATE_HANG_UP && state != STATE_HANDSTAND;

        if (standHang) {
            maxHeight = 0;
            maxAscent = maxDescent = 64;
            offset    = getDir() * (LARA_RADIUS + 32.0f);
            offset.y  -= LARA_HANG_OFFSET + 32;
        }
        if (stand == STAND_UNDERWATER) {
            offset.y += LARA_HEIGHT_WATER * 0.5f;
        }

        collision = Collision(level, room, pos, offset, vel, radius, angleExt, minHeight, maxHeight, maxAscent, maxDescent);

        if (!standHang && (collision.side == Collision::LEFT || collision.side == Collision::RIGHT)) {
            float rot = TURN_WALL_Y * Core::deltaTime;
            rotateY((collision.side == Collision::LEFT) ? rot : -rot);
        }

        if (standHang && collision.side != Collision::FRONT) {
            offset.x = offset.z = 0.0f;
            minHeight  = LARA_HANG_OFFSET;
            maxDescent = 0xFFFFFF;
            maxAscent  = -LARA_HANG_OFFSET;
            vec3 p = pos;
            collision  = Collision(level, room, p, offset, vec3(0.0f), radius, angleExt, minHeight, maxHeight, maxAscent, maxDescent);
            if (collision.side == Collision::FRONT)
                pos = opos;
        }

    // get current leading foot in animation
        int rightStart = 0;
        if (state == STATE_RUN)  rightStart = 6;
        if (state == STATE_WALK) rightStart = 13;
        if (state == STATE_BACK) rightStart = 28;
        bool isLeftFoot = animation.frameIndex < rightStart || animation.frameIndex > (rightStart + animation.framesCount / 2);


        if (stand == STAND_UNDERWATER) {
            if (collision.side == Collision::TOP)
                rotateX(-TURN_WALL_X * Core::deltaTime);
            if (collision.side == Collision::BOTTOM)
                rotateX( TURN_WALL_X * Core::deltaTime);
        }

        if (stand == STAND_AIR && collision.side == Collision::TOP && velocity.y < 0.0f)
            velocity.y = 30.0f;

        if (collision.side == Collision::FRONT) {
            int floor = collision.info[Collision::FRONT].floor;
/*
            switch (angleQuadrant(angleExt - angle.y)) {
                case 0 : collision.side = Collision::FRONT; LOG("FRONT\n"); break;
                case 1 : collision.side = Collision::RIGHT; LOG("RIGHT\n"); break;
                case 2 : collision.side = Collision::BACK;  LOG("BACK\n");  break;
                case 3 : collision.side = Collision::LEFT;  LOG("LEFT\n");  break;
            }
*/
            if (velocity.dot(getDir()) <= EPS) 
                collision.side = Collision::NONE;

        // hit the wall
            switch (stand) {
                case STAND_AIR :
                    if (state == STATE_UP_JUMP || state == STATE_REACH)
                        velocity.x = velocity.z = 0.0f;

                    if (velocity.x != 0.0f || velocity.z != 0.0f) {
                        animation.setAnim(ANIM_SMASH_JUMP);
                        velocity.x = -velocity.x * 0.25f;
                        velocity.z = -velocity.z * 0.25f;
                        if (velocity.y < 0.0f)
                            velocity.y = 30.0f;
                    }
                    break;
                case STAND_GROUND :
                case STAND_HANG   :
                    if (opos.y - floor > (256 * 3 - 128) && state == STATE_RUN)
                        animation.setAnim(isLeftFoot ? ANIM_SMASH_RUN_LEFT : ANIM_SMASH_RUN_RIGHT);
                    else if (stand == STAND_HANG)
                        animation.setAnim(ANIM_HANG, -21);
                    else if (state != STATE_ROLL_1 && state != STATE_ROLL_2)
                        animation.setAnim((state == STATE_RUN || state == STATE_WALK) ? (isLeftFoot ? ANIM_STAND_LEFT : ANIM_STAND_RIGHT) : ANIM_STAND);
                    velocity.x = velocity.z = 0.0f;
                    break;
                case STAND_UNDERWATER :
                    if (fabsf(angle.x) > TURN_WALL_X_CLAMP)
                        rotateX(TURN_WALL_X * Core::deltaTime * sign(angle.x));
                    else
                        pos.y = opos.y;
                    break;
                default : ;// no smash animation
            }
        } else {
            if (stand == STAND_GROUND) {
                int floor = collision.info[Collision::NONE].floor;
                int h = int(floor - opos.y);

                if (h >= 256 && state == STATE_FAST_BACK) {
                    stand = STAND_AIR;
                    animation.setAnim(ANIM_FALL);
                } else if (h >= 128 && (state == STATE_WALK || state == STATE_BACK)) { // descend
                    if (state == STATE_WALK) animation.setAnim(isLeftFoot ? ANIM_WALK_DESCEND_LEFT : ANIM_WALK_DESCEND_RIGHT);
                    if (state == STATE_BACK) animation.setAnim(isLeftFoot ? ANIM_BACK_DESCEND_LEFT : ANIM_BACK_DESCEND_RIGHT);
                    pos.y = float(floor);
                } else if (h > -1.0f) {
                    pos.y = min(float(floor), pos.y += DESCENT_SPEED * Core::deltaTime);
                } else if (h > -128) {
                    pos.y = float(floor);
                } else if (h >= -(256 + 128) && (state == STATE_RUN || state == STATE_WALK)) { // ascend
                    if (state == STATE_RUN)  animation.setAnim(isLeftFoot ? ANIM_RUN_ASCEND_LEFT  : ANIM_RUN_ASCEND_RIGHT);
                    if (state == STATE_WALK) animation.setAnim(isLeftFoot ? ANIM_WALK_ASCEND_LEFT : ANIM_WALK_ASCEND_RIGHT);
                    pos.y = float(floor);
                } else
                    pos.y = float(floor);
            }
            collision.side = Collision::NONE;
        }

        updateEntity();
        if (dozy) stand = STAND_GROUND;
        checkRoom();
        if (dozy) stand = STAND_UNDERWATER;
    }

    virtual void applyFlow(TR::Camera &sink) {
        if (stand != STAND_UNDERWATER && stand != STAND_ONWATER) return;
        vec3 v(0.0f);
        v.x = (float)sign((sink.x / 1024 - (int)pos.x / 1024));
        v.z = (float)sign((sink.z / 1024 - (int)pos.z / 1024));
        velocity = v * (sink.speed * 8.0f);
    }


    void renderMuzzleFlash(MeshBuilder *mesh, const Basis &basis, const vec3 &offset, float time) {
        ASSERT(level->extra.muzzleFlash);
        if (time > MUZZLE_FLASH_TIME) return;
        float alpha = min(1.0f, (0.1f - time) * 20.0f);
        float lum   = 3.0f;
        Basis b(basis);
        b.rotate(quat(vec3(1, 0, 0), -PI * 0.5f));
        b.translate(offset);
        Core::active.shader->setParam(uMaterial, vec4(lum, 0.0f, 0.0f, alpha));
        Core::active.shader->setParam(uBasis, b);
        mesh->renderModel(level->extra.muzzleFlash);
    }

    virtual void render(Frustum *frustum, MeshBuilder *mesh, Shader::Type type, bool caustics) {
        Controller::render(frustum, mesh, type, caustics);
        chestOffset = animation.getJoints(getMatrix(), 7).pos; // TODO: move to update func

        if (braid)
            braid->render(mesh);

        if (wpnCurrent != Weapon::SHOTGUN && Core::pass != Core::passShadow && (arms[0].shotTimer < MUZZLE_FLASH_TIME || arms[1].shotTimer < MUZZLE_FLASH_TIME)) {
            mat4 matrix = getMatrix();
            game->setShader(Core::pass, Shader::FLASH, false, true);
            Core::setBlending(bmAlpha);
            renderMuzzleFlash(mesh, animation.getJoints(matrix, 10, true), vec3(-10, -50, 150), arms[0].shotTimer);
            renderMuzzleFlash(mesh, animation.getJoints(matrix, 13, true), vec3( 10, -50, 150), arms[1].shotTimer);
            Core::setBlending(bmNone);
        }
    }
};

#endif
