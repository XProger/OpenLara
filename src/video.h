#ifndef H_VIDEO
#define H_VIDEO

#include "utils.h"
#include "texture.h"
#include "sound.h"

struct AC_ENTRY {
    uint8 code;
    uint8 skip;
    uint8 ac;
    uint8 length;
};

// ISO 13818-2 table B-14
static const AC_ENTRY STR_AC[] = {
// signBit = (8 + shift) - length
// AC_LUT_1 (shift = 1)
    { 0XC0 , 1  , 1  , 4  }, // 11000000
    { 0X80 , 0  , 2  , 5  }, // 10000000
    { 0XA0 , 2  , 1  , 5  }, // 10100000
    { 0X50 , 0  , 3  , 6  }, // 01010000
    { 0X60 , 4  , 1  , 6  }, // 01100000
    { 0X70 , 3  , 1  , 6  }, // 01110000
    { 0X20 , 7  , 1  , 7  }, // 00100000
    { 0X28 , 6  , 1  , 7  }, // 00101000
    { 0X30 , 1  , 2  , 7  }, // 00110000
    { 0X38 , 5  , 1  , 7  }, // 00111000
    { 0X10 , 2  , 2  , 8  }, // 00010000
    { 0X14 , 9  , 1  , 8  }, // 00010100
    { 0X18 , 0  , 4  , 8  }, // 00011000
    { 0X1C , 8  , 1  , 8  }, // 00011100
    { 0X40 , 13 , 1  , 9  }, // 01000000
    { 0X42 , 0  , 6  , 9  }, // 01000010
    { 0X44 , 12 , 1  , 9  }, // 01000100
    { 0X46 , 11 , 1  , 9  }, // 01000110
    { 0X48 , 3  , 2  , 9  }, // 01001000
    { 0X4A , 1  , 3  , 9  }, // 01001010
    { 0X4C , 0  , 5  , 9  }, // 01001100
    { 0X4E , 10 , 1  , 9  }, // 01001110
// AC_LUT_6 (shift = 6)
    { 0X80 , 16 , 1  , 11 }, // 10000000
    { 0X90 , 5  , 2  , 11 }, // 10010000
    { 0XA0 , 0  , 7  , 11 }, // 10100000
    { 0XB0 , 2  , 3  , 11 }, // 10110000
    { 0XC0 , 1  , 4  , 11 }, // 11000000
    { 0XD0 , 15 , 1  , 11 }, // 11010000
    { 0XE0 , 14 , 1  , 11 }, // 11100000
    { 0XF0 , 4  , 2  , 11 }, // 11110000
    { 0X40 , 0  , 11 , 13 }, // 01000000
    { 0X44 , 8  , 2  , 13 }, // 01000100
    { 0X48 , 4  , 3  , 13 }, // 01001000
    { 0X4C , 0  , 10 , 13 }, // 01001100
    { 0X50 , 2  , 4  , 13 }, // 01010000
    { 0X54 , 7  , 2  , 13 }, // 01010100
    { 0X58 , 21 , 1  , 13 }, // 01011000
    { 0X5C , 20 , 1  , 13 }, // 01011100
    { 0X60 , 0  , 9  , 13 }, // 01100000
    { 0X64 , 19 , 1  , 13 }, // 01100100
    { 0X68 , 18 , 1  , 13 }, // 01101000
    { 0X6C , 1  , 5  , 13 }, // 01101100
    { 0X70 , 3  , 3  , 13 }, // 01110000
    { 0X74 , 0  , 8  , 13 }, // 01110100
    { 0X78 , 6  , 2  , 13 }, // 01111000
    { 0X7C , 17 , 1  , 13 }, // 01111100
    { 0X20 , 10 , 2  , 14 }, // 00100000
    { 0X22 , 9  , 2  , 14 }, // 00100010
    { 0X24 , 5  , 3  , 14 }, // 00100100
    { 0X26 , 3  , 4  , 14 }, // 00100110
    { 0X28 , 2  , 5  , 14 }, // 00101000
    { 0X2A , 1  , 7  , 14 }, // 00101010
    { 0X2C , 1  , 6  , 14 }, // 00101100
    { 0X2E , 0  , 15 , 14 }, // 00101110
    { 0X30 , 0  , 14 , 14 }, // 00110000
    { 0X32 , 0  , 13 , 14 }, // 00110010
    { 0X34 , 0  , 12 , 14 }, // 00110100
    { 0X36 , 26 , 1  , 14 }, // 00110110
    { 0X38 , 25 , 1  , 14 }, // 00111000
    { 0X3A , 24 , 1  , 14 }, // 00111010
    { 0X3C , 23 , 1  , 14 }, // 00111100
    { 0X3E , 22 , 1  , 14 }, // 00111110
// AC_LUT_9 (shift = 9)
    { 0X80 , 0  , 31 , 15 }, // 10000000
    { 0X88 , 0  , 30 , 15 }, // 10001000
    { 0X90 , 0  , 29 , 15 }, // 10010000
    { 0X98 , 0  , 28 , 15 }, // 10011000
    { 0XA0 , 0  , 27 , 15 }, // 10100000
    { 0XA8 , 0  , 26 , 15 }, // 10101000
    { 0XB0 , 0  , 25 , 15 }, // 10110000
    { 0XB8 , 0  , 24 , 15 }, // 10111000
    { 0XC0 , 0  , 23 , 15 }, // 11000000
    { 0XC8 , 0  , 22 , 15 }, // 11001000
    { 0XD0 , 0  , 21 , 15 }, // 11010000
    { 0XD8 , 0  , 20 , 15 }, // 11011000
    { 0XE0 , 0  , 19 , 15 }, // 11100000
    { 0XE8 , 0  , 18 , 15 }, // 11101000
    { 0XF0 , 0  , 17 , 15 }, // 11110000
    { 0XF8 , 0  , 16 , 15 }, // 11111000
    { 0X40 , 0  , 40 , 16 }, // 01000000
    { 0X44 , 0  , 39 , 16 }, // 01000100
    { 0X48 , 0  , 38 , 16 }, // 01001000
    { 0X4C , 0  , 37 , 16 }, // 01001100
    { 0X50 , 0  , 36 , 16 }, // 01010000
    { 0X54 , 0  , 35 , 16 }, // 01010100
    { 0X58 , 0  , 34 , 16 }, // 01011000
    { 0X5C , 0  , 33 , 16 }, // 01011100
    { 0X60 , 0  , 32 , 16 }, // 01100000
    { 0X64 , 1  , 14 , 16 }, // 01100100
    { 0X68 , 1  , 13 , 16 }, // 01101000
    { 0X6C , 1  , 12 , 16 }, // 01101100
    { 0X70 , 1  , 11 , 16 }, // 01110000
    { 0X74 , 1  , 10 , 16 }, // 01110100
    { 0X78 , 1  , 9  , 16 }, // 01111000
    { 0X7C , 1  , 8  , 16 }, // 01111100
    { 0X20 , 1  , 18 , 17 }, // 00100000
    { 0X22 , 1  , 17 , 17 }, // 00100010
    { 0X24 , 1  , 16 , 17 }, // 00100100
    { 0X26 , 1  , 15 , 17 }, // 00100110
    { 0X28 , 6  , 3  , 17 }, // 00101000
    { 0X2A , 16 , 2  , 17 }, // 00101010
    { 0X2C , 15 , 2  , 17 }, // 00101100
    { 0X2E , 14 , 2  , 17 }, // 00101110
    { 0X30 , 13 , 2  , 17 }, // 00110000
    { 0X32 , 12 , 2  , 17 }, // 00110010
    { 0X34 , 11 , 2  , 17 }, // 00110100
    { 0X36 , 31 , 1  , 17 }, // 00110110
    { 0X38 , 30 , 1  , 17 }, // 00111000
    { 0X3A , 29 , 1  , 17 }, // 00111010
    { 0X3C , 28 , 1  , 17 }, // 00111100
    { 0X3E , 27 , 1  , 17 }, // 00111110
};

static const uint8 STR_ZIG_ZAG[] = {
     0,  1,  8, 16,  9,  2,  3, 10,
    17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63,
};

static const uint8 STR_QUANTIZATION[] = {
     2, 16, 19, 22, 26, 27, 29, 34,
    16, 16, 22, 24, 27, 29, 34, 37,
    19, 22, 26, 27, 29, 34, 34, 38,
    22, 22, 26, 27, 29, 34, 37, 40,
    22, 26, 27, 29, 32, 35, 40, 48,
    26, 27, 29, 32, 35, 40, 48, 58,
    26, 27, 29, 34, 38, 46, 56, 69,
    27, 29, 35, 38, 46, 56, 69, 83
};

static const float STR_IDCT[] = {
    0.354f,  0.354f,  0.354f,  0.354f,  0.354f,  0.354f,  0.354f,  0.354f,
    0.490f,  0.416f,  0.278f,  0.098f, -0.098f, -0.278f, -0.416f, -0.490f,
    0.462f,  0.191f, -0.191f, -0.462f, -0.462f, -0.191f,  0.191f,  0.462f,
    0.416f, -0.098f, -0.490f, -0.278f,  0.278f,  0.490f,  0.098f, -0.416f,
    0.354f, -0.354f, -0.354f,  0.354f,  0.354f, -0.354f, -0.354f,  0.354f,
    0.278f, -0.490f,  0.098f,  0.416f, -0.416f, -0.098f,  0.490f, -0.278f,
    0.191f, -0.462f,  0.462f, -0.191f, -0.191f,  0.462f, -0.462f,  0.191f,
    0.098f, -0.278f,  0.416f, -0.490f,  0.490f, -0.416f,  0.278f, -0.098f,
};

struct Video {

    struct Decoder : Sound::Decoder {
        int width, height, fps;

        Decoder(Stream *stream) : Sound::Decoder(stream, 2, 0) {}
        virtual ~Decoder() { /* delete stream; */ }
        virtual bool decodeVideo(Color32 *pixels) { return false; }
    };

    // based on ffmpeg https://github.com/FFmpeg/FFmpeg/blob/master/libavcodec/ implementation of escape codecs
    struct Escape : Decoder {
        int vfmt, bpp;
        int sfmt, rate, channels, bps;
        int framesCount, chunksCount, offset;
        int curVideoPos, curVideoChunk;
        int curAudioPos, curAudioChunk;

        Sound::Decoder *audioDecoder;

        uint8 *prevFrame, *nextFrame, *lumaFrame;

        struct Chunk {
            int32 offset;
            int32 videoSize;
            int32 audioSize;
            uint8 *data;
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
                chunks[i].data      = NULL;
            }

            switch (vfmt) {
                case 124 :
                    prevFrame = new uint8[width * height * 4];
                    nextFrame = new uint8[width * height * 4];
                    memset(prevFrame, 0, width * height * sizeof(uint32));
                    memset(nextFrame, 0, width * height * sizeof(uint32));
                    break;
                case 130 :
                    // Y[w*h], Cb[w*h/4], Cr[w*h/4], F[w*h/4]
                    prevFrame = new uint8[width * height * 7 / 4];
                    nextFrame = new uint8[width * height * 7 / 4];
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

            curVideoPos   = curAudioPos   = 0;
            curVideoChunk = curAudioChunk = 0;

            nextChunk(0, 0);

            if (sfmt == 1)
                audioDecoder = new Sound::PCM(NULL, channels, rate, 0x7FFFFF, bps);      // TR2
            else if (sfmt == 101) {
                if (bps == 8)
                    audioDecoder = new Sound::PCM(NULL, channels, rate, 0x7FFFFF, bps);  // TR1
                else
                    audioDecoder = new Sound::IMA(NULL, channels, rate);          // TR3
            }
        }

        virtual ~Escape() {
            {
                OS_LOCK(Sound::lock);
                audioDecoder->stream = NULL;
                delete audioDecoder;
            }
            for (int i = 0; i < chunksCount; i++)
                delete[] chunks[i].data;
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
            for (uint32 i = 0; i < sizeof(buf); i++) {
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

            if ((value  = bs.readBit()) != 1 ||
                (value += bs.read(3)) != 8 ||
                (value += bs.read(7)) != 135)
                return value;

            return value + bs.read(12);
        }

        int getSkip130(BitStream &bs) {
            int value;

            if ((value = bs.readBit())) return 0;
            if ((value = bs.read(3)))   return value;
            if ((value = bs.read(8)))   return value + 7;
            if ((value = bs.read(15)))  return value + 262;

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
            int value = bs.readBit();
            if (value) {
                static const int8 trans[3][2] = { {2, 1}, {0, 2}, {1, 0} };
                value = bs.readBit();
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

        void nextChunk(int from, int to) {
            OS_LOCK(Sound::lock);

            if (from < curVideoChunk && from < curAudioChunk) {
                delete[] chunks[from].data;
                chunks[from].data = NULL;
            }

            Chunk &chunk = chunks[to];
            if (chunk.data)
                return;
            chunk.data = new uint8[chunk.videoSize + chunk.audioSize];
            stream->setPos(chunk.offset);
            stream->raw(chunk.data, chunk.videoSize + chunk.audioSize);
        }

        virtual bool decodeVideo(Color32 *pixels) {
            if (curVideoChunk >= chunksCount)
                return false;

            if (curVideoPos >= chunks[curVideoChunk].videoSize) {
                curVideoChunk++;
                curVideoPos = 0;
                if (curVideoChunk >= chunksCount)
                    return false;

                nextChunk(curVideoChunk - 1, curVideoChunk);
            }

            uint8 *data = chunks[curVideoChunk].data + curVideoPos;

            switch (vfmt) {
                case 124 : return decode124(data, pixels);
                case 130 : return decode130(data, pixels);
                default  : ASSERT(false);
            }

            return false;
        }

        bool decode124(uint8 *data, Color32 *pixels) {
            uint32 flags, size;
            memcpy(&flags, data + 0, 4);
            memcpy(&size,  data + 4, 4);
            data += 8;

            curVideoPos += size;

        // skip unchanged frame
            if (!(flags & 0x114) || !(flags & 0x7800000))
                return true;

            int sbCount = (width / 8) * (height / 8);

        // read data into bit stream
            size -= (sizeof(flags) + sizeof(size));

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

            static const uint16 maskMatrix[] = { 0x0001, 0x0002, 0x0010, 0x0020,
                                                 0x0004, 0x0008, 0x0040, 0x0080,
                                                 0x0100, 0x0200, 0x1000, 0x2000,
                                                 0x0400, 0x0800, 0x4000, 0x8000};

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

                    while (!bs.readBit()) {
                        decodeMacroBlock(bs, mb, cbIndex, sbIndex);
                        uint16 mask = bs.read(16);
                        multiMask |= mask;
                        for (int i = 0; i < 16; i++)
                            if (mask & maskMatrix[i])
                                insertMacroBlock(sb, mb, i);
                    }

                    if (!bs.readBit()) {
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
                            while (!bs.readBit()) {
                                decodeMacroBlock(bs, mb, cbIndex, sbIndex);
                                insertMacroBlock(sb, mb, bs.read(4));
                            }

                    copySuperBlock(dst, width, sb.pixels, 8);
                }

                skip--;
            }

            memcpy(pixels, nextFrame, width * height * 4);
            swap(prevFrame, nextFrame);

            return true;
        }

        bool decode130(uint8 *data, Color32 *pixels) {

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

            Chunk &chunk = chunks[curVideoChunk];
            curVideoPos = chunk.videoSize;

            BitStream bs(data, chunk.videoSize);
            bs.data += 16; // skip 16 bytes (frame size, version, gamma/linear chroma flags etc.)

            uint8 *lumaPtr = lumaFrame;

            int skip = -1;
            int bCount = width * height / 4;
            uint32 luma = 0, Y[4] = { 0 }, U = 16, V = 16, F = 0;

            uint8 *oY = prevFrame, *oU = oY + width * height, *oV = oU + width * height / 4, *oF = oV + width * height / 4;
            uint8 *nY = nextFrame, *nU = nY + width * height, *nV = nU + width * height / 4, *nF = nV + width * height / 4;

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
                    F    = oF[0];
                    luma = *lumaPtr;
                } else {
                    if (bs.readBit()) {
                        uint32 sign = bs.read(6);
                        uint32 diff = bs.read(2);

                        luma = bs.read(5) * 2;

                        for (int i = 0; i < 4; i++)
                            Y[i] = clamp(luma + offsetLUT[diff] * signLUT[sign][i], 0U, 63U);

                        F = 1;
                    } else {

                        if (bs.readBit())
                            luma = bs.readBit() ? bs.read(6) : ((luma + lumaLUT[bs.read(3)]) & 63);

                        for (int i = 0; i < 4; i++)
                            Y[i] = luma;

                        F = 0;
                    }

                    if (bs.readBit()) {
                        if (bs.readBit()) {
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
                nF[0]         = F;

                nY += 2; nU++; nV++; nF++;
                oY += 2; oU++; oV++; oF++;

                if (!(((bIndex + 1) * 2) % width)) {
                    nY += width;
                    oY += width;
                }

                skip--;
            }

            nY = nextFrame;
            nU = nY + width * height;
            nV = nU + width * height / 4;
            nF = nV + width * height / 4;

            for (int y = 0; y < height / 2; y++) {
                for (int x = 0; x < width / 2; x++) {
                    int i = (y * width + x) * 2;
                    
                    Color32::YCbCr_T871_420(nY[i] << 2, nY[i + 1] << 2, nY[i + width] << 2, nY[i + width + 1] << 2, 
                                            chromaValueLUT[*nU] - 128, chromaValueLUT[*nV] - 128, *nF * 4, 
                                            pixels[i], pixels[i + 1], pixels[i + width], pixels[i + width + 1]);

                    nU++;
                    nV++;
                    nF++;
                }
            }

            swap(prevFrame, nextFrame);

            return true;
        }

        virtual int decode(Sound::Frame *frames, int count) {
            if (!audioDecoder) return 0;

            if (bps != 4 && abs(curAudioChunk - curVideoChunk) > 1) { // sync with video chunk, doesn't work for IMA
                nextChunk(curAudioChunk, curVideoChunk);
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
                    nextChunk(curAudioChunk - 1, curAudioChunk);
                    continue;
                }

                int part = min(count - i, (chunk->audioSize - curAudioPos) / (channels * bps / 8));

                Stream *memStream = new Stream(NULL, chunk->data + chunk->videoSize + curAudioPos, chunk->audioSize - curAudioPos);
                audioDecoder->stream = memStream;

                while (part > 0) { 
                    int res = audioDecoder->decode(&frames[i], part);
                    i += res;
                    part -= res;
                }
                curAudioPos += memStream->pos;

                delete memStream;
            }

            return count;
        }
    };

    // based on https://raw.githubusercontent.com/m35/jpsxdec/readme/jpsxdec/PlayStation1_STR_format.txt
    struct STR : Decoder {

        enum {
            MAGIC_STR         = 0x80010160,

            VIDEO_SECTOR_SIZE = 2016,
            VIDEO_SECTOR_MAX  = 16,
            AUDIO_SECTOR_SIZE = (16 + 112) * 18, // XA ADPCM data block size

            MAX_CHUNKS        = 4,
        };

        struct SyncHeader {
            uint32 sync[3];
            uint8  mins, secs, block, mode;
            uint8  interleaved;
            uint8  channel;
            struct {
                uint8 isEnd:1, isVideo:1, isAudio:1, isData:1, trigger:1, form:1, realtime:1, eof:1;
            } submode;
            struct {
                uint8 stereo:1, :1, rate:1, :1, bps:1, :3;
            } coding;
            uint32 dup;
        };

        struct Sector {
            uint32 magic;
            uint16 chunkIndex;
            uint16 chunksCount;
            uint32 frameIndex;
            uint32 chunkSize;
            uint16 width, height;
            uint16 blocks;
            uint16 unk1;
            uint16 qscale;
            uint16 version;
            uint32 unk2;
        };

        struct VideoChunk {
            int    size;
            uint16 width;
            uint16 height;
            uint32 qscale;
            uint8  data[VIDEO_SECTOR_SIZE * VIDEO_SECTOR_MAX];
        };

        struct AudioChunk {
            int    size;
            uint8  data[AUDIO_SECTOR_SIZE];
        };

        uint8 AC_LUT_1[256];
        uint8 AC_LUT_6[256];
        uint8 AC_LUT_9[256];

        VideoChunk videoChunks[MAX_CHUNKS];
        AudioChunk audioChunks[MAX_CHUNKS];

        int   videoChunksCount;
        int   audioChunksCount;

        int   curVideoChunk;
        int   curAudioChunk;

        Sound::Decoder *audioDecoder;

        struct {
            uint8 code;
            uint8 length;
        } vlc[176];

        bool hasSyncHeader;

        STR(Stream *stream) : Decoder(stream), videoChunksCount(0), audioChunksCount(0), curVideoChunk(-1), curAudioChunk(-1), audioDecoder(NULL) {

            if (stream->pos >= stream->size) {
                LOG("Can't load STR format \"%s\"\n", stream->name);
                ASSERT(false);
                return;
            }

            memset(AC_LUT_1, 255, sizeof(AC_LUT_1));
            memset(AC_LUT_6, 255, sizeof(AC_LUT_6));
            memset(AC_LUT_9, 255, sizeof(AC_LUT_9));

            buildLUT(AC_LUT_1,  0,  22, 1);
            buildLUT(AC_LUT_6, 22,  62, 6);
            buildLUT(AC_LUT_9, 62, 110, 9);

            uint32 syncMagic[3];
            stream->raw(syncMagic, sizeof(syncMagic));
            stream->seek(-(int)sizeof(syncMagic));

            hasSyncHeader = syncMagic[0] == 0xFFFFFF00 && syncMagic[1] == 0xFFFFFFFF && syncMagic[2] == 0x00FFFFFF;
            
            if (!hasSyncHeader) {
                LOG("! No sync header found, please use jpsxdec tool to extract FMVs\n");
            }

            for (int i = 0; i < MAX_CHUNKS; i++) {
                videoChunks[i].size = 0;
                audioChunks[i].size = 0;
            }

            nextChunk();

            VideoChunk &chunk = videoChunks[0];
            width    = (chunk.width  + 15) / 16 * 16;
            height   = (chunk.height + 15) / 16 * 16;
            fps      = 150 / (chunk.size / VIDEO_SECTOR_SIZE);
            fps      = (fps < 20) ? 15 : 30;
            channels = 2;
            freq     = 37800;

            audioDecoder = new Sound::XA(NULL);
        }

        virtual ~STR() {
            OS_LOCK(Sound::lock);
            audioDecoder->stream = NULL;
            delete audioDecoder;
        }

        void buildLUT(uint8 *LUT, int start, int end, int shift) {
            for (int i = start; i < end; i++) {
                const AC_ENTRY &e = STR_AC[i];
                uint8 trash = (1 << (8 + shift - e.length + 1));
            // fill the value and all possible endings
                while (trash--)
                    LUT[e.code | trash] = i;
            }
        }

        bool nextChunk() {
            OS_LOCK(Sound::lock);

            if (videoChunks[videoChunksCount % MAX_CHUNKS].size > 0)
                return false;

            if (stream->pos >= stream->size)
                return false;

            bool readingVideo = false;

            while (stream->pos < stream->size) {

                if (hasSyncHeader)
                    stream->seek(24);

                Sector sector;
                stream->raw(&sector, sizeof(Sector));

                if (sector.magic == MAGIC_STR) {
                    VideoChunk *chunk = videoChunks + (videoChunksCount % MAX_CHUNKS);

                    if (sector.chunkIndex == 0) {
                        readingVideo  = true;
                        chunk->size   = 0;
                        chunk->width  = sector.width;
                        chunk->height = sector.height;
                        chunk->qscale = sector.qscale;
                    }

                    ASSERT(chunk->size + VIDEO_SECTOR_SIZE < sizeof(chunk->data));
                    stream->raw(chunk->data + chunk->size, VIDEO_SECTOR_SIZE);
                    chunk->size += VIDEO_SECTOR_SIZE;

                    if (hasSyncHeader)
                        stream->seek(280);

                    if (sector.chunkIndex == sector.chunksCount - 1) {
                        videoChunksCount++;
                        return true;
                    }

                } else {
                    AudioChunk *chunk = audioChunks + (audioChunksCount++ % MAX_CHUNKS);

                    memcpy(chunk->data, &sector, sizeof(sector)); // audio chunk has no sector header (just XA data)
                    stream->raw(chunk->data + sizeof(sector), AUDIO_SECTOR_SIZE - sizeof(sector)); // !!! MUST BE 2304 !!! most of CD image tools copy only 2048 per sector, so "clicks" will be there
                    chunk->size = AUDIO_SECTOR_SIZE;
                    stream->seek(24);

                    if (!hasSyncHeader)
                        stream->seek(2048 - (AUDIO_SECTOR_SIZE + 24));

                    if (!readingVideo)
                        return true;
                };
            }
            return false;
        }

        // http://jpsxdec.blogspot.com/2011/06/decoding-mpeg-like-bitstreams.html
        bool readCode(BitStream &bs, int16 &skip, int16 &ac) {
            if (bs.readU(1)) {
                if (bs.readU(1)) {
                    skip = 0;
                    ac   = bs.readU(1) ? -1 : 1;
                    return true;
                }
                return false; // end of block
            }

            int nz = 1;
            while (!bs.readU(1))
                nz++;

            if (nz == 5) { // escape code == 0b1000001
                uint16 esc = bs.readU(16);
                skip = esc >> 10;
                ac   = esc & 0x3FF;
                if (ac & 0x200)
                    ac -= 0x400;
                return true;
            }

            uint8 *table, shift;
            if (nz < 6) {
                table = AC_LUT_1;
                shift = 1;
            } else if (nz < 9) {
                table = AC_LUT_6;
                shift = 6;
            } else {
                table = AC_LUT_9;
                shift = 9;
            }

            BitStream state = bs;
            uint32 code = (1 << 7) | state.readU(7);

            code >>= nz - shift;

            ASSERT(table);

            int index = table[code];

            ASSERT(index != 255);

            const AC_ENTRY &e = STR_AC[index];
            bs.skip(e.length - nz - 1);
            skip = e.skip;
            ac = (code & (1 << (8 + shift - e.length))) ? -e.ac : e.ac;
            return true;
        }
        
        void IDCT(int16 *b) {
            float t[64];

            for (int x = 0; x < 8; x++)
                for (int y = 0; y < 8; y++)
                    t[x + y * 8] = b[x + 0 * 8] * STR_IDCT[0 * 8 + y]
                                 + b[x + 1 * 8] * STR_IDCT[1 * 8 + y]
                                 + b[x + 2 * 8] * STR_IDCT[2 * 8 + y]
                                 + b[x + 3 * 8] * STR_IDCT[3 * 8 + y]
                                 + b[x + 4 * 8] * STR_IDCT[4 * 8 + y]
                                 + b[x + 5 * 8] * STR_IDCT[5 * 8 + y]
                                 + b[x + 6 * 8] * STR_IDCT[6 * 8 + y]
                                 + b[x + 7 * 8] * STR_IDCT[7 * 8 + y];

            for (int x = 0; x < 8; x++)
                for (int y = 0; y < 8; y++) {
                    int i = y * 8;
                    b[x + i] = int16(
                               t[0 + i] * STR_IDCT[x + 0 * 8]
                             + t[1 + i] * STR_IDCT[x + 1 * 8]
                             + t[2 + i] * STR_IDCT[x + 2 * 8]
                             + t[3 + i] * STR_IDCT[x + 3 * 8]
                             + t[4 + i] * STR_IDCT[x + 4 * 8]
                             + t[5 + i] * STR_IDCT[x + 5 * 8]
                             + t[6 + i] * STR_IDCT[x + 6 * 8]
                             + t[7 + i] * STR_IDCT[x + 7 * 8]);
                }
        }

        virtual bool decodeVideo(Color32 *pixels) {
            curVideoChunk++;
            while (curVideoChunk >= videoChunksCount) {
                if (!nextChunk()) {
                    return false;
                }
            }

            VideoChunk *chunk = videoChunks + (curVideoChunk % MAX_CHUNKS);

            BitStream bs(chunk->data + 8, chunk->size - 8); // make bitstream without frame header

            int16 block[6][64]; // Cr, Cb, YTL, YTR, YBL, YBR
            for (int bX = 0; bX < width / 16; bX++)
                for (int bY = 0; bY < height / 16; bY++) {
                    memset(block, 0, sizeof(block));

                    for (int i = 0; i < 6; i++) {
                        bool nonZero = false;

                        int16 *channel = block[i];
                        channel[0] = bs.readU(10);
                        if (channel[0]) {
                            if (channel[0] & 0x200)
                                channel[0] -= 0x400;
                            channel[0] = channel[0] * STR_QUANTIZATION[0]; // DC
                            nonZero = true;
                        }
                        
                        int16 skip, ac;
                        int index = 0;
                        while (readCode(bs, skip, ac)) {
                            index += 1 + skip;
                            ASSERT(index < 64);
                            int zIndex = STR_ZIG_ZAG[index];
                            channel[zIndex] = (ac * STR_QUANTIZATION[zIndex] * chunk->qscale + 4) >> 3;
                            nonZero = true;
                        }

                        if (nonZero)
                            IDCT(channel);
                    }

                    Color32 *blockPixels = pixels + (width * bY * 16 + bX * 16);

                    for (uint32 i = 0; i < 8 * 8; i++) {
                        int x = (i % 8) * 2;
                        int y = (i / 8) * 2;
                        int j = (x & 7) + (y & 7) * 8;

                        Color32 *c = blockPixels + (width * y + x);

                        int16 *b = block[(x < 8) ? ((y < 8) ? 2 : 4) : ((y < 8) ? 3 : 5)];

                        Color32::YCbCr_T871_420(b[j] + 128, b[j + 1] + 128, b[j + 8] + 128, b[j + 8 + 1] + 128, block[1][i], block[0][i], 4,
                                                c[0], c[1], c[width], c[width + 1]);
                    }
                }

            chunk->size = 0;

            return true;
        }

        virtual int decode(Sound::Frame *frames, int count) {
            if (!audioDecoder) return 0;
            Sound::XA *xa = (Sound::XA*)audioDecoder;

            int i = 0;
            while (i < count) {
                if (xa->pos >= COUNT(xa->buffer)) {
                    curAudioChunk++;
                    while (curAudioChunk >= audioChunksCount) {
                        if (!nextChunk()) {
                            curAudioChunk--;
                            memset(frames, 0, count * sizeof(Sound::Frame));
                            return count;
                        }
                    }
                }

                AudioChunk *chunk = audioChunks + (curAudioChunk % MAX_CHUNKS);
                ASSERT(chunk->size > 0);
                Stream *memStream = new Stream(NULL, chunk->data, AUDIO_SECTOR_SIZE);
                audioDecoder->stream = memStream;

                i += audioDecoder->decode(&frames[i], count - i);

                delete memStream;
            }

            return count;
        }
    };

// based on https://wiki.multimedia.cx/index.php/Sega_FILM
    struct Cinepak : Decoder {

        struct Chunk {
            int    offset;
            int    size;
            uint32 info[2];
        } *chunks;

        int chunksCount;
        int audioChunkIndex;
        int audioChunkPos;
        Array<Sound::Frame> audioChunkFrames;

        int videoChunkIndex;
        int videoChunkPos;
        Array<uint8> videoChunkData;

        Cinepak(Stream *stream) : Decoder(stream), chunks(NULL), audioChunkIndex(-1), audioChunkPos(0), videoChunkIndex(-1), videoChunkPos(0) {
            ASSERTV(stream->readLE32() == FOURCC("FILM"));
            int sampleOffset = stream->readBE32();
            stream->seek(4); // skip version 1.06
            stream->seek(4); // skip reserved
            ASSERTV(stream->readLE32() == FOURCC("FDSC"));
            ASSERTV(stream->readBE32() == 32);
            ASSERTV(stream->readLE32() == FOURCC("cvid"));
            height = stream->readBE32();
            width  = stream->readBE32();
            ASSERTV(stream->read() == 24);
            channels = stream->read();
            ASSERT(channels == 2);
            ASSERTV(stream->read() == 16);
            ASSERTV(stream->read() == 0);
            freq = stream->readBE16();
            ASSERT(freq == 22254);

            stream->seek(6);
            ASSERTV(stream->readLE32() == FOURCC("STAB"));
            stream->seek(4); // skip STAB length
            fps = stream->readBE32() / 2;
            chunksCount = stream->readBE32();
            chunks = new Chunk[chunksCount];
            for (int i = 0; i < chunksCount; i++) {
                Chunk &c = chunks[i];
                c.offset  = stream->readBE32() + sampleOffset;
                c.size    = stream->readBE32();
                c.info[0] = stream->readBE32();
                c.info[1] = stream->readBE32();
            }
        }

        virtual ~Cinepak() {
            delete[] chunks;  
        }

        virtual bool decodeVideo(Color32 *pixels) {
            if (audioChunkIndex >= chunksCount)
                return false;
            /*
            // TODO: sega cinepak film decoder
            // get next audio chunk
            if (videoChunkPos >= videoChunkData.length) {
                videoChunkPos = 0;

                while (++videoChunkIndex < chunksCount) {
                    if (chunks[videoChunkIndex].info[0] != 0xFFFFFFFF || chunks[videoChunkIndex].info[1] != 1)
                        break;
                }

                if (videoChunkIndex >= chunksCount)
                    return true;

                const Chunk &chunk = chunks[videoChunkIndex];

                {
                    OS_LOCK(Sound::lock);
                    stream->setPos(chunk.offset);
                    videoChunkData.resize(chunk.size);
                    stream->raw(videoChunkData.items, videoChunkData.length);
                }
            }

            // TODO: decode
            Stream data(NULL, videoChunkData.items + videoChunkPos, videoChunkData.length - videoChunkPos);
            union FrameHeader {
                struct { uint32 flags:8, size:24; };
                uint32 value;
            } hdr;

            hdr.value = data.readBE32();
            ASSERT(hdr.size <= videoChunkData.length - videoChunkPos);
            videoChunkPos += hdr.size;
            */
            for (int y = 0; y < height; y++)
                for (int x = 0; x < width; x++) {
                    Color32 c;
                    c.r = c.g = c.b = x ^ y;
                    c.a = 255;
                    pixels[y * width + x] = c;
                }
            
            return true;
        }

        virtual int decode(Sound::Frame *frames, int count) {
            if (audioChunkIndex >= chunksCount) {
                memset(frames, 0, count * sizeof(Sound::Frame));
                return count;
            }

            // get next audio chunk
            if (audioChunkPos >= audioChunkFrames.length) {
                audioChunkPos = 0;

                while (++audioChunkIndex < chunksCount) {
                    if (chunks[audioChunkIndex].info[0] == 0xFFFFFFFF)
                        break;
                }

                if (audioChunkIndex >= chunksCount) {
                    memset(frames, 0, count);
                    return count;
                }

                const Chunk &chunk = chunks[audioChunkIndex];

                audioChunkFrames.resize(chunk.size / sizeof(Sound::Frame));

                stream->setPos(chunk.offset);
                // read LEFT channel samples
                for (int i = 0; i < audioChunkFrames.length; i++)
                    audioChunkFrames[i].L = stream->readBE16();
                // read RIGHT channel samples
                for (int i = 0; i < audioChunkFrames.length; i++)
                    audioChunkFrames[i].R = stream->readBE16();
            }

            for (int i = 0; i < count; i += 2) {
                frames[i + 0] = audioChunkFrames[audioChunkPos];
                frames[i + 1] = audioChunkFrames[audioChunkPos++];

                if (audioChunkPos >= audioChunkFrames.length)
                    return i + 2;
            }

            return count;
        }
    };

    enum Format {
        PC,
        PSX,
        SAT,
    } format;

    Decoder *decoder;
    Texture *frameTex[2];
    Color32 *frameData;
    float   step, stepTimer, time;
    bool    isPlaying;
    bool    needUpdate;
    Sound::Sample *sample;

    Video(Stream *stream) : decoder(NULL), stepTimer(0.0f), time(0.0f), isPlaying(false) {
        frameTex[0] = frameTex[1] = NULL;

        if (!stream) return;

        uint32 magic = stream->readLE32();
        stream->seek(-4);

        float pitch = 1.0f;

        if (magic == FOURCC("FILM")) {
            format  = SAT;
            decoder = new Cinepak(stream);
            pitch = decoder->freq / 22050.0f; // 22254 / 22050 = 1.00925
        } else if (magic == FOURCC("ARMo")) {
            format  = PC;
            decoder = new Escape(stream);
        } else {
            format  = PSX;
            decoder = new STR(stream);
        }

        frameData = new Color32[decoder->width * decoder->height];
        memset(frameData, 0, decoder->width * decoder->height * sizeof(Color32));

        for (int i = 0; i < 2; i++)
            frameTex[i] = new Texture(decoder->width, decoder->height, FMT_RGBA, 0, frameData);

        sample = Sound::play(decoder);
        sample->pitch = pitch;

        step      = 1.0f / decoder->fps;
        stepTimer = step;
        time      = 0.0f;
        isPlaying = true;
    }

    virtual ~Video() {
        OS_LOCK(Sound::lock);
        sample->decoder = NULL;
        sample->stop();
        delete decoder;
        delete frameTex[0];
        delete frameTex[1];
        delete[] frameData;
    }

    void update() {
        if (!isPlaying) return;

        stepTimer += Core::deltaTime;
        if (stepTimer < step)
            return;
        stepTimer -= step;
        time += step;
    #ifdef VIDEO_TEST
        int t = Core::getTime();
        while (decoder->decodeVideo(frameData)) {}
        LOG("time: %d\n", Core::getTime() - t);
        isPlaying = false;
    #else
        isPlaying = needUpdate = decoder->decodeVideo(frameData);
    #endif
    }

    void render() { // just update GPU texture if it's necessary
        if (!needUpdate) return;
        frameTex[0]->update(frameData);
        swap(frameTex[0], frameTex[1]);
        needUpdate = false;
    }
};

#endif
