#include "commons.h"
#define OFFSET 3

int pipe_read(int fd, char * buffer);

int main() {
    
    char path[MAX_PATH] = {0};
    char * md5sum = "md5sum %s";
    char command[MAX_PATH+strlen(md5sum)];
    char md5[MAX_MD5+MAX_PATH+OFFSET];

    while(pipe_read(STDIN_FILENO, path) > 0){       
        
        if(path[0] == 0){
            break;
        }

        sprintf(command, md5sum, path);

        FILE * log = popen(command, "r");

        if(log == NULL){
            perror("peopen");
            exit(EXIT_FAILURE);
        }

        fgets(md5, MAX_MD5+strlen(path), log);

        //Cierro
        pclose(log);

        //Escribo en el pipe 
        write(STDOUT_FILENO, md5, strlen(md5) + 1);

    
    }
    close(STDOUT_FILENO);
    exit(EXIT_SUCCESS);

}

int pipe_read(int fd, char * buffer){
   int i = 0;
   char last_read[1];
   last_read[0] = 1;
   while(last_read[0] != 0 && last_read[0] != '\n' && read(fd, last_read, 1) > 0){
      buffer[i++] = last_read[0];
   }
   buffer[i] = 0;
   return i;
}