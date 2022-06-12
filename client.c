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

char* extract_directory(char* path);
FILE* create_file(char* path);
void write_in_file(FILE* filepointer, char* buffer);

int main(int argc, char** argv){
    int status;
    int sockfd, n, sendbytes, id, i;
    struct sockaddr_in servaddr;
    char sendline[MAXLINE];
    char recvline[MAXLINE];

    sem_unlink(SEM_REQUEST);
    sem_t* sem_request;
    sem_request = sem_open(SEM_REQUEST, O_CREAT, 0644, 2);

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

        sprintf(sendline, "/home/xv6/Desktop/dirtest\n");
        sendbytes = strlen(sendline);

        if(write(sockfd, sendline, sendbytes) != sendbytes){
            perror("Write error");
            exit(1);
        }

        memset(recvline, 0, MAXLINE);
        while((n = read(sockfd, recvline, MAXLINE-1)) > 0){ //prepei na metraei ka8e file pou erxetai, na to dhmeiourgei kai na grafei ta periexomena tou
            printf("Reading %d bytes\n", n);
            //printf("%s\n", recvline);
            
            char path[PATH_MAX];
            char* path_start;
            char* path_end = recvline;

            char buffer[n];
            char* buffer_start;
            char* buffer_end;

            char* no_o_f = strstr(recvline, "NO: ");
            int number_of_files = atoi(&no_o_f[4]);
            printf("Number of files is %d\n", number_of_files);


            for(int i = 1; i <= number_of_files; i++){
                path_start = strstr(path_end, "FILE: ");
                memset(path, 0, sizeof(path));
                memset(buffer, 0, sizeof(buffer));

                path_start = path_start + 6; //exclude the "FILE: "
                path_end = memchr(path_start, '\n', strlen(path_start)*sizeof(char)); //find where the line ends so that you know you've taken the full path
                memcpy(path, path_start, path_end-path_start); //copy the number of relevant bytes described by the relation [where the line ends] - [where the path begins], begining from where the path begins, into the the variable path
                //printf("The path is: %s\n", path);
                //FILE* fp = create_file(path);
                

                buffer_start = strstr(path_end, "CONTENTS: ");
                buffer_start = buffer_start + 11; //exclude the "CONTENTS: \n"
                if((buffer_end = strstr(buffer_start, "FILE: ")) == NULL){
                    buffer_end = memchr(buffer_start, '\n', strlen(buffer_start)*sizeof(char));
                    if(buffer_end > no_o_f){
                        buffer_end = no_o_f;
                    }
                }
                memcpy(buffer, buffer_start, buffer_end - buffer_start);
                //printf("The contents are: %s\n", buffer);

                FILE* fp = create_file(path);
                write_in_file(fp, buffer);
            }
            close(sockfd);

            // while((path_start = strstr(path_end, "FILE: ")) != NULL){
            //     memset(path, 0, sizeof(path));
            //     memset(buffer, 0, sizeof(buffer));

            //     path_start = path_start + 6; //exclude the "FILE: "
            //     path_end = memchr(path_start, '\n', strlen(path_start)*sizeof(char)); //find where the line ends so that you know you've taken the full path
            //     memcpy(path, path_start, path_end-path_start); //copy the number of relevant bytes described by the relation [where the line ends] - [where the path begins], begining from where the path begins, into the the variable path
            //     printf("The path is: %s\n", path);
            //     //FILE* fp = create_file(path);
                

            //     buffer_start = strstr(path_end, "CONTENTS: ");
            //     buffer_start = buffer_start + 11; //exclude the "CONTENTS: \n"
            //     if((buffer_end = strstr(buffer_start, "FILE: ")) == NULL){
            //         buffer_end = memchr(buffer_start, '\n', strlen(buffer_start)*sizeof(char));
            //     }
            //     memcpy(buffer, buffer_start, buffer_end - buffer_start);
            //     printf("The contents are: %s\n", buffer);

            //     FILE* fp = create_file(path);
            //     write_in_file(fp, buffer);

            // }
        }

        
        // if(n < 0){
        //     perror("Read error");
        //     exit(1);
        // }
        printf("Process %d got a response\n", getpid());
        sem_post(sem_request);
    }
    else{
        wait(&status);
        return 1;
    }
}

char* extract_directory(char* path){ //seperates the directory path from the file, returns the filename and null terminates the path earlier
    char* last = strrchr(path, '/');
    path[last - path] = 0; 
    return last+1;
}

FILE* create_file(char* path){ //it deletes the file if it already exists 
    
    FILE* fp = fopen(path, "r+");
    if(fp != NULL){
        if(unlink(path) == -1){
            perror("Unlink");
        }  
    }
    fp = fopen(path,"w+");
    return fp;
}


void write_in_file(FILE* filepointer, char* buffer){
    fprintf(filepointer,"%s", buffer);
}      