#ifndef _SHM_MANAGER
#define _SHM_MANAGER

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>

void * init_shm(const char *name, size_t size, int * shm_fd, int mode);
void clean_shm(void * ptr, size_t size, int shm_fd);
void * init_semaphore(const char *sem_name);
void post_semaphore(void * ptr, char * md5, sem_t *sem);
void clean_semaphore(sem_t * sem, const char * sem_name);

#endif