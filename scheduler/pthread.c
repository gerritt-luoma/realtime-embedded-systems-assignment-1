#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>

#include <pthread.h>
#include <sched.h>
#include <time.h>
#include <semaphore.h>

#include <syslog.h>
#include <sys/time.h>
#include <sys/sysinfo.h>
#include <errno.h>

#include <signal.h>

#define NS_PER_MSEC (1000 * 1000)
#define F10_WAIT_NS (10 * NS_PER_MSEC)
#define F20_WAIT_NS (20 * NS_PER_MSEC)

static timer_t scheduler_timer;
static struct itimerspec itime = {{1,0}, {1,0}};
static struct itimerspec last_itime;

static uint64_t seqCnt=0;
static uint64_t sequencePeriods;

typedef struct
{
    int threadIdx;
} threadParams_t;


bool abortTest = false;
bool abort_f10 = false, abort_f20 = false;
// POSIX thread declarations and scheduling attributes
//
pthread_t threads[2];
threadParams_t threadParams[2];
pthread_attr_t threadAttrs[2];
struct sched_param schedparams[2];
sem_t sem_f10, sem_f20;

// Unsafe global
int gsum=0;

void *f10(void *threadp)
{
    struct timespec time_start, time_now;
    uint64_t elapsed_ns = 0;
    double dtime;

    while (!abort_f10)
    {
        sem_wait(&sem_f10);
        elapsed_ns = 0;

        // Get the CPU time consumed by this thread
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &time_start);
        dtime = ((double)(time_start.tv_sec) + ((double)(time_start.tv_nsec) / 1000000000.0));
        printf("[%6.9lf] f10: Task start\n", dtime);
        while (elapsed_ns < F10_WAIT_NS)
        {
            // Update time
            clock_gettime(CLOCK_THREAD_CPUTIME_ID, &time_now);
            elapsed_ns = (time_now.tv_sec - time_start.tv_sec) * 1000000000ULL +
            (time_now.tv_nsec - time_start.tv_nsec);
        }

        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &time_now);
        dtime = ((double)(time_now.tv_sec) + ((double)(time_now.tv_nsec) / 1000000000.0));
        printf("[%6.9lf] f10: Task end\n", dtime);
    }

    pthread_exit((void *)0);
}


void *f20(void *threadp)
{
    struct timespec time_start, time_now;
    uint64_t elapsed_ns = 0;
    double dtime;

    while (!abort_f20)
    {
        sem_wait(&sem_f20);
        elapsed_ns = 0;

        // Get the CPU time consumed by this thread
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &time_start);
        dtime = ((double)(time_start.tv_sec) + ((double)(time_start.tv_nsec) / 1000000000.0));
        printf("[%6.9lf] f20: Task start\n", dtime);
        while (elapsed_ns < F20_WAIT_NS)
        {
            // Update time
            clock_gettime(CLOCK_THREAD_CPUTIME_ID, &time_now);
            elapsed_ns = (time_now.tv_sec - time_start.tv_sec) * 1000000000ULL +
            (time_now.tv_nsec - time_start.tv_nsec);
        }

        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &time_now);
        dtime = ((double)(time_now.tv_sec) + ((double)(time_now.tv_nsec) / 1000000000.0));
        printf("[%6.9lf] f20: Task end\n", dtime);
    }

    pthread_exit((void *)0);
}

void Sequencer(int id)
{
    struct timespec current_time;
    double current_realtime;
    int rc, flags = 0;

    seqCnt++;

    // Timer fires at 100 Hz, f10 period runs at 50 Hz
    if ((seqCnt % 2) == 0) sem_post(&sem_f10);

    // Timer fires at 100 Hz, f20 period runs at 20 Hz
    if ((seqCnt % 5) == 0) sem_post(&sem_f20);

    if (abortTest || (seqCnt >= sequencePeriods))
    {
        // disable interval timer
        itime.it_interval.tv_sec = 0;
        itime.it_interval.tv_nsec = 0;
        itime.it_value.tv_sec = 0;
        itime.it_value.tv_nsec = 0;
        timer_settime(scheduler_timer, flags, &itime, &last_itime);
	    printf("Disabling sequencer interval timer with abort=%d and %llu of %lld\n", abortTest, seqCnt, sequencePeriods);

	    // shutdown all services
        sem_post(&sem_f10);
        sem_post(&sem_f20);

        abort_f10=true;
        abort_f20=true;
    }
}

int main (int argc, char *argv[])
{
   int rc, cpuidx, flags=0;
   int i=0;
   cpu_set_t cpuset;

   // Set the scheduling policy to fifo with the maximum priority
   struct sched_param mainParam;
   mainParam.sched_priority = sched_get_priority_max(SCHED_FIFO);
   if (0 != sched_setscheduler(0, SCHED_FIFO, &mainParam))
   {
       perror("Failed to set scheduler");
   }

   if (sem_init(&sem_f10, 0, 0))
   {
        printf("Failed to init f10 semaphore\n");
        exit(-1);
   }
   else if (sem_init(&sem_f20, 0, 0))
   {
        printf("Failed to init f20 semaphore\n");
        exit(-1);
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
        schedparams[i].sched_priority = (i == 0) ? 80 : 70; // f10 > f20
        pthread_attr_setschedparam(&threadAttrs[i], &schedparams[i]);

        // Create the thread and let it go
        threadParams[i].threadIdx = i;
        pthread_create(&threads[i],                // pointer to thread descriptor
                        &threadAttrs[i],           // use default attributes
                        (i == 0) ? f10 : f20,      // thread function entry point
                        (void *)&(threadParams[i]) // parameters to pass in
                      );
    }

    // Let it run for 20 seconds
    sequencePeriods = 2000;
    // Sequencer = RT_MAX	@ 100 Hz
    //
    /* set up to signal SIGALRM if timer expires */
    timer_create(CLOCK_MONOTONIC_RAW, NULL, &scheduler_timer);

    signal(SIGALRM, (void(*)()) Sequencer);


    /* arm the interval timer */
    itime.it_interval.tv_sec = 0;
    itime.it_interval.tv_nsec = 10000000;
    itime.it_value.tv_sec = 0;
    itime.it_value.tv_nsec = 10000000;

    timer_settime(scheduler_timer, flags, &itime, &last_itime);

    // Wait for both threads to finish
    for(i=0; i<2; i++)
        pthread_join(threads[i], NULL);

   printf("TEST COMPLETE\n");
}
