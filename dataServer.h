#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> //sockets
#include <sys/types.h> //sockets
#include <netinet/in.h> //struct sockaddr_in
#include <arpa/inet.h> //hton
#include <pthread.h> //thread functions
#include <limits.h> //PATH_MAX
#include <queue> //queues
#include <string.h> //string functions
#include <unistd.h> //close
#include <dirent.h> //searching the directory for files

#define SERVER_BACKLOG 100
#define MAXLINE 4096

typedef struct request{
    int Client;
    char Path[PATH_MAX];
}Request;

typedef struct com_thread_args {
    int CS;
    int BLOCK;
}Args;

int bind_on_port (int sock, short port);
void* communicator_thread_function(void* args);
void* worker_thread_function(void* args);
int queue_files(char* path, int client); //pushes the full paths of the files of the client's request into the execution queue and returns the number of them
void* handle_request(Request* request);
int count_files_in_req(char* path);
std::queue<Request*> exe_queue;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition_variable = PTHREAD_COND_INITIALIZER;

pthread_mutex_t c_thread_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t c_thread_cv = PTHREAD_COND_INITIALIZER;

int* block_size, *queue_size;