/****************************************************************************/
/* Function: nanosleep and POSIX 1003.1b RT clock demonstration             */
/*                                                                          */
/* Sam Siewert - 02/05/2011                                                 */
/*                                                                          */
/****************************************************************************/

#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

/* Defines used for requesting the duration of a sleep based off the timespec nsec field */
#define NSEC_PER_SEC (1000000000)
#define NSEC_PER_MSEC (1000000)
#define NSEC_PER_USEC (1000)

/* Common error and success codes */
#define ERROR (-1)
#define OK (0)

/* The requested amount of time to sleep each test iteration */
#define TEST_SECONDS (0)
/* 10ms in nanoseconds */
#define TEST_NANOSECONDS (NSEC_PER_MSEC * 10)

/* Helper function used to display statistics of each test sleep iteration */
void end_delay_test(void);

/* Static variables to track the requested, actual, and remaining sleep time */
static struct timespec sleep_time = {0, 0};
static struct timespec sleep_requested = {0, 0};
static struct timespec remaining_time = {0, 0};

/* Variable used for tracking the number of failed sleeps per test iteration */
static unsigned int sleep_count = 0;

/* Variables for configuring the main scheduler and retrieving the priorities */
pthread_t main_thread;
pthread_attr_t main_sched_attr;
int rt_max_prio, rt_min_prio, min;
struct sched_param main_param;

/**
 * @brief Prints the current scheduling policy of the calling process.
 *
 * This function retrieves the scheduling policy of the process identified
 * by its process ID (`getpid()`) and prints a human-readable description
 * to standard output.
 */
void print_scheduler(void)
{
   int schedType;

   schedType = sched_getscheduler(getpid());

   switch(schedType)
   {
      case SCHED_FIFO:
         printf("Pthread Policy is SCHED_FIFO\n");
         break;
      case SCHED_OTHER:
         printf("Pthread Policy is SCHED_OTHER\n");
         break;
      case SCHED_RR:
         printf("Pthread Policy is SCHED_RR\n");
         break;
      default:
         printf("Pthread Policy is UNKNOWN\n");
   }
}

/**
 * @brief Computes the elapsed time between two timespec timestamps in seconds.
 *
 * This function takes two `struct timespec` pointers representing the start
 * and stop times, converts them into floating-point seconds, and returns
 * the difference as a `double`. The result is the duration in seconds between
 * `fstart` and `fstop`.
 *
 * @param fstart Pointer to the start time (`struct timespec`).
 * @param fstop Pointer to the stop time (`struct timespec`).
 * @return Elapsed time in seconds as a `double`.
 *
 */
double d_ftime(struct timespec *fstart, struct timespec *fstop)
{
   /* Convert start and stop times to seconds as doubles */
   double dfstart = ((double)(fstart->tv_sec) + ((double)(fstart->tv_nsec) / 1000000000.0));
   double dfstop = ((double)(fstop->tv_sec) + ((double)(fstop->tv_nsec) / 1000000000.0));

   return(dfstop - dfstart);
}

/**
 * @brief Calculates the time difference between two timespec timestamps.
 *
 * This function computes the difference between `stop` and `start` times
 * and stores the result in the `delta_t` structure. It handles various edge cases
 * such as nanosecond roll-over and overflow, and ensures the result is properly
 * normalized.
 *
 * @param stop Pointer to the stop time (`struct timespec`).
 * @param start Pointer to the start time (`struct timespec`).
 * @param delta_t Pointer to the structure where the computed delta time will be stored.
 *
 * @return
 * - `OK` if the calculation is successful.
 * - `ERROR` if the stop time is earlier than the start time.
 */
int delta_t(struct timespec *stop, struct timespec *start, struct timespec *delta_t)
{
   int dt_sec=stop->tv_sec - start->tv_sec;
   int dt_nsec=stop->tv_nsec - start->tv_nsec;

   // case 1 - less than a second of change
   if(dt_sec == 0)
   {
      //printf("dt less than 1 second\n");

      if(dt_nsec >= 0 && dt_nsec < NSEC_PER_SEC)
      {
         //printf("nanosec greater at stop than start\n");
         delta_t->tv_sec = 0;
         delta_t->tv_nsec = dt_nsec;
      }

      else if(dt_nsec > NSEC_PER_SEC)
      {
         //printf("nanosec overflow\n");
         delta_t->tv_sec = 1;
         delta_t->tv_nsec = dt_nsec-NSEC_PER_SEC;
      }

      else // dt_nsec < 0 means stop is earlier than start
      {
         printf("stop is earlier than start\n");
         return(ERROR);
      }
   }

   // case 2 - more than a second of change, check for roll-over
   else if(dt_sec > 0)
   {
      if(dt_nsec >= 0 && dt_nsec < NSEC_PER_SEC)
      {
         //printf("nanosec greater at stop than start\n");
         delta_t->tv_sec = dt_sec;
         delta_t->tv_nsec = dt_nsec;
      }

      else if(dt_nsec > NSEC_PER_SEC)
      {
         //printf("nanosec overflow\n");
         delta_t->tv_sec = delta_t->tv_sec + 1;
         delta_t->tv_nsec = dt_nsec-NSEC_PER_SEC;
      }

      else // dt_nsec < 0 means roll over
      {
         //printf("nanosec roll over\n");
         delta_t->tv_sec = dt_sec-1;
         delta_t->tv_nsec = NSEC_PER_SEC + dt_nsec;
      }
   }

   return(OK);
}

/* Static variables used for tracking start, stop, delta, and error times */
static struct timespec rtclk_dt = {0, 0};
static struct timespec rtclk_start_time = {0, 0};
static struct timespec rtclk_stop_time = {0, 0};
static struct timespec delay_error = {0, 0};

//#define MY_CLOCK CLOCK_REALTIME
//#define MY_CLOCK CLOCK_MONOTONIC
#define MY_CLOCK CLOCK_MONOTONIC_RAW
//#define MY_CLOCK CLOCK_REALTIME_COARSE
//#define MY_CLOCK CLOCK_MONOTONIC_COARSE

#define TEST_ITERATIONS (100)

/**
 * @brief Performs a delay accuracy test using `nanosleep()` and POSIX clocks.
 *
 * This function is intended to run as a POSIX thread and evaluates how closely
 * the system's sleep duration matches the requested duration using high-resolution
 * clock measurements. It uses `nanosleep()` in a loop to handle interruptions
 * and measures the actual delay vs. requested delay across multiple iterations.
 *
 * @param threadID A pointer to the thread identifier (unused in this function).
 *
 * @return Always returns `NULL`. Intended to be used with `pthread_create()`.
 */
void *delay_test(void *threadID)
{
   int idx, rc;
   unsigned int max_sleep_calls=3;
   int flags = 0;
   struct timespec rtclk_resolution;

   sleep_count = 0;

   if(clock_getres(MY_CLOCK, &rtclk_resolution) == ERROR)
   {
      perror("clock_getres");
      exit(-1);
   }
   else
   {
      printf("\n\nPOSIX Clock demo using system RT clock with resolution:\n\t%ld secs, %ld microsecs, %ld nanosecs\n", rtclk_resolution.tv_sec, (rtclk_resolution.tv_nsec/1000), rtclk_resolution.tv_nsec);
   }

   for(idx=0; idx < TEST_ITERATIONS; idx++)
   {
      printf("test %d\n", idx);

      /* run test for defined seconds */
      sleep_time.tv_sec=TEST_SECONDS;
      sleep_time.tv_nsec=TEST_NANOSECONDS;
      sleep_requested.tv_sec=sleep_time.tv_sec;
      sleep_requested.tv_nsec=sleep_time.tv_nsec;

      /* start time stamp */
      clock_gettime(MY_CLOCK, &rtclk_start_time);

      /* request sleep time and repeat if time remains */
      do
      {
         if(rc=nanosleep(&sleep_time, &remaining_time) == 0) break;

         sleep_time.tv_sec = remaining_time.tv_sec;
         sleep_time.tv_nsec = remaining_time.tv_nsec;
         sleep_count++;
      }
      while (((remaining_time.tv_sec > 0) || (remaining_time.tv_nsec > 0))
            && (sleep_count < max_sleep_calls));

      clock_gettime(MY_CLOCK, &rtclk_stop_time);

      delta_t(&rtclk_stop_time, &rtclk_start_time, &rtclk_dt);
      delta_t(&rtclk_dt, &sleep_requested, &delay_error);

      end_delay_test();
   }
}

/**
 * @brief Finalizes and reports the results of a delay test.
 *
 * This function computes and prints timing information gathered during a delay
 * test using a POSIX clock (`MY_CLOCK`). It reports:
 * - The elapsed (actual) time between the start and stop timestamps.
 * - The delta time (`rtclk_dt`) in seconds, milliseconds, microseconds, and nanoseconds.
 * - The total elapsed time as a floating-point value in seconds.
 * - The delay error (difference between requested and actual sleep time).
 */
void end_delay_test(void)
{
   double real_dt;
#if 0
  printf("MY_CLOCK start seconds = %ld, nanoseconds = %ld\n",
         rtclk_start_time.tv_sec, rtclk_start_time.tv_nsec);

  printf("MY_CLOCK clock stop seconds = %ld, nanoseconds = %ld\n",
         rtclk_stop_time.tv_sec, rtclk_stop_time.tv_nsec);
#endif

   real_dt=d_ftime(&rtclk_start_time, &rtclk_stop_time);
   printf("MY_CLOCK clock DT seconds = %ld, msec=%ld, usec=%ld, nsec=%ld, sec=%6.9lf\n",
         rtclk_dt.tv_sec, rtclk_dt.tv_nsec/1000000, rtclk_dt.tv_nsec/1000, rtclk_dt.tv_nsec, real_dt);

#if 0
  printf("Requested sleep seconds = %ld, nanoseconds = %ld\n",
         sleep_requested.tv_sec, sleep_requested.tv_nsec);

  printf("\n");
  printf("Sleep loop count = %ld\n", sleep_count);
#endif
   printf("MY_CLOCK delay error = %ld, nanoseconds = %ld\n",
         delay_error.tv_sec, delay_error.tv_nsec);
}

#define RUN_RT_THREAD

void main(void)
{
   int rc, scope;

   printf("Before adjustments to scheduling policy:\n");
   print_scheduler();

#ifdef RUN_RT_THREAD
   pthread_attr_init(&main_sched_attr);
   pthread_attr_setinheritsched(&main_sched_attr, PTHREAD_EXPLICIT_SCHED);
   pthread_attr_setschedpolicy(&main_sched_attr, SCHED_FIFO);

   rt_max_prio = sched_get_priority_max(SCHED_FIFO);
   rt_min_prio = sched_get_priority_min(SCHED_FIFO);

   main_param.sched_priority = rt_max_prio;
   rc=sched_setscheduler(getpid(), SCHED_FIFO, &main_param);


   if (rc)
   {
      printf("ERROR; sched_setscheduler rc is %d\n", rc);
      perror("sched_setschduler"); exit(-1);
   }

   printf("After adjustments to scheduling policy:\n");
   print_scheduler();

   main_param.sched_priority = rt_max_prio;
   pthread_attr_setschedparam(&main_sched_attr, &main_param);

   rc = pthread_create(&main_thread, &main_sched_attr, delay_test, (void *)0);

   if (rc)
   {
      printf("ERROR; pthread_create() rc is %d\n", rc);
      perror("pthread_create");
      exit(-1);
   }

   pthread_join(main_thread, NULL);

   if(pthread_attr_destroy(&main_sched_attr) != 0)
      perror("attr destroy");
#else
   delay_test((void *)0);
#endif

   printf("TEST COMPLETE\n");
}

