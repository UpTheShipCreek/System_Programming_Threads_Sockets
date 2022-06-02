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

int bind_on_port (int sock, short port);
void* communicator_thread_function(void* args);
void* worker_thread_function(void* args);
void queue_files(char* path);

typedef struct com_thread_args {
    int CS;
    int BLOCK;
}Args;

std::queue<char*> exe_queue;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition_variable = PTHREAD_COND_INITIALIZER;

int main(int argc, char** argv){
    short port;
    int thread_pool_size, queue_size, block_size, i;

    int server_socket, client_socket, address_len;
    struct sockaddr_in server_address, client_address;

    char s_address[MAXLINE+1];

    /*--------------------------Getting Arguments--------------------------*/
    if(argc != 9){
        printf("usage: ./dataServer -p <port> -s <thread_pool_size> -q <queue_size> -b <block_size>\n");
        exit(1);
    }

    port = atoi(argv[2]);  //number of creatures for our society
    thread_pool_size = atoi(argv[4]);  //number of actions
    queue_size = atoi(argv[6]);
    block_size = atoi(argv[8]);
    printf("Server's parameters are\nport: %d\nthread_pool_size: %d\nqueue_size: %d\nblock_size %d\n", port, thread_pool_size, queue_size, block_size);

    /*--------------------------Getting Arguments--------------------------*/

    /*--------------------------Creating Thread Pool--------------------------*/
    pthread_t thread_pool[thread_pool_size];
    for(i = 0; i < thread_pool_size; i++){
        pthread_create(&thread_pool[i], NULL, worker_thread_function, NULL);
    }
    /*--------------------------Creating Thread Pool--------------------------*/

    /*--------------------------Open Bind and Listen--------------------------*/
    if((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0){ //0 is TCP
        perror("socket create");
        exit(1);
    }
    if(bind_on_port(server_socket, port) < 0){
        perror("bind");
        exit(1);
    }
    if((listen(server_socket, SERVER_BACKLOG)) < 0){
        perror("listen");
        exit(1);
    }

    printf("Server was successfully initialized...\n");
    /*--------------------------Open Bind and Listen--------------------------*/

    /*--------------------------Waiting and Accepting Connections--------------------------*/
    while(1){

        /*Feedback that the server is indeed waiting for a connection*/
        printf("Listening for connections to port %d\n", port);
        fflush(stdout);
        /*Feedback that the server is indeed waiting for a connection*/

        /*Accepting the Connection and saving the client socket*/
        address_len = sizeof(struct sockaddr_in);
        client_socket = accept(server_socket, (struct sockaddr*) &client_address, (socklen_t*)&address_len);
        if(client_socket == -1){
            perror("accept");
            exit(1);
        }
        /*Accepting the Connection and saving the client socket*/

        /*Converting the address into a string just so we can look at it and print it*/
        inet_ntop(AF_INET, &client_address, s_address, MAXLINE); 
        printf("Accepted connection from  %s\n", s_address);
        /*Converting the address into a string just so we can look at it and print it*/

        /*Create a thread for the client*/
        pthread_t c_thread;
        Args* args = (Args*)malloc(sizeof(Args));
        args->CS = client_socket;
        args->BLOCK = block_size;
        pthread_create(&c_thread, NULL, communicator_thread_function, args);
        /*Create a thread for the client*/

    }

    /*--------------------------Waiting and Accepting Connections--------------------------*/
    return 1;
}

int bind_on_port (int sock, short port){ //code from the class pdfs 
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl (INADDR_ANY);
    server.sin_port = htons(port);
    return bind(sock, (struct sockaddr *) &server, sizeof(server));
}

void* communicator_thread_function(void* args){
    int client_socket = ((Args*)args)->CS;
    int size = ((Args*)args)->BLOCK;
    free(args);

    char buffer[size];
    size_t bytes_read;
    int msgsize = 0;
    char path[PATH_MAX+1];

    while((bytes_read = read(client_socket, buffer+msgsize, sizeof(buffer) - msgsize-1)) > 0){
        msgsize+=bytes_read;
        if(msgsize > size-1 || buffer[msgsize-1] == '\n') break;
    }

    // check(bytes_read, "recv error");
    buffer[msgsize-1] = 0; //null terminate the message

    printf("Request: %s\n", buffer);
    fflush(stdout);

    if(realpath(buffer, path) == NULL){
        perror("Bad path");
        close(client_socket);
        return NULL;
    }

    queue_files(path);

    close(client_socket);
    printf("closing connection\n");

    return NULL;
}

void queue_files(char* path){ //pushes the files that it finds under the intial and all the subsequent foldiers in to the queue
    DIR* d = opendir(path); // open the path
    if(d == NULL){
        perror("Dir open");
        return;
    }
    struct dirent * dir; // for the directory entries
    while ((dir = readdir(d)) != NULL){ // if we were able to read somehting from the directory
        if(dir->d_type != DT_DIR){
            char* file = (char*)malloc(sizeof(char[PATH_MAX]));
            sprintf(file, "%s/%s", path, dir->d_name);

            pthread_mutex_lock(&mutex);
            exe_queue.push(file);
            pthread_cond_signal(&condition_variable);
            pthread_mutex_unlock(&mutex);
        }
        else if(dir -> d_type == DT_DIR && strcmp(dir->d_name,".")!=0 && strcmp(dir->d_name,"..")!=0 ){ // if it is a directory
            //printf("%s\n", dir->d_name); 
            char d_path[PATH_MAX]; // here I am using sprintf which is safer than strcat
            sprintf(d_path, "%s/%s", path, dir->d_name);
            queue_files(d_path); // recall with the new path
        }
    }
    closedir(d); // finally close the directory
}

void* worker_thread_function(void* args){
    char* file;
    while(1){
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&condition_variable, &mutex);
        file = exe_queue.front(); 
        exe_queue.pop();
        pthread_mutex_unlock(&mutex);
        
        if(file != NULL){
            printf("%s\n", file);
            free(file);
        }
    }
    
}