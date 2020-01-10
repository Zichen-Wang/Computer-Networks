target: remote-command-srv remote-command-cli

remote-command-srv: remote-command-srv.o serverutil.o mysocket.o
	gcc -o remote-command-srv remote-command-srv.o serverutil.o mysocket.o

remote-command-cli: remote-command-cli.o clientutil.o mysocket.o
	gcc -o remote-command-cli remote-command-cli.o clientutil.o mysocket.o

remote-command-srv.o: remote-command-srv.c server.h
	gcc -Wall -c remote-command-srv.c

remote-command-cli.o: remote-command-cli.c client.h
	gcc -Wall -c remote-command-cli.c

serverutil.o: serverutil.c server.h
	gcc -Wall -c serverutil.c

clientutil.o: clientutil.c client.h
	gcc -Wall -c clientutil.c

mysocket.o: mysocket.c
	gcc -Wall -c mysocket.c

clean:
	rm -f remote-command-srv.o remote-command-cli.o
	rm -f serverutil.o clientutil.o mysocket.o
	rm -f remote-command-srv remote-command-cli
	rm -f serverutil clientutil mysocket
