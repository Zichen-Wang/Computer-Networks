terve: terve.o handler.o util.o mysocket.o
	gcc -o terve terve.o handler.o util.o mysocket.o

terve.o: terve.c terve.h
	gcc -Wall -c terve.c

handler.o: handler.c terve.h
	gcc -Wall -c handler.c

util.o: util.c terve.h
	gcc -Wall -c util.c

mysocket.o: mysocket.c
	gcc -Wall -c mysocket.c

clean:
	rm -f terve terve.o handler.o util.o mysocket.o
