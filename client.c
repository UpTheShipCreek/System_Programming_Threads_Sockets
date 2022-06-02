#include <sys/socket.h>
#include <sys/types.h>
#include <wait.h>
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
#include <semaphore.h>

#define SERVER_PORT 1800
#define REQUESTS 5
#define MAXLINE 4096
#define SA struct sockaddr
#define SEM_REQUEST "Request"

int main(int argc, char** argv){
    int status;
    int sockfd, n, sendbytes, id, i;
    struct sockaddr_in servaddr;
    char sendline[MAXLINE];
    char recvline[MAXLINE];

    sem_unlink(SEM_REQUEST);
    sem_t* sem_request;
    sem_request = sem_open(SEM_REQUEST, O_CREAT, 0644, 1);

    if(argc != 2){
        printf("usage: <server address>\n");
        exit(1);
    }


    for(i = 0; i < REQUESTS; i++){
        id = fork();
        if(id == 0) break;
    }

    if(id == 0){
        sem_wait(sem_request);

        if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){ //0 is TCP
            perror("Error while creating the socket");
            exit(1);
        }

        //setting up the address
        bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family = AF_INET; //internet address
        servaddr.sin_port = htons(SERVER_PORT);  //host to network short
        //setting up the address

        if(inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0){
            printf("Error getting the IP address\n");
            exit(1);
        }
        if(connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) < 0){
            perror("Connection failed");
            exit(1);
        }

        sprintf(sendline, "/home/xv6/Desktop/testing/dirtest\n");
        sendbytes = strlen(sendline);

        if(write(sockfd, sendline, sendbytes) != sendbytes){
            perror("Write error");
            exit(1);
        }

        while((n = read(sockfd, recvline, MAXLINE -1)) > 0){
            //printf("%s", recvline);
            memset(recvline, 0, MAXLINE);
        }
        if(n < 0){
            perror("Read error");
            exit(1);
        }
        printf("Process %d got a response\n", getpid());
        sem_post(sem_request);
    }
    else{
        wait(&status);
        return 1;
    }
}