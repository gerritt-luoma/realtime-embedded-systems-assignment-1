# RTES Assignment 1
Repository for the first assignment of CU Boulder's Real Time Embedded Systems course

## Additional Sources
- [Geeks for Geeks RMS](https://www.geeksforgeeks.org/rate-monotonic-scheduling/)

## Problem 1

> The Rate Monotonic Policy states that services which share a CPU core should multiplex it (with context switches that preempt and dispatch tasks) based on priority, where highest priority is assigned to the most frequently requested service and lowest priority is assigned to the least frequently requested AND total shared CPU core utilization must preserve some margin (not be fully utilized or overloaded).

### Part A


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
U = \frac{1}{3} + \frac{2}{5} + \frac{3}{15} = 0.933 = 93.33\%
$$

A simple check to perform a baseline validation of the safety of the services is to calulate the LUB of the services.  The LUB can be calculated with:

$$
U_{\text{max}} = n(2^{1/n} - 1)
$$

Where:
- \( n \) is the number of tasks

For the current set of services the LUB is:

$$
U_{\text{max}} = 3(2^{1/3} - 1) \approx 0.78 \approx 78\%
$$

To guarantee a system is schedulable under RMP the total CPU time must be less than or equal to the LUB otherwise additional verification is required.  In the case of these services, it is not guaranteed to be shedulable due to the total CPU usage being greater than the LUB.

After performing a by hand analysis in section B it is determined that this set of services is repeatable and safe due to the harmonic nature of their periods and computation periods.  If any of the periods were changed it could easily lead to the system not being schedulable.
