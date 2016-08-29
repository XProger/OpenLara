#ifndef H_CAMERA
#define H_CAMERA

#include "core.h"

struct Camera {
    float fov, znear, zfar;
    vec3 pos, angle, offset;

    void update() {
    #ifdef FREE_CAMERA
        vec3 dir = vec3(sinf(angle.y - PI) * cosf(-angle.x), -sinf(-angle.x), cosf(angle.y - PI) * cosf(-angle.x));
        vec3 v = vec3(0);

        if (Input::down[ikW]) v = v + dir;
        if (Input::down[ikS]) v = v - dir;
        if (Input::down[ikD]) v = v + dir.cross(vec3(0, 1, 0));
        if (Input::down[ikA]) v = v - dir.cross(vec3(0, 1, 0));
        pos = pos + v.normal() * (Core::deltaTime * 2048.0f);
    #endif

        if (Input::down[ikMouseL]) {
            vec2 delta = Input::mouse.pos - Input::mouse.start.L;
            angle.x -= delta.y * 0.01f;
            angle.y -= delta.x * 0.01f;
            angle.x = min(max(angle.x, -PI * 0.5f + EPS), PI * 0.5f - EPS);
            Input::mouse.start.L = Input::mouse.pos;
        }
    }

    void setup() {
        Core::mView.identity();
        Core::mView.translate(vec3(-offset.x, -offset.y, -offset.z));
        Core::mView.rotateZ(-angle.z);
        Core::mView.rotateX(-angle.x);
        Core::mView.rotateY(-angle.y);
        Core::mView.translate(vec3(-pos.x, -pos.y, -pos.z));
        Core::mView.scale(vec3(-1, -1, 1));

        Core::mProj = mat4(fov, (float)Core::width / (float)Core::height, znear, zfar);

        Core::mViewProj = Core::mProj * Core::mView;

        Core::viewPos = Core::mView.inverse().getPos();
    }
};

#endif