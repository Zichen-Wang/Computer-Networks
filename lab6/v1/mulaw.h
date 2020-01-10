#ifndef MULAW_H
#define MULAW_H

#include <stdio.h>
#include <alsa/asoundlib.h>
#include <unistd.h>


extern snd_pcm_t *mulawdev;
extern snd_pcm_uframes_t mulawfrms;


#define mulawwrite(x) snd_pcm_writei(mulawdev, x, mulawfrms)


void mulawopen(size_t *);

void mulawclose(void);


#endif //MULAW_H
