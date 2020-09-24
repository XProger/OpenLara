#ifndef H_LARA
#define H_LARA
/*****************************************/
/*     Desine sperare qui hic intras     */
/*****************************************/
#include "character.h"
#include "objects.h"
#include "sprite.h"
#include "enemy.h"
#include "inventory.h"

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

#define LARA_MAX_HEALTH     1000
#define LARA_MAX_OXYGEN     1800

#define LARA_HANG_SLANT     60
#define LARA_HANG_OFFSET    724
#define LARA_HEIGHT         762
#define LARA_HEIGHT_JUMP    870 // LARA_HEIGHT + hands up
#define LARA_HEIGHT_UW      400
#define LARA_HEIGHT_SURF    700
#define LARA_RADIUS         100
#define LARA_RADIUS_WATER   300
#define LARA_RADIUS_CLIMB   220

#define LARA_WATER_ACCEL    2.0f
#define LARA_SURF_SPEED     15.0f
#define LARA_SWIM_SPEED     50.0f
#define LARA_SWIM_FRICTION  1.0f

#define LARA_WADE_MIN_DEPTH 384
#define LARA_WADE_MAX_DEPTH 730
#define LARA_SWIM_MIN_DEPTH 512.0f

#define LARA_MIN_SPECULAR   0.03f
#define LARA_WET_SPECULAR   0.5f
#define LARA_WET_TIMER      (LARA_WET_SPECULAR / 16.0f)   // 4 sec

#define LARA_DAMAGE_TIME    (40.0f / 30.0f)

#define PICKUP_FRAME_GROUND     40
#define PICKUP_FRAME_UNDERWATER 18
#define PUZZLE_FRAME            80
#define KEY_FRAME               110

#define MAX_TRIGGER_ACTIONS 64

#define DESCENT_SPEED       2048.0f
#define TARGET_MAX_DIST     (8.0f * 1024.0f)

#define UNITS_PER_METER     445.0f

#define LARA_VIBRATE_HIT_TIME   0.2f

#define COLLIDE_MAX_RANGE   (1024.0f * 4.0f)

#define DECL_S_HANDLER(v) &Lara::s_##v, 
#define DECL_C_HANDLER(v) &Lara::c_##v, 
#define S_HANDLER(state) void s_##state()
#define C_HANDLER(state) void c_##state()

struct Lara : Character {

    // http://www.tombraiderforums.com/showthread.php?t=148859
    enum {
        ANIM_RUN                = 0,

        ANIM_STAND_LEFT         = 2,
        ANIM_STAND_RIGHT        = 3,

        ANIM_RUN_START          = 6,

        ANIM_STAND              = 11,

        ANIM_LANDING            = 24,

        ANIM_CLIMB_JUMP         = 26,

        ANIM_FALL_HANG          = 28,

        ANIM_SMASH_JUMP         = 32,

        ANIM_FALL_FORTH         = 34,

        ANIM_BACK               = 41,
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

        ANIM_UW_GLIDE           = 87,

        ANIM_FALL_BACK          = 93,

        ANIM_HANG               = 96,

        ANIM_STAND_NORMAL       = 103,

        ANIM_SLIDE_BACK         = 104,

        ANIM_UNDERWATER         = 108,

        ANIM_WATER_OUT          = 111,
        ANIM_WATER_FALL         = 112,
        ANIM_SURF               = 114,
        ANIM_SURF_SWIM          = 116,
        ANIM_SURF_DIVE          = 119,

        ANIM_HIT_FRONT          = 125,
        ANIM_HIT_BACK           = 126,
        ANIM_HIT_LEFT           = 127,
        ANIM_HIT_RIGHT          = 128,

        ANIM_DEATH_BOULDER      = 139,
        ANIM_SURF_BACK          = 140,

        ANIM_SURF_LEFT          = 143,
        ANIM_SURF_RIGHT         = 144,

        ANIM_STAND_ROLL_BEGIN   = 146,
        ANIM_STAND_ROLL_END     = 147,

        ANIM_DEATH_SPIKES       = 149,
        ANIM_HANG_SWING         = 150,

        ANIM_CLIMB_START        = 164,

        ANIM_WADE_SWIM          = 176,
        ANIM_WADE               = 177,
        ANIM_WADE_RUN_LEFT      = 178,
        ANIM_WADE_RUN_RIGHT     = 179,
        ANIM_WADE_STAND         = 186,
        ANIM_WADE_ASCEND        = 190,
        ANIM_SURF_OUT           = 191,
        ANIM_SWIM_STAND         = 192,
        ANIM_SURF_STAND         = 193,

        ANIM_SWITCH_BIG_DOWN    = 195,
        ANIM_SWITCH_BIG_UP      = 196,
        ANIM_PUSH_BUTTON        = 197,

        ANIM_UW_ROLL         = 203,
    };

    // http://www.tombraiderforums.com/showthread.php?t=211681
    #define LARA_STATES(E) \
        /* TR1 ------------------*/ \
        E( STATE_WALK             ) \
        E( STATE_RUN              ) \
        E( STATE_STOP             ) \
        E( STATE_FORWARD_JUMP     ) \
        E( STATE_POSE             ) \
        E( STATE_FAST_BACK        ) \
        E( STATE_TURN_RIGHT       ) \
        E( STATE_TURN_LEFT        ) \
        E( STATE_DEATH            ) \
        E( STATE_FALL             ) \
        E( STATE_HANG             ) \
        E( STATE_REACH            ) \
        E( STATE_SPLAT            ) \
        E( STATE_TREAD            ) \
        E( STATE_LAND             ) \
        E( STATE_COMPRESS         ) \
        E( STATE_BACK             ) \
        E( STATE_SWIM             ) \
        E( STATE_GLIDE            ) \
        E( STATE_HANG_UP          ) \
        E( STATE_FAST_TURN        ) \
        E( STATE_STEP_RIGHT       ) \
        E( STATE_STEP_LEFT        ) \
        E( STATE_ROLL_END         ) \
        E( STATE_SLIDE            ) \
        E( STATE_BACK_JUMP        ) \
        E( STATE_RIGHT_JUMP       ) \
        E( STATE_LEFT_JUMP        ) \
        E( STATE_UP_JUMP          ) \
        E( STATE_FALL_BACK        ) \
        E( STATE_HANG_LEFT        ) \
        E( STATE_HANG_RIGHT       ) \
        E( STATE_SLIDE_BACK       ) \
        E( STATE_SURF_TREAD       ) \
        E( STATE_SURF_SWIM        ) \
        E( STATE_DIVE             ) \
        E( STATE_PUSH_BLOCK       ) \
        E( STATE_PULL_BLOCK       ) \
        E( STATE_PUSH_PULL_READY  ) \
        E( STATE_PICK_UP          ) \
        E( STATE_SWITCH_DOWN      ) \
        E( STATE_SWITCH_UP        ) \
        E( STATE_USE_KEY          ) \
        E( STATE_USE_PUZZLE       ) \
        E( STATE_UW_DEATH         ) \
        E( STATE_ROLL_START       ) \
        E( STATE_SPECIAL          ) \
        E( STATE_SURF_BACK        ) \
        E( STATE_SURF_LEFT        ) \
        E( STATE_SURF_RIGHT       ) \
        E( STATE_MIDAS_USE        ) \
        E( STATE_MIDAS_DEATH      ) \
        E( STATE_SWAN_DIVE        ) \
        E( STATE_FAST_DIVE        ) \
        E( STATE_HANDSTAND        ) \
        E( STATE_WATER_OUT        ) \
        /* TR2 ------------------*/ \
        E( STATE_CLIMB_START      ) \
        E( STATE_CLIMB_UP         ) \
        E( STATE_CLIMB_LEFT       ) \
        E( STATE_CLIMB_END        ) \
        E( STATE_CLIMB_RIGHT      ) \
        E( STATE_CLIMB_DOWN       ) \
        E( STATE_UNUSED_1         ) \
        E( STATE_UNUSED_2         ) \
        E( STATE_UNUSED_3         ) \
        E( STATE_WADE             ) \
        E( STATE_UW_ROLL          ) \
        E( STATE_PICKUP_FLARE     ) \
        E( STATE_AIR_ROLL         ) \
        E( STATE_UNUSED_5         ) \
        E( STATE_ZIPLINE          ) \
        /* TR3 ------------------*/ \
        E( STATE_DUCK             ) \
        E( STATE_DUCK_ROLL        ) \
        E( STATE_DASH             ) \
        E( STATE_DASH_DIVE        ) \
        E( STATE_MONKEYSWING_IDLE ) \
        E( STATE_MONKEYSWING      )

    enum { LARA_STATES(DECL_ENUM) STATE_MAX };

    typedef void (Lara::*sHandler)();
    typedef void (Lara::*cHandler)();

    static const sHandler sLaraHandlers[STATE_MAX];
    static const cHandler cLaraHandlers[STATE_MAX];

    #define LARA_RGUN_JOINT 10
    #define LARA_LGUN_JOINT 13
    #define LARA_RGUN_OFFSET vec3(-10, -50, 0)
    #define LARA_LGUN_OFFSET vec3( 10, -50, 0)

    enum {
        JOINT_HIPS = 0,
        JOINT_LEG_L1,
        JOINT_LEG_L2,
        JOINT_LEG_L3,
        JOINT_LEG_R1,
        JOINT_LEG_R2,
        JOINT_LEG_R3,
        JOINT_CHEST,
        JOINT_ARM_R1,
        JOINT_ARM_R2,
        JOINT_ARM_R3,
        JOINT_ARM_L1,
        JOINT_ARM_L2,
        JOINT_ARM_L3,
        JOINT_HEAD,
        JOINT_MAX
    };

    enum {
        JOINT_MASK_HIPS       = 1 << JOINT_HIPS,
        JOINT_MASK_LEG_L1     = 1 << JOINT_LEG_L1,
        JOINT_MASK_LEG_L2     = 1 << JOINT_LEG_L2,
        JOINT_MASK_LEG_L3     = 1 << JOINT_LEG_L3,
        JOINT_MASK_LEG_R1     = 1 << JOINT_LEG_R1,
        JOINT_MASK_LEG_R2     = 1 << JOINT_LEG_R2,
        JOINT_MASK_LEG_R3     = 1 << JOINT_LEG_R3,
        JOINT_MASK_CHEST      = 1 << JOINT_CHEST,
        JOINT_MASK_ARM_R1     = 1 << JOINT_ARM_R1,
        JOINT_MASK_ARM_R2     = 1 << JOINT_ARM_R2,
        JOINT_MASK_ARM_R3     = 1 << JOINT_ARM_R3,
        JOINT_MASK_ARM_L1     = 1 << JOINT_ARM_L1,
        JOINT_MASK_ARM_L2     = 1 << JOINT_ARM_L2,
        JOINT_MASK_ARM_L3     = 1 << JOINT_ARM_L3,
        JOINT_MASK_HEAD       = 1 << JOINT_HEAD,
        JOINT_MASK_ARM_L      = JOINT_MASK_ARM_L1 | JOINT_MASK_ARM_L2 | JOINT_MASK_ARM_L3,
        JOINT_MASK_ARM_R      = JOINT_MASK_ARM_R1 | JOINT_MASK_ARM_R2 | JOINT_MASK_ARM_R3,
        JOINT_MASK_LEG_L      = JOINT_MASK_LEG_L1 | JOINT_MASK_LEG_L2 | JOINT_MASK_LEG_L3,
        JOINT_MASK_LEG_R      = JOINT_MASK_LEG_R1 | JOINT_MASK_LEG_R2 | JOINT_MASK_LEG_R3,
        JOINT_MASK_UPPER      = JOINT_MASK_CHEST  | JOINT_MASK_ARM_L  | JOINT_MASK_ARM_R,       // without head
        JOINT_MASK_LOWER      = JOINT_MASK_HIPS   | JOINT_MASK_LEG_L  | JOINT_MASK_LEG_R,
        JOINT_MASK_BRAID      = JOINT_MASK_HEAD   | JOINT_MASK_CHEST  | JOINT_MASK_ARM_L1 | JOINT_MASK_ARM_L2 | JOINT_MASK_ARM_R1 | JOINT_MASK_ARM_R2,
    };

    struct Weapon {
        enum State { IS_HIDDEN, IS_ARMED, IS_FIRING };
        struct Anim {
            enum Type { NONE, PREPARE, UNHOLSTER, HOLSTER, HOLD, AIM, FIRE };
        };
    };

    TR::Entity::Type wpnCurrent;
    TR::Entity::Type wpnNext;
    Weapon::State    wpnState;

    struct Arm {
        Controller      *tracking;       // tracking target (main target)
        Controller      *target;         // target for shooting
        quat            rot, rotAbs;

        Weapon::Anim::Type anim;
        Animation          animation;

        Arm() : tracking(NULL), target(NULL) {}
    } arms[2];

    TR::Entity::Type  itemHolster;
    TR::Entity::Type  usedItem;
    int               pickupListCount;
    Controller        *pickupList[32];
    KeyHole           *keyHole;
    Controller        *keyItem;
    Lightning         *lightning;
    Texture           *environment;
    vec2              rotFactor;

    int32       oxygen;
    float       damageTime;
    float       hitTime;
    int         hitDir;
    vec3        collisionOffset;
    vec3        flowVelocity;
    float       statsDistDelta;

    Camera      *camera;

    float       hitTimer;

    bool        dozy;
    bool        canJump;

    int32       networkInput;

#ifdef _DEBUG
    //uint16      *dbgBoxes;
    //int         dbgBoxesCount;
#endif
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

        TR::Model* getModel() {
            return &lara->level->models[lara->level->extra.braid];
        }

        Basis getBasis() {
            return lara->getJoint(lara->jointHead);
        }

        vec3 getPos() {
            return getBasis() * offset;
        }

        void integrate() {
            float TIMESTEP = Core::deltaTime;
            float ACCEL    = 16.0f * GRAVITY * 30.0f * TIMESTEP * TIMESTEP;
            float DAMPING  = 1.5f;

            if (lara->stand == STAND_UNDERWATER) {
                ACCEL *= 0.5f;
                DAMPING = 4.0f;
            }

            DAMPING = 1.0f / (1.0f + DAMPING * TIMESTEP); // Pade approximation

            for (int i = 1; i < jointsCount; i++) {
                Joint &j = joints[i];
                vec3 delta = j.pos - j.posPrev;
                delta = delta.normal() * (min(delta.length(), 2048.0f * Core::deltaTime) * DAMPING); // speed limit
                j.posPrev  = j.pos;
                j.pos     += delta;
                if ((lara->stand == STAND_WADE || lara->stand == STAND_ONWATER || lara->stand == STAND_UNDERWATER) && (j.pos.y > lara->waterLevel))
                    j.pos.y -= ACCEL;
                else
                    j.pos.y += ACCEL;
            }
        }

        void collide() {
            TR::Level *level = lara->level;
            const TR::Model *model = lara->getModel();

            TR::Level::FloorInfo info;
            lara->getFloorInfo(lara->getRoomIndex(), lara->getViewPoint(), info);

            for (int j = 1; j < jointsCount; j++)
                if (joints[j].pos.y > info.floor)
                    joints[j].pos.y = info.floor;

            #define BRAID_RADIUS 0.0f

            lara->updateJoints();

            for (int i = 0; i < model->mCount; i++) {
                if (!(JOINT_MASK_BRAID & (1 << i))) continue;

                int offset = level->meshOffsets[model->mStart + i];
                TR::Mesh *mesh = (TR::Mesh*)&level->meshes[offset];

                vec3 center    = lara->joints[i] * mesh->center;
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
                m.up()     = vec4(u, 0.0f);
                m.dir()    = vec4(d, 0.0f);
                m.right()  = vec4(r, 0.0f);
                m.offset() = vec4(0.0f, 0.0f, 0.0f, 1.0f);

                basis[i].identity();
                basis[i].translate(joints[i].pos);
                basis[i].rotate(m.getRot());
            }
        }
        
        void render(MeshBuilder *mesh) {
            Core::setBasis(basis, jointsCount - 1);
            mesh->renderModel(lara->level->extra.braid);
        }

    } *braid[2];

    Lara(IGame *game, int entity) : Character(game, entity, LARA_MAX_HEALTH), wpnCurrent(TR::Entity::NONE), wpnNext(TR::Entity::NONE) {
        camera = new Camera(game, this);

        v2pos.x = int32(pos.x);
        v2pos.y = int32(pos.y);
        v2pos.z = int32(pos.z);

        v2rot.x = 0;
        v2rot.y = int16(angle.y / PI * ANGLE_180);
        v2rot.z = 0;
            
        waterState = WATER_STATE_ABOVE;
        gravity = 0;
        hSpeed = 0;
        vSpeed = 0;
        vSpeedHack = 0;
        animIndex = ANIM_STAND;
        frameIndex = (level->anims + animIndex)->frameStart;
        turnSpeed = 0;

        animation.setAnim(animIndex, frameIndex);
        state = animation.state;
        nextState = state;
        targetState = state;


        braid[0] = braid[1] = NULL;

        itemHolster  = TR::Entity::NONE;
        hitTimer     = 0.0f;
        networkInput = -1;

        dozy    = false;
        canJump = true;

        if (level->extra.laraSkin > -1)
            level->entities[entity].modelIndex = level->extra.laraSkin + 1;

        jointChest = level->isCutsceneLevel() ? 0 : JOINT_CHEST;
        jointHead  = JOINT_HEAD;
        rangeChest = vec4(-0.50f, 0.50f, -0.95f, 0.95f) * PI;
        rangeHead  = vec4(-0.30f, 0.30f, -0.55f, 0.55f) * PI;

        statsDistDelta = 0.0f;

        oxygen     = LARA_MAX_OXYGEN;
        hitDir     = -1;
        damageTime = LARA_DAMAGE_TIME;
        hitTime    = 0.0f;

        keyHole     = NULL;
        keyItem     = NULL;
        lightning   = NULL;
        environment = NULL;

        flags.active = 1;
        initMeshOverrides();

        if (level->isHome()) {
            if (level->version & TR::VER_TR1)
                meshSwap(1, TR::MODEL_LARA_SPEC, JOINT_MASK_UPPER | JOINT_MASK_LOWER);
        } else {
            int *wpnAmmo = game->invCount(TR::Entity::PISTOLS);
            if (wpnAmmo && *wpnAmmo > 0)
                wpnSet(TR::Entity::PISTOLS);
        }

        for (int i = 0; i < 2; i++) {
            arms[i].rot       = quat(0, 0, 0, 1);
            arms[i].rotAbs    = quat(0, 0, 0, 1);
        }

        if (level->extra.braid > -1) {
            vec3 offset(0.0f);
            switch (level->version & TR::VER_VERSION) {
                case TR::VER_TR1 :
                    //braid[0] = new Braid(this, vec3(-4.0f, 24.0f, -48.0f)); // it's just ugly :)
                    break;
                case TR::VER_TR2 :
                case TR::VER_TR3 :
                    braid[0] = new Braid(this, vec3(0.0f, -23.0f, -55.0f));
                    break;
                case TR::VER_TR4 :
                    if (isYoung()) {
                        braid[0] = new Braid(this, vec3(-32.0f, -48.0f, -32.0f));
                        braid[1] = new Braid(this, vec3( 32.0f, -48.0f, -32.0f));
                    } else {
                        braid[0] = new Braid(this, vec3(0.0f, -23.0f, -32.0f));
                    }
                    break;
            }
        }

    // TR1
        //reset(14, vec3(40448, 3584, 60928), PI * 0.5f, STAND_ONWATER);  // gym (pool)
        //reset(0, vec3(74858, 3072, 20795), 0);           // level 1 (dart)
        //reset(14, vec3(20215, 6656, 52942), PI);         // level 1 (bridge)
        //reset(20, vec3(8952, 3840, 68071), PI);          // level 1 (crystal)
        //reset(26, vec3(24475, 6912, 83505), 90 * DEG2RAD);     // level 1 (switch timer)
        //reset(33, vec3(48229, 4608, 78420), 270 * DEG2RAD);     // level 1 (end)
        //reset(9, vec3(63008, 0, 37787), 0);              // level 2 (switch)
        //reset(15, vec3(70067, -256, 29104), -0.68f);     // level 2 (pool)
        //reset(5, vec3(55398, 0, 29246), -PI * 0.5f);     // level 2 (key hole)
        //reset(71, vec3(12473, -768, 30208), -PI * 0.5f); // level 2 (puzzle hole)
        //reset(26, vec3(71980, 1546, 19000), 270 * DEG2RAD);     // level 2 (underwater switch)
        //reset(61, vec3(27221, -1024, 29205), -PI * 0.5f); // level 2 (blade)
        //reset(43, vec3(31400, -2560, 25200), PI);        // level 2 (reach)
        //reset(16, vec3(60907, 0, 39642), PI * 3 / 2);    // level 2 (hang & climb)
        //reset(19, vec3(60843, 1024, 30557), PI);         // level 2 (block)
        //reset(1,  vec3(62630, -1280, 19633), 0);         // level 2 (dark medikit)
        //reset(7,  vec3(64108, -512, 16514), -PI * 0.5f); // level 2 (bat trigger)
        //reset(15, vec3(70082, -512, 26935), PI * 0.5f);  // level 2 (bear)
        //reset(63, vec3(31390, -2048, 33472), 0.0f);      // level 2 (trap floor)
        //reset(61, vec3(21987, -1024, 29144), PI * 3.0f * 0.5f); // level 2 (trap door)
        //reset(51, vec3(41015, 3584, 34494), -PI);        // level 3a (t-rex)
        //reset(5,  vec3(38643, -3072, 92370), PI * 0.5f); // level 3a (gears)
        //reset(43, vec3(64037, 6656, 48229), PI);         // level 3b (movingblock)
        //reset(27, vec3(72372, 8704, 46547), PI * 0.5f);  // level 3b (spikes)
        //reset(5, vec3(73394, 3840, 60758), 0);           // level 3b (scion)
        //reset(20, vec3(57724, 6656, 61941), 90 * DEG2RAD); // level 3b (boulder)
        //reset(18, vec3(34914, 11008, 41315), 90 * DEG2RAD); // level 4 (main hall)
        //reset(19, vec3(33368, 19968, 45643), 270 * DEG2RAD); // level 4 (damocles)
        //reset(24, vec3(45609, 18176, 41500), 90 * DEG2RAD); // level 4 (thor)
        //reset(19, vec3(41418, -3707, 58863), 270 * DEG2RAD);  // level 5 (triangle)
        //reset(21, vec3(24106, -4352, 52089), 0);              // level 6 (flame traps)
        //reset(73, vec3(73372, 122, 51687), PI * 0.5f);       // level 6 (midas hand)
        //reset(20, vec3(25088, -5120, 17593), PI);            // level 6 (puzzle slots)
        //reset(64, vec3(36839, -2560, 48769), 270 * DEG2RAD);  // level 6 (flipmap effect)
        //reset(99,  vec3(45562, -3328, 63366), 225 * DEG2RAD); // level 7a (flipmap)
        //reset(77,  vec3(36943, -4096, 62821), 270 * DEG2RAD); // level 7b (heavy trigger)
        //reset(90,  vec3(19438, 3840, 78341), 90 * DEG2RAD); // level 7b (statues)
        //reset(90,  vec3(29000, 3840 - 512, 78341), 90 * DEG2RAD); // level 7b (statues)
        //reset(57,  vec3(54844, -3328, 53145), 0);        // level 8b (bridge switch)
        //reset(12,  vec3(34236, -2415, 14974), 0);        // level 8b (sphinx)
        //reset(0,  vec3(40913, -1012, 42252), PI);        // level 8c
        //reset(56,  vec3(18541, 512, 52869), PI * 0.5f);        // level 8c
        //reset(30, vec3(69689, -8448, 34922), 330 * DEG2RAD);      // Level 10a (cabin)
        //reset(27, vec3(52631, -4352, 57893), 270 * DEG2RAD);      // Level 10a (TNT / Cowboy)
        //reset(57, vec3(71081, 5632, 73042), 0);                   // Level 10a (Skaterboy)
        //reset(68, vec3(52458, -9984, 93724), 270 * DEG2RAD);      // Level 10a (MrT)
        //reset(44, vec3(75803, -11008, 21097), 90 * DEG2RAD);      // Level 10a (boat)
        //reset(47, vec3(50546, -13056, 53783), 270 * DEG2RAD);     // Level 10b (trap door slope)
        //reset(59, vec3(42907, -13056, 63012), 270 * DEG2RAD);     // Level 10b (doppelganger)
        //reset(53, vec3(39617, -18385, 48950), 180 * DEG2RAD);     // Level 10b (centaur)
        //reset(50, vec3(52122, -18688, 47313), 150 * DEG2RAD);     // Level 10b (scion holder pickup)
        //reset(50, vec3(53703, -18688, 13769), PI);                // Level 10c (scion holder)
        //reset(19, vec3(35364, -512, 40199), PI * 0.5f);           // Level 10c (lava flow)
        //reset(9, vec3(69074, -14592, 25192), 0);                  // Level 10c (trap slam)
        //reset(21, vec3(47668, -10752, 32163), 0);                 // Level 10c (lava emitter)
        //reset(29, vec3(61586, -439, 51734), PI * 0.5f);           // Level 10c (precise falling)
        //reset(33, vec3(64641, 9578, 61861), 0);                   // Level 10c (Natla)
        //reset(10, vec3(90443, 11264 - 256, 114614), PI, STAND_ONWATER);   // villa mortal 2
    // TR2
        //reset(36, vec3(64190, 5632, 35743), 75 * DEG2RAD);   // WALL (wade)
        //reset(19, vec3(28353, 2560, 58587), 150 * DEG2RAD);  // ASSAULT (wade)

        //dbgBoxes = NULL;

        if (!level->isCutsceneLevel()) {
            if (getRoom().flags.water) {
                stand = STAND_UNDERWATER;
                animation.setAnim(ANIM_UNDERWATER);
            } else
                animation.setAnim(ANIM_STAND);
        }
    }

    virtual ~Lara() {
        delete camera;
        delete braid[0];
        delete braid[1];
        delete environment;
    }

    bool isYoung() {
        return level->id == TR::LVL_TR4_ANGKOR1 || level->id == TR::LVL_TR4_ANG_RACE;
    }

    bool canSaveGame() {
        return health > 0.0f && !burn 
               && state != STATE_PICK_UP
               && state != STATE_PICKUP_FLARE
               && state != STATE_USE_KEY
               && state != STATE_USE_PUZZLE; // && (state == STATE_STOP || state == STATE_TREAD || state == STATE_SURF_TREAD);
    }

    TR::Entity::Type getItemHands() {
        return (wpnState == Weapon::IS_HIDDEN) ? TR::Entity::NONE : wpnCurrent;
    }

    TR::Entity::Type getItemBack() {
        if (game->invCount(TR::Entity::INV_SHOTGUN) && (wpnCurrent != TR::Entity::SHOTGUN || wpnState == Weapon::IS_HIDDEN))
            return TR::Entity::SHOTGUN;
        return TR::Entity::NONE;
    }

    virtual bool getSaveData(SaveEntity &data) {
        if (camera->cameraIndex != 0) // only player1 can be saved
            return false;

        Character::getSaveData(data);
        data.extraSize = sizeof(data.extra.lara);
        data.extra.lara.velX        = velocity.x;
        data.extra.lara.velY        = velocity.y;
        data.extra.lara.velZ        = velocity.z;
        data.extra.lara.angleX      = angle.x;
        data.extra.lara.health      = health;
        data.extra.lara.oxygen      = float(oxygen); // TODO to int
        data.extra.lara.stamina     = 0.0f;
        data.extra.lara.poison      = 0.0f;
        data.extra.lara.freeze      = 0.0f;
        data.extra.lara.reserved    = 0;
        data.extra.lara.itemWeapon  = wpnCurrent;
        data.extra.lara.itemHands   = getItemHands();
        data.extra.lara.itemBack    = getItemBack();
        data.extra.lara.itemHolster = itemHolster;
        data.extra.lara.spec.value  = 0;
        data.extra.lara.spec.burn   = burn;
        data.extra.lara.spec.wet    = specular > LARA_MIN_SPECULAR;
        return true;
    }

    virtual void setSaveData(const SaveEntity &data) {
        Character::setSaveData(data);
        statsDistDelta = 0.0f;

        velocity = vec3(data.extra.lara.velX, data.extra.lara.velY, data.extra.lara.velZ);
        angle.x  = TR::angle(data.extra.lara.angleX);
        health   = data.extra.lara.health;
        oxygen   = int(data.extra.lara.oxygen); // TODO to int

        if (level->isHome()) return;

        layers[1].mask = layers[2].mask = layers[3].mask = 0;

        if (data.extra.lara.itemWeapon)
            wpnSet(TR::Entity::Type(data.extra.lara.itemWeapon));

        wpnCurrent  = TR::Entity::Type(data.extra.lara.itemWeapon);
        itemHolster = TR::Entity::Type(data.extra.lara.itemHolster);
        wpnState    = Weapon::IS_HIDDEN;

        if (data.extra.lara.itemHands && data.extra.lara.itemHands == data.extra.lara.itemWeapon)
            wpnDraw(true);

        if (itemHolster != TR::Entity::NONE)
            meshSwap(1, level->extra.weapons[itemHolster], JOINT_MASK_LEG_L1 | JOINT_MASK_LEG_R1);

        if (getRoom().flags.water) {
            stand = STAND_UNDERWATER;
            if (state == STATE_SURF_TREAD || state == STATE_SURF_SWIM || state == STATE_SURF_BACK || state == STATE_SURF_LEFT || state == STATE_SURF_RIGHT)
                stand = STAND_ONWATER;
        } else
            stand = STAND_AIR;

        v2pos.x = int32(pos.x);
        v2pos.y = int32(pos.y);
        v2pos.z = int32(pos.z);
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
        statsDistDelta = 0.0f;

        visibleMask = 0xFFFFFFFF;
        health = LARA_MAX_HEALTH;
        oxygen = LARA_MAX_OXYGEN;
        dozy   = false;

        keyHole = NULL;
        keyItem = NULL;

        if (room == TR::NO_ROOM) {
            stand = STAND_AIR;
            room  = getRoomByPos(pos);
        }

        if (room == TR::NO_ROOM)
            return;

        if (level->rooms[room].flags.water) {
            stand = STAND_UNDERWATER;
            animation.setAnim(ANIM_UNDERWATER);
        } else {
            stand = STAND_GROUND;
            animation.setAnim(ANIM_STAND);
        }

        velocity = vec3(0.0f);

        roomIndex   = room;
        this->pos   = pos;
        this->angle = vec3(0.0f, angle, 0.0f);

        v2pos.x = int32(pos.x);
        v2pos.y = int32(pos.y);
        v2pos.z = int32(pos.z);

        v2rot.x = 0;
        v2rot.y = int16(angle * ANGLE_180);
        v2rot.z = 0;

        if (forceStand != STAND_GROUND) {
            stand = forceStand;
            switch (stand) {
                case STAND_ONWATER    : animation.setAnim(ANIM_SURF); break;
                case STAND_UNDERWATER : animation.setAnim(ANIM_UNDERWATER); break;
                default               : ;
            }
        }

        updateZone();
        updateLights(false);

        camera->changeView(camera->firstPerson);
    }

    void wpnSet(TR::Entity::Type wType) {
        wpnCurrent = wType;
        wpnState   = Weapon::IS_FIRING;

        arms[0].animation = arms[1].animation = Animation(level, &level->models[wType == TR::Entity::SHOTGUN ? TR::MODEL_SHOTGUN : TR::MODEL_PISTOLS]);

        wpnSetAnim(arms[0], Weapon::IS_HIDDEN, Weapon::Anim::NONE, 0.0f, 0.0f);
        wpnSetAnim(arms[1], Weapon::IS_HIDDEN, Weapon::Anim::NONE, 0.0f, 0.0f);
    }

    void wpnSetAnim(Arm &arm, Weapon::State wState, Weapon::Anim::Type wAnim, float wAnimTime, float wAnimDir, bool playing = true) {
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
            case TR::Entity::PISTOLS : return 1;
            case TR::Entity::SHOTGUN : return 1;
            case TR::Entity::MAGNUMS : return 2;
            case TR::Entity::UZIS    : return 1;
            default : ;
        }
        return 0;
    }

    void wpnSetState(Weapon::State wState) {
        if (wpnState == wState || !layers) return;

        int mask = 0;
        switch (wpnCurrent) {
            case TR::Entity::PISTOLS :
            case TR::Entity::MAGNUMS :
            case TR::Entity::UZIS    :
                switch (wState) {
                    case Weapon::IS_HIDDEN : mask = JOINT_MASK_LEG_L1 | JOINT_MASK_LEG_R1;              break;
                    case Weapon::IS_ARMED  : mask = JOINT_MASK_ARM_L3 | JOINT_MASK_ARM_R3;              break;
                    case Weapon::IS_FIRING : mask = JOINT_MASK_ARM_L3 | JOINT_MASK_ARM_R3 | JOINT_MASK_HEAD;  break;
                }
                break;
            case TR::Entity::SHOTGUN :
                switch (wState) {
                    case Weapon::IS_HIDDEN : mask = JOINT_MASK_CHEST;                             break;
                    case Weapon::IS_ARMED  : mask = JOINT_MASK_ARM_L3 | JOINT_MASK_ARM_R3;              break;
                    case Weapon::IS_FIRING : mask = JOINT_MASK_ARM_L3 | JOINT_MASK_ARM_R3 | JOINT_MASK_HEAD;  break;
                }
                break;
            default : ;
        }

        if (wpnState == Weapon::IS_HIDDEN && wState == Weapon::IS_ARMED)  game->playSound(TR::SND_UNHOLSTER, pos, Sound::PAN);
        if (wpnState == Weapon::IS_ARMED  && wState == Weapon::IS_HIDDEN) game->playSound(TR::SND_HOLSTER,   pos, Sound::PAN);

    // swap layers
    // 0 - body (full)
    // 1 - legs (hands, legs)
    // 2 - shotgun (hands, chest)
    // 3 - angry (head)

        // swap weapon parts
        if (wpnCurrent != TR::Entity::SHOTGUN) {
            meshSwap(1, level->extra.weapons[wpnCurrent], mask);
            // have a shotgun in inventory place it on the back if another weapon is in use
            meshSwap(2, level->extra.weapons[TR::Entity::SHOTGUN], game->invCount(TR::Entity::INV_SHOTGUN) ? JOINT_MASK_CHEST : 0);
            itemHolster = (wState == Weapon::IS_HIDDEN) ? wpnCurrent : TR::Entity::NONE;
        } else {
            meshSwap(2, level->extra.weapons[wpnCurrent], mask);
        }

        // mesh swap to angry Lara's head while firing (from uzis model)
        meshSwap(3, level->extra.weapons[TR::Entity::UZIS], (wState == Weapon::IS_FIRING) ? JOINT_MASK_HEAD : 0);

        wpnState = wState;
    }

    bool emptyHands() {
        return wpnCurrent == TR::Entity::NONE || arms[0].anim == Weapon::Anim::NONE;
    }

    bool canLookAt() {
        return (stand != STAND_HANG)
               && state != STATE_REACH
               && state != STATE_PUSH_BLOCK
               && state != STATE_PULL_BLOCK
               && state != STATE_PUSH_PULL_READY
               && state != STATE_PICK_UP;
    }

    bool canDrawWeapon() {
        if (wpnCurrent == TR::Entity::NONE)
            return false;

        if (dozy) return true;

        return    animation.index != ANIM_CLIMB_3
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
               && state != STATE_UW_DEATH
               && state != STATE_SPECIAL
               && state != STATE_SURF_BACK
               && state != STATE_SURF_LEFT
               && state != STATE_SURF_RIGHT
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

    void wpnDraw(bool instant = false) {
        if (!canDrawWeapon()) return;

        if (wpnReady() && emptyHands()) {
            if (wpnCurrent != TR::Entity::SHOTGUN) {
                wpnSetAnim(arms[0], wpnState, instant ? Weapon::Anim::AIM : Weapon::Anim::PREPARE, 0.0f, 1.0f);
                wpnSetAnim(arms[1], wpnState, instant ? Weapon::Anim::AIM : Weapon::Anim::PREPARE, 0.0f, 1.0f);
            } else
                wpnSetAnim(arms[0], wpnState, instant ? Weapon::Anim::AIM : Weapon::Anim::UNHOLSTER, 0.0f, 1.0f);
        }
    }

    void wpnHide() {
        if (wpnReady() && !emptyHands()) {
            if (wpnCurrent != TR::Entity::SHOTGUN) {
                wpnSetAnim(arms[0], wpnState, Weapon::Anim::UNHOLSTER, 0.0f, -1.0f);
                wpnSetAnim(arms[1], wpnState, Weapon::Anim::UNHOLSTER, 0.0f, -1.0f);
            } else
                wpnSetAnim(arms[0], wpnState, Weapon::Anim::HOLSTER, 0.0f, 1.0f);
        }
    }

    void wpnChange(TR::Entity::Type wType) {
        if (wpnCurrent == wType || level->isHome()) {
            if (emptyHands())
                wpnDraw();
            return;
        }
        wpnNext = wType;
        wpnHide();
    }

    int wpnGetAnimIndex(Weapon::Anim::Type wAnim) {
        if (wpnCurrent == TR::Entity::SHOTGUN) {
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
            case TR::Entity::PISTOLS : return TR::SND_PISTOLS_SHOT;
            case TR::Entity::SHOTGUN : return TR::SND_SHOTGUN_SHOT;
            case TR::Entity::MAGNUMS : return TR::SND_MAGNUMS_SHOT;
            case TR::Entity::UZIS    : return TR::SND_UZIS_SHOT;
            default                  : return TR::SND_NO;
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
                        if ((input & ACTION) && (!arm.tracking || arm.target))
                            armShot[i] = true;
                        else
                            wpnSetAnim(arm, Weapon::IS_ARMED, Weapon::Anim::AIM, 0.0f, -1.0f, arm.target == NULL);
                    }
                // shotgun reload sound
                    if (wpnCurrent == TR::Entity::SHOTGUN) {
                        if (anim.frameIndex == 10)
                            game->playSound(TR::SND_SHOTGUN_RELOAD, pos, Sound::PAN);
                    }
                }
            }
            arm.animation.framePrev = arm.animation.frameIndex;

            if (wpnCurrent == TR::Entity::SHOTGUN) break;
        }

        if (armShot[0] || armShot[1])
            doShot(armShot[0], armShot[1]);
    }

    void doShot(bool rightHand, bool leftHand) {
        int *wpnAmmo = game->invCount(wpnCurrent);

        if (wpnAmmo && *wpnAmmo != UNLIMITED_AMMO && *wpnAmmo <= 0) { // check for no ammo
            game->playSound(TR::SND_EMPTY, pos, Sound::PAN);
            wpnChange(TR::Entity::PISTOLS);
        }

        int count = wpnCurrent == TR::Entity::SHOTGUN ? 6 : 2;
        float nearDist = 32.0f * 1024.0f;
        vec3  nearPos;
        int   shots = 0, hits = 0;

        for (int i = 0; i < count; i++) {
            int armIndex;
            if (wpnCurrent == TR::Entity::SHOTGUN) {
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
                if (wpnCurrent != TR::Entity::SHOTGUN)
                    *wpnAmmo -= 1;
            }

            shots++;

            if (wpnCurrent != TR::Entity::SHOTGUN)
                game->addMuzzleFlash(this, i ? LARA_LGUN_JOINT : LARA_RGUN_JOINT, i ? LARA_LGUN_OFFSET : LARA_RGUN_OFFSET, 1 + camera->cameraIndex);

        // TODO: use new trace code
            int joint = wpnCurrent == TR::Entity::SHOTGUN ? 8 : (i ? 11 : 8);
            vec3 p = getJoint(joint).pos;
            vec3 d = arm->rotAbs * vec3(0, 0, 1);
            vec3 t = p + d * (24.0f * 1024.0f) + ((vec3(randf(), randf(), randf()) * 2.0f) - vec3(1.0f)) * 1024.0f;

            int room;
            vec3 hit = trace(getRoomIndex(), p, t, room, false);
            if (arm->target && checkHit(arm->target, p, hit, hit)) {
                hits++;
                TR::Entity::Type type = arm->target->getEntity().type;
                ((Character*)arm->target)->hit(wpnGetDamage(), this);
                hit -= d * 64.0f;
                if (type != TR::Entity::SCION_TARGET)
                    game->addEntity(TR::Entity::BLOOD, room, hit);
            } else {
                hit -= d * 64.0f;
                game->addEntity(TR::Entity::RICOCHET, room, hit);

                float dist = (hit - p).length();
                if (dist < nearDist) {
                    nearPos  = hit;
                    nearDist = dist;
                }
            }
        }

        if (shots) {
            saveStats.ammoUsed += ((wpnCurrent == TR::Entity::SHOTGUN) ? 1 : 2);

            game->playSound(wpnGetSound(), pos, Sound::PAN);
            if (shots != hits)
                game->playSound(TR::SND_RICOCHET, nearPos, Sound::PAN);

             if (wpnAmmo && *wpnAmmo != UNLIMITED_AMMO && wpnCurrent == TR::Entity::SHOTGUN)
                *wpnAmmo -= 1;
        }
    }

    void updateWeapon() {
        if (level->isCutsceneLevel()) return;

        if (wpnNext != TR::Entity::NONE && emptyHands()) {
            wpnSet(wpnNext);
            wpnDraw();
            wpnNext = TR::Entity::NONE;
        }

    // apply weapon state changes
        if (input & WEAPON) {
            if (emptyHands())
                wpnDraw();
            else
                wpnHide();
        }

        if (!emptyHands()) {
            bool isRifle = wpnCurrent == TR::Entity::SHOTGUN;

            for (int i = 0; i < 2; i++) {
                Arm &arm = arms[i];

                if (arm.target || ((input & ACTION) && !arm.tracking)) {
                    if (arm.anim == Weapon::Anim::HOLD)
                        wpnSetAnim(arm, wpnState, Weapon::Anim::AIM, 0.0f, 1.0f);
                } else
                    if (arm.anim == Weapon::Anim::AIM)
                        arm.animation.dir = -1.0f;

                if (isRifle) break;
            }

            for (int i = 0; i < 2; i++)
                arms[i].animation.update();

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
                            wpnSetAnim(arm, Weapon::IS_FIRING, Weapon::Anim::FIRE, arm.animation.time - arm.animation.timeMax, wpnCurrent == TR::Entity::UZIS ? 2.0f : 1.0f);
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
        // Copy all current animation joints
        for (int i = 0; i < JOINT_MAX; i++)
            animation.overrides[i] = animation.getJointRot(i);

        int overrideMask = 0;
        // head & chest
        overrideMask |= JOINT_MASK_CHEST | JOINT_MASK_HEAD;

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
            
            overrideMask &= ~(JOINT_MASK_CHEST | JOINT_MASK_HEAD);
            int hitMask = (JOINT_MASK_UPPER | JOINT_MASK_LOWER | JOINT_MASK_HEAD) & ~overrideMask;
            int index    = 0;
            while (hitMask) {
                if (hitMask & 1)
                    animation.overrides[index] = hitAnim.getJointRot(index);
                index++;
                hitMask >>= 1;
            }

            hitTime += Core::deltaTime;
            overrideMask = JOINT_MASK_UPPER | JOINT_MASK_LOWER | JOINT_MASK_HEAD;
        }

        // arms
        if (!emptyHands()) {
            // right arm
            Arm *arm = &arms[0];
            animation.overrides[JOINT_ARM_R1] = animation.overrides[JOINT_CHEST].inverse() * animation.overrides[JOINT_HIPS].inverse() * arm->animation.getJointRot(JOINT_ARM_R1);
            animation.overrides[JOINT_ARM_R2] = arm->animation.getJointRot(JOINT_ARM_R2);
            animation.overrides[JOINT_ARM_R3] = arm->animation.getJointRot(JOINT_ARM_R3);
            // left arm
            if (wpnCurrent != TR::Entity::SHOTGUN) arm = &arms[1];

            animation.overrides[JOINT_ARM_L1] = animation.overrides[JOINT_CHEST].inverse() * animation.overrides[JOINT_HIPS].inverse() * arm->animation.getJointRot(JOINT_ARM_L1);
            animation.overrides[JOINT_ARM_L2] = arm->animation.getJointRot(JOINT_ARM_L2);
            animation.overrides[JOINT_ARM_L3] = arm->animation.getJointRot(JOINT_ARM_L3);

            overrideMask |= (JOINT_MASK_ARM_R | JOINT_MASK_ARM_L);
        } else
            overrideMask &= ~(JOINT_MASK_ARM_R | JOINT_MASK_ARM_L);

        animation.overrideMask = overrideMask;
        jointsFrame = -1;
    }

    vec3 getAngle(const vec3 &dir) {
        return vec3(atan2f(dir.y, sqrtf(dir.x * dir.x + dir.z * dir.z)) - angle.x, atan2f(dir.x, dir.z) - angle.y + PI, 0.0f);
    }

    vec3 getAngleAbs(const vec3 &dir) {
        return vec3(-atan2f(dir.y, sqrtf(dir.x * dir.x + dir.z * dir.z)), -atan2f(dir.x, dir.z), 0.0f);
    }

    virtual void lookAt(Controller *target) {
        if (health <= 0.0f)
            return;

        updateOverrides();
        updateTargets();

        Controller *lookTarget = canLookAt() ? target : NULL;
        if (camera->mode == Camera::MODE_LOOK) {
            vec3 p = pos + vec3(camera->targetAngle.x, camera->targetAngle.y) * 8192.0f;
            Character::lookAtPos(&p);
        } else
            Character::lookAt(lookTarget);

        if (wpnReady() && !emptyHands()) {
            if (wpnCurrent == TR::Entity::SHOTGUN)
                aimShotgun();
            else
                aimPistols();
        }

        if (target) {
            Box box = target->getBoundingBox();
            vec3 dir = getJoint(jointHead).pos - box.center();
            vec3 angle = getAngle(dir) * RAD2DEG;
            camera->setAngle(angle.x, angle.y);
        } else {
            if (camera->mode == ICamera::MODE_COMBAT) {
                camera->setAngle(0, 0);
            }
        }
    }

    void aimShotgun() {
        float speed = 8.0f * Core::deltaTime;

        int joints[2] = { JOINT_ARM_R1, JOINT_ARM_L1 };

        bool hasAim = true;

        quat rot;
        Arm &arm = arms[0];
        if (!aim(arm.target, JOINT_ARM_R1, vec4(-PI * 0.4f, PI * 0.4f, -PI * 0.25f, PI * 0.25f), rot, &arm.rotAbs)) {
            rot = quat(0, 0, 0, 1);
            arm.target = NULL;
            hasAim = false;
        }

        if (hasAim)
            rot = animation.getJointRot(JOINT_HIPS) * animation.getJointRot(JOINT_CHEST) * rot;

        for (int i = 0; i < 2; i++) {
            int j = joints[i];
            arm.rot = arm.rot.slerp(rot, speed);
            animation.overrides[j] = arm.rot * animation.overrides[j];
            jointsFrame = -1;
        }
    }

    void aimPistols() {
        float speed = 8.0f * Core::deltaTime;

        int joints[2] = { JOINT_ARM_R1, JOINT_ARM_L1 };

        vec4 ranges[2] = {
            vec4(-PI * 0.4f, PI * 0.4f, -PI * 0.2f, PI * 0.5f),
            vec4(-PI * 0.4f, PI * 0.4f, -PI * 0.5f, PI * 0.2f),
        };
        
        for (int i = 0; i < 2; i++) {
            quat rot;
            Arm &arm = arms[i];
            int j = joints[i];

            bool hasAim = true;
            if (!aim(arm.target, j, ranges[i], rot, &arm.rotAbs)) {
                arm.target = arms[i^1].target;
                if (!aim(arm.target, j, ranges[i], rot, &arm.rotAbs)) {
                    rot = quat(0, 0, 0, 1);
                    arm.target = NULL;
                    hasAim = false;
                }
            }

            if (hasAim)
                rot = animation.getJointRot(JOINT_HIPS) * animation.getJointRot(JOINT_CHEST) * rot;

            arm.rot = arm.rot.slerp(rot, speed);
            animation.overrides[j] = arm.rot * animation.overrides[j];
            jointsFrame = -1;
        }
    }

    void updateTargets() {
        arms[0].target = arms[1].target = NULL;
        viewTarget = NULL;

        if (emptyHands() || !wpnReady()) {
            arms[0].tracking = arms[1].tracking = NULL;
            return;
        }

        // auto retarget 
        bool retarget = false;
        if (Core::settings.controls[camera->cameraIndex].retarget) {
            for (int i = 0; i < 2; i++)
                if (!arms[i].tracking || !((Character*)arms[i].tracking)->isActiveTarget()) {
                    retarget = true;
                    break;
                }
        }

        int count = wpnCurrent != TR::Entity::SHOTGUN ? 2 : 1;
        if (!(input & ACTION) || retarget) {
            getTargets(arms[0].tracking, arms[1].tracking);
            if (count == 1)
                arms[1].tracking = NULL;
            else if (!arms[0].tracking && arms[1].tracking)
                arms[0].tracking = arms[1].tracking;
            else if (!arms[1].tracking && arms[0].tracking)
                arms[1].tracking = arms[0].tracking;
            arms[0].target = arms[0].tracking;
            arms[1].target = arms[1].tracking;
        } else {
            if (!arms[0].tracking && !arms[1].tracking)
                return;

        // flip left and right by relative target direction
            if (count > 1) {
                float side[2] = { 0, 0 };
                vec3 dir = getDir();
                dir.y = 0.0f;

                for (int i = 0; i < count; i++)
                    if (arms[i].tracking) {
                        vec3 v = arms[i].tracking->pos - pos;
                        v.y = 0;
                        side[i] = sign(v.cross(dir).y);
                    }

                if (side[0] > 0 && side[1] < 0)
                    swap(arms[0].tracking, arms[1].tracking);
            }

        // check occlusion for tracking targets
            for (int i = 0; i < count; i++)
                if (arms[i].tracking) {
                    Controller *enemy = (Controller*)arms[i].tracking;

                    Box box = enemy->getBoundingBox();
                    vec3 to = box.center();
                    to.y = box.min.y + (box.max.y - box.min.y) / 3.0f;

                    vec3 from = pos - vec3(0, 650, 0);
                    arms[i].target = checkOcclusion(from, to, (to - from).length()) ? arms[i].tracking : NULL;
                }

            if (count == 1)
                arms[1].target = NULL;
            else if (!arms[0].target && arms[1].target)
                arms[0].target = arms[1].target;
            else if (!arms[1].target && arms[0].target)
                arms[1].target = arms[0].target;
        }

        if (arms[0].target && arms[1].target && arms[0].target != arms[1].target) {
            viewTarget = (arms[0].target->pos - pos).length2() < (arms[1].target->pos - pos).length2() ? arms[0].target : arms[1].target;
        } else if (arms[0].target)
            viewTarget = arms[0].target;
        else if (arms[1].target)
            viewTarget = arms[1].target;
        else if (arms[0].tracking)
            viewTarget = arms[0].tracking;
        else if (arms[1].tracking)
            viewTarget = arms[1].tracking;
    }

    void getTargets(Controller *&target1, Controller *&target2) {
        vec3 dir = getDir().normal();
        float dist[2]  = { TARGET_MAX_DIST, TARGET_MAX_DIST };

        target1 = target2 = NULL;

        vec3 from = pos - vec3(0, 650, 0);

        Controller *c = Controller::first;
        do {
            if (!c->getEntity().isEnemy())
                continue;

            Character *enemy = (Character*)c;
            if (!enemy->isActiveTarget())
                continue;

            Box box = enemy->getBoundingBox();
            vec3 p = box.center();
            p.y = box.min.y + (box.max.y - box.min.y) / 3.0f;
            
            vec3 v = p - pos;
            if (dir.dot(v.normal()) <= 0.5f)
                continue; // target is out of view range -60..+60 degrees

            float d = v.length();

            if ((d > dist[0] && d > dist[1]) || !checkOcclusion(from, p, d)) 
                continue;

            if (d < dist[0]) {
                target2 = target1;
                dist[1] = dist[0];
                target1 = enemy;
                dist[0] = d;
            } else if (d < dist[1]) {
                target2 = enemy;
                dist[1] = d;
            }
        } while ((c = c->next));

        if (!target2 || dist[1] > dist[0] * 4)
            target2 = target1;
    }

    bool checkOcclusion(const vec3 &from, const vec3 &to, float dist) {
        int room;
        vec3 d = trace(getRoomIndex(), from, to, room, false); // check occlusion
        return ((d - from).length() > (dist - 512.0f));
    }

    bool checkHit(Controller *target, const vec3 &from, const vec3 &to, vec3 &point) {
        Box box = target->getBoundingBoxLocal();
        mat4 m  = target->getMatrix();

        float t;
        vec3 v = to - from;
        
        if (box.intersect(m, from, v, t)) {
            t *= v.length();
            v = v.normal();
            Sphere spheres[MAX_JOINTS];
            int count = target->getSpheres(spheres);
            for (int i = 0; i < count; i++) {
                float st;
                if (spheres[i].intersect(from, v, st)) {
                    point = from + v * max(t, st);
                    return true;
                }
            }
        }
        return false;
    }

    virtual void cmdEmpty() {
        wpnHide();
    }

    virtual void cmdOffset(const vec3 &offset) {
    }

    virtual void cmdJump(const vec3 &vel) {
    }

    void drawGun(int right) {
        wpnCurrent = TR::Entity::PISTOLS;
        int mask = (right ? JOINT_MASK_ARM_R3 : JOINT_MASK_ARM_L3); // unholster
        if (layers[1].mask & mask)
            mask = (layers[1].mask & ~mask) | (right ? JOINT_MASK_LEG_R1 : JOINT_MASK_LEG_L1); // holster
        else
            mask |= layers[1].mask;
        meshSwap(1, level->extra.weapons[wpnCurrent], mask);
    }

    void doBubbles() {
        if ((level->version & TR::VER_VERSION) > TR::VER_TR2) {
            return; // TODO
        }
        int count = rand() % 3;
        if (!count) return;
        game->playSound(TR::SND_BUBBLE, pos, Sound::PAN);
        vec3 head = getJoint(jointHead) * vec3(0.0f, 0.0f, 50.0f);
        for (int i = 0; i < count; i++)
            game->addEntity(TR::Entity::BUBBLE, getRoomIndex(), head, 0);
    }

    virtual void cmdEffect(int fx) {
    }

    virtual void hit(float damage, Controller *enemy = NULL, TR::HitType hitType = TR::HIT_DEFAULT) {
        if (dozy || level->isCutsceneLevel()) return;

        if (health <= 0.0f && hitType != TR::HIT_FALL) return;

        damageTime = LARA_DAMAGE_TIME;

        Character::hit(damage, enemy, hitType);

        hitTimer = LARA_VIBRATE_HIT_TIME;

        switch (hitType) {
            case TR::HIT_DART      : addBlood(enemy->pos, vec3(0));
            case TR::HIT_BLADE     : addBloodBlade(); break;
            case TR::HIT_SPIKES    : addBloodSpikes(); break;
            case TR::HIT_SWORD     : addBloodBlade(); break;
            case TR::HIT_SLAM      : addBloodSlam(enemy); break;
            case TR::HIT_LIGHTNING : lightning = (Lightning*)enemy; break;
            default                : ;
        }

        if (health > 0.0f)
            return;

        game->stopTrack();

        Core::lightColor[1 + 0] = Core::lightColor[1 + 1] = vec4(0, 0, 0, 1);
        arms[0].tracking  = arms[1].tracking  = NULL;
        arms[0].target    = arms[1].target    = NULL;
        viewTarget        = NULL;
        animation.overrideMask = 0;
        int oldState = state;

        switch (hitType) {
            case TR::HIT_FALL : {
                animation.setState(STATE_DEATH);
                break;
            }
            case TR::HIT_BOULDER : {
                animation.setAnim(ANIM_DEATH_BOULDER);

                vec3 v(0.0f);
                if (enemy && enemy->getEntity().type == TR::Entity::TRAP_BOULDER) {
                    angle = enemy->angle;
                    TR::Level::FloorInfo info;
                    getFloorInfo(getRoomIndex(), pos, info);
                    vec3 d = getDir();
                    v = info.getSlant(d);
                    float dp = d.dot(v);
                    if (fabsf(dp) < 0.999)
                        angle.x = -acosf(dp);
                    v = ((TrapBoulder*)enemy)->velocity * 2.0f;
                }

                for (int i = 0; i < 15; i++)
                    addBlood(256.0f, 512.0f, v);
                break;
            }
            case TR::HIT_SPIKES : {
                pos.y = enemy->pos.y;
                animation.setAnim(ANIM_DEATH_SPIKES);
                for (int i = 0; i < 19; i++)
                    addBloodSpikes();
                break;
            }
            case TR::HIT_REX : {
                pos   = enemy->pos;
                angle = enemy->angle;

                meshSwap(1, TR::MODEL_LARA_SPEC, JOINT_MASK_UPPER | JOINT_MASK_LOWER);
                meshSwap(2, level->extra.weapons[TR::Entity::SHOTGUN], 0);
                meshSwap(3, level->extra.weapons[TR::Entity::UZIS],    0);

                animation.setAnim(level->models[TR::MODEL_LARA_SPEC].animation + 1);
                break;
            }
            case TR::HIT_MIDAS : {
            // generate environment map for reflections
                bakeEnvironment(environment);
            // set death animation
                animation.setAnim(level->models[TR::MODEL_LARA_SPEC].animation + 1);
                camera->doCutscene(pos, angle.y);
                break;
            }
            case TR::HIT_GIANT_MUTANT : {
                pos   = enemy->pos;
                angle = enemy->angle;

                animation.setAnim(level->models[TR::MODEL_LARA_SPEC].animation);
                break;
            }

            default : ;
        }

        if (hitType != TR::HIT_LAVA) {
            TR::Level::FloorInfo info;
            getFloorInfo(getRoomIndex(), pos, info);

            if (info.lava && info.floor == pos.y)
                hitType = TR::HIT_LAVA;
        }

        if (hitType == TR::HIT_LAVA) {
            Flame::add(game, this, 0);
            for (int i = 1; i < 10; i++)
                Flame::add(game, this, rand() % getModel()->mCount);
        }

        if (state != oldState)
            velocity = vec3(0.0f);
    };

    bool useItem(TR::Entity::Type item) {
        if (game->isCutscene()) return false;

        switch (item) {
            case TR::Entity::INV_PISTOLS       : wpnChange(TR::Entity::PISTOLS); break;
            case TR::Entity::INV_SHOTGUN       : wpnChange(TR::Entity::SHOTGUN); break;
            case TR::Entity::INV_MAGNUMS       : wpnChange(TR::Entity::MAGNUMS); break;
            case TR::Entity::INV_UZIS          : wpnChange(TR::Entity::UZIS);    break;
            case TR::Entity::INV_MEDIKIT_SMALL :
            case TR::Entity::INV_MEDIKIT_BIG   :
                saveStats.mediUsed += (item == TR::Entity::INV_MEDIKIT_SMALL) ? 1 : 2;
                usedItem = item;
                break;
            case TR::Entity::INV_PUZZLE_1   :
            case TR::Entity::INV_PUZZLE_2   :
            case TR::Entity::INV_PUZZLE_3   :
            case TR::Entity::INV_PUZZLE_4   :
            case TR::Entity::INV_KEY_ITEM_1 :
            case TR::Entity::INV_KEY_ITEM_2 :
            case TR::Entity::INV_KEY_ITEM_3 :
            case TR::Entity::INV_KEY_ITEM_4 :
                if (usedItem == item)
                    return false;
                usedItem = item;
                break;
            case TR::Entity::INV_LEADBAR  :
                for (int i = 0; i < level->entitiesCount; i++) {
                    const TR::Entity &e = level->entities[i];
                    if (e.controller && e.type == TR::Entity::MIDAS_HAND) {
                        MidasHand *controller = (MidasHand*)e.controller;
                        if (controller->interaction) {
                            controller->invItem = item;
                            return false; // remove item from inventory
                        }
                        return true;
                    }
                }
                return false;
            default : return false;
        }
        return true;
    }

    int goUnderwater() {
        angle.x = -PI * 0.25f;
        game->waterDrop(pos, 256.0f, 0.2f);
        stand = STAND_UNDERWATER;
        return animation.setAnim(ANIM_SURF_DIVE);
    }

    bool doPickUp() {
        if (!animation.canSetState(STATE_PICK_UP))
            return false;

        int room = getRoomIndex();

        pickupListCount = 0;

        for (int i = 0; i < level->entitiesCount; i++) {
            TR::Entity &entity = level->entities[i];
            if (!entity.controller || !entity.isPickup())
                continue;

            Controller *controller = (Controller*)entity.controller;

            if (controller->getRoomIndex() != room || controller->flags.invisible)
                continue;

            if (entity.type == TR::Entity::CRYSTAL) {
                if (Input::lastState[camera->cameraIndex] == cAction) {
                    vec3 dir = controller->pos - pos;
                    if (dir.length2() < SQR(350.0f) && getDir().dot(dir.normal()) > COS30) {
                        pickupListCount = 0;
                        game->invShow(camera->cameraIndex, Inventory::PAGE_SAVEGAME, i);
                        return true;
                    }
                }
            } else {
                if (!canPickup(controller))
                    continue;

                ASSERT(pickupListCount < COUNT(pickupList));
                pickupList[pickupListCount++] = controller;
            }
        }

        if (pickupListCount > 0) {
            state = STATE_PICK_UP;
            return true;
        }

        return false;
    }

    bool canPickup(Controller *controller) {
        TR::Entity::Type type = controller->getEntity().type;

        // get limits
        TR::Limits::Limit *limit;
        switch (type) {
            case TR::Entity::SCION_PICKUP_QUALOPEC : limit = &TR::Limits::SCION; break;
            case TR::Entity::SCION_PICKUP_HOLDER   : limit = &TR::Limits::SCION_HOLDER; break;
            default : limit = level->rooms[getRoomIndex()].flags.water ? &TR::Limits::PICKUP_UNDERWATER : &TR::Limits::PICKUP;
        }

        if (!checkInteraction(controller, limit, true))
            return false;

        if (stand == Character::STAND_UNDERWATER)
            angle.x = -25 * DEG2RAD;

        // set new state
        switch (type) {
            case TR::Entity::SCION_PICKUP_QUALOPEC :
                animation.setAnim(level->models[TR::MODEL_LARA_SPEC].animation);
                camera->doCutscene(pos, angle.y);
                break;
            case TR::Entity::SCION_PICKUP_HOLDER   :
                animation.setAnim(level->models[TR::MODEL_LARA_SPEC].animation);
                
                angle = controller->angle;
                pos   = controller->pos - vec3(0, -280, LARA_RADIUS + 512).rotateY(angle.y);

                camera->doCutscene(pos, angle.y - PI * 0.5f);
                break;
            default : ; 
        }

        return true;
    }

    int doTutorial(int track) {
        if (level->version & TR::VER_TR1)
            switch (track) { // GYM tutorial routine
                case 28 : if (level->state.tracks[track].once && state == STATE_UP_JUMP) track = 29; break;
                case 37 : 
                case 41 : if (state != STATE_HANG) return 0; break;
                case 42 : if (level->state.tracks[track].once && state == STATE_HANG) track = 43; break;
                case 49 : if (state != STATE_SURF_TREAD) return 0; break;
                case 50 : // end of GYM
                    if (level->state.tracks[track].once) {
                        timer += Core::deltaTime;
                        if (timer > 3.0f)
                            game->loadNextLevel();
                    } else {
                        if (state != STATE_WATER_OUT)
                            return 0;
                        timer = 0.0f;
                    }
                    break;
            }
        return track;
    }


    bool checkInteraction(Controller *controller, const TR::Limits::Limit *limit, bool action) {
        if ((state != STATE_STOP && state != STATE_TREAD && state != STATE_PUSH_PULL_READY) || !action || !emptyHands())
            return false;

        vec3 tmpAngle = controller->angle;
        vec3 ctrlAngle = controller->angle;
        if (stand == STAND_UNDERWATER)
            ctrlAngle.x = -25 * DEG2RAD;
        if (!limit->alignAngle)
            ctrlAngle.y = angle.y;
        controller->angle = ctrlAngle;
        mat4 m = controller->getMatrix();
        controller->angle = tmpAngle;

        float fx = 0.0f;
        if (!limit->alignHoriz)
            fx = (m.transpose() * vec4(pos - controller->pos, 0.0f)).x;

        vec3 targetPos = controller->pos + (m * vec4(fx, limit->dy, limit->dz, 0.0f)).xyz();

        vec3 deltaAbs = pos - targetPos;

        vec3 deltaRel = (m.transpose() * vec4(pos - controller->pos, 0.0f)).xyz(); // inverse transform
        
        // set item orientation to hack limits check
        if (limit->box.contains(deltaRel)) {
            float deltaAngY = shortAngle(angle.y, ctrlAngle.y);

            if (stand == STAND_UNDERWATER) {
                float deltaAngX = shortAngle(angle.x, ctrlAngle.x);

                if (deltaAbs.length() > 64.0f || max(fabs(deltaAngX), fabs(deltaAngY)) > (10.0f * DEG2RAD)) {
                    pos     -= deltaAbs.normal() * min(deltaAbs.length(), Core::deltaTime * 512.0f);
                    angle.x += sign(deltaAngX)   * min(fabsf(deltaAngX), Core::deltaTime * (90.0f * DEG2RAD));
                    angle.y += sign(deltaAngY)   * min(fabsf(deltaAngY), Core::deltaTime * (90.0f * DEG2RAD));
                    return false;
                }
            }

            if (fabsf(deltaAngY) <= limit->ay * DEG2RAD) {
            // align
                if (limit->alignAngle)
                    angle = controller->angle;
                else
                    angle.x = angle.z = 0.0f;

                pos      = targetPos;
                velocity = vec3(0.0f);
                speed    = 0.0f;

                return true;
            }
        }

        return false;
    }

    void refreshCamera(const TR::Level::FloorInfo &info) {
        const TR::FloorData::TriggerCommand *cameraCmdSwitch = NULL;
        const TR::FloorData::TriggerCommand *cameraCmdTarget = NULL;
        
        int cmdIndex = 0;

        while (cmdIndex < info.trigCmdCount) {
            const TR::FloorData::TriggerCommand &cmd = info.trigCmd[cmdIndex++];

            switch (cmd.action) {
                case TR::Action::CAMERA_SWITCH :
                    ASSERT(!cameraCmdSwitch);
                    cameraCmdSwitch = &cmd;
                    cmdIndex++; // skip camera info cmd
                    break;
                case TR::Action::CAMERA_TARGET :
                    ASSERT(!cameraCmdTarget);
                    cameraCmdTarget = &cmd;
                    break;
                default : ;
            }
        }

        if (cameraCmdTarget && camera->mode != Camera::MODE_LOOK && camera->mode != Camera::MODE_COMBAT) {
            camera->viewTarget = (Controller*)level->entities[cameraCmdTarget->args].controller;
        }

        if (cameraCmdSwitch) {
            if (cameraCmdSwitch->args == camera->viewIndexLast) {
                camera->viewIndex = camera->viewIndexLast;

                if (camera->mode == Camera::MODE_LOOK || camera->mode == Camera::MODE_COMBAT || camera->timer < 0) {
                    camera->timer = -1.0f;
                    camera->viewTarget = NULL;
                } else {
                    camera->mode = Camera::MODE_STATIC;
                }
            } else {
                camera->viewTarget = NULL;
            }
        } else {
            if (viewTarget && camera->viewTarget != camera->viewTargetLast) {
                camera->viewTarget = NULL;
            }
        }
    }

    void checkTrigger(Controller *controller, bool heavy) {
        TR::Level::FloorInfo info;
        getFloorInfo(controller->getRoomIndex(), controller->pos, info);

        if (getEntity().isLara() && info.lava && info.floor == pos.y) {
            hit(LARA_MAX_HEALTH + 1, NULL, TR::HIT_LAVA);
            return;
        }

        if (!info.trigCmdCount) return; // has no trigger

        if (camera->mode != Camera::MODE_HEAVY) {
            refreshCamera(info);
        }

        TR::Limits::Limit *limit = NULL;
        bool switchIsDown = false;
        float timer = info.trigInfo.timer == 1 ? EPS : float(info.trigInfo.timer);
        int cmdIndex = 0;
        int actionState = state;

        switch (info.trigger) {
            case TR::Level::Trigger::ACTIVATE : break;

            case TR::Level::Trigger::SWITCH : {
                Switch *controller = (Switch*)level->entities[info.trigCmd[cmdIndex++].args].controller;

                if (controller->flags.state != TR::Entity::asActive) {
                    limit = state == STATE_STOP ? (controller->getEntity().type == TR::Entity::SWITCH_BUTTON ? &TR::Limits::SWITCH_BUTTON : &TR::Limits::SWITCH) : &TR::Limits::SWITCH_UNDERWATER;
                    if (checkInteraction(controller, limit, Input::state[camera->cameraIndex][cAction])) {
                        actionState = (controller->state == Switch::STATE_DOWN && stand == STAND_GROUND) ? STATE_SWITCH_UP : STATE_SWITCH_DOWN;

                        int animIndex;
                        switch (controller->getEntity().type) {
                            case TR::Entity::SWITCH_BUTTON : animIndex = ANIM_PUSH_BUTTON; break;
                            case TR::Entity::SWITCH_BIG    : animIndex = controller->state == Switch::STATE_DOWN ? ANIM_SWITCH_BIG_UP : ANIM_SWITCH_BIG_DOWN; break;
                            default : animIndex = -1;
                        }

                        if (animation.setState(actionState, animIndex)) 
                            controller->activate();
                    }
                }

                if (!controller->setTimer(timer))
                    return;

                switchIsDown = controller->state == Switch::STATE_DOWN;
                break;
            }

            case TR::Level::Trigger::KEY : {
                TR::Entity &entity = level->entities[info.trigCmd[cmdIndex++].args];
                KeyHole *controller = (KeyHole*)entity.controller;

                if (controller->flags.state == TR::Entity::asNone) {
                    if (controller->flags.active == TR::ACTIVE || state != STATE_STOP)
                        return;

                    actionState = entity.isPuzzleHole() ? STATE_USE_PUZZLE : STATE_USE_KEY;
                    if (!animation.canSetState(actionState))
                        return;

                    limit = actionState == STATE_USE_PUZZLE ? &TR::Limits::PUZZLE_HOLE : &TR::Limits::KEY_HOLE;
                    if (!checkInteraction(controller, limit, isPressed(ACTION) || usedItem != TR::Entity::NONE))
                        return;

                    if (usedItem == TR::Entity::NONE) {
                        if (isPressed(ACTION) && !game->invChooseKey(camera->cameraIndex, entity.type))
                            game->playSound(TR::SND_NO, pos, Sound::PAN); // no compatible items in inventory
                        return;
                    }

                    if (TR::Level::convToInv(TR::Entity::getItemForHole(entity.type)) != usedItem) { // check compatibility if user select other
                        game->playSound(TR::SND_NO, pos, Sound::PAN); // uncompatible item
                        return;
                    }

                    keyHole = controller;

                    if (game->invUse(camera->cameraIndex, usedItem)) {
                        keyItem = game->addEntity(usedItem, getRoomIndex(), pos, 0);
                        keyItem->lockMatrix = true;
                        keyItem->pos     = keyHole->pos + vec3(0, -590, 484).rotateY(-keyHole->angle.y);
                        keyItem->angle.x = PI * 0.5f;
                        keyItem->angle.y = keyHole->angle.y;
                    }

                    animation.setState(actionState);
                }

                if (controller->flags.state != TR::Entity::asInactive)
                    return;

                break;
            }

            case TR::Level::Trigger::PICKUP : {
                Controller *controller = (Controller*)level->entities[info.trigCmd[cmdIndex++].args].controller;
                if (!controller->flags.invisible)
                    return;
                break;
            }

            case TR::Level::Trigger::COMBAT :
                if (wpnReady() && !emptyHands())
                    return;
                break;

            case TR::Level::Trigger::PAD :
            case TR::Level::Trigger::ANTIPAD :
                if (pos.y != info.floor) return;
                break;

            case TR::Level::Trigger::HEAVY :
                if (!heavy) return;
                break;
            case TR::Level::Trigger::DUMMY :
                return;
        }

        bool needFlip = false;
        TR::Effect::Type effect = TR::Effect::NONE;

        while (cmdIndex < info.trigCmdCount) {
            TR::FloorData::TriggerCommand &cmd = info.trigCmd[cmdIndex++];

            switch (cmd.action) {
                case TR::Action::ACTIVATE : {
                    if (cmd.args >= level->entitiesBaseCount) {
                        break;
                    }
                    TR::Entity &e = level->entities[cmd.args];
                    Controller *controller = (Controller*)e.controller;
                    ASSERT(controller);
                    TR::Entity::Flags &flags = controller->flags;

                    if (flags.once)
                        break;
                    controller->timer = timer;

                    if (info.trigger == TR::Level::Trigger::SWITCH)
                        flags.active ^= info.trigInfo.mask;
                    else if (info.trigger == TR::Level::Trigger::ANTIPAD)
                        flags.active &= ~info.trigInfo.mask;
                    else
                        flags.active |= info.trigInfo.mask;

                    if (flags.active != TR::ACTIVE)
                        break;

                    flags.once |= info.trigInfo.once;
                    
                    controller->activate();
                    break;
                }
                case TR::Action::CAMERA_SWITCH : {
                    TR::FloorData::TriggerCommand &cam = info.trigCmd[cmdIndex++];

                    if (!level->cameras[cmd.args].flags.once) {
                        camera->viewIndex = cmd.args;

                        if (!(info.trigger == TR::Level::Trigger::COMBAT) &&
                            !(info.trigger == TR::Level::Trigger::SWITCH && info.trigInfo.timer && !switchIsDown) &&
                             (info.trigger == TR::Level::Trigger::SWITCH || camera->viewIndex != camera->viewIndexLast))
                        {
                            camera->smooth = cam.speed > 0;
                            camera->mode   = heavy ? Camera::MODE_HEAVY : Camera::MODE_STATIC;
                            camera->timer  = cam.timer == 1 ? EPS : float(cam.timer);
                            camera->speed  = cam.speed * 8;

                            level->cameras[camera->viewIndex].flags.once |= cam.once;
                        }
                    }
                    break;
                }
                case TR::Action::FLOW :
                    applyFlow(level->cameras[cmd.args]);
                    break;
                case TR::Action::FLIP : {
                    SaveState::ByteFlags &flip = level->state.flipmaps[cmd.args];

                    if (flip.once)
                        break;

                    if (info.trigger == TR::Level::Trigger::SWITCH)
                        flip.active ^= info.trigInfo.mask;
                    else
                        flip.active |= info.trigInfo.mask;

                    if (flip.active == TR::ACTIVE)
                        flip.once |= info.trigInfo.once;

                    if ((flip.active == TR::ACTIVE) ^ level->state.flags.flipped)
                         needFlip = true;

                    break;
                }
                case TR::Action::FLIP_ON :
                    if (level->state.flipmaps[cmd.args].active == TR::ACTIVE && !level->state.flags.flipped)
                        needFlip = true;
                    break;
                case TR::Action::FLIP_OFF :
                    if (level->state.flipmaps[cmd.args].active == TR::ACTIVE && level->state.flags.flipped)
                        needFlip = true;
                    break;
                case TR::Action::CAMERA_TARGET :
                    if (camera->mode == Camera::MODE_STATIC || camera->mode == Camera::MODE_HEAVY) {
                        camera->viewTarget = (Controller*)level->entities[cmd.args].controller;
                    }
                    break;
                case TR::Action::END :
                    game->loadNextLevel();
                    break;
                case TR::Action::SOUNDTRACK : {
                    int track = doTutorial(cmd.args);

                    if (track == 0) break;

                // check trigger
                    SaveState::ByteFlags &flags = level->state.tracks[track];

                    if (flags.once)
                        break;

                    if (info.trigger == TR::Level::Trigger::SWITCH)
                        flags.active ^= info.trigInfo.mask;
                    else if (info.trigger == TR::Level::Trigger::ANTIPAD)
                        flags.active &= ~info.trigInfo.mask;
                    else
                        flags.active |= info.trigInfo.mask;

                    if ( (flags.active == TR::ACTIVE) || (((level->version & (TR::VER_TR2 | TR::VER_TR3))) && flags.active) ) {
                        flags.once |= info.trigInfo.once;
                        game->playTrack(track);
                    } else
                        game->stopTrack();

                    break;
                }
                case TR::Action::EFFECT :
                    effect = TR::Effect::Type(cmd.args);
                    break;
                case TR::Action::SECRET :
                    if (!(saveStats.secrets & (1 << cmd.args))) {
                        saveStats.secrets |= 1 << cmd.args;
                        if (!game->playSound(TR::SND_SECRET, pos))
                            game->playTrack(TR::TRACK_TR1_SECRET, true);
                    }
                    break;
                case TR::Action::CLEAR_BODIES :
                    break;
                case TR::Action::FLYBY :
                    cmdIndex++; // TODO
                    break;
                case TR::Action::CUTSCENE :
                    cmdIndex++; // TODO
                    break;
            }
        }

        if (needFlip) {
            game->flipMap();
            game->setEffect(this, effect);
        }
    }

    void waterSplash() {
        if (level->extra.waterSplash > -1)
            game->addEntity(TR::Entity::WATER_SPLASH, getRoomIndex(), vec3(pos.x, waterLevel, pos.z));
        specular = LARA_WET_SPECULAR;
    }

    virtual Stand getStand() { // TODO remove
        /*
        if (dozy) return STAND_UNDERWATER;

        if (stand == STAND_ONWATER && state == STATE_STOP)
            return STAND_GROUND;

        if (state == STATE_HANG || state == STATE_HANG_LEFT || state == STATE_HANG_RIGHT) {
            if (input & ACTION)
                return STAND_HANG;
            animation.setAnim(ANIM_FALL_HANG);
            velocity = vec3(0.0f);
            pos.y += 128.0f;
            return STAND_AIR;
        }

        if (state == STATE_HANDSTAND || (state == STATE_HANG_UP && animation.index != ANIM_CLIMB_JUMP))
            return STAND_HANG;

        if (getRoom().flags.water) {
            if (waterDepth > LARA_WADE_MAX_DEPTH || (waterLevel == 0.0f && waterDepth == 0.0f)) {
                if (stand == STAND_GROUND)
                    return STAND_AIR;
                wpnHide();
                if (stand == STAND_UNDERWATER || stand == STAND_ONWATER)
                    return stand;
                if (stand == STAND_AIR) {
                    if (velocity.y > 0.0f && pos.y - waterLevel > 300.0) {
                        stopScreaming();
                        return STAND_UNDERWATER;
                    }
                } else if (stand == STAND_GROUND) {
                    return STAND_UNDERWATER;
                } else {
                    pos.y = waterLevel;
                    updateRoom();
                    switch (state) {
                        case STATE_BACK       : animation.setAnim(ANIM_SURF_BACK); break;
                        case STATE_STEP_LEFT  : animation.setAnim(ANIM_SURF_LEFT); break;
                        case STATE_STEP_RIGHT : animation.setAnim(ANIM_SURF_RIGHT); break;
                        default               : animation.setAnim(ANIM_SURF_SWIM); 
                    }
                    return STAND_ONWATER;
                }
            } else if (waterDepth > LARA_WADE_MIN_DEPTH) {
                if (stand == STAND_GROUND && (waterLevel + waterDepth) - pos.y > 300)
                    return STAND_AIR;

                if (stand == STAND_AIR) {
                    if (velocity.y > 0.0f && pos.y - waterLevel > 300.0) {
                        waterSplash();
                        pos.y = waterLevel + waterDepth;
                        game->playSound(TR::SND_WATER_SPLASH, pos, Sound::PAN);
                        updateRoom();
                        animation.setAnim(ANIM_SWIM_STAND);
                        stopScreaming();
                        return STAND_WADE;
                    }
                } else if (stand == STAND_UNDERWATER) {
                    if (waterDepth <= LARA_SWIM_MIN_DEPTH) {
                        pos.y = waterLevel + waterDepth;
                        updateRoom();
                        animation.setAnim(ANIM_SWIM_STAND);
                        return STAND_WADE;
                    }
                } else if (stand == STAND_ONWATER || stand == STAND_GROUND) {
                    if (state == STATE_RUN || state == STATE_SURF_SWIM)
                        animation.setAnim(ANIM_WADE);
                    pos.y = waterLevel + waterDepth;
                    updateRoom();
                    return STAND_WADE;
                } else
                    return STAND_WADE;
            }
        }

        if (stand == STAND_ONWATER && state != STATE_STOP) {
            if (!getRoom().flags.water && state != STATE_WATER_OUT && state != STATE_STOP)
                return STAND_AIR;
            if (state != STATE_DIVE)
                return stand;
        }

        if (stand == STAND_UNDERWATER || stand == STAND_ONWATER) {
            return stand;
        }

        TR::Level::FloorInfo info;
        getFloorInfo(getRoomIndex(), pos, info);

        if ((stand == STAND_SLIDE || stand == STAND_GROUND) && (state != STATE_FORWARD_JUMP && state != STATE_BACK_JUMP) && health > 0.0f) {
            if (pos.y + 8 >= info.floor && (abs(info.slantX) > 2 || abs(info.slantZ) > 2)) {
                pos.y = info.floor;

                slide();

                return STAND_SLIDE;
            }
        }

        int extra = (stand != STAND_AIR && stand != STAND_SLIDE) ? 256 : 0;

        if (pos.y + extra >= info.floor && !(stand == STAND_AIR && velocity.y < 0)) {
            if (stand != STAND_GROUND) {
                pos.y = info.floor;
            // get damage from falling
                if (velocity.y > 0.0f) {
                    stopScreaming();
                    if (state == STATE_FAST_DIVE && velocity.y > 133.0f) {
                        hit(health + 1.0f, NULL, TR::HIT_FALL);
                    } else {
                        float v = velocity.y - 140.0f;
                        if (v > 14.0f)
                            hit(health + 1.0f, NULL, TR::HIT_FALL);
                        else
                            if (v > 0.0f)
                                hit(v * v * LARA_MAX_HEALTH / 196.0f, NULL, TR::HIT_FALL);
                    }

                    if (state == STATE_FALL && health > 0.0f)
                        animation.setAnim(ANIM_LANDING);
                }
            }
            return STAND_GROUND;
        }
        */
        return STAND_AIR;
    }

    void startScreaming() {
        game->playSound(TR::SND_SCREAM, pos, Sound::PAN | Sound::UNIQUE);
    }

    void stopScreaming() {
        Sound::stop(TR::SND_SCREAM);
    }

    virtual int getHeight() {
        if (stand == STAND_GROUND || stand == STAND_AIR)
            return 768;
        return 0;
    }

    virtual int getStateAir() {
        angle.x = 0.0f;

        float EPSILON = 3.0f;
        if (velocity.y > (131.0f + EPSILON) && state != STATE_SWAN_DIVE && state != STATE_FAST_DIVE)
            return STATE_FALL;

        if (state == STATE_REACH && getDir().dot(vec3(velocity.x, 0.0f, velocity.z)) < 0)
            velocity.x = velocity.z = 0.0f;

        if ((state == STATE_REACH || state == STATE_UP_JUMP) && (input & ACTION) && emptyHands()) {
            if (state == STATE_REACH && velocity.y < 0.0f)
                return state;

            Box bounds = animation.getBoundingBox(pos, 0);

            vec3 p = vec3(pos.x, bounds.min.y, pos.z);

            Collision c = Collision(this, getRoomIndex(), p, getDir() * 128.0f, vec3(0.0f), LARA_RADIUS, angleExt, 0, 0, 0, 0);

            if (c.side != Collision::FRONT)
                return state;

            float floor   = c.info[Collision::FRONT].floor;
            float ceiling = c.info[Collision::FRONT].ceiling;
            float hands   = bounds.min.y;

            if (fabsf(floor - hands) < 64 && int(floor) != int(ceiling)) {
                alignToWall(-LARA_RADIUS);
                pos.y = float(floor + LARA_HANG_OFFSET);
                stand = STAND_HANG;

                if (state == STATE_REACH) {
                    velocity = vec3(0.0f);
                    TR::Level::FloorInfo info;
                    getFloorInfo(getRoomIndex(), pos + getDir() * 256.0f, info);
                    int h = int(info.ceiling - floor);
                    return animation.setAnim((h > 0 && h < 400) ? ANIM_HANG_SWING : ANIM_HANG);
                } else
                    return animation.setAnim(ANIM_HANG, -15);
            }
        }

        if ((level->version & TR::VER_VERSION) > TR::VER_TR1) {
            bool roll = (input & (FORTH | BACK)) == (FORTH | BACK);

            if ((state == STATE_FORWARD_JUMP && (roll || (input & BACK)  )) ||
                (state == STATE_BACK_JUMP    && (roll || (input & FORTH) )) || 
                (state == STATE_FAST_DIVE    &&  roll))
                return STATE_AIR_ROLL;
        }

        if (state == STATE_FORWARD_JUMP || state == STATE_FALL_BACK) {
            if (emptyHands()) {
                if (input & ACTION) return STATE_REACH;
                if ((input & (JUMP | FORTH | WALK)) == (JUMP | FORTH | WALK)) return STATE_SWAN_DIVE;
            }
        } else
            if (state != STATE_FALL && state != STATE_FALL_BACK && state != STATE_SWAN_DIVE && state != STATE_FAST_DIVE && state != STATE_REACH && state != STATE_UP_JUMP && state != STATE_BACK_JUMP && state != STATE_LEFT_JUMP && state != STATE_RIGHT_JUMP)
                return animation.setAnim(ANIM_FALL_FORTH);// (state == STATE_FAST_BACK || state == STATE_SLIDE_BACK || state == STATE_ROLL_2) ? ANIM_FALL_BACK :  ANIM_FALL_FORTH);

        if (state == STATE_SWAN_DIVE)
            return STATE_FAST_DIVE;

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
            if (!e.controller || !e.isBlock())
                continue;

            Block *block = (Block*)e.controller;
            float oldAngle = block->angle.y;
            block->angle.y = angleQuadrant(angle.y) * (PI * 0.5f);

            if (!checkInteraction(block, &TR::Limits::BLOCK, (input & ACTION) != 0)) {
                block->angle.y = oldAngle;
                continue;
            }

            return block;
        }
        return NULL;
    }

    int getTurn() {
        if (state == STATE_FAST_TURN)
            return state;

        bool weaponReady = wpnReady() && !emptyHands();

        if (input & LEFT)  return (state == STATE_TURN_LEFT  && (animation.prev == animation.index || weaponReady)) ? STATE_FAST_TURN : STATE_TURN_LEFT;
        if (input & RIGHT) return (state == STATE_TURN_RIGHT && (animation.prev == animation.index || weaponReady)) ? STATE_FAST_TURN : STATE_TURN_RIGHT;

        ASSERT(false);
        return STATE_STOP;
    }

    bool checkClimb() {
        if ((input & (FORTH | ACTION)) == (FORTH | ACTION) && !(input & (LEFT | RIGHT)) && (animation.index == ANIM_STAND || animation.index == ANIM_STAND_NORMAL) && emptyHands() && collision.side == Collision::FRONT) { // TODO: get rid of animation.index
            float floor   = collision.info[Collision::FRONT].floor;
            float ceiling = collision.info[Collision::FRONT].ceiling; 

            float h = pos.y - floor;

            int aIndex = animation.index;
            bool canClimb = (floor - ceiling >= LARA_HEIGHT) && (h >= 256);

            if (canClimb && h <= 2 * 256 + 128) {
                aIndex = ANIM_CLIMB_2;
                pos.y  = floor + 512.0f;
            } else if (canClimb && h <= 3 * 256 + 128) {
                aIndex = ANIM_CLIMB_3;
                pos.y  = floor + 768.0f;
            } else if (h > 3 * 128 + 128 && h <= 7 * 256 + 128)
                aIndex = ANIM_CLIMB_JUMP;

            if (aIndex != animation.index) {
                alignToWall(-LARA_RADIUS);
                animation.setAnim(aIndex);
                return true;
            }
        }
        return false;
    }

    virtual int getStateGround() {
        /*
        int res = STATE_STOP;
        angle.x = 0.0f;

        if (waterDepth > 0.0f && !(animation.frameIndex % 4))
            game->waterDrop(getJoint(jointChest).pos + vec3(randf(), 0.0f, randf()) * 64.0f, 96.0f, 0.02f);

        if ((input == ACTION) && (state == STATE_STOP) && emptyHands() && doPickUp())
            return state;

        if (checkClimb())
            return state;

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
                collision  = Collision(this, getRoomIndex(), p, vec3(0.0f), vec3(0.0f), LARA_RADIUS * 2.5f, ext, 0, LARA_HEIGHT, 256 + 128, 0xFFFFFF);
                if (collision.side == Collision::FRONT)
                    res = STATE_UP_JUMP;
            }

            return res;
        }

        if (state == STATE_RUN) {
            if (animation.index == ANIM_RUN_START) {
                canJump = false;
            } else if (animation.index == ANIM_RUN) {
                if (animation.frameIndex >= 4 && animation.frameIndex <= 5) {
                    canJump = true;
                }
            } else {
                canJump = true;
            }
        }

        // jump button is pressed
        if (input & JUMP) {
            if ((input & FORTH) && state == STATE_FORWARD_JUMP)
                return STATE_RUN;
            if (state == STATE_RUN && canJump)
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

            if (state == STATE_STOP && res != STATE_STOP)
                res = checkMove(res, maxAscent, maxDescent);
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
        else if (input & (LEFT | RIGHT))
            return getTurn();

        if (state == STATE_STOP && res != STATE_STOP) {
            float ext = angle.y + (res == STATE_RUN ? 0.0f : PI);
            vec3 p = pos;
            collision  = Collision(this, getRoomIndex(), p, vec3(0.0f), vec3(0.0f), LARA_RADIUS * 1.1f, ext, 0, LARA_HEIGHT, 256 + 128, 0xFFFFFF);
            if (collision.side == Collision::FRONT)
                res = STATE_STOP;
        }

        return res;
            */
        return STATE_STOP;
    }

    virtual int getStateSlide() { // TODO remove
        /*
        if (input & JUMP)
            return state == STATE_SLIDE ? STATE_FORWARD_JUMP : STATE_BACK_JUMP;
        // TODO: update slide direction
                */
        return state;
    }

    virtual int getStateHang() { // TODO remove
        /*
        if (input & LEFT)  return STATE_HANG_LEFT;
        if (input & RIGHT) return STATE_HANG_RIGHT;
        if (input & FORTH) {
            // possibility check
            TR::Level::FloorInfo info;
            getFloorInfo(getRoomIndex(), pos + getDir() * (LARA_RADIUS + 2.0f), info);
            if (info.floor - info.ceiling >= LARA_HEIGHT)
                return (input & WALK) ? STATE_HANDSTAND : STATE_HANG_UP;            
        }
        */
        return STATE_HANG;
    }

    virtual int getStateUnderwater() { // TODO remove
        /*
        if ((input == ACTION) && (state == STATE_TREAD) && emptyHands() && doPickUp())
            return state;

        if (state == STATE_RUN || state == STATE_BACK || state == STATE_FAST_BACK || state == STATE_FORWARD_JUMP || state == STATE_UP_JUMP || state == STATE_BACK_JUMP || state == STATE_LEFT_JUMP || state == STATE_RIGHT_JUMP || state == STATE_FALL || state == STATE_REACH || state == STATE_SLIDE || state == STATE_SLIDE_BACK) {
            game->waterDrop(pos, 256.0f, 0.2f);
            waterSplash();
            pos.y += 100.0f;
            angle.x = -45.0f * DEG2RAD;
            velocity.y *= 1.5f;
            return animation.setAnim(ANIM_WATER_FALL); // TODO: wronng animation
        }

        if ((level->version & TR::VER_VERSION) > TR::VER_TR1 && state != STATE_UW_ROLL) {
            if ((input & (FORTH | BACK)) == (FORTH | BACK))
                return animation.setAnim(ANIM_UW_ROLL);
        }

        if (state == STATE_SWAN_DIVE || state == STATE_FAST_DIVE) {
            angle.x = (state == STATE_SWAN_DIVE ? -45.0f : -85.0f) * DEG2RAD;
            pos.y += 100.0f;
            velocity.y *= 2.0f;
            game->waterDrop(pos, 128.0f, 0.2f);
            waterSplash();
            return STATE_DIVE;
        }

        if (input & JUMP) return STATE_SWIM;

        if (state == STATE_GLIDE && speed < LARA_SWIM_SPEED * 2.0f / 3.0f)
            return STATE_TREAD;
            */
        return STATE_GLIDE;
    }

    virtual int getStateOnwater() {
        /*
        angle.x = 0.0f;

        if (state == STATE_WATER_OUT) return state;

        if (state != STATE_SURF_TREAD && state != STATE_SURF_LEFT && state != STATE_SURF_RIGHT && state != STATE_SURF_SWIM && state != STATE_SURF_BACK && state != STATE_STOP) {
            game->waterDrop(pos, 128.0f, 0.2f);
            specular = LARA_WET_SPECULAR;
            switch (state) {
                case STATE_WADE       : return animation.setAnim(ANIM_SURF_SWIM);
                case STATE_BACK       : return animation.setAnim(ANIM_SURF_BACK);
                case STATE_STEP_LEFT  : return animation.setAnim(ANIM_SURF_LEFT);
                case STATE_STEP_RIGHT : return animation.setAnim(ANIM_SURF_RIGHT);
                default               : return animation.setAnim(ANIM_SURF);
            }
        }



        if (input & FORTH) {
            if (input & JUMP) 
                return goUnderwater();

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
        */
        return STATE_SURF_TREAD;
    }

    virtual int getStateWade() { // TODO remove
    /*
        angle.x = 0.0f;

        if (waterDepth > 0.0f && !(animation.frameIndex % 4))
            game->waterDrop(getJoint(jointChest).pos + vec3(randf(), 0.0f, randf()) * 64.0f, 96.0f, 0.02f);

        if ((input & FORTH) && (input & BACK))
            input &= ~(FORTH | BACK);

        if (checkClimb())
            return state;

        if (input & JUMP) {
            return STATE_COMPRESS;
        }

        if (state == STATE_COMPRESS || state == STATE_UP_JUMP)
            return state;

        if (state != STATE_WADE) {
            if (state == STATE_SWIM || state == STATE_GLIDE)
                return animation.setAnim(ANIM_WADE_ASCEND);
            if (state == STATE_COMPRESS)
                return STATE_UP_JUMP;
            if (state == STATE_RUN) {
                waterSplash();
                return animation.setAnim(getLeadingFoot() ? ANIM_WADE_RUN_LEFT : ANIM_WADE_RUN_RIGHT);
            }
            if (state == STATE_SURF_SWIM)
                return animation.setAnim(ANIM_WADE_SWIM);
            if (state == STATE_SURF_BACK)
                return animation.setAnim(ANIM_BACK);
        }

        if (input & FORTH) {
            if (checkMove(STATE_WADE) != STATE_WADE)
                return STATE_STOP;
            if (state == STATE_STOP)
                return animation.setAnim(ANIM_WADE_STAND);
            if (state != STATE_WADE && state != STATE_WATER_OUT)
                return animation.setAnim(ANIM_WADE);
            return STATE_WADE;
        }

        if (input & BACK) {
            if (checkMove(STATE_BACK) != STATE_BACK)
                return STATE_STOP;
            return STATE_BACK;
        }

        // walk button is pressed
        if ((input & WALK) && (input & (LEFT | RIGHT)) && animation.index != ANIM_RUN_START) {
            int res;

            float ext = angle.y;

            if (input & LEFT) {
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

            if (state == STATE_STOP && res != STATE_STOP)
                res = checkMove(res, maxAscent, maxDescent);
            return res;
        }

        if (input & (LEFT | RIGHT))
            return getTurn();
            */
        return STATE_STOP;
    }

    virtual int getStateDeath() { // TODO remove
        return STATE_DEATH;
    }

    virtual int getStateDefault() { // TODO remove
        return STATE_FALL;
    }

    virtual void updateState() {
        Character::updateState();
    }

    void updateCamera() {
        if (camera->mode != ICamera::MODE_FOLLOW)
            return;

        camera->centerView = false;

        if ((waterState == WATER_STATE_WADE) || (waterState == WATER_STATE_SURFACE))
        {
            camera->setAngle(-22, 0);
            return;
        }

        switch (state) {
            case STATE_WATER_OUT  :
                camera->centerView = true;
                break;
            case STATE_DEATH      :
            case STATE_UW_DEATH :
                camera->centerView = true;
                break;
            case STATE_REACH      :
                camera->setAngle(0, 85);
                break;
            case STATE_BACK_JUMP  :
                camera->setAngle(0, 135);
                break;
            case STATE_SLIDE      :
            case STATE_SLIDE_BACK :
                camera->setAngle(-45, 0);
                break;
            case STATE_HANG       :
            case STATE_HANG_LEFT  :
            case STATE_HANG_RIGHT :
                camera->setAngle(-60, 0);
                break;
            case STATE_PUSH_BLOCK :
            case STATE_PULL_BLOCK :
                camera->setAngle(-25, 35);
                camera->centerView = true;
                break;
            case STATE_PUSH_PULL_READY :
                camera->setAngle(0, 75);
                break;
            case STATE_PICK_UP :
                camera->setAngle(-15, -130);
                break;
            case STATE_SWITCH_DOWN :
            case STATE_SWITCH_UP   :
                camera->setAngle(-25, 80);
                break;
            case STATE_USE_KEY    :
            case STATE_USE_PUZZLE :
                camera->setAngle(-25, -80);
                break;
            case STATE_SPECIAL    :
                camera->setAngle(-25, 170);
                camera->centerView = true;
                break;
            case STATE_CLIMB_START :
                camera->setAngle(-20, 0);
                break;
            case STATE_CLIMB_UP    :
                camera->setAngle(-30, 0);
                break;                
            case STATE_CLIMB_LEFT  :
                camera->setAngle(-15, -30);
                break;
            case STATE_CLIMB_END   :
                camera->setAngle(-20, -45);
                camera->centerView = true;
                break;
            case STATE_CLIMB_RIGHT :
                camera->setAngle(-15, 30);
                break;
            case STATE_CLIMB_DOWN  :
                camera->setAngle(-45, 0);
                break;
            default :
                camera->setAngle(0, 0);
        }
    }

    void setDozy(bool enable) {
        if (enable) {
            waterState = WATER_STATE_UNDER;
            gravity = false;
            oxygen = LARA_MAX_OXYGEN;
            health = LARA_MAX_HEALTH;

            v2pos.y += 100;
            updateRoomV2(0);
            stopScreaming();

            v2rot.x = 0;
            setAnimV2(ANIM_UW_GLIDE, true);
            targetState = STATE_SWIM;
        } else {
            stand = getRoom().flags.water ? STAND_UNDERWATER : STAND_AIR;
        }
        dozy = enable;
    }

    virtual int getInput() { // TODO: updateInput
        if (level->isCutsceneLevel()) return 0;

        if (networkInput != -1)
            return networkInput;

        input = 0;
        int pid = camera->cameraIndex;

        if (!dozy && ((Input::state[pid][cAction] && Input::state[pid][cJump] && Input::state[pid][cLook] && Input::state[pid][cDash]) || Input::down[ikO])) {
            setDozy(true);
            return input;
        }

        if (dozy && Input::state[pid][cWalk]) {
            setDozy(false);
            return input;
        }

        input = Character::getInput();

        if (Input::state[pid][cUp])        input |= FORTH;
        if (Input::state[pid][cRight])     input |= RIGHT;
        if (Input::state[pid][cDown])      input |= BACK;
        if (Input::state[pid][cLeft])      input |= LEFT;
        if (Input::state[pid][cRoll])      input  = FORTH | BACK;
        if (Input::state[pid][cJump])      input |= JUMP;
        if (Input::state[pid][cWalk])      input |= WALK;
        if (Input::state[pid][cAction])    input |= ACTION;
        if (Input::state[pid][cWeapon])    input |= WEAPON;
        if (Input::state[pid][cLook] && canLookAt()) input = LOOK;

        if (Input::down[ikP]) {
            switch (level->id) {
                case TR::LVL_TR1_GYM :
                    reset(14, vec3(40448, 3584, 60928), PI * 0.5f, STAND_ONWATER);  // gym (pool)
                    break;
                case TR::LVL_TR1_1 :
                    reset(33, vec3(48229, 4608, 78420), 270 * DEG2RAD);     // level 1 (end)
                    break;
                case TR::LVL_TR1_2 :
                    reset(61, vec3(21987, -1024, 29144), PI * 3.0f * 0.5f); // level 2 (trap door)
                    break;
                case TR::LVL_TR1_3A :
                    reset(51, vec3(41015, 3584, 34494), -PI);        // level 3a (t-rex)
                    break;
                case TR::LVL_TR1_3B :
                    reset(5, vec3(73394, 3840, 60758), 0); // level 3b (scion)
                    break;
                case TR::LVL_TR1_4 :
                    reset(18, vec3(34914, 11008, 41315), 90 * DEG2RAD); // main hall
                    break;
                case TR::LVL_TR1_5 :
                    reset(74, vec3(52549, -3584, 60871), -150 * DEG2RAD); // main hall
                    break;
                case TR::LVL_TR1_6 :
                    reset(73, vec3(73372, 122, 51687), PI * 0.5f);       // level 6 (midas hand)
                    break;
                case TR::LVL_TR1_7A :
                    reset(99,  vec3(45562, -3328, 63366), 225 * DEG2RAD); // level 7a (flipmap)
                    break;
                case TR::LVL_TR1_7B :
//                    reset(77, vec3(36943, -4096, 62821), 270 * DEG2RAD); // level 7b (heavy trigger)
                    reset(49, vec3(45596, -6144, 71579), 0); // level 7b (water trigger)
                    break;
                case TR::LVL_TR2_WALL :
                    //reset(44, vec3(62976, 1536, 23040), 0);
                    //reset(44, vec3(62976, 1536, 23040), 0);
                    break;
                case TR::LVL_TR2_PLATFORM :
                    reset(16, vec3(53029, -5120, 77359), 0);
                    break;
                case TR::LVL_TR3_TEMPLE :
                    reset(204, vec3(40562, 3584, 58694), 0);
                    break;
                default : game->playSound(TR::SND_NO, pos, Sound::PAN);
            }
        }

    // analog control
        rotFactor = vec2(1.0f);

        if (input & LOOK)
            return input;

        if (camera->spectator)
            return input;

        Input::Joystick &joy = Input::joy[Core::settings.controls[pid].joyIndex];

        vec2 L = joy.L;

        if (L.length() < INPUT_JOY_DZ_STICK) L = vec2(0.0f); // dead zone

        if (!((state == STATE_STOP || state == STATE_SURF_TREAD || state == STATE_HANG) && fabsf(L.x) < 0.5f && fabsf(L.y) < 0.5f)) {
            bool moving = state == STATE_RUN || state == STATE_WALK || state == STATE_BACK || state == STATE_FAST_BACK || state == STATE_SURF_SWIM || state == STATE_SURF_BACK || state == STATE_FORWARD_JUMP;

            if (!moving) {
                if (fabsf(L.x) < fabsf(L.y))
                    L.x = 0.0f;
                else
                    L.y = 0.0f;
            }

            if (L.x != 0.0f) {
                input |= (L.x < 0.0f) ? LEFT : RIGHT;
                if (moving || stand == STAND_UNDERWATER || stand == STAND_ONWATER)
                    rotFactor.y = min(fabsf(L.x) / 0.9f, 1.0f);
            }

            if (L.y != 0.0f) {
                input |= (L.y < 0.0f) ? FORTH : BACK;
                if (stand == STAND_UNDERWATER)
                    rotFactor.x = min(fabsf(L.y) / 0.9f, 1.0f);
            }
        }

    // VR control
        if (Core::settings.detail.stereo == Core::Settings::STEREO_VR && camera->firstPerson && canFreeRotate()) {

            if (!(input & WALK)) {
                input &= ~(LEFT | RIGHT);
            }

            vec3 ang = getAngleAbs(Input::hmd.head.dir().xyz());
            angle.y = ang.y;
            if (stand == STAND_UNDERWATER) {
                input &= ~(FORTH | BACK);
                angle.x = ang.x;
            }
        }
        return input;
    }

    virtual bool useHeadAnimation() {
        return state == STATE_WATER_OUT
            || state == STATE_DEATH
            || state == STATE_UW_DEATH
            || state == STATE_HANG
            || state == STATE_HANG_UP
            || state == STATE_HANG_LEFT
            || state == STATE_HANG_RIGHT
            || state == STATE_PUSH_BLOCK
            || state == STATE_PULL_BLOCK
            || state == STATE_PUSH_PULL_READY
            || state == STATE_PICK_UP
            || state == STATE_SWITCH_DOWN
            || state == STATE_SWITCH_UP
            || state == STATE_USE_KEY
            || state == STATE_USE_PUZZLE
            || state == STATE_SPECIAL
            || state == STATE_REACH
            || state == STATE_SWAN_DIVE
            || state == STATE_FAST_DIVE
            || state == STATE_HANDSTAND
            || state == STATE_ROLL_START
            || state == STATE_ROLL_END
            || state == STATE_MIDAS_USE
            || state == STATE_MIDAS_DEATH
            || animation.index == ANIM_CLIMB_2
            || animation.index == ANIM_CLIMB_3
            || animation.index == ANIM_CLIMB_JUMP;
    }

    bool canFreeRotate() {
        return !(useHeadAnimation() || state == STATE_SLIDE || state == STATE_SLIDE_BACK);
    }

    virtual void doCustomCommand(int curFrame, int prevFrame) {
        switch (state) {
            case STATE_PICK_UP : {
                int pickupFrame = stand == STAND_GROUND ? PICKUP_FRAME_GROUND : PICKUP_FRAME_UNDERWATER;
                if (animation.isFrameActive(pickupFrame)) {
                    camera->setup(true);

                    for (int i = 0; i < pickupListCount; i++) {
                        Controller *item = pickupList[i];

                        if (item->getEntity().type == TR::Entity::SCION_PICKUP_HOLDER)
                            continue;
                        item->deactivate();
                        item->flags.invisible = true;
                        game->invAdd(item->getEntity().type, 1);

                        vec4 p = game->projectPoint(vec4(item->pos, 1.0f));

                        if (p.w != 0.0f) {
                            p.x = ( p.x / p.w * 0.5f + 0.5f) * UI::width;
                            p.y = (-p.y / p.w * 0.5f + 0.5f) * UI::height;
                            if (game->getLara(1)) {
                                p.x *= 0.5f;
                            }
                        } else
                            p = vec4(UI::width * 0.5f, UI::height * 0.5f, 0.0f, 0.0f);

                        UI::addPickup(item->getEntity().type, camera->cameraIndex, vec2(p.x, p.y));
                        saveStats.pickups++;
                    }
                    pickupListCount = 0;
                }
                break;
            }
            case STATE_USE_KEY    :
            case STATE_USE_PUZZLE : {
                if (keyHole) {
                    if (animation.isFrameActive(state == STATE_USE_PUZZLE ? PUZZLE_FRAME : KEY_FRAME)) {
                        keyHole->activate();
                        if (keyItem) {
                            if (state == STATE_USE_KEY)
                                keyItem->lockMatrix = false;
                            else
                                game->removeEntity(keyItem);
                        }

                        keyItem = NULL;
                        keyHole = NULL;
                    }
                }
                break;
            }
        }
    }

    virtual void update() {
        lastInput = input;
        input = getInput();

        updateV2();

        int32 oldState = state;

        TR::Animation *anim = animation.anims + animIndex;
        animation.setAnim(animIndex, frameIndex);

        state = animation.state;

        state = oldState;

        pos.x = float(v2pos.x);
        pos.y = float(v2pos.y);
        pos.z = float(v2pos.z);

        angle.x = PI * float(v2rot.x) / ANGLE_180;
        angle.y = PI * float(v2rot.y) / ANGLE_180;
        angle.z = PI * float(v2rot.z) / ANGLE_180;

        camera->update();
    }

/*
    virtual void update() {
        if (Input::state[camera->cameraIndex][cLook] && Input::lastState[camera->cameraIndex] == cAction)
            camera->changeView(!camera->firstPerson);

        if (level->isCutsceneLevel()) {
            updateAnimation(true);

            updateLights();

            if (fixRoomIndex()) {
                for (int i = 0; i < COUNT(braid); i++) {
                    if (braid[i]) {
                        braid[i]->update();
                    }
                }
            }
        } else {
            switch (usedItem) {
                case TR::Entity::INV_MEDIKIT_SMALL :
                case TR::Entity::INV_MEDIKIT_BIG   :
                    if (health < LARA_MAX_HEALTH) {
                        damageTime = LARA_DAMAGE_TIME;
                        health = min(LARA_MAX_HEALTH, health + (usedItem == TR::Entity::INV_MEDIKIT_SMALL ? LARA_MAX_HEALTH / 2 : LARA_MAX_HEALTH));
                        game->playSound(TR::SND_HEALTH, pos, Sound::PAN);
                        inventory->remove(usedItem);
                    }
                    usedItem = TR::Entity::NONE;
                default : ;
            }

            Character::update();
            for (int i = 0; i < COUNT(braid); i++) {
                if (braid[i]) {
                    braid[i]->update();
                }
            }
        }
        
        camera->update();

        if (hitTimer > 0.0f) {
            hitTimer -= Core::deltaTime;
            if (hitTimer > 0.0f)
                Input::setJoyVibration(camera->cameraIndex, 0.5f, 0.5f);
            else
                Input::setJoyVibration(camera->cameraIndex, 0, 0);
        }

        if (level->isCutsceneLevel())
            return;

        if (damageTime > 0.0f)
            damageTime = max(0.0f, damageTime - Core::deltaTime);

        usedItem = TR::Entity::NONE;

        if (camera->mode != Camera::MODE_CUTSCENE && camera->mode != Camera::MODE_STATIC) {
            camera->mode = (emptyHands() || health <= 0.0f) ? Camera::MODE_FOLLOW : Camera::MODE_COMBAT;
            if (input & LOOK)
                camera->mode = Camera::MODE_LOOK;
        }

        if (keyItem) {
            keyItem->flags.invisible = animation.frameIndex < (state == STATE_USE_KEY ? 70 : 30);
            keyItem->lockMatrix = true;
            mat4 &m = keyItem->matrix;
            Basis b;

            if (state == STATE_USE_KEY) {
                b = getJoint(10);
                b.rot = b.rot * quat(vec3(0, 1, 0), PI * 0.5f);
                b.translate(vec3(0, 120, 0));
            } else {
                vec3 rot(0.0f);
                
                // TODO: hardcode item-hand alignment 8)
                rot.x = -PI * 0.5f;

                if (animation.frameIndex < 55)
                    b = getJoint(13);
                else
                    b = getJoint(10);

                b.translate(vec3(0, 48, 0));

                if (rot.x != 0.0f) b.rot = b.rot * quat(vec3(1, 0, 0), rot.x);
                if (rot.y != 0.0f) b.rot = b.rot * quat(vec3(0, 1, 0), rot.y);
                if (rot.z != 0.0f) b.rot = b.rot * quat(vec3(0, 0, 1), rot.z);
            }

            keyItem->joints[0] = b;

            m.identity();
            m.setRot(b.rot);
            m.setPos(b.pos);
            //m = getMatrix();
        }
    }
*/
    virtual void updateAnimation(bool commands) {
    }

    virtual void updateVelocity() {
    }

    virtual void updatePosition() { // TODO: sphere / bbox collision
    }

    virtual vec3 getPos() {
        return level->isCutsceneLevel() ? getViewPoint() : pos;
    }
/*
    bool checkCollisions() {
    // check static objects (TODO: check linked rooms?)
        const TR::Room &room = getRoom();
        Box box(pos - vec3(LARA_RADIUS, LARA_HEIGHT, LARA_RADIUS), pos + vec3(LARA_RADIUS, 0.0f, LARA_RADIUS));

        for (int i = 0; i < room.meshesCount; i++) {
            TR::Room::Mesh &m  = room.meshes[i];
            TR::StaticMesh &sm = level->staticMeshes[m.meshIndex];
            if (sm.flags != 2) continue;
            Box meshBox;
            sm.getBox(true, m.rotation, meshBox);
            meshBox.translate(vec3(float(m.pos.x), float(m.pos.y), float(m.pos.z)));
            if (!box.intersect(meshBox)) continue;

            collisionOffset += meshBox.pushOut2D(box);
        }

    // check enemies & doors
        for (int i = 0; i < level->entitiesCount; i++) {
            const TR::Entity &e = level->entities[i];

            Controller *controller = (Controller*)e.controller;

            if (!controller || controller->flags.invisible || !controller->isCollider()) continue;

            if (e.type == TR::Entity::TRAP_DOOR_1 || e.type == TR::Entity::TRAP_DOOR_2) continue;

            if (e.isEnemy()) {
                if (e.type != TR::Entity::ENEMY_REX && (controller->flags.active != TR::ACTIVE || ((Character*)controller)->health <= 0)) continue;
            } else {
            // fast distance check for object
                if (e.type != TR::Entity::HAMMER_HANDLE && e.type != TR::Entity::HAMMER_BLOCK && e.type != TR::Entity::SCION_HOLDER)
                    if (fabsf(pos.x - controller->pos.x) > COLLIDE_MAX_RANGE || 
                        fabsf(pos.z - controller->pos.z) > COLLIDE_MAX_RANGE || 
                        fabsf(pos.y - controller->pos.y) > COLLIDE_MAX_RANGE) continue;
            }

            if (e.type == TR::Entity::TRAP_BOULDER && !controller->flags.unused) continue; // boulder should stay still

            vec3 dir = pos - vec3(0.0f, 128.0f, 0.0f) - controller->pos;
            vec3 p   = dir.rotateY(controller->angle.y);

            Box box = controller->getBoundingBoxLocal();
            box.expand(vec3(LARA_RADIUS + 50.0f, 0.0f, LARA_RADIUS + 50.0f));
            box.max.y += LARA_HEIGHT;

            if (!box.contains(p)) // TODO: Box vs Box or check Lara's head point? (check thor hammer handle)
                continue;

            if (!collide(controller, false))
                continue;

            if (e.isEnemy()) { // enemy collision
            //    velocity.x = velocity.y = 0.0f;
            } else {
                p += box.pushOut2D(p);
                p = (p.rotateY(-controller->angle.y) + controller->pos) - pos;
                collisionOffset += vec3(p.x, 0.0f, p.z);
            }

            if (e.type == TR::Entity::ENEMY_REX && ((Character*)controller)->health <= 0)
                return true;
            if (!e.isEnemy() || e.type == TR::Entity::ENEMY_BAT)
                return true;

            if (canHitAnim()) { // TODO: check enemy type and health here
            // get hit dir
                if (hitDir == -1) {
                    if (health > 0)
                        game->playSound(TR::SND_HIT, pos, Sound::PAN);
                    hitTime = 0.0f;
                }

                if (level->version & TR::VER_TR1) // TODO: check hit animation indices for TR2 and TR3
                    hitDir = angleQuadrant(dir.rotateY(angle.y + PI * 0.5f).angleY());
                return true;
            }
        };

        if (lightning && lightning->flash && !lightning->armed) {
            if (hitDir == -1)
                hitTime = 0.0f;
            hitDir = rand() % 4;
        } else {
            hitDir = -1;
            lightning = NULL;
        }
        return false;
    }
*/
    virtual void applyFlow(TR::Camera &sink) {
        if (stand != STAND_UNDERWATER && stand != STAND_ONWATER) return;

        vec3 target = vec3(float(sink.x), float(sink.y), float(sink.z));

    #ifdef _DEBUG
        //delete[] dbgBoxes;
        //dbgBoxes = NULL;
    #endif

        if (box != sink.flags.boxIndex) {
            uint16 *boxes;
            uint16 count = game->findPath(0xFFFFFF, -0xFFFFFF, false, box, sink.flags.boxIndex, getZones(), &boxes);
            if (count > 1) {
            #ifdef _DEBUG
                //dbgBoxesCount = count;
                //dbgBoxes = new uint16[dbgBoxesCount];
                //memcpy(dbgBoxes, boxes, sizeof(uint16) * dbgBoxesCount);
            #endif
                TR::Box &b = level->boxes[boxes[1]];
                target.x = (b.minX + b.maxX) * 0.5f;
                if (target.y > b.floor)
                    target.y = float(b.floor);
                target.z = (b.minZ + b.maxZ) * 0.5f;
            }
        }

        flowVelocity = vec3(0);
        if (!dozy) {
            float speed = sink.speed * 6.0f;
            flowVelocity.x = clamp(target.x - pos.x, -speed, +speed);
            flowVelocity.y = clamp(target.y - pos.y, -speed, +speed);
            flowVelocity.z = clamp(target.z - pos.z, -speed, +speed);

            if (stand == STAND_ONWATER)
                goUnderwater();
        }
    }

    uint32 getMidasMask() {
        if (state == STATE_MIDAS_USE) {
            return JOINT_MASK_ARM_L3 | JOINT_MASK_ARM_R3;
        }

        uint32 mask = 0;
        if (frameIndex > 4  ) mask |= JOINT_MASK_LEG_L3 | JOINT_MASK_LEG_R3;
        if (frameIndex > 69 ) mask |= JOINT_MASK_LEG_L2;
        if (frameIndex > 79 ) mask |= JOINT_MASK_LEG_L1;
        if (frameIndex > 99 ) mask |= JOINT_MASK_LEG_R2;
        if (frameIndex > 119) mask |= JOINT_MASK_LEG_R1 | JOINT_MASK_HIPS;
        if (frameIndex > 134) mask |= JOINT_MASK_CHEST;
        if (frameIndex > 149) mask |= JOINT_MASK_ARM_L1;
        if (frameIndex > 162) mask |= JOINT_MASK_ARM_L2;
        if (frameIndex > 173) mask |= JOINT_MASK_ARM_L3;
        if (frameIndex > 185) mask |= JOINT_MASK_ARM_R1;
        if (frameIndex > 194) mask |= JOINT_MASK_ARM_R2;
        if (frameIndex > 217) mask |= JOINT_MASK_ARM_R3;
        if (frameIndex > 224) mask |= JOINT_MASK_HEAD;
        return mask;
    }

    virtual void render(Frustum *frustum, MeshBuilder *mesh, Shader::Type type, bool caustics) {
        uint32 visMask = visibleMask;
        if (Core::pass != Core::passShadow && camera->firstPerson && camera->viewIndex == -1 && game->getCamera() == camera) // hide head in first person view // TODO: fix for firstPerson with viewIndex always == -1
            visibleMask &= ~JOINT_MASK_HEAD;
        Controller::render(frustum, mesh, type, caustics);

        if (level->extra.laraJoints > -1) {
            const TR::Model *model = getModel();
            for (int i = 0; i < model->mCount; i++) {
                joints[i].w = 1.0f;
            }
            Core::setBasis(joints, model->mCount);
            mesh->renderModel(level->extra.laraJoints, caustics); 
        }

        visibleMask = visMask;

        for (int i = 0; i < COUNT(braid); i++) {
            if (braid[i]) {
                braid[i]->render(mesh);
            }
        }

        if (state == STATE_MIDAS_DEATH || state == STATE_MIDAS_USE) {
            game->setRoomParams(getRoomIndex(), Shader::MIRROR, 1.2f, 1.0f, 0.2f, 1.0f, false);

            environment->bind(sEnvironment);
            visibleMask ^= 0xFFFFFFFF;
            Controller::render(frustum, mesh, type, caustics);
            visibleMask ^= 0xFFFFFFFF;
        }
    }

#if 1
    #define LARA_TURN_ACCEL         (2 * DEG2SHORT + DEG2SHORT / 4)
    #define LARA_TURN_JUMP          (3 * DEG2SHORT)
    #define LARA_TURN_VERY_SLOW     (2 * DEG2SHORT)
    #define LARA_TURN_SLOW          (4 * DEG2SHORT)
    #define LARA_TURN_MED           (6 * DEG2SHORT)
    #define LARA_TURN_FAST          (8 * DEG2SHORT)
    #define LARA_TILT_ACCEL         (DEG2SHORT + DEG2SHORT / 2)
    #define LARA_TILT_MAX           (11 * DEG2SHORT)
    #define LARA_STEP_HEIGHT        384
    #define LARA_SMASH_HEIGHT       640
    #define LARA_FLOAT_UP_SPEED     5
    #define LARA_SWIM_FRICTION      6
    #define LARA_SWIM_ACCEL         8
    #define LARA_SWIM_SPEED_MIN     133
    #define LARA_SWIM_SPEED_MAX     200
    #define LARA_SWIM_TIMER         10  // 1/3 sec
    #define LARA_SURF_FRICTION      4
    #define LARA_SURF_ACCEL         8
    #define LARA_SURF_SPEED_MAX     60
    #define LARA_DIVE_SPEED         80

    enum WaterState {
        WATER_STATE_ABOVE,
        WATER_STATE_SURFACE,
        WATER_STATE_UNDER,
        WATER_STATE_WADE,
    };

    WaterState waterState;
    bool       gravity;

    enum CollisionType {
        CT_NONE          = 0,
        CT_FRONT         = (1 << 0),
        CT_LEFT          = (1 << 1),
        CT_RIGHT         = (1 << 2),
        CT_CEILING       = (1 << 3),
        CT_FRONT_CEILING = (1 << 4),
        CT_FLOOR_CEILING = (1 << 5),
    };

    struct CollisionInfo {

        enum SideType {
            ST_MIDDLE,
            ST_FRONT,
            ST_LEFT,
            ST_RIGHT,
            ST_MAX
        };

        struct Side {
            int32         floor;
            int32         ceiling;
            TR::SlantType slantType;
        };

        int16 *trigger;

        Side m;
        Side f;
        Side l;
        Side r;

        int32 radius;

        int32 gapPos;
        int32 gapNeg;
        int32 gapCeiling;

        vec3i offset;
        vec3i pos;

        int16 angle;
        int16 quadrant;

        CollisionType type;

        int8 slantX;
        int8 slantZ;

        bool enemyPush;
        bool enemyHit;
        bool staticHit;
        bool stopOnSlant;
        bool stopOnLava;

        void setSide(SideType st, int32 floor, int32 ceiling, TR::SlantType slantType) {
            if (st != ST_MIDDLE) {
                if (stopOnSlant && floor < 0 && slantType == TR::SLANT_HIGH) {
                    floor -= 0x7FFF;
                } else if (stopOnSlant && floor > 0 && slantType == TR::SLANT_HIGH) {
                    floor = 512;
                } else if (stopOnLava && floor > 0 && trigger && TR::FloorData(*(uint16*)trigger).cmd.func == TR::FloorData::LAVA) {
                    floor = 512;
                }
            }

            Side *s = &m + st;
            s->slantType = slantType;
            s->floor     = floor;
            s->ceiling   = ceiling;
        }
    };

    CollisionInfo cinfo;

    int16 v2angle;
    vec3i v2pos;
    vec3s v2rot;
    vec3s v2torso;
    vec3s v2head;
    int32 v2floor;
    int32 swimTimer;

    int32 nextState;
    int32 targetState;
    int32 hSpeed;
    int32 vSpeed;   // vertical speed, but used as main speed while swim
    int32 vSpeedHack;
    int32 frameIndex;
    int32 animIndex;
    int16 turnSpeed;

    int32 getWaterLevel() {
        int16 waterRoomIndex = roomIndex;

        TR::Room::Sector *sector = level->getWaterLevelSector(waterRoomIndex, v2pos.x, v2pos.z);

        if (sector) {
            return sector->ceiling * 256;
        }

        return TR::NO_VALUE;
    }

    int32 getWaterDepth() {
        int16 waterRoomIndex = roomIndex;

        TR::Room::Sector *sector = level->getWaterLevelSector(waterRoomIndex, v2pos.x, v2pos.z);

        if (sector) {
            return level->getFloor(sector, v2pos.x, v2pos.y, v2pos.z) - (sector->ceiling * 256);
        }

        return TR::NO_VALUE;
    }

    void setAnimV2(int32 index, bool resetState, int32 frameOffset = 0) {
        TR::Animation *anim = level->anims + index;

        animIndex   = index;
        frameIndex  = anim->frameStart + frameOffset;

        if (resetState) {
            state       = anim->state;
            targetState = state;
        }
    }

    void dive() {
        setAnimV2(ANIM_SURF_DIVE, true);
        v2rot.x = -45 * DEG2SHORT;
        vSpeed = LARA_DIVE_SPEED;
        waterState = WATER_STATE_UNDER;
    }

    void updateRoomV2(int32 height) {
        int16 newRoomIndex = roomIndex;

        TR::Room::Sector *sector = level->getSector(newRoomIndex, v2pos.x, v2pos.y + height, v2pos.z);

        if (sector) {
            v2floor = level->getFloor(sector, v2pos.x, v2pos.y + height, v2pos.z);
            if (roomIndex != newRoomIndex) {
                roomIndex = newRoomIndex;
            }
        }
    }

    void moveV2(int16 x, int16 y, int16 z) {
        int32 s = phd_sin(v2rot.y);
        int32 c = phd_cos(v2rot.y);

        v2pos.x += (c * x + s * z) >> FIXED_SHIFT;
        v2pos.y += y;
        v2pos.z += (c * z - s * x) >> FIXED_SHIFT;
    }

    void playSoundV2(int16 cmd) {
        if (!(level->version & TR::VER_TR1)) {
            if ((cmd & 0x8000) && waterDepth <= 0.0f)
                return;
            if ((cmd & 0x4000) && waterDepth > 0.0f)
                return;
        }
        game->playSound(cmd & 0x3FFF, pos, Sound::PAN);
    }

    void applyEffectV2(int16 fx) {
        switch (fx) {
            case TR::Effect::ROTATE_180     : v2rot.y -= ANGLE_180; break;
            case TR::Effect::FLOOR_SHAKE    : ASSERT(false);
            case TR::Effect::LARA_NORMAL    : animation.setAnim(ANIM_STAND); break;
            case TR::Effect::LARA_BUBBLES   : doBubbles(); break;
            case TR::Effect::LARA_HANDSFREE : break;//meshSwap(1, level->extra.weapons[wpnCurrent], BODY_LEG_L1 | BODY_LEG_R1); break;
            case TR::Effect::DRAW_RIGHTGUN  : drawGun(true); break;
            case TR::Effect::DRAW_LEFTGUN   : drawGun(false); break;
            case TR::Effect::SHOT_RIGHTGUN  : game->addMuzzleFlash(this, LARA_RGUN_JOINT, LARA_RGUN_OFFSET, 1 + camera->cameraIndex); break;
            case TR::Effect::SHOT_LEFTGUN   : game->addMuzzleFlash(this, LARA_LGUN_JOINT, LARA_LGUN_OFFSET, 1 + camera->cameraIndex); break;
            case TR::Effect::MESH_SWAP_1    : 
            case TR::Effect::MESH_SWAP_2    : 
            case TR::Effect::MESH_SWAP_3    : Character::cmdEffect(fx);
            case 26 : break; // TODO TR2 reset_hair
            case 32 : break; // TODO TR3 footprint
            default : LOG("unknown effect command %d (anim %d)\n", fx, animation.index); ASSERT(false);
        }
    }

    void applyCommandsV2(bool effects) {
        TR::Animation *anim = level->anims + animIndex;

        int16 *ptr = &level->commands[anim->animCommand];

        for (int i = 0; i < anim->acCount; i++) {
            int cmd = *ptr++; 
            switch (cmd) {
                case TR::ANIM_CMD_OFFSET :
                    if (!effects) {
                        moveV2(ptr[0], ptr[1], ptr[2]);
                    }
                    ptr += 3;
                    break;
                case TR::ANIM_CMD_JUMP :
                    if (!effects) {
                        if (vSpeedHack) {
                            vSpeed = vSpeedHack;
                            vSpeedHack = 0;
                        } else {
                            vSpeed = ptr[0];
                        }
                        hSpeed = ptr[1];
                        gravity = true;
                    }
                    ptr += 2;
                    break;
                case TR::ANIM_CMD_EMPTY :
                    break;
                case TR::ANIM_CMD_KILL :
                    break;
                case TR::ANIM_CMD_SOUND :
                    if (effects && frameIndex == ptr[0]) {
                        playSoundV2(ptr[1]);
                    }
                    ptr += 2;
                    break;
                case TR::ANIM_CMD_EFFECT :
                    if (effects && frameIndex == ptr[0]) {
                        applyEffectV2(ptr[1]);
                    }
                    ptr += 2;
                    break;
            }
        }
    }

    void updateAnimationV2() {
        frameIndex++;

        const TR::Animation* anim = level->anims + animIndex;

        if (state == targetState || anim->scCount == 0) {
            return;
        }

        const TR::AnimState *states = level->states + anim->scOffset;

        for (int i = 0; i < anim->scCount; i++) {

            if (states[i].state != targetState) {
                continue;
            }
            
            const TR::AnimRange *range = level->ranges + states[i].rangesOffset;

            for (int j = 0; j < states[i].rangesCount; j++) {
                if (frameIndex >= range->low && frameIndex <= range->high) {
                    animIndex  = range->nextAnimation;
                    frameIndex = range->nextFrame;
                    state      = level->anims[animIndex].state;
                    return;
                }
                range++;
            }
        }
    }

    void processAnimation() {
        updateAnimationV2();

        const TR::Animation *anim = level->anims + animIndex;
        
        if (frameIndex > anim->frameEnd) {
            applyCommandsV2(false);
            animIndex = anim->nextAnimation;
            frameIndex = anim->nextFrame;
            anim = level->anims + animIndex;
            state = anim->state;
        }

        applyCommandsV2(true);

        int32 speed = anim->speed.value + anim->accel.value * (frameIndex - anim->frameStart);

        if (gravity) {
            hSpeed -= int16((speed - anim->accel.value) >> 16);
            hSpeed += int16(speed >> 16);
            vSpeed += vSpeed < 128 ? GRAVITY : 1;
            v2pos.y += vSpeed;
        } else {
            hSpeed = int16(speed >> 16);
        }

        v2pos.x += (phd_sin(v2angle) * hSpeed) >> FIXED_SHIFT;
        v2pos.z += (phd_cos(v2angle) * hSpeed) >> FIXED_SHIFT;
    }

    void updateV2() {
        TR::Room &room = getRoom();

        int32 waterLevel = getWaterLevel();
        int32 waterDist  = TR::NO_VALUE;

        if (waterLevel != TR::NO_VALUE) {
            waterDist = v2pos.y - waterLevel;
        }

    // change water state
        switch (waterState) {
            case WATER_STATE_ABOVE : {
                if (waterDist == TR::NO_VALUE || waterDist < LARA_WADE_MIN_DEPTH) {
                    break;
                }

                int32 waterDepth = getWaterDepth();
                if (waterDepth > LARA_WADE_MAX_DEPTH - 256) {
                    if (room.flags.water) { // go dive
                        waterState = WATER_STATE_UNDER;
                        gravity = false;
                        oxygen = LARA_MAX_OXYGEN;

                        v2pos.y += 100;
                        updateRoomV2(0);
                        stopScreaming();

                        if (state == STATE_SWAN_DIVE) {
                            v2rot.x = -45 * DEG2SHORT;
                            targetState = STATE_DIVE;
                            processAnimation();
                            vSpeed *= 2;
                            game->waterDrop(pos, 128.0f, 0.2f);
                        } else if (state == STATE_FAST_DIVE) {
                            v2rot.x = -85 * DEG2SHORT;
                            targetState = STATE_DIVE;
                            processAnimation();
                            vSpeed *= 2;
                            game->waterDrop(pos, 128.0f, 0.2f);
                        } else {
                            v2rot.x = -45 * DEG2SHORT;
                            setAnimV2(ANIM_WATER_FALL, true);
                            state       = STATE_DIVE; // TODO check necessary
                            targetState = STATE_SWIM;
                            vSpeed = vSpeed * 3 / 2;
                            game->waterDrop(pos, 256.0f, 0.2f);
                        }

                        v2head.x = v2head.y = 0;
                        v2torso.x = v2torso.y = 0;
                    
                        waterSplash();
                    }
                } else if (waterDist > LARA_WADE_MIN_DEPTH) {
                    waterState = WATER_STATE_WADE;
                    if (!gravity) {
                        targetState = STATE_STOP;
                    }
                }
                break;
            }
            case WATER_STATE_SURFACE : {
                if (!room.flags.water) {
                    if (waterDist > LARA_WADE_MIN_DEPTH) {
                        waterState = WATER_STATE_WADE;
                        setAnimV2(ANIM_STAND_NORMAL, true);
                        targetState = STATE_WADE;
                        processAnimation();
                    } else {
                        waterState = WATER_STATE_ABOVE;
                        setAnimV2(ANIM_FALL_FORTH, true);
                        hSpeed = vSpeed / 4;
                        gravity = true;
                    }

                    vSpeed = 0;
                    v2rot.x = v2rot.z = 0;
                    v2head.x = v2head.y = 0;
                    v2torso.x = v2torso.y = 0;
                }
                break;
            }
            case WATER_STATE_UNDER : {
                if (!room.flags.water && !dozy) {
                    if ((getWaterDepth() != TR::NO_VALUE) && abs(waterDist) < 256) {
                        waterState = WATER_STATE_SURFACE;
                        v2pos.y -= (waterDist - 1);
                        setAnimV2(ANIM_SURF, true);
                        vSpeed = 0;
                        swimTimer = LARA_SWIM_TIMER + 1; // block dive before we press jump button again
                        updateRoomV2(-LARA_HEIGHT / 2);
                        game->playSound(TR::SND_BREATH, pos, Sound::PAN | Sound::UNIQUE);
                    } else {
                        waterState = WATER_STATE_ABOVE;
                        setAnimV2(ANIM_FALL_FORTH, true);
                        hSpeed = vSpeed / 4;
                        vSpeed = 0;
                        gravity = true;
                    }

                    v2rot.x = v2rot.z = 0;
                    v2head.x = v2head.y = 0;
                    v2torso.x = v2torso.y = 0;
                }
                break;
            }
            case WATER_STATE_WADE : {
                if (waterDist < LARA_WADE_MIN_DEPTH) {
                    waterState = WATER_STATE_ABOVE;
                    if (state == STATE_WADE) {
                        targetState = STATE_RUN;
                    }
                } else if (waterDist > LARA_WADE_MAX_DEPTH) {
                    waterState = WATER_STATE_SURFACE;
                    v2pos.y -= (waterDist - 1);

                    if (state == STATE_BACK) {
                        setAnimV2(ANIM_SURF_BACK, true);
                    } else if (state == STATE_STEP_RIGHT) {
                        setAnimV2(ANIM_SURF_RIGHT, true);
                    } else if (state == STATE_STEP_LEFT) {
                        setAnimV2(ANIM_SURF_LEFT, true);
                    } else {
                        setAnimV2(ANIM_SURF_SWIM, true);
                    }

                    swimTimer = 0;
                    vSpeed = 0;
                    gravity = false;
                    v2rot.x = v2rot.z = 0;
                    v2head.x = v2head.y = 0;
                    v2torso.x = v2torso.y = 0;
                    updateRoomV2(0);
                }
                break;
            }
        }

        switch (waterState) {
            case WATER_STATE_ABOVE : 
            case WATER_STATE_WADE  : {
                oxygen = LARA_MAX_OXYGEN;
                updateAbove();
                break;
            }
            case WATER_STATE_SURFACE : {
                if (health >= 0.0f) {
                    oxygen = min(oxygen + 10, LARA_MAX_OXYGEN);
                }
                updateSurface();
                break;
            }
            case WATER_STATE_UNDER : {
                if (health >= 0.0f) {
                    oxygen = max(oxygen - 1, -1);
                    if (oxygen < 0) {
                        health -= 5;
                    }
                }
                updateUnder();
                break;
            }
        }

        if (state == STATE_MIDAS_DEATH || state == STATE_MIDAS_USE) {
            uint32 sparklesMask = getMidasMask();

            if (state == STATE_MIDAS_DEATH) {
                visibleMask = sparklesMask ^ 0xFFFFFFFF;
            }

            addSparks(sparklesMask);
        }

        if (waterState == WATER_STATE_UNDER) {
            specular = LARA_MIN_SPECULAR;
        } else if (specular > LARA_MIN_SPECULAR) {
            specular = max(LARA_MIN_SPECULAR, specular - LARA_WET_TIMER * (1.0f / 30.0f));
        }

        updateCamera();
    }

    void unroll(int16 &angle, int32 degrees) {
        if (angle < -degrees * DEG2SHORT) {
            angle += degrees * DEG2SHORT;
        } else if (angle > degrees * DEG2SHORT) {
            angle -= degrees * DEG2SHORT;
        } else {
            angle = 0;
        }
    }

    void unroll_smooth(int16 &angle, int32 degrees, int32 factor) {
        if (abs(angle) >= degrees * DEG2SHORT) {
            angle -= angle / factor;
        } else {
            angle = 0;
        }
    }

    void updateAbove() {
        cinfo.trigger     = NULL;
        cinfo.radius      = LARA_RADIUS;
        cinfo.pos         = v2pos;
        cinfo.enemyPush   = true;
        cinfo.enemyHit    = true;
        cinfo.stopOnSlant = false;
        cinfo.stopOnLava  = false;

        processState();

        if (camera->mode != Camera::MODE_LOOK) {
            unroll_smooth(v2head.x, 2, 8);
            unroll_smooth(v2head.y, 2, 8);
            v2torso = v2head;
        }

        unroll(v2rot.z, 1);
        unroll(turnSpeed, 2);
        v2rot.y += turnSpeed;

        processAnimation();

        processCollisions();

        updateRoomV2(-LARA_HEIGHT / 2);

        updateWeaponV2();

        checkTriggerV2(cinfo.trigger, false);
    }

    void updateSurface() {
        cinfo.trigger     = NULL;
        cinfo.radius      = LARA_RADIUS;
        cinfo.gapPos      = -TR::NO_VALUE;
        cinfo.gapNeg      = -128;
        cinfo.gapCeiling  = 100;
        cinfo.pos      = v2pos;
        cinfo.enemyPush   = false;
        cinfo.enemyHit    = false;
        cinfo.stopOnSlant = false;
        cinfo.stopOnLava  = false;

        processState();

        if (camera->mode != Camera::MODE_LOOK) {
            unroll_smooth(v2head.x, 3, 8);
            unroll_smooth(v2head.y, 3, 8);
            v2torso = v2head;
        }

        unroll(v2rot.z, 2);

        updateFlowV2();

        processAnimation();

        v2pos.x += (phd_sin(v2angle) * vSpeed) >> 16; // TODO WHY vSpeed?
        v2pos.z += (phd_cos(v2angle) * vSpeed) >> 16;

        processCollisions();

        updateRoomV2(100);

        updateWeaponV2();

        checkTriggerV2(cinfo.trigger, false);
    }

    void updateUnder() {
        cinfo.trigger     = NULL;
        cinfo.radius      = LARA_RADIUS_WATER;
        cinfo.gapPos      = -TR::NO_VALUE;
        cinfo.gapNeg      = -LARA_HEIGHT_UW;
        cinfo.gapCeiling  = LARA_HEIGHT_UW;
        cinfo.pos      = v2pos;
        cinfo.enemyPush   = false;
        cinfo.enemyHit    = false;
        cinfo.stopOnSlant = false;
        cinfo.stopOnLava  = false;

        processState();

        unroll(v2rot.z, 2);

        v2rot.x = clamp(int32(v2rot.x), -85 * DEG2SHORT, 85 * DEG2SHORT); // 100 for TR1?
        v2rot.z = clamp(int32(v2rot.z), -22 * DEG2SHORT, 22 * DEG2SHORT);

        unroll(turnSpeed, 2);
        v2rot.y += turnSpeed;

        updateFlowV2();
        
        processAnimation();

        int32 c = phd_cos(v2rot.x);

        v2pos.y -= (phd_sin(v2rot.x) * vSpeed) >> 16;
        v2pos.x += (((phd_sin(v2rot.y) * vSpeed) >> 16) * c) >> FIXED_SHIFT;
        v2pos.z += (((phd_cos(v2rot.y) * vSpeed) >> 16) * c) >> FIXED_SHIFT;
        
        processCollisions();

        updateRoomV2(0);

        updateWeaponV2();

        checkTriggerV2(cinfo.trigger, false);
    }

    void updateFlowV2() {
    }

    void updateWeaponV2() {
    }

    void updateEnemyCollisionV2() {
    }

    void checkTriggerV2(int16 *trigger, bool hard) {
    }

    void processCollisions() {
        updateEnemyCollisionV2();

        (this->*cLaraHandlers[state])();
    }

    void processState() {
        (this->*sLaraHandlers[state])();
    }

    int32 alignOffset(int32 a, int32 b) {
        int32 ca = a / 1024;
        int32 cb = b / 1024;

        if (ca == cb) {
            return 0;
        }

        a &= 1023;

        if (ca < cb) {
            return 1025 - a;
        }

        return -(a + 1);
    }

    bool collideStatic(CollisionInfo &cinfo, const vec3i &p, int32 height) {
        cinfo.staticHit = false;
        cinfo.offset    = vec3i(0);

        BoxV2 objBox(v2pos - vec3i(cinfo.radius, height, cinfo.radius), 
                     v2pos + vec3i(cinfo.radius, 0, cinfo.radius));

    // TODO: check linked rooms
        for (int j = 0; j < level->roomsCount; j++) {
            const TR::Room &room = level->rooms[j];

            for (int i = 0; i < room.meshesCount; i++) {
                TR::Room::Mesh &m  = room.meshes[i];
                TR::StaticMesh &sm = level->staticMeshes[m.meshIndex];

                if (sm.flags & 1) continue;

                BoxV2 meshBox = sm.getBox(true);
                meshBox.rotate90(m.rotation.value / ANGLE_90);
                meshBox.translate(m.pos);

                if (!objBox.intersect(meshBox)) continue;

                cinfo.offset = meshBox.pushOut2D(objBox);

                bool flip = (cinfo.quadrant > 1);

                if (cinfo.quadrant & 1) {
                    if (abs(cinfo.offset.z) > cinfo.radius) {
                        cinfo.offset.z = cinfo.pos.z - p.z;
                        cinfo.type = CT_FRONT;
                    } else if (cinfo.offset.z != 0) {
                        cinfo.offset.x = 0;
                        cinfo.type = ((cinfo.offset.z > 0) ^ flip) ? CT_RIGHT : CT_LEFT;
                    } else {
                        cinfo.offset = vec3i(0);
                    }
                } else {
                    if (abs(cinfo.offset.x) > cinfo.radius) {
                        cinfo.offset.x = cinfo.pos.x - p.x;
                        cinfo.type = CT_FRONT;
                    } else if (cinfo.offset.x != 0) {
                        cinfo.offset.z = 0;
                        cinfo.type = ((cinfo.offset.x > 0) ^ flip) ? CT_LEFT : CT_RIGHT;
                    } else {
                        cinfo.offset = vec3i(0);
                    }
                }

                cinfo.staticHit = true;

                return true;
            }
        }

        return false;
    }

    void collideRoom(int32 height, int32 yOffset = 0) {
        cinfo.type     = CT_NONE;
        cinfo.offset   = vec3i(0, 0, 0);

        vec3i p = v2pos;
        p.y += yOffset;

        int32 y = p.y - height;

        int32 cy = y - 160;

        TR::Room::Sector *sector;
        int32 floor, ceiling;
        int16 slant;
        int16 *trigger;
        TR::SlantType slantType;

        #define CHECK_HEIGHT(v) {\
            int16 dummyRoom = roomIndex;\
            sector = level->getSector(dummyRoom, v.x, cy, v.z);\
            floor = level->getFloor(sector, v.x, cy, v.z, &slantType, &slant, &trigger);\
            if (floor != TR::NO_VALUE) floor -= p.y;\
            ceiling = level->getCeiling(sector, v.x, cy, v.z);\
            if (ceiling != TR::NO_VALUE) ceiling -= y;\
        }

    // middle
        CHECK_HEIGHT(p);

        cinfo.trigger = trigger;
        {
            TR::FloorData fd;
            fd.value = slant;
            cinfo.slantX = fd.slantX;
            cinfo.slantZ = fd.slantZ;
        }

        cinfo.setSide(CollisionInfo::ST_MIDDLE, floor, ceiling, slantType);

        vec3i f, l, r;
        int32 R = cinfo.radius;

        switch (cinfo.quadrant) {
            case 0 : {
                f = vec3i((R * phd_sin(cinfo.angle)) >> FIXED_SHIFT, 0, R);
                l = vec3i(-R, 0,  R);
                r = vec3i( R, 0,  R);
                break;
            }
            case 1 : {
                f = vec3i( R, 0, (R * phd_cos(cinfo.angle)) >> FIXED_SHIFT);
                l = vec3i( R, 0,  R);
                r = vec3i( R, 0, -R);
                break;
            }
            case 2 : {
                f = vec3i((R * phd_sin(cinfo.angle)) >> FIXED_SHIFT, 0, -R);
                l = vec3i( R, 0, -R);
                r = vec3i(-R, 0, -R);
                break;
            }
            case 3 : {
                f = vec3i(-R, 0, (R * phd_cos(cinfo.angle)) >> FIXED_SHIFT);
                l = vec3i(-R, 0, -R);
                r = vec3i(-R, 0,  R);
                break;
            }
        }

        f = f + p;
        l = l + p;
        r = r + p;

        vec3i delta = cinfo.pos - p;

    // front
        CHECK_HEIGHT(f);
        cinfo.setSide(CollisionInfo::ST_FRONT, floor, ceiling, slantType);

    // left
        CHECK_HEIGHT(l);
        cinfo.setSide(CollisionInfo::ST_LEFT, floor, ceiling, slantType);

    // right
        CHECK_HEIGHT(r);
        cinfo.setSide(CollisionInfo::ST_RIGHT, floor, ceiling, slantType);

    // static objects
        collideStatic(cinfo, p, height);

    // check middle
        if (cinfo.m.floor == TR::NO_VALUE) {
            cinfo.offset = delta;
            cinfo.type   = CT_FRONT;
            return;
        }

        if (cinfo.m.floor <= cinfo.m.ceiling) {
            cinfo.offset = delta;
            cinfo.type   = CT_FLOOR_CEILING;
            return;
        }

        if (cinfo.m.ceiling >= 0) {
            cinfo.offset.y = cinfo.m.ceiling;
            cinfo.type     = CT_CEILING;
        }

    // front
        if (cinfo.f.floor > cinfo.gapPos || cinfo.f.floor < cinfo.gapNeg || cinfo.f.ceiling > cinfo.gapCeiling) {
            if (cinfo.quadrant & 1) {
                cinfo.offset.x = alignOffset(f.x, p.x);
                cinfo.offset.z = delta.z;
            } else {
                cinfo.offset.x = delta.x;
                cinfo.offset.z = alignOffset(f.z, p.z);
            }
            cinfo.type = CT_FRONT;
            return;
        }

    // front ceiling
        if (cinfo.f.ceiling >= cinfo.gapCeiling) {
            cinfo.offset = delta;
            cinfo.type   = CT_FRONT_CEILING;
            return;
        }

    // left
        if (cinfo.l.floor > cinfo.gapPos || cinfo.l.floor < cinfo.gapNeg) {
            if (cinfo.quadrant & 1) {
                cinfo.offset.z = alignOffset(l.z, f.z);
            } else {
                cinfo.offset.x = alignOffset(l.x, f.x);
            }
            cinfo.type = CT_LEFT;
            return;
        }

    // right
        if (cinfo.r.floor > cinfo.gapPos || cinfo.r.floor < cinfo.gapNeg) {
            if (cinfo.quadrant & 1) {
                cinfo.offset.z = alignOffset(r.z, f.z);
            } else {
                cinfo.offset.x = alignOffset(r.x, f.x);
            }
            cinfo.type = CT_RIGHT;
            return;
        }
    }

    void c_applyOffset() {
        v2pos = v2pos + cinfo.offset;
        cinfo.offset = vec3i(0, 0, 0);
    }

    int32 getFrames(const TR::AnimFrame *&frameA, const TR::AnimFrame *&frameB, int32 &frameRate) {
        TR::Animation *anim = level->anims + animIndex;

        frameRate = anim->frameRate;

        int32 animFrame = frameIndex - anim->frameStart;
        int32 keyframe = animFrame / frameRate;

        frameA = animation.getFrame(anim, keyframe);
        frameB = animation.getFrame(anim, keyframe + 1);

        int32 mod = animFrame % frameRate;
        if (mod == 0) {
            return 0;
        }

        int32 next = (keyframe + 1) * frameRate;
        if (next > anim->frameEnd) {
            frameRate = anim->frameEnd - next + frameRate;
        }

        return mod;
    }

    TR::MinMax getBounds() {
        const TR::AnimFrame *frameA, *frameB;
        int32 frameRate;

        int32 mod = getFrames(frameA, frameB, frameRate);

        if (mod == 0) {
            return frameA->box;
        }

        return frameA->box.lerp(frameB->box, mod, frameRate);
    }

    bool checkDeath() {
        return health < 0; 
    }

    void getHeight(int16 angle, int32 radius, int16 *floor, int16 *ceiling, TR::SlantType *slantType) {
        angle += v2rot.y;

        vec3i p = v2pos;
        p.x += (radius * phd_sin(angle)) >> FIXED_SHIFT;
        p.z += (radius * phd_cos(angle)) >> FIXED_SHIFT;
        p.y -= LARA_HEIGHT;

        int16 frontRoomIndex = roomIndex;
        TR::Room::Sector *sector = level->getSector(frontRoomIndex, p.x, p.y, p.z);

        if (slantType) {
            *slantType = TR::SLANT_NONE;
        }

        if (sector) {
            if (floor && (*floor = level->getFloor(sector, p.x, p.y, p.z, slantType)) != TR::NO_VALUE) {
                *floor -= v2pos.y;
            }

            if (ceiling && (*ceiling = level->getCeiling(sector, p.x, p.y, p.z)) != TR::NO_VALUE) {
                *ceiling -= p.y;
            }
        } else {
            if (floor) {
                *floor = TR::NO_VALUE;
            }

            if (ceiling) {
                *ceiling = TR::NO_VALUE;
            }
        }
    }

    int16 getFloor(int32 angle, int16 radius) {
        int16 floor;
        getHeight(angle, radius, &floor, NULL, NULL);
        return floor;
    }

    int16 getCeiling(int32 angle, int16 radius) {
        int16 ceiling;
        getHeight(angle, radius, NULL, &ceiling, NULL);
        return ceiling;
    }
    
// state handlers
    void s_ignoreEnemy() { // should be called from state process, before collision check with enemies
        cinfo.enemyPush = false;
        cinfo.enemyHit  = false;
    }

    void s_rotate(int32 maxSpeed, int32 tilt = 0) {
        tilt *= LARA_TILT_ACCEL;

        if (input & LEFT) {
            turnSpeed = max(turnSpeed - LARA_TURN_ACCEL, -maxSpeed);
            v2rot.z = max(v2rot.z - tilt, -LARA_TILT_MAX);
        } else if (input & RIGHT) {
            turnSpeed = min(turnSpeed + LARA_TURN_ACCEL, maxSpeed);
            v2rot.z = min(v2rot.z + tilt, LARA_TILT_MAX);
        }
    }

    bool s_checkFall() {
        if (vSpeed > 131) {
            if (state == STATE_SWAN_DIVE) {
                targetState = STATE_FAST_DIVE;
            } else {
                targetState = STATE_FALL;
            }
            return true;
        }
        return false;
    }

    void s_checkWalk(int32 stopState = STATE_STOP) {
        if ((input & FORTH) && s_checkFront(ANGLE_0)) {
            if (waterState == WATER_STATE_WADE) {
                targetState = STATE_WADE;
            } else {
                if (input & WALK) {
                    targetState = STATE_WALK;
                } else {
                    targetState = STATE_RUN;
                }
            }
        } else {
            targetState = stopState;
        }
    }

    bool s_checkRoll() {
        if ((waterState != WATER_STATE_ABOVE) && (waterState != WATER_STATE_UNDER)) {
            return false;
        }

        bool roll = (input & (FORTH | BACK)) == (FORTH | BACK);

        if ((waterState == WATER_STATE_ABOVE) && roll) {
            if ((state == STATE_RUN) || (state == STATE_STOP)) {
                setAnimV2(ANIM_STAND_ROLL_BEGIN, true, 2);
                targetState = STATE_STOP;
                return true;
            }
        }

        if (level->version & TR::VER_TR1) {
            return false;
        }

        if ((waterState == WATER_STATE_UNDER) && roll) {
            setAnimV2(ANIM_UW_ROLL, true);
            targetState = STATE_SWIM;
            return true;
        }

        if (waterState == WATER_STATE_ABOVE) {
            if (targetState != state) {
                return false;
            }

            if (((state == STATE_FAST_DIVE)    && roll) ||
                ((state == STATE_FORWARD_JUMP) && (input & BACK)) ||
                ((state == STATE_BACK_JUMP)    && (input & FORTH)))
            {
                targetState = STATE_AIR_ROLL;
                return true;
            }
        }

        return false;
    }

    void s_turnUW() {
        if (input & FORTH) {
            v2rot.x -= 2 * DEG2SHORT;
        } else if (input & BACK) {
            v2rot.x += 2 * DEG2SHORT;
        }

        if (input & LEFT) {
            turnSpeed = max(turnSpeed - LARA_TURN_ACCEL, -LARA_TURN_MED);
            v2rot.z -= LARA_TILT_ACCEL * 2;
        } else if (input & RIGHT) {
            turnSpeed = min(turnSpeed + LARA_TURN_ACCEL,  LARA_TURN_MED);
            v2rot.z += LARA_TILT_ACCEL * 2;
        }
    }

    bool s_checkFront(int16 angle) {
        CollisionInfo tmp = cinfo;
        int16 tmpAngle = v2angle;

        c_angle(angle);
        cinfo.radius = LARA_RADIUS + 4;

        cinfo.gapPos      = -TR::NO_VALUE;
        cinfo.gapNeg      = -LARA_STEP_HEIGHT;
        cinfo.stopOnSlant = true;
        cinfo.gapCeiling  = 0;

        if ((angle == -ANGLE_180) && ((input & WALK) || (waterState == WATER_STATE_WADE))) {
            cinfo.gapPos = LARA_STEP_HEIGHT;
        }

        collideRoom(LARA_HEIGHT, 0);

        bool collide = (cinfo.type == CT_FRONT) || (cinfo.type == CT_FRONT_CEILING);

        cinfo = tmp;
        v2angle = tmpAngle;

        return !collide;
    }

    S_HANDLER(STATE_WALK) {
        if (checkDeath()) {
            targetState = STATE_STOP;
            return;
        }

        s_rotate(LARA_TURN_SLOW);

        s_checkWalk();
    }

    S_HANDLER(STATE_RUN) {
        if (checkDeath()) {
            targetState = STATE_DEATH;
            return;
        }

        if (s_checkRoll()) {
            return;
        }

        s_rotate(LARA_TURN_FAST, 1);
        
        if (state == STATE_RUN) {
            if (animIndex == ANIM_RUN_START) {
                canJump = false;
            } else if (animIndex == ANIM_RUN) {
                if (frameIndex == 2) {
                    canJump = true;
                }
            } else {
                canJump = true;
            }
        }

        if ((input & JUMP) && canJump && !gravity) {
            targetState = STATE_FORWARD_JUMP;
        } else {
            s_checkWalk();
        }
    }
    
    S_HANDLER(STATE_STOP) {
        targetState = STATE_STOP;

        if (checkDeath()) {
            targetState = STATE_DEATH;
            return;
        }

        if (s_checkRoll()) {
            return;
        }

        if ((input & (FORTH | ACTION)) == (FORTH | ACTION)) {
            c_angle(ANGLE_0);
            cinfo.radius = LARA_RADIUS + 4;
            c_default();
            cinfo.radius = LARA_RADIUS;
            if (c_checkClimbUp()) {
                return;
            }
        }

        if (input & WALK) {
            if ((input & LEFT) && s_checkFront(-ANGLE_90)) {
                targetState = STATE_STEP_LEFT;
            } else if ((input & RIGHT) && s_checkFront(ANGLE_90)) {
                targetState = STATE_STEP_RIGHT;
            }
        } else {
            if (input & LEFT) {
                targetState = STATE_TURN_LEFT;
            } else if (input & RIGHT) {
                targetState = STATE_TURN_RIGHT;
            }
        }

        if (waterState == WATER_STATE_WADE) {

            if (input & JUMP) {
                targetState = STATE_COMPRESS;
            } else if ((input & FORTH) && s_checkFront(ANGLE_0)) {
                if (input & WALK) {
                    s_STATE_WADE();
                } else {
                    s_STATE_WALK();
                }
            } else if ((input & BACK) && s_checkFront(-ANGLE_180)) {
                s_STATE_BACK();
            }

        } else {

            if (input & JUMP) {
                targetState = STATE_COMPRESS;
            } else if ((input & FORTH) && s_checkFront(ANGLE_0)) {
                if (input & WALK) {
                    s_STATE_WALK();
                } else {
                    s_STATE_RUN();
                }
            } else if ((input & BACK) && s_checkFront(-ANGLE_180)) {
                if (input & WALK) {
                    s_STATE_BACK();
                } else {
                    targetState = STATE_FAST_BACK;
                }
            }

        }
    }

    S_HANDLER(STATE_FORWARD_JUMP) {
        if (targetState == STATE_SWAN_DIVE || targetState == STATE_REACH) {
            targetState = STATE_FORWARD_JUMP;
        }
    
        if (targetState != STATE_DEATH && targetState != STATE_STOP && targetState != STATE_RUN) {
            if (input & ACTION) {
                targetState = STATE_REACH;
            }
            if (input & WALK) {
                targetState = STATE_SWAN_DIVE;
            }
            s_checkRoll();
            s_checkFall();
        }

        s_rotate(LARA_TURN_JUMP);
    }

    S_HANDLER(STATE_POSE) {
        // empty
    }

    S_HANDLER(STATE_FAST_BACK) {
        s_rotate(LARA_TURN_MED);
        targetState = STATE_STOP;
    }
    
    S_HANDLER(STATE_TURN_RIGHT) {
        if (checkDeath() || (input & LOOK)) {
            targetState = STATE_STOP;
            return;
        }

        turnSpeed += LARA_TURN_ACCEL;

        if (turnSpeed > LARA_TURN_SLOW) {
            targetState = STATE_FAST_TURN;
        }

        s_checkWalk((input & RIGHT) ? targetState : STATE_STOP);
    }

    S_HANDLER(STATE_TURN_LEFT) {
        if (checkDeath() || (input & LOOK)) {
            targetState = STATE_STOP;
            return;
        }

        turnSpeed -= LARA_TURN_ACCEL;

        if (turnSpeed < -LARA_TURN_SLOW) {
            targetState = STATE_FAST_TURN;
        }

        s_checkWalk((input & LEFT) ? targetState : STATE_STOP);
    }

    S_HANDLER(STATE_DEATH) {
        s_ignoreEnemy();
    }

    S_HANDLER(STATE_FALL) {
        hSpeed = (hSpeed * 95) / 100;
        if (vSpeed >= 154) {
            startScreaming();
        }
    }

    S_HANDLER(STATE_HANG) {
        s_ignoreEnemy();
        if (input & LEFT) {
            targetState = STATE_HANG_LEFT;
        } else if (input & RIGHT) {
            targetState = STATE_HANG_RIGHT;
        }
    }

    S_HANDLER(STATE_REACH) {
        s_checkFall();
    }

    S_HANDLER(STATE_SPLAT) {
        // empty
    }

    S_HANDLER(STATE_TREAD) {
        if (checkDeath()) {
            targetState = STATE_UW_DEATH;
            return;
        }

        if (s_checkRoll()) {
            return;
        }

        s_turnUW();

        if (input & JUMP) {
            targetState = STATE_SWIM;
        }

        vSpeed = max(vSpeed - LARA_SWIM_FRICTION, 0);
    }
    
    S_HANDLER(STATE_LAND) {
        // empty
    }

    S_HANDLER(STATE_COMPRESS) {
        if (waterState == WATER_STATE_WADE) {
            // up jump
        } else {
            if ((input & FORTH) && getFloor(0, 256) >= -LARA_STEP_HEIGHT) {
                targetState = STATE_FORWARD_JUMP;
            } else if ((input & LEFT) && getFloor(-ANGLE_90, 256) >= -LARA_STEP_HEIGHT) {
                targetState = STATE_LEFT_JUMP;
            } else if ((input & RIGHT) && getFloor(ANGLE_90, 256) >= -LARA_STEP_HEIGHT) {
                targetState = STATE_RIGHT_JUMP;
            } else if ((input & BACK) && getFloor(-ANGLE_180, 256) >= -LARA_STEP_HEIGHT) {
                targetState = STATE_BACK_JUMP;
            }
        }
        s_checkFall();
    }

    S_HANDLER(STATE_BACK) {
        if (checkDeath()) {
            targetState = STATE_STOP;
            return;
        }

        if ((input & BACK) && ((input & WALK) || (waterState == WATER_STATE_WADE))) {
            targetState = STATE_BACK;
        } else {
            targetState = STATE_STOP;
        }

        s_rotate(LARA_TURN_SLOW);
    }

    S_HANDLER(STATE_SWIM) {
        if (checkDeath()) {
            targetState = STATE_UW_DEATH;
            return;
        }

        if (s_checkRoll()) {
            return;
        }

        s_turnUW();

        vSpeed = min(vSpeed + LARA_SWIM_ACCEL, LARA_SWIM_SPEED_MAX);

        if (!(input & JUMP)) {
            targetState = STATE_GLIDE;
        }
    }

    S_HANDLER(STATE_GLIDE) {
        if (checkDeath()) {
            targetState = STATE_UW_DEATH;
            return;
        }

        if (s_checkRoll()) {
            return;
        }

        s_turnUW();

        if (input & JUMP) {
            targetState = STATE_SWIM;
        }

        vSpeed = max(vSpeed - LARA_SWIM_FRICTION, 0);

        if (vSpeed <= LARA_SWIM_SPEED_MIN) {
            targetState = STATE_TREAD;
        }
    }

    S_HANDLER(STATE_HANG_UP) {
        s_ignoreEnemy();
    }

    S_HANDLER(STATE_FAST_TURN) {
        if (checkDeath() || (input & LOOK)) {
            targetState = STATE_STOP;
            return;
        }

        if (turnSpeed < 0) {
            turnSpeed = -LARA_TURN_FAST;
            if (!(input & LEFT)) {
                targetState = STATE_STOP;
            }
        } else {
            turnSpeed = LARA_TURN_FAST;
            if (!(input & RIGHT)) {
                targetState = STATE_STOP;
            }
        }
    }

    S_HANDLER(STATE_STEP_RIGHT) {
        if (checkDeath() || (input & (WALK | RIGHT)) != (WALK | RIGHT)) {
            targetState = STATE_STOP;
        }
    }

    S_HANDLER(STATE_STEP_LEFT) {
        if (checkDeath() || (input & (WALK | LEFT)) != (WALK | LEFT)) {
            targetState = STATE_STOP;
        }
    }

    S_HANDLER(STATE_ROLL_END) {
        // empty
    }

    S_HANDLER(STATE_SLIDE) {
        if (input & JUMP) {
            targetState = STATE_FORWARD_JUMP;
        }
    }

    S_HANDLER(STATE_BACK_JUMP) {
        if (s_checkFall()) {
            return;
        }

        if (targetState == STATE_RUN) {
            targetState = STATE_STOP;
        } else {
            s_checkRoll();
        }
    }

    S_HANDLER(STATE_RIGHT_JUMP) {
        if (s_checkFall()) {
            return;
        }
        s_checkRoll();
    }

    S_HANDLER(STATE_LEFT_JUMP) {
        if (s_checkFall()) {
            return;
        }
        s_checkRoll();
    }

    S_HANDLER(STATE_UP_JUMP) {
        s_checkFall();
    }

    S_HANDLER(STATE_FALL_BACK) {
        s_checkFall();
        if (input & ACTION) {
            targetState = STATE_REACH;
        }
    }

    S_HANDLER(STATE_HANG_LEFT) {
        s_ignoreEnemy();
        if (!(input & LEFT)) {
            targetState = STATE_HANG;
        }
    }

    S_HANDLER(STATE_HANG_RIGHT) {
        s_ignoreEnemy();
        if (!(input & RIGHT)) {
            targetState = STATE_HANG;
        }
    }

    S_HANDLER(STATE_SLIDE_BACK) {
        if (input & JUMP) {
            targetState = STATE_BACK_JUMP;
        }
    }

    S_HANDLER(STATE_SURF_TREAD) {
        vSpeed = max(vSpeed - LARA_SURF_FRICTION, 0);

        if (input & LEFT) {
            v2rot.y -= LARA_TURN_SLOW;
        } else if (input & RIGHT) {
            v2rot.y += LARA_TURN_SLOW;
        }

        if (input & FORTH) {
            targetState = STATE_SURF_SWIM;
        } else if (input & BACK) {
            targetState = STATE_SURF_BACK;
        } else if ((input & (WALK | LEFT)) == (WALK | LEFT)) {
            targetState = STATE_SURF_LEFT;
        } else if ((input & (WALK | RIGHT)) == (WALK | RIGHT)) {
            targetState = STATE_SURF_RIGHT;
        }

        if (input & JUMP) {
            swimTimer++;
            if (swimTimer == LARA_SWIM_TIMER) {
                dive();
            }
        } else {
            swimTimer = 0;
        }
    }

    S_HANDLER(STATE_SURF_SWIM) {
        if (checkDeath()) {
            targetState = STATE_UW_DEATH;
            return;
        }

        swimTimer = 0;

        if (input & LEFT) {
            v2rot.y -= LARA_TURN_SLOW;
        } else if (input & RIGHT) {
            v2rot.y += LARA_TURN_SLOW;
        }

        if (!(input & FORTH) || (input & JUMP)) {
            targetState = STATE_SURF_TREAD;
        }

        vSpeed = min(vSpeed + LARA_SURF_ACCEL, LARA_SURF_SPEED_MAX);
    }
    
    S_HANDLER(STATE_DIVE) {
        if (input & FORTH) {
            v2rot.x -= ANGLE_1;
        }
    }

    S_HANDLER(STATE_PUSH_BLOCK) {
        s_ignoreEnemy();
    }

    S_HANDLER(STATE_PULL_BLOCK) {
        s_ignoreEnemy();
    }

    S_HANDLER(STATE_PUSH_PULL_READY) {
        s_ignoreEnemy();
        if (!(input & ACTION)) {
            targetState = STATE_STOP;
        }
    }

    S_HANDLER(STATE_PICK_UP) {
        s_ignoreEnemy();
    }

    S_HANDLER(STATE_SWITCH_DOWN) {
        s_ignoreEnemy();
    }

    S_HANDLER(STATE_SWITCH_UP) {
        s_ignoreEnemy();
    }

    S_HANDLER(STATE_USE_KEY) {
        s_ignoreEnemy();
    }

    S_HANDLER(STATE_USE_PUZZLE) {
        s_ignoreEnemy();
    }

    S_HANDLER(STATE_UW_DEATH) {
        vSpeed = max(vSpeed - LARA_SWIM_ACCEL, 0);
        unroll(v2rot.x, 2);
    }

    S_HANDLER(STATE_ROLL_START) {
        // empty
    }

    S_HANDLER(STATE_SPECIAL) {
        // empty
    }

    S_HANDLER(STATE_SURF_BACK) {
        if (checkDeath()) {
            targetState = STATE_UW_DEATH;
            return;
        }

        swimTimer = 0;

        if (input & LEFT) {
            v2rot.y -= LARA_TURN_VERY_SLOW;
        } else if (input & RIGHT) {
            v2rot.y += LARA_TURN_VERY_SLOW;
        }

        if (!(input & BACK)) {
            targetState = STATE_SURF_TREAD;
        }

        vSpeed = min(vSpeed + LARA_SURF_ACCEL, LARA_SURF_SPEED_MAX);
    }

    S_HANDLER(STATE_SURF_LEFT) {
        if (checkDeath()) {
            targetState = STATE_UW_DEATH;
            return;
        }

        swimTimer = 0;

        if ((input & (WALK | LEFT)) != (WALK | LEFT)) {
            targetState = STATE_SURF_TREAD;
        }

        vSpeed = min(vSpeed + LARA_SURF_ACCEL, LARA_SURF_SPEED_MAX);
    }

    S_HANDLER(STATE_SURF_RIGHT) {
        if (checkDeath()) {
            targetState = STATE_UW_DEATH;
            return;
        }

        swimTimer = 0;

        if ((input & (WALK | RIGHT)) != (WALK | RIGHT)) {
            targetState = STATE_SURF_TREAD;
        }

        vSpeed = min(vSpeed + LARA_SURF_ACCEL, LARA_SURF_SPEED_MAX);
    }

    S_HANDLER(STATE_MIDAS_USE) {
        s_ignoreEnemy();
    }

    S_HANDLER(STATE_MIDAS_DEATH) {
        s_ignoreEnemy();
        gravity = false;
    }

    S_HANDLER(STATE_SWAN_DIVE) {
        cinfo.enemyPush = true;
        cinfo.enemyHit  = false;
        s_checkFall();
    }

    S_HANDLER(STATE_FAST_DIVE) {
        cinfo.enemyPush = true;
        cinfo.enemyHit  = false;
        hSpeed = (hSpeed * 95) / 100;
        s_checkRoll();
    }

    S_HANDLER(STATE_HANDSTAND) {
        s_ignoreEnemy();
    }

    S_HANDLER(STATE_WATER_OUT) {
        s_ignoreEnemy();
    }

    S_HANDLER(STATE_CLIMB_START) {
        s_ignoreEnemy();
        if (input & LEFT) {
            targetState = STATE_CLIMB_LEFT;
        } else if (input & RIGHT) {
            targetState = STATE_CLIMB_RIGHT;
        } else if (input & JUMP) {
            targetState = STATE_BACK_JUMP;
            c_angle(-ANGLE_180);
        }
    }

    S_HANDLER(STATE_CLIMB_UP) {
        s_ignoreEnemy();
    }

    S_HANDLER(STATE_CLIMB_LEFT) {
        s_ignoreEnemy();

        if (!(input & LEFT)) {
            targetState = STATE_CLIMB_START;
        }
    }

    S_HANDLER(STATE_CLIMB_END) {
        s_ignoreEnemy();
    }

    S_HANDLER(STATE_CLIMB_RIGHT) {
        s_ignoreEnemy();

        if (!(input & LEFT)) {
            targetState = STATE_CLIMB_START;
        }
    }

    S_HANDLER(STATE_CLIMB_DOWN) {
        s_ignoreEnemy();
    }

    S_HANDLER(STATE_UNUSED_1) {
        // empty
    }

    S_HANDLER(STATE_UNUSED_2) {
        // empty
    }

    S_HANDLER(STATE_UNUSED_3) {
        // empty
    }

    S_HANDLER(STATE_WADE) {
        if (checkDeath()) {
            targetState = STATE_STOP;
            return;
        }

        s_rotate(LARA_TURN_FAST, 1);
        
        if (input & FORTH) {
            targetState = (waterState == WATER_STATE_ABOVE) ? STATE_RUN : STATE_WADE;
        } else {
            targetState = STATE_STOP;
        }
    }

    S_HANDLER(STATE_UW_ROLL) {
        vSpeed = 0;
    }
    
    S_HANDLER(STATE_PICKUP_FLARE) {}

    S_HANDLER(STATE_AIR_ROLL) {
        // empty
    }

    S_HANDLER(STATE_UNUSED_5) {
        // empty
    }

    S_HANDLER(STATE_ZIPLINE) {
        // TODO checkTriggerV2();

        if (!(input & ACTION)) {
            targetState = STATE_FORWARD_JUMP;
            hSpeed  = 100;
            vSpeed  = 40;
            gravity = true;
        }
    }

    S_HANDLER(STATE_DUCK) {}
    S_HANDLER(STATE_DUCK_ROLL) {}
    S_HANDLER(STATE_DASH) {}
    S_HANDLER(STATE_DASH_DIVE) {}
    S_HANDLER(STATE_MONKEYSWING_IDLE) {}
    S_HANDLER(STATE_MONKEYSWING) {}


// collision handlers
    bool alignAngle(int16 &angle, int16 threshold) {
        if (angle >= -threshold && angle <= threshold) {
            angle = 0;
        } else if (angle >= ANGLE_90 - threshold && angle <= ANGLE_90 + threshold) {
            angle = ANGLE_90;
        } else if (angle >= -ANGLE_90 - threshold && angle <= -ANGLE_90 + threshold) {
            angle = -ANGLE_90;
        } else if (angle >= (ANGLE_180 - 1) - threshold || angle <= -(ANGLE_180 - 1) + threshold) {
            angle = -ANGLE_180;
        }

        return (angle & (ANGLE_90 - 1)) > 0;
    }

    void alignWall() {
        int x = (~1023) & v2pos.x;
        int z = (~1023) & v2pos.z;

        switch (v2rot.y) {
            case  ANGLE_0   : v2pos.z = z + 1024 + LARA_RADIUS; break;
            case  ANGLE_90  : v2pos.x = x + 1024 + LARA_RADIUS; break;
            case -ANGLE_90  : v2pos.x = x - LARA_RADIUS; break;
            case -ANGLE_180 : v2pos.z = z - LARA_RADIUS; break;
            default         : ASSERT(false);
        }
    }

    void c_angle(int32 angleDelta) {
        v2angle = v2rot.y + angleDelta;

        cinfo.angle    = v2angle;
        cinfo.quadrant = uint16(cinfo.angle + ANGLE_45) / ANGLE_90;
    }

    bool c_checkCeiling() {
        if (cinfo.type != CT_CEILING && cinfo.type != CT_FLOOR_CEILING) {
            return false;
        }

        setAnimV2(ANIM_STAND, true);
        targetState = state;
        hSpeed      = 0;
        vSpeed      = 0;
        gravity     = false;
        v2pos       = cinfo.pos;

        return true;
    }

    bool c_checkWall() {
        if (cinfo.type == CT_FRONT || cinfo.type == CT_FRONT_CEILING) {
            c_applyOffset();
            targetState = STATE_STOP;
            hSpeed      = 0;
            gravity     = false;
            return true;
        }

        if (cinfo.type == CT_LEFT) {
            c_applyOffset();
            v2rot.y += 5 * DEG2SHORT;
            unroll(v2rot.z, 2);
        } else if (cinfo.type == CT_RIGHT) {
            c_applyOffset();
            v2rot.y -= 5 * DEG2SHORT;
            unroll(v2rot.z, 2);
        }

        return false;
    }

    bool c_checkWallUW() {
        if (cinfo.type == CT_FRONT) {
            if (v2rot.x > 35 * DEG2SHORT) {
                v2rot.x += 2 * DEG2SHORT;
            } else if (v2rot.x < -35 * DEG2SHORT) {
                v2rot.x -= 2 * DEG2SHORT;
            } else {
                vSpeed = 0;
            }
        } else if (cinfo.type == CT_CEILING) {
            if (v2rot.x >= -45 * DEG2SHORT) {
                v2rot.x -= 2 * DEG2SHORT;
            }
        } else if (cinfo.type == CT_FRONT_CEILING) {
            vSpeed = 0;
        } else if (cinfo.type == CT_LEFT) {
            v2rot.y += 5 * DEG2SHORT;
        } else if (cinfo.type == CT_RIGHT) {
            v2rot.y -= 5 * DEG2SHORT;
        } else if (cinfo.type == CT_FLOOR_CEILING) {
            v2pos = cinfo.pos;
            vSpeed = 0;
            return true;
        }

        if (cinfo.m.floor < 0) {
            v2pos.y += cinfo.m.floor;
            v2rot.x += 2 * DEG2SHORT;
        }

        if (dozy) {
            return false;
        }

        int32 waterDepth = getWaterDepth();

        if (waterDepth == TR::NO_VALUE) {
            vSpeed = 0;
            v2pos = cinfo.pos;
        } else if (waterDepth <= 512) {
            waterState = WATER_STATE_WADE;

            setAnimV2(ANIM_SWIM_STAND, true);
            targetState = STATE_STOP;

            v2rot.x = 0;
            v2rot.z = 0;
            gravity = false;
            hSpeed  = 0;
            vSpeed  = 0;

            int16 waterRoomIndex = roomIndex;
            TR::Room::Sector *sector = level->getSector(waterRoomIndex, v2pos.x, v2pos.y, v2pos.z);
            v2pos.y = level->getFloor(sector, v2pos.x, v2pos.y, v2pos.z);
        }

        return false;
    }

    bool c_checkWallSurf() {
        if ((cinfo.m.floor < 0 && cinfo.m.slantType == TR::SLANT_HIGH) || (cinfo.type & (CT_FRONT | CT_CEILING | CT_FRONT_CEILING | CT_FLOOR_CEILING))) {
            v2pos  = cinfo.pos;
            vSpeed = 0;
        } else if (cinfo.type == CT_LEFT) {
            v2rot.y += 5 * DEG2SHORT;
        } else if (cinfo.type == CT_RIGHT) {
            v2rot.y -= 5 * DEG2SHORT;
        }

        return true;
    }

    bool c_checkSlide() {
        if (waterState == WATER_STATE_WADE) {
            return false;
        }

        if (cinfo.m.slantType != TR::SLANT_HIGH) {
            return false;
        }

        c_applyOffset();

        int16 angle;

        if (cinfo.slantX > 2) {
            angle = -ANGLE_90;
        } else if (cinfo.slantX < -2) {
            angle =  ANGLE_90;
        } else if (cinfo.slantZ > 2) {
            angle = -ANGLE_180;
        } else {
            angle = 0;
        }

        if (abs(angle - v2rot.y) <= ANGLE_90) {
            if (state != STATE_SLIDE) {
                setAnimV2(ANIM_SLIDE_FORTH, true);
            }
            v2angle = angle;
            v2rot.y = angle;
        } else {
            if (state != STATE_SLIDE_BACK) {
                setAnimV2(ANIM_SLIDE_BACK, true);
            }
            v2angle = angle;
            v2rot.y = angle - ANGLE_180;
        }

        return true;
    }

    bool c_checkFall(int32 height, int32 fallAnimIndex = ANIM_FALL_FORTH) {
        if (waterState == WATER_STATE_WADE) {
            return false;
        }

        if (cinfo.m.floor <= height) {
            return false;
        }

        setAnimV2(fallAnimIndex, true);

        vSpeed  = 0;
        gravity = true;

        return true;
    }

    bool c_checkLanding() {
        if ((state == STATE_FAST_DIVE || state == STATE_AIR_ROLL) && vSpeed > 133) {
            health = 0;
            return true;
        }

        int32 y = v2pos.y;
        v2pos.y += cinfo.m.floor;
        v2floor = v2pos.y;

        checkTriggerV2(cinfo.trigger, false);

        v2pos.y = y;

        if (vSpeed <= 140) {
            return false;
        }

        if (vSpeed > 154) {
            health = 0;
        } else {
            health -= (SQR(vSpeed - 140) * LARA_MAX_HEALTH) / 196;
        }

        return checkDeath();
    }

    bool c_checkSwing() {
        int32 x = v2pos.x;
        int32 y = v2pos.y;
        int32 z = v2pos.z;
        int16 r = roomIndex;

        switch (v2rot.y) {
            case  ANGLE_0   : z += 256; break;
            case  ANGLE_90  : x += 256; break;
            case -ANGLE_90  : x -= 256; break;
            case -ANGLE_180 : z -= 256; break;
        }

        TR::Room::Sector *sector = level->getSector(r, x, y, z);
        int32 floor = level->getFloor(sector, x, y, z);

        if (floor != TR::NO_VALUE) {
            int32 ceiling = level->getCeiling(sector, x, y, z);

            floor   -= y;
            ceiling -= y;

            if (floor > 0 && ceiling < -400) {
                return true;
            }
        }

        return false;
    }

    bool c_checkGrab() {
        return !(input & ACTION) || cinfo.type != CT_FRONT || abs(cinfo.r.floor - cinfo.l.floor) >= LARA_HANG_SLANT;
    }

    bool c_checkSpace() {
        return (cinfo.f.floor < cinfo.f.ceiling ||
                cinfo.l.floor < cinfo.l.ceiling ||
                cinfo.r.floor < cinfo.r.ceiling);
    }

    bool c_checkClimbUp() {
        if (c_checkGrab()) {
            return false;
        }

        int16 angle = v2rot.y;
        if (alignAngle(angle, 30 * DEG2SHORT)) {
            return false;
        }

        if (cinfo.f.floor >= -640 && cinfo.f.floor <= -384) {
            if (c_checkSpace()) return false;

            setAnimV2(ANIM_CLIMB_2, true);
            state = STATE_HANG_UP;

            v2pos.y += 512 + cinfo.f.floor;
        } else if (cinfo.f.floor >= -896 && cinfo.f.floor <= -640) {
            if (c_checkSpace()) return false;

            setAnimV2(ANIM_CLIMB_3, true);
            state = STATE_HANG_UP;

            v2pos.y += 768 + cinfo.f.floor;
        } else if (cinfo.f.floor >= -1920 && cinfo.f.floor <= -896) {
            setAnimV2(ANIM_STAND, true);
            targetState = STATE_UP_JUMP;
            vSpeedHack = -int32(phd_sqrt(-2 * GRAVITY * (cinfo.f.floor + 800)) + 3);
            processAnimation();
        } else {
            return false;
        }

        v2rot.y = angle;
        c_applyOffset();

        return true;
    }

    bool c_checkHang() {
        if (c_checkGrab()) {
            return false;
        }

        if (cinfo.f.ceiling > 0 || cinfo.m.ceiling > -LARA_STEP_HEIGHT || (cinfo.m.floor < 200 && state == STATE_REACH)) {
            return false;
        }

        int32 h = cinfo.f.floor - getBounds().minY;
        int32 v = h + vSpeed;

        if ((h < 0 && v < 0) || (h > 0 && v > 0)) {
            LOG("%d %d\n", h, vSpeed);
            return false;
        }

        if (alignAngle(v2rot.y, 35 * DEG2SHORT)) {
            return false;
        }

        if (state == STATE_REACH) {
            if (c_checkSwing()) {
                setAnimV2(ANIM_HANG_SWING, true);
            } else {
                setAnimV2(ANIM_HANG, true);
            }
        } else {
            setAnimV2(ANIM_HANG, true, 12);
        }

        cinfo.offset.y = cinfo.f.floor - getBounds().minY;

        c_applyOffset();

        gravity = false;
        hSpeed  = 0;
        vSpeed  = 0;

        return true;
    }

    bool c_checkDrop() {
        // TODO getTrigger here

        if ((health > 0) && (input & ACTION)) {
            gravity = false;
            vSpeed  = 0;
            return false;
        }

        gravity = true;
        hSpeed  = 2;
        vSpeed  = 1;

        setAnimV2(ANIM_FALL_FORTH, true);

        return true;
    }

    bool c_checkWaterOut() {
        if (!(input & ACTION) ||
            (cinfo.type != CT_FRONT) ||
            (cinfo.f.ceiling > 0) ||
            (cinfo.m.ceiling > -LARA_STEP_HEIGHT) ||
            (abs(cinfo.r.floor - cinfo.l.floor) >= LARA_HANG_SLANT))
        {
            return false;
        }

        int32 h = cinfo.f.floor + LARA_HEIGHT_SURF;

        if (h <= -512 || h > 316) {
            return false;
        }

        if (alignAngle(v2rot.y, 35 * DEG2SHORT)) {
            return false;
        }

        v2pos.y += h - 5;

        updateRoomV2(-LARA_HEIGHT / 2);

        alignWall();

        if ((h < -128) || (level->version & TR::VER_TR1)) {
            setAnimV2(ANIM_WATER_OUT, true);
            specular = LARA_WET_SPECULAR;
        } else if (h < 128) {
            setAnimV2(ANIM_SURF_OUT, true);
        } else {
            setAnimV2(ANIM_SURF_STAND, true);
        }
        
        game->waterDrop(pos, 128.0f, 0.2f);

        targetState = STATE_STOP;
        v2rot.x = 0;
        v2rot.z = 0;
        gravity = false;
        hSpeed  = 0;
        vSpeed  = 0;
        waterState = WATER_STATE_ABOVE;

        return true;
    }

    void c_default() {
        cinfo.gapPos      = LARA_STEP_HEIGHT;
        cinfo.gapNeg      = -LARA_STEP_HEIGHT;
        cinfo.gapCeiling  = 0;
        cinfo.stopOnSlant = true;

        collideRoom(LARA_HEIGHT, 0);
    }

    void c_step() {
        cinfo.gapPos      = (waterState == WATER_STATE_WADE) ? -TR::NO_VALUE : 128;
        cinfo.gapNeg      = -128;
        cinfo.gapCeiling  = 0;
        cinfo.stopOnSlant = true;

        collideRoom(LARA_HEIGHT, 0);

        if (c_checkCeiling()) return;

        if (c_checkWall()) {
            setAnimV2(ANIM_STAND, true);
        }

        if (c_checkSlide()) return;

        v2pos.y += cinfo.m.floor;
    }

    void c_fall() {
        if (vSpeed > 0 && cinfo.m.floor <= 0) {
            if (c_checkLanding()) {
                targetState = STATE_DEATH;
            } else if (state == STATE_FORWARD_JUMP && (input & FORTH) && !(input & WALK)) {
                targetState = STATE_RUN;
            } else if (state == STATE_FALL) {
                setAnimV2(ANIM_LANDING, true);
            } else {
                targetState = STATE_STOP;
            }

            stopScreaming();

            v2pos.y += cinfo.m.floor;
            vSpeed  = 0;
            gravity = false;

            if (state == STATE_FORWARD_JUMP) {
                processAnimation();
            }
        }
    }

    void c_jump() {
        cinfo.gapPos     = -TR::NO_VALUE;
        cinfo.gapNeg     = (state == STATE_REACH) ? 0 : -LARA_STEP_HEIGHT;
        cinfo.gapCeiling = 192;

        collideRoom(state == STATE_UP_JUMP ? LARA_HEIGHT_JUMP : LARA_HEIGHT, 0);

        if ((state == STATE_REACH || state == STATE_UP_JUMP) && c_checkHang()) {
            return;
        }

        c_applyOffset();

        bool slide = (state == STATE_FALL) || (state == STATE_REACH) || (state == STATE_UP_JUMP);

        if ((cinfo.type == CT_CEILING) || ((cinfo.type == CT_FRONT_CEILING) && slide)) {
            if (vSpeed <= 0) {
                vSpeed = 1;
            }
        } else if (!slide && ((cinfo.type == CT_FRONT) || (cinfo.type == CT_FRONT_CEILING))) {
            setAnimV2(ANIM_SMASH_JUMP, true, 1);
            v2angle -= ANGLE_180;
            hSpeed /= 4;
            if (vSpeed <= 0) {
                vSpeed = 1;
            }
        } else if (cinfo.type == CT_FLOOR_CEILING) {
            v2pos.x -= (LARA_RADIUS * phd_sin(cinfo.angle)) >> FIXED_SHIFT;
            v2pos.z -= (LARA_RADIUS * phd_cos(cinfo.angle)) >> FIXED_SHIFT;
            cinfo.m.floor = 0;
            hSpeed = 0;
            if (vSpeed <= 0) {
                vSpeed = 16;
            }
        } else if (cinfo.type == CT_LEFT) {
            v2rot.y += 5 * DEG2SHORT;
        } else if (cinfo.type == CT_RIGHT) {
            v2rot.y -= 5 * DEG2SHORT;
        }

        c_fall();
    }

    void c_slide() {
        cinfo.gapPos      = -TR::NO_VALUE;
        cinfo.gapNeg      = -512;
        cinfo.gapCeiling  = 0;
        cinfo.stopOnSlant = true;

        collideRoom(LARA_HEIGHT, 0);

        if (c_checkCeiling()) return;

        c_checkWall();

        if (c_checkFall(200, state == STATE_SLIDE ? ANIM_FALL_FORTH : ANIM_FALL_BACK)) return;

        c_checkSlide();

        v2pos.y += cinfo.m.floor;

        if (cinfo.m.slantType != TR::SLANT_HIGH) {
            targetState = STATE_STOP;
        }
    }

    void c_roll() {
        vSpeed  = 0;
        gravity = false;

        cinfo.gapPos      = -TR::NO_VALUE;
        cinfo.gapNeg      = -LARA_STEP_HEIGHT;
        cinfo.gapCeiling  = 0;
        cinfo.stopOnSlant = true;

        collideRoom(LARA_HEIGHT, 0);

        if (c_checkCeiling()) return;

        if (c_checkSlide()) return;

        if (c_checkFall(200, state == STATE_ROLL_START ? ANIM_FALL_FORTH : ANIM_FALL_BACK)) return;

        c_applyOffset();

        v2pos.y += cinfo.m.floor;
    }

    void c_hang(int32 angleDelta) {
        c_angle(angleDelta);
        cinfo.gapPos     = -TR::NO_VALUE;
        cinfo.gapNeg     = TR::NO_VALUE;
        cinfo.gapCeiling = 0;
        collideRoom(LARA_HEIGHT, 0);

        bool noFloor = cinfo.f.floor < 200;

        c_angle(ANGLE_0);
        cinfo.gapPos     = -TR::NO_VALUE;
        cinfo.gapNeg     = -LARA_STEP_HEIGHT;
        cinfo.gapCeiling = 0;

        switch (cinfo.quadrant) {
            case 0 : v2pos.z += 2; break;
            case 1 : v2pos.x += 2; break;
            case 2 : v2pos.z -= 2; break;
            case 3 : v2pos.x -= 2; break;
        }

        collideRoom(LARA_HEIGHT, 0);

        v2angle = v2rot.y + angleDelta;

        if (health <= 0 || !(input & ACTION)) {
            setAnimV2(ANIM_FALL_HANG, true, 9);

            cinfo.offset.y = cinfo.f.floor - getBounds().minY + 2;
            c_applyOffset();

            hSpeed  = 2;
            vSpeed  = 1;
            gravity = true;
            return;
        }

        vSpeed  = 0;
        gravity = false;

        if (noFloor || (cinfo.type != CT_FRONT) || (cinfo.m.ceiling >= 0) || abs(cinfo.r.floor - cinfo.l.floor) >= LARA_HANG_SLANT)
        {
            if (state != STATE_HANG) {
                setAnimV2(ANIM_HANG, true, 21);
            }
            v2pos = cinfo.pos;
            return;
        }

        if (cinfo.quadrant & 1) {
            v2pos.x += cinfo.offset.x;
        } else {
            v2pos.z += cinfo.offset.z;
        }

        int32 h = cinfo.f.floor - getBounds().minY;
        if (abs(h) <= 256) {
            v2pos.y += h;
        }
    }

    enum ClimbState {
        CLIMB_HANG,
        CLIMB_COLLIDE,
        CLIMB_OK,
    };

    ClimbState c_climbCollide(int32 width) {
        int32 dx = 0, dz = 0;

        int32 x = v2pos.x;
        int32 y = v2pos.y - 512;
        int32 z = v2pos.z;

        switch (cinfo.quadrant) {
            case 0 :
                x += width;
                z += cinfo.radius;
                dz = 4;
                break;
            case 1 :
                x += cinfo.radius;
                z -= width;
                dx = 4;
                break;
            case 2 :
                x -= width;
                z -= cinfo.radius;
                dz = -4;
                break;
            case 3 :
                x -= cinfo.radius;
                z += width;
                dx = -4;
                break;
        }

        // TODO
        cinfo.offset.y = 0;
        return CLIMB_OK;
    }

    void c_climbSide(int32 width) {
        if (c_checkDrop()) {
            return;
        }

        switch (c_climbCollide(width)) {
            case CLIMB_HANG : {
                v2pos.x = cinfo.pos.x;
                v2pos.z = cinfo.pos.z;
                targetState = STATE_HANG;
                break;
            }
            case CLIMB_COLLIDE : {
                v2pos.x = cinfo.pos.x;
                v2pos.z = cinfo.pos.z;
                setAnimV2(ANIM_CLIMB_START, true);
                break;
            }
            case CLIMB_OK : {
                if (input & LEFT) {
                    targetState = STATE_CLIMB_LEFT;
                } else if (input & RIGHT) {
                    targetState = STATE_CLIMB_RIGHT;
                } else {
                    targetState = STATE_CLIMB_START;
                }
                v2pos.y += cinfo.offset.y;
                break;
            }
        }
    }

    void c_swim() {
        c_angle(ANGLE_0);

        collideRoom(LARA_HEIGHT_UW, LARA_HEIGHT_UW / 2);

        c_applyOffset();

        if (c_checkWallUW()) {
            return;
        }
    }

    void c_surf() {
        collideRoom(LARA_HEIGHT_SURF + 100, LARA_HEIGHT_SURF);

        c_applyOffset();

        c_checkWallSurf();

        if (state == STATE_SURF_TREAD) {
            if (frameIndex == 0) {
                game->waterDrop(getJoint(jointHead).pos, 96.0f, 0.03f);
            }
        } else {
            if (frameIndex % 4 == 0) {
                game->waterDrop(getJoint(jointHead).pos, 96.0f, 0.02f);
            }
        }

        int32 waterLevel = getWaterLevel();
        if (waterLevel - v2pos.y <= -100) {
            dive();
            return;
        }

        if (level->version & TR::VER_TR1) {
            return;
        }

        if ((cinfo.type == CT_FRONT) || (cinfo.m.slantType == TR::SLANT_HIGH) || (cinfo.m.floor >= 0)) {
            return;
        }

        if (cinfo.m.floor >= -128) {
            if (targetState == STATE_SURF_LEFT) {
                targetState = STATE_STEP_LEFT;
            } else if (targetState == STATE_SURF_RIGHT) {
                targetState = STATE_STEP_RIGHT;
            } else {
                setAnimV2(ANIM_WADE, true);
            }
        } else {
            setAnimV2(ANIM_WADE_ASCEND, true);
            targetState = STATE_STOP;
        }

        v2pos.y += cinfo.f.floor + LARA_HEIGHT - 5;
        updateRoomV2(-LARA_HEIGHT / 2);

        gravity = false;
        v2rot.x = 0;
        v2rot.z = 0;
        hSpeed  = 0;
        vSpeed  = 0;
        waterState = WATER_STATE_WADE;
    }

    C_HANDLER(STATE_WALK) {
        vSpeed  = 0;
        gravity = false;
        cinfo.stopOnLava = true;

        c_angle(ANGLE_0);
        c_default();

        if (c_checkCeiling()) return;
        
        if (c_checkClimbUp()) return;

        if (c_checkWall()) {
            if (frameIndex >= 29 && frameIndex <= 47) {
                setAnimV2(ANIM_STAND_RIGHT, false);
            } else if ((frameIndex >= 22 && frameIndex <= 28) || (frameIndex >= 48 && frameIndex <= 57)) {
                setAnimV2(ANIM_STAND_LEFT, false);
            } else {
                setAnimV2(ANIM_STAND, false);
            }
        }

        if (c_checkFall(LARA_STEP_HEIGHT)) return;

        // descend
        if (cinfo.m.floor > 128) {
            if (frameIndex >= 28 && frameIndex <= 45) {
                setAnimV2(ANIM_WALK_DESCEND_RIGHT, false);
            } else {
                setAnimV2(ANIM_WALK_DESCEND_LEFT, false);
            }
        }

        // ascend
        if (cinfo.m.floor >= -LARA_STEP_HEIGHT && cinfo.m.floor < -128) {
            if (frameIndex >= 27 && frameIndex <= 44) {
                setAnimV2(ANIM_WALK_ASCEND_RIGHT, false);
            } else {
                setAnimV2(ANIM_WALK_ASCEND_LEFT, false);
            }
        }

        if (c_checkSlide()) return;

        v2pos.y += cinfo.m.floor;
    }

    C_HANDLER(STATE_RUN) {
        c_angle(ANGLE_0);

        cinfo.gapPos      = -TR::NO_VALUE;
        cinfo.gapNeg      = -LARA_STEP_HEIGHT;
        cinfo.gapCeiling  = 0;
        cinfo.stopOnSlant = true;

        collideRoom(LARA_HEIGHT, 0);

        if (c_checkCeiling()) return;

        if (c_checkClimbUp()) return;

        if (c_checkWall()) {
            v2rot.z = 0;

            if (cinfo.f.slantType == TR::SLANT_NONE && cinfo.f.floor < -LARA_SMASH_HEIGHT && frameIndex < 22) {
                setAnimV2(frameIndex < 10 ? ANIM_SMASH_RUN_LEFT : ANIM_SMASH_RUN_RIGHT, false);
                state = STATE_SPLAT;
                return;
            }

            setAnimV2(ANIM_STAND, true);
        }

        if (c_checkFall(LARA_STEP_HEIGHT)) return;

        // ascend
        if (cinfo.m.floor >= -LARA_STEP_HEIGHT && cinfo.m.floor < -128) {
            if (frameIndex >= 3 && frameIndex <= 14) {
                setAnimV2(ANIM_RUN_ASCEND_RIGHT, false);
            } else {
                setAnimV2(ANIM_RUN_ASCEND_LEFT, false);
            }
        }

        if (c_checkSlide()) return;

        if (cinfo.m.floor >= 50) {
            v2pos.y += 50;
            return;
        }

        v2pos.y += cinfo.m.floor;
    }
    
    C_HANDLER(STATE_STOP) {
        vSpeed  = 0;
        gravity = false;

        c_angle(ANGLE_0);
        c_default();

        if (c_checkCeiling()) return;

        if (c_checkFall(100)) return;

        if (c_checkSlide()) return;

        c_applyOffset();

        v2pos.y += cinfo.m.floor;
    }

    C_HANDLER(STATE_FORWARD_JUMP) {
        c_angle(ANGLE_0);
        c_jump();
    }
    
    C_HANDLER(STATE_POSE) {
        c_STATE_STOP();
    }

    C_HANDLER(STATE_FAST_BACK) {
        vSpeed  = 0;
        gravity = false;

        c_angle(-ANGLE_180);

        cinfo.gapPos      = -TR::NO_VALUE;
        cinfo.gapNeg      = -LARA_STEP_HEIGHT;
        cinfo.gapCeiling  = 0;
        cinfo.stopOnSlant = true;

        collideRoom(LARA_HEIGHT, 0);

        if (c_checkCeiling()) return;

        if (c_checkFall(200, ANIM_FALL_BACK)) return;

        if (c_checkWall()) {
            setAnimV2(ANIM_STAND, false);
        }

        v2pos.y += cinfo.m.floor;
    }

    C_HANDLER(STATE_TURN_RIGHT) {
        c_angle(ANGLE_0);
        c_default();

        if (c_checkFall(100)) return;

        if (c_checkSlide()) return;

        v2pos.y += cinfo.m.floor;
    }

    C_HANDLER(STATE_TURN_LEFT) {
        c_STATE_TURN_RIGHT();
    }

    C_HANDLER(STATE_DEATH) {
        cinfo.radius = LARA_RADIUS * 4;
        
        c_angle(ANGLE_0);
        c_default();

        c_applyOffset();
        v2pos.y += cinfo.m.floor;
    }

    C_HANDLER(STATE_FALL) {
        gravity = true;
        c_jump();
    }

    C_HANDLER(STATE_HANG) {
        c_hang(0);

        if ((input & FORTH) && targetState == STATE_HANG) {

            if (cinfo.f.floor <= -850 ||
                cinfo.f.floor >= -650 ||
                c_checkSpace() ||
                cinfo.staticHit) return;

            if (input & WALK) {
                targetState = STATE_HANDSTAND;
            } else {
                targetState = STATE_HANG_UP;
            }

        }
    }

    C_HANDLER(STATE_REACH) {
        ASSERT(gravity);
        gravity = true;
        c_angle(ANGLE_0);
        c_jump();
    }

    C_HANDLER(STATE_SPLAT) {
        c_angle(ANGLE_0);
        c_default();
        c_applyOffset();
    }

    C_HANDLER(STATE_TREAD) {
        c_swim();
    }

    C_HANDLER(STATE_LAND) {
        c_STATE_STOP();
    }

    C_HANDLER(STATE_COMPRESS) {
        vSpeed  = 0;
        gravity = false;

        cinfo.gapPos      = -TR::NO_VALUE;
        cinfo.gapNeg      = TR::NO_VALUE;
        cinfo.gapCeiling  = 0;

        collideRoom(LARA_HEIGHT, 0);

        if (cinfo.m.ceiling > -100) {
            setAnimV2(ANIM_STAND, true);
            hSpeed  = 0;
            vSpeed  = 0;
            gravity = false;
            v2pos   = cinfo.pos;
        }
    }

    C_HANDLER(STATE_BACK) {
        vSpeed  = 0;
        gravity = false;

        c_angle(-ANGLE_180);

        cinfo.gapPos      = (waterState == WATER_STATE_WADE) ? -TR::NO_VALUE : LARA_STEP_HEIGHT;
        cinfo.gapNeg      = -LARA_STEP_HEIGHT;
        cinfo.gapCeiling  = 0;
        cinfo.stopOnSlant = true;

        collideRoom(LARA_HEIGHT, 0);

        if (c_checkCeiling()) return;

        if (c_checkWall()) {
            setAnimV2(ANIM_STAND, true);
        }

        if (cinfo.m.floor > 128 && cinfo.m.floor < LARA_STEP_HEIGHT) {
            if (frameIndex < 568) {
                setAnimV2(ANIM_BACK_DESCEND_LEFT, false);
            } else {
                setAnimV2(ANIM_BACK_DESCEND_RIGHT, false);
            }
        }

        if (c_checkSlide()) return;

        v2pos.y += cinfo.m.floor;
    }

    C_HANDLER(STATE_SWIM) {
        c_swim();
    }

    C_HANDLER(STATE_GLIDE) {
        c_swim();
    }

    C_HANDLER(STATE_HANG_UP) {
        c_angle(ANGLE_0);
        c_default();
    }

    C_HANDLER(STATE_FAST_TURN) {
        c_STATE_STOP();
    }

    C_HANDLER(STATE_STEP_RIGHT) {
        c_angle(+ANGLE_90);
        c_step();
    }

    C_HANDLER(STATE_STEP_LEFT) {
        c_angle(-ANGLE_90);
        c_step();
    }

    C_HANDLER(STATE_ROLL_END) {
        c_angle(-ANGLE_180);
        c_roll();
    }

    C_HANDLER(STATE_SLIDE) {
        c_angle(ANGLE_0);
        c_slide();
    }

    C_HANDLER(STATE_BACK_JUMP) {
        c_angle(-ANGLE_180);
        c_jump();
    }

    C_HANDLER(STATE_RIGHT_JUMP) {
        c_angle(ANGLE_90);
        c_jump();
    }

    C_HANDLER(STATE_LEFT_JUMP) {
        c_angle(-ANGLE_90);
        c_jump();
    }

    C_HANDLER(STATE_UP_JUMP) {
        c_angle(ANGLE_0);
        c_jump();
    }
    
    C_HANDLER(STATE_FALL_BACK) {
        c_angle(-ANGLE_180);
        c_jump();
    }

    C_HANDLER(STATE_HANG_LEFT) {
        c_hang(-ANGLE_90);
    }

    C_HANDLER(STATE_HANG_RIGHT) {
        c_hang(ANGLE_90);
    }

    C_HANDLER(STATE_SLIDE_BACK) {
        c_angle(-ANGLE_180);
        c_slide();
    }

    C_HANDLER(STATE_SURF_TREAD) {
        c_angle(ANGLE_0);
        c_surf();
    }

    C_HANDLER(STATE_SURF_SWIM) {
        cinfo.gapNeg = -LARA_STEP_HEIGHT;
        c_angle(ANGLE_0);
        c_surf();
        c_checkWaterOut();
    }

    C_HANDLER(STATE_DIVE) {
        c_swim();
    }

    C_HANDLER(STATE_PUSH_BLOCK) {
        c_angle(ANGLE_0);
        c_default();
    }

    C_HANDLER(STATE_PULL_BLOCK) {
        c_angle(ANGLE_0);
        c_default();
    }

    C_HANDLER(STATE_PUSH_PULL_READY) {
        c_angle(ANGLE_0);
        c_default();
    }

    C_HANDLER(STATE_PICK_UP) {
        c_angle(ANGLE_0);
        c_default();
    }

    C_HANDLER(STATE_SWITCH_DOWN) {
        c_angle(ANGLE_0);
        c_default();
    }

    C_HANDLER(STATE_SWITCH_UP) {
        c_angle(ANGLE_0);
        c_default();
    }

    C_HANDLER(STATE_USE_KEY) {
        c_angle(ANGLE_0);
        c_default();
    }

    C_HANDLER(STATE_USE_PUZZLE) {
        c_angle(ANGLE_0);
        c_default();
    }

    C_HANDLER(STATE_UW_DEATH) {
        health = 0;
        oxygen = 0;

        int16 waterLevel = getWaterLevel();
        if (waterLevel != TR::NO_VALUE && waterLevel < v2pos.y - LARA_RADIUS) {
            v2pos.y -= LARA_FLOAT_UP_SPEED;
        }

        c_swim();
    }

    C_HANDLER(STATE_ROLL_START) {
        c_angle(ANGLE_0);
        c_roll();
    }

    C_HANDLER(STATE_SPECIAL) {
        // empty
    }

    C_HANDLER(STATE_SURF_BACK) {
        c_angle(ANGLE_180);
        c_surf();
    }

    C_HANDLER(STATE_SURF_LEFT) {
        c_angle(-ANGLE_90);
        c_surf();
    }

    C_HANDLER(STATE_SURF_RIGHT) {
        c_angle(ANGLE_90);
        c_surf();
    }

    C_HANDLER(STATE_MIDAS_USE) {
        c_angle(ANGLE_0);
        c_default();
    }

    C_HANDLER(STATE_MIDAS_DEATH) {
        c_angle(ANGLE_0);
        c_default();
    }

    C_HANDLER(STATE_SWAN_DIVE) {
        c_angle(ANGLE_0);
        c_jump();
    }

    C_HANDLER(STATE_FAST_DIVE) {
        c_angle(ANGLE_0);
        c_jump();
    }
    
    C_HANDLER(STATE_HANDSTAND) {
        c_angle(ANGLE_0);
        c_default();
    }

    C_HANDLER(STATE_WATER_OUT) {
        c_angle(ANGLE_0);
        c_default();
    }

    C_HANDLER(STATE_CLIMB_START) {}
    C_HANDLER(STATE_CLIMB_UP) {}

    C_HANDLER(STATE_CLIMB_LEFT) {
        c_angle(-ANGLE_90);
        c_climbSide(-LARA_RADIUS_CLIMB);
    }

    C_HANDLER(STATE_CLIMB_END) {
        // empty
    }

    C_HANDLER(STATE_CLIMB_RIGHT) {
        c_angle(ANGLE_90);
        c_climbSide(LARA_RADIUS_CLIMB);
    }

    C_HANDLER(STATE_CLIMB_DOWN) {}

    C_HANDLER(STATE_UNUSED_1) {
        // empty
    }

    C_HANDLER(STATE_UNUSED_2) {
        // empty
    }

    C_HANDLER(STATE_UNUSED_3) {
        // empty
    }

    C_HANDLER(STATE_WADE) {
        c_STATE_RUN();
    }

    C_HANDLER(STATE_UW_ROLL) {
        c_swim();
    }

    C_HANDLER(STATE_PICKUP_FLARE) {}

    C_HANDLER(STATE_AIR_ROLL) {
        // empty
    }
    
    C_HANDLER(STATE_UNUSED_5) {
        // empty
    }

    C_HANDLER(STATE_ZIPLINE) {
        // empty
    }

    C_HANDLER(STATE_DUCK) {}
    C_HANDLER(STATE_DUCK_ROLL) {}
    C_HANDLER(STATE_DASH) {}
    C_HANDLER(STATE_DASH_DIVE) {}
    C_HANDLER(STATE_MONKEYSWING_IDLE) {}
    C_HANDLER(STATE_MONKEYSWING) {}
#endif
};

const Lara::sHandler Lara::sLaraHandlers[Lara::STATE_MAX] = { LARA_STATES(DECL_S_HANDLER) };
const Lara::cHandler Lara::cLaraHandlers[Lara::STATE_MAX] = { LARA_STATES(DECL_C_HANDLER) };

#undef LARA_STATES
#undef DECL_S_HANDLER
#undef DECL_C_HANDLER

#endif
