#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sched.h>

#define LOG_FILE "log_file"

#define NUM_CORES 8

#define PERM (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)
#define OPEN_MODE (O_RDWR | O_CREAT) 

//| O_TRUNC
#define MAX_SH_FILE_SIZE (sizeof(rpc_signal_wait_t) + sizeof(rpc_msg_t))
#define MAX_LOG_FILE_SIZE 8*256*1024

#define UNREACHED 0

#define MAX_TICKETS 32

#define WORD_LENGTH 4 //4 bytes
#define CACHE_LINE 4*WORD_LENGTH; //4 such words

//aligned buff
#define MAX_BUFF_SIZE 84 //5*CACHE_LINE + WORD_LENGTH

#define ABS(val) ((val) < 0 ? (val) * (-1) : (val))

#define BINDER 1
#define OTHER 99

//// START FOR PERF TIMING



