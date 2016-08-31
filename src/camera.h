#ifndef H_CAMERA
#define H_CAMERA

#include "core.h"

#define MAX_CLIP_PLANES 16

struct Camera {

    struct Frustum {

        struct Poly {
            vec3    vertices[MAX_CLIP_PLANES];
            int     count;
        };

        vec3 pos;
        vec4 planes[MAX_CLIP_PLANES];
        int  count;

        void calcPlanes(const mat4 &m) {
            count = 4;
            planes[0] = vec4(m.e30 - m.e00, m.e31 - m.e01, m.e32 - m.e02, m.e33 - m.e03); // right
            planes[1] = vec4(m.e30 + m.e00, m.e31 + m.e01, m.e32 + m.e02, m.e33 + m.e03); // left
            planes[2] = vec4(m.e30 - m.e10, m.e31 - m.e11, m.e32 - m.e12, m.e33 - m.e13); // top
            planes[3] = vec4(m.e30 + m.e10, m.e31 + m.e11, m.e32 + m.e12, m.e33 + m.e13); // bottom
            for (int i = 0; i < count; i++)
                planes[i] *= 1.0f / planes[i].xyz.length();
        }

        void calcPlanes(const Poly &poly) {
            count = poly.count;
            ASSERT(count < MAX_CLIP_PLANES);
            if (!count) return;

            vec3 e1 = poly.vertices[0] - pos;
            for (int i = 0; i < count; i++) {
                vec3 e2 = poly.vertices[(i + 1) % count] - pos;
                planes[i].xyz = e1.cross(e2).normal();
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

                if (t1 >= 0.0f) {
                    dst.vertices[dst.count++] = v1;
                    ASSERT(dst.count < MAX_CLIP_PLANES);
                }
            
                if (t1 * t2 < 0.0f) {
                    float k1 = t2 / (t2 - t1);
                    float k2 = t1 / (t2 - t1);
                    dst.vertices[dst.count++] = v1 * k1 - v2 * k2;
                    ASSERT(dst.count < MAX_CLIP_PLANES);
                }

                t1 = t2;
            }
        }

        bool clipByPortal(const vec3 *vertices, const vec3 &normal) { // 4 vertices
            if (normal.dot(pos - vertices[0]) < 0.0f) // check portal winding order
                return false;

            Poly poly[2];

            poly[0].count = 4;
            memmove(poly[0].vertices, vertices, sizeof(vec3) * poly[0].count);

            int j = 0;
            for (int i = 0; i < count; i++, j ^= 1)
                clipPlane(poly[j], poly[j ^ 1], planes[i]);

            if (poly[j].count < 3)
                return false;

            calcPlanes(poly[j]);
            return true;
        }

        bool isVisible(const vec3 &min, const vec3 &max) const {
            if (count < 3) return false;

            for (int i = 0; i < count; i++) {
                const vec3 &n = planes[i].xyz;
                const float d = -planes[i].w;

                if (n.dot(max) < d &&
                    n.dot(vec3(min.x, max.y, max.z)) < d &&
                    n.dot(vec3(max.x, min.y, max.z)) < d &&
                    n.dot(vec3(min.x, min.y, max.z)) < d &&
                    n.dot(vec3(max.x, max.y, min.z)) < d &&
                    n.dot(vec3(min.x, max.y, min.z)) < d &&
                    n.dot(vec3(max.x, min.y, min.z)) < d &&
                    n.dot(min) < d)
                    return false;
            }
            return true;
        }

        bool isVisible(const vec3 &center, float radius) {
            if (count < 3) return false;

            for (int i = 0; i < count; i++)
                if (planes[i].xyz.dot(center) + planes[i].w < -radius)
                    return false;
            return true;
        }

    } *frustum;

    float fov, znear, zfar;
    vec3 pos, angle, offset;

    Camera() : frustum(new Frustum()) {}

    ~Camera() {
        delete frustum;
    }

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

        frustum->pos = Core::viewPos;
        frustum->calcPlanes(Core::mViewProj);
    }
};

#endif