#ifndef H_UTILS
#define H_UTILS

#include <stdlib.h>
#include <math.h>
#include <float.h>

//#define TEST_SLOW_FIO

#ifdef _DEBUG
    #if defined(_OS_WP8)
        #define debugBreak() /* TODO */
    #elif defined(_OS_LINUX) || defined(_OS_RPI) || defined(_OS_CLOVER)
        #define debugBreak() raise(SIGTRAP);
    #elif defined(_OS_3DS)
        #define debugBreak() svcBreak(USERBREAK_ASSERT); 
    #else
        #define debugBreak() _asm { int 3 }
    #endif

    #define ASSERT(expr) if (!(expr)) { LOG("ASSERT:\n  %s:%d\n  %s => %s\n", __FILE__, __LINE__, __FUNCTION__, #expr); debugBreak(); }
    #define ASSERTV(expr) ASSERT(expr)

    #ifndef _OS_ANDROID
        #define LOG printf
    #endif

    #if defined(_OS_XBOX) || defined(_OS_XB1) || defined(_OS_WP8)
        #define MAX_LOG_LENGTH 1024

        #undef LOG
        void LOG(const char *format, ...) {
            char str[MAX_LOG_LENGTH];
            va_list arglist;
            va_start(arglist, format);
            _vsnprintf(str, MAX_LOG_LENGTH, format, arglist);
            va_end(arglist);
            OutputDebugStringA(str);
        }
    #endif

#else

    #define ASSERT(expr)
    #define ASSERTV(expr) (expr) ? 1 : 0

    #ifdef PROFILE
        #ifdef _OS_LINUX
            #define LOG(...) printf(__VA_ARGS__); fflush(stdout)
        #else
            #define LOG(...) printf(__VA_ARGS__)
        #endif
    #else
        #define LOG printf
    #endif
#endif

#ifdef _OS_PSV
    #undef LOG
    #define LOG(...) psvDebugScreenPrintf(__VA_ARGS__)
#endif

#ifdef _OS_ANDROID
    #include <android/log.h>
    #undef LOG
    #define LOG(...) __android_log_print(ANDROID_LOG_INFO,"OpenLara",__VA_ARGS__)
#endif


#ifdef _OS_PSP
    extern "C" {
    // pspmath.h
        extern float vfpu_sinf(float x);
        extern float vfpu_cosf(float x);
        extern float vfpu_atan2f(float x, float y);
        extern void  vfpu_sincos(float r, float *s, float *c);
    }

    #define sinf(x)         vfpu_sinf(x)
    #define cosf(x)         vfpu_cosf(x)
    #define atan2f(x, y)    vfpu_atan2f(x, y)
    #define sincos(a, s, c) vfpu_sincos(a, s, c)
#else
    void sincos(float r, float *s, float *c) {
        *s = sinf(r);
        *c = cosf(r);
    }
#endif

#define DECL_ENUM(v) v,
#define DECL_STR(v)  #v,

#define EPS     FLT_EPSILON
#define INF     FLT_MAX
#define PI      3.14159265358979323846f
#define PIH     (PI * 0.5f)
#define PI2     (PI * 2.0f)
#define DEG2RAD (PI / 180.0f)
#define RAD2DEG (180.0f / PI)

#define COS30   0.86602540378f
#define COS45   0.70710678118f
#define COS60   0.50000000000f

#define SQR(x)  ((x)*(x))
#define randf() (float(rand())/float(RAND_MAX))

typedef signed char        int8;
typedef signed short       int16;
typedef signed int         int32;
typedef signed long long   int64;
typedef unsigned char      uint8;
typedef unsigned short     uint16;
typedef unsigned int       uint32;
typedef unsigned long long uint64;

#define FOURCC(str)        uint32( ((uint8*)(str))[0] | (((uint8*)(str))[1] << 8) | (((uint8*)(str))[2] << 16) | (((uint8*)(str))[3] << 24) )
#define TWOCC(str)         uint32( ((uint8*)(str))[0] | (((uint8*)(str))[1] << 8) )

#define ALIGNADDR(x, a)    (((x) + ((a) - 1)) & ~((a) - 1))
#define COUNT(arr)         int(sizeof(arr) / sizeof(arr[0]))
#define OFFSETOF(T, E)     ((size_t)&(((T*)0)->E))
#define TEST_BIT(arr, bit) ((arr[bit / 32] >> (bit % 32)) & 1)

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
inline const T sign(const T &x) {
    return (T)(x > 0 ? 1 : (x < 0 ? -1 : 0));
}

template <class T>
inline void swap(T &a, T &b) {
    T tmp = a;
    a = b;
    b = tmp;
}

inline uint16 swap16(uint16 x) {
    return ((x & 0x00FF) << 8) | ((x & 0xFF00) >> 8);
}

inline uint32 swap32(uint32 x) {
    return ((x & 0x000000FF) << 24) | ((x & 0x0000FF00) << 8) | ((x & 0x00FF0000) >> 8) | ((x & 0xFF000000) >> 24);
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

int angleQuadrant(float angle, float offset) {
    return int(normalizeAngle(angle + PI * offset) / (PI * 0.5f));
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

inline float hermite(float x) {
    return x * x * (3 - 2 * x);
}

inline float quintic(float x) {
    return x * x * x * (x * (x * 6 - 15) + 10);
}

inline float lerp(float a, float b, float t) {
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

inline uint32 log2i(uint32 value) {
    int res = 0;
    for(; value; value >>= 1, res++);
    return res ? res - 1 : res;
}

uint32 fnv32(const char *data, int32 size, uint32 hash = 0x811c9dc5) {
    for (int i = 0; i < size; i++)
        hash = (hash ^ data[i]) * 0x01000193;
    return hash;
}

template <class T>
void qsort(T* v, int L, int R) {
    int i = L;
    int j = R;
    const T m = v[(L + R) / 2];

    while (i <= j) {
        while (T::cmp(v[i], m) < 0) i++;
        while (T::cmp(m, v[j]) < 0) j--;

        if (i <= j)
            swap(v[i++], v[j--]);
    }

    if (L < j) qsort(v, L, j);
    if (i < R) qsort(v, i, R);
}

template <class T>
void sort(T *items, int count) {
    if (count)
        qsort(items, 0, count - 1);
}


namespace Noise { // based on https://github.com/Auburns/FastNoise
    int seed;

    uint8 m_perm[512];
    uint8 m_perm12[512];

    const float GRAD_X[] = { 1, -1, 1, -1, 1, -1, 1, -1, 0, 0, 0, 0 };
    const float GRAD_Y[] = { 1, 1, -1, -1, 0, 0, 0, 0, 1, -1, 1, -1 };
    const float GRAD_Z[] = { 0, 0, 0, 0, 1, 1, -1, -1, 1, 1, -1, -1 };

    uint8 index(int x, int y, int z) {
        return m_perm12[(x & 0xff) + m_perm[(y & 0xff) + m_perm[(z & 0xff)]]];
    }

    float noise(int x, int y, int z, float xd, float yd, float zd) {
        uint8 lutPos = index(x, y, z);
        return xd * GRAD_X[lutPos] + yd * GRAD_Y[lutPos] + zd * GRAD_Z[lutPos];
    }

    void setSeed(int seed) {
        Noise::seed = seed;
        srand(seed);

        for (int i = 0; i < 256; i++)
            m_perm[i] = i;

        for (int j = 0; j < 256; j++) {
            int k       = (rand() % (256 - j)) + j;
            int p       = m_perm[j];
            m_perm[j]   = m_perm[j + 256] = m_perm[k];
            m_perm[k]   = p;
            m_perm12[j] = m_perm12[j + 256] = m_perm[j] % 12;
        }
    }

    float value(float x, float y, float z, int size) {
        x *= size;
        y *= size;
        z *= size;

        int x0 = (int)x;
        int y0 = (int)y;
        int z0 = (int)z;
        int x1 = (x0 + 1) % size;
        int y1 = (y0 + 1) % size;
        int z1 = (z0 + 1) % size;

        float dx0 = x - x0;
        float dy0 = y - y0;
        float dz0 = z - z0;
        float dx1 = dx0 - 1;
        float dy1 = dy0 - 1;
        float dz1 = dz0 - 1;

        float fx = quintic(dx0);
        float fy = quintic(dy0);
        float fz = quintic(dz0);

        return lerp(lerp(lerp(noise(x0, y0, z0, dx0, dy0, dz0), noise(x1, y0, z0, dx1, dy0, dz0), fx), 
                         lerp(noise(x0, y1, z0, dx0, dy1, dz0), noise(x1, y1, z0, dx1, dy1, dz0), fx), fy),
                    lerp(lerp(noise(x0, y0, z1, dx0, dy0, dz1), noise(x1, y0, z1, dx1, dy0, dz1), fx), 
                         lerp(noise(x0, y1, z1, dx0, dy1, dz1), noise(x1, y1, z1, dx1, dy1, dz1), fx), fy), fz);
    }

    uint8* generate(uint32 seed, int size, int octaves, int frequency, float amplitude) {
        setSeed(seed);

        float *out = new float[size * size * size];
        memset(out, 0, size * size * size * sizeof(float));

        float isize = 1.0f / size;

        for (int j = 0; j < octaves; j++) {
            float *ptr = out;

            for (int z = 0; z < size; z++) {
                float iz = z * isize;
                for (int y = 0; y < size; y++) {
                    float iy = y * isize;
                    for (int x = 0; x < size; x++) {
                        float ix = x * isize;
                        *ptr++ += value(ix, iy, iz, frequency) * amplitude;
                    }
                }
            }

            frequency *= 2;
            amplitude *= 0.5f;
        }

        uint8 *dst = new uint8[size * size * size];
        for (int i = 0; i < size * size * size; i++) {
            dst[i] = clamp(int((out[i] * 0.5f + 0.5f) * 255.0f), 0, 255);
        }

        delete[] out;

        return dst;
    }
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
    vec2& rotate(float angle)    { vec2 cs; sincos(angle, &cs.y, &cs.x); return rotate(cs); }

    vec2 lerp(const vec2 &v, const float t) const {
        if (t <= 0.0f) return *this;
        if (t >= 1.0f) return v;
        return *this + (v - *this) * t; 
    }
};

struct vec3 {
    float x, y, z;

    vec3() {}
    vec3(float s) : x(s), y(s), z(s) {}
    vec3(float x, float y, float z) : x(x), y(y), z(z) {}
    vec3(const vec2 &xy, float z = 0.0f) : x(xy.x), y(xy.y), z(z) {}
    vec3(float lng, float lat) : x(sinf(lat) * cosf(lng)), y(-sinf(lng)), z(cosf(lat) * cosf(lng)) {}

    vec2& xy() const { return *((vec2*)&x); }
    vec2& yz() const { return *((vec2*)&y); }

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
        float s, c;
        sincos(angle, &s, &c);
        return vec3(x*c - z*s, y, x*s + z*c);
    }

    float angle(const vec3 &v) const {
        return dot(v) / (length() * v.length());
    }

    float angleX() const { return atan2f(sqrtf(x * x + z * z), y); }
    float angleY() const { return atan2f(z, x); }
};

struct vec4 {
    float x, y, z, w;

    vec2& xy()  const { return *((vec2*)&x); }
    vec3& xyz() const { return *((vec3*)&x); }

    vec4() {}
    vec4(float s) : x(s), y(s), z(s), w(s) {}
    vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    vec4(const vec3 &xyz) : x(xyz.x), y(xyz.y), z(xyz.z), w(0) {}
    vec4(const vec3 &xyz, float w) : x(xyz.x), y(xyz.y), z(xyz.z), w(w) {}
    vec4(const vec2 &xy, const vec2 &zw) : x(xy.x), y(xy.y), z(zw.x), w(zw.y) {}

    inline float& operator [] (int index) const { ASSERT(index >= 0 && index <= 3); return ((float*)this)[index]; }

    inline bool operator == (const vec4 &v) const { return x == v.x && y == v.y && z == v.z && w == v.w; }
    inline bool operator != (const vec4 &v) const { return !(*this == v); }

    vec4 operator + (const vec4 &v) const { return vec4(x + v.x, y + v.y, z + v.z, w + v.w); }
    vec4 operator - (const vec4 &v) const { return vec4(x - v.x, y - v.y, z - v.z, w - v.w); }
    vec4 operator * (const vec4 &v) const { return vec4(x*v.x, y*v.y, z*v.z, w*v.w); }
    vec4& operator *= (const vec4 &v) { x*=v.x; y*=v.y; z*=v.z; w*=v.w; return *this; }

    float dot(const vec4 &v) const { return x * v.x + y * v.y + z * v.z + w * v.w; }

    vec4 lerp(const vec4 &v, const float t) const {
        if (t <= 0.0f) return *this;
        if (t >= 1.0f) return v;
        return *this + (v - *this) * t; 
    }
};

struct quat {
    float x, y, z, w;

    vec3& xyz() const { return *((vec3*)&x); }

    quat() {}
    quat(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

    quat(const vec3 &axis, float angle) {
        float s, c;
        sincos(angle * 0.5f, &s, &c);
        x = axis.x * s;
        y = axis.y * s;
        z = axis.z * s;
        w = c;
    }

    inline bool operator == (const quat &q) const { return x == q.x && y == q.y && z == q.z && w == q.w; }
    inline bool operator != (const quat &v) const { return !(*this == v); }

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
        return v + xyz().cross(xyz().cross(v) + v * w) * 2.0f;
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

    enum ProjRange {
        PROJ_NEG_POS,
        PROJ_NEG_ZERO,
        PROJ_ZERO_POS,
    };

    float e00, e10, e20, e30,
          e01, e11, e21, e31,
          e02, e12, e22, e32,
          e03, e13, e23, e33;

    vec4& right()  const { return *((vec4*)&e00); }
    vec4& up()     const { return *((vec4*)&e01); }
    vec4& dir()    const { return *((vec4*)&e02); }
    vec4& offset() const { return *((vec4*)&e03); }

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

    void ortho(ProjRange range, float l, float r, float b, float t, float znear, float zfar, bool rotate90 = false) {
        identity();

        if (rotate90) {
            e00 = e11 = 0.0f;
            e10 = 2.0f / (l - r);
            e01 = 2.0f / (t - b);
        } else {
            e00 = 2.0f / (r - l);
            e11 = 2.0f / (t - b);
        }

        e03 = (l + r) / (l - r);
        e13 = (t + b) / (b - t);

        switch (range) {
            case PROJ_NEG_POS :
                e22 = 2.0f / (znear - zfar);
                e23 = (znear + zfar) / (znear - zfar);
                break;
            case PROJ_NEG_ZERO :
                e22 = 1.0f / (znear - zfar);
                e23 = (znear + zfar) / (znear - zfar) * 0.5f - 0.5f;
                e03 = -e03;
                break;
            case PROJ_ZERO_POS :
                e22 = 2.0f / (znear - zfar);
                e23 = znear / (znear - zfar);
                break;
        }
    }

    void frustum(ProjRange range, float l, float r, float b, float t, float znear, float zfar, bool rotate90 = false) {
        identity();

        if (rotate90) {
            e00 = e11 = 0.0f;
            e01 = 2.0f * znear / (r - l);
            e10 = 2.0f * znear / (b - t);
        } else {
            e00 = 2.0f * znear / (r - l);
            e11 = 2.0f * znear / (t - b);
        }

        e02 = (r + l) / (r - l);
        e12 = (t + b) / (t - b);
        e32 = -1.0f;
        e33 = 0.0f;

        switch (range) {
            case PROJ_NEG_POS :
                e22 = (znear + zfar) / (znear - zfar);
                e23 = 2.0f * zfar * znear / (znear - zfar);
                break;
            case PROJ_NEG_ZERO :
                e22 = znear / (znear - zfar);
                e23 = zfar * znear / (znear - zfar);
                break;
            case PROJ_ZERO_POS :
                e22 = zfar / (znear - zfar);
                e23 = znear * e22;
                break;
        }
    }

    void perspective(ProjRange range, float fov, float aspect, float znear, float zfar, float eye = 0.0f, bool rotate90 = false) {
        float y = tanf(fov * 0.5f * DEG2RAD) * znear;
        float x = y;
        float eyeX, eyeY;

        if (rotate90) {
            eyeX = 0.0f;
            eyeY = -eye;
            aspect = 1.0f / aspect;
            swap(x, y);
        } else {
            eyeX = eye;
            eyeY = 0.0f;
        }

        if (aspect >= 1.0f) {
            x = y * aspect;
        } else {
            y /= aspect;
        }

        frustum(range, -x - eyeX, x - eyeX, -y - eyeY, y - eyeY, znear, zfar, rotate90);
    }

    void viewport(float x, float y, float width, float height, float n, float f) {
        identity();
        e00 = width   * 0.5f;
        e11 = height  * 0.5f;
        e22 = (f - n) * 0.5f;
        e23 = (f + n) * 0.5f;
        e03 = x + e00;
        e13 = y + e11;
    }

    mat4(const vec3 &from, const vec3 &at, const vec3 &up) {
        vec3 r, u, d;
        d = (from - at).normal();
        r = up.cross(d).normal();
        u = d.cross(r);

        this->right()   = vec4(r, 0.0f);
        this->up()      = vec4(u, 0.0f);
        this->dir()     = vec4(d, 0.0f);
        this->offset()  = vec4(from, 1.0f);
    }

    mat4(const vec4 &reflectPlane) {
        float a = reflectPlane.x, 
              b = reflectPlane.y, 
              c = reflectPlane.z, 
              d = reflectPlane.w;

        right()  = vec4(1 - 2*a*a,   - 2*b*a,   - 2*c*a, 0);
        up()     = vec4(  - 2*a*b, 1 - 2*b*b,   - 2*c*b, 0);
        dir()    = vec4(  - 2*a*c,   - 2*b*c, 1 - 2*c*c, 0);
        offset() = vec4(  - 2*a*d,   - 2*b*d,   - 2*c*d, 1);
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
        float s, c;
        sincos(angle, &s, &c);
        m.e11 = c;  m.e21 = s;
        m.e12 = -s; m.e22 = c;
        *this = *this * m;
    }

    void rotateY(float angle) {
        mat4 m;
        m.identity();
        float s, c;
        sincos(angle, &s, &c);
        m.e00 = c;  m.e20 = -s;
        m.e02 = s;  m.e22 = c;
        *this = *this * m;
    }

    void rotateZ(float angle) {
        mat4 m;
        m.identity();
        float s, c;
        sincos(angle, &s, &c);
        m.e00 = c;  m.e01 = -s;
        m.e10 = s;  m.e11 = c;
        *this = *this * m;
    }

    void rotateYXZ(const vec3 &angle) {
        float s, c, a, b;

        if (angle.y != 0.0f) {
            sincos(angle.y, &s, &c);

            a = e00 * c - e02 * s;
            b = e02 * c + e00 * s;
            e00 = a;
            e02 = b;

            a = e10 * c - e12 * s;
            b = e12 * c + e10 * s;
            e10 = a;
            e12 = b;

            a = e20 * c - e22 * s;
            b = e22 * c + e20 * s;
            e20 = a;
            e22 = b;
        }

        if (angle.x != 0.0f) {
            sincos(angle.x, &s, &c);

            a = e01 * c + e02 * s;
            b = e02 * c - e01 * s;
            e01 = a;
            e02 = b;

            a = e11 * c + e12 * s;
            b = e12 * c - e11 * s;
            e11 = a;
            e12 = b;

            a = e21 * c + e22 * s;
            b = e22 * c - e21 * s;
            e21 = a;
            e22 = b;
        }

        if (angle.z != 0.0f) {
            sincos(angle.z, &s, &c);

            a = e00 * c + e01 * s;
            b = e01 * c - e00 * s;
            e00 = a;
            e01 = b;

            a = e10 * c + e11 * s;
            b = e11 * c - e10 * s;
            e10 = a;
            e11 = b;

            a = e20 * c + e21 * s;
            b = e21 * c - e20 * s;
            e20 = a;
            e21 = b;
        }
    }

    void lerp(const mat4 &m, float t) {
        e00 += (m.e00 - e00) * t;
        e01 += (m.e01 - e01) * t;
        e02 += (m.e02 - e02) * t;
        e03 += (m.e03 - e03) * t;

        e10 += (m.e10 - e10) * t;
        e11 += (m.e11 - e11) * t;
        e12 += (m.e12 - e12) * t;
        e13 += (m.e13 - e13) * t;

        e20 += (m.e20 - e20) * t;
        e21 += (m.e21 - e21) * t;
        e22 += (m.e22 - e22) * t;
        e23 += (m.e23 - e23) * t;
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

    mat4 inverseOrtho() const {
        mat4 r;
        r.e00 =  e00; r.e10 = e01; r.e20 = e02; r.e30 = 0;
        r.e01 =  e10; r.e11 = e11; r.e21 = e12; r.e31 = 0;
        r.e02 =  e20; r.e12 = e21; r.e22 = e22; r.e32 = 0;
        r.e03 = -(e03 * e00 + e13 * e10 + e23 * e20); // -dot(pos, right)
        r.e13 = -(e03 * e01 + e13 * e11 + e23 * e21); // -dot(pos, up)
        r.e23 = -(e03 * e02 + e13 * e12 + e23 * e22); // -dot(pos, dir)
        r.e33 = 1;
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
        return offset().xyz();
    }

    void setPos(const vec3 &pos) {
        offset().xyz() = pos;
    }

    void rot90()
    {
        swap(e00, e10);
        swap(e01, e11);
        swap(e02, e12);
        swap(e03, e13);

        e10 = -e10;
        e11 = -e11;
        e12 = -e12;
        e13 = -e13;
    }

    void unrot90()
    {
        e10 = -e10;
        e11 = -e11;
        e12 = -e12;
        e13 = -e13;

        swap(e00, e10);
        swap(e01, e11);
        swap(e02, e12);
        swap(e03, e13);
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
        if (t <= 0.0f) return *this;
        if (t >= 1.0f) return basis;
        Basis b;
        b.rot = rot.lerp(basis.rot, t);
        b.pos = pos.lerp(basis.pos, t);
        return b;
    }
};

struct ubyte2 {
    uint8 x, y;
};

union ubyte4 {
    struct { uint8 x, y, z, w; };
    uint32 value;

    ubyte4() {}
    ubyte4(uint8 x, uint8 y, uint8 z, uint8 w) : x(x), y(y), z(z), w(w) {}
};

struct short2 {
    int16 x, y;

    short2() {}
    short2(int16 x, int16 y) : x(x), y(y) {}

    inline bool operator == (const short2 &v) const { return x == v.x && y == v.y; }
};

struct short3 {
    int16 x, y, z;

    short3() {}
    short3(int16 x, int16 y, int16 z) : x(x), y(y), z(z) {}

    operator vec3() const { return vec3((float)x, (float)y, (float)z); };

    short3 operator + (const short3 &v) const { return short3(x + v.x, y + v.y, z + v.z); }
    short3 operator - (const short3 &v) const { return short3(x - v.x, y - v.y, z - v.z); }
    inline bool operator == (const short3 &v) const { return x == v.x && y == v.y && z == v.z; }
};

struct short4 {
    int16 x, y, z, w;

    short4() {}
    short4(int16 x, int16 y, int16 z, int16 w) : x(x), y(y), z(z), w(w) {}

    operator vec2()   const { return vec2((float)x, (float)y); };
    operator vec3()   const { return vec3((float)x, (float)y, (float)z); };
    operator vec4()   const { return vec4((float)x, (float)y, (float)z, (float)w); };

    operator short2() const { return *((short2*)this); }
    operator short3() const { return *((short3*)this); }

    inline bool operator == (const short4 &v) const { return x == v.x && y == v.y && z == v.z && w == v.w; }
    inline bool operator != (const short4 &v) const { return !(*this == v); }

    inline int16& operator [] (int index) const { ASSERT(index >= 0 && index <= 3); return ((int16*)this)[index]; }
};

quat rotYXZ(const vec3 &angle) {
    mat4 m;
    m.identity();
    m.rotateYXZ(angle);
    return m.getRot();
}

quat lerpAngle(const vec3 &a, const vec3 &b, float t) { // TODO: optimization

    return rotYXZ(a).lerp(rotYXZ(b), t);//.normal();
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
            res += v.xyz() /= v.w;
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
        mat4 mInv = matrix.inverseOrtho();
        return intersect(mInv * rayPos, (mInv * vec4(rayDir, 0)).xyz(), t);
    }
};

union Color32 { // RGBA8888
    uint32 value;
    struct { uint8 r, g, b, a; };

    Color32() {}
    Color32(uint32 value) : value(value) {}
    Color32(uint8 r, uint8 g, uint8 b, uint8 a) : r(r), g(g), b(b), a(a) {}

    void SetRGB15(uint16 v) {
        r = (v & 0x7C00) >> 7;
        g = (v & 0x03E0) >> 2;
        b = (v & 0x001F) << 3;
        a = 255;
    }

    static void YCbCr_T871_420(int32 Y0, int32 Y1, int32 Y2, int32 Y3, int32 Cb, int32 Cr, int32 F, Color32 &C0, Color32 &C1, Color32 &C2, Color32 &C3) {
        static const uint32 dither[8] = {
            0x00000600, 0x00060006, 0x00040204, 0x00020402,
            0x00000000, 0x00000000, 0x00000000, 0x00000000,
        };

        ASSERT(F == 0 || F == 4);

        int32 R = ( 91881  * Cr              ) >> 16;
        int32 G = ( 22550  * Cb + 46799 * Cr ) >> 16;
        int32 B = ( 116129 * Cb              ) >> 16;

        const Color32 *d = (Color32*)dither + F;

        C0.r = clamp(Y0 + R + d->r, 0, 255);
        C0.g = clamp(Y0 - G + d->g, 0, 255);
        C0.b = clamp(Y0 + B + d->b, 0, 255);
        C0.a = 255;
        d++;

        C1.r = clamp(Y1 + R + d->r, 0, 255);
        C1.g = clamp(Y1 - G + d->g, 0, 255);
        C1.b = clamp(Y1 + B + d->b, 0, 255);
        C1.a = 255;
        d++;

        C2.r = clamp(Y2 + R + d->r, 0, 255);
        C2.g = clamp(Y2 - G + d->g, 0, 255);
        C2.b = clamp(Y2 + B + d->b, 0, 255);
        C2.a = 255;
        d++;

        C3.r = clamp(Y3 + R + d->r, 0, 255);
        C3.g = clamp(Y3 - G + d->g, 0, 255);
        C3.b = clamp(Y3 + B + d->b, 0, 255);
        C3.a = 255;
    }
};

struct Color24 { // RGB888
    uint8 r, g, b;

    Color24() {}
    Color24(uint8 r, uint8 g, uint8 b) : r(r), g(g), b(b) {}

    operator Color32() const { return Color32(r, g, b, 255); }
};

union Color16 { // RGBA5551
    struct { uint16 b:5, g:5, r:5, a:1; };
    uint16 value;

    Color16() {}
    Color16(uint16 value) : value(value) {}

    operator Color24() const { return Color24((r << 3) | (r >> 2), (g << 3) | (g >> 2), (b << 3) | (b >> 2)); }
    operator Color32() const { return Color32((r << 3) | (r >> 2), (g << 3) | (g >> 2), (b << 3) | (b >> 2), -a); }
};

union ColorCLUT { // RGBA5551
    struct { uint16 r:5, g:5, b:5, a:1; };
    uint16 value;

    ColorCLUT() {}
    ColorCLUT(uint16 value) : value(value) {}

    operator Color24() const { return Color24((r << 3) | (r >> 2), (g << 3), (b << 3)); }
    operator Color32() const { return Color32((r << 3) | (r >> 2), (g << 3), (b << 3), -a); }
};

struct ColorIndex4 {
    uint8 a:4, b:4;
};

struct Tile4 {
    ColorIndex4 index[256 * 256 / 2];
};

struct Tile8 {
    uint8 index[256 * 256];
};

struct Tile16 {
    Color16 color[256 * 256];
};

#ifdef USE_ATLAS_RGBA16
union AtlasColor {
    struct { uint16 a:1, b:5, g:5, r:5; };
    uint16 value;

    AtlasColor() {}
    AtlasColor(uint16 value) : value(value) {}
    AtlasColor(const Color16 &value) : a(value.a), b(value.b), g(value.g), r(value.r) {}
    AtlasColor(const Color24 &value) : a(1), b(value.b >> 3), g(value.g >> 3), r(value.r >> 3) {}
    AtlasColor(const Color32 &value) : a(value.a ? 1 : 0), b(value.b >> 3), g(value.g >> 3), r(value.r >> 3) {}
    AtlasColor(const ColorCLUT &value) : a(value.a), b(value.b), g(value.g), r(value.r) {}
};

#define ATLAS_FORMAT FMT_RGBA16
#else
typedef Color32 AtlasColor;
#define ATLAS_FORMAT FMT_RGBA
#endif

struct AtlasTile {
    AtlasColor color[256 * 256];
};

struct Tile32 {
    Color32 color[256 * 256];
};

struct CLUT {
    ColorCLUT color[16];
};

namespace StrUtils {

    void toLower(char *str) {
        if (!str) return;

        while (char &c = *str++) {
            if (c >= 'A' && c <= 'Z')
                c -= 'Z' - 'z';
        }
    }

    char* copy(const char *str) {
        if (str == NULL) {
            return NULL;
        }
        char *res = new char[strlen(str) + 1];
        strcpy(res, str);
        return res;
    }

    int32 length(uint16 *str) {
        if (!str || !str[0]) return 0;
        int32 len = 0;
        while (*str++) {
            len++;
        }
        return len;
    }
}


template <typename T>
struct Array {
    int capacity;
    int length;
    T   *items;

    Array(int capacity = 32) : capacity(capacity), length(0), items(NULL) {}

    ~Array() { 
        clear();
    }

    void reserve(int capacity) {
        this->capacity = capacity;
        if (items)
            items = (T*)realloc(items, capacity * sizeof(T));
        else
            items = (T*)malloc(capacity * sizeof(T));
    }

    int push(const T &item) {
        if (!items)
            items = (T*)malloc(capacity * sizeof(T));

        if (length == capacity)
            reserve(capacity + capacity / 2);

        items[length] = item;
        return length++;
    }

    int pop() {
        ASSERT(length > 0);
        return --length;
    }

    void removeFast(int index) {
        (*this)[index] = (*this)[length - 1];
        length--;
    }

    void remove(int index) {
        length--;
        ASSERT(length >= 0);
        for (int i = index; i < length; i++)
            items[i] = items[i + 1];
    }

    void resize(int length) {
        if (capacity < length)
            reserve(length);
        this->length = length;
    }

    void reset() {
        length = 0;
    }

    void clear() {
        reset();
        free(items);
        items = NULL;
    }

    int find(const T &item) {
        for (int i = 0; i < length; i++) {
            if (items[i] == item)
                return i;
        }
        return -1;
    }

    void sort() {
        ::sort(items, length);
    }

    T& operator[] (int index) {
        ASSERT(index >= 0 && index < length);
        return items[index]; 
    };
};


struct Stream;

extern void osCacheWrite (Stream *stream);
extern void osCacheRead  (Stream *stream);

extern void osReadSlot   (Stream *stream);
extern void osWriteSlot  (Stream *stream);

#ifdef _OS_WEB
extern void osDownload   (Stream *stream);
#endif

char cacheDir[255];
char saveDir[255];
char contentDir[255];

#define STREAM_BUFFER_SIZE (16 * 1024)

#define MAX_PACKS 32

struct Stream {
    typedef void (Callback)(Stream *stream, void *userData);
    Callback    *callback;
    void        *userData;

    FILE        *f;
    char        *data;
    char        *name;
    int         size, pos, fpos;

    char        *buffer;
    int         bufferIndex;
    bool        buffering;
    uint32      baseOffset;

    struct Pack
    {
        Stream* stream;
        uint8*  table;
        uint32  count;

        struct FileInfo
        {
            uint32 size;
            uint32 offset;
        };

        bool findFile(const char* name, FileInfo &info)
        {
            if (!table || !name || !name[0]) {
                return false;
            }

            uint16 len = (uint16)strlen(name);
            uint8* ptr = table;

            for (uint32 i = 0; i < count; i++)
            {
                uint32 magic;
                memcpy(&magic, ptr, sizeof(magic));
                if (magic != 0x02014B50) {
                    ASSERT(false);
                    return false;
                }

                uint16 nameLen, extraLen, infoLen;
                memcpy(&nameLen, ptr + 28, sizeof(nameLen));
                memcpy(&extraLen, ptr + 30, sizeof(extraLen));

                if ((nameLen == len) && (memcmp(ptr + 46, name, len) == 0))
                {
                    uint16 compression;
                    memcpy(&compression, ptr + 10, sizeof(compression));

                    if (compression != 0)
                    {
                        ASSERT(false);
                        return false;
                    }

                    memcpy(&info.size,   ptr + 24, sizeof(info.size));
                    memcpy(&info.offset, ptr + 42, sizeof(info.offset));

                    stream->setPos(info.offset);
                    magic = stream->readLE32();

                    if (magic != 0x04034B50) {
                        ASSERT(false);
                        return false;
                    }
                    stream->seek(22);
                    nameLen = stream->readLE16();
                    extraLen = stream->readLE16();

                    info.offset += 4 + 22 + 2 + 2 + nameLen + extraLen; 

                    return true;
                }

                memcpy(&infoLen,  ptr + 32, sizeof(infoLen));

                ptr += 46 + nameLen + extraLen + infoLen;
            }

            return false;
        }

        Pack(const char *name) : stream(NULL), table(NULL), count(0)
        {
            stream = new Stream(name);
            stream->buffering = false;
            stream->setPos(stream->size - 22);
            uint32 magic = stream->readLE32();
            
            if (magic != 0x06054B50)
            {
                ASSERT(false);
                return;
            }

            stream->seek(6);
            count = stream->readLE16();
            uint32 tableSize = stream->readLE32();
            uint32 tableOffset = stream->readLE32();

            stream->setPos(tableOffset);

            table = new uint8[tableSize];
            stream->raw(table, tableSize);
        }

        ~Pack() {
            delete stream;
            delete[] table;
        }
    };

    static Pack* packs[MAX_PACKS];

    static Array<char*> fileList;

    static bool addPack(const char *name)
    {
        if (!existsContent(name)) {
            return false;
        }

        for (int i = 0; i < MAX_PACKS; i++)
        {
            if (!packs[i])
            {
                packs[i] = new Pack(name);
                return true;
            }
        }
        return false;
    }

    static void init() {
        readFileList();
    }

    static void deinit()
    {
        for (int i = 0; i < MAX_PACKS; i++)
        {
            if (!packs[i]) break;
            delete packs[i];
        }

        for (int i = 0; i < fileList.length; i++) {
            delete[] fileList[i];
        }
        fileList.clear();
    }

private:
#ifdef _OS_3DS
    static void streamThread(void *arg) {
        Stream* stream = (Stream*)arg;
        stream->openFile();
    }
#endif

    void openFile() {
        char path[255];

        path[0] = 0;
        if (contentDir[0] && (!cacheDir[0] || !strstr(name, cacheDir))) {
            strcpy(path, contentDir);
        }
        strcat(path, name);
        fixBackslash(path);

        f = fopen(path, "rb");

        if (!f) {
            #ifdef _OS_WEB
                osDownload(this);
            #else
                LOG("error loading file \"%s\"\n", name);
                if (callback) {
                    callback(NULL, userData);
                    delete this;
                } else {
                    ASSERT(false);
                }
            #endif
        } else {
            fseek(f, 0, SEEK_END);
            size = (int32)ftell(f);
            fseek(f, 0, SEEK_SET);

            fpos = 0;
            bufferIndex = -1;

            if (callback) callback(this, userData);
        }
    }
public:

    Stream(const char *name, const void *data, int size, Callback *callback = NULL, void *userData = NULL) : callback(callback), userData(userData), f(NULL), data((char*)data), name(NULL), size(size), pos(0), buffer(NULL) {
        this->name = StrUtils::copy(name);
    }

    Stream(const char *name, Callback *callback = NULL, void *userData = NULL) : callback(callback), userData(userData), f(NULL), data(NULL), name(NULL), size(-1), pos(0), buffer(NULL), buffering(true), baseOffset(0) {
        if (!name && callback) {
            callback(NULL, userData);
            delete this;
            return;
        }

        if (!name) {
            ASSERT(false);
        }

        Stream::Pack::FileInfo info;

        char path[256];

        for (int i = 0; i < MAX_PACKS; i++)
        {
            if (!packs[i]) break;

            if (packs[i]->findFile(name, info))
            {
                path[0] = 0;
                if (contentDir[0] && (!cacheDir[0] || !strstr(name, cacheDir))) {
                    strcpy(path, contentDir);
                }
                strcat(path, packs[i]->stream->name);
                fixBackslash(path);

                f = fopen(path, "rb");
                if (!f) {
                    LOG("error loading file from pack \"%s -> %s\"\n", packs[i]->stream->name, name);
                    ASSERT(false);
                    return;
                }
                baseOffset = info.offset;
                fseek(f, info.offset, SEEK_SET);
                size = info.size;

                fpos = 0;
                bufferIndex = -1;

                this->name = StrUtils::copy(name);
                if (callback) callback(this, userData);
                return;
            }
        }

        this->name = StrUtils::copy(name);

    #ifdef _OS_3DS /* TODO
        if (callback) {
            s32 priority = 0;
            svcGetThreadPriority(&priority, CUR_THREAD_HANDLE);
            threadCreate(streamThread, this, 64 * 1024, priority - 1, -2, false);
            return;
        }*/
    #endif

        openFile();
    }

    ~Stream() {
        delete[] name;
        delete[] buffer;
        if (f) fclose(f);
    }

#if _OS_3DS
    static void readDirectory(const FS_Archive &archive, const char* path) {
        char buf[255];
        strcpy(buf, contentDir + 5); // 5 to skip "sdmc:"
        strcat(buf, path);

        FS_Path fsPath = fsMakePath(FS_PathType::PATH_ASCII, buf);

        Handle dir;
        if (FSUSER_OpenDirectory(&dir, archive, fsPath) != 0) {
            return;
        }

        int32 pathLen = strlen(path);
        strcpy(buf, path);

        while (1) {
            FS_DirectoryEntry entry;

            u32 entriesRead = 0;
            FSDIR_Read(dir, &entriesRead, 1, &entry);

            if (entriesRead == 0) {
                break;
            }

            int32 len = utf16_to_utf8((uint8*)buf + pathLen, entry.name, StrUtils::length(entry.name));
            buf[pathLen + len] = 0;

            if (entry.attributes & FS_ATTRIBUTE_DIRECTORY) {
                strcat(buf, "/");
                readDirectory(archive, buf);
            } else {
                fileList.push(StrUtils::copy(buf));
            }
        }

        FSDIR_Close(dir);
    }

    static void readFileList() {
        FS_Archive sdmc;
        FS_Path fsRoot = fsMakePath(FS_PathType::PATH_ASCII, "");

        if (FSUSER_OpenArchive(&sdmc, FS_ArchiveID::ARCHIVE_SDMC, fsRoot) != 0) {
            return;
        }

        readDirectory(sdmc, "");

        FSUSER_CloseArchive(sdmc);
    }
#elif _OS_PSV
    static void readDirectory(char* path) {
        SceUID dd = sceIoDopen(path);

        size_t len = strlen(path);

        SceIoDirent entry;
        while (sceIoDread(dd, &entry) > 0)
        {
            strcat(path, entry.d_name);

            if (SCE_S_ISDIR(entry.d_stat.st_mode))
            {
                strcat(path, "/");
                readDirectory(path);
            } else {
                fileList.push(StrUtils::copy(path + strlen(contentDir)));
            }

            path[len] = 0;
        }

        sceIoClose(dd);
    }

    static void readFileList() {
        char path[255];
        strcpy(path, contentDir);
        readDirectory(path);
    }
#else
    static void readFileList() {};
#endif

    static void cacheRead(const char *name, Callback *callback = NULL, void *userData = NULL) {
        Stream *stream = new Stream(name, NULL, 0, callback, userData);
        #ifdef _OS_ANDROID // use saveDir for settings on android devices
            if (name && strcmp(name, "settings") == 0) {
                osReadSlot(stream);
                return;
            }
        #endif
        osCacheRead(stream);
    }

    static void cacheWrite(const char *name, const char *data, int size, Callback *callback = NULL, void *userData = NULL) {
        Stream *stream = new Stream(name, data, size, callback, userData);
        #ifdef _OS_ANDROID // use saveDir for settings on android devices
            if (name && strcmp(name, "settings") == 0) {
                osWriteSlot(stream);
                return;
            }
        #endif
        osCacheWrite(stream);
    }

    static void fixBackslash(char *str) {
    #ifdef _OS_XBOX
        int len = strlen(str);
        for (int i = 0; i < len; i++) {
            if (str[i] == '/') {
                str[i] = '\\';
            }
        }
    #endif
    }

    static bool exists(const char *name) {
        FILE *f = fopen(name, "rb");
        if (!f) return false;
        fclose(f);
        return true;
    }

    static bool existsContent(const char *name) {
        for (uint32 i = 0; i < MAX_PACKS; i++)
        {
            if (!packs[i]) break;

            Pack::FileInfo info;
            if (packs[i]->findFile(name, info))
                return true;
        }

        if (fileList.length) {
            for (int i = 0; i < fileList.length; i++) {
                if (strcmp(fileList[i], name) == 0) {
                    return true;
                }
            }
            return false;
        }

        char fileName[1024];
        strcpy(fileName, contentDir);
        strcat(fileName, name);
        fixBackslash(fileName);
        return exists(fileName);
    }

    void setPos(int pos) {
        this->pos = pos;
    }

    void seek(int offset) {
        pos += offset;
    }

    void raw(void *data, int count) {
        if (!count) return;

        if (f) {

            if (!buffering) {
                if (fpos != pos) {
                    fseek(f, baseOffset + pos, SEEK_SET);
                    fpos = pos;
                }
                fread(data, 1, count, f);
                pos += count;
                fpos += count;
                return;
            }

            uint8 *ptr = (uint8*)data;

            while (count > 0) {
                int bIndex = pos / STREAM_BUFFER_SIZE;

                if (bufferIndex != bIndex) {
                    bufferIndex = bIndex;

                    int readed;
                    int part;

                    if (fpos == pos) {
                        part = min(count / STREAM_BUFFER_SIZE * STREAM_BUFFER_SIZE, size - fpos);
                        if (part > STREAM_BUFFER_SIZE) {
                            readed = (int)fread(ptr, 1, part, f);

                            #ifdef TEST_SLOW_FIO
                                LOG("%s read %d + %d\n", name, fpos, readed);
                                Sleep(5);
                            #endif

                            ASSERT(part == readed);
                            count -= readed;
                            fpos  += readed;
                            pos   += readed;
                            ptr   += readed;

                            if (count <= 0) {
                                bufferIndex = -1;
                                break;
                            }

                            bufferIndex = pos / STREAM_BUFFER_SIZE;
                        }
                    }

                    if (fpos != bufferIndex * STREAM_BUFFER_SIZE) {
                        fpos = bufferIndex * STREAM_BUFFER_SIZE;
                        fseek(f, baseOffset + fpos, SEEK_SET);

                        #ifdef TEST_SLOW_FIO
                            LOG("%s seek %d\n", name, fpos);
                            Sleep(5);
                        #endif
                    }

                    if (!buffer) {
                        buffer = new char[STREAM_BUFFER_SIZE];
                    }

                    part   = min(STREAM_BUFFER_SIZE, size - fpos);
                    readed = (int)fread(buffer, 1, part, f);

                    #ifdef TEST_SLOW_FIO
                        LOG("%s read %d + %d\n", name, fpos, readed);
                        Sleep(5);
                    #endif

                    ASSERT(part == readed);
                    fpos += readed;
                }

                ASSERT(buffer);

                int bPos   = pos % STREAM_BUFFER_SIZE;
                int delta  = min(STREAM_BUFFER_SIZE - bPos, count);

                memcpy(ptr, buffer + bPos, delta);
                count -= delta;
                pos   += delta;
                ptr   += delta;
            }
        } else {
            memcpy(data, this->data + pos, count);
            pos += count;
        }
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

    inline uint8 read() {
        uint8 x;
        return read(x);
    }

    inline uint16 readLE16() {
        uint16 x;
        return read(x);
    }

    inline uint32 readLE32() {
        uint32 x;
        return read(x);
    }

    inline uint16 readBE16() {
        uint16 x;
        return swap16(read(x));
    }

    inline uint32 readBE32() {
        uint32 x;
        return swap32(read(x));
    }

    inline uint64 read64() {
        uint64 x;
        return read(x);
    }
};

Stream::Pack* Stream::packs[MAX_PACKS];
Array<char*> Stream::fileList;

#ifdef OS_FILEIO_CACHE
void osDataWrite(Stream *stream, const char *dir) {
    char path[255];
    strcpy(path, dir);
    strcat(path, stream->name);
    FILE *f = fopen(path, "wb");
    if (f) {
        fwrite(stream->data, 1, stream->size, f);
        fclose(f);
        if (stream->callback)
            stream->callback(new Stream(stream->name, stream->data, stream->size), stream->userData);
    } else
        if (stream->callback)
            stream->callback(NULL, stream->userData);

    delete stream;
}

void osDataRead(Stream *stream, const char *dir) {
    char path[255];
    strcpy(path, dir);
    strcat(path, stream->name);
    FILE *f = fopen(path, "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        int size = (int)ftell(f);
        fseek(f, 0, SEEK_SET);
        char *data = new char[size];
        fread(data, 1, size, f);
        fclose(f);
        if (stream->callback)
            stream->callback(new Stream(stream->name, data, size), stream->userData);
        delete[] data;
    } else
        if (stream->callback)
            stream->callback(NULL, stream->userData);
    delete stream;
}

void osCacheWrite(Stream *stream) {
    osDataWrite(stream, cacheDir);
}

void osCacheRead(Stream *stream) {
    osDataRead(stream, cacheDir);
}

void osWriteSlot(Stream *stream) {
    osDataWrite(stream, saveDir);
}

void osReadSlot(Stream *stream) {
    osDataRead(stream, saveDir);
}

#endif


#ifdef OS_PTHREAD_MT
#include <pthread.h>

// multi-threading
void* osMutexInit() {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

    pthread_mutex_t *mutex = new pthread_mutex_t();
    pthread_mutex_init(mutex, &attr);
    return mutex;
}

void osMutexFree(void *obj) {
    pthread_mutex_destroy((pthread_mutex_t*)obj);
    delete (pthread_mutex_t*)obj;
}

void osMutexLock(void *obj) {
    pthread_mutex_lock((pthread_mutex_t*)obj);
}

void osMutexUnlock(void *obj) {
    pthread_mutex_unlock((pthread_mutex_t*)obj);
}

void* osRWLockInit() {
    pthread_rwlock_t *lock = new pthread_rwlock_t();
    pthread_rwlock_init(lock, NULL);
    return lock;
}

void osRWLockFree(void *obj) {
    pthread_rwlock_destroy((pthread_rwlock_t*)obj);
    delete (pthread_rwlock_t*)obj;
}

void osRWLockRead(void *obj) {
    pthread_rwlock_rdlock((pthread_rwlock_t*)obj);
}

void osRWUnlockRead(void *obj) {
    pthread_rwlock_unlock((pthread_rwlock_t*)obj);
}

void osRWLockWrite(void *obj) {
    pthread_rwlock_wrlock((pthread_rwlock_t*)obj);
}

void osRWUnlockWrite(void *obj) {
    pthread_rwlock_unlock((pthread_rwlock_t*)obj);
}
#endif


static const uint32 BIT_MASK[] = {
    0x00000000,
    0x00000001, 0x00000003, 0x00000007, 0x0000000F,
    0x0000001F, 0x0000003F, 0x0000007F, 0x000000FF,
    0x000001FF, 0x000003FF, 0x000007FF, 0x00000FFF,
    0x00001FFF, 0x00003FFF, 0x00007FFF, 0x0000FFFF,
    0x0001FFFF, 0x0003FFFF, 0x0007FFFF, 0x000FFFFF,
    0x001FFFFF, 0x003FFFFF, 0x007FFFFF, 0x00FFFFFF,
    0x01FFFFFF, 0x03FFFFFF, 0x07FFFFFF, 0x0FFFFFFF,
    0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF, 0xFFFFFFFF,
};

// TODO: refactor for LE, BE, byte and word reading
struct BitStream {
    uint8  *data;
    uint8  *end;
    uint16 index;
    uint16 value;

    BitStream(uint8 *data, int size) : data(data), end(data + size), index(0), value(0) {}

    inline uint32 readBit() {
        uint32 bit;

        if (!index--) {
            value = *data++;
            index = 7;
        }

        bit = value & 1;
        value >>= 1;

        return bit;
    }

    uint32 read(int count) {
        uint32 bits = 0, mask = 1;

        if (index == 0) {
            if (count > 7) {
                count -= 8;
                mask <<= 8;
                bits = *data++;
            }
        }

        while (count--) {
            if (readBit())
                bits += mask;
            mask <<= 1;
        }

        return bits;
    }

    uint8 readBE(int count) {
        uint32 bits = 0;

        while (count--) {
            if (!index) {
                ASSERT(data < end);
                value = *data++;
                index = 8;
            }

            bits <<= 1;

            if (value & 0x80)
                bits |= 1;

            value <<= 1;
            index--;
        }

        return bits;
    }

    uint8 readBitBE() {
        return readBE(1);
    }

    uint8 readByte() {
        ASSERT(data < end);
        return *data++;
    }

    uint32 readWord(bool littleEndian) {
        uint8 a, b;
        if (littleEndian) {
            a = data[0];
            b = data[1];
        } else {
            b = data[0];
            a = data[1];
        }
        data += 2;
        return a + (b << 8);
    }

    uint32 readU(int count) {
        if (!index) {
            value = readWord(true);
            index = 16;
        }

        uint32 bits;
        if (count <= index) {
            bits = (value >> (index - count)) & BIT_MASK[count];
            index -= count;
        } else {
            bits = value & BIT_MASK[index];
            count -= index;
            index = 0;

            while (count >= 16) {
                bits = (bits << 16) | readWord(true);
                count -= 16;
            }

            if (count > 0) {
                value = readWord(true);
                index = 16 - count;
                bits = (bits << count) | (value >> index);
            }
        }

        return bits;
    }

    void skip(int count) {
        readU(count);
    }
};


template <int N>
struct FixedStr {
    char data[N];

    void get(char *dst) {
        memcpy(dst, data, sizeof(data));
        dst[sizeof(data)] = 0;
    }

    FixedStr<N>& operator = (const char *str) {
        size_t len = min(sizeof(data), strlen(str));
        memset(data, 0, sizeof(data));
        memcpy(data, str, len);
        return *this;
    }
};

typedef FixedStr<16> str16;

#endif
