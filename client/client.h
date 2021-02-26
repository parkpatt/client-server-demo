#ifndef CLIENT_H
#define CLIENT_H
#define MAX_PATH_LEN 512
#define MAX_MSG_LEN 2048

typedef struct msg {
  long mtype;
  char mtext[2048];
} msg_t;

int path_partition(char* path);
void RemoveNewline(char* str);
void GetTime(char time_buf[24]);

#endif
