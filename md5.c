#include "commons.h"
#include <sys/wait.h>

#define MAX_SLAVES 2
#define FILES_PER_SLAVE 1

int main(int argc, char *argv[]) {
    
    if(argc < 2){
        fprintf(stderr, "Usage: %s <file_path>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *file_paths[argc - 1];
    int files_processed = 0;
    int max_fd = 0;

    const char * filename = "results.txt";
    FILE * file = fopen(filename, "w");

    if (file == NULL) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }


    // tengo que leer y escribir con los hijos asi que creo dos pipes
    int pipe_to_child[MAX_SLAVES][2], pipe_from_child[MAX_SLAVES][2]; 

    pid_t pids[MAX_SLAVES];

    for (int i = 0; i< MAX_SLAVES; i++) {
        if (pipe(pipe_to_child[i]) == -1 || pipe(pipe_from_child[i]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        if ((pids[i] = fork()) == 0) {
            //proceso hijo
            close(pipe_to_child[i][1]);
            close(pipe_from_child[i][0]);

            //redirecciono las entradas y salidas correspondiente de los pipes
            dup2(pipe_to_child[i][0], STDIN_FILENO);
            close(pipe_to_child[i][0]);
            

            dup2(pipe_from_child[i][1], STDOUT_FILENO);
            close(pipe_from_child[i][1]);

            //ver tema argumentos
            char *args[] = {"./slave", NULL};
            execve(args[0], args, NULL);
            perror("execve");
            exit(EXIT_FAILURE);

        }
        //proceso padre
        close(pipe_to_child[i][0]); //cierro el extreo de lectura
        close(pipe_from_child[i][1]); //cierro el extremo de escritura 
    }

    fd_set read_fds, write_fds; //armo mis 2 fd sets para el select (uno para cada pipe)
    struct timeval timeout; // ni idea el select recibe este struct pero detalles

    while (files_processed < argc - 1) {
        //inicializo ambos sets para que esten vacios
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);


        for (int i = 0; i < MAX_SLAVES; i++) {
            
            //meto todos los fds en los sets correspondientes
            FD_SET(pipe_to_child[i][1], &write_fds); 
            FD_SET(pipe_from_child[i][0], &read_fds); 

            //calculo el maximo para select
            if (pipe_to_child[i][1] > max_fd) {
                max_fd = pipe_to_child[i][1];
            }
            if (pipe_from_child[i][0] > max_fd) {
                max_fd = pipe_from_child[i][0];
            }
        }

        timeout.tv_sec = 1; // nuevamente, ni idea le clavo un segundo juan carlos me dijo
        timeout.tv_usec = 0;

        //el famoso select, que borra de los sets todos los fds que esten blocked y deja solo los ready
        int activity = select(max_fd + 1, &read_fds, &write_fds, NULL, &timeout); 

        //standard issue chequeo de error
        if (activity < 0) {
            perror("select");
            exit(EXIT_FAILURE);
        }


        for (int i = 0; i < MAX_SLAVES; i++) {
            if (FD_ISSET(pipe_to_child[i][1], &write_fds)) {
                //esta disponible el pipe para pasarle cositas al hijo
                if (files_processed < argc - 1) { //confirmar si es necesario? no creo pero no mata a nadie asi q lo dejo
                    ssize_t bytes_written = write(pipe_to_child[i][1], argv[files_processed + 1], strlen(argv[files_processed + 1]) + 1);
                    //standard issue chequeo de error
                    if (bytes_written == -1) {
                        perror("write");
                        exit(EXIT_FAILURE);
                    }
                    files_processed++;
                }
            }

            if (FD_ISSET(pipe_from_child[i][0], &read_fds)) {
                //esta listo el pipe para ser leido
                char buffer[256];
                ssize_t bytes_read = read(pipe_from_child[i][0], buffer, sizeof(buffer));
                if (bytes_read > 0) {
                    buffer[bytes_read-1] = '\n'; // quiero que haga enter >:(
                    buffer[bytes_read] = '\0'; // null terinated
                    // printf("recibido de hijo %d: %s\n", i, buffer); lol no va
                    if (fprintf(file, "%s", buffer) < 0) {// escribo en el archivo results un md5
                        perror("Error writing to file");
                        fclose(file);
                        exit(EXIT_FAILURE);
                    }

                    // si hay mas archivos por procesar cuando leo aprovecho y mando
                    if (files_processed < argc - 1) {
                        ssize_t bytes_written = write(pipe_to_child[i][1], argv[files_processed + 1], strlen(argv[files_processed + 1]) + 1);
                        //standard issue chequeo de error
                        if (bytes_written == -1) {
                            perror("write");
                            exit(EXIT_FAILURE);
                        }
                        files_processed++;
                    }
                } else if (bytes_read == -1) { //standard issue chequeo de error
                    perror("read");
                    exit(EXIT_FAILURE);
                }
            }
        }

    }

    //cierro el archivo results y standard issue chequeo de error
    if (fclose(file) != 0) {
        perror("Error closing file");
        return EXIT_FAILURE;
    }




}
    


    

