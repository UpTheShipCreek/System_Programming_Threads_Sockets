#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <limits.h>
#include <pthread.h>
#include <queue>

#define SERVER_PORT 18000
#define BUF_SIZE 4096
#define MAXLINE 4096
#define SOCKETERROR (-1)
#define SERVER_BACKLOG 100
#define THREAD_POOL_SIZE 10

pthread_t thread_pool[THREAD_POOL_SIZE];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition_variable = PTHREAD_COND_INITIALIZER;


typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;

std::queue<int*> client_q;

void* connection_handler(void* p_client_socket);

void* thread_function(void* argc);

int main(int argc, char** argv){
    int server_socket, client_socket, address_size, i;
    SA_IN server_address,client_address;
    char address[MAXLINE+1];
    
    for(i = 0; i < THREAD_POOL_SIZE-1; i++){
        pthread_create(&thread_pool[i], NULL, thread_function, NULL);
    }
    //opening socket
    if((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0){ //0 is TCP
        perror("Error while creating the socket");
        exit(1);
    }
    //opening socket

    //setting up the address
    server_address.sin_family = AF_INET; //internet address
    server_address.sin_addr.s_addr = htonl(INADDR_ANY); //any internet address
    server_address.sin_port = htons(SERVER_PORT);  //host to network short
    //setting up the address

    //listen and bind
    if(bind(server_socket, (SA *) &server_address, sizeof(server_address)) < 0){
        perror("Bind error");
        exit(1);
    }
    if((listen(server_socket, SERVER_BACKLOG)) < 0){
        perror("Listen error");
        exit(1);
    }
    //listen and bind

    while(1){
        printf("Waiting for connection on port %d\n", SERVER_PORT);
        //fflush(stdout);
        address_size = sizeof(SA_IN);
        client_socket = accept(server_socket, (SA*) &client_address, (socklen_t*)&address_size);
        if(client_socket == -1){
            perror("accept");
            exit(1);
        }
        inet_ntop(AF_INET, &client_address, address, MAXLINE);
        printf("Succesfully accepted client %s\n", address);
        //connection_handler(client_socket);
        //pthread_t t;
        int* pclient = (int*)malloc(sizeof(int));
        *pclient = client_socket;
        
        pthread_mutex_lock(&mutex);
        client_q.push(pclient);
        pthread_cond_signal(&condition_variable);
        pthread_mutex_unlock(&mutex);
        printf("Pushed client %s in the queue\n", address);

        //pthread_create(&t, NULL, connection_handler, pclient);
        
    }
    return 0;
}

void* connection_handler(void* p_client_socket){
    int client_socket = (*(int*)p_client_socket);
    free(p_client_socket);

    char buffer[BUF_SIZE];
    size_t bytes_read;
    int msgsize = 0;
    char path[PATH_MAX+1];

    while((bytes_read = read(client_socket, buffer+msgsize, sizeof(buffer) - msgsize-1)) > 0){
        msgsize+=bytes_read;
        if(msgsize > BUF_SIZE-1 || buffer[msgsize-1] == '\n') break;
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
    FILE* fp = fopen(path, "r");
    if(fp == NULL){
        perror("File open");
        close(client_socket);
        return NULL;
    }

    while((bytes_read = fread(buffer, 1, BUF_SIZE, fp)) > 0){
        printf("sending %ld bytes\n", bytes_read);
        write(client_socket, buffer, bytes_read);
    }
    close(client_socket);
    fclose(fp);
    printf("closing connection\n");

    return NULL;
}

void* thread_function(void* argc){
    while(1){
        int* pclient;

        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&condition_variable, &mutex);
        pclient = client_q.front(); 
        client_q.pop();
        pthread_mutex_unlock(&mutex);
        
        if(pclient != NULL){
            connection_handler(pclient);
        }
    }
}