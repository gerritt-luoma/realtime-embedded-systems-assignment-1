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