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

int main(int argc, char *argv[])
{

    memory_adt adt = {0};
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
        initialize_resources(&adt, shm_name, SEM_MUTEX_NAME, SEM_SWITCH_NAME, SIZE);
    }
    else
    {
        initialize_resources(&adt, argv[1], SEM_MUTEX_NAME, SEM_SWITCH_NAME, SIZE);
    }

    open_resources(&adt);

    int index = 0;
    int status = 0;

    while (1)
    {
        read_memory(&adt, &index, &status);
        if (status)
        {
            break;
        }
    }

    close_resources(&adt);

    return 0;
}
