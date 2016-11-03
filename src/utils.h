#ifndef H_UTILS
#define H_UTILS

#include <stdlib.h>
#include <cstring>
#include <math.h>
#include <float.h>

#ifdef _DEBUG
    #define debugBreak() _asm { int 3 }
    #define ASSERT(expr) if (expr) {} else { LOG("ASSERT %s in %s:%d\n", #expr, __FILE__, __LINE__); debugBreak(); }

    #ifndef ANDROID
        #define LOG(...) printf(__VA_ARGS__)
    #else
        #include <android/log.h>
        #define LOG(...) __android_log_print(ANDROID_LOG_INFO,"X5",__VA_ARGS__)
    #endif

#else
    #define ASSERT(expr)
    #define LOG(...)
//    #define LOG(...) printf(__VA_ARGS__)
#endif


#define PI      3.14159265358979323846f
#define PI2     (PI * 2.0f)
#define DEG2RAD (PI / 180.0f)
#define RAD2DEG (180.0f / PI)
#define EPS     FLT_EPSILON

typedef char            int8;
typedef short           int16;
typedef int             int32;
typedef unsigned char   uint8;
typedef unsigned short  uint16;
typedef unsigned int    uint32;

#define FOURCC(str)     (*((uint32*)str))

struct ubyte4 {
    uint8 x, y, z, w;
};

struct short2 {
    int16 x, y;
};

struct short3 {
    int16 x, y, z;
};

struct short4 {
    int16 x, y, z, w;
};

template <typename T>
inline const T& min(const T &a, const T &b) {
    return a < b ? a : b;
}

template <class T>
inline const T& max(const T &a, const T &b) {
    return a > b ? a : b;
}

template <class T>
inline const T& clamp(const T &x, const T &a, const T &b) {
    return x < a ? a : (x > b ? b : x);
}

template <class T>
inline const int sign(const T &x) {
    return x > 0 ? 1 : (x < 0 ? -1 : 0);
}

float clampAngle(float a) {
    return a < -PI ? a + PI2 : (a >= PI ? a - PI2 : a);
}

float shortAngle(float a, float b) {
    float n = clampAngle(b) - clampAngle(a);
    return clampAngle(n - int(n / PI2) * PI2);
}


struct vec2 {
    float x, y;
    vec2() {}
    vec2(float s) : x(s), y(s) {}
    vec2(float x, float y) : x(x), y(y) {}

    vec2 operator + (const vec2 &v) const { return vec2(x+v.x, y+v.y); }
    vec2 operator - (const vec2 &v) const { return vec2(x-v.x, y-v.y); }
    vec2 operator * (float s) const { return vec2(x*s, y*s); }
    float dot(const vec2 &v) const { return x*v.x + y*v.y; }
    float cross(const vec2 &v) const { return x*v.y - y*v.x; }
    float length2() const { return dot(*this); }
    float length()  const { return sqrtf(length2()); }
    vec2  normal()  const { float s = length(); return s == 0.0 ? (*this) : (*this)*(1.0f/s); }
};

struct vec3 {
    float x, y, z;
    vec3() {}
    vec3(float s) : x(s), y(s), z(s) {}
    vec3(float x, float y, float z) : x(x), y(y), z(z) {}
    vec3(const vec2 &xy, float z = 0.0f) : x(xy.x), y(xy.y), z(z) {}
    vec3(float lng, float lat) : x(sinf(lat) * cosf(lng)), y(-sinf(lng)), z(cosf(lat) * cosf(lng)) {}

    float& operator [] (int index) const { return ((float*)this)[index]; }

    vec3& operator += (const vec3 &v) { x += v.x; y += v.y; z += v.z; return *this; }
    vec3& operator -= (const vec3 &v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
    vec3& operator *= (const vec3 &v) { x *= v.x; y *= v.y; z *= v.z; return *this; }
    vec3& operator *= (float s)       { x *= s;   y *= s;   z *= s;   return *this; }

    vec3 operator + (const vec3 &v) const { return vec3(x+v.x, y+v.y, z+v.z); }
    vec3 operator - (const vec3 &v) const { return vec3(x-v.x, y-v.y, z-v.z); }
    vec3 operator * (const vec3 &v) const { return vec3(x*v.x, y*v.y, z*v.z); }
    vec3 operator * (float s) const { return vec3(x*s, y*s, z*s); }

    float dot(const vec3 &v) const { return x*v.x + y*v.y + z*v.z; }
    vec3  cross(const vec3 &v) const { return vec3(y*v.z - z*v.y, z*v.x - x*v.z, x*v.y - y*v.x); }
    float length2() const { return dot(*this); }
    float length()  const { return sqrtf(length2()); }
    vec3  normal()  const { float s = length(); return s == 0.0f ? (*this) : (*this)*(1.0f/s); }

    vec3 lerp(const vec3 &v, const float t) const {
        if (t <= 0.0f) return *this;
        if (t >= 1.0f) return v;
        return *this + (v - *this) * t; 
    }

    vec3 rotateY(float angle) const {
        float s = sinf(angle), c = cosf(angle); 
        return vec3(x*c - z*s, y, x*s + z*c);
    }
};

struct vec4 {
    union {
        struct { vec3 xyz; };
        struct { float x, y, z, w; };
    };

    vec4() {}
    vec4(float s) : x(s), y(s), z(s), w(s) {}
    vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    vec4(const vec3 &xyz, float w) : x(xyz.x), y(xyz.y), z(xyz.z), w(w) {}

    vec4& operator *= (const vec4 &v) { x*=v.x; y*=v.y; z*=v.z; w*=v.w; return *this; }
};

struct quat {
    float x, y, z, w;

    quat() {}
    quat(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

    quat(const vec3 &axis, float angle) {
        angle *= 0.5f;
        float s = sinf(angle);
        x = axis.x * s;
        y = axis.y * s;
        z = axis.z * s;
        w = cosf(angle);
    }

    quat operator - () const {
        return quat(-x, -y, -z, -w);
    }

    quat operator + (const quat &q) const { 
        return quat(x + q.x, y + q.y, z + q.z, w + q.w); 
    }

    quat operator - (const quat &q) const { 
        return quat(x - q.x, y - q.y, z - q.z, w - q.w); 
    }

    quat operator * (const float s) const { 
        return quat(x * s, y * s, z * s, w * s); 
    }

    quat operator * (const quat &q) const {
        return quat(w * q.x + x * q.w + y * q.z - z * q.y,
                    w * q.y + y * q.w + z * q.x - x * q.z,
                    w * q.z + z * q.w + x * q.y - y * q.x,
                    w * q.w - x * q.x - y * q.y - z * q.z);
    }

    float dot(const quat &q) const {
        return x * q.x + y * q.y + z * q.z + w * q.w;
    }

    float length2() const { 
        return dot(*this); 
    }

    float length() const { 
        return sqrtf(length2());
    }

    void normalize() {
        *this = normal();
    }

    quat normal() const {
        return *this * (1.0f / length());
    }

    quat conjugate() const {
        return quat(-x, -y, -z, w);
    }

    quat inverse() const {
        return conjugate() * (1.0f / length2());
    }

    quat lerp(const quat &q, float t) const {
        if (t <= 0.0f) return *this;
        if (t >= 1.0f) return q;

        return dot(q) < 0 ? (*this - (q + *this) * t) : 
                            (*this + (q - *this) * t);
    }

    quat slerp(const quat &q, float t) const {
        if (t <= 0.0f) return *this;
        if (t >= 1.0f) return q;

        quat temp;
        float omega, cosom, sinom, scale0, scale1;

        cosom = dot(q);
        if (cosom < 0.0f) {
            temp = -q;
            cosom = -cosom;
        } else
            temp = q;

        if (1.0f - cosom > EPS) {
            omega = acosf(cosom);
            sinom = 1.0f / sinf(omega);
            scale0 = sinf((1.0f - t) * omega) * sinom;
            scale1 = sinf(t * omega) * sinom;
        } else {
            scale0 = 1.0f - t;
            scale1 = t;
        }

        return *this * scale0 + temp * scale1;
    }
};

struct mat4 {

    union {
        struct {
            float e00, e10, e20, e30,
                  e01, e11, e21, e31,
                  e02, e12, e22, e32,
                  e03, e13, e23, e33;
        };
        struct { vec4 right, up, dir, offset; };
    };

    mat4() {}

    mat4(const quat &rot, const vec3 &pos) {
        setRot(rot);
        setPos(pos);
        e30 = e31 = e32 = 0.0f;
        e33 = 1.0f;
    }

    mat4(float l, float r, float b, float t, float znear, float zfar) {
        identity();
        e00 = 2.0f / (r - l);
        e11 = 2.0f / (t - b);
        e22 = 2.0f / (znear - zfar);
        e03 = (l + r) / (l - r);
        e13 = (b + t) / (b - t);
        e23 = (zfar + znear) / (znear - zfar);
    }

    mat4(float fov, float aspect, float znear, float zfar) {
        float k = 1.0f / tanf(fov * 0.5f * DEG2RAD);
        identity();
        e00 = k / aspect;
        e11 = k;
        e22 = (znear + zfar) / (znear - zfar);
        e33 = 0.0f;
        e32 = -1.0f;
        e23 = 2.0f * zfar * znear / (znear - zfar);
    }

    mat4(const vec3 &from, const vec3 &at, const vec3 &up) {
        vec3 r, u, d;
        d = (from - at).normal();
        r = up.cross(d).normal();
        u = d.cross(r);

        this->right     = vec4(r, 0.0f);
        this->up        = vec4(u, 0.0f);
        this->dir       = vec4(d, 0.0f);
        this->offset    = vec4(from, 1.0f);
    }

    void identity() {
        e10 = e20 = e30 = e01 = e21 = e31 = e02 = e12 = e32 = e03 = e13 = e23 = 0.0f;
        e00 = e11 = e22 = e33 = 1.0f;
    }

    mat4 operator * (const mat4 &m) const {
        mat4 r;
        r.e00 = e00 * m.e00 + e01 * m.e10 + e02 * m.e20 + e03 * m.e30;
        r.e10 = e10 * m.e00 + e11 * m.e10 + e12 * m.e20 + e13 * m.e30;
        r.e20 = e20 * m.e00 + e21 * m.e10 + e22 * m.e20 + e23 * m.e30;
        r.e30 = e30 * m.e00 + e31 * m.e10 + e32 * m.e20 + e33 * m.e30;
        r.e01 = e00 * m.e01 + e01 * m.e11 + e02 * m.e21 + e03 * m.e31;
        r.e11 = e10 * m.e01 + e11 * m.e11 + e12 * m.e21 + e13 * m.e31;
        r.e21 = e20 * m.e01 + e21 * m.e11 + e22 * m.e21 + e23 * m.e31;
        r.e31 = e30 * m.e01 + e31 * m.e11 + e32 * m.e21 + e33 * m.e31;
        r.e02 = e00 * m.e02 + e01 * m.e12 + e02 * m.e22 + e03 * m.e32;
        r.e12 = e10 * m.e02 + e11 * m.e12 + e12 * m.e22 + e13 * m.e32;
        r.e22 = e20 * m.e02 + e21 * m.e12 + e22 * m.e22 + e23 * m.e32;
        r.e32 = e30 * m.e02 + e31 * m.e12 + e32 * m.e22 + e33 * m.e32;
        r.e03 = e00 * m.e03 + e01 * m.e13 + e02 * m.e23 + e03 * m.e33;
        r.e13 = e10 * m.e03 + e11 * m.e13 + e12 * m.e23 + e13 * m.e33;
        r.e23 = e20 * m.e03 + e21 * m.e13 + e22 * m.e23 + e23 * m.e33;
        r.e33 = e30 * m.e03 + e31 * m.e13 + e32 * m.e23 + e33 * m.e33;
        return r;
    }

    vec3 operator * (const vec3 &v) const {
        return vec3(
            e00 * v.x + e01 * v.y + e02 * v.z + e03,
            e10 * v.x + e11 * v.y + e12 * v.z + e13,
            e20 * v.x + e21 * v.y + e22 * v.z + e23);
    }
    
    vec4 operator * (const vec4 &v) const {
        return vec4(
            e00 * v.x + e01 * v.y + e02 * v.z + e03 * v.w,
            e10 * v.x + e11 * v.y + e12 * v.z + e13 * v.w,
            e20 * v.x + e21 * v.y + e22 * v.z + e23 * v.w,
            e30 * v.x + e31 * v.y + e32 * v.z + e33 * v.w);
    }

    void translate(const vec3 &offset) {
        mat4 m;
        m.identity();
        m.setPos(offset);
        *this = *this * m;
    };

    void scale(const vec3 &factor) {
        mat4 m;
        m.identity();
        m.e00 = factor.x;
        m.e11 = factor.y;
        m.e22 = factor.z;
        *this = *this * m;
    }

    void rotateX(float angle) {
        mat4 m;
        m.identity();
        float s = sinf(angle), c = cosf(angle);
        m.e11 = c;  m.e21 = s;
        m.e12 = -s; m.e22 = c;
        *this = *this * m;
    }

    void rotateY(float angle) {
        mat4 m;
        m.identity();
        float s = sinf(angle), c = cosf(angle);
        m.e00 = c;  m.e20 = -s;
        m.e02 = s;  m.e22 = c;
        *this = *this * m;
    }

    void rotateZ(float angle) {
        mat4 m;
        m.identity();
        float s = sinf(angle), c = cosf(angle);
        m.e00 = c;  m.e01 = -s;
        m.e10 = s;  m.e11 = c;
        *this = *this * m;
    }

    float det() const {
        return  e00 * (e11 * (e22 * e33 - e32 * e23) - e21 * (e12 * e33 - e32 * e13) + e31 * (e12 * e23 - e22 * e13)) -
                e10 * (e01 * (e22 * e33 - e32 * e23) - e21 * (e02 * e33 - e32 * e03) + e31 * (e02 * e23 - e22 * e03)) +
                e20 * (e01 * (e12 * e33 - e32 * e13) - e11 * (e02 * e33 - e32 * e03) + e31 * (e02 * e13 - e12 * e03)) -
                e30 * (e01 * (e12 * e23 - e22 * e13) - e11 * (e02 * e23 - e22 * e03) + e21 * (e02 * e13 - e12 * e03));
    }

    mat4 inverse() const {
        float idet = 1.0f / det();
        mat4 r;
        r.e00 =  (e11 * (e22 * e33 - e32 * e23) - e21 * (e12 * e33 - e32 * e13) + e31 * (e12 * e23 - e22 * e13)) * idet;
        r.e01 = -(e01 * (e22 * e33 - e32 * e23) - e21 * (e02 * e33 - e32 * e03) + e31 * (e02 * e23 - e22 * e03)) * idet;
        r.e02 =  (e01 * (e12 * e33 - e32 * e13) - e11 * (e02 * e33 - e32 * e03) + e31 * (e02 * e13 - e12 * e03)) * idet;
        r.e03 = -(e01 * (e12 * e23 - e22 * e13) - e11 * (e02 * e23 - e22 * e03) + e21 * (e02 * e13 - e12 * e03)) * idet;
        r.e10 = -(e10 * (e22 * e33 - e32 * e23) - e20 * (e12 * e33 - e32 * e13) + e30 * (e12 * e23 - e22 * e13)) * idet;
        r.e11 =  (e00 * (e22 * e33 - e32 * e23) - e20 * (e02 * e33 - e32 * e03) + e30 * (e02 * e23 - e22 * e03)) * idet;
        r.e12 = -(e00 * (e12 * e33 - e32 * e13) - e10 * (e02 * e33 - e32 * e03) + e30 * (e02 * e13 - e12 * e03)) * idet;
        r.e13 =  (e00 * (e12 * e23 - e22 * e13) - e10 * (e02 * e23 - e22 * e03) + e20 * (e02 * e13 - e12 * e03)) * idet;
        r.e20 =  (e10 * (e21 * e33 - e31 * e23) - e20 * (e11 * e33 - e31 * e13) + e30 * (e11 * e23 - e21 * e13)) * idet;
        r.e21 = -(e00 * (e21 * e33 - e31 * e23) - e20 * (e01 * e33 - e31 * e03) + e30 * (e01 * e23 - e21 * e03)) * idet;
        r.e22 =  (e00 * (e11 * e33 - e31 * e13) - e10 * (e01 * e33 - e31 * e03) + e30 * (e01 * e13 - e11 * e03)) * idet;
        r.e23 = -(e00 * (e11 * e23 - e21 * e13) - e10 * (e01 * e23 - e21 * e03) + e20 * (e01 * e13 - e11 * e03)) * idet;
        r.e30 = -(e10 * (e21 * e32 - e31 * e22) - e20 * (e11 * e32 - e31 * e12) + e30 * (e11 * e22 - e21 * e12)) * idet;
        r.e31 =  (e00 * (e21 * e32 - e31 * e22) - e20 * (e01 * e32 - e31 * e02) + e30 * (e01 * e22 - e21 * e02)) * idet;
        r.e32 = -(e00 * (e11 * e32 - e31 * e12) - e10 * (e01 * e32 - e31 * e02) + e30 * (e01 * e12 - e11 * e02)) * idet;
        r.e33 =  (e00 * (e11 * e22 - e21 * e12) - e10 * (e01 * e22 - e21 * e02) + e20 * (e01 * e12 - e11 * e02)) * idet;
        return r;
    }

    mat4 transpose() const {
        mat4 r;
        r.e00 = e00; r.e10 = e01; r.e20 = e02; r.e30 = e03;
        r.e01 = e10; r.e11 = e11; r.e21 = e12; r.e31 = e13;
        r.e02 = e20; r.e12 = e21; r.e22 = e22; r.e32 = e23;
        r.e03 = e30; r.e13 = e31; r.e23 = e32; r.e33 = e33;
        return r;
    }

    quat getRot() const {
        float t, s;
        t = 1.0f + e00 + e11 + e22;
        if (t > EPS) {
            s = 0.5f / sqrtf(t);
            return quat((e21 - e12) * s, (e02 - e20) * s, (e10 - e01) * s, 0.25f / s);
        } else
            if (e00 > e11 && e00 > e22) {
                s = 0.5f / sqrtf(1.0f + e00 - e11 - e22);
                return quat(0.25f / s, (e01 + e10) * s, (e02 + e20) * s, (e21 - e12) * s);
            } else
                if (e11 > e22) {
                    s = 0.5f / sqrtf(1.0f - e00 + e11 - e22);
                    return quat((e01 + e10) * s, 0.25f / s, (e12 + e21) * s, (e02 - e20) * s);
                } else {
                    s = 0.5f / sqrtf(1.0f - e00 - e11 + e22);
                    return quat((e02 + e20) * s, (e12 + e21) * s, 0.25f / s, (e10 - e01) * s);
                }
    }

    void setRot(const quat &rot) {
        float   sx = rot.x * rot.x,
                sy = rot.y * rot.y,
                sz = rot.z * rot.z,
                sw = rot.w * rot.w,
                inv = 1.0f / (sx + sy + sz + sw);

        e00 = ( sx - sy - sz + sw) * inv;
        e11 = (-sx + sy - sz + sw) * inv;
        e22 = (-sx - sy + sz + sw) * inv;
        inv *= 2.0f;

        float t1 = rot.x * rot.y;
        float t2 = rot.z * rot.w;
        e10 = (t1 + t2) * inv;
        e01 = (t1 - t2) * inv;

        t1 = rot.x * rot.z;
        t2 = rot.y * rot.w;
        e20 = (t1 - t2) * inv;
        e02 = (t1 + t2) * inv;

        t1 = rot.y * rot.z;
        t2 = rot.x * rot.w;
        e21 = (t1 + t2) * inv;
        e12 = (t1 - t2) * inv;
    }

    vec3 getPos() const {
        return offset.xyz;
    }

    void setPos(const vec3 &pos) {
        offset.xyz = pos;
    }
};

struct Box {
    vec3 min, max;

    Box() {}
    Box(const vec3 &min, const vec3 &max) : min(min), max(max) {}

    void rotate90(int n) {
        switch (n) {
            case 0  : break;
            case 1  : *this = Box(vec3( min.z, min.y, -max.x), vec3( max.z, max.y, -min.x)); break;
            case 2  : *this = Box(vec3(-max.x, min.y, -max.z), vec3(-min.x, max.y, -min.z)); break;
            case 3  : *this = Box(vec3(-max.z, min.y,  min.x), vec3(-min.z, max.y,  max.x)); break;
            default : ASSERT(false);
        }
    }

    bool intersect(const Box &box) const {
        return !((max.x < box.min.x || min.x > box.max.x) || (max.y < box.min.y || min.y > box.max.y) || (max.z < box.min.z || min.z > box.max.z));
    }
};

struct Stream {
    FILE        *f;
    const char	*data;
    int         size, pos;

    Stream(const void *data, int size) : f(NULL), data((char*)data), size(size), pos(0) {}

    Stream(const char *name) : data(NULL), size(-1), pos(0) {
    #ifdef __APPLE__
        extern char *contentPath;
        int len = strlen(contentPath);
        strcat(contentPath, name);
        f = fopen(contentPath, "rb");
        contentPath[len] = '\0';
    #else
        f = fopen(name, "rb");
    #endif
        if (!f) LOG("error loading file\n");
        fseek(f, 0, SEEK_END);
        size = ftell(f);
        fseek(f, 0, SEEK_SET);
    }

    ~Stream() {
        if (f) fclose(f);
    }

    void setPos(int pos) {
        this->pos = pos;
        if (f) fseek(f, pos, SEEK_SET);
    }

    void seek(int offset) {
        if (!offset) return;
        if (f) fseek(f, offset, SEEK_CUR);
        pos += offset;
    }

    void raw(void *data, int count) {
        if (f)
            fread(data, 1, count, f);
        else
            memcpy(data, this->data + pos, count);
        pos += count;
    }

    template <typename T>
    inline T& read(T &x) {
        raw(&x, sizeof(x));
        return x;
    }

    template <typename T>
    inline T* read(T *&a, int count) {
        if (count) {
            a = new T[count];
            raw(a, count * sizeof(T));
        } else
            a = NULL;
        return a;
    }
};

#endif 
