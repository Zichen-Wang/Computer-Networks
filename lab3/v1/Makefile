target: myfetchfiled myfetchfile

myfetchfiled: myfetchfiled.o mysocket.o serverio.o
	gcc -o myfetchfiled mysocket.o serverio.o myfetchfiled.o

myfetchfile: myfetchfile.o mysocket.o clientio.o
	gcc -o myfetchfile mysocket.o clientio.o myfetchfile.o

myfetchfiled.o: myfetchfiled.c mysocket.h serverio.h
	gcc -Wall -c myfetchfiled.c

myfetchfile.o: myfetchfile.c mysocket.h clientio.h
	gcc -Wall -c myfetchfile.c

mysocket.o: mysocket.c mysocket.h
	gcc -Wall -c mysocket.c

serverio.o: serverio.c serverio.h
	gcc -Wall -c serverio.c

clientio.o: clientio.c clientio.h
	gcc -Wall -c clientio.c

clean:
	rm -f myfetchfiled.o myfetchfile.o mysocket.o serverio.o clientio.o
	rm -f myfetchfiled myfetchfile mysocket serverio clientio
