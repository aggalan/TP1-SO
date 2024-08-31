#ifndef _MEMORY
#define _MEMORY
#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>

#define SEM_SWITCH_NAME "/SHM_SWITCH"
#define SEM_MUTEX_NAME "/SHM_MUTEX"
#define SHM_NAME "/buffer"

#endif