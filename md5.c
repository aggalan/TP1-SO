#include "commons.h"
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    
    if(argc < 2){
        fprintf(stderr, "Usage: %s <file_path>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    //Como crear un pipe

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }



    //Logica de la creacion de procesos hijo

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Proceso hijo
        close(pipefd[1]); // Cierro el pipe de escritura
        dup2(pipefd[0], STDIN_FILENO); // Redirecciono el stdin a la lectura del pipe
        close(pipefd[0]); // Cierro el pipe de lectura

        char *args[] = {"./slave", NULL};
        execve(args[0], args, NULL);
        // Si execve falla esto de abajo no se ejecuta
        perror("execve");
        exit(EXIT_FAILURE);

    } else {
        // Proceso padre
        close(pipefd[0]); // Cierro el pipe de lectura
        dup2(pipefd[1], STDOUT_FILENO); // Redirecciono el stdout a la escritura del pipe
        close(pipefd[1]); // Cierro el pipe de escritura

        //Aca podria ejecutar un programa para manejar el stream de paths
        char *args[] = {"./manager", NULL};
        execve(args[0], args, NULL);
        // Si execve falla esto de abajo no se ejecuta
        perror("execve");
        exit(EXIT_FAILURE);
    }

    // Parent waits for the child to finish
    wait(NULL);
}
    

