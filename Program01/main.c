/*
 * name: Jarod Luebbert
 * desc: Forking processes in C
 * date: February 3rd, 2010
 *
 * */
#include <stdio.h> /* printf, stderr, fprintf */
#include <stdlib.h> /*  exit */
#include <unistd.h> /* _exit, fork */
#include <errno.h> /* errno */
#include <sys/wait.h> /* wait */

#define NUM_CHILDREN 3

pid_t r_wait(int *stat_loc);

int main(void) {
    int i;
    int n = NUM_CHILDREN;
    pid_t pids[NUM_CHILDREN];

    printf("this is the parent process, my PID is: %d\n", getpid());

    /* create the child processes */
    for (i = 0; i < n; i++) {
        /* if the process cannot be forked */
        if ((pids[i] = fork()) < 0) {
            fprintf(stderr, "can't fork, error %d\n", errno);
            abort();
            exit(1);
        }
        /* make sure the process is a child */
        else if (pids[i] == 0) {
            /* make sure it's the first child */
            if (i == 0) {
                printf("this is the first child process, my PID is: %d\n",
                        getpid());
                execl("/bin/ls", "ls", "-l", (char *)0);
                fprintf(stderr, "first child failed to execute ls, error %d\n", 
                        errno);
                exit(1);
            }
            /* make sure it's the second child */
            else if (i == 1) {
                printf("this is the second child process, my PID is: %d\n",
                        getpid());
                execl("/bin/ps", "ps", "-lf", (char *)0);
                fprintf(stderr, "second child failed to execute ps, error %d\n",
                        errno);
                exit(1);
            }
            /* make sure it's the third child */
            else if (i == 2) {
                int j;
                for (j = 0; j < 4; j++) {
                    printf("this is the third child process, my PID is: %d\n",
                            getpid());
                }
                exit(0);
            }
        }
        else {
        }
    }

    /* wait for all of the child processes */
    while (r_wait(NULL) > 0) ;
    printf("this is the parent process after my child processes with PIDS of %d, %d, and %d are terminated, the main process is terminated!\n",
            pids[0], pids[1], pids[2]);

    return 0;
}

/* function that restarts wait if interrupted by a signal */
pid_t r_wait(int *stat_loc) {
    int retval;

    while (((retval = wait(stat_loc)) == -1) && (errno == EINTR));
    return retval;
}
