#ifndef __MINIMP3_H_INCLUDED__
#define __MINIMP3_H_INCLUDED__

typedef struct _mp3_info {
    int sample_rate;
    int channels;
    int audio_bytes;  // generated amount of audio per frame
} mp3_info_t;

typedef void* mp3_decoder_t;

mp3_decoder_t mp3_create(void);
extern int mp3_decode(mp3_decoder_t dec, void *buf, int bytes, signed short *out, mp3_info_t *info);
void mp3_done(mp3_decoder_t dec);
int  mp3_decode_init();
void mp3_decode_free();

#endif//__MINIMP3_H_INCLUDED__
