#include "shm_manager.h"
#include "commons.h"

void * init_shm(const char *name, size_t size, int * shm_fd, int mode)
{
    // crea la shared memory
    (*shm_fd) = shm_open(name, O_CREAT | O_RDWR, 0666);
    if ((*shm_fd) == -1)
    {
        perror("shm_open");
        exit(1); 
    }

    // Seteo la shm para que sea del size que quiero, si es el proceso md5
    if (mode > 0)
    {
        if (ftruncate(*shm_fd, size) == -1)
        {
            perror("ftruncate");
            exit(1);
        }
    }

    // Mapeo la shared memory a menoria
    void *ptr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, *shm_fd, 0);
    if (ptr == MAP_FAILED)
    {
        perror("mmap");
        exit(1);
    }

    return ptr;
}


void clean_shm(void * ptr, size_t size, int shm_fd){
    //limpio la shared memory
    if (munmap(ptr, size) == -1) {
        perror("munmap");
        exit(1);
    }

    if (close(shm_fd) == -1) {
        perror("close");
        exit(1);
    }

    // if (shm_unlink(name) == -1) {
    //     perror("shm_unlink");
    //     exit(1);
    // }
}

void * init_semaphore(const char *sem_name){
    sem_t *sem = sem_open(sem_name, O_CREAT, 0666, 0);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        exit(1);
    }
    return sem;
}

void post_semaphore(void * ptr, char * md5, sem_t * sem_name){

    //Aca deberia escribir todo
    sprintf((char *)ptr, "%s", md5);
    //aviso que esta listo para ser leido
    sem_post(sem_name);
}

void clean_semaphore(sem_t * sem, const char * sem_name){
 // Close the semaphore
    if (sem_close(sem) == -1) {
        perror("sem_close");
        exit(1);
    }

    // Unlink the semaphore now that we're done with it
    if (sem_unlink(sem_name) == -1) {
        perror("sem_unlink");
        exit(1);
    }

}


