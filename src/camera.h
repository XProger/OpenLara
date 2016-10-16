#ifndef H_CAMERA
#define H_CAMERA

#include "core.h"
#include "controller.h"
#include "lara.h"

#define MAX_CLIP_PLANES 10

#define CAMERA_OFFSET (1024.0f + 256.0f)

struct Frustum {

    struct Poly {
        vec3    vertices[MAX_CLIP_PLANES];
        int     count;
    };

    vec3 pos;
    vec4 planes[MAX_CLIP_PLANES * 2];   // + buffer for OBB visibility test
    int  start, count;
#ifdef _DEBUG
    int dbg;
    Poly debugPoly;
#endif

    void calcPlanes(const mat4 &m) {
    #ifdef _DEBUG
        dbg = 0;
    #endif
        start = 0;
        count = 5;
        planes[0] = vec4(m.e30 - m.e20, m.e31 - m.e21, m.e32 - m.e22, m.e33 - m.e23); // near
        planes[1] = vec4(m.e30 - m.e10, m.e31 - m.e11, m.e32 - m.e12, m.e33 - m.e13); // top
        planes[2] = vec4(m.e30 - m.e00, m.e31 - m.e01, m.e32 - m.e02, m.e33 - m.e03); // right
        planes[3] = vec4(m.e30 + m.e10, m.e31 + m.e11, m.e32 + m.e12, m.e33 + m.e13); // bottom
        planes[4] = vec4(m.e30 + m.e00, m.e31 + m.e01, m.e32 + m.e02, m.e33 + m.e03); // left
        for (int i = 0; i < count; i++)
            planes[i] *= 1.0f / planes[i].xyz.length();
    }

    void calcPlanes(const Poly &poly) {
        count = 1 + poly.count; // add one for near plane (not changing)
        ASSERT(count < MAX_CLIP_PLANES);
        if (count < 4) return;

        vec3 e1 = poly.vertices[0] - pos;
        for (int i = 1; i < count; i++) {
            vec3 e2 = poly.vertices[i % poly.count] - pos;
            planes[i].xyz =  e1.cross(e2).normal();
            planes[i].w   = -(pos.dot(planes[i].xyz));
            e1 = e2;
        }
    }

    void clipPlane(const Poly &src, Poly &dst, const vec4 &plane) {
        dst.count = 0;
        if (!src.count) return;

        float t1 = src.vertices[0].dot(plane.xyz) + plane.w;

        for (int i = 0; i < src.count; i++) {
            const vec3 &v1 = src.vertices[i];
            const vec3 &v2 = src.vertices[(i + 1) % src.count];

            float t2 = v2.dot(plane.xyz) + plane.w;
        
        // hack for big float numbers
            int s1 = (int)t1;
            int s2 = (int)t2;

            if (s1 >= 0) {
                dst.vertices[dst.count++] = v1;
                ASSERT(dst.count < MAX_CLIP_PLANES);
            }
            
            if ((s1 ^ s2) < 0) { // check for opposite signs
                float k1 = t2 / (t2 - t1);
                float k2 = t1 / (t2 - t1);
                dst.vertices[dst.count++] = v1 * (float)k1 - v2 * (float)k2;
                ASSERT(dst.count < MAX_CLIP_PLANES);
            }

            t1 = t2;
        }
    }

    bool clipByPortal(const vec3 *vertices, int vCount, const vec3 &normal) {
        if (normal.dot(pos - vertices[0]) < 0.0f) // check portal winding order
            return false;

        Poly poly[2];

        poly[0].count = vCount;
        memmove(poly[0].vertices, vertices, sizeof(vec3) * poly[0].count);
#ifdef _DEBUG
        debugPoly.count = 0;
#endif
        int j = 0;
        for (int i = 1; i < count; i++, j ^= 1)
            clipPlane(poly[j], poly[j ^ 1], planes[i]);

        calcPlanes(poly[j]);
        return count >= 4;
    }

    // AABB visibility check
    bool isVisible(const vec3 &min, const vec3 &max) const {
        if (count < 4) return false;

        for (int i = start; i < start + count; i++) {
            const vec3 &n =  planes[i].xyz;
            const float d = -planes[i].w;

            if (n.dot(max) < d && 
                n.dot(min) < d &&
                n.dot(vec3(min.x, max.y, max.z)) < d &&
                n.dot(vec3(max.x, min.y, max.z)) < d &&
                n.dot(vec3(min.x, min.y, max.z)) < d &&
                n.dot(vec3(max.x, max.y, min.z)) < d &&
                n.dot(vec3(min.x, max.y, min.z)) < d &&
                n.dot(vec3(max.x, min.y, min.z)) < d)
                return false;
        }
        return true;
    }

    // OBB visibility check
    bool isVisible(const mat4 &matrix, const vec3 &min, const vec3 &max) {
        start = count;
        // transform clip planes (relative)
        mat4 m = matrix.inverse();
        for (int i = 0; i < count; i++) {
            vec4 &p = planes[i];
            vec4 o = m * vec4(p.xyz * (-p.w), 1.0f);
            vec4 n = m * vec4(p.xyz, 0.0f);
            planes[start + i] = vec4(n.xyz, -n.xyz.dot(o.xyz));
        }
        bool visible = isVisible(min, max);
        start = 0;
        return visible;
    }

    // Sphere visibility check
    bool isVisible(const vec3 &center, float radius) {
        if (count < 4) return false;

        for (int i = 0; i < count; i++)
            if (planes[i].xyz.dot(center) + planes[i].w < -radius)
                return false;
        return true;
    }

};


struct Camera : Controller {
    Lara    *owner;
    Frustum *frustum;

    float   fov, znear, zfar;
    vec3    target, destPos, lastDest, angleAdv;
    int     room;

    float   timer;
    int     actTargetEntity, actCamera;

    Camera(TR::Level *level, Lara *owner) : Controller(level, owner ? owner->entity : 0), owner(owner), frustum(new Frustum()), timer(0.0f), actTargetEntity(-1), actCamera(-1) {
        fov         = 75.0f;
        znear       = 128;
        zfar        = 100.0f * 1024.0f;
        angleAdv    = vec3(0.0f);
        
        if (owner) {
            room = owner->getEntity().room;
            pos = pos - owner->getDir() * 1024.0f;
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

        vec3 dir;
        target = vec3(owner->pos.x, owner->pos.y, owner->pos.z) + owner->getViewOffset();

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

            if (owner->state != Lara::STATE_BACK_JUMP || actTargetEntity > -1) {
                vec3 eye = target - dir * CAMERA_OFFSET;
                destPos = trace(owner->getRoomIndex(), target, eye);
                lastDest = destPos;
            } else {
                vec3 eye = lastDest + dir.cross(vec3(0, 1, 0)).normal() * 2048.0f - vec3(0.0f, 512.0f, 0.0f);
                destPos = trace(owner->getRoomIndex(), target, eye);
            }
        }

        float lerpFactor = (actTargetEntity == -1) ? 2.0f : 10.0f;

        pos = pos.lerp(destPos, Core::deltaTime * lerpFactor);

        if (actCamera > -1) return;

        TR::Level::FloorInfo info;
        level->getFloorInfo(room, (int)pos.x, (int)pos.z, info);
        
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
    }

    vec3 trace(int fromRoom, const vec3 &from, const vec3 &to) { // TODO: use Bresenham
        int room = fromRoom;

        vec3 pos = from, dir = to - from;
        int px = (int)pos.x, py = (int)pos.y, pz = (int)pos.z;

        float dist = dir.length();
        dir = dir * (1.0f / dist);

        int lr = -1, lx = -1, lz = -1;
        TR::Level::FloorInfo info;
        while (dist > 1.0f) {
            int sx = px / 1024 * 1024 + 512,
                sz = pz / 1024 * 1024 + 512;

            if (lr != room || lx != sx || lz != sz) {
                level->getFloorInfo(room, sx, sz, info);
                if (info.roomNext != 0xFF) {
                    room = info.roomNext;
                    level->getFloorInfo(room, sx, sz, info);
                }
                lr = room;
                lx = sx;
                lz = sz;
            }

            if (py > info.floor && info.roomBelow != 0xFF)
                room = info.roomBelow;
            else if (py < info.ceiling && info.roomAbove != 0xFF)
                room = info.roomAbove;
            else if (py > info.floor || py < info.ceiling) {
                int minX = px / 1024 * 1024;
                int minZ = pz / 1024 * 1024;
                int maxX = minX + 1024;
                int maxZ = minZ + 1024;

                pos = vec3(clamp(px, minX, maxX), pos.y, clamp(pz, minZ, maxZ)) + boxNormal(px, pz) * 256.0f;
                dir = (pos - from).normal();
            }

            float d = min(dist, 128.0f);    // STEP = 128
            dist -= d;
            pos = pos + dir * d;

            px = (int)pos.x, py = (int)pos.y, pz = (int)pos.z;
        }

        return pos;
    }

    vec3 boxNormal(int x, int z) {
        x %= 1024;
        z %= 1024;

        if (x > 1024 - z)
            return x < z ? vec3(0, 0, 1)  : vec3(1, 0, 0);
        else
            return x < z ? vec3(-1, 0, 0) : vec3(0, 0, -1);
    }

    virtual void setup() {
        Core::mViewInv = mat4(pos, target, vec3(0, -1, 0));
        Core::mView    = Core::mViewInv.inverse();
        Core::mProj    = mat4(fov, (float)Core::width / (float)Core::height, znear, zfar);

        Core::mViewProj = Core::mProj * Core::mView;        
        Core::viewPos   = Core::mViewInv.offset.xyz;

        frustum->pos = Core::viewPos;
        frustum->calcPlanes(Core::mViewProj);      
    }
};

#endif