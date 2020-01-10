target: myftpd myftp

myftpd: myftpd.o mysocket.o server.o
	gcc -o myftpd mysocket.o server.o myftpd.o

myftp: myftp.o mysocket.o
	gcc -o myftp mysocket.o myftp.o

myftpd.o: myftpd.c mysocket.h myftpd.h
	gcc -Wall -c myftpd.c

myftp.o: myftp.c mysocket.h myftp.h
	gcc -Wall -c myftp.c

server.o: server.c mysocket.h myftpd.h
	gcc -Wall -c server.c

mysocket.o: mysocket.c mysocket.h
	gcc -Wall -c mysocket.c

clean:
	rm -f myftpd.o myftp.o mysocket.o server.o
	rm -f myftpd myftp mysocket server