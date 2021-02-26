#include "client.h"
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
#include <sys/wait.h>
#include <time.h>

#define MAX_PATH_LEN 512
#define MAX_PATH_COUNT 500

char *all_paths[MAX_PATH_COUNT]; //storage for all the paths of txt files found
static int path_counter; //global counter for tracking next location to put a path name
char *txt = ".txt"; //used to compare and look for txt files
time_t t;

int path_partition(char *path)
{//begin path_partition
  char time_buf[24];
  GetTime(time_buf);
  fprintf(stdout,"\n[%s] %s traversal and file partitioning...\n",time_buf,path);
  if (path_counter > MAX_PATH_COUNT)
  {
    return 2; //the all_paths array is full so no more paths can be added
  }
  //printf("in path_partition: start\n");
  struct dirent **nameList; //initiate a local pointer for use with scandir call
	int dircount;//the number of entries returned by scandir call
  dircount = scandir(path, &nameList, NULL, alphasort); //free nameList later
  if (dircount == -1)
  {
    //printf("in path_partition: path error, could not scandir given path\n");
    return 1;
  }
  //printf("in path_partition: dircount is: %d\n", dircount);
  if (dircount == 2) //this means the subfolder was empty
  {
    return 1;
  }

  //traverse nameList for txt files to add to all_paths
  for (int i = 2; i<dircount; i++){
    //printf("in path_partition: d_name is: %s\n", nameList[i]->d_name);
    int namelength = strlen(nameList[i]->d_name);
    if (strlen(nameList[i]->d_name) < 4)//avoid a segfault or out of bounds ptr
    {
      continue; //assumption: it's not a txt file if name is less than 4 chars
    }
    char *file_type = &nameList[i]->d_name[namelength-4];//pulls the last four chars of the name of the file
    //printf("in path_partition: last four of name is: %s\n", file_type);


    if (strncmp(file_type, txt, 4) == 0) // this is a txt file and we need to add the whole path to the paths array
    {
      //printf("adding a text path to all_paths, path_counter is: %d\n", path_counter);
      all_paths[path_counter] = (char*)malloc(MAX_PATH_LEN*sizeof(char));
			strncpy(all_paths[path_counter], path, MAX_PATH_LEN-1);
			char slash[2] = "/";
			strncat(all_paths[path_counter], slash, 2);
			strncat(all_paths[path_counter], nameList[i]->d_name, MAX_PATH_LEN-1);
      path_counter++;//increment so the next one writes to an open array slot

    }
  }

  for (int i = 2; i<dircount; i++)//traverse nameList again but this time for directories for recursive calls
  {
    if (nameList[i]->d_type == DT_DIR)
    {
      char newdir[MAX_PATH_LEN];
			strncpy(newdir, path, MAX_PATH_LEN-1);
			char slash[2] = "/";
			strncat(newdir, slash, 2);
			strncat(newdir, nameList[i]->d_name, MAX_PATH_LEN-1);
      path_partition(newdir);
    }
  }

  for (int i = 0; i < dircount; i++) //freeing malloc nameList
  {
    free(nameList[i]);
  }
  free(nameList);

  return 0;
}//end path_partition

void RemoveNewline(char* str){
  // removes one newline character from a null-terminated string, replacing it
  // with '\0'
  int end = 0;
  while (str[end] != '\0')
    end++;
  for(int i = 0; i < end; i++){
    if (str[i] == '\n'){
      str[i] = '\0';
      break;
    }
  }
}


void GetTime(char time_buf[24]){
  time(&t);
  strncpy(time_buf, ctime(&t), 24);
  time_buf[24] = '\0';
}

int main(int argc, char** argv)
{//begin main
  if (argc > 3)
  {
    printf("too many arguments. use command ./client <path> <number of clients>\n");
    exit(1);
  }
  if (argc < 3)
  {
    printf("too few arguments. use command ./client <path> <number of clients>\n");
    exit(1);
  }

  //printf("in main: root path: %s\n", argv[1]);
  char time_buf[24];
  GetTime(time_buf);
  fprintf(stdout,"\n[%s] Client starts...\n", time_buf);
  int numclients = atoi(argv[2]);
  //printf("in main: numclients %d\n",numclients);
  int path_partition_success;
  path_counter = 0;

  path_partition_success = path_partition(argv[1]);
  //printf("in main: after call to path partition\n");
  if (path_partition_success == 1)
  {
    printf("path error, path not found\n");
    exit(1);
  }

  /*
  for(int i = 0; i<225; i++) //loop to check first 225 contents of all_paths
  {
    printf("in all paths index %d is: %s\n", i, all_paths[i]);
  }
  */

  //make the client input directory and open the appropriately named and amount of client files
  mkdir("ClientInput", 0777);
  chdir("./ClientInput");
  FILE *client_input_files[numclients];
  char client_file_name[numclients][15];
  for (int i = 0; i<numclients; i++)
  {
    sprintf(client_file_name[i], "client%d.txt", i);
    client_input_files[i] = fopen(client_file_name[i], "w");
  }
  //now we write the contents of all_paths to c txt files in a client
  int i = 0;
  while (all_paths[i] != NULL)
  {
    fprintf(client_input_files[i%numclients], "%s\n", all_paths[i]);
    i++;
  }
  for (int i = 0; i<numclients; i++)
  {
    fclose(client_input_files[i]);
  }

  // this loop forks child processes and then instructs the forked processes to
  // communicate with the server via a message queue.
  int process_iter = 0;
  int pid;
  for (process_iter; process_iter < numclients; process_iter++){
    pid = fork();
    if (pid == 0) break;
  }
  if (pid == 0){ // child section
    int msg_type = process_iter + 1;
    pid_t childpid = getpid();
    key_t key = ftok("../..", 0);
    int msqid = msgget(key, 0666 | IPC_CREAT);
    // printf("DEBUGGING: process_iter: %d msqid is %d\n",process_iter, msqid);
    char path_buf[MAX_PATH_LEN + 1];
    struct msg client_msg; // this will store the message to be sent from client
    struct msg msg_rcvd; // this will store the message sent back from server
    FILE* input_file = fopen(client_file_name[process_iter], "r");
    while (1){
      fgets(path_buf, MAX_PATH_LEN, input_file);
      if (feof(input_file)) break;
      RemoveNewline(path_buf);
      client_msg.mtype = msg_type;
      strncpy(client_msg.mtext, path_buf, MAX_PATH_LEN);
      GetTime(time_buf);
      printf("[%s] Sending %s from client process %d\n",
	    time_buf, client_msg.mtext, childpid);
      msgsnd(msqid, (void*) &client_msg, sizeof(client_msg), 0);
      //printf("send finished \n");
      msgrcv(msqid, (void*) &msg_rcvd, sizeof(msg_rcvd), msg_type + numclients, 0);
      if (strncmp(msg_rcvd.mtext, "ACK", 3) != 0){
	fprintf(stderr, "client received unrecognized message. Expected \'ACK\'\n");
      }
      GetTime(time_buf);
      printf("[%s] Client process %d recieved ACK from server for %s\n",
	     time_buf, childpid, client_msg.mtext);
    }
    fclose(input_file);

    GetTime(time_buf);
    strncpy(client_msg.mtext, "END\0", 4);
    printf("[%s] Sending %s from client process %d\n",
	   time_buf, client_msg.mtext, childpid);
    msgsnd(msqid, (void*) &client_msg, sizeof(client_msg), 0);
    msgrcv(msqid, (void*) &msg_rcvd, sizeof(msg_rcvd), msg_type + numclients, 0);
    GetTime(time_buf);
    printf("[%s] Client process %d recieved %s\n", time_buf, childpid, msg_rcvd.mtext);
    chdir("..");
    mkdir("Output", 0777);
    chdir("./Output");
    char out_filename[20];
    sprintf(out_filename, "./Client%d_out.txt", process_iter);
    FILE* out = fopen(out_filename, "w");
    fputs(msg_rcvd.mtext, out);
    fclose(out);
  } // end of 'child' section
  else{
    wait(NULL);
    GetTime(time_buf);
    printf("[%s] Client ends...\n", time_buf);
  }
  //freeing
  for (int i = 0; i<=path_counter; i++)
  {
    free(all_paths[i]);
  }
  return 0;
}//end main
