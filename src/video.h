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
    { 0b11000000 , 1  , 1  , 4  },
    { 0b10000000 , 0  , 2  , 5  },
    { 0b10100000 , 2  , 1  , 5  },
    { 0b01010000 , 0  , 3  , 6  },
    { 0b01100000 , 4  , 1  , 6  },
    { 0b01110000 , 3  , 1  , 6  },
    { 0b00100000 , 7  , 1  , 7  },
    { 0b00101000 , 6  , 1  , 7  },
    { 0b00110000 , 1  , 2  , 7  },
    { 0b00111000 , 5  , 1  , 7  },
    { 0b00010000 , 2  , 2  , 8  },
    { 0b00010100 , 9  , 1  , 8  },
    { 0b00011000 , 0  , 4  , 8  },
    { 0b00011100 , 8  , 1  , 8  },
    { 0b01000000 , 13 , 1  , 9  },
    { 0b01000010 , 0  , 6  , 9  },
    { 0b01000100 , 12 , 1  , 9  },
    { 0b01000110 , 11 , 1  , 9  },
    { 0b01001000 , 3  , 2  , 9  },
    { 0b01001010 , 1  , 3  , 9  },
    { 0b01001100 , 0  , 5  , 9  },
    { 0b01001110 , 10 , 1  , 9  },
// AC_LUT_6 (shift = 6)
    { 0b10000000 , 16 , 1  , 11 },
    { 0b10010000 , 5  , 2  , 11 },
    { 0b10100000 , 0  , 7  , 11 },
    { 0b10110000 , 2  , 3  , 11 },
    { 0b11000000 , 1  , 4  , 11 },
    { 0b11010000 , 15 , 1  , 11 },
    { 0b11100000 , 14 , 1  , 11 },
    { 0b11110000 , 4  , 2  , 11 },
    { 0b01000000 , 0  , 11 , 13 },
    { 0b01000100 , 8  , 2  , 13 },
    { 0b01001000 , 4  , 3  , 13 },
    { 0b01001100 , 0  , 10 , 13 },
    { 0b01010000 , 2  , 4  , 13 },
    { 0b01010100 , 7  , 2  , 13 },
    { 0b01011000 , 21 , 1  , 13 },
    { 0b01011100 , 20 , 1  , 13 },
    { 0b01100000 , 0  , 9  , 13 },
    { 0b01100100 , 19 , 1  , 13 },
    { 0b01101000 , 18 , 1  , 13 },
    { 0b01101100 , 1  , 5  , 13 },
    { 0b01110000 , 3  , 3  , 13 },
    { 0b01110100 , 0  , 8  , 13 },
    { 0b01111000 , 6  , 2  , 13 },
    { 0b01111100 , 17 , 1  , 13 },
    { 0b00100000 , 10 , 2  , 14 },
    { 0b00100010 , 9  , 2  , 14 },
    { 0b00100100 , 5  , 3  , 14 },
    { 0b00100110 , 3  , 4  , 14 },
    { 0b00101000 , 2  , 5  , 14 },
    { 0b00101010 , 1  , 7  , 14 },
    { 0b00101100 , 1  , 6  , 14 },
    { 0b00101110 , 0  , 15 , 14 },
    { 0b00110000 , 0  , 14 , 14 },
    { 0b00110010 , 0  , 13 , 14 },
    { 0b00110100 , 0  , 12 , 14 },
    { 0b00110110 , 26 , 1  , 14 },
    { 0b00111000 , 25 , 1  , 14 },
    { 0b00111010 , 24 , 1  , 14 },
    { 0b00111100 , 23 , 1  , 14 },
    { 0b00111110 , 22 , 1  , 14 },
// AC_LUT_9 (shift = 9)
    { 0b10000000 , 0  , 31 , 15 },
    { 0b10001000 , 0  , 30 , 15 },
    { 0b10010000 , 0  , 29 , 15 },
    { 0b10011000 , 0  , 28 , 15 },
    { 0b10100000 , 0  , 27 , 15 },
    { 0b10101000 , 0  , 26 , 15 },
    { 0b10110000 , 0  , 25 , 15 },
    { 0b10111000 , 0  , 24 , 15 },
    { 0b11000000 , 0  , 23 , 15 },
    { 0b11001000 , 0  , 22 , 15 },
    { 0b11010000 , 0  , 21 , 15 },
    { 0b11011000 , 0  , 20 , 15 },
    { 0b11100000 , 0  , 19 , 15 },
    { 0b11101000 , 0  , 18 , 15 },
    { 0b11110000 , 0  , 17 , 15 },
    { 0b11111000 , 0  , 16 , 15 },
    { 0b01000000 , 0  , 40 , 16 },
    { 0b01000100 , 0  , 39 , 16 },
    { 0b01001000 , 0  , 38 , 16 },
    { 0b01001100 , 0  , 37 , 16 },
    { 0b01010000 , 0  , 36 , 16 },
    { 0b01010100 , 0  , 35 , 16 },
    { 0b01011000 , 0  , 34 , 16 },
    { 0b01011100 , 0  , 33 , 16 },
    { 0b01100000 , 0  , 32 , 16 },
    { 0b01100100 , 1  , 14 , 16 },
    { 0b01101000 , 1  , 13 , 16 },
    { 0b01101100 , 1  , 12 , 16 },
    { 0b01110000 , 1  , 11 , 16 },
    { 0b01110100 , 1  , 10 , 16 },
    { 0b01111000 , 1  , 9  , 16 },
    { 0b01111100 , 1  , 8  , 16 },
    { 0b00100000 , 1  , 18 , 17 },
    { 0b00100010 , 1  , 17 , 17 },
    { 0b00100100 , 1  , 16 , 17 },
    { 0b00100110 , 1  , 15 , 17 },
    { 0b00101000 , 6  , 3  , 17 },
    { 0b00101010 , 16 , 2  , 17 },
    { 0b00101100 , 15 , 2  , 17 },
    { 0b00101110 , 14 , 2  , 17 },
    { 0b00110000 , 13 , 2  , 17 },
    { 0b00110010 , 12 , 2  , 17 },
    { 0b00110100 , 11 , 2  , 17 },
    { 0b00110110 , 31 , 1  , 17 },
    { 0b00111000 , 30 , 1  , 17 },
    { 0b00111010 , 29 , 1  , 17 },
    { 0b00111100 , 28 , 1  , 17 },
    { 0b00111110 , 27 , 1  , 17 },
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

    union Color32 {
        uint32 value;
        struct { uint8 r, g, b, a; };

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

            curVideoPos = curVideoChunk = 0; 
            curAudioPos = curAudioChunk = 0;

            if (sfmt == 1)
                audioDecoder = new Sound::PCM(stream, channels, rate, 0, bps);      // TR2
            else if (sfmt == 101) {
                if (bps == 8)
                    audioDecoder = new Sound::PCM(stream, channels, rate, 0, bps);  // TR1
                else
                    audioDecoder = new Sound::IMA(stream, channels, rate);          // TR3
            }
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

        virtual bool decodeVideo(Color32 *pixels) {
            OS_LOCK(Sound::lock);

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
                case 124 : return decode124(pixels);
                case 130 : return decode130(pixels);
                default  : ASSERT(false);
            }

            return false;
        }

        bool decode124(Color32 *pixels) {
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

            delete[] data;

            memcpy(pixels, nextFrame, width * height * 4);
            swap(prevFrame, nextFrame);

            return true;
        }

        bool decode130(Color32 *pixels) {

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

            delete[] data;

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

    // based on https://raw.githubusercontent.com/m35/jpsxdec/readme/jpsxdec/PlayStation1_STR_format.txt
    struct STR : Decoder {

        enum {
            MAGIC_STR         = 0x80010160,
            SECTOR_SIZE       = 2352,

            VIDEO_CHUNK_SIZE  = 2016,
            VIDEO_MAX_CHUNKS  = 16,
            VIDEO_MAX_FRAMES  = 4,

            AUDIO_CHUNK_SIZE  = (16 + 112) * 18, // XA ADPCM data block size
            AUDIO_MAX_FRAMES  = 8,

            BLOCK_EOD         = 0xFE00,
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

        struct VideoFrame {
            uint8  data[VIDEO_CHUNK_SIZE * VIDEO_MAX_CHUNKS];
            int    size;
            uint16 width;
            uint16 height;
            uint32 qscale;
        };

        struct AudioFrame {
            int pos;
            int size;
        };

        uint8 AC_LUT_1[256];
        uint8 AC_LUT_6[256];
        uint8 AC_LUT_9[256];

        VideoFrame vFrames[VIDEO_MAX_FRAMES];
        AudioFrame aFrames[AUDIO_MAX_FRAMES];

        int   vFrameIndex;
        int   aFrameIndex;

        Sound::Decoder *audioDecoder;

        struct {
            uint8 code;
            uint8 length;
        } vlc[176];

        int curAudioPos;
        int curAudioFrame;

        STR(Stream *stream) : Decoder(stream), vFrameIndex(-1), aFrameIndex(-1), audioDecoder(NULL) {
            curAudioFrame = 0;

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

            int pos = stream->pos;
            nextFrame();

            VideoFrame &frame = vFrames[vFrameIndex];
            width  = (frame.width  + 15) / 16 * 16;
            height = (frame.height + 15) / 16 * 16;
            fps    = 150 / (frame.size / VIDEO_CHUNK_SIZE);
            fps    = (fps < 20) ? 15 : 30;

            stream->setPos(pos);

            vFrameIndex = aFrameIndex = -1;
            memset(aFrames, 0, sizeof(aFrames));

            audioDecoder = new Sound::XA(stream);
        }

        virtual ~STR() {
            if (audioDecoder) {
                audioDecoder->stream = NULL;
                delete audioDecoder;
            }        
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

        bool nextFrame() {
            OS_LOCK(Sound::lock);

            uint8 data[SECTOR_SIZE];

            VideoFrame *vFrame = vFrames + vFrameIndex;
            while (stream->pos < stream->size) {
                if (stream->raw(data, sizeof(data)) != sizeof(data)) {
                    ASSERT(false);
                    return false;
                }

                SyncHeader *syncHeader = (SyncHeader*)data;

                if (syncHeader->sync[0] != 0xFFFFFF00 || syncHeader->sync[1] != 0xFFFFFFFF || syncHeader->sync[2] != 0x00FFFFFF) {
                    ASSERT(false);
                    return false;
                }

                if (syncHeader->submode.isVideo || syncHeader->submode.isData) {
                    Sector *sector = (Sector*)(data + sizeof(SyncHeader));

                    if (sector->magic == MAGIC_STR) {

                        if (sector->chunkIndex == 0) {
                            vFrameIndex = (vFrameIndex + 1) % VIDEO_MAX_FRAMES;
                            vFrame = vFrames + vFrameIndex;
                            vFrame->size    = 0;
                            vFrame->width   = sector->width;
                            vFrame->height  = sector->height;
                            vFrame->qscale  = sector->qscale;
                        }

                        ASSERT(vFrame->size + VIDEO_CHUNK_SIZE < sizeof(vFrame->data));
                        memcpy(vFrame->data + vFrame->size, data + sizeof(SyncHeader) + sizeof(Sector), VIDEO_CHUNK_SIZE);
                        vFrame->size += VIDEO_CHUNK_SIZE;

                        if (sector->chunkIndex == sector->chunksCount - 1) {
                            //LOG("frame %d: %dx%d %d\n", sector->frameIndex, frame->width, frame->height, frame->size);
                            return true;
                        }
                    }

                } else if (syncHeader->submode.isAudio) {
                    channels = syncHeader->coding.stereo ? 2 : 1;
                    freq     = syncHeader->coding.rate ? 37800 : 18900;

                    aFrameIndex = (aFrameIndex + 1) % AUDIO_MAX_FRAMES;
                    AudioFrame *aFrame = aFrames + aFrameIndex;
                    aFrame->pos  = stream->pos - sizeof(data);
                    aFrame->size = AUDIO_CHUNK_SIZE;
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
            if (!nextFrame())
                return false;

            VideoFrame *frame = vFrames + vFrameIndex;

            BitStream bs(frame->data + 8, frame->size - 8); // make bitstream without frame header

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
                            channel[zIndex] = (ac * STR_QUANTIZATION[zIndex] * frame->qscale + 4) >> 3;
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

            return true;
        }

        virtual int decode(Sound::Frame *frames, int count) {
            if (!audioDecoder) return 0;

            int oldPos = stream->pos;

            Sound::XA *xa = (Sound::XA*)audioDecoder;

            int i = 0;
            while (i < count) {
                if (xa->pos >= COUNT(xa->buffer)) {
                    if (aFrames[curAudioFrame].size == 0) {
                        curAudioFrame = (curAudioFrame + 1) % AUDIO_MAX_FRAMES;

                        if (aFrames[curAudioFrame].size == 0) {
                            // check next 3 frames for audio frame
                            stream->setPos(oldPos);
                            for (int j = 0; j < 3; j++) {
                                nextFrame();
                                curAudioFrame = aFrameIndex;
                                if (curAudioFrame != -1 && aFrames[curAudioFrame].size != 0)
                                    break;
                            }

                            if (curAudioFrame == -1) { // no audio frames found!
                                ASSERT(false);
                                memset(frames, 0, count * sizeof(Sound::Frame));
                                return count;
                            }

                            oldPos = stream->pos;
                        }
                    }
                }
                stream->setPos(aFrames[curAudioFrame].pos);

                i += audioDecoder->decode(&frames[i], count - i);

                if (xa->pos >= COUNT(xa->buffer))
                    aFrames[curAudioFrame].size = 0;
            }

            stream->setPos(oldPos);

            return count;
        }
    };

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

        uint32 magic;
        stream->read(magic);
        stream->seek(-4);

        if (magic == 0x6F4D5241)
            decoder = new Escape(stream);
        else
            decoder = new STR(stream);

        frameData = new Color32[decoder->width * decoder->height];
        memset(frameData, 0, decoder->width * decoder->height * sizeof(Color32));

        for (int i = 0; i < 2; i++)
            frameTex[i] = new Texture(decoder->width, decoder->height, FMT_RGBA, 0, frameData);

        sample = Sound::play(decoder);

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
