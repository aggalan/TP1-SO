// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
#include "commons.h"
#include "memory.h"

#include <sys/wait.h>
#include <sys/select.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>

int pipe_read(int fd, char *buffer);
void read_memory(sem_t *sem_mutex, sem_t *sem_switch, char *shm);
sem_t *open_semaphores(const char *sem_name, int mode);
char *init_shm(const char *shm_name, size_t size, int *shm_fd);

int main(int argc, char *argv[])
{

    memoryADT adt = {0};

    sem_unlink(SEM_SWITCH_NAME);
    sem_unlink(SEM_MUTEX_NAME);
    char shm_name[MAX_PATH] = {0};

    if (argc < 2)
    {

        pipe_read(STDIN_FILENO, shm_name);
        if (shm_name[0] == '\0')
        {
            printf("ERROR");
            exit(1);
        }
        initializeResources(&adt, shm_name, SEM_MUTEX_NAME, SEM_SWITCH_NAME, SIZE);
    }
    else
    {
        initializeResources(&adt, argv[1], SEM_MUTEX_NAME, SEM_SWITCH_NAME, SIZE);
    }


    openResources(&adt);

    int index = 0;
    int status = 0;

    while (1)
    {
        readMemory(&adt, &index, &status);
        if (status)
        {
            break;
        }
    }

    closeResources(&adt);

    return 0;
}

int pipe_read(int fd, char *buff)
{
    int i = 0;
    char last_charater_read[1];
    last_charater_read[0] = 1;

    while (last_charater_read[0] != 0 && last_charater_read[0] != '\n' && read(fd, last_charater_read, 1) > 0)
    {
        buff[i++] = last_charater_read[0];
    }
    buff[i] = 0;

    return i;
}

sem_t *open_semaphores(const char *sem_name, int mode)
{
    sem_t *sem = sem_open(sem_name, O_CREAT, 0666, mode);
    if (sem == SEM_FAILED)
    {
        perror("sem_open");
        exit(1);
    }
    return sem;
}

char *init_shm(const char *shm_name, size_t size, int *shm_fd)
{
    *shm_fd = shm_open(shm_name, O_RDWR, 0666);
    if ((*shm_fd) == -1)
    {
        perror("shm_open_VIEW");
        exit(EXIT_FAILURE);
    }
    char *shared_memory = mmap(NULL, size, PROT_READ, MAP_SHARED, *shm_fd, 0);
    if (shared_memory == MAP_FAILED)
    {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    return shared_memory;
}