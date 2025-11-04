#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

// Assignment Input Data
static const char* names[] = { "P1","P2","P3","P4","P5" }; // the process identifiers 
static const int   arrival[] = { 0,   1,   2,   3,   4  }; // the arrival time of each process (in time units). 
static const int   burst[] = { 10,   5,   8,   6,   3 }; // the burst time (execution time in time units) of each process. 
enum {
	N = (int)(sizeof(names) / sizeof(names[0]))
};

// Global Output Arrays
static int waiting_times[N];
static int turnaround_times[N];
static int core_allocated[N];

// Process Data Structure
// Represents a single process and its state during the simulation.
typedef struct Process {
	int index;
	const char* name;
	int arrival_time;
	int burst_time;
	int completed; // 0 for not completed, 1 for completed
} Process;

// The master list of all processes to be scheduled.
struct Process master_process_list[N];

// Function to initialize the master process list
// Populates the master_process_list with the initial data from the static arrays.
static void initialize_master_list() {
	for (int i = 0; i < N; i++) {
		master_process_list[i].index = i;
		master_process_list[i].name = names[i];
		master_process_list[i].arrival_time = arrival[i];
		master_process_list[i].burst_time = burst[i];
		master_process_list[i].completed = 0; // Mark as not completed initially
	}
}

// --- Main Program ---
int main(int argc, char* argv[]) {
	initialize_master_list();

	int current_time = 0;
	int processes_completed = 0;
	int CPU1_time = 0; // Tracks when CPU 1 will become free
	int CPU2_time = 0; // Tracks when CPU 2 will become free

	// The main simulation loop continues until all processes are completed.
	while (processes_completed < N) {
		int best_job_index = -1;
		int best_cpu = -1; // 0 for CPU1, 1 for CPU2
		int shortest_burst = INT_MAX;

		// Job Selection Logic (SJF)
		// At the current time, find the best job for the CPU that will be free earliest.
		int available_cpu_time = (CPU1_time <= CPU2_time) ? CPU1_time : CPU2_time;
		int target_cpu = (CPU1_time <= CPU2_time) ? 0 : 1;

		// The effective time for checking arrivals is the time the chosen CPU is free. However, if the CPU is idle, we must advance time to the next arrival.
		int effective_time = available_cpu_time;

		int next_arrival = INT_MAX;
		int has_eligible_job = 0;

		// First pass: Check for eligible jobs at the time the CPU becomes free.
		for (int i = 0; i < N; i++) {
			if (!master_process_list[i].completed) {
				// Find the absolute next arrival time among remaining processes
				if (master_process_list[i].arrival_time < next_arrival) {
					next_arrival = master_process_list[i].arrival_time;
				}
				// Check if the process has arrived by the time the target CPU is free
				if (master_process_list[i].arrival_time <= effective_time) {
					has_eligible_job = 1;
					if (master_process_list[i].burst_time < shortest_burst) {
						shortest_burst = master_process_list[i].burst_time;
						best_job_index = i;
						best_cpu = target_cpu;
					}
				}
			}
		}

		// If no job was ready for the free CPU, it means there's an idle period. We must advance time to the next process arrival.
		if (!has_eligible_job && processes_completed < N) {
			current_time = next_arrival;
			// Advance the earliest free CPU's time to this new arrival time.
			if (CPU1_time <= CPU2_time) {
				CPU1_time = current_time;
			}
			else {
				CPU2_time = current_time;
			}
			continue; // Re-run the selection logic at the new time.
		}

		// --- Job Execution and Metric Calculation ---
		if (best_job_index != -1) {
			Process* job = &master_process_list[best_job_index];

			int start_time;
			if (best_cpu == 0) { // Assign to CPU 1
				start_time = (CPU1_time > job->arrival_time) ? CPU1_time : job->arrival_time;
				CPU1_time = start_time + job->burst_time;
				core_allocated[job->index] = 0;
			}
			else { // Assign to CPU 2
				start_time = (CPU2_time > job->arrival_time) ? CPU2_time : job->arrival_time;
				CPU2_time = start_time + job->burst_time;
				core_allocated[job->index] = 1;
			}

			waiting_times[job->index] = start_time - job->arrival_time;
			turnaround_times[job->index] = (start_time + job->burst_time) - job->arrival_time;

			job->completed = 1;
			processes_completed++;
		}
	}

	// Final Output
	double avg_waiting_time = 0.0;
	double avg_turnaround_time = 0.0;

	// Sort results for consistent printing order (P1, P2, ...)
	Process final_results[N];
	for (int i = 0; i < N; i++) {
		int original_index = master_process_list[i].index;
		final_results[original_index] = master_process_list[i];
	}

	for (int i = 0; i < N; i++) {
		printf("Process: %s Arrival: %d Burst: %d CPU: %d Waiting Time: %d Turnaround Time: %d \n", 
			names[i], arrival[i], burst[i], core_allocated[i], waiting_times[i], turnaround_times[i]);
		avg_waiting_time += waiting_times[i];
		avg_turnaround_time += turnaround_times[i];
	}

	printf("Average waiting time = %.2f \n", avg_waiting_time / N);
	printf("Average turnaround time = %.2f \n", avg_turnaround_time / N);

			return 0;
}
/*
OUTPUT:
Process: P1 Arrival : 0 Burst : 10 CPU : 0 Waiting Time : 0 Turnaround Time : 10
Process : P2 Arrival : 1 Burst : 5 CPU : 1 Waiting Time : 0 Turnaround Time : 5
Process : P3 Arrival : 2 Burst : 8 CPU : 0 Waiting Time : 8 Turnaround Time : 16
Process : P4 Arrival : 3 Burst : 6 CPU : 1 Waiting Time : 6 Turnaround Time : 12
Process : P5 Arrival : 4 Burst : 3 CPU : 1 Waiting Time : 2 Turnaround Time : 5
Average waiting time = 3.20
Average turnaround time = 9.60

----------------------------------------------------------------------------------------------

if data is changed to:
static const char* names[] = { "P1", "P2", "P3", "P4", "P5" };
static const int arrival[] = { 0, 1, 2, 3, 4 };
static const int burst[] = { 1, 8, 5, 2, 3 };

Process: P1 Arrival: 0 Burst: 1 CPU: 0 Waiting Time: 0 Turnaround Time: 1
Process: P2 Arrival: 1 Burst: 8 CPU: 0 Waiting Time: 0 Turnaround Time: 8
Process: P3 Arrival: 2 Burst: 5 CPU: 1 Waiting Time: 0 Turnaround Time: 5
Process: P4 Arrival: 3 Burst: 2 CPU: 1 Waiting Time: 4 Turnaround Time: 6
Process: P5 Arrival: 4 Burst: 3 CPU: 0 Waiting Time: 5 Turnaround Time: 8
Average waiting time = 1.80
Average turnaround time = 5.60

----------------------------------------------------------------------------------------------

if data is changed to:
static const char* names[] = { "P1", "P2", "P3", "P4", "P5", "P6", "P7"};
static const int arrival[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
static const int burst[] = { 1, 8, 5, 2, 3, 10, 5, 4 };

Process: P1 Arrival: 0 Burst: 1 CPU: 0 Waiting Time: 0 Turnaround Time: 1
Process: P2 Arrival: 1 Burst: 8 CPU: 0 Waiting Time: 0 Turnaround Time: 8
Process: P3 Arrival: 2 Burst: 5 CPU: 1 Waiting Time: 0 Turnaround Time: 5
Process: P4 Arrival: 3 Burst: 2 CPU: 1 Waiting Time: 4 Turnaround Time: 6
Process: P5 Arrival: 4 Burst: 3 CPU: 0 Waiting Time: 5 Turnaround Time: 8
Process: P6 Arrival: 5 Burst: 10 CPU: 0 Waiting Time: 7 Turnaround Time: 17
Process: P7 Arrival: 6 Burst: 5 CPU: 1 Waiting Time: 3 Turnaround Time: 8
Average waiting time = 2.71
Average turnaround time = 7.57
*/
