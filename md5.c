#include "commons.h"
#include <sys/wait.h>
#include <sys/select.h>
#include <errno.h>

#define FILES_PER_SLAVE 1

int main(int argc, const char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file_path>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int slaves = ((argc - 1) / 20) > 0 ? (argc - 1) / 20 : 1;
    int files_processed = 0;
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

        if ((pids[i] = fork()) == 0) {
            // Child process
            close(pipe_to_child[i][1]);
            close(pipe_from_child[i][0]);

            dup2(pipe_to_child[i][0], STDIN_FILENO);
            close(pipe_to_child[i][0]);

            //dup2(pipe_from_child[i][1], STDOUT_FILENO);
            close(pipe_from_child[i][1]);

            char *args[] = {"./slave", NULL};
            execve(args[0], args, NULL);
            perror("execve");
            exit(EXIT_FAILURE);
        }
        // Parent process
       close(pipe_to_child[i][0]); // Close read end

       
        if (pipe_to_child[i][1] > max_fd) {
            max_fd = pipe_to_child[i][1];
        }
        if (pipe_from_child[i][0] > max_fd) {
            max_fd = pipe_from_child[i][0];
        }
    }

    fd_set read_fds, write_fds;

    while (files_processed < argc - 1) {
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);

        for (int i = 0; i < slaves; i++) {
            FD_SET(pipe_to_child[i][1], &write_fds);
            FD_SET(pipe_from_child[i][0], &read_fds);
        }

        int activity = select(max_fd + 1, &read_fds, &write_fds, NULL, NULL);

        if (activity < 0) {
            perror("select");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < slaves; i++) {
            if (FD_ISSET(pipe_to_child[i][1], &write_fds)) {
                if (files_processed < argc - 1) {
                    ssize_t bytes_written = write(pipe_to_child[i][1], argv[files_processed + 1], strlen(argv[files_processed + 1]) + 1);
                    if (bytes_written == -1) {
                        if (errno == EPIPE) {
                            // Handle broken pipe error
                            fprintf(stderr, "Broken pipe detected\n");
                            continue;
                        }
                        perror("write");
                        exit(EXIT_FAILURE);
                    }
                    files_processed++;
                }
            }

            if (FD_ISSET(pipe_from_child[i][0], &read_fds)) {
                char buffer[256];
                ssize_t bytes_read = read(pipe_from_child[i][0], buffer, sizeof(buffer) - 1);
                if (bytes_read > 0) {
                    // Ensure null-termination
                    buffer[bytes_read] = '\0';
                    if (fprintf(file, "%s\n", buffer) < 0) {
                        perror("Error writing to file");
                        fclose(file);
                        exit(EXIT_FAILURE);
                    }

                    if (files_processed < argc - 1) {
                        ssize_t bytes_written = write(pipe_to_child[i][1], argv[files_processed + 1], strlen(argv[files_processed + 1]) + 1);
                        if (bytes_written == -1) {
                            if (errno == EPIPE) {
                                // Handle broken pipe error
                                fprintf(stderr, "Broken pipe detected\n");
                                continue;
                            }
                            perror("write");
                            exit(EXIT_FAILURE);
                        }
                        files_processed++;
                    }
                } else if (bytes_read == 0) {
                    // EOF or no data available
                    continue;
                } else {
                    perror("read");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }

    // Close all remaining pipes
    for (int i = 0; i < slaves; i++) {
        close(pipe_to_child[i][1]);
        close(pipe_from_child[i][0]);
    }

    if (fclose(file) != 0) {
        perror("Error closing file");
        return EXIT_FAILURE;
    }
    return 0;
}
