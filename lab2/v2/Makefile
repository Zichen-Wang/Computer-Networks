target: remote-command-srv remote-command-cli

remote-command-srv: remote-command-srv.o mysocket.o
	gcc -o remote-command-srv mysocket.o remote-command-srv.o

remote-command-cli: remote-command-cli.o mysocket.o
	gcc -o remote-command-cli mysocket.o remote-command-cli.o

remote-command-srv.o: remote-command-srv.c
	gcc -Wall -c remote-command-srv.c

remote-command-cli.o: remote-command-cli.c
	gcc -Wall -c remote-command-cli.c

mysocket.o: mysocket.c mysocket.h
	gcc -Wall -c mysocket.c

clean:
	rm -f remote-command-srv.o remote-command-cli.o mysocket.o
	rm -f remote-command-srv remote-command-cli mysocket
