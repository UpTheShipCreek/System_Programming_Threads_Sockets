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

#define BUFFER 8192
#define SA struct sockaddr

FILE* create_file(char* path);

int main(int argc, char** argv){
    int status;
    int sockfd, n, sendbytes, id, i;
    struct sockaddr_in servaddr;
    char send[BUFFER];
    char recv[BUFFER];

    int port;
    char* want_path;

    /*--------------------------Getting Arguments--------------------------*/
    if(argc != 7){
        printf("usage: -i <server_ip> -p <server_port> -d <directory>\n");
        exit(1);
    }
    port = atoi(argv[4]); //saving port in a varible, the others I will use directly

    /*--------------------------Getting Arguments--------------------------*/

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){ //0 is TCP
        perror("Error while creating the socket");
        exit(1);
    }

    //setting up the address
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET; //internet address
    servaddr.sin_port = htons(port);  //host to network short
    //setting up the address

    if(inet_pton(AF_INET, argv[2], &servaddr.sin_addr) <= 0){
        printf("Error getting the IP address\n");
        exit(1);
    }
    if(connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) < 0){
        perror("Connection failed");
        exit(1);
    }

    fflush(stdout);
    sprintf(send, "%s",argv[6]);
    sendbytes = strlen(send);

    if(write(sockfd, send, sendbytes) < sendbytes){
        perror("Write");
        exit(1);
    }

    printf("I are you going to send me something you fuckin bitch?\n");

    memset(recv, 0, BUFFER);
    while((n = read(sockfd, recv, BUFFER-1)) > 0){ //prepei na metraei ka8e file pou erxetai, na to dhmeiourgei kai na grafei ta periexomena tou
        printf("Reading %d bytes\n", n);
        printf("%s\n", recv);
        
        char path[PATH_MAX];
        char* path_start;
        char* path_end = recv;

        char buffer[n];
        char* buffer_start;
        char* buffer_end;

        char* no_o_f = strstr(recv, "NO: ");
        int number_of_files = atoi(&no_o_f[4]);

        printf("Number of files in the directory is %d.\n", number_of_files);

        while((path_start = strstr(path_end, "FILE: ")) != NULL){
            memset(path, 0, sizeof(path));
            memset(buffer, 0, sizeof(buffer));

            path_start = path_start + 6; //exclude the "FILE: "
            path_end = strstr(path_start, "CONTENTS: ");
            memcpy(path, path_start, path_end-path_start); //copy the number of relevant bytes described by the relation [where the line ends] - [where the path begins], begining from where the path begins, into the the variable path
            
            buffer_start = strstr(path_end, "CONTENTS: ");
            buffer_start = buffer_start + 10; //exclude the "CONTENTS: \n"
            if((buffer_end = strstr(buffer_start, "FILE: ")) == NULL){
                if((buffer_end = strstr(buffer_start, "NO: ")) == NULL){
                    buffer_end = buffer_start + sizeof(buffer_start);
                }
            }
            memcpy(buffer, buffer_start, buffer_end - buffer_start);

            FILE* fp = create_file(path);
            printf("Created file with pathname: %s\n", path);
            fprintf(fp,"%s", buffer);
            printf("Copied the contents of the file\n");
        }
        close(sockfd);
    }
    printf("Got a response\n");
    return 1;
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