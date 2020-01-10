target: overlaygopher createoverlay

overlaygopher: overlaygopher.o util.o mysocket.o
	gcc -o overlaygopher overlaygopher.o util.o mysocket.o

createoverlay: createoverlay.o util.o mysocket.o
	gcc -o createoverlay createoverlay.o util.o mysocket.o

overlaygopher.o: overlaygopher.c overlaygopher.h
	gcc -Wall -c overlaygopher.c

createoverlay.o: createoverlay.c createoverlay.h
	gcc -Wall -c createoverlay.c

util.o: util.c
	gcc -Wall -c util.c

mysocket.o: mysocket.c
	gcc -Wall -c mysocket.c

clean:
	rm -f overlaygopher.o createoverlay.o
	rm -f util.o mysocket.o
	rm -f overlaygopher createoverlay
	rm -f util mysocket
