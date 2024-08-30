#include "commons.h"
#include "memory.h"

#include <sys/wait.h>
#include <sys/select.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>

int pipe_read(int fd, char * buffer);
void read_memory(sem_t * sem_mutex, sem_t * sem_switch, char * shm);
sem_t *open_semaphores(const char *sem_name, int mode);
char * init_shm(const char * shm_name, size_t size, int *shm_fd);

int main(int argc, char *argv[]) {
    sem_unlink("/SHM_SWITCH");
    sem_unlink("/SHM_MUTEX");
    
    int shm_fd;
    char shm_name[100] = {0};
    size_t size = 1048576;
    char * shm;
    sem_t * sem_mutex = open_semaphores("/SHM_MUTEX", 1);
    sem_t * sem_switch = open_semaphores("/SHM_SWITCH", 0);



    if(argc < 2){
        
        pipe_read(STDIN_FILENO, shm_name);
        if(shm_name[0] == '\0'){
            printf("ERROR");
            exit(1);
        }
        shm = init_shm(shm_name, size, &shm_fd);
    }
    else{
        shm = init_shm(argv[1], size, &shm_fd);
    }

    read_memory(sem_mutex, sem_switch, shm);
    close(shm_fd);
    if(sem_close(sem_mutex) == -1){
        perror("sem_close_MUTEX_VIEW");
        exit(EXIT_FAILURE);
    }
    if(sem_close(sem_switch) == -1){
        perror("sem_close_switch_view");
        exit(EXIT_FAILURE);
    }


    return 0;
}


int pipe_read(int fd, char *buff){
    int i=0;
    char last_charater_read[1];
    last_charater_read[0]=1;

    while(last_charater_read[0]!=0 && last_charater_read[0]!='\n' && read(fd,last_charater_read,1)>0){
        buff[i++]=last_charater_read[0];
    }
    buff[i]=0;

    return i;
}

void read_memory(sem_t * sem_mutex, sem_t * sem_switch, char * shm){
    int index = 0;

    while(1){
        sem_wait(sem_switch);
        sem_wait(sem_mutex);  
        while(shm[index] != '\n' && shm[index] != '\t'){
            printf("%c", shm[index]);
            index++;
        } 
        if(shm[index] == '\t'){
            sem_post(sem_mutex);
            break;
        }
        printf("%c", shm[index]);
        index++;
        sem_post(sem_mutex);
    }
}

sem_t * open_semaphores(const char *sem_name, int mode){
    sem_t *sem = sem_open(sem_name, O_CREAT, 0666, mode);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        exit(1);
    }
    return sem;
}

char * init_shm(const char * shm_name, size_t size, int *shm_fd){
    *shm_fd = shm_open(shm_name, O_RDWR, 0666);
    if ((*shm_fd) == -1) {
        perror("shm_open_VIEW");
        exit(EXIT_FAILURE);
    }
    char *shared_memory = mmap(NULL, size, PROT_READ, MAP_SHARED, *shm_fd, 0);
    if (shared_memory == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    return shared_memory;
}