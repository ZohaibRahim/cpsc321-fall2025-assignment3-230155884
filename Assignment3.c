#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <stddef.h>
#include <limits.h>
#include <sched.h>

static const char* names[] = { "P1","P2","P3","P4","P5" }; // the process identifiers 
static const int   arrival[] = { 0,   1,   2,   3,   4 }; // the arrival time of each process (in time units). 
static const int   burst[] = { 10,   5,   8,   6,   3 }; // the burst time (execution time in time units) of each process.

enum { 
	N = (int)(sizeof(names) / sizeof(names[0])) 
}; // Number of processes

static int waiting_times[N];    // Array to store waiting times of processes
static int turnaround_times[N]; // Array to store turnaround times of processes
static int core_allocated[N]; // Array to store allocted core number for each process
static int CPU1_time = 0; // Total time consumed by CPU core 1
static int CPU2_time = 0; // Total time consumed by CPU core 2

// Synchronization Primitives
pthread_mutex_t mutex; // Mutex for thread safety
pthread_mutex_t time_mutex;         // Protects current_cpu_time

// Structure to represent a process
typedef struct Process {
	int index;         
	const char* name; // Pointer to the process name string
	int arrival_time; // Arrival time
	int burst_time;   // Burst time (used for priority)
	int waiting_time; // Waiting time
	int turnaround_time; // Turnaround time
	int core_allocated;  // CPU core allocated
	int completed_time; // Completion time
} Process;

//Process list of eligible processes
struct Process master_process_list[N];
int processesLeftInMasterList = 0;

// Function to initialize the master process list based on arrival times/ eligible processes
static void initialize_master_list() {
	for (int i = 0; i < N; i++) {
		master_process_list[i].index = i;
		master_process_list[i].name = names[i];
		master_process_list[i].arrival_time = arrival[i];
		master_process_list[i].burst_time = burst[i];

		master_process_list[i].waiting_time = 0;
		master_process_list[i].turnaround_time = 0;
		master_process_list[i].core_allocated = -1; // -1 means not allocated yet
		master_process_list[i].completed_time = 0;

		processesLeftInMasterList++;
	}
}

// Structure to represent the Priority Queue (Min-Heap)
typedef struct PriorityQueue {
	Process* arr; // Array to store heap elements
	int size;      // Current number of elements in the heap
	int capacity;  // Maximum capacity of the heap
} PriorityQueue;

// Helper function to swap two processes
static void swap(Process* p1, Process* p2) {
	Process temp = *p1;
	*p1 = *p2;
	*p2 = temp;
}

// Find the next (minimum) arrival among not-yet-moved processes
static int next_arrival_time(void) {
	int t = INT_MAX;
	pthread_mutex_lock(&mutex);
	for (int i = 0; i < processesLeftInMasterList; ++i) {
		if (master_process_list[i].arrival_time < t)
			t = master_process_list[i].arrival_time;
	}
	pthread_mutex_unlock(&mutex);
	return (t == INT_MAX) ? -1 : t;
}

// Advance the earlier CPU clock to t so min(CPU1_time, CPU2_time) becomes t 
static void advance_time_to_next(int t) {
	if (t < 0) return;
	pthread_mutex_lock(&time_mutex);
	if (CPU1_time <= CPU2_time) {
		if (CPU1_time < t) CPU1_time = t;
	}
	else {
		if (CPU2_time < t) CPU2_time = t;
	}
	pthread_mutex_unlock(&time_mutex);
}

// Function to maintain the min - heap property from a given index
static void minHeapify(PriorityQueue* pq, int idx) {
	int smallest = idx;
	int left = 2 * idx + 1;
	int right = 2 * idx + 2;

	if (left < pq->size && pq->arr[left].burst_time < pq->arr[smallest].burst_time) {
		smallest = left;
	}
	if (right < pq->size && pq->arr[right].burst_time < pq->arr[smallest].burst_time) {
		smallest = right;
	}

	if (smallest != idx) {
		swap(&pq->arr[idx], &pq->arr[smallest]);
		minHeapify(pq, smallest); // Recursively heapify the affected sub-tree
	}
}

// Heap push for SJF by burst_time
static void heapPush(PriorityQueue* pq, Process p) {
	if (pq->size >= pq->capacity) return; // or grow
	int i = pq->size++;
	pq->arr[i] = p;
	while (i > 0) {
		int parent = (i - 1) / 2;
		if (pq->arr[parent].burst_time <= pq->arr[i].burst_time) break;
		swap(&pq->arr[parent], &pq->arr[i]);
		i = parent;
	}
}

// Heap pop for SJF by burst_time
static Process heapPop(PriorityQueue* pq) {
	Process empty = (Process){ .index = -1 };
	if (pq->size == 0) return empty;
	Process root = pq->arr[0];
	pq->arr[0] = pq->arr[pq->size - 1];
	pq->size--;
	minHeapify(pq, 0);
	return root;
}

static void deleteElement(int indexToDelete) {
	// Check if the index is valid
	if (indexToDelete < 0 || indexToDelete >= processesLeftInMasterList) {
		return;
	}

	// Shift elements to the left
	for (int i = indexToDelete; i < processesLeftInMasterList - 1; i++) {
		master_process_list[i] = master_process_list[i + 1];
	}

	processesLeftInMasterList--;
}

// Function to check for newly arrived processes and add them to the eligible process list
static void check_for_arrivals(PriorityQueue* pq, int now) {
	pthread_mutex_lock(&mutex);	// LOCK the shared queue before manipulation

	// Iterate through master list and move eligible processes
	for (int i = 0; i < processesLeftInMasterList; ){
		if (master_process_list[i].arrival_time <= now) {
			heapPush(pq, master_process_list[i]); // Add to priority queue
			deleteElement(i);
		}
		else
			i++; // Only increment if no deletion
	}
	pthread_mutex_unlock(&mutex);	// UNLOCK the queue
}


// Function to create a new priority queue
static PriorityQueue* createPriorityQueue(int capacity) {
	PriorityQueue* pq = (PriorityQueue*)malloc(sizeof(PriorityQueue));
	pq->capacity = capacity;
	pq->arr = (Process*)malloc(pq->capacity * sizeof(Process));
	pq->size = 0;
	return pq;
}


// Function to extract the minimum process safely using a mutex lock
static Process threadSafeHeapPop(PriorityQueue* pq) {
	pthread_mutex_lock(&mutex);
	Process p = heapPop(pq);
	pthread_mutex_unlock(&mutex);
	return p;
}

static void short_sleep(void) {
	struct timespec ts = { .tv_sec = 0, .tv_nsec = 1000000 }; // 1 ms
	nanosleep(&ts, NULL);
}

// Thread functions for CPU core 1
static void* CPU0(void* arg) {
    PriorityQueue* pq = (PriorityQueue*)arg;

    for (;;) {
		Process current = threadSafeHeapPop(pq);


		int pending;
		pthread_mutex_lock(&mutex);
		pending = processesLeftInMasterList;
		pthread_mutex_unlock(&mutex);

		if (current.index == -1) {
			if (pending > 0) {
				int t = next_arrival_time();   // earliest arrival left
				advance_time_to_next(t);       // move earlier CPU clock to t
				continue;                      // then loop back to admit+pop again
			}
			break; // no heap items and nothing pending => done
		}

		// align this core's clock to arrival, compute waiting
		pthread_mutex_lock(&time_mutex);
		if (CPU1_time < current.arrival_time) CPU1_time = current.arrival_time;
		current.waiting_time = CPU1_time - current.arrival_time;
		pthread_mutex_unlock(&time_mutex);

		// run non-preemptive
		for (int t = 0; t < current.burst_time; t++) {
			// advance this core's time by 1
			pthread_mutex_lock(&time_mutex);
			CPU1_time++;
			int now = (CPU1_time > CPU2_time) ? CPU1_time : CPU2_time;
			pthread_mutex_unlock(&time_mutex);

			// admit any newly-arrived jobs to the shared heap
			check_for_arrivals(pq, now);

			sched_yield();
		}
        pthread_mutex_lock(&time_mutex);
        current.completed_time = CPU1_time;
        pthread_mutex_unlock(&time_mutex);

        // record metrics
        int idx = current.index;
        waiting_times[idx]    = current.waiting_time;
        turnaround_times[idx] = current.completed_time - current.arrival_time;
        core_allocated[idx]   = 0;
    }
    return NULL;
}

static void* CPU1(void* arg) {
    PriorityQueue* pq = (PriorityQueue*)arg;

    for (;;) {
		Process current = threadSafeHeapPop(pq);

		int pending;
		pthread_mutex_lock(&mutex);
		pending = processesLeftInMasterList;
		pthread_mutex_unlock(&mutex);

		if (current.index == -1) {
			if (pending > 0) {
				int t = next_arrival_time();   // earliest arrival left
				advance_time_to_next(t);       // move earlier CPU clock to t
				continue;                      // then loop back to admit+pop again
			}
			break; // no heap items and nothing pending -> done
		}

		// align this core's clock to arrival, compute waiting
		pthread_mutex_lock(&time_mutex);
		if (CPU2_time < current.arrival_time) CPU2_time = current.arrival_time;
		current.waiting_time = CPU2_time - current.arrival_time;
		pthread_mutex_unlock(&time_mutex);

		// run non-preemptive
		for (int t = 0; t < current.burst_time; t++) {
			pthread_mutex_lock(&time_mutex);
			CPU2_time++;
			int now = (CPU1_time > CPU2_time) ? CPU1_time : CPU2_time;
			pthread_mutex_unlock(&time_mutex);

			check_for_arrivals(pq, now);

			sched_yield();

			}

        pthread_mutex_lock(&time_mutex);
        current.completed_time = CPU2_time;
        pthread_mutex_unlock(&time_mutex);

        int idx = current.index;
        waiting_times[idx]    = current.waiting_time;
		turnaround_times[idx] = current.completed_time - current.arrival_time;
        core_allocated[idx]   = 1;
    }
    return NULL;
}


int main(int argc, char* argv[]) {

	pthread_mutex_init(&mutex, NULL);
	pthread_mutex_init(&time_mutex, NULL);

	processesLeftInMasterList = 0;

	initialize_master_list(); //initializing master process list
	PriorityQueue* pq = createPriorityQueue(N); //creating priority queue of size N for eligible processes

	// Find the earliest arrival time
	int earliest = arrival[0];
	for (int i = 1; i < N; ++i)
		if (arrival[i] < earliest)
			earliest = arrival[i];

	check_for_arrivals(pq, earliest);
	advance_time_to_next(earliest);

	
	pthread_t t0, t1;
	pthread_create(&t0, NULL, CPU0, (void*)pq);
	pthread_create(&t1, NULL, CPU1, (void*)pq);

	pthread_join(t0, NULL);
	pthread_join(t1, NULL);

	// Calculate average waiting time and turnaround time
	double avg_waiting_time = 0.0;
	double avg_turnaround_time = 0.0;

	for (int i = 0; i < N; i++) {
		avg_waiting_time += waiting_times[i];
		avg_turnaround_time += turnaround_times[i];
	}
	avg_waiting_time /= N;
	avg_turnaround_time /= N;


	for (int i = 0; i < N; i++) {
		printf("Process: %s Arrival: %d Burst: %d CPU: %d Waiting Time: %d Turnaround Time: %d\n", names[i], arrival[i], burst[i], core_allocated[i], waiting_times[i], turnaround_times[i]);
	}
	printf("Average waiting time: %.2f\n", avg_waiting_time);
	printf("Average turnaround time: %.2f\n", avg_turnaround_time);

	// Destroy the mutex
	pthread_mutex_destroy(&time_mutex);
	pthread_mutex_destroy(&mutex);
	if (pq) {
		free(pq->arr);
		free(pq);
	}
	return 0;
}
