/* splite 20 oct 2019, inspired by https://gist.github.com/ghedo/963382 */
/* compile: gcc -o testaudio testaudio.c -lasound */
/* usage: see example main() code that reads from AU audio file argv[1]
   in blocks of bufsiz = 4096 bytes and writes to alsa audio device
   in blocks of bufsiz */
/* use /usr/bin/aplay on pod machines (not installed on escher) to check
   audio quality of pp.au and kline-jarrett.au without streaming */
/* in lab6 of pseudo-real-time audio streaming server use mulatopen(),
   mulawrite(), mulaclose() to write AU audio data received at client from
   server to audio device in unit of bufsiz = 4096 bytes */

/* audio codec library functions */

#include "mulaw.h"


snd_pcm_t *mulawdev;
snd_pcm_uframes_t mulawfrms;


void mulawopen(size_t *bufsiz) {
    snd_pcm_hw_params_t *p;
    unsigned int rate = 8000;

    snd_pcm_open(&mulawdev, "default", SND_PCM_STREAM_PLAYBACK, 0);
    snd_pcm_hw_params_alloca(&p);
    snd_pcm_hw_params_any(mulawdev, p);
    snd_pcm_hw_params_set_access(mulawdev, p, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(mulawdev, p, SND_PCM_FORMAT_MU_LAW);
    snd_pcm_hw_params_set_channels(mulawdev, p, 1);
    snd_pcm_hw_params_set_rate_near(mulawdev, p, &rate, 0);
    snd_pcm_hw_params(mulawdev, p);
    snd_pcm_hw_params_get_period_size(p, &mulawfrms, 0);
    *bufsiz = (size_t)mulawfrms;
    return;
}


void mulawclose(void) {
    snd_pcm_drain(mulawdev);
    snd_pcm_close(mulawdev);
}
