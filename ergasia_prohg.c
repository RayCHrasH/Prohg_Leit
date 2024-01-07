#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MESSAGE_SIZE 100

typedef struct{
    char name[64];
    int pipe_fd[2];
    char *filename;
    int id;
} Child;

void childFunc(Child *child){
    close(child->pipe_fd[1]);

    char message[MESSAGE_SIZE];
    read(child->pipe_fd[0], message,sizeof(message));
    close(child->pipe_fd[0]);

    FILE *file = fopen(child->filename,"a");
    fprintf(file, "%d ---> %s\n", getpid(), message);
    fclose(file);

    write(child->pipe_fd[1],"done",4);
    close(child->pipe_fd[1]);

    exit(EXIT_SUCCESS);
}

void parentFunc(Child *child){
    close(child->pipe_fd[0]);
    char message[MESSAGE_SIZE];
    sprintf(message, "Hello child, I am your father and I call you: %s", child->name);
    write(child->pipe_fd[1],message,sizeof(message));
    close(child->pipe_fd[1]);

    char done_message[4];
    read(child->pipe_fd[0], done_message, sizeof(done_message));
    close(child->pipe_fd[0]);
    
}

int main(int argc, char *argv[]){
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

        if (pipe(children[i].pipe_fd) == -1) {
            perror("Error with opening pipes");
            exit(EXIT_FAILURE);
        }
        children[i].id = i + 1;
        children[i].filename = filename;


        snprintf(children[i].name, sizeof(children[i].name), "Child%d", i + 1);

    }

    for(int i = 0; i < numChildren; i++){
        pid_t child_pid = fork();

        if (child_pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (child_pid == 0) {       // Child process
            childFunc(&children[i]);
        }else {                     // Parent process
            parentFunc(&children[i]);
        }


    }

    for (int i = 0; i < numChildren; ++i) {
        wait(NULL);
    }

    free(children);


    return 0;

}