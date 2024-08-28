#include "commons.h"
#include <sys/wait.h>
#include <sys/select.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILES_PER_SLAVE 2

struct file_info {
    pid_t pid;
    char md5[100];
    char filename[100];
};


void write_to_pipe(int fd, char ** argv, int *files_processed, int total_files, int qty) {
    for (int i = 0; i < qty; i++) {
        if ((*files_processed) < total_files) {
            int bytes_written = write(fd, argv[*files_processed + 1], strlen(argv[(*files_processed) + 1]) + 1);
            if (bytes_written < 0) {
                perror("write pipe");
                exit(EXIT_FAILURE);
            }
            (*files_processed)++;
        } else {
            break;
        }
    }
}

char * read_from_pipe(int fd, char * buff) {
    int bytes_read = read(fd, buff, 100);
    if (bytes_read < 0) {
        perror("read pipe");
        exit(EXIT_FAILURE);
    }
    return buff;
}


int main(int argc, char *argv[]) {
    int slaves = 2;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file_path>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int files_processed = 0, files_read = 0;
    int max_fd = 0;

    const char *filename = "results.txt";
    FILE *file = fopen(filename, "w");

    if (file == NULL) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    int pipe_to_child[slaves][2], pipe_from_child[slaves][2];
    pid_t pids[slaves];

    for (int i = 0; i < slaves; i++) { 
        if (pipe(pipe_to_child[i]) == -1 || pipe(pipe_from_child[i]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < slaves; i++) {
        if ((pids[i] = fork()) == 0) {
            // Child process
            close(pipe_to_child[i][1]);
            close(pipe_from_child[i][0]);

            dup2(pipe_to_child[i][0], STDIN_FILENO);
            close(pipe_to_child[i][0]);

            dup2(pipe_from_child[i][1], STDOUT_FILENO);
            close(pipe_from_child[i][1]);

            char *args[] = {"./slave", NULL};
            execve(args[0], args, NULL);
            perror("execve");
            exit(EXIT_FAILURE);
        }
        // Parent process
        close(pipe_to_child[i][0]); 
        close(pipe_from_child[i][1]); 
    }




    // Initial file assignment
    for (int i = 0; i < slaves; i++) {
        write_to_pipe(pipe_to_child[i][1], argv, &files_processed ,argc-1, FILES_PER_SLAVE);
    }

    

    while (files_read < argc - 1) {
    fd_set read_fds, write_fds;
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);
        max_fd = 0;

        for (int i = 0; i < slaves; i++) {
            FD_SET(pipe_from_child[i][0], &read_fds);
            FD_SET(pipe_to_child[i][1], &write_fds);

            if (pipe_from_child[i][0] > max_fd) {
                max_fd = pipe_from_child[i][0];
            }
            if (pipe_to_child[i][1] > max_fd) {
                max_fd = pipe_to_child[i][1];
            }
        }

        int activity = select(max_fd + 1, &read_fds, &write_fds, NULL, NULL);

        if (activity == -1) {
            perror("select");
            exit(EXIT_FAILURE);
        } else if (activity > 0) {
            for (int i = 0; i < slaves; i++) {
                if (FD_ISSET(pipe_from_child[i][0], &read_fds)) {
                    char buff[100];
                    fprintf(file, "PID: %d HASH: %s\n", pids[i], read_from_pipe(pipe_from_child[i][0], buff));
                    files_read++;
                    if (FD_ISSET(pipe_to_child[i][1], &write_fds) ) {
                        if (files_processed < argc - 1) {
                            write_to_pipe(pipe_to_child[i][1], argv, &files_processed ,argc-1, 1);
                        }
                    }
                }
            }
        }
    }

    for (int i = 0; i < slaves; i++) {
        close(pipe_to_child[i][1]);
        close(pipe_from_child[i][0]);
    }

    if (fclose(file) != 0) {
        perror("Error closing file");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
