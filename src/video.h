#ifndef H_VIDEO
#define H_VIDEO

#include "utils.h"
#include "texture.h"
#include "sound.h"

struct Video {

    struct Decoder : Sound::Decoder {
        int width, height, fps;

        Decoder(Stream *stream) : Sound::Decoder(stream, 2) {}
        virtual ~Decoder() { /* delete stream; */ }
        virtual bool decode(uint8 *frame) { return false; }
    };

    // based on ffmpeg code https://github.com/FFmpeg/FFmpeg/blob/master/libavcodec/escape124.c
    struct Escape124 : Decoder {
        int vfmt, bpp;
        int sfmt, rate, channels, bps;
        int framesCount, chunksCount, offset;
        int curVideoPos, curVideoChunk;
        int curAudioPos, curAudioChunk;

        uint32 *prevFrame, *nextFrame;
        Sound::Frame prevSample;

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

        Escape124(Stream *stream) : Decoder(stream), chunks(NULL) {
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

            prevFrame = new uint32[width * height];
            nextFrame = new uint32[width * height];
            memset(prevFrame, 0, width * height * sizeof(uint32));
            memset(nextFrame, 0, width * height * sizeof(uint32));

            codebook[0].blocks =
            codebook[1].blocks =
            codebook[2].blocks = NULL;

            curVideoPos = curVideoChunk = 0; 
            curAudioPos = curAudioChunk = 0;

            prevSample.L = prevSample.R = 0;
        }

        virtual ~Escape124() {
            delete[] chunks;
            delete[] codebook[0].blocks;
            delete[] codebook[1].blocks;
            delete[] codebook[2].blocks;
            delete[] prevFrame;
            delete[] nextFrame;
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

        void copySuperBlock(uint32 *dst, int dstWidth, uint32 *src, int srcWidth) {
            for (int i = 0; i < 8; i++) {
                memcpy(dst, src, 8 * sizeof(uint32));
                src += srcWidth;
                dst += dstWidth;
            }
        }

        int getSkipCount(BitStream &bs) {
            int value;

            value = bs.read(1);
            if (!value)
                return value;

            value += bs.read(3);
            if (value != (1 + ((1 << 3) - 1)))
                return value;

            value += bs.read(7);
            if (value != (1 + ((1 << 3) - 1)) + ((1 << 7) - 1))
                return value;

            return value + bs.read(12);
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

        union Color32 {
            uint32 value;
            struct { uint8 r, g, b, a; };

            Color32() {}

            Color32(uint16 v) {
                r = (v & 0x7C00) >> 7;
                g = (v & 0x03E0) >> 2;
                b = (v & 0x001F) << 3;
                a = 255;
            }
        };

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
                        Color32 cA = Color32(uint16(bs.read(15)));
                        Color32 cB = Color32(uint16(bs.read(15)));

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
                uint32 *src = prevFrame + sbOffset;
                uint32 *dst = nextFrame + sbOffset;

                uint16 multiMask = 0;

                if (skip == -1)
                    skip = getSkipCount(bs);

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

        virtual int decode(Sound::Frame *frames, int count) {
            if (abs(curAudioChunk - curVideoChunk) > 1) { // sync with video chunk
                curAudioChunk = curVideoChunk;
                curAudioPos   = 0;
            }

            if (curAudioChunk >= chunksCount) {
                for (int i = 0; i < count; i++)
                    frames[i].L = frames[i].R = 0;
                return count;
            }

            Chunk *chunk = &chunks[curAudioChunk];
            stream->setPos(chunk->offset + chunk->videoSize + curAudioPos);

            int sampleSize = channels * (bps / 8);

            for (int i = 0; i < count; i++) {

                if (curAudioChunk >= chunksCount) {
                    frames[i].L = frames[i].R = 0;
                    continue;
                }

                if (sfmt == 101) {
                    ubyte2 sample;
                    stream->raw(&sample, sizeof(sample));
                    frames[i].L = uint16(sample.x) << 7;
                    frames[i].R = uint16(sample.y) << 7;
                } else if (sfmt == 1) {
                    Sound::Frame sample;

                    stream->raw(&sample, sizeof(Sound::Frame));

                    frames[i].L = (int(prevSample.L) + int(sample.L)) / 2;
                    frames[i].R = (int(prevSample.R) + int(sample.R)) / 2;
                    i++;
                    frames[i] = prevSample = sample;
                }

                curAudioPos += sampleSize;
                if (curAudioPos >= chunk->audioSize) {
                    curAudioPos = 0;
                    curAudioChunk++;
                    if (curAudioChunk < chunksCount) {
                        chunk = &chunks[curAudioChunk];
                        stream->setPos(chunk->offset + chunk->videoSize);
                    }
                }
            }

            return count;
        }
    };

    Decoder *decoder;
    Texture *frameTex[2];
    uint8   *frameData;
    float   time, step;
    float   invFPS;
    bool    isPlaying;
    bool    needUpdate;
    Sound::Sample *sample;

    Video(Stream *stream) : decoder(NULL), time(0.0f), isPlaying(false) {
        frameTex[0] = frameTex[1] = NULL;

        if (!stream) return;

        decoder   = new Escape124(stream);
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
