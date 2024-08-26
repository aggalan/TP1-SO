#include "commons.h"
#define MAX_CMD_SIZE 100
#define MAX_PATH_SIZE 100

int main() {
    
    char path[MAX_PATH_SIZE] = {0};
    char command[MAX_CMD_SIZE];
    char * md5 = "md5sum %s";

    while(1){

    // lectura del pipe (HACER)

    //Me fijo que queden path para leer, sino corto
    if(path[0] ==  0){
        break;
    }
    
    //Guardo el comando 
    sprintf(command, md5, path);

    //Ejecuto el comando en la shell
    FILE * log = popen(command, "r");

    if(log == NULL){
        perror("peopen");
        exit(EXIT_FAILURE);
    }

    pclose(log);

    }

    // Escribir el pipe

    return 0;
}
