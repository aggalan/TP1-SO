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

typedef struct memory_adt
{
    char *shm;
    sem_t *sem_mutex;
    sem_t *sem_switch;
    size_t size;
    int shm_fd;
    char *shm_name;
    char *sem_mutex_name;
    char *sem_switch_name;
    int view_opened;

} memory_adt;

void start_resources(memory_adt *adt, char *shm_name, char *sem_mutex_name, char *sem_switch_name, size_t size);

void initialize_resources(memory_adt *adt, char *shm_name, char *sem_mutex_name, char *sem_switch_name, size_t size);

void create_resources(memory_adt *adt);

void open_resources(memory_adt *adt);

void close_resources(memory_adt *adt);

void read_memory(memory_adt *adt, int *index, int *status);

void setup_pipes_and_forks(int slaves, int pipe_to_child[][2], int pipe_from_child[][2], pid_t pids[], int *shm_fd);

void write_to_pipe(int fd, char **argv, int *files_processed, int total_files, int qty);

int pipe_read(int fd, char *buffer);

#endif