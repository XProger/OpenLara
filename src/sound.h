#ifndef H_SOUND
#define H_SOUND

#define DECODE_ADPCM
#define DECODE_IMA
#define DECODE_VAG
#define DECODE_XA

#define DECODE_OGG

#if !defined(_OS_PSP) && !defined(_OS_WEB) && !defined(_OS_PSV) && !defined(_OS_3DS)
    #define DECODE_MP3
#endif

#include "utils.h"

#ifdef DECODE_MP3
    #include "libs/minimp3/minimp3.h"
#endif

#ifdef DECODE_OGG
    #ifdef USE_LIBVORBIS
        #include <tremor/ivorbisfile.h>
    #else
        #define STB_VORBIS_HEADER_ONLY
        #include "libs/stb_vorbis/stb_vorbis.c"
    #endif
#endif

#define SND_CHANNELS_MAX    128
#define SND_FADEOFF_DIST    (1024.0f * 8.0f)
#define SND_LOWPASS_FREQ    0.2f
#define SND_MAX_VOLUME      20
#define SND_PAN_FACTOR      0.7f
#define SND_FACING_FACTOR   0.3f

namespace Sound {

    static const int8 SPU_POS[] = { 0, 60, 115,  98, 122 };
    static const int8 SPU_NEG[] = { 0,  0, -52, -55, -60 };

    static const int16 SPU_ZIG_ZAG[7][30] = {
        { 0, 0, 0, 0, 0, 0, -0x0002, +0x000A, -0x0022, +0x0041, -0x0054, +0x0034, +0x0009, -0x010A, +0x0400, -0x0A78, +0x234C, +0x6794, -0x1780, +0x0BCD, -0x0623, +0x0350, -0x016D, +0x006B, +0x000A, -0x0010, +0x0011, -0x0008, +0x0003, -0x0001 },
        { 0, 0, 0, 0, -0x0002, 0, +0x0003, -0x0013, +0x003C, -0x004B, +0x00A2, -0x00E3, +0x0132, -0x0043, -0x0267, +0x0C9D, +0x74BB, -0x11B4, +0x09B8, -0x05BF, +0x0372, -0x01A8, +0x00A6, -0x001B, +0x0005, +0x0006, -0x0008, +0x0003, -0x0001, 0 },
        { 0, 0, 0, -0x0001, +0x0003, -0x0002, -0x0005, +0x001F, -0x004A, +0x00B3, -0x0192, +0x02B1, -0x039E, +0x04F8, -0x05A6, +0x7939, -0x05A6, +0x04F8, -0x039E, +0x02B1, -0x0192, +0x00B3, -0x004A, +0x001F, -0x0005, -0x0002, +0x0003, -0x0001, 0, 0 },
        { 0, 0, -0x0001, +0x0003, -0x0008, +0x0006, +0x0005, -0x001B, +0x00A6, -0x01A8, +0x0372, -0x05BF, +0x09B8, -0x11B4, +0x74BB, +0x0C9D, -0x0267, -0x0043, +0x0132, -0x00E3, +0x00A2, -0x004B, +0x003C, -0x0013, +0x0003, 0, -0x0002, 0, 0, 0 },
        { 0, -0x0001, +0x0003, -0x0008, +0x0011, -0x0010, +0x000A, +0x006B, -0x016D, +0x0350, -0x0623, +0x0BCD, -0x1780, +0x6794, +0x234C, -0x0A78, +0x0400, -0x010A, +0x0009, +0x0034, -0x0054, +0x0041, -0x0022, +0x000A, -0x0001, 0, +0x0001, 0, 0, 0 },
        { 0, +0x0002, -0x0008, +0x0010, -0x0023, +0x002B, +0x001A, -0x00EB, +0x027B, -0x0548, +0x0AFA, -0x16FA, +0x53E0, +0x3C07, -0x1249, +0x080E, -0x0347, +0x015B, -0x0044, -0x0017, +0x0046, -0x0023, +0x0011, -0x0005, 0, 0, 0, 0, 0, 0 },
        { 0, -0x0005, +0x0011, -0x0023, +0x0046, -0x0017, -0x0044, +0x015B, -0x0347, +0x080E, -0x1249, +0x3C07, +0x53E0, -0x16FA, +0x0AFA, -0x0548, +0x027B, -0x00EB, +0x001A, +0x002B, -0x0023, +0x0010, -0x0008, +0x0002, 0, 0, 0, 0, 0, 0 },
    };

    // https://problemkaputt.de/psx-spx.htm#spureverbformula
    struct ReverbPreset {
        uint16 size;
        int16  dAPF1,  dAPF2,  vIIR,   vCOMB1, vCOMB2,  vCOMB3,  vCOMB4,  vWALL;
        int16  vAPF1,  vAPF2,  mLSAME, mRSAME, mLCOMB1, mRCOMB1, mLCOMB2, mRCOMB2;
        int16  dLSAME, dRSAME, mLDIFF, mRDIFF, mLCOMB3, mRCOMB3, mLCOMB4, mRCOMB4;
        int16  dLDIFF, dRDIFF, mLAPF1, mRAPF1, mLAPF2,  mRAPF2,  vLIN,    vRIN;
    };

    static const uint16 SPU_REVERB[8][sizeof(ReverbPreset) / 2] = {
        // Room
        { 0x26C0 / 2,
          0x007D, 0x005B, 0x6D80, 0x54B8, 0xBED0, 0x0000, 0x0000, 0xBA80,
          0x5800, 0x5300, 0x04D6, 0x0333, 0x03F0, 0x0227, 0x0374, 0x01EF,
          0x0334, 0x01B5, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
          0x0000, 0x0000, 0x01B4, 0x0136, 0x00B8, 0x005C, 0x8000, 0x8000 },
        // Studio Small
        { 0x1F40 / 2,
          0x0033, 0x0025, 0x70F0, 0x4FA8, 0xBCE0, 0x4410, 0xC0F0, 0x9C00,
          0x5280, 0x4EC0, 0x03E4, 0x031B, 0x03A4, 0x02AF, 0x0372, 0x0266,
          0x031C, 0x025D, 0x025C, 0x018E, 0x022F, 0x0135, 0x01D2, 0x00B7,
          0x018F, 0x00B5, 0x00B4, 0x0080, 0x004C, 0x0026, 0x8000, 0x8000 },
        // Studio Medium
        { 0x4840 / 2,
          0x00B1, 0x007F, 0x70F0, 0x4FA8, 0xBCE0, 0x4510, 0xBEF0, 0xB4C0,
          0x5280, 0x4EC0, 0x0904, 0x076B, 0x0824, 0x065F, 0x07A2, 0x0616,
          0x076C, 0x05ED, 0x05EC, 0x042E, 0x050F, 0x0305, 0x0462, 0x02B7,
          0x042F, 0x0265, 0x0264, 0x01B2, 0x0100, 0x0080, 0x8000, 0x8000 },
        // Studio Large
        { 0x6FE0 / 2,
          0x00E3, 0x00A9, 0x6F60, 0x4FA8, 0xBCE0, 0x4510, 0xBEF0, 0xA680,
          0x5680, 0x52C0, 0x0DFB, 0x0B58, 0x0D09, 0x0A3C, 0x0BD9, 0x0973,
          0x0B59, 0x08DA, 0x08D9, 0x05E9, 0x07EC, 0x04B0, 0x06EF, 0x03D2,
          0x05EA, 0x031D, 0x031C, 0x0238, 0x0154, 0x00AA, 0x8000, 0x8000 },
        // Hall
        { 0xADE0 / 2,
          0x01A5, 0x0139, 0x6000, 0x5000, 0x4C00, 0xB800, 0xBC00, 0xC000,
          0x6000, 0x5C00, 0x15BA, 0x11BB, 0x14C2, 0x10BD, 0x11BC, 0x0DC1,
          0x11C0, 0x0DC3, 0x0DC0, 0x09C1, 0x0BC4, 0x07C1, 0x0A00, 0x06CD,
          0x09C2, 0x05C1, 0x05C0, 0x041A, 0x0274, 0x013A, 0x8000, 0x8000 },
        // Half Echo
        { 0x3C00 / 2,
          0x0017, 0x0013, 0x70F0, 0x4FA8, 0xBCE0, 0x4510, 0xBEF0, 0x8500,
          0x5F80, 0x54C0, 0x0371, 0x02AF, 0x02E5, 0x01DF, 0x02B0, 0x01D7,
          0x0358, 0x026A, 0x01D6, 0x011E, 0x012D, 0x00B1, 0x011F, 0x0059,
          0x01A0, 0x00E3, 0x0058, 0x0040, 0x0028, 0x0014, 0x8000, 0x8000 },
        // Space Echo
        { 0xF6C0 / 2,
          0x033D, 0x0231, 0x7E00, 0x5000, 0xB400, 0xB000, 0x4C00, 0xB000,
          0x6000, 0x5400, 0x1ED6, 0x1A31, 0x1D14, 0x183B, 0x1BC2, 0x16B2,
          0x1A32, 0x15EF, 0x15EE, 0x1055, 0x1334, 0x0F2D, 0x11F6, 0x0C5D,
          0x1056, 0x0AE1, 0x0AE0, 0x07A2, 0x0464, 0x0232, 0x8000, 0x8000 },
        // Reverb off
        { 0x0010 / 2,
          0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
          0x0000, 0x0000, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
          0x0000, 0x0000, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
          0x0000, 0x0000, 0x0001, 0x0001, 0x0001, 0x0001, 0x0000, 0x0000 }
    };

    struct Frame {
        int16 L, R;
    };

    struct FrameHI {
        int32 L, R;
    };

    struct Stats {
        int mixer;
        int reverb;
        int render[2];
        int ogg;
    } stats;

    namespace Filter {

        #define DSP_SCALE_BIT 8
        #define DSP_SCALE     (1 << DSP_SCALE_BIT)

        struct LowPass {
            float buffer[2][4];

            LowPass() {
                memset(buffer, 0, sizeof(buffer));
            }

            inline void process(int32 &x, float *out, float freq) {
                out[0] += freq * (float(x) - out[0]);
                out[1] += freq * (out[0] - out[1]);
                out[2] += freq * (out[1] - out[2]);
                out[3] += freq * (out[2] - out[3]);
                x = int32(out[3]);
            }

            void process(FrameHI *frames, int count, float freq) {
                for (int i = 0; i < count; i++) {
                    process(frames[i].L, buffer[0], freq);
                    process(frames[i].R, buffer[1], freq);
                }
            }
        };

        struct Reverberation {
            int32 pos;
            int16 buffer[0x8000];

            ReverbPreset preset;

            Reverberation() {
                memcpy(&preset, SPU_REVERB + 0, sizeof(preset));
                clear();
            }

            void clear() {
                // TODO
                setRoomSize(vec3(1.0f));
            }

            void setRoomSize(const vec3 &size) {
                // TODO
            };

            int16& value(int index) {
                index += pos;
                if (index >= preset.size) index -= preset.size;
                if (index <  0)           index += preset.size;
                return buffer[index];
            }

            #define R(x)   int32(preset.##x)
            #define V(x)   value(R(x))
            #define D(a,b) value(preset.##a - preset.##b)

            void process(FrameHI *frames, int count) {
                PROFILE_CPU_TIMING(stats.reverb);

                for (int i = 0; i < count; i++) {
                    FrameHI &frame = frames[i];

                    int32 Lin = frame.L;
                    int32 Rin = frame.R;

                    // Same Side Reflection (left-to-left and right-to-right)
                    V(mLSAME) = ((Lin + (V(dLSAME) * R(vWALL) >> 15) - V(mLSAME - 1)) * R(vIIR) >> 15) + V(mLSAME - 1);
                    V(mRSAME) = ((Rin + (V(dRSAME) * R(vWALL) >> 15) - V(mRSAME - 1)) * R(vIIR) >> 15) + V(mRSAME - 1);

                    // Different Side Reflection (left-to-right and right-to-left)
                    V(mLDIFF) = ((Lin + (V(dRDIFF) * R(vWALL) >> 15) - V(mLDIFF - 1)) * R(vIIR) >> 15) + V(mLDIFF - 1);
                    V(mRDIFF) = ((Rin + (V(dLDIFF) * R(vWALL) >> 15) - V(mRDIFF - 1)) * R(vIIR) >> 15) + V(mRDIFF - 1);

                    // Early Echo (Comb Filter, with input from buffer)
                    int32 Lout = (R(vCOMB1) * V(mLCOMB1) + R(vCOMB2) * V(mLCOMB2) + R(vCOMB3) * V(mLCOMB3) + R(vCOMB4) * V(mLCOMB4)) >> 15;
                    int32 Rout = (R(vCOMB1) * V(mRCOMB1) + R(vCOMB2) * V(mRCOMB2) + R(vCOMB3) * V(mRCOMB3) + R(vCOMB4) * V(mRCOMB4)) >> 15;

                    // Late Reverb APF1 (All Pass Filter 1, with input from COMB)
                    Lout = Lout - (R(vAPF1) * D(mLAPF1, dAPF1) >> 15); V(mLAPF1) = Lout; Lout = (Lout * R(vAPF1) >> 15) + D(mLAPF1, dAPF1);
                    Rout = Rout - (R(vAPF1) * D(mRAPF1, dAPF1) >> 15); V(mRAPF1) = Rout; Rout = (Rout * R(vAPF1) >> 15) + D(mRAPF1, dAPF1);

                    // Late Reverb APF2 (All Pass Filter 2, with input from APF1)
                    Lout = Lout - (R(vAPF2) * D(mLAPF2, dAPF2) >> 15); V(mLAPF2) = Lout; Lout = (Lout * R(vAPF2) >> 15) + D(mLAPF2, dAPF2);
                    Rout = Rout - (R(vAPF2) * D(mRAPF2, dAPF2) >> 15); V(mRAPF2) = Rout; Rout = (Rout * R(vAPF2) >> 15) + D(mRAPF2, dAPF2);

                    // Output to Mixer (Output volume multiplied with input from APF2)
                    frame.L = Lout;
                    frame.R = Rout;

                    pos++;
                    if (pos >= preset.size) {
                        pos -= preset.size;
                    }
                }
            }

            #undef R
            #undef V
            #undef D
        };
    };

    struct Decoder {
        Stream  *stream;
        int     channels, freq, offset;
        Frame   prevFrame;

        Decoder(Stream *stream, int channels, int freq) : stream(stream), channels(channels), freq(freq), offset(stream ? stream->pos : 0) {
            memset(&prevFrame, 0, sizeof(prevFrame));
        }

        virtual ~Decoder() { delete stream; }
        virtual int decode(Frame *frames, int count) { return 0; }
        virtual void replay() { stream->seek(offset - stream->pos); }

        int resample(Sound::Frame *frames, Sound::Frame &frame) {
            if (freq == 44100) {
                frames[0] = frame;
                return 1;
            }

            int dL = int(frame.L) - int(prevFrame.L);
            int dR = int(frame.R) - int(prevFrame.R);
            switch (freq) {
                case 11025 :
                    if (channels == 2) {
                        frames[0].L = prevFrame.L + dL / 4;                   // 0.25 L
                        frames[0].R = prevFrame.R + dR / 4;                   // 0.25 R
                        frames[1].L = prevFrame.L + dL / 2;                   // 0.50 L
                        frames[1].R = prevFrame.R + dR / 2;                   // 0.50 R
                        frames[2].L = prevFrame.L + dL * 3 / 4;               // 0.75 L
                        frames[2].R = prevFrame.R + dR * 3 / 4;               // 0.75 R
                    } else {
                        frames[0].L = frames[0].R = prevFrame.L + dL / 4;     // 0.25 LR
                        frames[1].L = frames[1].R = prevFrame.L + dL / 2;     // 0.50 LR
                        frames[2].L = frames[2].R = prevFrame.L + dL * 3 / 4; // 0.75 LR
                    }
                    frames[3] = prevFrame = frame;                            // 1.00 LR
                    return 4;
                case 22050 :
                    if (channels == 2) {
                        frames[0].L = prevFrame.L + dL / 2;                   // 0.50 L
                        frames[0].R = prevFrame.R + dR / 2;                   // 0.50 R
                    } else
                        frames[0].L = frames[0].R = prevFrame.L + dL / 2;     // 0.50 LR
                    frames[1] = prevFrame = frame;                            // 1.00 LR
                    return 2;
                default    : // impossible
                    ASSERT(false);
                    int k = 44100 / freq;
                    for (int i = 0; i < k; i++) frames[i] = frame; // no lerp
                    return k;
            }        
        }
    };

    struct PCM : Decoder {
        int size, bits;

        PCM(Stream *stream, int channels, int freq, int size, int bits) : Decoder(stream, channels, freq), size(size), bits(bits) {}

        virtual int decode(Frame *frames, int count) {
            if (stream->pos - offset >= size) return 0;

            // ! in the original game series only 11025 and 22050 Hz single channel samples were used ! //

            Frame frame;
            if (bits == 16) {
                int16 value;
                if (channels == 2) {
                    frame.L = stream->read(value);
                    frame.R = stream->read(value);
                } else
                    frame.L = frame.R = stream->read(value);
            } else if (bits == 8 || bits == -8) {

                if (bits > 0) {
                    uint8 value;
                    if (channels == 2) {
                        frame.L = stream->read(value) * 257 - 32768;
                        frame.R = stream->read(value) * 257 - 32768;
                    } else
                        frame.L = frame.R = stream->read(value) * 257 - 32768;
                } else {
                    int8 value;
                    if (channels == 2) {
                        frame.L = (stream->read(value) + 128) * 257 - 32768;
                        frame.R = (stream->read(value) + 128) * 257 - 32768;
                    } else
                        frame.L = frame.R = (stream->read(value) + 128) * 257 - 32768;
                }

            } else {
                ASSERT(false);
                return 0;
            }

            return resample(frames, frame);
        }
    };

#ifdef DECODE_ADPCM
    struct ADPCM : Decoder { // https://wiki.multimedia.cx/?title=Microsoft_ADPCM
        int size, block;

        ADPCM(Stream *stream, int channels, int freq, int size, int block) : Decoder(stream, channels, freq), size(size), block(block) {}

        struct Channel {
            int16 c1, c2;
            int16 delta;
            int16 sample1;
            int16 sample2;

            int predicate(uint8 nibble) {
                static const int table[] = { 230, 230, 230, 230, 307, 409, 512, 614, 768, 614, 512, 409, 307, 230, 230, 230 };
    
                int8 ns = nibble;
                if (ns & 8) ns -= 16;

                int sample = (sample1 * c1 + sample2 * c2) / 256 + ns * delta;
                sample  = clamp(sample, -32768, 32767);
                sample2 = sample1;
                sample1 = sample;
                delta   = max(table[nibble] * delta / 256, 16);
                return sample;
            }
        } channel[2];

        virtual int decode(Frame *frames, int count) {
            static const int coeff1[] = { 256, 512, 0, 192, 240, 460, 392 };
            static const int coeff2[] = { 0, -256, 0, 64, 0, -208, -232 };
            
            int seek = stream->pos - offset;
            if (seek >= size) return 0;

            if (seek % block == 0) {
                for (int i = 0; i < channels; i++) {
                    uint8 index;
                    stream->read(index);
                    channel[i].c1 = coeff1[index];
                    channel[i].c2 = coeff2[index];
                }
                for (int i = 0; i < channels; i++) stream->read(channel[i].delta);
                for (int i = 0; i < channels; i++) stream->read(channel[i].sample1);
                for (int i = 0; i < channels; i++) stream->read(channel[i].sample2);

                if (channels == 1) {
                    if (freq == 22050) {
                        ASSERT(count >= 4);
                        frames[0].L = frames[0].R =
                        frames[1].L = frames[1].R = channel[0].sample2;
                        frames[2].L = frames[2].R =
                        frames[3].L = frames[3].R = channel[0].sample1;
                        return 4;
                    } else {
                        ASSERT(count >= 2);
                        frames[0].L = frames[0].R = channel[0].sample2;
                        frames[1].L = frames[1].R = channel[0].sample1;
                        return 2;
                    }
                } else {
                    ASSERT(freq == 44100);
                    ASSERT(count >= 2);
                    frames[0].L = channel[0].sample2;
                    frames[0].R = channel[1].sample2;
                    frames[1].L = channel[0].sample1;
                    frames[1].R = channel[1].sample1;
                    return 2;
                }
            } else {
                uint8 value;
                stream->read(value);
                uint8 n1 = value >> 4, n2 = value & 0xF;

                if (channels == 1) {
                    if (freq == 22050) {
                        ASSERT(count >= 4);
                        frames[0].L = frames[0].R =
                        frames[1].L = frames[1].R = channel[0].predicate(n1);
                        frames[2].L = frames[2].R =
                        frames[3].L = frames[3].R = channel[0].predicate(n2);
                        return 4;
                    } else {
                        ASSERT(count >= 2);
                        frames[0].L = frames[0].R = channel[0].predicate(n1);
                        frames[1].L = frames[1].R = channel[0].predicate(n2);
                        return 2;
                    }
                } else {
                    ASSERT(freq == 44100);
                    frames[0].L = channel[0].predicate(n1);
                    frames[0].R = channel[1].predicate(n2);
                    return 1;
                }
            }
        }
    };
#endif

#ifdef DECODE_IMA
    struct IMA : Decoder { // https://wiki.multimedia.cx/?title=Microsoft_ADPCM
        struct State {
            int amp, idx;
        } state[2];

        int freq;

        IMA(Stream *stream, int channels, int freq) : Decoder(stream, channels, freq) { 
            memset(state, 0, sizeof(state)); 
        }

        int16 getSample(uint8 n, State &state) {
            static int indexLUT[] = {
                -1, -1, -1, -1, 2, 4, 6, 8,
            };

            static int stepLUT[] = {
                7,     8,     9,     10,    11,    12,    13,    14,
                16,    17,    19,    21,    23,    25,    28,    31,
                34,    37,    41,    45,    50,    55,    60,    66,
                73,    80,    88,    97,    107,   118,   130,   143,
                157,   173,   190,   209,   230,   253,   279,   307,
                337,   371,   408,   449,   494,   544,   598,   658,
                724,   796,   876,   963,   1060,  1166,  1282,  1411,
                1552,  1707,  1878,  2066,  2272,  2499,  2749,  3024,
                3327,  3660,  4026,  4428,  4871,  5358,  5894,  6484,
                7132,  7845,  8630,  9493,  10442, 11487, 12635, 13899,
                15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
                32767
            };

            int step = stepLUT[state.idx];
            int idx  = n & 7;

            state.idx = clamp(state.idx + indexLUT[idx], 0, 88);

            int diff = (2 * idx + 1) * step >> 3;

            if (n & 8) {
                state.amp -= diff;
                if (state.amp < -32768)
                    state.amp = -32768;
            } else {
                state.amp += diff;
                if (state.amp > 32767)
                    state.amp = 32767;
            }

            return state.amp;
        }

        virtual int decode(Frame *frames, int count) {
            uint8 n;
            stream->read(n);

            int a = getSample(n >> 4,   state[0]);
            int b = getSample(n & 0x0F, state[1 % channels]);

            Frame frame;
            if (channels == 2) {
                frame.L = a;
                frame.R = b;
                return resample(frames, frame);
            } else {
                frame.L = frame.R = a;
                int i = resample(frames, frame);
                frame.L = frame.R = b;
                return i + resample(frames + i, frame);
            }
        }
    };
#endif

#ifdef DECODE_VAG
    struct VAG : Decoder {
        uint8 pred, shift, flags;
        int s1, s2;
        Frame buffer[28 * 4];
        int bufferSize;

        VAG(Stream *stream) : Decoder(stream, 1, 11025), s1(0), s2(0), bufferSize(0) {}

        void predicate(short value) {
            int s = (s1 * SPU_POS[pred] + s2 * SPU_NEG[pred]) >> 6;
            s = clamp((value >> shift) + s, -32768, 32767);
            s2 = s1;
            s1 = s;
        }

        void resample(Frame *frames, short value) {
            predicate(value);
            frames[0].L = frames[0].R = s2 + (s1 - s2) / 4;     // 0.25
            frames[1].L = frames[1].R = s2 + (s1 - s2) / 2;     // 0.50
            frames[2].L = frames[2].R = s2 + (s1 - s2) * 3 / 4; // 0.75
            frames[3].L = frames[3].R = s1;                     // 1.00
        }

        int processBlock() {
            if (stream->pos >= stream->size)
                return 0;
            stream->read(pred);
            stream->read(flags);
            shift = pred & 0x0F;
            pred >>= 4;

            int i = 0;
            while (i < 14) {
                uint8 d;
                stream->read(d);
                resample(&buffer[i * 8 + 0], (d & 0x0F) << 12);
                resample(&buffer[i * 8 + 4], (d & 0xF0) <<  8);
                i++;
            }
            return i * 8;
        }

        virtual int decode(Frame *frames, int count) {
            int res = 0;

            while (res < count) {
                if (!bufferSize && !(bufferSize = processBlock())) // if no data in buffer - process next block
                    break;

                int length = min(bufferSize, count - res);
                memcpy(&frames[res], buffer, length * sizeof(Frame));
                res += length;

                if (bufferSize -= length) { // if data remained in buffer, move it to the beginning
                    memcpy(buffer, &buffer[sizeof(buffer) / sizeof(Frame) - bufferSize], bufferSize * sizeof(Frame));
                    break;
                }
            }

            return res;
        }

        virtual void replay() {
            stream->setPos(0);
            s1 = s2 = 0;
        }
    };
#endif

#ifdef DECODE_XA
    // http://problemkaputt.de/psx-spx.htm#cdromxaaudioadpcmcompression
    struct XA : Decoder {
        uint8 pred, shift, flags;
        int s1, s2;

        Frame buffer[18 * 112];
        int   pos;

        struct Group {
            uint8 params[16];
            uint8 data[112];
        } groups[18];

        Frame  prevFrames[2];

        Frame  lerpFrames[32];
        uint32 lerpPos;

        XA(Stream *stream) : Decoder(stream, 1, 11025), s1(0), s2(0), pos(COUNT(buffer)), lerpPos(0) {
            memset(prevFrames, 0, sizeof(prevFrames));
            memset(lerpFrames, 0, sizeof(lerpFrames));
        }

        void decode28(Group &group, int block, int channel) {
            int16 *dst   = channel ? &buffer[pos].R  : &buffer[pos].L;
            int16 &old   = channel ? prevFrames[0].R : prevFrames[0].L; 
            int16 &older = channel ? prevFrames[1].R : prevFrames[1].L; 

            int shift  = 12 - (group.params[4 + block * 2 + channel] & 0x0F);
            int filter =      (group.params[4 + block * 2 + channel] & 0x30) >> 4;

            int f0 = SPU_POS[filter];
            int f1 = SPU_NEG[filter];

            for (int i = 0; i < 28; i++) {
                int t = (group.data[block + i * 4] >> (channel * 4)) & 0x0F;
                if (t & 8) 
                    t -= 16;
                int s = (t << shift) + ((old * f0 + older * f1 + 32) / 64);
                s = clamp(s, -32768, 32767);
                older  = old;
                old    = s;
                dst[0] = s;
                dst += 2; // skip second channel
            }
        }

        void processBlock() {
            if (stream->pos >= stream->size)
                return;

            stream->raw(groups, sizeof(groups));

            pos = 0;

            for (int i = 0; i < COUNT(groups); i++)
                for (int j = 0; j < 4; j++) {
                    decode28(groups[i], j, 0);
                    decode28(groups[i], j, 1);
                    pos += 28;
                }

            pos = 0;
        }

        void ZigZagOut(Frame &frame, uint8 p, const int16 *LUT) {
            FrameHI sum;
            sum.L = sum.R = 0;

            for (uint8 i = 1; i < 30; i++) {
                Frame &f = lerpFrames[uint8(p - i) & 0x1F];
                sum.L += f.L * LUT[i];
                sum.R += f.R * LUT[i];
            }

            frame.L = clamp(sum.L >> 15, -32767, 32767);
            frame.R = clamp(sum.R >> 15, -32767, 32767);
        }

        virtual int decode(Frame *frames, int count) {
            if (pos >= COUNT(buffer))
                processBlock();

            ASSERT((int(COUNT(buffer)) - pos) % 6 == 0)
            ASSERT(count % 7 == 0)

            count = min(count, (int(COUNT(buffer)) - pos) / 6 * 7);

            int i = 0;
            while (i < count) {
                ASSERT(pos < COUNT(buffer));
                lerpFrames[lerpPos++ & 0x1F] = buffer[pos++];
                lerpFrames[lerpPos++ & 0x1F] = buffer[pos++];
                lerpFrames[lerpPos++ & 0x1F] = buffer[pos++];
                lerpFrames[lerpPos++ & 0x1F] = buffer[pos++];
                lerpFrames[lerpPos++ & 0x1F] = buffer[pos++];
                lerpFrames[lerpPos++ & 0x1F] = buffer[pos++];
                ZigZagOut(frames[i++], lerpPos, SPU_ZIG_ZAG[0]);
                ZigZagOut(frames[i++], lerpPos, SPU_ZIG_ZAG[1]);
                ZigZagOut(frames[i++], lerpPos, SPU_ZIG_ZAG[2]);
                ZigZagOut(frames[i++], lerpPos, SPU_ZIG_ZAG[3]);
                ZigZagOut(frames[i++], lerpPos, SPU_ZIG_ZAG[4]);
                ZigZagOut(frames[i++], lerpPos, SPU_ZIG_ZAG[5]);
                ZigZagOut(frames[i++], lerpPos, SPU_ZIG_ZAG[6]);
            }

            ASSERT(i == count);

            return count;
        }

        virtual void replay() {
            stream->setPos(0);
            s1 = s2 = 0;
        }
    };
#endif

#ifdef DECODE_MP3
    struct MP3 : Decoder {
        mp3_decoder_t   mp3;
        char    *buffer;
        int     size, pos;        

        MP3(Stream *stream, int channels) : Decoder(stream, channels, 0), size(stream->size), pos(0) {
            mp3 = mp3_create();
            buffer = new char[size]; // TODO: file streaming
            stream->raw(buffer, size);
        }

        virtual ~MP3() {
            delete[] buffer;
            mp3_done(mp3);
        }

        virtual int decode(Frame *frames, int count) {
            mp3_info_t info;
            int i = 0;
            char *ptr = (char*)frames;
            while (ptr < (char*)&frames[count]) {
                int res = mp3_decode(mp3, buffer + pos, size - pos, (short*)ptr, &info);
                if (res) {
                    pos += res;
                    ptr += info.audio_bytes;
                    i   += info.audio_bytes;
                } else
                    break;
            }
            return i;
        }

        virtual void replay() {
            mp3_done(mp3);
            mp3 = mp3_create();
            pos = 0;
        }
    };
#endif

#ifdef DECODE_OGG

#ifdef USE_LIBVORBIS
    struct OGG : Decoder {
        OggVorbis_File   vf;
        FILE            *memFile;
        uint8           *data;

        OGG(Stream *stream, int channels) : Decoder(stream, channels, 0) {
            char buf[255];
            strcpy(buf, contentDir);
            strcat(buf, stream->name);

            data = new uint8[stream->size];
            stream->raw(data, stream->size);

            memFile = fmemopen(data, stream->size, "rb");
            int err = ov_open(memFile, &vf, NULL, 0);
            ASSERT(err >= 0);
            vorbis_info *info = ov_info(&vf, -1);
            this->channels = info->channels;
            this->freq     = info->rate;
        }

        virtual ~OGG() {
            ov_clear(&vf);
            fclose(memFile);
            delete[] data;
        }

        virtual int decode(Frame *frames, int count) {
            PROFILE_CPU_TIMING(stats.ogg);
            int i = 0;
            int bytes = count * sizeof(Frame);
            while (i < bytes) {
                int bitstream;
                int res = ov_read(&vf, (char*)frames + i, bytes - i, &bitstream);
                if (!res) break;
                i += res;
            }
            return i / sizeof(Frame);
        }

        virtual void replay() {
            fseek(memFile, 0, SEEK_SET);
            ov_open(memFile, &vf, NULL, 0);
        }
    };
#else // stb_vorbis
    struct OGG : Decoder {
        stb_vorbis       *ogg;
        stb_vorbis_alloc alloc;
        uint8            *data;

        OGG(Stream *stream, int channels) : Decoder(stream, channels, 0) {
            char buf[255];
            strcpy(buf, contentDir);
            strcat(buf, stream->name);

            data = new uint8[stream->size];
            stream->raw(data, stream->size);

            alloc.alloc_buffer_length_in_bytes = 256 * 1024;
            alloc.alloc_buffer = new char[alloc.alloc_buffer_length_in_bytes];
            ogg = stb_vorbis_open_memory(data, stream->size, NULL, &alloc);
            ASSERT(ogg);
            stb_vorbis_info info = stb_vorbis_get_info(ogg);
            this->channels = info.channels;
            this->freq     = info.sample_rate;
        }

        virtual ~OGG() {
            stb_vorbis_close(ogg);
            delete[] alloc.alloc_buffer;
            delete[] data;
        }

        virtual int decode(Frame *frames, int count) {
            PROFILE_CPU_TIMING(stats.ogg);
            int i = 0;
            while (i < count) {
                int res = stb_vorbis_get_samples_short_interleaved(ogg, channels, (short*)frames + i, (count - i) * 2);
                if (!res) break;
                i += res;
            }
            return i;
        }

        virtual void replay() {
            stb_vorbis_seek_start(ogg);
        }
    };
#endif

#endif // DECODE_OGG

    Core::Mutex lock;

    struct Listener {
        mat4 matrix;
        bool underwater;
    } listener[2];

    int listenersCount;

    Listener& getListener(const vec3 &pos) {
        if (listenersCount == 1 || (listener[0].matrix.getPos() - pos).length2() < (listener[1].matrix.getPos() - pos).length2())
            return listener[0];
        return listener[1];
    }

    enum Flags {
        LOOP            = 1,
        PAN             = 2,
        UNIQUE          = 4,
        REPLAY          = 8,
        MUSIC           = 16,
        FLIPPED         = 32,
        UNFLIPPED       = 64,
    };

    bool flipped;

    struct Sample {
        const vec3 *uniquePtr;
        Decoder *decoder;
        vec3    pos;
        float   volume;
        float   volumeTarget;
        float   volumeDelta;
        float   pitch;
        int     flags;
        int     id;
        bool    isPlaying;
        bool    isPaused;
        bool    stopAfterFade;

        Sample(Decoder *decoder, float volume, float pitch, int flags, int id) : uniquePtr(NULL), decoder(decoder), volume(volume), volumeTarget(volume), volumeDelta(0.0f), pitch(pitch), flags(flags), id(id) {
            isPlaying = decoder != NULL;
            isPaused  = false;
        }

        Sample(Stream *stream, const vec3 *pos, float volume, float pitch, int flags, int id) : uniquePtr(pos), decoder(NULL), volume(volume), volumeTarget(volume), volumeDelta(0.0f), pitch(pitch), flags(flags), id(id) {
            this->pos = pos ? *pos : vec3(0.0f);
            
            uint32 fourcc;
            stream->read(fourcc);
            if (fourcc == FOURCC("RIFF")) { // wav

                struct {
                    uint16  format;
                    uint16  channels;
                    uint32  samplesPerSec;
                    uint32  bytesPerSec;
                    uint16  block;
                    uint16  sampleBits;
                } waveFmt;

                stream->seek(8);
                while (stream->pos < stream->size) {
                    uint32 type, size;
                    stream->read(type);
                    stream->read(size);
                    if (type == FOURCC("fmt ")) {
                        stream->raw(&waveFmt, sizeof(waveFmt));
                        stream->seek(size - sizeof(waveFmt));
                    } else if (type == FOURCC("data")) {
                        if (waveFmt.format == 1) decoder = new PCM(stream, waveFmt.channels, waveFmt.samplesPerSec, size, waveFmt.sampleBits);
                        #ifdef DECODE_ADPCM
                        if (waveFmt.format == 2) decoder = new ADPCM(stream, waveFmt.channels, waveFmt.samplesPerSec, size, waveFmt.block);
                        #endif
                        break;
                    } else
                        stream->seek(size);
                }
            } 
            else if (fourcc == FOURCC("OggS")) { // ogg
                stream->seek(-4);
                #ifdef DECODE_OGG
                    decoder = new OGG(stream, 2);
                #endif 
            }
            else if (fourcc == FOURCC("ID3\3")) { // mp3
                #ifdef DECODE_MP3
                    decoder = new MP3(stream, 2);
                #endif
            }
            else if (fourcc == FOURCC("SEGA")) { // Sega Saturn PCM mono signed 8-bit 11025 Hz
                decoder = new PCM(stream, 1, 11025, stream->size, -8);
            }
            else { // vag
                stream->setPos(0);
                #ifdef DECODE_VAG
                    decoder = new VAG(stream);
                #endif
            }

            if (!decoder)
                delete stream;

            isPlaying = decoder != NULL;
            isPaused  = false;
        }

        ~Sample() {
            delete decoder;
        }

        void setVolume(float value, float time) {
            if (value < 0.0f) {
                stopAfterFade = true;
                value = 0.0f;
            } else
                stopAfterFade = false;

            volumeTarget = value;
            volumeDelta  = volumeTarget - volume;
            if (time > 0.0f)
                volumeDelta /= 44100.0f * time;
        }

        vec2 getPan() {
            if (!(flags & PAN))
                return vec2(1.0f);
            mat4  m = Sound::getListener(pos).matrix;
            vec3  v = pos - m.offset().xyz();
            vec3  n = v.normal();

            float dist   = max(0.0f, 1.0f - (v.length() / SND_FADEOFF_DIST));
            float pan    = m.right().xyz().dot(n);
            float facing = (0.5f - m.dir().xyz().dot(n) * 0.5f) * SND_FACING_FACTOR + (1.0f - SND_FACING_FACTOR);

            vec2  value(min(1.0f, 1.0f - pan),
                        min(1.0f, 1.0f + pan));

            return (value * SND_PAN_FACTOR + (1.0f - SND_PAN_FACTOR)) * facing * dist;
        }

        bool render(Frame *frames, int count) {
            if (!isPlaying) return false;

            if (isPaused) {
                memset(frames, 0, sizeof(Frame) * count);
                return true;
            }

        // decode
            int i = 0;
            while (i < count) {
                int res = decoder->decode(&frames[i], count - i);
                if (res == 0) {
                    if (!(flags & LOOP)) {
                        isPlaying = false;
                        break;
                    } else
                        decoder->replay();
                }
                i += res;
            }
        // apply volume
            #define VOL_CONV(x) (1.0f - sqrtf(1.0f - x * x));

            float m = ((flags & MUSIC) ? Core::settings.audio.music : Core::settings.audio.sound) / float(SND_MAX_VOLUME);
            float v = volume * m;
            vec2 pan = getPan();
            vec2 vol = pan * VOL_CONV(v);
            for (int j = 0; j < i; j++) {
                if (volumeDelta != 0.0f) { // increase / decrease channel volume
                    volume += volumeDelta;
                    if ((volumeDelta < 0.0f && volume < volumeTarget) ||
                        (volumeDelta > 0.0f && volume > volumeTarget)) {
                        volume = volumeTarget;
                        volumeDelta = 0.0f;
                        if (stopAfterFade)
                            isPlaying = false;
                    }
                    v   = volume * m;
                    vol = pan * VOL_CONV(v);
                }
                frames[j].L = int(frames[j].L * vol.x);
                frames[j].R = int(frames[j].R * vol.y);
            }
            #undef VOL_CONV

            return true;
        }

        void stop() {
            isPlaying = false;
        }

        void replay() {
            decoder->replay();
        }

        void pause() {
            isPaused = true;
        }

        void resume() {
            isPaused = false;
        }
    } *channels[SND_CHANNELS_MAX];
    int channelsCount;

    typedef void (Callback)(Sample *channel);
    Callback *callback;

    FrameHI *result;
    Frame   *buffer;

    // TODO: per listener
    Filter::Reverberation reverb;
    Filter::LowPass       lowPass;

    void init() {
        flipped = false;
        channelsCount = 0;
        callback = NULL;
        buffer = NULL;
        result = NULL;
    #ifdef DECODE_MP3
        mp3_decode_init();
    #endif
    }

    void deinit() {
        for (int i = 0; i < channelsCount; i++)
            delete channels[i];
    #ifdef DECODE_MP3
        mp3_decode_free();
    #endif
        delete[] buffer;
        delete[] result;
    }

    void renderChannels(FrameHI *result, int count, bool music) {
        PROFILE_CPU_TIMING(stats.render[music]);

        int bufSize = count + count / 2 + 4;
        if (!buffer) buffer = new Frame[bufSize]; // + 50% for pitch

        for (int i = 0; i < channelsCount; i++) {
            if (music != ((channels[i]->flags & MUSIC) != 0))
                continue;
            
            if (channels[i]->flags & (FLIPPED | UNFLIPPED)) {
                if (!(channels[i]->flags & (flipped ? FLIPPED : UNFLIPPED)))
                    continue;

                vec3 d = channels[i]->pos - getListener(channels[i]->pos).matrix.getPos();
                if (fabsf(d.x) > SND_FADEOFF_DIST || fabsf(d.y) > SND_FADEOFF_DIST || fabsf(d.z) > SND_FADEOFF_DIST)
                    continue;
            }

            if ((channels[i]->flags & LOOP) && channels[i]->volume < EPS && channels[i]->volumeTarget < EPS)
                continue;

            memset(buffer, 0, sizeof(Frame) * bufSize);
            channels[i]->render(buffer, (int(count * channels[i]->pitch) + 3) / 4 * 4);

            if (channels[i]->pitch == 1.0f) { // no pitch
                for (int j = 0; j < count; j++) {
                    result[j].L += buffer[j].L;
                    result[j].R += buffer[j].R;
                }
            } else { // has pitch (interpolate values for smooth wave)
                float t = 0.0f;
                for (int j = 0; j < count; j++, t += channels[i]->pitch) {
                    int idxA = int(t);
                    int idxB = (j == (count - 1)) ? idxA : (idxA + 1);
                    int st = int((t - idxA) * DSP_SCALE);
                    Frame &a = buffer[idxA];
                    Frame &b = buffer[idxB];

                    result[j].L += a.L + ((b.L - a.L) * st >> DSP_SCALE_BIT);
                    result[j].R += a.R + ((b.R - a.R) * st >> DSP_SCALE_BIT);
                }
            }
        }
    }

    void convFrames(FrameHI *from, Frame *to, int count) {
        for (int i = 0; i < count; i++) {
            to[i].L = clamp(from[i].L, -32767, 32767);
            to[i].R = clamp(from[i].R, -32767, 32767);
        }
    }

    void fill(Frame *frames, int count) {
        OS_LOCK(lock);
        PROFILE_CPU_TIMING(stats.mixer);

        if (!channelsCount) {
            if (result && (Core::settings.audio.music != 0 || Core::settings.audio.sound != 0)) {
                memset(result, 0, sizeof(FrameHI) * count);
                if (Core::settings.audio.reverb)
                    reverb.process(result, count);
                convFrames(result, frames, count);
            } else
                memset(frames, 0, sizeof(frames[0]) * count);
            return;
        }

        if (!result) result = new FrameHI[count];
        memset(result, 0, sizeof(FrameHI) * count);

        if (Core::settings.audio.sound != 0) {
            renderChannels(result, count, false);

            if (Core::settings.audio.reverb) {
                if (listener[0].underwater) {
                    lowPass.process(result, count, SND_LOWPASS_FREQ);
                }
                reverb.process(result, count);
            }
        }

        if (Core::settings.audio.music != 0) {
            renderChannels(result, count, true);
        }

        convFrames(result, frames, count);

        for (int i = 0; i < channelsCount; i++) 
            if (!channels[i]->isPlaying) {
                if (callback) callback(channels[i]);
                delete channels[i];
                channels[i] = channels[--channelsCount];
                i--;
            }
    }

    Stream *openCDAudioWAD(const char *name, int index = -1) {
        if (!Stream::existsContent(name))
            return NULL;

        Stream *stream = new Stream(name);
        if (stream->size) {
            struct Item {
                char name[260];
                int  size;
                int  offset;
            } entity;

            stream->seek(sizeof(entity) * index);
            stream->raw(&entity, sizeof(entity));
            stream->setPos(entity.offset);
            return stream;
        }
        delete stream;
        return NULL;
    }

    Stream *openCDAudioMP3(const char *dat, const char *name, int index = -1) {
        if (!Stream::existsContent(dat) || !Stream::existsContent(name))
            return NULL;
        Stream *stream = new Stream(name);
        return stream;
    }

    Sample* getChannel(int id, const vec3 *pos) {
        for (int i = 0; i < channelsCount; i++)
            if (channels[i]->id == id && channels[i]->uniquePtr == pos)
                return channels[i];
        return NULL;
    }

    Sample* play(Stream *stream, const vec3 *pos = NULL, float volume = 1.0f, float pitch = 0.0f, int flags = 0, int id = - 1) {
        OS_LOCK(lock);

        ASSERT(pitch >= 0.0f);
        if (!stream) return NULL;
        if (volume > 0.001f) {
            if (pos && !(flags & (FLIPPED | UNFLIPPED | MUSIC)) && (flags & PAN)) {
                vec3 listenerPos = getListener(*pos).matrix.getPos();
                vec3 d = *pos - listenerPos;

                if (fabsf(d.x) > SND_FADEOFF_DIST || fabsf(d.y) > SND_FADEOFF_DIST || fabsf(d.z) > SND_FADEOFF_DIST) {
                    delete stream;
                    return NULL;
                }
            }

            if (flags & (UNIQUE | REPLAY)) {
                Sample *ch = getChannel(id, pos);

                if (ch) {
                    if (pos)
                        ch->pos = *pos;
                    ch->pitch = pitch;

                    if (flags & REPLAY)
                        ch->replay();

                    delete stream;
                    return ch;
                }
            }

            if (channelsCount < SND_CHANNELS_MAX)
                return channels[channelsCount++] = new Sample(stream, pos, volume, pitch, flags, id);

            LOG("! no free channels\n");
        }
        delete stream;
        return NULL;
    }

    Sample* play(Decoder *decoder) {
        OS_LOCK(lock);

        if (channelsCount < SND_CHANNELS_MAX)
            return channels[channelsCount++] = new Sample(decoder, 1.0f, 1.0f, MUSIC, -1);
        return NULL;
    }

    void stop(int id = -1) {
        OS_LOCK(lock);

        for (int i = 0; i < channelsCount; i++)
            if (id == -1 || channels[i]->id == id)
                channels[i]->stop();
    }

    void stopAll() {
        OS_LOCK(lock);
        reverb.clear();

        for (int i = 0; i < channelsCount; i++)
            delete channels[i];
        channelsCount = 0;
    }
}

#endif
