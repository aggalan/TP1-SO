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

void startResources(memoryADT *adt, char *shm_name, char *sem_mutex_name, char *sem_switch_name, size_t size)
{
    initializeResources(adt, shm_name, sem_mutex_name, sem_switch_name, size);
    unlinkResources(adt);
    createResources(adt);
}

void initializeResources(memoryADT *adt, char *shm_name, char *sem_mutex_name, char *sem_switch_name, size_t size)
{
    adt->shm_name = shm_name;
    adt->sem_mutex_name = sem_mutex_name;
    adt->sem_switch_name = sem_switch_name;
    adt->size = size;
}

void createResources(memoryADT *adt)
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
        printf("NO VIEW OPENED\n");
    }

    if ((ftruncate(adt->shm_fd, adt->size)) < 0)
    {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }
}

void openResources(memoryADT *adt)
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

    printf("VIEW OPENED\n");
}

void unlinkResources(memoryADT *adt)
{
    sem_unlink(adt->sem_mutex_name);
    sem_unlink(adt->sem_switch_name);

}

void closeResources(memoryADT *adt)
{
    sem_close(adt->sem_mutex);
    sem_close(adt->sem_switch);
    close(adt->shm_fd);
}

void readMemory(memoryADT *adt, int *index, int *status)
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
