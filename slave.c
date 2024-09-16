// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include "commons.h"
#define OFFSET 3
#include "memory.h"

int main()
{

    char path[MAX_PATH] = {0};
    char *md5sum = "md5sum %s";
    char command[MAX_PATH + strlen(md5sum)];
    char md5[MAX_MD5 + MAX_PATH + OFFSET];

    while (pipe_read(STDIN_FILENO, path) > 0)
    {

        if (path[0] == 0)
        {
            break;
        }

        sprintf(command, md5sum, path);

        FILE *log = popen(command, "r");

        if (log == NULL)
        {
            perror("peopen");
            exit(EXIT_FAILURE);
        }

        fgets(md5, MAX_MD5 + strlen(path), log);

        pclose(log);

        write(STDOUT_FILENO, md5, strlen(md5) + 1);
    }
    close(STDOUT_FILENO);
    exit(EXIT_SUCCESS);
}
