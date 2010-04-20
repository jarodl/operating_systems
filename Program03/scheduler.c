/*
 * =====================================================================================
 *
 *       Filename:  scheduler.c
 *
 *    Description:  A pre-emptive user mode thread scheduler.
 *
 *        Version:  1.0
 *        Created:  04/19/2010 21:47:34
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Jarod Luebbert
 *
 * =====================================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>

#define NTHREADS 5
#define NUM_LOOPS 1000000

pthread_mutex_t condition_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  condition_cond  = PTHREAD_COND_INITIALIZER;

void *child_thread(void *arg);

int main(void) {
    // holds the ids for each thread
    int ids[NTHREADS];

    // holds the system assigned thread ids
    pthread_t threads[NTHREADS];

    // init thread scheduler vector
    int schedule_vector[NTHREADS];
    // give each thread their priority
    schedule_vector[0] = 0;
    schedule_vector[1] = 2;
    schedule_vector[2] = 4;
    schedule_vector[3] = 1;
    schedule_vector[4] = 3;

    int i;
    // create the child threads
    for (i = 0; i < NTHREADS; i++) {
        // set the thread id and pass it as the argument
        ids[i] = i;

        // create the thread
        pthread_create(&threads[i], NULL, child_thread, &ids[i]);
    }

    // start the scheduler
    while (1) {
        // wait one second
        sleep(1);

        // let the next child run
        ;
    }

    return 0;
}

void *child_thread(void *arg) {
    long int i;
    long int counter;
    int myid = *(int*) arg;

    counter = 0;

    for (;;) {
        printf("T: %d runs -t\t\n", myid);

        for (i = 0; i < NUM_LOOPS; i++)
            counter += 1;

        if (counter == 500000)
            counter = 0;
    }
}
