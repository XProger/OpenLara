#ifndef H_SOUND
#define H_SOUND

//#define DECODE_ADPCM
//#define DECODE_MP3
#define DECODE_OGG

#ifdef __EMSCRIPTEN__ // TODO: http streaming
    #undef DECODE_MP3
    #undef DECODE_OGG
#endif

#include "utils.h"
#ifdef DECODE_MP3
    #include "libs/minimp3/minimp3.h"
#endif
#ifdef DECODE_OGG
    #define STB_VORBIS_HEADER_ONLY
    #include "libs/stb_vorbis/stb_vorbis.c"
#endif

#define SND_CHANNELS_MAX    32

namespace Sound {

    struct Frame {
        short L, R;
    };

    struct Decoder {
        Stream  *stream;
        int     channels, offset;

        Decoder(Stream *stream, int channels) : stream(stream), channels(channels), offset(stream->pos) {}
        virtual ~Decoder() { delete stream; }
        virtual int decode(Frame *frames, int count) { return 0; }
    };

    struct PCM : Decoder {
        int freq, size, bits;

        PCM(Stream *stream, int channels, int freq, int size, int bits) : Decoder(stream, channels), freq(freq), size(size), bits(bits) {}

        virtual int decode(Frame *frames, int count) {
            if (stream->pos - offset >= size) return 0;
            if (bits == 16) {
                int16 value;
                if (channels == 2) {
                    frames[0].L = stream->read(value);
                    frames[0].R = stream->read(value);
                } else
                    frames[0].L = frames[0].R = stream->read(value);
            } else if (bits == 8) {
                uint8 value;
                if (channels == 2) {
                    frames[0].L = stream->read(value) * 257 - 32768;
                    frames[0].R = stream->read(value) * 257 - 32768;
                } else
                    frames[0].L = frames[0].R = stream->read(value) * 257 - 32768;
            } else {
                ASSERT(false);
                return 0;
            }

            int k = 44100 / freq;
            for (int i = 1; i < k; i++) frames[i] = frames[0];
            return k;
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
                    char index;
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
    };
#endif

#ifdef DECODE_OGG
    struct OGG : Decoder {
        stb_vorbis       *ogg;
        stb_vorbis_alloc alloc;

        char       *buffer;
        int        size, pos;        

        OGG(Stream *stream, int channels) : Decoder(stream, channels), size(stream->size), pos(0) {
            buffer = new char[size]; // TODO: file streaming
            stream->raw(buffer, size);
            int error;
            alloc.alloc_buffer_length_in_bytes = 256 * 1024;
            alloc.alloc_buffer = new char[alloc.alloc_buffer_length_in_bytes];
            ogg = stb_vorbis_open_memory((unsigned char*)buffer, size, &error, &alloc);
            stb_vorbis_info info = stb_vorbis_get_info(ogg);
            channels = info.channels;
        }

        virtual ~OGG() {
            stb_vorbis_close(ogg);
            delete[] alloc.alloc_buffer;
            delete[] buffer;
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
    };
#endif

    struct Listener {
        mat4 matrix;
        vec3 velocity;
    } listener;

    enum Flags {
        LOOP            = 1,
        PAN             = 2,
        REVERB_NEAR     = 4,
        REVERB_MIDDLE   = 8,
        REVERB_FAR      = 16,
    };

    struct Sample {
        Decoder *decoder;
        float   volume;
        float   pitch;
        int     flags;
        bool    isPlaying;

        Sample(Stream *stream, float volume, float pitch, int flags) : decoder(NULL), volume(volume), pitch(pitch), flags(flags) {
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
                        stream->read(waveFmt);
                        stream->seek(size - sizeof(waveFmt));
                    } else if (type == FOURCC("data")) {
                        if (waveFmt.format == 1) decoder = new   PCM(stream, waveFmt.channels, waveFmt.samplesPerSec, size, waveFmt.sampleBits);
                        #ifdef DECODE_ADPCM
                        if (waveFmt.format == 2) decoder = new ADPCM(stream, waveFmt.channels, size, waveFmt.block);
                        #endif
                        break;
                    } else
                        stream->seek(size);
                }
            } 
            #ifdef DECODE_OGG
            else if (fourcc == FOURCC("OggS")) { // ogg
                stream->seek(-4);
                decoder = new OGG(stream, 2);
            }
            #endif 
            #ifdef DECODE_MP3
            else if (fourcc == FOURCC("ID3\3")) { // mp3                
                decoder = new MP3(stream, 2);
            }
            #endif
			
            isPlaying = decoder != NULL;
            ASSERT(isPlaying);
        }

        ~Sample() {
            delete decoder;
        }

        bool render(Frame *frames, int count) {
            if (!isPlaying) return 0;
            int i = 0;
            while (i < count) {
                int res = decoder->decode(&frames[i], count - i);
                if (res == 0) {
                    if (i == 0) isPlaying = false;
                    break;
                }
                i += res;
            }
            return true;
        }

    } *channels[SND_CHANNELS_MAX];
    int channelsCount;

    void init() {
        channelsCount = 0;
    #ifdef DECODE_MP3
        mp3_decode_init();
    #endif
    }

    void free() {
        for (int i = 0; i < channelsCount; i++)
            delete channels[i];
    #ifdef DECODE_MP3
        mp3_decode_free();
    #endif
    }

    void fill(Frame *frames, int count) {
        struct FrameHI {
            int L, R;
        };

        FrameHI *result = new FrameHI[count];
        memset(result, 0, sizeof(FrameHI) * count);

        Frame *buffer = new Frame[count];

        for (int i = 0; i < channelsCount; i++) {
            
            memset(buffer, 0, sizeof(Frame) * count);
            channels[i]->render(buffer, count);
            
            for (int j = 0; j < count; j++) {
                result[j].L += buffer[j].L;
                result[j].R += buffer[j].R;
            }
        }

        for (int i = 0; i < count; i++) {
            frames[i].L = clamp(result[i].L, -32768, 32767);
            frames[i].R = clamp(result[i].R, -32768, 32767);
        }

        delete[] buffer;
        delete[] result;

        for (int i = 0; i < channelsCount; i++) 
            if (!channels[i]->isPlaying) {
                delete channels[i];
                channels[i] = channels[--channelsCount];
                i--;
            }
    }

    Stream *openWAD(const char *name) {
        Stream *stream = new Stream("cdaudio.wad");
        if (stream->size) {
            struct Item {
                char name[260];
                int  size;
                int  offset;
            } entity;

            for (int i = 0; i < 130; i++) {
                stream->read(entity);
                if (strcmp(name, entity.name) == 0) {
                    stream->setPos(entity.offset);
                    return stream;
                }
            }
        }
        delete stream;
        return NULL;
    }

    void play(Stream *stream, float volume, float pitch, int flags) {
        if (!stream) return;
        if (channelsCount < SND_CHANNELS_MAX)
            channels[channelsCount++] = new Sample(stream, volume, pitch, flags);
        else {
            LOG("! no free channels\n");  
            delete stream;
        }
    }
}

#endif