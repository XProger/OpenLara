#ifndef H_UTILS
#define H_UTILS

#include <stdlib.h>
#include <cstring>
#include <math.h>
#include <float.h>

#ifdef _DEBUG
    #ifdef LINUX
        #define debugBreak() raise(SIGTRAP);
    #else
        #define debugBreak() _asm { int 3 }
    #endif

    #define ASSERT(expr) if (expr) {} else { LOG("ASSERT:\n  %s:%d\n  %s => %s\n", __FILE__, __LINE__, __FUNCTION__, #expr); debugBreak(); }

    #ifndef ANDROID
        #define LOG(...) printf(__VA_ARGS__)
    #endif

#else
    #define ASSERT(expr)
    #ifdef LINUX
        #define LOG(...) printf(__VA_ARGS__); fflush(stdout)
    #else
        #define LOG(...) printf(__VA_ARGS__)
    //    #define LOG(...) 0
    #endif
#endif

#ifdef ANDROID
        #include <android/log.h>
        #undef LOG
        #define LOG(...) __android_log_print(ANDROID_LOG_INFO,"OpenLara",__VA_ARGS__)
#endif

#define DECL_ENUM(v) v,
#define DECL_STR(v)  #v,

#define EPS     FLT_EPSILON
#define INF     INFINITY
#define PI      3.14159265358979323846f
#define PI2     (PI * 2.0f)
#define DEG2RAD (PI / 180.0f)
#define RAD2DEG (180.0f / PI)
#define randf() ((float)rand()/RAND_MAX)

typedef signed char     int8;
typedef signed short    int16;
typedef signed int      int32;
typedef unsigned char   uint8;
typedef unsigned short  uint16;
typedef unsigned int    uint32;

#define FOURCC(str)     uint32(((uint8*)(str))[0] | (((uint8*)(str))[1] << 8) | (((uint8*)(str))[2] << 16) | (((uint8*)(str))[3] << 24) )

#define COUNT(arr)      (sizeof(arr) / sizeof(arr[0]))

template <typename T>
inline const T& min(const T &a, const T &b) {
    return a < b ? a : b;
}

template <typename T>
inline const T& min(const T &a, const T &b, const T &c) {
    return (a < b && a < c) ? a : ((b < a && b < c) ? b : c);
}

template <class T>
inline const T& max(const T &a, const T &b) {
    return a > b ? a : b;
}

template <typename T>
inline const T& max(const T &a, const T &b, const T &c) {
    return (a > b && a > c) ? a : ((b > a && b > c) ? b : c);
}

template <class T>
inline const T& clamp(const T &x, const T &a, const T &b) {
    return x < a ? a : (x > b ? b : x);
}

template <class T>
inline const int sign(const T &x) {
    return x > 0 ? 1 : (x < 0 ? -1 : 0);
}

template <class T>
inline void swap(T &a, T &b) {
    T tmp = a;
    a = b;
    b = tmp;
}

float clampAngle(float a) {
    return a < -PI ? a + PI2 : (a >= PI ? a - PI2 : a);
}

float shortAngle(float a, float b) {
    float n = clampAngle(b) - clampAngle(a);
    return clampAngle(n - int(n / PI2) * PI2);
}

float normalizeAngle(float angle) {
    while (angle < 0.0f) angle += PI2;
    while (angle > PI2)  angle -= PI2;
    return angle;
}

int angleQuadrant(float angle) {
    return int(normalizeAngle(angle + PI * 0.25f) / (PI * 0.5f));
}

float decrease(float delta, float &value, float &speed) {
    if (speed > 0.0f && fabsf(delta) > 0.001f) {
        if (delta > 0) speed = min(delta,  speed);
        if (delta < 0) speed = max(delta, -speed);
        value += speed;
        return speed;
    } else
        return 0.0f;
}

float hermite(float x) {
    return x * x * (3.0f - 2.0f * x);
}

float lerp(float a, float b, float t) {
    if (t <= 0.0f) return a;
    if (t >= 1.0f) return b;
    return a + (b - a) * t; 
}

float lerpAngle(float a, float b, float t) {
    if (t <= 0.0f) return a;
    if (t >= 1.0f) return b;
    return a + shortAngle(a, b) * t; 
}

int nextPow2(uint32 x) {
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x++;
    return x;
}

uint32 fnv32(const char *data, int32 size, uint32 hash = 0x811c9dc5) {
    for (int i = 0; i < size; i++)
        hash = (hash ^ data[i]) * 0x01000193;
    return hash;
}

struct vec2 {
    float x, y;
    vec2() {}
    vec2(float s) : x(s), y(s) {}
    vec2(float x, float y) : x(x), y(y) {}

    inline float& operator [] (int index) const { ASSERT(index >= 0 && index <= 1); return ((float*)this)[index]; }

    inline bool operator == (const vec2 &v) const { return x == v.x && y == v.y; }
    inline bool operator != (const vec2 &v) const { return !(*this == v); }
    inline bool operator == (float s)       const { return x == s && y == s; }
    inline bool operator != (float s)       const { return !(*this == s); }
    inline bool operator <  (const vec2 &v) const { return x < v.x && y < v.y; }
    inline bool operator >  (const vec2 &v) const { return x > v.x && y > v.y; }
    inline vec2 operator -  ()              const { return vec2(-x, -y); }

    vec2& operator += (const vec2 &v) { x += v.x; y += v.y; return *this; }
    vec2& operator -= (const vec2 &v) { x -= v.x; y -= v.y; return *this; }
    vec2& operator *= (const vec2 &v) { x *= v.x; y *= v.y; return *this; }
    vec2& operator /= (const vec2 &v) { x /= v.x; y /= v.y; return *this; }
    vec2& operator += (float s)       { x += s;   y += s;   return *this; }
    vec2& operator -= (float s)       { x -= s;   y -= s;   return *this; }
    vec2& operator *= (float s)       { x *= s;   y *= s;   return *this; }
    vec2& operator /= (float s)       { x /= s;   y /= s;   return *this; }

    vec2 operator + (const vec2 &v) const { return vec2(x + v.x, y + v.y); }
    vec2 operator - (const vec2 &v) const { return vec2(x - v.x, y - v.y); }
    vec2 operator * (const vec2 &v) const { return vec2(x * v.x, y * v.y); }
    vec2 operator / (const vec2 &v) const { return vec2(x / v.x, y / v.y); }
    vec2 operator + (float s)       const { return vec2(x + s,   y + s  ); }
    vec2 operator - (float s)       const { return vec2(x - s,   y - s  ); }
    vec2 operator * (float s)       const { return vec2(x * s,   y * s  ); }
    vec2 operator / (float s)       const { return vec2(x / s,   y / s  ); }

    float dot(const vec2 &v)   const { return x * v.x + y * v.y; }
    float cross(const vec2 &v) const { return x * v.y - y * v.x; }

    float length2() const { return dot(*this); }
    float length()  const { return sqrtf(length2()); }
    vec2  abs()     const { return vec2(fabsf(x), fabsf(y)); }
    vec2  normal()  const { float s = length(); return s == 0.0 ? (*this) : (*this)*(1.0f/s); }
    float angle()   const { return atan2f(y, x); }
    vec2& rotate(const vec2 &cs) { *this = vec2(x*cs.x - y*cs.y, x*cs.y + y*cs.x); return *this; }
    vec2& rotate(float angle)    { return rotate(vec2(cosf(angle), sinf(angle))); }
};

struct vec3 {
    union {
        struct { float x, y, z; };
        struct { vec2 xy; };
    };

    vec3() {}
    vec3(float s) : x(s), y(s), z(s) {}
    vec3(float x, float y, float z) : x(x), y(y), z(z) {}
    vec3(const vec2 &xy, float z = 0.0f) : x(xy.x), y(xy.y), z(z) {}
    vec3(float lng, float lat) : x(sinf(lat) * cosf(lng)), y(-sinf(lng)), z(cosf(lat) * cosf(lng)) {}

    inline float& operator [] (int index) const { ASSERT(index >= 0 && index <= 2); return ((float*)this)[index]; }

    inline bool operator == (const vec3 &v) const { return x == v.x && y == v.y && z == v.z; }
    inline bool operator != (const vec3 &v) const { return !(*this == v); }
    inline bool operator == (float s)       const { return x == s && y == s && z == s; }
    inline bool operator != (float s)       const { return !(*this == s); }
    inline bool operator <  (const vec3 &v) const { return x < v.x && y < v.y && z < v.z; }
    inline bool operator >  (const vec3 &v) const { return x > v.x && y > v.y && z > v.z; }
    inline vec3 operator -  ()              const { return vec3(-x, -y, -z); }

    vec3& operator += (const vec3 &v) { x += v.x; y += v.y; z += v.z; return *this; }
    vec3& operator -= (const vec3 &v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
    vec3& operator *= (const vec3 &v) { x *= v.x; y *= v.y; z *= v.z; return *this; }
    vec3& operator /= (const vec3 &v) { x /= v.x; y /= v.y; z /= v.z; return *this; }
    vec3& operator += (float s)       { x += s;   y += s;   z += s;   return *this; }
    vec3& operator -= (float s)       { x -= s;   y -= s;   z -= s;   return *this; }
    vec3& operator *= (float s)       { x *= s;   y *= s;   z *= s;   return *this; }
    vec3& operator /= (float s)       { x /= s;   y /= s;   z /= s;   return *this; }

    vec3 operator + (const vec3 &v) const { return vec3(x + v.x, y + v.y, z + v.z); }
    vec3 operator - (const vec3 &v) const { return vec3(x - v.x, y - v.y, z - v.z); }
    vec3 operator * (const vec3 &v) const { return vec3(x * v.x, y * v.y, z * v.z); }
    vec3 operator / (const vec3 &v) const { return vec3(x / v.x, y / v.y, z / v.z); }
    vec3 operator + (float s)       const { return vec3(x + s,   y + s,   z + s);   }
    vec3 operator - (float s)       const { return vec3(x - s,   y - s,   z - s);   }
    vec3 operator * (float s)       const { return vec3(x * s,   y * s,   z * s);   }
    vec3 operator / (float s)       const { return vec3(x / s,   y / s,   z / s);   }

    float dot(const vec3 &v)   const { return x * v.x + y * v.y + z * v.z; }
    vec3  cross(const vec3 &v) const { return vec3(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x); }

    float length2() const { return dot(*this); }
    float length()  const { return sqrtf(length2()); }
    vec3  abs()     const { return vec3(fabsf(x), fabsf(y), fabsf(z)); }
    vec3  normal()  const { float s = length(); return s == 0.0f ? (*this) : (*this)*(1.0f/s); }
    vec3  axisXZ()  const { return (fabsf(x) > fabsf(z)) ? vec3(float(sign(x)), 0, 0) : vec3(0, 0, float(sign(z))); }

    vec3 reflect(const vec3 &n) const {
        return *this - n * (dot(n) * 2.0f);
    }

    vec3 lerp(const vec3 &v, const float t) const {
        if (t <= 0.0f) return *this;
        if (t >= 1.0f) return v;
        return *this + (v - *this) * t; 
    }

    vec3 rotateY(float angle) const {
        float s = sinf(angle), c = cosf(angle); 
        return vec3(x*c - z*s, y, x*s + z*c);
    }

    float angle(const vec3 &v) const {
        return dot(v) / (length() * v.length());
    }

    float angleY() const { return atan2f(z, x); }
};

struct vec4 {
    union {
        struct { float x, y, z, w; };
        struct { vec2 xy, zw; };
        struct { vec3 xyz; };
    };

    vec4() {}
    vec4(float s) : x(s), y(s), z(s), w(s) {}
    vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    vec4(const vec3 &xyz) : x(xyz.x), y(xyz.y), z(xyz.z), w(0) {}
    vec4(const vec3 &xyz, float w) : x(xyz.x), y(xyz.y), z(xyz.z), w(w) {}
    vec4(const vec2 &xy, const vec2 &zw) : x(xy.x), y(xy.y), z(zw.x), w(zw.y) {}

    inline bool operator == (const vec4 &v) const { return x == v.x && y == v.y && z == v.z && w == v.w; }
    inline bool operator != (const vec4 &v) const { return !(*this == v); }

    vec4 operator + (const vec4 &v) const { return vec4(x + v.x, y + v.y, z + v.z, w + v.w); }
    vec4 operator - (const vec4 &v) const { return vec4(x - v.x, y - v.y, z - v.z, w - v.w); }
    vec4 operator * (const vec4 &v) const { return vec4(x*v.x, y*v.y, z*v.z, w*v.w); }
    vec4& operator *= (const vec4 &v) { x*=v.x; y*=v.y; z*=v.z; w*=v.w; return *this; }

    vec4 lerp(const vec4 &v, const float t) const {
        if (t <= 0.0f) return *this;
        if (t >= 1.0f) return v;
        return *this + (v - *this) * t; 
    }
};

struct quat {
    union {
        struct { float x, y, z, w; };
        struct { vec3  xyz; };
    };

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

    vec3 operator * (const vec3 &v) const {
    //	return v + xyz.cross(xyz.cross(v) + v * w) * 2.0f;
	    return (*this * quat(v.x, v.y, v.z, 0) * inverse()).xyz;
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

    mat4(float e00, float e10, float e20, float e30,
         float e01, float e11, float e21, float e31,
         float e02, float e12, float e22, float e32,
         float e03, float e13, float e23, float e33) : 
         e00(e00), e10(e10), e20(e20), e30(e30),
         e01(e01), e11(e11), e21(e21), e31(e31),
         e02(e02), e12(e12), e22(e22), e32(e32),
         e03(e03), e13(e13), e23(e23), e33(e33) {}

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
        if (aspect >= 1.0f) {
            e00 = k / aspect;
            e11 = k;
        } else {
            e00 = k;
            e11 = k * aspect;
        }
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

    mat4(const vec4 &reflectPlane) {
        float a = reflectPlane.x, 
              b = reflectPlane.y, 
              c = reflectPlane.z, 
              d = reflectPlane.w;

        right  = vec4(1 - 2*a*a,   - 2*b*a,   - 2*c*a, 0);
        up     = vec4(  - 2*a*b, 1 - 2*b*b,   - 2*c*b, 0);
        dir    = vec4(  - 2*a*c,   - 2*b*c, 1 - 2*c*c, 0);
        offset = vec4(  - 2*a*d,   - 2*b*d,   - 2*c*d, 1);
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
        if (t > 0.0001f) {
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

struct Basis {
    quat    rot;
    vec3    pos;
    float   w;

    Basis() {}
    Basis(const quat &rot, const vec3 &pos) : rot(rot), pos(pos), w(1.0f) {}
    Basis(const mat4 &matrix) : rot(matrix.getRot()), pos(matrix.getPos()), w(1.0f) {}

    void identity() {
        rot = quat(0, 0, 0, 1);
        pos = vec3(0, 0, 0);
        w   = 1.0f;
    }

	Basis operator * (const Basis &basis) const {
        return Basis(rot * basis.rot, pos + rot * basis.pos);
    }

	vec3 operator * (const vec3 &v) const {
        return rot * v + pos;
    }

	Basis inverse() const {
        quat q = rot.conjugate();
        return Basis(q, -(q * pos));
    }

    void translate(const vec3 &v) {
        pos += rot * v;
    }

    void rotate(const quat &q) {
        rot = rot * q;
    }

    Basis lerp(const Basis &basis, float t) {
        Basis b;
        b.rot = rot.lerp(basis.rot, t);
        b.pos = pos.lerp(basis.pos, t);
        return b;
    }
};

struct ubyte2 {
    uint8 x, y;
};

struct ubyte4 {
    uint8 x, y, z, w;
};

struct short2 {
    int16 x, y;
};

struct short3 {
    int16 x, y, z;

    short3() {}
    short3(int16 x, int16 y, int16 z) : x(x), y(y), z(z) {}

    operator vec3() const { return vec3((float)x, (float)y, (float)z); };

    short3 operator + (const short3 &v) const { return short3(x + v.x, y + v.y, z + v.z); }
    short3 operator - (const short3 &v) const { return short3(x - v.x, y - v.y, z - v.z); }
};

struct short4 {
    int16 x, y, z, w;

    short4() {}
    short4(int16 x, int16 y, int16 z, int16 w) : x(x), y(y), z(z), w(w) {}

    operator vec3()   const { return vec3((float)x, (float)y, (float)z); };
    operator short3() const { return *((short3*)this); }

    inline bool operator == (const short4 &v) const { return x == v.x && y == v.y && z == v.z && w == v.w; }
    inline bool operator != (const short4 &v) const { return !(*this == v); }

    inline int16& operator [] (int index) const { ASSERT(index >= 0 && index <= 3); return ((int16*)this)[index]; }
};

quat rotYXZ(const vec3 &a) {
    mat4 m;
    m.identity();
    m.rotateY(a.y);
    m.rotateX(a.x);
    m.rotateZ(a.z);
    return m.getRot();
}

quat lerpAngle(const vec3 &a, const vec3 &b, float t) { // TODO: optimization
    return rotYXZ(a).slerp(rotYXZ(b), t).normal();
}

vec3 boxNormal(int x, int z) {
    x %= 1024;
    z %= 1024;

    if (x > 1024 - z)
        return x < z ? vec3(0, 0, 1)  : vec3(1, 0, 0);
    else
        return x < z ? vec3(-1, 0, 0) : vec3(0, 0, -1);
}


struct Sphere {
    vec3  center;
    float radius;

    Sphere() {}
    Sphere(const vec3 &center, float radius) : center(center), radius(radius) {}

    bool intersect(const Sphere &s) const {
        float d = (center - s.center).length2();
        float r = (radius + s.radius);
        return d < r * r;
    }

    bool intersect(const vec3 &rayPos, const vec3 &rayDir, float &t) const {
        vec3 v = rayPos - center;
        float h = -v.dot(rayDir);
        float d = h * h + radius * radius - v.length2();

        if (d > 0.0f) {
            d = sqrtf(d);
            float tMin = h - d;
            float tMax = h + d;
            if (tMax > 0.0f) {
                if (tMin < 0.0f)
                    tMin = 0.0f;
                t = tMin;
                return true;
            }
        }
        return false;
    }
};


struct Box {
    vec3 min, max;

    Box() {}
    Box(const vec3 &min, const vec3 &max) : min(min), max(max) {}

    vec3 operator [] (int index) const {        
        ASSERT(index >= 0 && index <= 7);
        switch (index) {
            case 0 : return min;
            case 1 : return max;
            case 2 : return vec3(min.x, max.y, max.z);
            case 3 : return vec3(max.x, min.y, max.z);
            case 4 : return vec3(min.x, min.y, max.z);
            case 5 : return vec3(max.x, max.y, min.z);
            case 6 : return vec3(min.x, max.y, min.z);
            case 7 : return vec3(max.x, min.y, min.z);
        }
        return min;
    }

    Box intersection2D(const Box &b) const {
        Box r(vec3(0.0f), vec3(0.0f));
        if (max.x < b.min.x || min.x > b.max.x) return r;
        if (max.y < b.min.y || min.y > b.max.y) return r;
        r.max.x = ::min(max.x, b.max.x);
        r.max.y = ::min(max.y, b.max.y);
        r.min.x = ::max(min.x, b.min.x);
        r.min.y = ::max(min.y, b.min.y);
        return r;
    }

    Box& operator += (const Box &box) {
        min.x = ::min(min.x, box.min.x);
        min.y = ::min(min.y, box.min.y);
        min.z = ::min(min.z, box.min.z);
        max.x = ::max(max.x, box.max.x);
        max.y = ::max(max.y, box.max.y);
        max.z = ::max(max.z, box.max.z);
        return *this; 
    }

    Box& operator += (const vec3 &v) {
        min.x = ::min(min.x, v.x);
        min.y = ::min(min.y, v.y);
        min.z = ::min(min.z, v.z);
        max.x = ::max(max.x, v.x);
        max.y = ::max(max.y, v.y);
        max.z = ::max(max.z, v.z);
        return *this; 
    }

    Box& operator -= (const Box &box) {
        min.x = ::max(min.x, box.min.x);
        min.y = ::max(min.y, box.min.y);
        min.z = ::max(min.z, box.min.z);
        max.x = ::min(max.x, box.max.x);
        max.y = ::min(max.y, box.max.y);
        max.z = ::min(max.z, box.max.z);
        return *this; 
    }

    Box operator * (const mat4 &m) const {
        Box res(vec3(+INF), vec3(-INF));
        for (int i = 0; i < 8; i++) {
            vec4 v = m * vec4((*this)[i], 1.0f);            
            res += v.xyz /= v.w;
        }
        return res;
    }

    vec3 center() const {
        return (min + max) * 0.5f;
    }

    vec3 size() const {
        return max - min;
    }

    void expand(const vec3 &v) {
        min -= v;
        max += v;
    }

    void rotate90(int n) {
        switch (n) {
            case 0  : break;
            case 1  : *this = Box(vec3( min.z, min.y, -max.x), vec3( max.z, max.y, -min.x)); break;
            case 2  : *this = Box(vec3(-max.x, min.y, -max.z), vec3(-min.x, max.y, -min.z)); break;
            case 3  : *this = Box(vec3(-max.z, min.y,  min.x), vec3(-min.z, max.y,  max.x)); break;
            default : ASSERT(false);
        }
    }

    void translate(const vec3 &offset) {
        min += offset;
        max += offset;
    }

    vec3 closestPoint(const vec3 &p) const {
        return vec3(clamp(p.x, min.x, max.x),
                    clamp(p.y, min.y, max.y),
                    clamp(p.z, min.z, max.z));
    }

    bool contains(const vec3 &v) const {
        return v.x >= min.x && v.x <= max.x && v.y >= min.y && v.y <= max.y && v.z >= min.z && v.z <= max.z;
    }

    vec3 pushOut2D(const vec3 &v) const {
        float ax = v.x - min.x;
        float bx = max.x - v.x;
        float az = v.z - min.z;
        float bz = max.z - v.z;

        vec3 p = vec3(0.0f);
        if (ax <= bx && ax <= az && ax <= bz)
            p.x = -ax;
        else if (bx <= ax && bx <= az && bx <= bz)
            p.x =  bx;
        else if (az <= ax && az <= bx && az <= bz)
            p.z = -az;
        else
            p.z =  bz;

        return p;
    }

    vec3 pushOut2D(const Box &b) const {
        float ax = b.max.x - min.x;
        float bx = max.x - b.min.x;
        float az = b.max.z - min.z;
        float bz = max.z - b.min.z;

        vec3 p = vec3(0.0f);
        if (ax <= bx && ax <= az && ax <= bz)
            p.x -= ax;
        else if (bx <= ax && bx <= az && bx <= bz)
            p.x += bx;
        else if (az <= ax && az <= bx && az <= bz)
            p.z -= az;
        else
            p.z += bz;

        return p;
    }

    bool intersect(const Box &box) const {
        return !((max.x < box.min.x || min.x > box.max.x) || (max.y < box.min.y || min.y > box.max.y) || (max.z < box.min.z || min.z > box.max.z));
    }

    bool intersect(const Sphere &sphere) const {
        if (contains(sphere.center))
            return true;
        vec3 dir = sphere.center - closestPoint(sphere.center);
        return (dir.length2() < sphere.radius * sphere.radius);
    }

    bool intersect(const vec3 &rayPos, const vec3 &rayDir, float &t) const {
        float tMax = INF, tMin = -tMax;

        for (int i = 0; i < 3; i++) 
            if (rayDir[i] != 0) {
                float lo = (min[i] - rayPos[i]) / rayDir[i];
                float hi = (max[i] - rayPos[i]) / rayDir[i];
                tMin = ::max(tMin, ::min(lo, hi));
                tMax = ::min(tMax, ::max(lo, hi));
            } else
                if (rayPos[i] < min[i] || rayPos[i] > max[i])
                    return false;
        t = tMin;
        return (tMin <= tMax) && (tMax > 0.0f);
    }

    bool intersect(const mat4 &matrix, const vec3 &rayPos, const vec3 &rayDir, float &t) const {
        mat4 mInv = matrix.inverse();
        return intersect(mInv * rayPos, (mInv * vec4(rayDir, 0)).xyz, t);
    }
};


struct Stream {
    static char cacheDir[255];
    static char contentDir[255];

    typedef void (Callback)(Stream *stream, void *userData);
    Callback    *callback;
    void        *userData;

    FILE        *f;
    char        *data;
    int         size, pos;
    char        *name;

    Stream(const void *data, int size) : callback(NULL), userData(NULL), f(NULL), data((char*)data), size(size), pos(0), name(NULL) {}

    Stream(const char *name, Callback *callback = NULL, void *userData = NULL) : callback(callback), userData(userData), data(NULL), size(-1), pos(0), name(NULL) {
        if (contentDir[0] && (!cacheDir[0] || !strstr(name, cacheDir))) {
            char path[255];
            path[0] = 0;
            strcat(path, contentDir);
            strcat(path, name);
            f = fopen(path, "rb");
        } else
            f = fopen(name, "rb");

        if (!f) {
            #ifdef __EMSCRIPTEN__
                this->name = new char[64];
                strcpy(this->name, name);

                extern void osDownload(Stream *stream);
                osDownload(this);
                return;
            #else
                LOG("error loading file \"%s\"\n", name);
                if (callback) {
                    callback(NULL, userData);
                    delete this;
                    return;
                } else {
                    ASSERT(false);
                }
            #endif
        } else {
            fseek(f, 0, SEEK_END);
            size = ftell(f);
            fseek(f, 0, SEEK_SET);

            if (callback)
                callback(this, userData);
        }
    }

    ~Stream() {
        delete[] name;
        if (f) fclose(f);
    }

    static bool fileExists(const char *name) {
        FILE *f = fopen(name, "rb");
        if (!f)
            return false;
        else
            fclose(f);
        return true;
    }

    static void write(const char *name, const void *data, int size) {
    #ifdef __EMSCRIPTEN__
        extern void osSave(const char *name, const void *data, int size);
        osSave(name, data, size);
    #else
        FILE *f = fopen(name, "wb");
        if (!f) return;
        fwrite(data, size, 1, f);
        fclose(f);
    #endif
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
        if (!count) return;
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
