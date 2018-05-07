#ifndef H_SOUND
#define H_SOUND

#define DECODE_VAG
#define DECODE_ADPCM

#define DECODE_OGG

#if !defined(_OS_PSP) && !defined(_OS_WEB)
    #define DECODE_MP3
#endif

#include "utils.h"

#ifdef DECODE_MP3
    #include "libs/minimp3/minimp3.h"
#endif

#ifdef DECODE_OGG
    #define STB_VORBIS_HEADER_ONLY
    #include "libs/stb_vorbis/stb_vorbis.c"
#endif

#define SND_CHANNELS_MAX    128
#define SND_FADEOFF_DIST    (1024.0f * 8.0f)
#define SND_MAX_VOLUME      20

namespace Sound {

    struct Frame {
        short L, R;
    };

    struct FrameHI {
        int L, R;
    };

    namespace Filter {
        #define MAX_FDN     16
        #define MAX_DELAY   1024

        static const int16 FDN[MAX_FDN] = { 281, 331, 373, 419, 461, 503, 547, 593, 641, 683, 727, 769, 811, 853, 907, 953 };

        struct Delay {
            int     index;
            float   out[MAX_DELAY];

            float process(float x, int delay) {
                index = (index + 1) % delay;
                float y = out[index];
                return out[index] = x, y;
            }
        };

        struct Absorption {
            float   out;

            float process(float x, float gain, float damping) {
                return out = out * damping + x * gain * (1.0f - damping);
            }
        };

        struct Reverberation {
            Delay       df[MAX_FDN];
            Absorption  af[MAX_FDN];

            float       output[MAX_FDN];
            float       panCoeff[MAX_FDN][2];
            float       absCoeff[MAX_FDN][2];   // absorption gain & damping

            Reverberation() {
                float k = 1.0f / MAX_FDN;

                for (int i = 0; i < MAX_FDN; i++) {
                    output[i]      = 0.0f;
                    panCoeff[i][0] = (i % 2) ? -k : k;
                }

                for (int i = 0; i < MAX_FDN; i += 2) {
                    if ((i / 2) % 2)
                        panCoeff[i][1] = panCoeff[i + 1][1] = -k;
                    else
                        panCoeff[i][1] = panCoeff[i + 1][1] =  k;
                }

                setRoomSize(vec3(1.0f));
            }

            void setRoomSize(const vec3 &size) {
                float S = (size.x * size.z + size.x * size.y + size.z * size.y) * 2;
                float V = size.x * size.y * size.z;
                float f = 0.1f; // absorption factor
                float rt60 = 0.161f * (V / (S * f));
                float k = -10.0f / (44100.0f * rt60);

                for (int i = 0; i < MAX_FDN; i++) {
                    absCoeff[i][0] = powf(10.0f, FDN[i] * k);
                    absCoeff[i][1] = 1.0f - (2.0f / (1.0f + powf(absCoeff[i][0], 1.0f - 1.0f / 0.15f)));
                }
            };

            void process(FrameHI *frames, int count) {
                float buffer[MAX_FDN];

                for (int i = 0; i < count; i++) {
                    FrameHI &frame = frames[i];
                    float L   = frame.L * (1.0f / 32768.0f);
                    float R   = frame.R * (1.0f / 32768.0f);
                    float in  = (L + R) * 0.5f;
                    float out = 0.0f;

                // apply delay & absorption filters
                    for (int j = 0; j < MAX_FDN; j++) {
                        float k = in + output[j];
                        k = df[j].process(k, FDN[j]);
                        k = af[j].process(k, absCoeff[j][0], absCoeff[j][1]);
                        out += k * (2.0f / MAX_FDN);
                        buffer[j] = k;
                    }

                // apply pan
                    for (int j = 0; j < MAX_FDN; j++) {
                        output[j] = out - buffer[(j + MAX_FDN - 1) % MAX_FDN];
                        if (output[j] < EPS)
                            output[j] = 0.0f;
                        L += buffer[j] * panCoeff[j][0];
                        R += buffer[j] * panCoeff[j][1];
                    }

                    frame.L = int(L * 32768.0f);
                    frame.R = int(R * 32768.0f);
                }
            }
        };

        #undef MAX_FDN
        #undef MAX_DELAY
    };

    struct Decoder {
        Stream  *stream;
        int     channels, offset;

        Decoder(Stream *stream, int channels) : stream(stream), channels(channels), offset(stream->pos) {}
        virtual ~Decoder() { delete stream; }
        virtual int decode(Frame *frames, int count) { return 0; }
        virtual void replay() { stream->seek(offset - stream->pos); }
    };

    struct PCM : Decoder {
        int freq, size, bits;
        Frame frameLast;

        PCM(Stream *stream, int channels, int freq, int size, int bits) : Decoder(stream, channels), freq(freq), size(size), bits(bits) { frameLast.L = frameLast.R = 0; }

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
            } else if (bits == 8) {
                uint8 value;
                if (channels == 2) {
                    frame.L = stream->read(value) * 257 - 32768;
                    frame.R = stream->read(value) * 257 - 32768;
                } else
                    frame.L = frame.R = stream->read(value) * 257 - 32768;
            } else {
                ASSERT(false);
                return 0;
            }

            int dL = int(frame.L) - int(frameLast.L);
            int dR = int(frame.R) - int(frameLast.R);
            switch (freq) {
                case 11025 :
                    if (channels == 2) {
                        frames[0].L = frameLast.L + dL / 4;                   // 0.25 L
                        frames[0].R = frameLast.R + dR / 4;                   // 0.25 R
                        frames[1].L = frameLast.L + dL / 2;                   // 0.50 L
                        frames[1].R = frameLast.R + dR / 2;                   // 0.50 R
                        frames[2].L = frameLast.L + dL * 3 / 4;               // 0.75 L
                        frames[2].R = frameLast.R + dR * 3 / 4;               // 0.75 R
                    } else {
                        frames[0].L = frames[0].R = frameLast.L + dL / 4;     // 0.25 LR
                        frames[1].L = frames[1].R = frameLast.L + dL / 2;     // 0.50 LR
                        frames[2].L = frames[2].R = frameLast.L + dL * 3 / 4; // 0.75 LR
                    }
                    frames[3] = frameLast = frame;                            // 1.00 LR
                    return 4;
                case 22050 :
                    if (channels == 2) {
                        frames[0].L = frameLast.L + dL / 2;                   // 0.50 L
                        frames[0].R = frameLast.R + dR / 2;                   // 0.50 R
                    } else
                        frames[0].L = frames[0].R = frameLast.L + dL / 2;     // 0.50 LR
                    frames[1] = frameLast = frame;                            // 1.00 LR
                    return 2;
                case 44100 : // not used
                    frames[0] = frameLast = frame;
                    return 1;
                default    : // impossible
                    ASSERT(false);
                    int k = 44100 / freq;
                    for (int i = 0; i < k; i++) frames[i] = frame; // no lerp
                    return k;
            }
        }
    };

#ifdef DECODE_ADPCM
    struct ADPCM : Decoder { // https://wiki.multimedia.cx/?title=Microsoft_ADPCM
        int size, block;

        ADPCM(Stream *stream, int channels, int size, int block) : Decoder(stream, channels), size(size), block(block) {}

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
                    frames[0].L = frames[0].R = channel[0].sample2;
                    frames[1].L = frames[1].R = channel[0].sample1;
                } else {
                    frames[0].L = channel[0].sample2;
                    frames[0].R = channel[1].sample2;
                    frames[1].L = channel[0].sample1;
                    frames[1].R = channel[1].sample1;
                }
                return 2;
            } else {
                uint8 value;
                stream->read(value);
                uint8 n1 = value >> 4, n2 = value & 0xF;

                if (channels == 1) {
                    frames[0].L = frames[0].R = channel[0].predicate(n1);
                    frames[1].L = frames[1].R = channel[0].predicate(n2);
                    return 2;
                } else {
                    frames[0].L = channel[0].predicate(n1);
                    frames[0].R = channel[1].predicate(n2);
                    return 1;
                }
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

        VAG(Stream *stream) : Decoder(stream, 1), s1(0), s2(0), bufferSize(0) {}

        void predicate(short value) {
            int inc[] = { 0, 60, 115,  98, 122 };
            int dec[] = { 0,  0, -52, -55, -60 };

            int s = (s1 * inc[pred] + s2 * dec[pred]) >> 6;
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

#ifdef DECODE_MP3
    struct MP3 : Decoder {
        mp3_decoder_t   mp3;
        char    *buffer;
        int     size, pos;        

        MP3(Stream *stream, int channels) : Decoder(stream, channels), size(stream->size), pos(0) {
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
    struct OGG : Decoder {
        stb_vorbis       *ogg;
        stb_vorbis_alloc alloc;

        OGG(Stream *stream, int channels) : Decoder(stream, channels), ogg(NULL) {
            char buf[255];
            strcpy(buf, Stream::contentDir);
            strcat(buf, stream->name);

            alloc.alloc_buffer_length_in_bytes = 256 * 1024;
            alloc.alloc_buffer = new char[alloc.alloc_buffer_length_in_bytes];
            ogg = stb_vorbis_open_filename(buf, NULL, &alloc);
            ASSERT(ogg);
            stb_vorbis_info info = stb_vorbis_get_info(ogg);
            this->channels = info.channels;
        }

        virtual ~OGG() {
            stb_vorbis_close(ogg);
            delete[] alloc.alloc_buffer;
        }

        virtual int decode(Frame *frames, int count) {
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
    Mutex lock;

    struct Listener {
        mat4 matrix;
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
        Decoder *decoder;
        vec3    pos;
        float   volume;
        float   volumeTarget;
        float   volumeDelta;
        float   pitch;
        int     flags;
        int     id;
        bool    isPlaying;
        bool    stopAfterFade;

        Sample(Stream *stream, const vec3 &pos, float volume, float pitch, int flags, int id) : decoder(NULL), pos(pos), volume(volume), volumeTarget(volume), volumeDelta(0.0f), pitch(pitch), flags(flags), id(id) {
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
                        if (waveFmt.format == 2) decoder = new ADPCM(stream, waveFmt.channels, size, waveFmt.block);
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
            else { // vag
                stream->setPos(0);
                #ifdef DECODE_VAG
                    decoder = new VAG(stream);
                #endif
            }

            if (!decoder)
                delete stream;

            isPlaying = decoder != NULL;
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

            float dist = max(0.0f, 1.0f - (v.length() / SND_FADEOFF_DIST));
            float pan  = m.right().xyz().dot(v.normal());

            float l = min(1.0f, 1.0f - pan);
            float r = min(1.0f, 1.0f + pan);

            return vec2(l, r) * dist;
        }

        bool render(Frame *frames, int count) {
            if (!isPlaying) return false;
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
    } *channels[SND_CHANNELS_MAX];
    int channelsCount;

    typedef void (Callback)(Sample *channel);
    Callback *callback;

    FrameHI *result;
    Frame   *buffer;
    Filter::Reverberation reverb;

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
        int bufSize = count + count / 2;
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
            channels[i]->render(buffer, int(count * channels[i]->pitch));

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
                    float k = t - idxA;
                    result[j].L += int(lerp(buffer[idxA].L, buffer[idxB].L, k));
                    result[j].R += int(lerp(buffer[idxA].R, buffer[idxB].R, k));
                }
            }
        }
    }

    void convFrames(FrameHI *from, Frame *to, int count) {
        for (int i = 0; i < count; i++) {
            to[i].L = clamp(from[i].L, -32768, 32767);
            to[i].R = clamp(from[i].R, -32768, 32767);
        }
    }

    void fill(Frame *frames, int count) {
        OS_LOCK(lock);

        if (!channelsCount) {
            if (result) {
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

        renderChannels(result, count, false);

        if (Core::settings.audio.reverb)
            reverb.process(result, count);

        renderChannels(result, count, true);

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

    Sample* play(Stream *stream, const vec3 &pos, float volume = 1.0f, float pitch = 0.0f, int flags = 0, int id = - 1) {
        OS_LOCK(lock);

        ASSERT(pitch >= 0.0f);
        if (!stream) return NULL;
        if (volume > 0.001f) {
            vec3 listenerPos = getListener(pos).matrix.getPos();

            if (!(flags & (FLIPPED | UNFLIPPED | MUSIC)) && (flags & PAN)) {
                vec3 d = pos - listenerPos;
                if (fabsf(d.x) > SND_FADEOFF_DIST || fabsf(d.y) > SND_FADEOFF_DIST || fabsf(d.z) > SND_FADEOFF_DIST) {
                    delete stream;
                    return NULL;
                }
            }

            if (flags & (UNIQUE | REPLAY)) {
                for (int i = 0; i < channelsCount; i++)
                    if (channels[i]->id == id) {
                        vec3 p = listenerPos;

                        if ((p - channels[i]->pos).length2() > (p - pos).length2()) {
                            channels[i]->pos = pos;
                            channels[i]->pitch = pitch;
                        }

                        if (flags & REPLAY)
                            channels[i]->replay();

                        delete stream;
                        return channels[i];
                    }
            }

            if (channelsCount < SND_CHANNELS_MAX)
                return channels[channelsCount++] = new Sample(stream, pos, volume, pitch, flags, id);

            LOG("! no free channels\n");
        }
        delete stream;
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

        for (int i = 0; i < channelsCount; i++)
            delete channels[i];
        channelsCount = 0;
    }
}

#endif