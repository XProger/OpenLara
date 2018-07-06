#ifndef H_VIDEO
#define H_VIDEO

#include "utils.h"
#include "texture.h"
#include "sound.h"

struct Video {

    union Color32 {
        uint32 value;
        struct { uint8 r, g, b, a; };

        void SetRGB15(uint16 v) {
            r = (v & 0x7C00) >> 7;
            g = (v & 0x03E0) >> 2;
            b = (v & 0x001F) << 3;
            a = 255;
        }

        void SetYCbCr(int32 Y, int32 Cb, int32 Cr) {
            Y = max(0, 1191 * (Y - 16));
            Cb -= 128;
            Cr -= 128;
            r = clamp((Y + 1836 * Cr) >> 10, 0, 255);
            g = clamp((Y - 547 * Cr - 218 * Cb) >> 10, 0, 255);
            b = clamp((Y + 2165 * Cb) >> 10, 0, 255);
            a = 255;
        }

        void SetYUV(int32 Y, int32 U, int32 V) {
            r = clamp(Y + (74698 * V >> 16), 0, 255);
            g = clamp(Y - ((25863 * U + 38049 * V) >> 16), 0, 255);
            b = clamp(Y + (133174 * U >> 16), 0, 255);
            a = 255;
        }
    };

    struct Decoder : Sound::Decoder {
        int width, height, fps;

        Decoder(Stream *stream) : Sound::Decoder(stream, 2, 0) {}
        virtual ~Decoder() { /* delete stream; */ }
        virtual bool decode(uint8 *frame) { return false; }
    };

    // based on ffmpeg code https://github.com/FFmpeg/FFmpeg/blob/master/libavcodec/escape124.c
    struct Escape : Decoder {
        int vfmt, bpp;
        int sfmt, rate, channels, bps;
        int framesCount, chunksCount, offset;
        int curVideoPos, curVideoChunk;
        int curAudioPos, curAudioChunk;

        Sound::Decoder *audioDecoder;

        uint8 *prevFrame, *nextFrame, *lumaFrame;

        struct Chunk {
            int offset;
            int videoSize;
            int audioSize;
        } *chunks;

        union MacroBlock {
            uint32 pixels[4];
        };

        union SuperBlock {
            uint32 pixels[64];
        };

        struct Codebook {
            uint32      size;
            uint32      depth;
            MacroBlock  *blocks;
        } codebook[3];

        Escape(Stream *stream) : Decoder(stream), audioDecoder(NULL), prevFrame(NULL), nextFrame(NULL), lumaFrame(NULL), chunks(NULL) {
            for (int i = 0; i < 4; i++)
                skipLine();

            vfmt        = readValue();      // video format
            width       = readValue();      // x size in pixels
            height      = readValue();      // y size in pixels
            bpp         = readValue();      // bits per pixel RGB
            fps         = readValue();      // frames per second
            sfmt        = readValue();      // sound format
            rate        = readValue();      // Hz Samples
            channels    = readValue();      // channel
            bps         = readValue();      // bits per sample (LINEAR UNSIGNED)
            framesCount = readValue();      // frames per chunk
            chunksCount = readValue() + 1;  // number of chunks
            skipLine();                     // even chunk size
            skipLine();                     // odd chunk size
            offset      = readValue();      // offset to chunk cat
            skipLine();                     // offset to sprite
            skipLine();                     // size of sprite
            skipLine();                     // offset to key frames

            stream->setPos(offset);

            chunks = new Chunk[chunksCount];
            for (int i = 0; i < chunksCount; i++) {
                chunks[i].offset    = readValue();
                chunks[i].videoSize = readValue();
                chunks[i].audioSize = readValue();
            }

            switch (vfmt) {
                case 124 :
                    prevFrame = new uint8[width * height * 4];
                    nextFrame = new uint8[width * height * 4];
                    memset(prevFrame, 0, width * height * sizeof(uint32));
                    memset(nextFrame, 0, width * height * sizeof(uint32));
                    break;
                case 130 :
                    prevFrame = new uint8[width * height * 3 / 2];
                    nextFrame = new uint8[width * height * 3 / 2];
                    lumaFrame = new uint8[width * height / 4];
                    memset(prevFrame, 0, width * height);
                    memset(prevFrame + width * height, 16, width * height / 2);
                    break;
                default  :
                    LOG("! unsupported Escape codec version (%d)\n", vfmt);
                    ASSERT(false);
            }

            codebook[0].blocks =
            codebook[1].blocks =
            codebook[2].blocks = NULL;

            curVideoPos = curVideoChunk = 0; 
            curAudioPos = curAudioChunk = 0;

            if (sfmt == 1)
                audioDecoder = new Sound::PCM(stream, channels, rate, 0, bps);      // TR2
            else if (sfmt == 101)
                if (bps == 8)
                    audioDecoder = new Sound::PCM(stream, channels, rate, 0, bps);  // TR1
                else
                    audioDecoder = new Sound::IMA(stream, channels, rate);          // TR3
        }

        virtual ~Escape() {
            if (audioDecoder) {
                audioDecoder->stream = NULL;
                delete audioDecoder;
            }
            delete[] chunks;
            delete[] codebook[0].blocks;
            delete[] codebook[1].blocks;
            delete[] codebook[2].blocks;
            delete[] prevFrame;
            delete[] nextFrame;
            delete[] lumaFrame;
        }

        void skipLine() {
            char c;
            while (stream->read(c) != '\n');
        }

        int readValue() {
            char buf[255];
            for (int i = 0; i < sizeof(buf); i++) {
                char &c = buf[i];
                stream->read(c);
                if (c == ' ' || c == '.' || c == ',' || c == ';' || c == '\n') {
                    if (c == ' ' || c == '.')
                        skipLine();
                    c = '\0';
                    return atoi(buf);
                }
            }
            ASSERT(false);
            return 0;
        }

        int getSkip124(BitStream &bs) {
            int value;

            if ((value  = bs.read(1)) != 1 ||
                (value += bs.read(3)) != 8 ||
                (value += bs.read(7)) != 135)
                return value;

            return value + bs.read(12);
        }

        int getSkip130(BitStream &bs) {
            int value;

            if (value = bs.read(1))  return 0;
            if (value = bs.read(3))  return value;
            if (value = bs.read(8))  return value + 7;
            if (value = bs.read(15)) return value + 262;

            return -1;
        }

        void copySuperBlock(uint32 *dst, int dstWidth, uint32 *src, int srcWidth) {
            for (int i = 0; i < 8; i++) {
                memcpy(dst, src, 8 * sizeof(uint32));
                src += srcWidth;
                dst += dstWidth;
            }
        }

        void decodeMacroBlock(BitStream &bs, MacroBlock &mb, int &cbIndex, int sbIndex) {
            int value = bs.read(1);
            if (value) {
                static const int8 trans[3][2] = { {2, 1}, {0, 2}, {1, 0} };
                value = bs.read(1);
                cbIndex = trans[cbIndex][value];
            }

            Codebook &cb = codebook[cbIndex];
            uint32 bIndex = bs.read(cb.depth);

            if (cbIndex == 1)
                bIndex += sbIndex << cb.depth;

            memcpy(&mb, cb.blocks + bIndex, sizeof(mb));
        }

        void insertMacroBlock(SuperBlock &sb, const MacroBlock &mb, int index) {
            uint32 *dst = sb.pixels + (index + (index & -4)) * 2;
            dst[0] = mb.pixels[0];
            dst[1] = mb.pixels[1];
            dst[8] = mb.pixels[2];
            dst[9] = mb.pixels[3];
        }

        bool decode(uint8 *frame) {
            if (curVideoChunk >= chunksCount)
                return false;

            if (curVideoPos >= chunks[curVideoChunk].videoSize) {
                curVideoChunk++;
                curVideoPos = 0;
                if (curVideoChunk >= chunksCount)
                    return false;
            }
            stream->setPos(chunks[curVideoChunk].offset + curVideoPos);

            switch (vfmt) {
                case 124 : return decode124(frame);
                case 130 : return decode130(frame);
                default  : ASSERT(false);
            }

            return false;
        }

        bool decode124(uint8 *frame) {
            uint32 flags, size;
            stream->read(flags);
            stream->read(size);
            
            curVideoPos += size;

        // skip unchanged frame
            if (!(flags & 0x114) || !(flags & 0x7800000))
                return true;

            int sbCount = (width / 8) * (height / 8);

        // read data into bit stream
            size -= (sizeof(flags) + sizeof(size));

            uint8 *data = new uint8[size];
            stream->raw(data, size);
            BitStream bs(data, size);

        // read codebook changes
            for (int i = 0; i < 3; i++) {
                if (flags & (1 << (17 + i))) {

                    Codebook &cb = codebook[i];

                    if (i == 2) {
                        cb.size  = bs.read(20);
                        cb.depth = log2i(cb.size - 1) + 1;
                    } else {
                        cb.depth = bs.read(4);
                        cb.size  = (i == 0 ? 1 : sbCount) << cb.depth;
                    }

                    delete[] cb.blocks;
                    cb.blocks = new MacroBlock[cb.size];

                    for (uint32 j = 0; j < cb.size; j++) {
                        uint8  mask = bs.read(4);
                        Color32 cA, cB;
                        cA.SetRGB15(bs.read(15));
                        cB.SetRGB15(bs.read(15));

                        if (cA.value != cB.value && (mask == 6 || mask == 9) && // check for 0101 or 1010 mask
                            abs(int(cA.r) - int(cB.r)) <= 8 &&
                            abs(int(cA.g) - int(cB.g)) <= 8 &&
                            abs(int(cA.b) - int(cB.b)) <= 8) {

                            cA.r = (int(cA.r) + int(cB.r)) / 2;
                            cA.g = (int(cA.g) + int(cB.g)) / 2;
                            cA.b = (int(cA.b) + int(cB.b)) / 2;

                            cB = cA;
                        }

                        for (int k = 0; k < 4; k++)
                            cb.blocks[j].pixels[k] = (mask & (1 << k)) ? cB.value : cA.value;
                    }
                }
            }

            static const uint16 maskMatrix[] = { 0x1,   0x2,   0x10,   0x20,
                                                 0x4,   0x8,   0x40,   0x80,
                                                 0x100, 0x200, 0x1000, 0x2000,
                                                 0x400, 0x800, 0x4000, 0x8000};

            SuperBlock sb;
            MacroBlock mb;
            int cbIndex = 1;

            int skip = -1;
            for (int sbIndex = 0; sbIndex < sbCount; sbIndex++) {
                int sbLine   = width / 8;
                int sbOffset = ((sbIndex / sbLine) * width + (sbIndex % sbLine)) * 8;
                uint32 *src = (uint32*)prevFrame + sbOffset;
                uint32 *dst = (uint32*)nextFrame + sbOffset;

                uint16 multiMask = 0;

                if (skip == -1)
                    skip = getSkip124(bs);

                if (skip) {
                    copySuperBlock(dst, width, src, width);
                } else {
                    copySuperBlock(sb.pixels, 8, src, width);

                    while (!bs.read(1)) {
                        decodeMacroBlock(bs, mb, cbIndex, sbIndex);
                        uint16 mask = bs.read(16);
                        multiMask |= mask;
                        for (int i = 0; i < 16; i++)
                            if (mask & maskMatrix[i])
                                insertMacroBlock(sb, mb, i);
                    }

                    if (!bs.read(1)) {
                        uint16 invMask = bs.read(4);
                        for (int i = 0; i < 4; i++)
                            multiMask ^= ((invMask & (1 << i)) ? 0x0F : bs.read(4)) << (i * 4);
                        for (int i = 0; i < 16; i++)
                            if (multiMask & maskMatrix[i]) {
                                decodeMacroBlock(bs, mb, cbIndex, sbIndex);
                                insertMacroBlock(sb, mb, i);
                            }
                    } else 
                        if (flags & (1 << 16))
                            while (!bs.read(1)) {
                                decodeMacroBlock(bs, mb, cbIndex, sbIndex);
                                insertMacroBlock(sb, mb, bs.read(4));
                            }

                    copySuperBlock(dst, width, sb.pixels, 8);
                }

                skip--;
            }

            delete[] data;

            memcpy(frame, nextFrame, width * height * 4);
            swap(prevFrame, nextFrame);

            return true;
        }

        bool decode130(uint8 *frame) {

            static const uint8 offsetLUT[] = { 
                2, 4, 10, 20
            };

            static const int8 signLUT[64][4] = {
                {  0,  0,  0,  0 }, { -1,  1,  0,  0 }, {  1, -1,  0,  0 }, { -1,  0,  1,  0 },
                { -1,  1,  1,  0 }, {  0, -1,  1,  0 }, {  1, -1,  1,  0 }, { -1, -1,  1,  0 },
                {  1,  0, -1,  0 }, {  0,  1, -1,  0 }, {  1,  1, -1,  0 }, { -1,  1, -1,  0 },
                {  1, -1, -1,  0 }, { -1,  0,  0,  1 }, { -1,  1,  0,  1 }, {  0, -1,  0,  1 },
                {  0,  0,  0,  0 }, {  1, -1,  0,  1 }, { -1, -1,  0,  1 }, { -1,  0,  1,  1 },
                { -1,  1,  1,  1 }, {  0, -1,  1,  1 }, {  1, -1,  1,  1 }, { -1, -1,  1,  1 },
                {  0,  0, -1,  1 }, {  1,  0, -1,  1 }, { -1,  0, -1,  1 }, {  0,  1, -1,  1 },
                {  1,  1, -1,  1 }, { -1,  1, -1,  1 }, {  0, -1, -1,  1 }, {  1, -1, -1,  1 },
                {  0,  0,  0,  0 }, { -1, -1, -1,  1 }, {  1,  0,  0, -1 }, {  0,  1,  0, -1 },
                {  1,  1,  0, -1 }, { -1,  1,  0, -1 }, {  1, -1,  0, -1 }, {  0,  0,  1, -1 },
                {  1,  0,  1, -1 }, { -1,  0,  1, -1 }, {  0,  1,  1, -1 }, {  1,  1,  1, -1 },
                { -1,  1,  1, -1 }, {  0, -1,  1, -1 }, {  1, -1,  1, -1 }, { -1, -1,  1, -1 },
                {  0,  0,  0,  0 }, {  1,  0, -1, -1 }, {  0,  1, -1, -1 }, {  1,  1, -1, -1 },
                { -1,  1, -1, -1 }, {  1, -1, -1, -1 }, {  0,  0,  0,  0 }, {  0,  0,  0,  0 },
                {  0,  0,  0,  0 }, {  0,  0,  0,  0 }, {  0,  0,  0,  0 }, {  0,  0,  0,  0 },
                {  0,  0,  0,  0 }, {  0,  0,  0,  0 }, {  0,  0,  0,  0 }, {  0,  0,  0,  0 },
            };

            static const int8 lumaLUT[] = {
                -4, -3, -2, -1, 1, 2, 3, 4
            };

            static const int8 chromaLUT[2][8] = {
                { 1, 1, 0, -1, -1, -1,  0,  1 },
                { 0, 1, 1,  1,  0, -1, -1, -1 }
            };

            static const uint8 chromaValueLUT[] = {
                 20,  28,  36,  44,  52,  60,  68,  76,
                 84,  92, 100, 106, 112, 116, 120, 124,
                128, 132, 136, 140, 144, 150, 156, 164,
                172, 180, 188, 196, 204, 212, 220, 228
            };

            Chunk &chunk = chunks[curVideoChunk++];

            uint8 *data = new uint8[chunk.videoSize];
            stream->raw(data, chunk.videoSize);
            BitStream bs(data, chunk.videoSize);
            bs.data += 16; // skip 16 bytes

            uint8 *lumaPtr = lumaFrame;

            int skip = -1;
            int bCount = width * height / 4;
            uint32 luma = 0, Y[4] = { 0 }, U = 16, V = 16;

            uint8 *oY = prevFrame, *oU = oY + width * height, *oV = oU + width * height / 4;
            uint8 *nY = nextFrame, *nU = nY + width * height, *nV = nU + width * height / 4;

            for (int bIndex = 0; bIndex < bCount; bIndex++) {
                if (skip == -1)
                    skip = getSkip130(bs);

                if (skip) {
                    Y[0] = oY[0];
                    Y[1] = oY[1];
                    Y[2] = oY[width];
                    Y[3] = oY[width + 1];
                    U    = oU[0];
                    V    = oV[0];
                    luma = *lumaPtr;
                } else {
                    if (bs.read(1)) {
                        uint32 sign = bs.read(6);
                        uint32 diff = bs.read(2);

                        luma = bs.read(5) * 2;

                        for (int i = 0; i < 4; i++)
                            Y[i] = clamp(luma + offsetLUT[diff] * signLUT[sign][i], 0U, 63U);

                    } else if (bs.read(1)) {
                        luma = bs.read(1) ? bs.read(6) : ((luma + lumaLUT[bs.read(3)]) & 63);

                        for (int i = 0; i < 4; i++)
                            Y[i] = luma;
                    }

                    if (bs.read(1)) {
                        if (bs.read(1)) {
                            U = bs.read(5);
                            V = bs.read(5);
                        } else {
                            uint32 idx = bs.read(3);
                            U = (U + chromaLUT[0][idx]) & 31;
                            V = (V + chromaLUT[1][idx]) & 31;
                        }
                    }
                }
                *lumaPtr++ = luma;

                nY[0]         = Y[0];
                nY[1]         = Y[1];
                nY[width]     = Y[2];
                nY[width + 1] = Y[3];
                nU[0]         = U;
                nV[0]         = V;

                nY += 2; nU++; nV++;
                oY += 2; oU++; oV++;

                if (!(((bIndex + 1) * 2) % width)) {
                    nY += width;
                    oY += width;
                }

                skip--;
            }

            delete[] data;

            nY  = nextFrame;
            nU = nY  + width * height;
            nV = nU + width * height / 4;

            Color32 *p = (Color32*)frame;
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    int Y = nY[y * width + x] << 2;
                    int U = chromaValueLUT[nU[x / 2]] - 128;
                    int V = chromaValueLUT[nV[x / 2]] - 128;
                    (p++)->SetYUV(Y, U, V);
                }

                if (y & 1) {
                    nU += width / 2;
                    nV += width / 2;
                }
            }

            swap(prevFrame, nextFrame);

            return true;
        }

        virtual int decode(Sound::Frame *frames, int count) {
            if (!audioDecoder) return 0;

            if (bps != 4 && abs(curAudioChunk - curVideoChunk) > 1) { // sync with video chunk, doesn't work for IMA
                curAudioChunk = curVideoChunk;
                curAudioPos   = 0;
            }

            int i = 0;
            while (i < count) {
                if (curAudioChunk >= chunksCount) {
                    memset(&frames[i], 0, sizeof(Sound::Frame) * (count - i));
                    break;
                }

                Chunk *chunk = &chunks[curAudioChunk];

                if (curAudioPos >= chunk->audioSize) {
                    curAudioPos = 0;
                    curAudioChunk++;
                    continue;
                }

                stream->setPos(chunk->offset + chunk->videoSize + curAudioPos);

                int part = min(count - i, (chunk->audioSize - curAudioPos) / (channels * bps / 8));

                int pos = stream->pos;

                while (part > 0) { 
                    int res = audioDecoder->decode(&frames[i], part);
                    i += res;
                    part -= res;
                }

                curAudioPos += stream->pos - pos;
            }

            return count;
        }
    };

    Decoder *decoder;
    Texture *frameTex[2];
    uint8   *frameData;
    float   time, step;
    bool    isPlaying;
    bool    needUpdate;
    Sound::Sample *sample;

    Video(Stream *stream) : decoder(NULL), time(0.0f), isPlaying(false) {
        frameTex[0] = frameTex[1] = NULL;

        if (!stream) return;

        decoder   = new Escape(stream);
        frameData = new uint8[decoder->width * decoder->height * 4];
        memset(frameData, 0, decoder->width * decoder->height * 4);

        for (int i = 0; i < 2; i++)
            frameTex[i] = new Texture(decoder->width, decoder->height, FMT_RGBA, 0, frameData);

        sample = Sound::play(decoder);

        step      = 1.0f / decoder->fps;
        time      = step;
        isPlaying = true;
    }

    ~Video() {
        sample->decoder = NULL;
        sample->stop();
        delete decoder;
        delete frameTex[0];
        delete frameTex[1];
        delete[] frameData;
    }

    void update() {
        if (!isPlaying) return;

        time += Core::deltaTime;
        if (time < step)
            return;
        time -= step;

        isPlaying = needUpdate = decoder->decode(frameData);
    }

    void render() { // just update GPU texture if it's necessary
        if (!needUpdate) return;
        frameTex[0]->update(frameData);
        swap(frameTex[0], frameTex[1]);
        needUpdate = false;
    }
};

#endif
