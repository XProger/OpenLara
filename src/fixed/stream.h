#ifndef H_STREAM
#define H_STREAM

#include "common.h"

struct DataStream
{
    const uint8* data;
    int32 size;
    int32 pos;
    bool bigEndian;

    DataStream(const uint8* data, int32 size) : data(data), size(size), pos(0), bigEndian(false)
    {
        //
    }

    const uint8* getPtr()
    {
        return data + pos;
    }

    int32 getPos()
    {
        return pos;
    }

    void setPos(int32 newPos)
    {
        pos = newPos;
    }

    void seek(int32 offset)
    {
        pos += offset;
    }

    X_INLINE uint8 read8u()
    {
        return data[pos++];
    }

    X_INLINE int8 read8s()
    {
        return (int8)read8u();
    }

    uint16 read16u()
    {
        const uint8* ptr = data + pos;
        pos += 2;

        uint16 n;
        if (intptr_t(ptr) & 1) {
            n = ptr[0] | (ptr[1] << 8);
        } else {
            n = *(uint16*)ptr;
        }

        return bigEndian ? swap16(n) : n;
    }

    X_INLINE int16 read16s()
    {
        return (int16)read16u();
    }

    uint32 read32u()
    {
        const uint8* ptr = data + pos;
        pos += 4;

        uint32 n;
        if (intptr_t(ptr) & 3) {
            n = ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);
        } else {
            n = *(uint32*)ptr;
        }

        return bigEndian ? swap32(n) : n;
    }

    X_INLINE int32 read32s()
    {
        return (int32)read32u();
    }
};

#endif
