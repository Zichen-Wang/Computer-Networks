all: server client

server: fifoser-v3.o parser.o
	gcc -o server fifoser-v3.o parser.o

client: fifocli-v3.o sender.o handler.o receiver.o
	gcc -o client fifocli-v3.o sender.o handler.o receiver.o

fifoser-v3.o: fifoser-v3.c
	gcc -Wall -c fifoser-v3.c

parser.o: parser.c
	gcc -Wall -c parser.c

fifocli-v3.o: fifocli-v3.c
	gcc -Wall -c fifocli-v3.c

sender.o: sender.c
	gcc -Wall -c sender.c

handler.o: handler.c
	gcc -Wall -c handler.c

receiver.o: receiver.c
	gcc -Wall -c receiver.c

clean:
	rm -f fifoser-v3.o parser.o
	rm -f fifocli-v3.o sender.o handler.o receiver.o
	rm -f server client
