#ifndef H_CAMERA
#define H_CAMERA

#include "core.h"
#include "frustum.h"
#include "controller.h"
#include "character.h"

#define CAMERA_OFFSET (1024.0f + 256.0f)

struct Camera : ICamera {

    enum {
        STATE_FOLLOW,
        STATE_STATIC,
        STATE_LOOK,
        STATE_COMBAT,
        STATE_CUTSCENE,
        STATE_HEAVY
    } state;

    IGame      *game;
    TR::Level  *level;
    Character  *owner;
    Frustum    *frustum;

    float   fov, znear, zfar;
    vec3    target, destPos, lastDest, angle, advAngle;
    float   advTimer;
    mat4    mViewInv;
    int     room;

    float   timer;
    float   shake;

    Basis   prevBasis;

    int         viewIndex;
    int         viewIndexLast;
    Controller* viewTarget;
    float       speed;

    bool    firstPerson;
    bool    isVR;

    Camera(IGame *game, Character *owner) : ICamera(), game(game), level(game->getLevel()), owner(owner), frustum(new Frustum()), timer(-1.0f), shake(0.0f), viewIndex(-1), viewIndexLast(-1), viewTarget(NULL), isVR(false) {
        changeView(false);
        if (owner->getEntity().type != TR::Entity::LARA && level->cameraFrames) {
            state = STATE_CUTSCENE;
            room  = level->entities[level->cutEntity].room;
        } else
            state = STATE_FOLLOW;
        destPos  = owner->pos - owner->getDir() * 1024.0f;
        pos      = destPos;
        advTimer = -1.0f;
    }

    virtual ~Camera() {
        delete frustum;
    }
    
    virtual int getRoomIndex() const {
        return room;
    }

    virtual void checkRoom() {
        if (state == STATE_CUTSCENE) {
            for (int i = 0; i < level->roomsCount; i++)
                if (owner->insideRoom(pos, i)) {
                    room = i;
                    break;
                }
            return;
        }

        TR::Level::FloorInfo info;
        level->getFloorInfo(getRoomIndex(), (int)pos.x, (int)pos.y, (int)pos.z, info);

        if (info.roomNext != TR::NO_ROOM)
            room = info.roomNext;
        
        if (pos.y < info.roomCeiling) {
            if (info.roomAbove != TR::NO_ROOM)
                room = info.roomAbove;
            else
                if (info.roomCeiling != 0xffff8100)
                    pos.y = (float)info.roomCeiling;
        }

        if (pos.y > info.roomFloor) {
            if (info.roomBelow != TR::NO_ROOM)
                room = info.roomBelow;
            else
                if (info.roomFloor != 0xffff8100)
                    pos.y = (float)info.roomFloor;
        }
    }

    void updateListener() {
        Sound::listener.matrix = mViewInv;
        TR::Room &r = level->rooms[getRoomIndex()];
        int h = (r.info.yBottom - r.info.yTop) / 1024;
        Sound::reverb.setRoomSize(vec3(float(r.xSectors), float(h), float(r.zSectors)) * 2.419f); // convert cells size into meters
    }

    bool isUnderwater() {
        return level->rooms[getRoomIndex()].flags.water;
    }

    void setView(int viewIndex, float timer, float speed) {
//        if (viewIndex == viewIndexLast) return;
        viewIndexLast = viewIndex;

        state           = STATE_STATIC;
        this->viewIndex = viewIndex;
        this->timer     = timer;
        this->speed     = speed;
        lastDest        = destPos;

        if (viewIndex > -1) {
            TR::Camera &cam = level->cameras[viewIndex];
            room = cam.room;
            pos  = vec3(float(cam.x), float(cam.y), float(cam.z));
        }
    }

    vec3 getViewPoint() {
        vec3 p = owner->getViewPoint();
        if (owner->stand != Character::STAND_UNDERWATER)
            p.y -= 256.0f;
        if (state == STATE_COMBAT)
            p.y -= 256.0f;
        return p;
    }

    void resetTarget(const vec3 &viewPoint) {
        if (state == STATE_STATIC)
            pos = destPos = lastDest;

        timer      = -1.0f;
        state      = STATE_FOLLOW;
        viewTarget = NULL;
        viewIndex  = -1;
        target     = viewPoint;
    }

    virtual void update() {
        if (shake > 0.0f)
            shake = max(0.0f, shake - Core::deltaTime);

        if (state == STATE_CUTSCENE) {
            timer += Core::deltaTime * 30.0f;
            float t = timer - int(timer);
            int indexA = min(int(timer), level->cameraFramesCount - 1);
            int indexB = min((indexA + 1), level->cameraFramesCount - 1);

            if (indexA == level->cameraFramesCount - 1) {
                if (level->cutEntity != -1)
                    game->loadLevel(TR::LevelID(level->id + 1));
                else
                    state = STATE_FOLLOW;
            }

            TR::CameraFrame *frameA = &level->cameraFrames[indexA];
            TR::CameraFrame *frameB = &level->cameraFrames[indexB];

            const int eps = 512;

            if (abs(frameA->pos.x - frameB->pos.x) > eps || abs(frameA->pos.y - frameB->pos.y) > eps || abs(frameA->pos.z - frameB->pos.z) > eps) {
                pos    = frameA->pos;
                target = frameA->target;
                fov    = frameA->fov / 32767.0f * 120.0f;
            } else {
                pos    = vec3(frameA->pos).lerp(frameB->pos, t);
                target = vec3(frameA->target).lerp(frameB->target, t);
                fov    = lerp(frameA->fov / 32767.0f * 120.0f, frameB->fov / 32767.0f * 120.0f, t);
            }

            pos    = level->cutMatrix * pos;
            target = level->cutMatrix * target;

            checkRoom();
        } else {
            vec3 advAngleOld = advAngle;

            if (Input::down[ikMouseL]) {
                vec2 delta = Input::mouse.pos - Input::mouse.start.L;
                advAngle.x -= delta.y * 0.01f;
                advAngle.y += delta.x * 0.01f;
                Input::mouse.start.L = Input::mouse.pos;
            }

            advAngle.x -= Input::joy.R.y * 2.0f * Core::deltaTime;
            advAngle.y += Input::joy.R.x * 2.0f * Core::deltaTime;

            if (advAngleOld == advAngle) {
                if (advTimer > 0.0f) {
                    advTimer = max(0.0f, advTimer - Core::deltaTime);
                }
            } else
                advTimer = -1.0f;

            if (owner->velocity != 0.0f && advTimer < 0.0f && !Input::down[ikMouseL])
                advTimer = -advTimer;

            if (advTimer == 0.0f && advAngle != 0.0f) {
                float t = 10.0f * Core::deltaTime;
                advAngle.x = lerp(clampAngle(advAngle.x), 0.0f, t);
                advAngle.y = lerp(clampAngle(advAngle.y), 0.0f, t);
            }

            angle = owner->angle + advAngle;
            angle.z = 0.0f;

            if (owner->stand == Character::STAND_ONWATER)
                angle.x -= 22.0f * DEG2RAD;
            if (owner->stand == Character::STAND_HANG)
                angle.x -= 60.0f * DEG2RAD;


            Controller *lookAt = viewTarget;
            
            if (state != STATE_STATIC) {
                if (owner->viewTarget)
                    owner->lookAt(lookAt = owner->viewTarget);
                else
                    owner->lookAt(lookAt = viewTarget);
            } else 
                 owner->lookAt(NULL);

            vec3 viewPoint = getViewPoint();

            if (firstPerson && viewIndex == -1) {
                Basis head = owner->animation.getJoints(owner->getMatrix(), 14, true);
                Basis eye(quat(0.0f, 0.0f, 0.0f, 1.0f), vec3(0.0f, -40.0f, 10.0f));
                eye = head * eye;
                mViewInv.identity();

                //prevBasis = prevBasis.lerp(eye, 15.0f * Core::deltaTime);

                mViewInv.setRot(eye.rot);
                mViewInv.setPos(eye.pos);
                mViewInv.rotateY(advAngle.y);
                mViewInv.rotateX(advAngle.x + PI);

                pos = mViewInv.getPos();
                checkRoom();
                updateListener();
                return;
            }

            float lerpFactor = lookAt ? 10.0f : 6.0f;
            vec3 dir;

            target = target.lerp(viewPoint, lerpFactor * Core::deltaTime);

            if (viewIndex > -1) {
                TR::Camera &cam = level->cameras[viewIndex];
                destPos = vec3(float(cam.x), float(cam.y), float(cam.z));
                if (room != getRoomIndex()) 
                    pos = destPos;
                if (lookAt)
                    target = lookAt->pos;
            } else {
                if (lookAt) {
                    dir = (lookAt->pos - target).normal();
                } else
                    dir = vec3(angle.x, angle.y);

                int destRoom;
                if ((state == STATE_COMBAT || owner->state != 25) || lookAt) { // TODO: FUUU! 25 == Lara::STATE_BACK_JUMP
                    vec3 eye = target - dir * CAMERA_OFFSET;
                    destPos = owner->trace(owner->getRoomIndex(), target, eye, destRoom, true);
                    lastDest = destPos;
                } else {
                    vec3 eye = lastDest + dir.cross(vec3(0, 1, 0)).normal() * 2048.0f - vec3(0.0f, 512.0f, 0.0f);
                    destPos = owner->trace(owner->getRoomIndex(), target, eye, destRoom, true);
                }

                room = destRoom;
            }
            pos = pos.lerp(destPos, Core::deltaTime * lerpFactor);

            if (timer > 0.0f) {
                timer -= Core::deltaTime;
                if (timer <= 0.0f)
                    resetTarget(viewPoint);
            } else
                resetTarget(target);

            if (viewIndex == -1)
                checkRoom();
        }

        mViewInv = mat4(pos, target, vec3(0, -1, 0));
        if (isVR) {
            mat4 head = Input::head.getMatrix();
            mViewInv = mViewInv * head;
        }
        updateListener();
    }

    mat4 getProjMatrix() {
        return mat4(fov, float(Core::width) / float(Core::height), znear, zfar);
        //return mat4(fov, Core::viewport.z / Core::viewport.w, znear, zfar);
    }

    virtual void setup(bool calcMatrices) {
        if (calcMatrices) {
            if (reflectPlane) {
                Core::mViewInv = mat4(*reflectPlane) * mViewInv;
                Core::mViewInv.scale(vec3(1.0f, -1.0f, 1.0f));
            } else
                Core::mViewInv = mViewInv;

            Core::mView = Core::mViewInv.inverse();
            if (shake > 0.0f)
                Core::mView.translate(vec3(0.0f, sinf(shake * PI * 7) * shake * 48.0f, 0.0f));

            if (isVR)
                Core::mView.translate(Core::mViewInv.right.xyz * (-Core::eye * 32.0f));

            Core::mProj = getProjMatrix();

        // TODO: temporal anti-aliasing
        //    Core::mProj.e02 = (randf() - 0.5f) * 32.0f / Core::width;
        //    Core::mProj.e12 = (randf() - 0.5f) * 32.0f / Core::height;
        }
        Core::mViewProj = Core::mProj * Core::mView;
        Core::viewPos   = Core::mViewInv.offset.xyz;

        frustum->pos = Core::viewPos;
        frustum->calcPlanes(Core::mViewProj);
    }

    void changeView(bool firstPerson) {
        this->firstPerson = firstPerson;

        room     = owner->getRoomIndex();
        pos      = owner->pos - owner->getDir() * 1024.0f;
        target   = getViewPoint();
        advAngle = vec3(0.0f);
        advTimer = 0.0f;

        fov   = firstPerson ? 90.0f : 65.0f;
        znear = firstPerson ? 8.0f  : 128.0f;
        zfar  = 40.0f * 1024.0f;
    }
};

#endif