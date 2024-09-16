// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
#include "commons.h"
#include "memory.h"
#include <sys/wait.h>
#include <sys/select.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void start_resources(memory_adt *adt, char *shm_name, char *sem_mutex_name, char *sem_switch_name, size_t size)
{
    initialize_resources(adt, shm_name, sem_mutex_name, sem_switch_name, size);
    unlink_resources(adt);
    create_resources(adt);
}

void initialize_resources(memory_adt *adt, char *shm_name, char *sem_mutex_name, char *sem_switch_name, size_t size)
{
    adt->shm_name = shm_name;
    adt->sem_mutex_name = sem_mutex_name;
    adt->sem_switch_name = sem_switch_name;
    adt->size = size;
}

void create_resources(memory_adt *adt)
{

    if ((adt->shm = mmap(NULL, adt->size, PROT_READ | PROT_WRITE, MAP_SHARED, adt->shm_fd, 0)) == MAP_FAILED)
    {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    sleep(2);

    if ((adt->sem_mutex = sem_open(adt->sem_mutex_name, 1)) != SEM_FAILED)
    {
        adt->view_opened++;
    }

    if ((adt->sem_switch = sem_open(adt->sem_switch_name, 0)) != SEM_FAILED)
    {
        adt->view_opened++;
    }

    if ((adt->view_opened) == 1)
    {
        perror("ERROR; MISSING 1 SEMAPHORE");
        exit(1);
    }

    else if ((adt->view_opened) == 0)
    {
        printf("No view opened\n");
    }

    if ((ftruncate(adt->shm_fd, adt->size)) < 0)
    {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }
}

void open_resources(memory_adt *adt)
{

    if ((adt->shm_fd = shm_open(adt->shm_name, O_RDWR, 0666)) < 0)
    {
        perror("shm_open_VIEW");
        exit(EXIT_FAILURE);
    }

    if ((adt->shm = mmap(NULL, adt->size, PROT_READ, MAP_SHARED, adt->shm_fd, 0)) == MAP_FAILED)
    {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    if ((adt->sem_mutex = sem_open(adt->sem_mutex_name, O_CREAT, 0666, 1)) == SEM_FAILED)
    {
        perror("sem_open_MUTEX_view");
        exit(EXIT_FAILURE);
    }

    if ((adt->sem_switch = sem_open(adt->sem_switch_name, O_CREAT, 0666, 0)) == SEM_FAILED)
    {
        perror("sem_open_SWITCH");
        exit(EXIT_FAILURE);
    }
}

void unlink_resources(memory_adt *adt)
{
    sem_unlink(adt->sem_mutex_name);
    sem_unlink(adt->sem_switch_name);
}

void close_resources(memory_adt *adt)
{
    sem_close(adt->sem_mutex);
    sem_close(adt->sem_switch);
    close(adt->shm_fd);
}

void read_memory(memory_adt *adt, int *index, int *status)
{
    sem_wait(adt->sem_switch);
    sem_wait(adt->sem_mutex);

    while (adt->shm[*index] != '\n' && adt->shm[*index] != '\t')
    {
        printf("%c", adt->shm[*index]);
        (*index)++;
    }
    if (adt->shm[*index] == '\t')
    {
        sem_post(adt->sem_mutex);
        *status = 1;
        return;
    }
    printf("%c", adt->shm[*index]);
    (*index)++;
    sem_post(adt->sem_mutex);
}

void setup_pipes_and_forks(int slaves, int pipe_to_child[][2], int pipe_from_child[][2], pid_t pids[], int *shm_fd)
{
    for (int i = 0; i < slaves; i++)
    {
        if (pipe(pipe_to_child[i]) == -1 || pipe(pipe_from_child[i]) == -1)
        {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        if ((pids[i] = fork()) == 0)
        {
            close(pipe_to_child[i][1]);
            close(pipe_from_child[i][0]);

            dup2(pipe_to_child[i][0], STDIN_FILENO);
            close(pipe_to_child[i][0]);
            close(*shm_fd);

            dup2(pipe_from_child[i][1], STDOUT_FILENO);
            close(pipe_from_child[i][1]);

            for (int j = 0; j < i; j++)
            {
                for (int k = 0; k < 2; k++)
                {
                    close(pipe_to_child[j][k]);
                    close(pipe_from_child[j][k]);
                }
            }

            char *args[] = {"./slave", NULL};
            execve(args[0], args, NULL);
            perror("execve");
            exit(EXIT_FAILURE);
        }
        close(pipe_to_child[i][0]);
        close(pipe_from_child[i][1]);
    }
}

void write_to_pipe(int fd, char **argv, int *files_processed, int total_files, int qty)
{
    for (int i = 0; i < qty; i++)
    {
        if ((*files_processed) < total_files)
        {
            int bytes_written = write(fd, argv[*files_processed + 1], strlen(argv[(*files_processed) + 1]) + 1);
            if (bytes_written < 0)
            {
                perror("write pipe");
                exit(EXIT_FAILURE);
            }
            (*files_processed)++;
        }
        else
        {
            break;
        }
    }
}

int pipe_read(int fd, char *buffer)
{
    int i = 0;
    char chunk[CHUNK_SIZE];
    ssize_t bytes_read;
    int chunk_end;

    while (1)
    {

        memset(chunk, 0, CHUNK_SIZE);

        bytes_read = read(fd, chunk, CHUNK_SIZE);
        if (bytes_read <= 0)
        {
            break;
        }

        for (chunk_end = 0; chunk_end < bytes_read; chunk_end++)
        {
            if (chunk[chunk_end] == '\n' || chunk[chunk_end] == '\0')
            {
                buffer[i++] = chunk[chunk_end];
                buffer[i] = 0;
                return i;
            }
            buffer[i++] = chunk[chunk_end];
        }
    }

    buffer[i] = 0;
    return i;
}
