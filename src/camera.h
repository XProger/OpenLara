#ifndef H_CAMERA
#define H_CAMERA

#include "core.h"
#include "frustum.h"
#include "controller.h"
#include "character.h"

#define CAM_OFFSET_FOLLOW (1024.0f + 512.0f)
#define CAM_OFFSET_COMBAT (2048.0f + 512.0f)
#define CAM_OFFSET_LOOK   (512.0f)

#define CAM_SPEED_FOLLOW  12
#define CAM_SPEED_COMBAT  8

#define CAM_FOLLOW_ANGLE     0.0f
#define CAM_LOOK_ANGLE_XMAX  ( 55.0f * DEG2RAD)
#define CAM_LOOK_ANGLE_XMIN  (-75.0f * DEG2RAD)
#define CAM_LOOK_ANGLE_Y     ( 80.0f * DEG2RAD)

struct Camera : ICamera {
    IGame      *game;
    TR::Level  *level;
    Character  *owner;
    Frustum    *frustum;

    float       fov, aspect, znear, zfar;
    vec3        lookAngle, targetAngle;
    mat4        mViewInv;

    float       timer;

    int         viewIndex;
    int         viewIndexLast;
    Controller* viewTarget;
    Basis       fpHead;
    int         speed;
    bool        smooth;

    Camera(IGame *game, Character *owner) : ICamera(), game(game), level(game->getLevel()), frustum(new Frustum()), timer(-1.0f), viewIndex(-1), viewIndexLast(-1), viewTarget(NULL) {
        this->owner = owner;
        reset();
    }

    void reset() {
        Sound::listener[cameraIndex].matrix.identity();
        Sound::listener[cameraIndex].matrix.translate(vec3(float(0x7FFFFFFF)));

        lookAngle = vec3(0.0f);

        changeView(false);
        if (level->isCutsceneLevel()) {
            mode = MODE_CUTSCENE;
//            room  = level->entities[level->cutEntity].room;
            timer = 0.0f;
        } else
            mode = MODE_FOLLOW;

        eye.pos     = owner->pos;
        eye.room    = owner->getRoomIndex();
        target.pos  = owner->pos;
        target.room = eye.room;

        target.pos.y -= 1024;
        eye.pos.z  -= 100;

        speed  = CAM_SPEED_FOLLOW;
        smooth = false;
    }

    virtual ~Camera() {
        delete frustum;
    }
    
    virtual Controller* getOwner() {
        return owner;
    }

    virtual int getRoomIndex() const {
        return eye.room;
    }

    void updateListener(const mat4 &matrix) {
        Sound::flipped = level->state.flags.flipped;
        Sound::listener[cameraIndex].matrix = matrix;
        if (cameraIndex == 0) { // reverb effect only for main player
            TR::Room &r = level->rooms[getRoomIndex()];
            float h = (r.info.yBottom - r.info.yTop) / 1024.0f;
            Sound::reverb.setRoomSize(vec3(float(r.xSectors), h, float(r.zSectors)) * 2.419f); // convert cells size into meters
        }
    }

    bool isUnderwater() {
        return level->rooms[getRoomIndex()].flags.water;
    }

    vec3 getViewPoint() {
        Box box = owner->getBoundingBox();
        vec3 pos = owner->pos;
        vec3 center = box.center();

        if (centerView) {
            pos.x = center.x;
            pos.z = center.z;
        }

        if (mode == MODE_LOOK) {
            Basis b = owner->getJoint(owner->jointHead);
            b.translate(vec3(0, -128, 0));
            pos = b.pos;
        } else {
            if (mode != MODE_STATIC)
                pos.y = box.max.y + (box.min.y - box.max.y) * (3.0f / 4.0f);
            else
                pos.y = center.y;

            if (owner->stand != Character::STAND_UNDERWATER)
                pos.y -= 256;
        }

        return pos;
    }

    void setView(int viewIndex, float timer, int speed) {
        viewIndexLast   = viewIndex;
        smooth          = speed > 0;
        mode            = MODE_STATIC;
        this->viewIndex = viewIndex;
        this->timer     = timer;
        this->speed     = speed * 8;
    }

    void resetTarget() {
        smooth        = speed > 0;
        mode          = MODE_FOLLOW;
        viewIndex     = -1;
        viewTarget    = NULL;
        timer         = -1.0f;
        speed         = CAM_SPEED_FOLLOW;
    }

    virtual void doCutscene(const vec3 &pos, float rotation) {
        mode = Camera::MODE_CUTSCENE;
        level->cutMatrix.identity();
        level->cutMatrix.rotateY(rotation);
        level->cutMatrix.setPos(pos);
        owner->animation.overrideMask = 0;
        timer = 0.0f;
    }

    bool updateFirstPerson() {
        Basis &joint = owner->getJoint(owner->jointHead);

        if (mode != MODE_CUTSCENE && !owner->useHeadAnimation()) {
            targetAngle.x += PI;
            targetAngle.z = -targetAngle.z;

            vec3 pos = joint.pos - joint.rot * vec3(0, 48, -24);
            quat rot = rotYXZ(targetAngle);

            fpHead.pos = pos;
            fpHead.rot = fpHead.rot.lerp(rot, smooth ? Core::deltaTime * 10.0f : 1.0f);
        } else {
            fpHead = joint;
            fpHead.rot = fpHead.rot * quat(vec3(1, 0, 0), PI);
            fpHead.pos -= joint.rot * vec3(0, 48, -24);
        }

        if (Core::settings.detail.stereo == Core::Settings::STEREO_VR)
            fpHead.rot = quat(vec3(1, 0, 0), PI);

        mViewInv.identity();
        mViewInv.setRot(fpHead.rot);
        mViewInv.setPos(fpHead.pos);

        eye.pos  = mViewInv.getPos();
        eye.room = owner->getRoomIndex();

        return true;
    }

    void clipSlide(float offset, float *toX, float *toZ, float targetX, float targetZ, const TR::Box &box) {
        TR::Box sqBox;
        sqBox.minZ = SQR(int(targetZ) - box.minZ);
        sqBox.maxZ = SQR(int(targetZ) - box.maxZ);
        sqBox.minX = SQR(int(targetX) - box.minX);
        sqBox.maxX = 0; // unused

        offset *= offset;

        int32 h = sqBox.minX + sqBox.minZ;
        if (h > 256 * 256) {
            *toX = float(box.minX);
            if (offset > h || offset < sqBox.minX)
                *toZ = float(box.minZ);
            else
                *toZ = targetZ + sqrtf(offset - sqBox.minX) * sign(box.minZ - box.maxZ);
            return;
        }
    
        h = sqBox.minX + sqBox.maxZ;
        if (h > 256 * 256) {
            *toX = float(box.minX);
            if (offset > h || offset < sqBox.minX)
                *toZ = float(box.maxZ);
            else
                *toZ = targetZ - sqrtf(offset - sqBox.minX) * sign(box.minZ - box.maxZ);
            return;
        }

        h = sqBox.maxX + sqBox.minZ;
        *toZ = float(box.minZ);
        if (offset > h || offset < sqBox.minZ)
            *toX = float(box.maxX);
        else
            *toX = targetX - sqrtf(offset - sqBox.minZ) * sign(box.minX - box.maxX);
    }

    void traceClip(float offset, TR::Location &to) {
        owner->trace(target, to);

        uint16 ownerBoxIndex  = level->getSector(target.room, target.pos)->boxIndex;
        uint16 cameraBoxIndex = level->getSector(to.room, to.pos)->boxIndex;

        if (ownerBoxIndex == TR::NO_BOX)
            return;

        TR::Box cBox = level->boxes[ownerBoxIndex];
        if (cameraBoxIndex != TR::NO_BOX && !level->boxes[ownerBoxIndex].contains(int(to.pos.x), int(to.pos.z)))
            cBox = level->boxes[cameraBoxIndex];

        clipBox(to.room, to.pos, cBox);
        cBox.expand(-256);

        // transform coord system by clipping direction
        float *toX    = &to.pos.x, 
              *toZ    = &to.pos.z,
              targetX = target.pos.x, 
              targetZ = target.pos.z;

        if (to.pos.z < cBox.minZ || to.pos.z > cBox.maxZ) {
            swap(toX, toZ);
            swap(targetX, targetZ);
            swap(cBox.minX, cBox.minZ);
            swap(cBox.maxX, cBox.maxZ);
        }

        if (*toX >= cBox.minX && *toX <= cBox.maxX)
            return;

        if (*toX > cBox.maxX)
            swap(cBox.minX, cBox.maxX);
    
        if (*toZ > targetZ)
            swap(cBox.minZ, cBox.maxZ);

        clipSlide(offset, toX, toZ, targetX, targetZ, cBox);
        level->getSector(to.room, to.pos);
    }

    void clipBox(int16 roomIndex, const vec3 &pos, TR::Box &box) {
        const TR::Room &room = level->rooms[roomIndex];
    
        int dx = int(pos.x) / 1024 * 1024;
        int dz = int(pos.z) / 1024 * 1024;

        TR::Box border;
        border.minX = dx - 1;
        border.minZ = dz - 1;
        border.maxX = dx + 1024;
        border.maxZ = dz + 1024;

        for (int i = 0; i < 4; i++) {
            vec3 p = pos;

            if (i / 2)
                p.x = float(border.sides[i]);
            else
                p.z = float(border.sides[i]);

            int16 index = roomIndex;
            if (level->isBlocked(index, p)) {
                box.sides[i] = border.sides[i];
                continue;
            }

            int sx = clamp((int(p.x) - room.info.x) / 1024, 0, room.xSectors - 1);
            int sz = clamp((int(p.z) - room.info.z) / 1024, 0, room.zSectors - 1);

            int boxIndex = room.sectors[sz + sx * room.zSectors].boxIndex;
        
            if (boxIndex != TR::NO_BOX) {
                if (i % 2)
                    box.sides[i] = max(box.sides[i], level->boxes[boxIndex].sides[i]);
                else
                    box.sides[i] = min(box.sides[i], level->boxes[boxIndex].sides[i]);
            }
        }
    }

    void move(const TR::Location &to) {
        float t = (smooth && speed) ? (30.0f / speed * Core::deltaTime) : 1.0f;
        if (mode == MODE_LOOK)
            t = 20.0f * Core::deltaTime;

        eye.pos  = eye.pos.lerp(to.pos, t);
        eye.room = to.room;

        TR::Room::Sector *sector = level->getSector(eye.room, eye.pos);

        float floor = level->getFloor(sector, eye.pos) - 256;

        if (to.pos.y >= floor && eye.pos.y >= floor) {
            owner->trace(target, eye);
            sector = level->getSector(eye.room, eye.pos);
            floor  = level->getFloor(sector, eye.pos) - 256;
        }
    
        float ceiling = level->getCeiling(sector, eye.pos) + 256;
        if (floor < ceiling)
            floor = ceiling = (floor + ceiling) * 0.5f;

        if (eye.pos.y > floor)   eye.pos.y = floor;
        if (eye.pos.y < ceiling) eye.pos.y = ceiling;
    }

    virtual void update() {
        if (shake > 0.0f) {
            shake = max(0.0f, shake - Core::deltaTime);
            Input::setJoyVibration(cameraIndex,  clamp(shake, 0.0f, 1.0f), 0);
        }

        if (mode == MODE_CUTSCENE) {
            ASSERT(level->cameraFramesCount && level->cameraFrames);

            timer += Core::deltaTime * 30.0f;
            float t = timer - int(timer);
            int indexA = min(int(timer), level->cameraFramesCount - 3);
            int indexB = min((indexA + 1), level->cameraFramesCount - 3);

            if (indexA == level->cameraFramesCount - 3) {
                if (level->isCutsceneLevel())
                    game->loadNextLevel();
                else {
                    Character *lara = (Character*)owner;
                    if (lara->health > 0.0f) {
                        mode = MODE_FOLLOW;
                        return;
                    }
                }
            }

            if (!firstPerson) {
                TR::CameraFrame *frameA = &level->cameraFrames[indexA];
                TR::CameraFrame *frameB = &level->cameraFrames[indexB];

                const float maxDelta = 512 * 512;

                float dp = (vec3(frameA->pos) - vec3(frameB->pos)).length2();
                float dt = (vec3(frameA->target) - vec3(frameB->target)).length2();

                if (dp > maxDelta || dt > maxDelta) {
                    eye.pos    = frameA->pos;
                    target.pos = frameA->target;
                    fov        = frameA->fov / 32767.0f * 120.0f;
                } else {
                    eye.pos    = vec3(frameA->pos).lerp(frameB->pos, t);
                    target.pos = vec3(frameA->target).lerp(frameB->target, t);
                    fov        = lerp(frameA->fov / 32767.0f * 120.0f, frameB->fov / 32767.0f * 120.0f, t);
                }

                eye.pos    = level->cutMatrix * eye.pos;
                target.pos = level->cutMatrix * target.pos;

                mViewInv   = mat4(eye.pos, target.pos, vec3(0, -1, 0));
            } else
                updateFirstPerson();
        } else {
            if (Core::settings.detail.stereo == Core::Settings::STEREO_VR) {
                lookAngle = vec3(0.0f);
            } else {
                if (mode == MODE_LOOK) {
                    float d = 3.0f * Core::deltaTime;

                    lookAngle.x += Input::joy[cameraIndex].L.y * d;
                    lookAngle.y += Input::joy[cameraIndex].L.x * d;

                    if (Input::state[cameraIndex][cUp])    lookAngle.x -= d;
                    if (Input::state[cameraIndex][cDown])  lookAngle.x += d;
                    if (Input::state[cameraIndex][cLeft])  lookAngle.y -= d;
                    if (Input::state[cameraIndex][cRight]) lookAngle.y += d;

                    lookAngle.x = clamp(lookAngle.x,  CAM_LOOK_ANGLE_XMIN, CAM_LOOK_ANGLE_XMAX);
                    lookAngle.y = clamp(lookAngle.y, -CAM_LOOK_ANGLE_Y,    CAM_LOOK_ANGLE_Y);
                } else
                    if (lookAngle.x != CAM_FOLLOW_ANGLE || lookAngle.y != 0.0f) {
                        float t = 10.0f * Core::deltaTime;
                        lookAngle.x = lerp(clampAngle(lookAngle.x), CAM_FOLLOW_ANGLE, t);
                        lookAngle.y = lerp(clampAngle(lookAngle.y), 0.0f, t);
                        if (fabsf(lookAngle.x - CAM_FOLLOW_ANGLE) < EPS) lookAngle.x = CAM_FOLLOW_ANGLE;
                        if (lookAngle.y < EPS) lookAngle.y = 0.0f;
                    }
            }

            targetAngle = owner->angle + lookAngle;

            targetAngle.x = clampAngle(targetAngle.x);
            targetAngle.y = clampAngle(targetAngle.y);

            targetAngle.x = clamp(targetAngle.x, -85 * DEG2RAD, +85 * DEG2RAD);

            Controller *lookAt = NULL;

            if (mode != MODE_STATIC) {
                if (!owner->viewTarget) {
                    if (viewTarget && !viewTarget->flags.invisible) {
                        vec3 targetVec = (viewTarget->pos - owner->pos).normal();
                        if (targetVec.dot(owner->getDir()) > 0.1f) {
                            lookAt = viewTarget;
                        }
                    }
                } else {
                    lookAt = owner->viewTarget;
                }

                owner->lookAt(lookAt);
            } else {
                lookAt = viewTarget;
                owner->lookAt(NULL);
            }

            if (!firstPerson && (mode == MODE_FOLLOW || mode == MODE_COMBAT)) {
                targetAngle += angle;
            }

            if (!firstPerson || viewIndex != -1) {

                if (timer > 0.0f) {
                    timer -= Core::deltaTime;
                    if (timer <= 0.0f) {
                        resetTarget();
                    }
                }

                TR::Location to;

                target.box  = TR::NO_BOX;
                if (viewIndex > -1) {
                    TR::Camera &cam = level->cameras[viewIndex];
                    to.room = cam.room;
                    to.pos  = vec3(float(cam.x), float(cam.y), float(cam.z));
                    if (lookAt) {
                        target.room = lookAt->getRoomIndex();
                        target.pos  = lookAt->getBoundingBox().center();
                    } else {
                        target.room = owner->getRoomIndex();
                        target.pos  = owner->pos;
                        target.pos.y -= 512.0f;
                    }
                } else {
                    vec3 p = getViewPoint();
                    target.room = owner->getRoomIndex();
                    target.pos.x = p.x;
                    target.pos.z = p.z;
                    if (smooth)
                        target.pos.y += (p.y - target.pos.y) * Core::deltaTime * 5.0f;
                    else
                        target.pos.y = p.y;

                    float offset;
                    if (mode == MODE_LOOK)
                        offset = CAM_OFFSET_LOOK;
                    else 
                        offset = (mode == MODE_COMBAT ? CAM_OFFSET_COMBAT : CAM_OFFSET_FOLLOW) * cosf(targetAngle.x);

                    vec3 dir = vec3(targetAngle.x, targetAngle.y) * offset;
                    to.pos  = target.pos - dir;
                    to.room = target.room;

                    traceClip(offset, to);
                }

                move(to);

                if (timer <= 0.0f)
                    resetTarget();

                mViewInv = mat4(eye.pos, target.pos, vec3(0, -1, 0));
            } else
                updateFirstPerson();
        }

        level->getSector(eye.room, eye.pos);

        if (Core::settings.detail.stereo == Core::Settings::STEREO_VR)
            updateListener(mViewInv * Input::hmd.head);
        else
            updateListener(mViewInv);

        smooth = true;
    }

    virtual void setup(bool calcMatrices) {
        if (calcMatrices) {
            Core::mViewInv = mViewInv;

            if (Core::settings.detail.stereo == Core::Settings::STEREO_VR)
                Core::mViewInv = Core::mViewInv * Input::hmd.eye[Core::eye == -1.0f ? 0 : 1];

            if (shake > 0.0f)
                Core::mViewInv.setPos(Core::mViewInv.getPos() + vec3(0.0f, sinf(shake * PI * 7) * shake * 48.0f, 0.0f));

            if (Core::settings.detail.stereo == Core::Settings::STEREO_ON)
                Core::mViewInv.setPos(Core::mViewInv.getPos() + Core::mViewInv.right().xyz() * (Core::eye * (firstPerson ? 8.0f : 32.0f) ));

            if (reflectPlane) {
                Core::mViewInv = mat4(*reflectPlane) * Core::mViewInv;
                Core::mViewInv.scale(vec3(1.0f, -1.0f, 1.0f));
            }

            Core::mView = Core::mViewInv.inverseOrtho();

            if (Core::settings.detail.stereo == Core::Settings::STEREO_VR)
                Core::mProj = Input::hmd.proj[Core::eye == -1.0f ? 0 : 1];
            else
                Core::mProj = GAPI::perspective(fov, aspect, znear, zfar);
        }

        Core::setViewProj(Core::mView, Core::mProj);
        Core::viewPos = Core::mViewInv.getPos();

        // update room for eye (with HMD offset)
        if (Core::settings.detail.stereo == Core::Settings::STEREO_VR)
            level->getSector(eye.room, Core::viewPos.xyz());

        frustum->pos = Core::viewPos.xyz();
        frustum->calcPlanes(Core::mViewProj);
    }

    void changeView(bool firstPerson) {
        this->firstPerson = firstPerson;

        if (firstPerson)
            smooth = false;

        fov   = firstPerson ? 90.0f : 65.0f;
        znear = firstPerson ? 16.0f : 32.0f;
        zfar  = 45.0f * 1024.0f;

        #ifdef _OS_PSP
            znear = 256.0f;
        #endif
    }
};

#endif
