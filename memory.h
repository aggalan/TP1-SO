#ifndef _MEMORY
#define _MEMORY
#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>

#define SEM_SWITCH_NAME "/SHM_SWITCH"
#define SEM_MUTEX_NAME "/SHM_MUTEX"
#define SHM_NAME "/buffer"
#define SIZE 1048576

typedef struct memoryADT{
    char * shm;
    sem_t * sem_mutex;
    sem_t * sem_switch;
    size_t size;
    int shm_fd;
    char * shm_name;
    char * sem_mutex_name;
    char * sem_switch_name;
    int view_opened;

}memoryADT;

void startResources(memoryADT * adt, char * shm_name, char * sem_mutex_name, char * sem_switch_name, size_t size);


void initializeResources(memoryADT * adt, char * shm_name, char * sem_mutex_name, char * sem_switch_name, size_t size);

void createResources(memoryADT * adt);


void openResources(memoryADT * adt);

//quizas no hace falta
void unlinkResources(memoryADT * adt);


void closeResources(memoryADT * adt);

void readMemory(memoryADT * adt, int * index, int * status);

#endif