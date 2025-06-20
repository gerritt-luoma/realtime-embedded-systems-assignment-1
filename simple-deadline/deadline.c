// https://www.i-programmer.info/programming/cc/13002-applying-c-deadline-scheduling.html?start=1
//

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <linux/sched.h>
#include <pthread.h>
#include <stdint.h>
#include <time.h>

#include <unistd.h>
#include <sys/syscall.h>

#define CURRENT_POLICY (SCHED_DEADLINE)
// #define CURRENT_POLICY (SCHED_FIFO)

struct sched_attr 
{
    uint32_t size;
    uint32_t sched_policy;
    uint64_t sched_flags;
    int32_t sched_nice;
    uint32_t sched_priority;
    uint64_t sched_runtime;
    uint64_t sched_deadline;
    uint64_t sched_period;
};

int sched_setattr(pid_t pid, const struct sched_attr *attr, unsigned int flags) 
{
    return syscall(__NR_sched_setattr, pid, attr, flags);
}

void * threadA(void *p) 
{
    struct timespec time_now;
    double dtime;
    struct sched_attr attr = 
    {
        .size = sizeof (attr),
        .sched_policy = CURRENT_POLICY,
        .sched_runtime = 10 * 1000 * 1000,
        .sched_period = 2 * 1000 * 1000 * 1000,
        .sched_deadline = 11 * 1000 * 1000
    };

    sched_setattr(0, &attr, 0);

    for (;;) 
    {
        clock_gettime(CLOCK_MONOTONIC, &time_now);
        dtime = ((double)(time_now.tv_sec) + ((double)(time_now.tv_nsec) / 1000000000.0));
        printf("Current monotonic time: %6.9lf\n", dtime);
        fflush(0);
        sched_yield();
    };
}


int main(int argc, char** argv) 
{
    pthread_t pthreadA;
    pthread_create(&pthreadA, NULL, threadA, NULL);
    pthread_exit(0);
    return (EXIT_SUCCESS);
}
