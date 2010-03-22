/*
 * =====================================================================================
 *
 *       Filename:  racecond.c
 *
 *    Description:  Example of a race condition in C.
 *
 *           Name:  Jarod Luebbert
 *
 * =====================================================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define SHM_SIZE sizeof(long int)
#define NUM_CHILDREN 2
#define ADD 0 /* the pid of the adder */
#define SUB 1 /* the pid of the subtractor */

pid_t r_wait(int *stat_loc);
void adder(long int *n);
void subtractor(long int *n);

int main(void) {
    key_t key;
    int shmid;
    long int* data;

    /* make the key */
    if ((key = ftok("racecond.c", 'R')) == -1) {
        perror("ftok");
        exit(1);
    }
    printf("Successfully generated key: %d\n", key);

    /* connect to a create the shared memory segment */
    if ((shmid = shmget(key, SHM_SIZE, 0644 | IPC_CREAT)) == -1) {
        perror("shmget");
        exit(1);
    }
    printf("Shared memory successfully created with id: %d.\n", shmid);

    /* attach to the segment and get a pointer to it */
    data = shmat(shmid, (void *)0, 0);
    if (data == (long int*)(-1)) {
        perror("shmat");
        exit(1);
    }
    printf("Successfully attached to shared memory with id: %d\n", shmid);
    *data = 0;
    printf("Shared memory value initialized to: %ld\n", *data);

    pid_t pids[NUM_CHILDREN];

    int i;
    for (i = 0; i < 2; i++) {
        if ((pids[i] = fork()) == -1) {
            perror("forking");
            exit(1);
        }
        else if (pids[i] == 0) {
            /* adder */
            if (i == ADD) {
                printf("I am the adder with pid: %d\n", getpid());
                adder(data);
                exit(0);
            }
            /* subtractor */
            else if (i == SUB) {
                printf("I am the subtractor with pid: %d\n", getpid());
                subtractor(data);
                exit(0);
            }
        }
    }

    /* wait for all of the child processes */
    while (r_wait(NULL) > 0) ;

    printf("The shared memory value is now: %ld\n", *data);

    /* detach from the shared memory segment */
    if (shmdt(data) == -1) {
        perror("shmdt");
        exit(1);
    }
    printf("Detached from shared memory segment with id: %d\n", shmid);

    /* remove the shared memory segment */
    if ((shmctl(shmid, IPC_RMID, NULL)) == -1) {
        perror("shmctl");
        exit(1);
    }
    printf("Removed shared memory segment with id: %d\n", shmid);

    return 0;
}

/* function that restarts wait if interrupted by a signal */
pid_t r_wait(int *stat_loc) {
    int retval;

    while (((retval = wait(stat_loc)) == -1) && (errno == EINTR));
    return retval;
}

void adder(long int *n) {
    int i, j;
    for (i = 0; i < 600000; i++) {
        for (j = 0; j < 3; j++) {
            *n = *n + 1;
        }
    }
}

void subtractor(long int *n) {
    int i, j;
    for (i = 0; i < 600000; i++) {
        for (j = 0; j < 3; j++) {
            *n = *n - 1;
        }
    }
}
