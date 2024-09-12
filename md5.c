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

void setup_pipes_and_forks(int slaves, int pipe_to_child[][2], int pipe_from_child[][2], pid_t pids[], int *shm_fd);
void write_to_pipe(int fd, char **argv, int *files_processed, int total_files, int qty);
int pipe_read(int fd, char *buffer);

int main(int argc, char *argv[])
{
    memoryADT adt = {0};
    int slaves = ((argc - 1) > 20) ? ((argc - 1) / 10) : 2;
    int initial_files_per_slave = ((argc - 1) / 10 / slaves > 1) ? (((argc - 1) / 10 / (slaves))) : 1;

    int files_to_process = argc - 1;
    int files_processed = 0, files_read = 0;
    int pipe_to_child[slaves][2], pipe_from_child[slaves][2];
    pid_t pids[slaves];
    int info_length = strlen("MD5: %s - PID %d\n") + MAX_MD5 + MAX_PATH + 2;

    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <file_path>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    adt.shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);
    if (adt.shm_fd == -1)
    {
        perror("shm_open_MD5");
        exit(EXIT_FAILURE);
    }

    if (!isatty(STDOUT_FILENO))
    {
        write(STDOUT_FILENO, SHM_NAME, strlen(SHM_NAME) + 1);
    }
    else
    {
        printf("%s\n", SHM_NAME);
        fflush(stdout);
    }

    startResources(&adt, SHM_NAME, SEM_MUTEX_NAME, SEM_SWITCH_NAME, SIZE);

    const char *filename = "results.txt";
    FILE *file = fopen(filename, "w");
    if (file == NULL)
    {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    setup_pipes_and_forks(slaves, pipe_to_child, pipe_from_child, pids, &adt.shm_fd);

    for (int i = 0; i < slaves; i++)
    {
        write_to_pipe(pipe_to_child[i][1], argv, &files_processed, files_to_process, initial_files_per_slave);
    }

    while (files_read < files_to_process)
    {

        fd_set read_fds;
        FD_ZERO(&read_fds);
        int max_fd = -1;

        for (int i = 0; i < slaves; i++)
        {
            FD_SET(pipe_from_child[i][0], &read_fds);

            if (pipe_from_child[i][0] > max_fd)
            {
                max_fd = pipe_from_child[i][0];
            }
        }

        select(max_fd + 1, &read_fds, NULL, NULL, NULL);

        if (adt.view_opened == 2)
        {
            sem_wait(adt.sem_mutex);
        }
        for (int i = 0; i < slaves; i++)
        {

            if (FD_ISSET(pipe_from_child[i][0], &read_fds))
            {
                char md5[MAX_MD5 + MAX_PATH + 6] = {0};
                pipe_read(pipe_from_child[i][0], md5);
                fprintf(file, "MD5: %s - PID %d\n", md5, pids[i]);
                fflush(file);
                sprintf(adt.shm + files_read * info_length, "MD5: %s - PID %d\n", md5, pids[i]);
                if (adt.view_opened == 2)
                {
                    sem_post(adt.sem_switch);
                    sem_post(adt.sem_mutex);
                }

                if (files_processed < files_to_process)
                {
                    write_to_pipe(pipe_to_child[i][1], argv, &files_processed, files_to_process, 1);
                }

                files_read++;
            }
        }
    }
    if (adt.view_opened == 2)
    {
        sem_wait(adt.sem_mutex);
        sprintf(adt.shm + files_read * info_length, "\t");
        sem_post(adt.sem_mutex);
        sem_post(adt.sem_switch);
        sem_close(adt.sem_mutex);
        sem_close(adt.sem_switch);
    }

    for (int i = 0; i < slaves; i++)
    {
        close(pipe_to_child[i][1]);
        close(pipe_from_child[i][0]);
    }

    if (fclose(file) != 0)
    {
        perror("Error closing file");
        return EXIT_FAILURE;
    }

    sem_unlink(SEM_MUTEX_NAME);
    sem_unlink(SEM_SWITCH_NAME);
    shm_unlink(SHM_NAME);
    close(adt.shm_fd);

    return EXIT_SUCCESS;
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
