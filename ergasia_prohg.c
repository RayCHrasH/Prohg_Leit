#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <errno.h>
#include <semaphore.h>
#define _XOPEN_SOURCE 700


#define MESSAGE_SIZE 100


//0 reading
//1 writting
int interrupt = 0;

void handle_sigint(int signo){
    interrupt = 1;
}

typedef struct{
    char name[64];
    int child_pipe_fd[2];   //C->P
    int parent_pipe_fd[2];  //P->C
    char *filename;
    int id;
} Child;


void childFunc(Child *child){
    signal(SIGINT, SIG_IGN);
    close(child->parent_pipe_fd[1]);
    close(child->child_pipe_fd[0]);

    char message[sizeof("done")];
    char work_message[MESSAGE_SIZE];
    FILE *file = fopen(child->filename, "a");

    if (!file) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    child->id = getpid();
    fprintf(file, "[%s] ---> <%d>\n", child->name, child->id);
    fflush(file);
    fclose(file);
    write(child->child_pipe_fd[1],"done",sizeof("done"));
    read(child->parent_pipe_fd[0],work_message,sizeof(work_message));
    printf("%s\n",work_message);    
    exit(EXIT_SUCCESS);
}    

void parentFunc(int numChildren,Child *children){

    fd_set read_fds;
    int max_fd = -1;

    for (int i = 0; i < numChildren; i++){
        close(children[i].parent_pipe_fd[0]);
        close(children[i].child_pipe_fd[1]);
    }

    while(!interrupt){

        FD_ZERO(&read_fds);

        for(int i = 0; i < numChildren; i++){
            FD_SET(children[i].child_pipe_fd[0], &read_fds);
            if (children[i].child_pipe_fd[0] > max_fd) {
                max_fd = children[i].child_pipe_fd[0];
            }
        }

        if(select(max_fd + 1, &read_fds, NULL,NULL,NULL) < 0){
            if(errno == EINTR){
                break;
            }else{
                perror("select");
                exit(EXIT_FAILURE);
            }
        }        

        for (int i = 0; i < numChildren;i++){
            if(FD_ISSET(children[i].child_pipe_fd[0],&read_fds)){
                char message[sizeof("done")];
                char work_message[MESSAGE_SIZE];
                ssize_t result = read(children[i].child_pipe_fd[0], message, sizeof(message));

                if (result > 0) {
                    printf("received message %s from child %d\n", message, i);
                    write(children[i].parent_pipe_fd[1],"I have work for you to do",sizeof("I have work for you to do"));
                } else if (result == 0) {
                    
                } else {
                    perror("read");
                    exit(EXIT_FAILURE);
                }
                
            }

        }
        
    }


     for (int i = 0; i < numChildren; i++) {
        close(children[i].child_pipe_fd[0]);
        kill(children[i].id, SIGTERM);
        waitpid(children[i].id, NULL, 0);
    }

    free(children);

    printf("Parent exiting\n");
    exit(EXIT_SUCCESS);
  
}

int main(int argc, char *argv[]){
    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigaction(SIGINT, &sa,NULL);

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <filename> <number_of_children>\n", argv[0]);
        exit(EXIT_FAILURE);
    }


    char *filename = argv[1];
    int numChildren = atoi(argv[2]);

    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    Child *children = (Child *)malloc(numChildren * sizeof(Child));
    
    if (children == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    
    for(int i = 0; i < numChildren; i++){

        if (pipe(children[i].child_pipe_fd) == -1) {
            perror("Error with opening pipes");
            exit(EXIT_FAILURE);
        }
        if (pipe(children[i].parent_pipe_fd) == -1) {
            perror("Error with opening pipes");
            exit(EXIT_FAILURE);
        }
        children[i].filename = filename;
        snprintf(children[i].name, sizeof(children[i].name), "Child%d", i+1);

    }

    
    for(int i = 0; i < numChildren; i++){
        pid_t child_pid = fork();
        if (child_pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (child_pid == 0) { // Child process
            childFunc(&children[i]);

            
        }else{ //Parent process

        }

    }
    parentFunc(numChildren,children);
    


    for (int i = 0; i < numChildren; ++i) {
        wait(NULL);
    }




    return 0;
}