#include "commons.h"
#define MAX_CMD_SIZE 100
#define MAX_PATH_SIZE 100
#define MAX_MD5_SIZE 100

// int pipe_read(int fd, char * buffer){
//    int i = 0;
//    char last_read[1];
//    last_read[0] = 1;
//    while(last_read[0] != 0 && last_read[0] != '\n' && read(fd, last_read, 1) > 0){
//       buffer[i++] = last_read[0];
//    }
//    buffer[i] = 0;
//    return i;
// }
//esto lo agrego post porque necesito una condicion para que el while corte


int main() {
    
    char path[MAX_PATH_SIZE] = {0};
    char command[MAX_CMD_SIZE];
    char * md5sum = "md5sum %s";
    char md5[MAX_MD5_SIZE];

    while(pipe_read(STDIN_FILENO, path) > 0){       
        printf("gay\n");     
        // lectura del pipe
        if (fgets(path, MAX_PATH_SIZE, stdin) == NULL) {
                // revisa si no hay nada mas que leer o error
                continue;
        }


        // saco el newline y lo reemplazo por null terminated
        path[strcspn(path, "\n")] = '\0';


        //Guardo el comando 
        sprintf(command, md5sum, path);
        //podria ser asi y se usa el tamano prefijado:
        //snprintf(command, MAX_CMD_SIZE, md5sum, path);

        //Ejecuto el comando en la shell
        FILE * log = popen(command, "r");

        if(log == NULL){
            perror("peopen");
            exit(EXIT_FAILURE);
        }



        //leo md5 checksum
        if (fgets(md5, MAX_MD5_SIZE, log) != NULL) {
                // Null-terminate the md5 string properly
                md5[strcspn(md5, "\n")] = '\0';
        }

        //Cierro
        pclose(log);

        //Escribo en el pipe 
        write(STDOUT_FILENO, md5, strlen(md5) + 1);

        // fflush(stdout); // basicamente las funciones com printf y demas usan un buffer y esta maravilla acelera y asegura el pasaje de datos al stream
    
    }
    close(STDOUT_FILENO);

}