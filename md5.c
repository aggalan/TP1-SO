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

int main(int argc, char *argv[])
{
    memory_adt adt = {0};
    int slaves = ((argc - 1) > 20) ? ((argc - 1) / 10) : ((argc - 1) > 1 ? 2 : 1);
    int initial_files_per_slave = ((argc - 1) / 50 > 0) ? ((argc - 1) / 50) : 1;

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

    start_resources(&adt, SHM_NAME, SEM_MUTEX_NAME, SEM_SWITCH_NAME, SIZE);

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
                if (adt.view_opened == 2)
                {
                    sprintf(adt.shm + files_read * info_length, "MD5: %s - PID %d\n", md5, pids[i]);
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
        int status;
        pid_t pid = waitpid(pids[i], &status, 0);
        if (pid == -1)
        {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }
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