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

    fd_set read_fds, write_fds; //armo mis 2 fd sets para el select (uno para cada pipe)



    for (int i = 0; i < slaves; i++) {
        for (int j = 0; j < FILES_PER_SLAVE; j++, files_processed++) {
            if (files_processed < argc - 1) {
                write(pipe_to_child[i][1], argv[files_processed + 1], strlen(argv[files_processed + 1]) + 1);
            } else {
                break;
            }
        }
    }

    while(files_read < argc - 1) {
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);

        for (int i = 0; i < slaves; i++) {

            FD_SET(pipe_to_child[i][1], &write_fds);
            FD_SET(pipe_from_child[i][0], &read_fds);

            if (pipe_to_child[i][1] > max_fd) {
                max_fd = pipe_to_child[i][1];
            }
            if (pipe_from_child[i][0] > max_fd) {
                max_fd = pipe_from_child[i][0];
            }

        }

        int activity = select(max_fd + 1, &read_fds, &write_fds, NULL, NULL);

        

        if (activity == -1) {
            //womp womp
            perror("select");
            exit(EXIT_FAILURE);
        } else if (activity > 0) {
            //hay algun archivo listo para recibir o leer
            for (int i = 0; i < slaves; i++) {
                if (FD_ISSET(pipe_from_child[i][0], &read_fds)) {
                    //hay algo para leer
                    char buff[100];
                    read(pipe_from_child[i][0], buff,100);
                    fprintf(file, "PID: %d HASH: %s\n", pids[i], buff);
                    files_read++;
                }

                if(FD_ISSET(pipe_to_child[i][1], &write_fds)) {
                    //hay algo para escribir
                    if (files_processed < argc - 1) {
                        write(pipe_to_child[i][1], argv[files_processed + 1], strlen(argv[files_processed + 1]) + 1);
                        files_processed++;
                    }
                }
            }
        }

    }


    for (int i = 0; i < slaves; i++) {
        close(pipe_to_child[i][1]);
        close(pipe_from_child[i][0]);
    }


    

    // printf("ACA");

    // write(pipe_to_child[0][1], buff1, strlen(buff1) + 1);

    // printf("ACA1");

    // wait(NULL);

    // printf("ACA2\n");

    // read(pipe_from_child[0][0], buff, 100);

    // printf("el buffer dice: %s\n", buff);

    if (fclose(file) != 0) {
        perror("Error closing file");
        return EXIT_FAILURE;
    }

}
   