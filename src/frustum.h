#ifndef H_FRUSTUM
#define H_FRUSTUM

#include "utils.h"

#define MAX_CLIP_PLANES 16

struct Frustum {
    vec3 pos;
    vec4 planes[MAX_CLIP_PLANES * 2];   // + buffer for OBB visibility test
    int  start, count;

    void calcPlanes(const mat4 &m) {
        start = 0;
        count = 5;
        planes[0] = vec4(m.e30 - m.e20, m.e31 - m.e21, m.e32 - m.e22, m.e33 - m.e23); // near
        planes[1] = vec4(m.e30 - m.e10, m.e31 - m.e11, m.e32 - m.e12, m.e33 - m.e13); // top
        planes[2] = vec4(m.e30 - m.e00, m.e31 - m.e01, m.e32 - m.e02, m.e33 - m.e03); // right
        planes[3] = vec4(m.e30 + m.e10, m.e31 + m.e11, m.e32 + m.e12, m.e33 + m.e13); // bottom
        planes[4] = vec4(m.e30 + m.e00, m.e31 + m.e01, m.e32 + m.e02, m.e33 + m.e03); // left
        for (int i = 0; i < count; i++)
            planes[i] *= 1.0f / planes[i].xyz().length();
    }

    // AABB visibility check
    bool isVisible(const vec3 &min, const vec3 &max) const {
        if (count < 4) return false;

        for (int i = start; i < start + count; i++) {
            const vec3 &n =  planes[i].xyz();
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
        mat4 m = matrix.inverseOrtho();
        for (int i = 0; i < count; i++) {
            vec4 &p = planes[i];
            vec4 o = m * vec4(p.xyz() * (-p.w), 1.0f);
            vec4 n = m * vec4(p.xyz(), 0.0f);
            planes[start + i] = vec4(n.xyz(), -n.xyz().dot(o.xyz()));
        }
        bool visible = isVisible(min, max);
        start = 0;
        return visible;
    }

    // Sphere visibility check
    bool isVisible(const vec3 &center, float radius) {
        if (count < 4) return false;

        for (int i = 0; i < count; i++)
            if (planes[i].xyz().dot(center) + planes[i].w < -radius)
                return false;
        return true;
    }

};

#endif