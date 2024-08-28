#include "commons.h"
#include <sys/wait.h>
#include <sys/select.h>

#define FILES_PER_SLAVE 2

struct file_info {
    pid_t pid;
    char md5[100];
    char filename[100];
};


int main(int argc, char *argv[]) {
    int slaves = ((argc - 1) / 20) > 0? (argc-1)/20:2;
    // int slaves = 15;
    
    if(argc < 2){
        fprintf(stderr, "Usage: %s <file_path>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int files_processed = 0, files_read = 0;
    int max_fd = 0;

    const char * filename = "results.txt";
    FILE * file = fopen(filename, "w");

    if (file == NULL) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    // char buff[100];
    // char * buff1 = "commons.h";


    // tengo que leer y escribir con los hijos asi que creo dos pipes
    int pipe_to_child[slaves][2], pipe_from_child[slaves][2]; 

    pid_t pids[slaves];


    for (int i = 0; i < slaves; i++) { 
        if (pipe(pipe_to_child[i]) == -1 || pipe(pipe_from_child[i]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i< slaves; i++) {

        if ((pids[i] = fork()) == 0) {
            //proceso hijo
            close(pipe_to_child[i][1]);
            close(pipe_from_child[i][0]);

            //redirecciono las entradas y salidas correspondiente de los pipes
            dup2(pipe_to_child[i][0], STDIN_FILENO);
            close(pipe_to_child[i][0]);
            

            dup2(pipe_from_child[i][1], STDOUT_FILENO);
            close(pipe_from_child[i][1]);

            char *args[] = {"./slave", NULL};
            execve(args[0], args, NULL);
            perror("execve");
            exit(EXIT_FAILURE);

        }
        //proceso padre
        close(pipe_to_child[i][0]); //cierro el extremo de lectura
        close(pipe_from_child[i][1]); //cierro el extremo de escritura 
    }

  

    for (int i = 0; i < slaves; i++) {
        for (int j = 0; j < FILES_PER_SLAVE && files_processed < argc - 1; j++, files_processed++) {
            write(pipe_to_child[i][1], argv[files_processed + 1], strlen(argv[files_processed + 1]) + 1);
        }
    }

   while (files_read < argc - 1) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        max_fd = 0;

        for (int i = 0; i < slaves; i++) {
            FD_SET(pipe_from_child[i][0], &read_fds);
            if (pipe_from_child[i][0] > max_fd) {
                max_fd = pipe_from_child[i][0];
            }
        }

        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);

        if (activity == -1) {
            perror("select");
            exit(EXIT_FAILURE);
        } else if (activity > 0) {
            for (int i = 0; i < slaves; i++) {
                if (FD_ISSET(pipe_from_child[i][0], &read_fds)) {
                    char buff[100];
                    int bytes_read = read(pipe_from_child[i][0], buff, sizeof(buff));
                    if (bytes_read > 0) {
                        buff[bytes_read] = '\0';
                        fprintf(file, "PID: %d HASH: %s\n", pids[i], buff);
                        files_read++;

                        // Asigna el siguiente archivo disponible al esclavo que acaba de terminar
                        if (files_processed < argc - 1) {
                            write(pipe_to_child[i][1], argv[files_processed + 1], strlen(argv[files_processed + 1]) + 1);
                            files_processed++;
                        }
                    }
                }
            }
        }
    }


    for (int i = 0; i < slaves; i++) {
        close(pipe_to_child[i][1]);
        close(pipe_from_child[i][0]);
    }


    if (fclose(file) != 0) {
        perror("Error closing file");
        return EXIT_FAILURE;
    }

}
   