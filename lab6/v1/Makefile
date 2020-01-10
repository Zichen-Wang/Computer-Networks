target: streamerd playaudio

streamerd: streamerd.o logger.o
	gcc -o streamerd streamerd.o logger.o -lm

playaudio: playaudio.o queue.o mulaw.o logger.o
	gcc -o playaudio playaudio.o queue.o mulaw.o logger.o -pthread -lasound

streamerd.o: streamerd.c streamerd.h
	gcc -Wall -c streamerd.c

playaudio.o: playaudio.c playaudio.h
	gcc -Wall -c playaudio.c

queue.o: queue.c queue.h
	gcc -Wall -c queue.c

mulaw.o: mulaw.c mulaw.h
	gcc -Wall -c mulaw.c

logger.o: logger.c logger.h
	gcc -Wall -c logger.c

clean:
	rm -f streamerd playaudio
	rm -f streamerd.o playaudio.o
