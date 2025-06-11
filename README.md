# RTES Assignment 1
Repository for the first assignment of CU Boulder's Real Time Embedded Systems course


## Additional Sources
- [Geeks for Geeks RMS](https://www.geeksforgeeks.org/rate-monotonic-scheduling/)

## Problem 1

> The Rate Monotonic Policy states that services which share a CPU core should multiplex it (with context switches that preempt and dispatch tasks) based on priority, where highest priority is assigned to the most frequently requested service and lowest priority is assigned to the least frequently requested AND total shared CPU core utilization must preserve some margin (not be fully utilized or overloaded).


> A: Draw a timing diagram for three services S1, S2, and S3 with T1=3, C1=1, T2=5, C2=2, T3=15, C3=3 where all times are in milliseconds.

> B: Label your diagram carefully and describe whether you think the schedule is feasible (mathematically repeatable as an invariant indefinitely) and safe (unlikely to ever miss a deadline).

> C: What is the total CPU utilization by the three services?

A: Below is the diagram of services S1, S2, and S3 with their defined periods and durations.  The raw spreadsheet used to generate this screenshot can be found at [assets/problem-1-diagram.xlsx](./assets/problem-1-diagram.xlsx)

<p align="center">
    <img src="./assets/timing-diagram-prob-1.png" />
</p>

B: In the image, each service has two rows.  The first row, labelled with just the service name, shows the currently active service based off the time step in milliseconds.  The second row, labelled with the service name + Deadline, graphically shows the deadline iterations of the service shown with `< - >`.  If there is no active service at a given time, the column is grayed out and is labelled as `Slack`.  Additionally, the period, service duration, and CPU utilization are all provided above the diagram along with the calulated Least Upper Bound (LUB) and CPU Utilization (U).

Purely based off visual analysis I believe this schedule of services is repeatable and safe.  Due to the services having harmonic frequencies and durations, the services are able to perform a repeatable pattern throughout each service set.  Additional mathematical analysis will be performed in part C.

C: To calculate the total CPU usage you simply sum up the quotient of the service computation times to their periods.  This is defined below:

$$
U = \sum_{i=1}^{n} \frac{C_i}{T_i}
$$

Where:
- \( C_i \) is the computation time of task \( i \)
- \( T_i \) is the period of task \( i \)
- \( n \) is the total number of tasks

For the current set of services the total CPU usage is:

$$
U = \frac{1}{3} + \frac{2}{5} + \frac{3}{15} = 0.933 = 93.33\\%
$$

A simple check to perform a baseline validation of the safety of the services is to calulate the LUB of the services.  The LUB can be calculated with:

$$
U_{\text{max}} = n(2^{1/n} - 1)
$$

Where:
- \( n \) is the number of tasks

For the current set of services the LUB is:

$$
U_{\text{max}} = 3(2^{1/3} - 1) \approx 0.78 \approx 78\\%
$$

To guarantee a system is schedulable under RMP the total CPU time must be less than or equal to the LUB otherwise additional verification is required.  In the case of these services, it is not guaranteed to be shedulable due to the total CPU usage being greater than the LUB.

After performing a by hand analysis in section B it is determined that this set of services is repeatable and safe due to the harmonic nature of their periods and computation periods.  If any of the periods were changed it could easily lead to the system not being schedulable.

---

## Problem 2

> Read through the Apollo 11 Lunar lander computer overload story as reported in RTECS Notes, based on this NASA account, and the descriptions of the 1201/1202 events described by chief software engineer Margaret Hamilton as recounted by Dylan Matthews. Summarize the story.

This story is about the 1201 and 1202 error codes that caused CPU reboots of the Apollo 11 flight computer during the landing phase of the mission that almost caused a mission abort.  The issue arose due to a misconfiguration of the radar switches leading to repeated low priority jobs being spawned to process rendezvous radar data that was not needed at the time.

Since the flight computer had very limited RAM, 2048 15-bit words, all of the real time jobs had to share a pool of 12 total RAM locations, 7 of which were considered core sets used by all spawned jobs and 5 of which were VAC areas which were larger.  Each time a job was spawned it would scan for the first open core set and, if needed, the first open VAC set to acquire for the duration of the job.  In the event of no VAC areas being available, the program would branch to the Alarm/Abort routine and raise error 1201.  In the event of no core sets being available it would raise error 1202.  Both of these errors would cause a CPU reboot.

Since the radars were generating data when the weren't supposed to, the core sets were eventually all taken up leading to 1202 error being raised.  Later in the landing, all of the VAC sets were taken up leading to a 1201 error being raised.  Due to the extensive testing done by MIT, specifically in error handling and reboots, the important tasks like GNC and ADCS were properly restarted while the faulty low priority tasks weren't allowing the mission to continue to the eventual landing.

> A: What was the root cause of the overload and why did it violate Rate Monotonic policy?

The root cause of the overload was the misconfiguration of the rendezvous radar leading to low priority tasks that weren't needed at the time hogging the shared resource pool of the higher priority tasks.  RMP assumes that resources are always available for high priority tasks which was not the case here leading to the errors being raised causing CPU reboots.

> B: Now, read Liu and Layland’s paper which describes Rate Monotonic policy and the Least Upper Bound – they derive an equation which advises margin of approximately 30% of the total CPU as the number of services sharing a single CPU core increases.

This paper defines the Least Upper Bound (LUB) that I referenced during question 1.  The idea of the LUB is that if the total CPU utilization of your tasks is less than the LUB, it is guaranteed to be schedulable using RMS.

$$
U = \sum_{i=1}^{n} \frac{C_i}{T_i} \leq n\left(2^{1/n} - 1\right)
$$

The reason why advised margin is roughly 30% is because this equation reaches a limit that is just below 70% of total CPU consumption meaning once you reach a certain number of tasks in the system, as long as their total CPU load is under that limit they will be guaranteed to be schedulable under RMS.

> C: Plot this Least Upper bound as a function of number of services.

Below is a plot of the LUB equation demonstrating how it very quickly begins to approach a value of 70% after just 5-10 tasks.

<p align="center">
    <img src="./assets/lub-plot.png" />
</p>

> D: Describe three key assumptions Liu and Layland make and document three or more aspects of their fixed priority LUB derivation that you do not understand.

Key Assumptions all pulled from (C. L. Liu and J. W. Layland):
1. Requests for all tasks for which hard deadlines are **periodic** with a constant interval between requests.
   1. This is where the period, or $T_n$ value, comes from when plotting or calculating RMS.  This is also where the basis of scheduling comes from since we are trying to schedule periodic tasks for the life of a program.
2. Tasks are independent in that requests for certain tasks do not depend on the initiation or completion of requests for other tasks.
   1. This intuitively makes sense to me since we need to be able to schedule and run tasks whenever they are available according to the scheduler.  If a higher priority task relied on a lower priority task, it would be impossible to run.
3. Run time for each task is constant for that task and does not vary with time. Run time here refers to the time which is taken by a processor to execute the task without interruption.
   1. This is where the $C_n$ value comes from when calculating RMS.  In other readings and on the slides this is mentioned as the worst case run time for the task which should be close or exactly on the average of the task run time in a real time system

Aspects of the derivation I don't understand or am confused by:
1. This seems to be a pretty idealized set of constraints.  In my experience, there is a relatively constant flow of interrputs being fired due to I/O while talking with external components.  How does one take the information of the LUB and then use it for a real world system?
2. I have also used one shot tasks in the past or tasks that aren't necessarily periodic but instead are called on an as-needed basis.  How can these fit into the calculation of LUB since the first assumption is that these tasks are periodic?
3. This paper is specifically written for analyzing single core scheduling.  I understand that SMP is much more complex but how much more complex will the math get for calculating something akin to LUB when dealing with 2 or more processors?

> E: Would RM analysis have prevented the Apollo 11 1201/1202 errors and potential mission abort? Why or why not?

I don't believe that RM analysis could have prevented thee errors on the Apollo 11 mission.  The situation described sounded like an off-nominal configuration leading to the breaking of RMP instead of a nominal software configuration leading to the fault.  This appears to be more of a symptom of improper testing or user error leading to the rendezvous radar producing the data.  In this particular case, I interpreted that the radar data shouldn't have been getting processed at all during the landing phase of the mission.  If that is the case, the RM analysis would have most likely been done not accounting for the radar data in the first place leading to this fault still occurring.  Instead, it was due to proper testing of the error handling and the intelligence of the restart system being able to bring back up critical tasks while ditching lower priority tasks (like the radar processing) that allowed for the mission to continue and successfully land.

---

## Problem 3

> Download RT-Clock and build it on an R-Pi3b+ or newer and execute the code.

Below is the output from running the program as-is.
```bash
Before adjustments to scheduling policy:
Pthread Policy is SCHED_OTHER
After adjustments to scheduling policy:
Pthread Policy is SCHED_FIFO


POSIX Clock demo using system RT clock with resolution:
	0 secs, 0 microsecs, 1 nanosecs
test 0
MY_CLOCK clock DT seconds = 0, msec=10, usec=10013, nsec=10013148, sec=0.010013148
MY_CLOCK delay error = 0, nanoseconds = 13148
test 1
MY_CLOCK clock DT seconds = 0, msec=10, usec=10012, nsec=10012314, sec=0.010012314
MY_CLOCK delay error = 0, nanoseconds = 12314
test 2
MY_CLOCK clock DT seconds = 0, msec=10, usec=10011, nsec=10011389, sec=0.010011389
MY_CLOCK delay error = 0, nanoseconds = 11389
test 3
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010259, sec=0.010010259
MY_CLOCK delay error = 0, nanoseconds = 10259
test 4
MY_CLOCK clock DT seconds = 0, msec=10, usec=10009, nsec=10009815, sec=0.010009815
MY_CLOCK delay error = 0, nanoseconds = 9815
test 5
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010463, sec=0.010010463
MY_CLOCK delay error = 0, nanoseconds = 10463
test 6
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010482, sec=0.010010482
MY_CLOCK delay error = 0, nanoseconds = 10482
test 7
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010333, sec=0.010010333
MY_CLOCK delay error = 0, nanoseconds = 10333
test 8
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010148, sec=0.010010148
MY_CLOCK delay error = 0, nanoseconds = 10148
test 9
MY_CLOCK clock DT seconds = 0, msec=10, usec=10009, nsec=10009056, sec=0.010009056
MY_CLOCK delay error = 0, nanoseconds = 9056
test 10
MY_CLOCK clock DT seconds = 0, msec=10, usec=10009, nsec=10009407, sec=0.010009407
MY_CLOCK delay error = 0, nanoseconds = 9407
test 11
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010111, sec=0.010010111
MY_CLOCK delay error = 0, nanoseconds = 10111
test 12
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010500, sec=0.010010500
MY_CLOCK delay error = 0, nanoseconds = 10500
test 13
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010408, sec=0.010010408
MY_CLOCK delay error = 0, nanoseconds = 10408
test 14
MY_CLOCK clock DT seconds = 0, msec=10, usec=10009, nsec=10009833, sec=0.010009833
MY_CLOCK delay error = 0, nanoseconds = 9833
test 15
MY_CLOCK clock DT seconds = 0, msec=10, usec=10009, nsec=10009926, sec=0.010009926
MY_CLOCK delay error = 0, nanoseconds = 9926
test 16
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010444, sec=0.010010444
MY_CLOCK delay error = 0, nanoseconds = 10444
test 17
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010092, sec=0.010010092
MY_CLOCK delay error = 0, nanoseconds = 10092
test 18
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010537, sec=0.010010537
MY_CLOCK delay error = 0, nanoseconds = 10537
test 19
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010981, sec=0.010010981
MY_CLOCK delay error = 0, nanoseconds = 10981
test 20
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010130, sec=0.010010130
MY_CLOCK delay error = 0, nanoseconds = 10130
test 21
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010223, sec=0.010010223
MY_CLOCK delay error = 0, nanoseconds = 10223
test 22
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010111, sec=0.010010111
MY_CLOCK delay error = 0, nanoseconds = 10111
test 23
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010019, sec=0.010010019
MY_CLOCK delay error = 0, nanoseconds = 10019
test 24
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010334, sec=0.010010334
MY_CLOCK delay error = 0, nanoseconds = 10334
test 25
MY_CLOCK clock DT seconds = 0, msec=10, usec=10009, nsec=10009481, sec=0.010009481
MY_CLOCK delay error = 0, nanoseconds = 9481
test 26
MY_CLOCK clock DT seconds = 0, msec=10, usec=10009, nsec=10009278, sec=0.010009278
MY_CLOCK delay error = 0, nanoseconds = 9278
test 27
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010556, sec=0.010010556
MY_CLOCK delay error = 0, nanoseconds = 10556
test 28
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010833, sec=0.010010833
MY_CLOCK delay error = 0, nanoseconds = 10833
test 29
MY_CLOCK clock DT seconds = 0, msec=10, usec=10009, nsec=10009537, sec=0.010009537
MY_CLOCK delay error = 0, nanoseconds = 9537
test 30
MY_CLOCK clock DT seconds = 0, msec=10, usec=10009, nsec=10009704, sec=0.010009704
MY_CLOCK delay error = 0, nanoseconds = 9704
test 31
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010185, sec=0.010010185
MY_CLOCK delay error = 0, nanoseconds = 10185
test 32
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010018, sec=0.010010018
MY_CLOCK delay error = 0, nanoseconds = 10018
test 33
MY_CLOCK clock DT seconds = 0, msec=10, usec=10011, nsec=10011426, sec=0.010011426
MY_CLOCK delay error = 0, nanoseconds = 11426
test 34
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010333, sec=0.010010333
MY_CLOCK delay error = 0, nanoseconds = 10333
test 35
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010167, sec=0.010010167
MY_CLOCK delay error = 0, nanoseconds = 10167
test 36
MY_CLOCK clock DT seconds = 0, msec=10, usec=10009, nsec=10009259, sec=0.010009259
MY_CLOCK delay error = 0, nanoseconds = 9259
test 37
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010463, sec=0.010010463
MY_CLOCK delay error = 0, nanoseconds = 10463
test 38
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010055, sec=0.010010055
MY_CLOCK delay error = 0, nanoseconds = 10055
test 39
MY_CLOCK clock DT seconds = 0, msec=10, usec=10012, nsec=10012092, sec=0.010012092
MY_CLOCK delay error = 0, nanoseconds = 12092
test 40
MY_CLOCK clock DT seconds = 0, msec=10, usec=10011, nsec=10011852, sec=0.010011852
MY_CLOCK delay error = 0, nanoseconds = 11852
test 41
MY_CLOCK clock DT seconds = 0, msec=10, usec=10009, nsec=10009630, sec=0.010009630
MY_CLOCK delay error = 0, nanoseconds = 9630
test 42
MY_CLOCK clock DT seconds = 0, msec=10, usec=10009, nsec=10009351, sec=0.010009351
MY_CLOCK delay error = 0, nanoseconds = 9351
test 43
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010833, sec=0.010010833
MY_CLOCK delay error = 0, nanoseconds = 10833
test 44
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010204, sec=0.010010204
MY_CLOCK delay error = 0, nanoseconds = 10204
test 45
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010463, sec=0.010010463
MY_CLOCK delay error = 0, nanoseconds = 10463
test 46
MY_CLOCK clock DT seconds = 0, msec=10, usec=10009, nsec=10009648, sec=0.010009648
MY_CLOCK delay error = 0, nanoseconds = 9648
test 47
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010222, sec=0.010010222
MY_CLOCK delay error = 0, nanoseconds = 10222
test 48
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010389, sec=0.010010389
MY_CLOCK delay error = 0, nanoseconds = 10389
test 49
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010704, sec=0.010010704
MY_CLOCK delay error = 0, nanoseconds = 10704
test 50
MY_CLOCK clock DT seconds = 0, msec=10, usec=10009, nsec=10009740, sec=0.010009740
MY_CLOCK delay error = 0, nanoseconds = 9740
test 51
MY_CLOCK clock DT seconds = 0, msec=10, usec=10009, nsec=10009111, sec=0.010009111
MY_CLOCK delay error = 0, nanoseconds = 9111
test 52
MY_CLOCK clock DT seconds = 0, msec=10, usec=10009, nsec=10009186, sec=0.010009186
MY_CLOCK delay error = 0, nanoseconds = 9186
test 53
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010426, sec=0.010010426
MY_CLOCK delay error = 0, nanoseconds = 10426
test 54
MY_CLOCK clock DT seconds = 0, msec=10, usec=10009, nsec=10009148, sec=0.010009148
MY_CLOCK delay error = 0, nanoseconds = 9148
test 55
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010148, sec=0.010010148
MY_CLOCK delay error = 0, nanoseconds = 10148
test 56
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010055, sec=0.010010055
MY_CLOCK delay error = 0, nanoseconds = 10055
test 57
MY_CLOCK clock DT seconds = 0, msec=10, usec=10009, nsec=10009537, sec=0.010009537
MY_CLOCK delay error = 0, nanoseconds = 9537
test 58
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010259, sec=0.010010259
MY_CLOCK delay error = 0, nanoseconds = 10259
test 59
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010203, sec=0.010010203
MY_CLOCK delay error = 0, nanoseconds = 10203
test 60
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010148, sec=0.010010148
MY_CLOCK delay error = 0, nanoseconds = 10148
test 61
MY_CLOCK clock DT seconds = 0, msec=10, usec=10009, nsec=10009222, sec=0.010009222
MY_CLOCK delay error = 0, nanoseconds = 9222
test 62
MY_CLOCK clock DT seconds = 0, msec=10, usec=10009, nsec=10009129, sec=0.010009129
MY_CLOCK delay error = 0, nanoseconds = 9129
test 63
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010259, sec=0.010010259
MY_CLOCK delay error = 0, nanoseconds = 10259
test 64
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010278, sec=0.010010278
MY_CLOCK delay error = 0, nanoseconds = 10278
test 65
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010148, sec=0.010010148
MY_CLOCK delay error = 0, nanoseconds = 10148
test 66
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010278, sec=0.010010278
MY_CLOCK delay error = 0, nanoseconds = 10278
test 67
MY_CLOCK clock DT seconds = 0, msec=10, usec=10009, nsec=10009592, sec=0.010009592
MY_CLOCK delay error = 0, nanoseconds = 9592
test 68
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010166, sec=0.010010166
MY_CLOCK delay error = 0, nanoseconds = 10166
test 69
MY_CLOCK clock DT seconds = 0, msec=10, usec=10020, nsec=10020611, sec=0.010020611
MY_CLOCK delay error = 0, nanoseconds = 20611
test 70
MY_CLOCK clock DT seconds = 0, msec=10, usec=10011, nsec=10011055, sec=0.010011055
MY_CLOCK delay error = 0, nanoseconds = 11055
test 71
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010611, sec=0.010010611
MY_CLOCK delay error = 0, nanoseconds = 10611
test 72
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010425, sec=0.010010425
MY_CLOCK delay error = 0, nanoseconds = 10425
test 73
MY_CLOCK clock DT seconds = 0, msec=10, usec=10008, nsec=10008667, sec=0.010008667
MY_CLOCK delay error = 0, nanoseconds = 8667
test 74
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010537, sec=0.010010537
MY_CLOCK delay error = 0, nanoseconds = 10537
test 75
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010482, sec=0.010010482
MY_CLOCK delay error = 0, nanoseconds = 10482
test 76
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010222, sec=0.010010222
MY_CLOCK delay error = 0, nanoseconds = 10222
test 77
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010167, sec=0.010010167
MY_CLOCK delay error = 0, nanoseconds = 10167
test 78
MY_CLOCK clock DT seconds = 0, msec=10, usec=10016, nsec=10016740, sec=0.010016740
MY_CLOCK delay error = 0, nanoseconds = 16740
test 79
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010777, sec=0.010010777
MY_CLOCK delay error = 0, nanoseconds = 10777
test 80
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010241, sec=0.010010241
MY_CLOCK delay error = 0, nanoseconds = 10241
test 81
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010518, sec=0.010010518
MY_CLOCK delay error = 0, nanoseconds = 10518
test 82
MY_CLOCK clock DT seconds = 0, msec=10, usec=10012, nsec=10012315, sec=0.010012315
MY_CLOCK delay error = 0, nanoseconds = 12315
test 83
MY_CLOCK clock DT seconds = 0, msec=10, usec=10008, nsec=10008667, sec=0.010008667
MY_CLOCK delay error = 0, nanoseconds = 8667
test 84
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010056, sec=0.010010056
MY_CLOCK delay error = 0, nanoseconds = 10056
test 85
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010037, sec=0.010010037
MY_CLOCK delay error = 0, nanoseconds = 10037
test 86
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010260, sec=0.010010260
MY_CLOCK delay error = 0, nanoseconds = 10260
test 87
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010204, sec=0.010010204
MY_CLOCK delay error = 0, nanoseconds = 10204
test 88
MY_CLOCK clock DT seconds = 0, msec=10, usec=10009, nsec=10009778, sec=0.010009778
MY_CLOCK delay error = 0, nanoseconds = 9778
test 89
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010093, sec=0.010010093
MY_CLOCK delay error = 0, nanoseconds = 10093
test 90
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010222, sec=0.010010222
MY_CLOCK delay error = 0, nanoseconds = 10222
test 91
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010222, sec=0.010010222
MY_CLOCK delay error = 0, nanoseconds = 10222
test 92
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010259, sec=0.010010259
MY_CLOCK delay error = 0, nanoseconds = 10259
test 93
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010092, sec=0.010010092
MY_CLOCK delay error = 0, nanoseconds = 10092
test 94
MY_CLOCK clock DT seconds = 0, msec=10, usec=10009, nsec=10009556, sec=0.010009556
MY_CLOCK delay error = 0, nanoseconds = 9556
test 95
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010092, sec=0.010010092
MY_CLOCK delay error = 0, nanoseconds = 10092
test 96
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010388, sec=0.010010388
MY_CLOCK delay error = 0, nanoseconds = 10388
test 97
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010185, sec=0.010010185
MY_CLOCK delay error = 0, nanoseconds = 10185
test 98
MY_CLOCK clock DT seconds = 0, msec=10, usec=10010, nsec=10010000, sec=0.010010000
MY_CLOCK delay error = 0, nanoseconds = 10000
test 99
MY_CLOCK clock DT seconds = 0, msec=10, usec=10009, nsec=10009500, sec=0.010009500
MY_CLOCK delay error = 0, nanoseconds = 9500
TEST COMPLETE
```

> A: Describe what the code is doing and make sure you understand clock_gettime and how to use it to time code execution (print or log timestamps between two points in your code).

The program starts through its `main()` function.  It will first print out the scheduling policy to stdout before taking one of two routes.  1, if `RUN_RT_THREAD` isn't defined it will run the test in the regular main thread which is not a `pthread`.  If the macro is defined, it will instead initialize the pthread scheduler, set it to the explicit scheduler and set it to the FIFO scheduler.  After setting the poliy, it retrieves the maximum priority from the FIFO scheduler and then sets it to the current process.

After configuring the scheduler, it will thenn call `print_scheduler()` again to print the settings of the scheduler for the current process.  After doing so, it will create a new pthread running the `delay_test()` function with the `main_sched_attr` settings.  If there wasn't an error when creating the pthread, the program will then call `pthread_join()` on the handle to that thread waiting for it to complete execution.

Within `delay_test()`, the first thing that is done is retrieving the current clock resolution using the `clock_getres()` function.  If there were no errors the program will print out the resolution of the clock before going into the main test loop of the program.  For `TEST_ITERATIONS` iterations (100 for this program), the program initializes a requested sleep time of 10 milliseconds using the defined values `TEST_SECONDS` and `TEST_NANOSECONDS`.  It then calls `clock_gettime()` to retrieve the current time from the selected clock before calling `nanosleep()` to cause the thread to sleep for the requested amount of time.  If the call to `nanosleep()` is zero, the sleep was successfully completed without an interrupt/error.  If the return value is non-zero the sleep was interrupted and is then retried up to `max_sleep_calls` iterations.  After the sleep, there is an additional call to `clock_gettime()` to get the ending time of the loop iteration.  After getting the stop time the helper method `delta_t()` is used to calculate the amount of time between the start and end time as well as the amount of error between the requested sleep duration and the actual sleep duration.  After computing the deltas the function `end_delay_test()` is called to print out the total coarse seconds, milliseconds, microseconds, and nanoseconds of the sleep as well as the seconds as a decimal value.  It also prints out the number of coarse seconds and nanoseconds of the delta between the requested and actual sleep duration.

Once the loop completes its iterations, the `pthread_join()` function call finishes and the main scheduler attributes are destroyed before the final `TEST COMPLETE` print is sent to stdout.

> B: Which clock is best to use? CLOCK_REALTIME, CLOCK_MONOTONIC or CLOCK_MONOTONIC_RAW? Please choose one and update code and improve the commenting.

By default the program is running with `CLOCK_MONOTONIC_RAW`.  For a hard realtime program I believe this is the best clock to use for the following reasons:
- This is a monotonic clock that counts the number of seconds since boot and is guaranteed to never go backwards/jump
- This clock is not adjusted by NTP meaning it will keep running at the same rate without being adjusted by external factors
- This clock is from a raw hardware timer which gives both higher resolution and deterministic timing