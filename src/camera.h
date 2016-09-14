#ifndef H_CAMERA
#define H_CAMERA

#include "core.h"
#include "controller.h"
#include "lara.h"

#define MAX_CLIP_PLANES 10

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
            int s1 = sign((int)t1);
            int s2 = sign((int)t2);

            if (s1 >= 0) {
                dst.vertices[dst.count++] = v1;
                ASSERT(dst.count < MAX_CLIP_PLANES);
            }
            
            if (s1 * s2 < 0) {
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
    Controller  *owner;
    Frustum     *frustum;

    float fov, znear, zfar;
    vec3 target, destPos, lastDest, angleAdv;

    int room;

    Camera(TR::Level *level, Controller *owner) : Controller(level, owner->entity), owner(owner), frustum(new Frustum()) {
        fov         = 75.0f;
        znear       = 0.1f * 2048.0f;
        zfar        = 1000.0f * 2048.0f;
        angleAdv    = vec3(0.0f);
        
        room = owner->getEntity().room;
        pos = pos - owner->getDir() * 1024.0f;
    }

    ~Camera() {
        delete frustum;
    }

    virtual int getRoomIndex() const {
        return room;
    }
    
    virtual void update() {
    #ifdef FREE_CAMERA
        vec3 dir = vec3(sinf(angle.y - PI) * cosf(-angle.x), -sinf(-angle.x), cosf(angle.y - PI) * cosf(-angle.x));
        vec3 v = vec3(0);

        if (Input::down[ikW]) v = v + dir;
        if (Input::down[ikS]) v = v - dir;
        if (Input::down[ikD]) v = v + dir.cross(vec3(0, 1, 0));
        if (Input::down[ikA]) v = v - dir.cross(vec3(0, 1, 0));
        pos = pos + v.normal() * (Core::deltaTime * 2048.0f);
    #endif
        if (Input::down[ikMouseR]) {
            vec2 delta = Input::mouse.pos - Input::mouse.start.R;
            angleAdv.x -= delta.y * 0.01f;
            angleAdv.y += delta.x * 0.01f;
            Input::mouse.start.R = Input::mouse.pos;
        }
 
        float height = 0.0f;
        switch (owner->stand) {
            case Controller::STAND_AIR    :
            case Controller::STAND_GROUND :
                height = 768.0f;
                break;
            case Controller::STAND_UNDERWATER :
            case Controller::STAND_ONWATER    :
                height = 256.0f;
                break;
        }

        angle = owner->angle + angleAdv;
        angle.z = 0.0f;        
        //angle.x  = min(max(angle.x, -80 * DEG2RAD), 80 * DEG2RAD);

        target = vec3(owner->pos.x, owner->pos.y - height, owner->pos.z);
        
        vec3 dir = getDir();

        if (owner->state != Lara::STATE_BACK_JUMP) {
            destPos = target - dir * 1024.0f;
            lastDest = destPos;
        } else
            destPos = lastDest + dir.cross(vec3(0, 1, 0)).normal() * 2048.0f - vec3(0.0f, 512.0f, 0.0f);       

        pos = pos.lerp(destPos, min(1.0f, Core::deltaTime * 2.0f));

        FloorInfo info = getFloorInfo((int)pos.x, (int)pos.z);
        
        if (info.roomNext != 255) 
            room = info.roomNext;
        
        if (destPos.y < info.ceiling) {
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

    void setup() {
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