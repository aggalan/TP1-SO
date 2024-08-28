#include "commons.h"
#define MAX_CMD_SIZE 100
#define MAX_PATH_SIZE 100
#define MAX_MD5_SIZE 100

int main() {
    
    char path[MAX_PATH_SIZE] = {0};
    char command[MAX_CMD_SIZE];
    char * md5sum = "md5sum %s";
    char md5[MAX_MD5_SIZE];

    // while (1) {
    //     if (read(STDIN_FILENO, path, 10)) {
    //         write(STDOUT_FILENO, path, 10);
    //         break;
    //     }
    // }

    


    while(1){
        
        // lectura del pipe
        if (read(STDIN_FILENO, path, MAX_PATH_SIZE) <= 0) {
                // revisa si no hay nada mas que leer o error
                continue;
        } else {
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
        fgets(md5, MAX_MD5_SIZE, log);
        

        //Cierro
        pclose(log);

        //Escribo en el pipe 
        write(STDOUT_FILENO, md5, strlen(md5) + 1);

        fflush(stdout); // basicamente las funciones com printf y demas usan un buffer y esta maravilla acelera y asegura el pasaje de datos al stream
        
        
        }


        
    
    }
}