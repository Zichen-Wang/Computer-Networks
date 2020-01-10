#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>


extern void parse(char *argv[], char buf[]);


int main(void) {
    pid_t k;

    int server_fifo_fd, client_fifo_fd;
    const char *server_fifo_name = "server_queue";
    const char *client_fifo_name = "client_queue";

    char buf[100], recv[100];
    
    int i, len, end_flag;
    ssize_t read_status;
    
    char *argv[100];
    
    // set a random seed
    srand(time(NULL));

    // create a server FIFO
    mkfifo(server_fifo_name, 0600);
     
    while (1) {
        // open the FIFO open with read only permission
        // block until a client opens it with write only permission
        server_fifo_fd = open(server_fifo_name, O_RDONLY);

        if (server_fifo_fd == -1) {
            // if failed, terminate
            printf("cannot open the server FIFO; terminating...");
            exit(1);
        }
        
        // block until reading bytes from the FIFO
        len = 0;
        end_flag = 0;
        while ((read_status = read(server_fifo_fd, (void *) buf, 100)) > 0) {
            for (i = 0; i < read_status; i++) {
                recv[len++] = buf[i];
                if (buf[i] == '\0') {
                    end_flag = 1;
                    break;
                }
            }
            if (end_flag == 1)
                break;
        }

        // close the server FIFO file descriptor
        close(server_fifo_fd);

        if (read_status == -1) {
            perror("read()");
            fflush(stderr);
            continue;
        }
        
        if (len == 0) {
            continue;
        }

        recv[len - 1] = '\0';

        if (rand() % 2 == 0) {
            // ignore the request
            printf("ignored the client's request \"%s\"\n", recv);
            continue;
        }

        parse(argv, recv);
        

        k = fork();

        if (k == 0) {
            // child code
            
            // open the FIFO open with write only permission
            // if it exists, block until the client opens it with read only permission
            client_fifo_fd = open(client_fifo_name, O_WRONLY);

            if (client_fifo_fd == -1) {
                // if the client FIFO does not exists, terminate
                fprintf(stderr, "cannot open the client FIFO\n");
                exit(1);
            }

            // redirect stdout to the client FIFO
            dup2(client_fifo_fd, 1);
            
            // close one of the client FIFO file descriptors
            close(client_fifo_fd);

            if (execvp((const char *)argv[0], argv) == -1) {
                // if execution failed, terminate child
                exit(1);
            }
        } else {
            // parent code
            // do nothing and returns to the beginning of the while-loop
        } 
    }

    return 0;
}
