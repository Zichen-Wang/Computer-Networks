#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>


#define MAXBUFSZM 1000000
#define MAX_FILE_NAME_SIZE 4096


extern void get_pathname(char *file_name, int fd);
extern off_t get_file_size(const char *pathname);
