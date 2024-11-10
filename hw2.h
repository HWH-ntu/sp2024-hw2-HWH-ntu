#define READ_FROM_PARENT_FD 3
#define WRITE_TO_PARENT_FD 4
#define TEMP_HIGHER_FD 100 //for meet fd assignment
#define MAX_CHILDREN 8
#define MAX_QUEUE_SIZE 9 // MAX_CHILDREN +1
#define MAX_FIFO_NAME_LEN 9
#define MAX_FRIEND_INFO_LEN 12
#define MAX_FRIEND_NAME_LEN 9
#define MAX_CMD_LEN 128
#include <sys/types.h>
typedef struct { // parent(current!) process下的struct
    pid_t pid; // signed int
    int read_fd; // for this friend(child) node to read
    int write_fd; // for this friend(child) node to write
    char info[MAX_FRIEND_INFO_LEN];
    char name[MAX_FRIEND_NAME_LEN];
    int value;
} friend; // child

