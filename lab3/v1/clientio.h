#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>


#define MAXBUFSZM 1000000
#define MAX_FILENAME_SIZE 4096
#define MAX_HOSTNAME_SIZE 256


extern void write_to_file(int fd, const void *buf, size_t len);
