#define _GNU_SOURCE
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <sched.h>

#define COUNT  1000

typedef struct
{
    int threadIdx;
} threadParams_t;


// POSIX thread declarations and scheduling attributes
//
pthread_t threads[2];
threadParams_t threadParams[2];
pthread_attr_t threadAttrs[2];
struct sched_param schedparams[2];


// Unsafe global
int gsum=0;

void *incThread(void *threadp)
{
    int i;
    threadParams_t *threadParams = (threadParams_t *)threadp;

    for(i=0; i<COUNT; i++)
    {
        gsum=gsum+i;
        printf("Increment thread idx=%d, gsum=%d\n", threadParams->threadIdx, gsum);
    }
}


void *decThread(void *threadp)
{
    int i;
    threadParams_t *threadParams = (threadParams_t *)threadp;

    for(i=0; i<COUNT; i++)
    {
        gsum=gsum-i;
        printf("Decrement thread idx=%d, gsum=%d\n", threadParams->threadIdx, gsum);
    }
}

int main (int argc, char *argv[])
{
   int rc, cpuidx;
   int i=0;
   cpu_set_t cpuset;

   // Set the scheduling policy to fifo with the maximum priority
   struct sched_param mainParam;
   mainParam.sched_priority = sched_get_priority_max(SCHED_FIFO);
   if (0 != sched_setscheduler(0, SCHED_FIFO, &mainParam))
   {
       perror("Failed to set scheduler");
   }

    // Initialize attributes and set SCHED_FIFO with priorities
    for(i = 0; i < 2; i++)
    {
        // Set attrs to be explicitly used with a fifo policy
        pthread_attr_init(&threadAttrs[i]);
        pthread_attr_setinheritsched(&threadAttrs[i], PTHREAD_EXPLICIT_SCHED);
        pthread_attr_setschedpolicy(&threadAttrs[i], SCHED_FIFO);

        // Force the threads to run on a single core so they can't run at the same time
        // on different cores
        CPU_ZERO(&cpuset);
        cpuidx=(3);
        CPU_SET(cpuidx, &cpuset);
        pthread_attr_setaffinity_np(&threadAttrs[i], sizeof(cpu_set_t), &cpuset);

        // Select the priority for the thread.
        // Give the inc thread a higher prio so it always goes first
        schedparams[i].sched_priority = (i == 0) ? 80 : 70; // incThread > decThread
        pthread_attr_setschedparam(&threadAttrs[i], &schedparams[i]);

        // Create the thread and let it go
        threadParams[i].threadIdx = i;
        pthread_create(&threads[i],                       // pointer to thread descriptor
                        &threadAttrs[i],                  // use default attributes
                        (i == 0) ? incThread : decThread, // thread function entry point
                        (void *)&(threadParams[i])        // parameters to pass in
                      );
    }

    // Wait for both threads to finish
    for(i=0; i<2; i++)
        pthread_join(threads[i], NULL);

   printf("TEST COMPLETE\n");
}
