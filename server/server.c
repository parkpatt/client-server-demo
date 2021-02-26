#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <ctype.h> // included for the use of function 'toupper()'
#include <pthread.h>
#include <sys/wait.h>
#include <time.h>
#include "server.h"
#include "../client/client.h"

#define MAX_LINE_LEN 50 // ASSUMPTION: from assignment pdf
#define MAX_RESULTS_LEN (26 * 5)
// ASSUMPTION: number of digits for a char's count will be at most 4

int char_counts[26];
time_t t;

typedef struct thread_args {
  int thread_id;
  int numclients;
  int msqid;
  pthread_mutex_t* mutex;
} thread_args_t ;

pthread_mutex_t* master_mutex;

void GetTime(char time_buf[24]){
  time(&t);
  strncpy(time_buf, ctime(&t), 24);
  time_buf[24] = '\0';
}

void CountFileChars(void *arg){
  // This function is used by threads to communicate with clients and count the
  // number of occurrences of each alphabet character in the file received as
  // a message from the client.
  struct thread_args* args = (struct thread_args*) arg;
  struct msg server_msg;
  struct msg msg_rcvd;
  char time_buf[24];
  GetTime(time_buf);
  printf("[%s] waiting to receive from client process %d\n",
	 time_buf, args->thread_id-1);
  while (1){
    msgrcv(args->msqid, (void*) &msg_rcvd, sizeof(msg_rcvd), args->thread_id, 0);
    GetTime(time_buf);
    printf("[%s] thread %d received %s from client process %d\n",
	   time_buf, args->thread_id - 1, msg_rcvd.mtext, args->thread_id-1);
    if (strncmp(msg_rcvd.mtext, "END", 3) == 0){
      break;
    }
    FILE* current_file = fopen(msg_rcvd.mtext, "r");
    if(current_file == NULL){
	GetTime(time_buf);
	printf("[%s] path error for: %s\n", time_buf, msg_rcvd.mtext);
    }
    char line_buf[MAX_LINE_LEN + 1];
    char current_char;
    while (1){
      fgets(line_buf, MAX_LINE_LEN, current_file);
      if (feof(current_file)) break;
      //printf("DEBUGGING: line_buf holds %s\n", line_buf);
      current_char = toupper(line_buf[0]);
      pthread_mutex_lock(args->mutex);
      char_counts[current_char - 65]++; // 65 is ASCII value for 'A'
      pthread_mutex_unlock(args->mutex);
    } // end of file-reading loop
    fclose(current_file);
    
    strncpy(server_msg.mtext, "ACK\0", 4);
    server_msg.mtype = args->thread_id + args->numclients;
    GetTime(time_buf);
    printf("[%s] thread %d sending ACK to client %d for %s\n",
	   time_buf, args->thread_id-1, args->thread_id-1,msg_rcvd.mtext);
    msgsnd(args->msqid, (void*) &server_msg, sizeof(server_msg), 0);
  }//end of outer while loop
  
  GetTime(time_buf);
  printf("[%s] server %d received END\n", time_buf, args->thread_id-1);
}

void SendResultString(void* arg){
  // This function is used by server threads to send the final results to clients.
  char time_buf[24];
  struct thread_args* args = (struct thread_args*) arg;
  struct msg server_msg;
  struct msg msg_rcvd;
  char results_buf[MAX_RESULTS_LEN] = {0};
  char count_buf[5];
  for (int i = 0; i < 26; i++){
    sprintf(count_buf, "%d", char_counts[i]);
    strncat(results_buf, count_buf, 5);
    if (i != 25)
      strncat(results_buf, "#", 1);
    else
      strncat(results_buf, "\0", 1);
  }
  strncpy(server_msg.mtext, results_buf, sizeof(results_buf) / sizeof(char));
  server_msg.mtype = args->thread_id + args->numclients;
  msgsnd(args->msqid, (void*) &server_msg, sizeof(server_msg), 0);
  GetTime(time_buf);
  printf("[%s] thread %d sent %s to client\n", time_buf, args->thread_id - 1, server_msg.mtext);
}

int main(int argc, char* argv[]){
  if (argc != 2){
    fprintf(stderr, "server requires 1 arg: number of clients\n");
    return 1;
  }
  char time_buf[24];
  GetTime(time_buf);
  printf("[%s] server started\n", time_buf);
  key_t key = ftok("..", 0);
  int msqid = msgget(key, 0666 | IPC_CREAT);
  //printf("DEBUGGING: msqid is %d\n", msqid);
  int numclients = atoi(argv[1]);

  pthread_t thread_pool[numclients];
  master_mutex = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t)); //only one mutex on the heap
  thread_args_t args_array[numclients];
  for (int i = 0; i < numclients; i++){
    args_array[i].thread_id = i + 1;
    args_array[i].numclients = numclients;
    args_array[i].msqid = msqid;
    args_array[i].mutex = master_mutex; //all threads share the master mutex
    pthread_create(&thread_pool[i], NULL, (void*)&CountFileChars, (void*) &args_array[i]);
  }
  for (int i = 0; i < numclients; i++){
    pthread_join(thread_pool[i], NULL);
  }
  free(master_mutex);
  for (int i = 0; i<numclients; i++)
  {
    pthread_create(&thread_pool[i], NULL, (void*)&SendResultString, (void*) &args_array[i]);
  }
  for (int i = 0; i < numclients; i++){
    pthread_join(thread_pool[i], NULL);
  }
  msgctl(msqid, IPC_RMID, NULL);
  GetTime(time_buf);
  printf("[%s] server ends\n", time_buf);
  return 0;
}
