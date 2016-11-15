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

    Camera(TR::Level *level, Lara *owner) : Controller(level, owner ? owner->entity : 0), owner(owner), frustum(new Frustum()), timer(0.0f), actTargetEntity(-1), actCamera(-1) {
        fov         = 80.0f;
        znear       = 128;
        zfar        = 100.0f * 1024.0f;
        angleAdv    = vec3(0.0f);
        
        if (owner) {
            room = owner->getEntity().room;
            pos = pos - owner->getDir() * 1024.0f;
            target = owner->getViewPoint();
        }
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
        actTargetEntity = owner->target;

        if (timer > 0.0f) {
            timer -= Core::deltaTime;
            if (timer <= 0.0f) {
                timer = 0.0f;
                if (room != getRoomIndex())
                    pos = lastDest;
                actTargetEntity = actCamera = -1;
            }
        }
    #ifdef FREE_CAMERA
        vec3 d = vec3(sinf(angle.y - PI) * cosf(-angle.x), -sinf(-angle.x), cosf(angle.y - PI) * cosf(-angle.x));
        vec3 v = vec3(0);

        if (Input::down[ikUp]) v = v + d;
        if (Input::down[ikDown]) v = v - d;
        if (Input::down[ikRight]) v = v + d.cross(vec3(0, 1, 0));
        if (Input::down[ikLeft]) v = v - d.cross(vec3(0, 1, 0));
        pos = pos + v.normal() * (Core::deltaTime * 2048.0f);
    #endif
        if (Input::down[ikMouseR]) {
            vec2 delta = Input::mouse.pos - Input::mouse.start.R;
            angleAdv.x -= delta.y * 0.01f;
            angleAdv.y += delta.x * 0.01f;
            Input::mouse.start.R = Input::mouse.pos;
        }

        angleAdv.x -= Input::joy.L.y * 2.0f * Core::deltaTime;
        angleAdv.y += Input::joy.L.x * 2.0f * Core::deltaTime;
 
        angle = owner->angle + angleAdv;
        angle.z = 0.0f;        
        //angle.x  = min(max(angle.x, -80 * DEG2RAD), 80 * DEG2RAD);

        float lerpFactor = (actTargetEntity == -1) ? 6.0f : 10.0f;
        vec3 dir;
        target = target.lerp(owner->getViewPoint(), lerpFactor * Core::deltaTime);

        if (actCamera > -1) {
            TR::Camera &c = level->cameras[actCamera];
            destPos = vec3(c.x, c.y, c.z);
            if (room != getRoomIndex()) 
                pos = destPos;
            if (actTargetEntity > -1) {
                TR::Entity &e = level->entities[actTargetEntity];
                target = vec3(e.x, e.y, e.z);
            }
        } else {
            if (actTargetEntity > -1) {
                TR::Entity &e = level->entities[actTargetEntity];
                dir = (vec3(e.x, e.y, e.z) - target).normal();
            } else
                dir = getDir();

            int destRoom;
            if ((owner->wpnState != Lara::Weapon::IS_HIDDEN || owner->state != Lara::STATE_BACK_JUMP) || actTargetEntity > -1) {
                vec3 eye = target - dir * CAMERA_OFFSET;
                destPos = trace(owner->getRoomIndex(), target, eye, destRoom, true);
                lastDest = destPos;
            } else {
                vec3 eye = lastDest + dir.cross(vec3(0, 1, 0)).normal() * 2048.0f - vec3(0.0f, 512.0f, 0.0f);
                destPos = trace(owner->getRoomIndex(), target, eye, destRoom, true);
            }
        }

        pos = pos.lerp(destPos, Core::deltaTime * lerpFactor);

        if (actCamera <= -1) {
            TR::Level::FloorInfo info;
            level->getFloorInfo(room, (int)pos.x, (int)pos.z, info);
        
            int lastRoom = room;

            if (info.roomNext != 255) 
                room = info.roomNext;
        
            if (pos.y < info.ceiling) {
                if (info.roomAbove != 255)
                    room = info.roomAbove;
                else
                    if (info.ceiling != 0xffff8100)
                        pos.y = info.ceiling;
            }

            if (pos.y > info.floor) {
                if (info.roomBelow != 255)
                    room = info.roomBelow;
                else
                    if (info.floor != 0xffff8100)
                        pos.y = info.floor;
            }

        // play underwater sound when camera goes under water
            if (lastRoom != room && !level->rooms[lastRoom].flags.water && level->rooms[room].flags.water)
                playSound(TR::SND_UNDERWATER, vec3(0.0f), 0);
        }

        mViewInv = mat4(pos, target, vec3(0, -1, 0));
        Sound::listener.matrix = mViewInv;
    }

    virtual void setup() {
        Core::mViewInv = mViewInv;
        Core::mView    = Core::mViewInv.inverse();
        Core::mProj    = mat4(fov, (float)Core::width / (float)Core::height, znear, zfar);

        Core::mViewProj = Core::mProj * Core::mView;        
        Core::viewPos   = Core::mViewInv.offset.xyz;

        frustum->pos = Core::viewPos;
        frustum->calcPlanes(Core::mViewProj);
    }
};

#endif