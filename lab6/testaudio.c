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

#include <stdio.h>
#include <alsa/asoundlib.h>
#include <unistd.h>

static snd_pcm_t *mulawdev;
static snd_pcm_uframes_t mulawfrms;

#define mulawwrite(x) snd_pcm_writei(mulawdev, x, mulawfrms)

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


/* example driver that behaves similarly to aplay */

int main(int argc, char **argv) {
	char *buf;		// audio buffer: holds one block
	size_t bufsiz;		// audio block size (4096 bytes)
	int fd1;		// AU file argv[1]
	int slptime;		// sleep time between read/write in while-loop
				// unit: microseconds
	
	if(argc !=3) {
		fprintf(stderr, "usage: testaudio <audiofile> <sleeptime>\n");
		exit(1);
	}
	slptime = atoi(argv[2]);
	if((fd1 = open(argv[1], O_RDONLY)) == -1) {
		fprintf(stderr, "file %s not found\n", argv[1]);
		exit(1);
	}

	mulawopen(&bufsiz);		// initialize audio codec
	buf = (char *)malloc(bufsiz); 	// audio buffer

	// read from AU file argv[1] and send to audio device
	while (read(fd1, buf, bufsiz) > 0) {
		mulawwrite(buf);
		usleep(slptime);	// pace write to audio codec
					// sleep time around 313 msec seems to
					// provide reasonable pacing;
					// in lab6 use nanosleep()
	}

	mulawclose();
	free(buf);
}
