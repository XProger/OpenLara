#ifndef H_CAMERA
#define H_CAMERA

#include "core.h"
#include "frustum.h"
#include "controller.h"
#include "lara.h"

#define CAMERA_OFFSET (1024.0f + 256.0f)

struct Camera : Controller {
    Lara    *owner;
    Frustum *frustum;

    float   fov, znear, zfar;
    vec3    target, destPos, lastDest, angleAdv;
    mat4    mViewInv;
    int     room;

    float   timer;
    int     actTargetEntity, actCamera;
    bool    cutscene;

    Camera(TR::Level *level, Lara *owner) : Controller(level, owner ? owner->entity : 0), owner(owner), frustum(new Frustum()), timer(0.0f), actTargetEntity(-1), actCamera(-1) {
        fov         = 65.0f;
        znear       = 16;
        zfar        = 32.0f * 1024.0f;
        angleAdv    = vec3(0.0f);
        
        if (owner) {
            room = owner->getEntity().room;
            pos = pos - owner->getDir() * 1024.0f;
            target = owner->getViewPoint();
        }

        cutscene = owner->getEntity().type != TR::Entity::LARA && level->cameraFrames;
    }

    virtual ~Camera() {
        delete frustum;
    }
    
    virtual int getRoomIndex() const {
        return actCamera > -1 ? level->cameras[actCamera].room : room;
    }

    virtual bool activate(ActionCommand *cmd) {
        Controller::activate(cmd);
        if (cmd->timer)
            this->timer = cmd->timer;
        if (cmd->action == TR::Action::CAMERA_TARGET)
            actTargetEntity = cmd->value;
        if (cmd->action == TR::Action::CAMERA_SWITCH) {
            actCamera = cmd->value;
            lastDest = pos;
        }
        activateNext();
        return true;
    }

    virtual void update() {
    #ifndef LEVEL_EDITOR
        if (cutscene) { // cutscene
            timer += Core::deltaTime * 30;
            float t = timer - int(timer);
            int indexA = int(timer) % level->cameraFramesCount;
            int indexB = min(indexA + 1, level->cameraFramesCount - 1);
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

            // TODO: frame->roll
        } else
    #endif        
        {
            if (Input::down[ikMouseR]) {
                vec2 delta = Input::mouse.pos - Input::mouse.start.R;
                angleAdv.x -= delta.y * 0.01f;
                angleAdv.y += delta.x * 0.01f;
                Input::mouse.start.R = Input::mouse.pos;
            }

            angleAdv.x -= Input::joy.R.y * 2.0f * Core::deltaTime;
            angleAdv.y += Input::joy.R.x * 2.0f * Core::deltaTime;
 
            angle = owner->angle + angleAdv;
            angle.z = 0.0f;        

        #ifdef LEVEL_EDITOR
            angle   = angleAdv;
            angle.x = min(max(angle.x, -80 * DEG2RAD), 80 * DEG2RAD);

            vec3 d = vec3(sinf(angle.y) * cosf(angle.x), -sinf(angle.x), cosf(angle.y) * cosf(angle.x));
            vec3 v = vec3(0);

            if (Input::down[ikW]) v = v + d;
            if (Input::down[ikS]) v = v - d;
            if (Input::down[ikA]) v = v + d.cross(vec3(0, 1, 0));
            if (Input::down[ikD]) v = v - d.cross(vec3(0, 1, 0));
            pos = pos + v.normal() * (Core::deltaTime * 512.0 * 10.0f);

            mViewInv.identity();
            mViewInv.translate(pos);
            mViewInv.rotateY(angle.y - PI);
            mViewInv.rotateX(-angle.x);
            mViewInv.rotateZ(PI);

            Sound::listener.matrix = mViewInv;

            return;
        #endif

            int lookAt = -1;
            if (actTargetEntity > -1)   lookAt = actTargetEntity;
            if (owner->target > -1)     lookAt = owner->target;

            owner->viewTarget = lookAt;

            if (timer > 0.0f) {
                timer -= Core::deltaTime;
                if (timer <= 0.0f) {
                    timer = 0.0f;
                    if (room != getRoomIndex())
                        pos = lastDest;
                    actTargetEntity = actCamera = -1;
                    target = owner->getViewPoint();
                }
            }

            float lerpFactor = (lookAt == -1) ? 6.0f : 10.0f;
            vec3 dir;
            target = target.lerp(owner->getViewPoint(), lerpFactor * Core::deltaTime);

            if (actCamera > -1) {
                TR::Camera &c = level->cameras[actCamera];
                destPos = vec3(float(c.x), float(c.y), float(c.z));
                if (room != getRoomIndex()) 
                    pos = destPos;
                if (lookAt > -1) {
                    TR::Entity &e = level->entities[lookAt];
                    target = ((Controller*)e.controller)->pos;
                }
            } else {
                if (lookAt > -1) {
                    TR::Entity &e = level->entities[lookAt];
                    dir = (((Controller*)e.controller)->pos - target).normal();
                } else
                    dir = getDir();

                int destRoom;
                if ((!owner->emptyHands() || owner->state != Lara::STATE_BACK_JUMP) || lookAt > -1) {
                    vec3 eye = target - dir * CAMERA_OFFSET;
                    destPos = trace(owner->getRoomIndex(), target, eye, destRoom, true);
                    lastDest = destPos;
                } else {
                    vec3 eye = lastDest + dir.cross(vec3(0, 1, 0)).normal() * 2048.0f - vec3(0.0f, 512.0f, 0.0f);
                    destPos = trace(owner->getRoomIndex(), target, eye, destRoom, true);
                }
                room = destRoom;
            }
            pos = pos.lerp(destPos, Core::deltaTime * lerpFactor);

            if (actCamera <= -1) {
                TR::Level::FloorInfo info;
                level->getFloorInfo(room, (int)pos.x, (int)pos.y, (int)pos.z, info);

                if (info.roomNext != 255) 
                    room = info.roomNext;
        
                if (pos.y < info.roomCeiling) {
                    if (info.roomAbove != 255)
                        room = info.roomAbove;
                    else
                        if (info.roomCeiling != 0xffff8100)
                            pos.y = (float)info.roomCeiling;
                }

                if (pos.y > info.roomFloor) {
                    if (info.roomBelow != 255)
                        room = info.roomBelow;
                    else
                        if (info.roomFloor != 0xffff8100)
                            pos.y = (float)info.roomFloor;
                }
            }
        }

        mViewInv = mat4(pos, target, vec3(0, -1, 0));
        Sound::listener.matrix = mViewInv;
    }

    virtual void setup(bool calcMatrices) {
        if (calcMatrices) {
            Core::mViewInv = mViewInv;
            Core::mView    = Core::mViewInv.inverse();
            Core::mProj    = mat4(fov, (float)Core::width / (float)Core::height, znear, zfar);
        // TODO: camera shake
        // TODO: temporal anti-aliasing
        //    Core::mProj.e02 = (randf() - 0.5f) * 32.0f / Core::width;
        //    Core::mProj.e12 = (randf() - 0.5f) * 32.0f / Core::height;
        }
        Core::mViewProj = Core::mProj * Core::mView;        
        Core::viewPos   = Core::mViewInv.offset.xyz;

        frustum->pos = Core::viewPos;
        frustum->calcPlanes(Core::mViewProj);
    }
};

#endif