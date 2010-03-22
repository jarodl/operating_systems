/*
 * =====================================================================================
 *
 *       Filename:  semaphore.c
 *
 *    Description:  Example of using a semaphore to prevent a
 *                  race condition in C.
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
#include <sys/sem.h>

#define SHM_SIZE sizeof(long int) /* size for shared memory */
#define NUM_CHILDREN 2
#define ADD 0 /* the pid of the adder */
#define SUB 1 /* the pid of the subtractor */
#define MAX_RETRIES 10

pid_t r_wait(int *stat_loc);

void adder(long int *n);
void subtractor(long int *n);

/* semaphore operations */
int initsem(key_t key);
void lock(int semid, struct sembuf *sb);
void unlock(int semid, struct sembuf *sb);

int main(void) {
    key_t key;

    int semid; /* id of the semaphore */
    struct sembuf sb;

    sb.sem_num = 0;
    sb.sem_op = -1; /* set to allocate resource */
    sb.sem_flg = SEM_UNDO;

    int shmid; /* id of the shared memory segment */
    long int* data; /* shared variable */

    /* make the key */
    if ((key = ftok("semaphore.c", 'R')) == -1) {
        perror("ftok");
        exit(1);
    }
    printf("Successfully generated key: %d\n\n", key);

    /* connect to or create the shared memory segment */
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
    printf("Shared memory value initialized to: %ld\n\n", *data);

    /* grab the semaphore created by seminit() */
    if ((semid = initsem(key)) == -1) {
        perror("initsem");
        exit(1);
    }
    printf("Successfully created a new semaphore set with id: %d\n\n", semid);


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

                lock(semid, &sb);
                printf("The adder locked the semaphore with id: %d\n", semid);
                adder(data);
                unlock(semid, &sb);
                printf("The adder unlocked the semaphore with id: %d\n", semid);

                printf("The adder (child process 1) terminated.\n\n");
                exit(0);
            }
            /* subtractor */
            else if (i == SUB) {
                printf("I am the subtractor with pid: %d\n", getpid());

                lock(semid, &sb);
                printf("The subtractor locked the semaphore with id: %d\n", semid);
                subtractor(data);
                unlock(semid, &sb);
                printf("The subtractor unlocked the semaphore with id: %d\n", semid);

                printf("The subtractor (child process 2) terminated.\n\n");
                exit(0);
            }
        }
    }

    /* wait for all of the child processes */
    while (r_wait(NULL) > 0) ;

    /* delete the semaphore */
    if ((semctl(semid, 0, IPC_RMID, NULL)) == -1) {
        perror("semctl");
        exit(1);
    }
    printf("Removed semaphore with id: %d\n\n", semid);

    printf("The shared memory value is now: %ld\n\n", *data);

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
    printf("Removed shared memory segment with id: %d\n\n", shmid);

    printf("The parent process is now terminating.\n");

    return 0;
}

/*
 * initsem() -- inspired by W. Richard Stevens' UNIX Network
 * Programming 2nd edition, volume 2, lockvsem.c, page 295.
 * Referenced from Beej's guide to IPC.
 *
 * Initializes a single semaphore, returning the id.
*/
int initsem(key_t key) { /* key from ftok() */
    int i;
    union semun arg;
    struct semid_ds buf;
    struct sembuf sb;
    int semid;

    semid = semget(key, 1, IPC_CREAT | IPC_EXCL | 0666);

    if (semid >= 0) { /* we got it first */
        sb.sem_op = 1;
        sb.sem_flg = 0;
        arg.val = 1;

        /* do a semop() to "free" the semaphores. */
        /* this sets the sem_otime field, as needed below. */
        if (semop(semid, &sb, 1) == -1) {
            int e = errno;
            semctl(semid, 0, IPC_RMID); /* clean up */
            errno = e;
            return -1; /* error, check errno */
        }
    }
    else if (errno = EEXIST) { /* someone else got it first */
        int ready = 0;

        semid = semget(key, 1, 0); /* get the id */
        if (semid < 0) return semid; /* error, check errno */

        /* wait for other process to initialize the semaphore */
        arg.buf = &buf;
        for (i =0; i < MAX_RETRIES && !ready; i++) {
            semctl(semid, 0, IPC_STAT, arg);
            if (arg.buf->sem_otime != 0) {
                ready = 1;
            }
            else {
                sleep(1);
            }
        }
        if (!ready) {
            errno = ETIME;
            return -1;
        }
    }
    else {
        return semid; /* error, check errno */
    }

    return semid;
}

/* function that restarts wait if interrupted by a signal */
pid_t r_wait(int *stat_loc) {
    int retval;

    while (((retval = wait(stat_loc)) == -1) && (errno == EINTR));
    return retval;
}

/* adder() - increments an integer over a million times */
void adder(long int *n) {
    int i, j;
    for (i = 0; i < 600000; i++) {
        for (j = 0; j < 3; j++) {
            *n = *n + 1;
        }
    }
}

/* subtractor() - decrements an integer over a million times */
void subtractor(long int *n) {
    int i, j;
    for (i = 0; i < 600000; i++) {
        for (j = 0; j < 3; j++) {
            *n = *n - 1;
        }
    }
}

/* lock() - locks the semaphore, blocking other processes */
void lock(int semid, struct sembuf *sb) {
    /* lock the semaphore */
    if (semop(semid, sb, 1) == -1) {
        perror("semop");
        exit(1);
    }
}

/* unlock() - unlocks the semaphore, allowing access by other processes */
void unlock(int semid, struct sembuf *sb) {
    /* unlock the semaphore */
    sb->sem_op = 1; /* free resource */
    if (semop(semid, sb, 1) == -1) {
        perror("semop");
        exit(1);
    }
}
