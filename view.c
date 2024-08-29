#include "commons.h"
#include "memory.h"
#include "shm_manager.h"
#include <sys/wait.h>
#include <sys/select.h>
#include <string.h>

int main() {

    int shm_fd;
    const char *shm_name = "/buffer";
    const char *sem_name = "/semaphore";
    size_t size = 4096;

    void * ptr = init_shm(shm_name, size, &shm_fd, 0);
    
    sem_t * sem = init_semaphore(sem_name);

    // Wait for the semaphore to be signaled
    while(1){

    sem_wait(sem);

    // Read from the shared memory
    printf("Message from shared memory: %s\n", (char *)ptr);


    }

    clean_shm(ptr, size, shm_fd);
    clean_semaphore(sem, sem_name);

    return 0;
}