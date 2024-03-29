#include "dataServer.h"

int main(int argc, char** argv){
    short port;
    int thread_pool_size, /*queue_size, /*block_size,*/ i;

    extern int* block_size, *queue_size;

    int enable;

    int server_socket, client_socket, address_len;
    struct sockaddr_in server_address, client_address;

    char s_address[MAXLINE+1];
    
    block_size = (int*)malloc(sizeof(int));
    queue_size = (int*)malloc(sizeof(int));

    /*--------------------------Getting Arguments--------------------------*/
    if(argc != 9){
        printf("usage: ./dataServer -p <port> -s <thread_pool_size> -q <queue_size> -b <block_size>\n");
        exit(1);
    }

    port = atoi(argv[2]);  //number of creatures for our society
    thread_pool_size = atoi(argv[4]);  //number of actions
    (*queue_size) = atoi(argv[6]);
    (*block_size) = atoi(argv[8]);
    printf("Server's parameters are\nport: %d\nthread_pool_size: %d\nqueue_size: %d\nblock_size %d\n", port, thread_pool_size, (*queue_size), (*block_size));

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
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0){
        perror("setsockopt(SO_REUSEADDR) failed");
    }
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int)) < 0){
        perror("setsockopt(SO_REUSEPORT) failed");
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
        printf("Accepted connection from  %s\n\n", s_address);
        /*Converting the address into a string just so we can look at it and print it*/

        printf("Creating a thread for it \n");
        /*Create a thread for the client*/
        pthread_t c_thread;
        Args* args = (Args*)malloc(sizeof(Args));
        args->CS = client_socket;
        args->BLOCK = (*block_size); //no longer need that since it's a global variable
        pthread_create(&c_thread, NULL, communicator_thread_function, args);
        /*Create a thread for the client*/
        printf("Okay I created the thread \n");

    }

    /*--------------------------Waiting and Accepting Connections--------------------------*/
   //free(block_size);
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
    printf("I am the thread, lets see what we got\n");
    char no_o_files;
    char expect_message[*block_size];
    int client_socket = ((Args*)args)->CS;
    int size = ((Args*)args)->BLOCK;
    free(args);

    char buffer[size];
    size_t bytes_read;
    char path[PATH_MAX+1];

    printf("Reading client's request...\n");
    bytes_read = read(client_socket, buffer, sizeof(buffer));
    
    buffer[bytes_read] = 0; //null terminate the message

    printf("Request: %s\n", buffer);
    fflush(stdout);

    if(realpath(buffer, path) == NULL){
        perror("Bad path");
        close(client_socket);
        return NULL;
    }
    printf("Client wants everything in this dierectory: %s\n", path);

    if(exe_queue.size() == (*queue_size)){ //if the queue is full 
        pthread_mutex_lock(&c_thread_lock);
        pthread_cond_wait(&c_thread_cv, &c_thread_lock); //wait for the signal that something was removed from it
        pthread_mutex_unlock(&c_thread_lock); 
    }
    no_o_files = '0' + queue_files(path, client_socket); //queue the requests

    /* WEIRD LETS SEE */
    // memset(expect_message, 0, sizeof(expect_message));
    // sprintf(expect_message, "NO: %c", no_o_files);
    // expect_message[strcspn(expect_message, "\0")] = ' ';
    // sprintf(expect_message, "NO: %c\n", no_o_files);
    // write(client_socket, expect_message, sizeof(expect_message));
    // printf("Expect message %s", expect_message);
    write(client_socket, "NO: ", 4);
    write(client_socket, &no_o_files, 1);
    //write(client_socket, "\n", 1);
    //printf("%s\n", expect_message);
    /* WEIRD LETS SEE */
    
    //write(client_socket, &no_o_files, sizeof(char)); // Send how many files it should expect

    //if(exe_queue.empty()) close(client_socket); // that has to suffice for now

    return NULL;
}

int queue_files(char* path, int client){ //pushes the files that it finds under the intial and all the subsequent foldiers, into the queue
    int file_count = 0;
    DIR* d = opendir(path); // open the path
    if(d == NULL){
        perror("Dir open");
        return -1;
    }
    Request* req = (Request*)malloc(sizeof(Request));
    req->Client = client;
    struct dirent* dir; // for the directory entries
    while ((dir = readdir(d)) != NULL){ // if we were able to read somehting from the directory
        if(dir->d_type != DT_DIR){
            file_count++;
            sprintf(req->Path, "%s/%s", path, dir->d_name);
            pthread_mutex_lock(&mutex);
            exe_queue.push(req);
            pthread_cond_signal(&condition_variable);
            pthread_mutex_unlock(&mutex);
            //printf("Added <%s/%s> to the queue\n", path, dir->d_name);
        }
        else if(dir -> d_type == DT_DIR && strcmp(dir->d_name,".")!=0 && strcmp(dir->d_name,"..")!=0 ){ // if it is a directory
            //printf("%s\n", dir->d_name); 
            char d_path[PATH_MAX]; 
            sprintf(d_path, "%s/%s", path, dir->d_name);
            file_count += queue_files(d_path, client); // recall with the new path
        }
    }
    closedir(d); // finally close the directory
    return file_count;
}

void* worker_thread_function(void* args){
    Request* req;
    while(1){
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&condition_variable, &mutex);
        req = exe_queue.front(); 
        exe_queue.pop();
        pthread_mutex_unlock(&mutex);

        pthread_mutex_lock(&c_thread_lock);
        pthread_cond_signal(&c_thread_cv); //signal that the queue is not full anymore so that the communicator can push a request in it
        pthread_mutex_unlock(&c_thread_lock); 

        if(req != NULL){
            printf("<%d> Recieved task <%s, %d>\n", gettid(), req->Path, req->Client);
            handle_request(req);
            //free(req); //yeah this free is bugged, so the requests don't get freed
        }
    }
}

void* handle_request(Request* request){
    char expect_file[*block_size]; //full replay message message
    char buffer[(*block_size)];
    size_t bytes_read;

    FILE* fp = fopen(request->Path, "r");
    if(fp == NULL){
        perror("File open");
        close(request->Client);
        return NULL;
    }
    //printf("About to read file %s\n", request->Path);

    write(request->Client, "FILE: ", 6);
    write(request->Client, request->Path, strlen(request->Path)*sizeof(char));


    /**/
    write(request->Client, "CONTENTS: ", 10);
    while((bytes_read = fread(buffer, 1, (*block_size)-1, fp)) > 0){
        printf("<%d> Sending %ld bytes\n", gettid(), bytes_read);
        write(request->Client, buffer, bytes_read);
        memset(buffer, 0, (*block_size));
    }
    fclose(fp);
    return NULL;
}
