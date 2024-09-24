#include "commons.h"
#define OFFSET 3
#include "memory.h"

int main()
{
    char path[MAX_PATH] = {0};
    char *md5sum = "md5sum %s";
    char command[MAX_PATH + 100];
    char md5[MAX_MD5 + MAX_PATH + OFFSET];
    int bytes_read = 0;

    while ((bytes_read = pipe_read(STDIN_FILENO, path)) > 0)
    {
        if (path[0] == 0)
        {
            break;
        }

        int i = 0;
        char *current = path;

        while (i < bytes_read)
        {
            if (path[i] == '\0')
            {
                snprintf(command, sizeof(command), md5sum, current);

                FILE *log = popen(command, "r");
                if (log == NULL)
                {
                    perror("popen");
                    exit(EXIT_FAILURE);
                }

                if (fgets(md5, sizeof(md5), log) != NULL)
                {
                    write(STDOUT_FILENO, md5, strlen(md5));
                }

                pclose(log);
                current = path + i + 1;
            }
            i++;
        }
    }
    close(STDOUT_FILENO);
    exit(EXIT_SUCCESS);
}
