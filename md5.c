#include "commons.h"
#include "memory.h"
#include <sys/wait.h>
#include <sys/select.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILES_PER_SLAVE 2
size_t size = 1048576;

void setup_pipes_and_forks(int slaves, int pipe_to_child[][2], int pipe_from_child[][2], pid_t pids[], int * shm_fd);
void write_to_pipe(int fd, char **argv, int *files_processed, int total_files, int qty);
char *read_from_pipe(int fd, char *buff);
int pipe_read(int fd, char *buffer);

int main(int argc, char *argv[])
{
    int view_opened = 0;
    int slaves = 5;
    int files_to_process = argc - 1;
    int files_processed = 0, files_read = 0;
    int pipe_to_child[slaves][2], pipe_from_child[slaves][2];
    pid_t pids[slaves];
    int info_length = strlen("MD5: %s - PID %d\n") + MAX_MD5 + MAX_PATH + 2;
    int shm_fd;

    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <file_path>\n", argv[0]);
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

    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);
    if (shm_fd == -1)
    {
        perror("shm_open_MD5");
        exit(EXIT_FAILURE);
    }


    char *shm = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm == MAP_FAILED)
    {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    sleep(2); //Le doy tiempo al view para que abra la shared memory y cree los semaforos

    sem_t *sem_mutex = sem_open("/SHM_MUTEX", 1);
    if (sem_mutex != SEM_FAILED)
    {
        view_opened++;
    }

    sem_t *sem_switch = sem_open("/SHM_SWITCH", 0);
    if (sem_switch != SEM_FAILED)
    {
        view_opened++;
    }
    

    if (view_opened == 1)
    {
        perror("ERROR; MISSING 1 SEMAPHORE");
        exit(1);
    }
    else if (view_opened == 0)
    {
        printf("NO VIEW OPENED\n");
    }
    if (ftruncate(shm_fd, size) == -1)
    {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    const char *filename = "results.txt";
    FILE *file = fopen(filename, "wr");
    if (file == NULL)
    {
        perror("Error opening file");
        return EXIT_FAILURE;
    }
    

    setup_pipes_and_forks(slaves, pipe_to_child, pipe_from_child, pids, &shm_fd);

    for (int i = 0; i < slaves; i++)
    {
        write_to_pipe(pipe_to_child[i][1], argv, &files_processed, files_to_process, FILES_PER_SLAVE);
    }

    while (files_read < files_to_process)
    {

        fd_set read_fds, write_fds;
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);
        int max_fd = -1;
        int max_wd = -1;

        for (int i = 0; i < slaves; i++)
        {
            FD_SET(pipe_from_child[i][0], &read_fds);
            FD_SET(pipe_to_child[i][1], &write_fds);

            if (pipe_from_child[i][0] > max_fd)
            {
                max_fd = pipe_from_child[i][0];
            }
            if (pipe_to_child[i][1] > max_wd)
            {
                max_wd = pipe_to_child[i][1];
            }
        }

        select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        select(max_wd + 1, NULL, &write_fds, NULL, NULL);

        if (view_opened == 2)
        {

            sem_wait(sem_mutex);
        }
        for (int i = 0; i < slaves; i++)
        {

            if (FD_ISSET(pipe_from_child[i][0], &read_fds))
            {
                char md5[MAX_MD5 + MAX_PATH + 6] = {0};
                pipe_read(pipe_from_child[i][0], md5);
                fprintf(file, "MD5: %s - PID %d\n", md5, pids[i]);
                fflush(file);
                sprintf(shm + files_read * info_length, "MD5: %s - PID %d\n", md5, pids[i]);
                if (view_opened == 2)
                {

                    sem_post(sem_switch);
                    sem_post(sem_mutex);
                }

                if (files_processed < files_to_process && FD_ISSET(pipe_to_child[i][1], &write_fds))
                {
                    write_to_pipe(pipe_to_child[i][1], argv, &files_processed, files_to_process, 1);
                }

                files_read++;
            }
        }
    }
    if (view_opened == 2)
    {
        sem_wait(sem_mutex);
        sprintf(shm + files_read * info_length, "\t");
        sem_post(sem_mutex);
        sem_post(sem_switch);
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

    sem_close(sem_mutex);
    sem_unlink(SEM_MUTEX_NAME);
    sem_close(sem_switch);
    sem_unlink(SEM_SWITCH_NAME);
    shm_unlink(SHM_NAME);
    close(shm_fd);


    return EXIT_SUCCESS;
}

void setup_pipes_and_forks(int slaves, int pipe_to_child[][2], int pipe_from_child[][2], pid_t pids[], int * shm_fd)
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
            // Proceso hijo
            close(pipe_to_child[i][1]);
            close(pipe_from_child[i][0]);

            dup2(pipe_to_child[i][0], STDIN_FILENO);
            close(pipe_to_child[i][0]);
            close(*shm_fd);

            dup2(pipe_from_child[i][1], STDOUT_FILENO);
            close(pipe_from_child[i][1]);

            char *args[] = {"./slave", NULL};
            execve(args[0], args, NULL);
            perror("execve");
            exit(EXIT_FAILURE);
        }
        // Proceso padre
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

char *read_from_pipe(int fd, char *buff)
{
    int bytes_read = read(fd, buff, 100);
    if (bytes_read < 0)
    {
        perror("read pipe");
        exit(EXIT_FAILURE);
    }
    return buff;
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
