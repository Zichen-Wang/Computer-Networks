target: supergopher minigopher

supergopher: supergopher.o util.o mysocket.o
	gcc -o supergopher supergopher.o util.o mysocket.o

minigopher: minigopher.o util.o mysocket.o
	gcc -o minigopher minigopher.o util.o mysocket.o

supergopher.o: supergopher.c supergopher.h
	gcc -Wall -c supergopher.c

minigopher.o: minigopher.c minigopher.h
	gcc -Wall -c minigopher.c

util.o: util.c
	gcc -Wall -c util.c

mysocket.o: mysocket.c
	gcc -Wall -c mysocket.c

clean:
	rm -f supergopher.o minigopher.o
	rm -f util.o mysocket.o
	rm -f supergopher minigopher
	rm -f util mysocket
