#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <errno.h>

#define MESSAGE_SIZE 100

int interrupt = 0;

typedef struct{
    char name[64];
    int pipe_fd[2];
    char *filename;
    int id;
} Child;

void interruptHandler(int signo){
    interrupt = 1;
}


void childFunc(Child *child){
   

    char message[MESSAGE_SIZE];
    FILE *file = fopen(child->filename, "a");

    if (!file) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    child->id = getpid();
    fprintf(file, "[%s] ---> <%d>\n", child->name, child->id);
    fclose(file);

    write(child->pipe_fd[1],"done",4);
    exit(EXIT_SUCCESS);

  


}    
//0 reading
//1 writting
void parentFunc(int numChildren,Child *children){

    fd_set read_fds;
    int max_fd = -1;


    while(!interrupt){

        FD_ZERO(&read_fds);

        for(int i = 0;i < numChildren;i++ ){
            FD_SET(children[i].pipe_fd[0],&read_fds);

            if(children[i].pipe_fd[0] > max_fd){
                max_fd = children[i].pipe_fd[0];
            }
    }

        if(select(max_fd + 1, &read_fds, NULL,NULL,NULL) < 0){
            if(errno == EINTR){
                continue;
            }else{
                perror("select");
                exit(EXIT_FAILURE);
            }
        }        

        for (int i = 0; i < numChildren;i++){
            if(FD_ISSET(children[i].pipe_fd[0],&read_fds)){
                char message[4];
                ssize_t result = read(children[i].pipe_fd[0], message, sizeof(message));

                if (result > 0) {
                    
                    printf("%s", message);
                } else if (result == 0) {
                    close(children[i].pipe_fd[0]);
                    FD_CLR(children[i].pipe_fd[0], &read_fds);
                    
                } else {
                    perror("read");
                    exit(EXIT_FAILURE);
                }
                
            }

        }
        
    }

    for (int i = 0; i < numChildren; i++) {
        close(children[i].pipe_fd[0]);
        kill(children[i].id,SIGTERM);
        
    }

    printf("Parent exiting\n");
    exit(EXIT_SUCCESS);

  
}

int main(int argc, char *argv[]){
    

    if (argc != 3) {
        printf("Usage: %s <filename> <number_of_children>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, interruptHandler);

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

        if (pipe(children[i].pipe_fd) == -1) {
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



    free(children);


    return 0;
}