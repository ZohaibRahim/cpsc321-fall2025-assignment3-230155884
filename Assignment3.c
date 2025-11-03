#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

static const int N = 5;
static const char* names[] = { "P1","P2","P3","P4","P5" }; // the process identifiers 
static const int   arrival[] = { 0,   1,   2,   3,   4 }; // the arrival time of each process (in time units). 
static const int   burst[] = { 10,   5,   8,   6,   3 }; // the burst time (execution time in time units) of each process.
static int waiting_times[N];    // Array to store waiting times of processes
static int turnaround_times[N]; // Array to store turnaround times of processes
static int core_allocated[N]; // Array to store allocted core number for each process
static int completion_times[N]; // Array to store completion times of processes
static int CPU1_time = 0; // Total time consumed by CPU core 1
static int CPU2_time = 0; // Total time consumed by CPU core 2

// Synchronization Primitives
pthread_mutex_t mutex; // Mutex for thread safety
pthread_mutex_t queue_mutex;        // Protects the ready_queue
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
int processesLeftInMasterList;

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
static void check_for_arrivals(PriorityQueue* pq, int current_cpu_time) {
	pthread_mutex_lock(&queue_mutex);	// LOCK the shared queue before manipulation

	// Iterate through master list and move eligible processes
	for (int i = 0; i < processesLeftInMasterList; ){
		if (master_process_list[i].arrival_time <= now) {
			heapPush(pq, master_process_list[i]); // Add to priority queue
			deleteElement(i);
		}
		else
			i++; // Only increment if no deletion
	}
	pthread_mutex_unlock(&queue_mutex);	// UNLOCK the queue
}


// Function to create a new priority queue
static PriorityQueue* createPriorityQueue(int capacity) {
	PriorityQueue* pq = (PriorityQueue*)malloc(sizeof(PriorityQueue));
	pq->capacity = capacity;
	pq->arr = (Process*)malloc(pq->capacity * sizeof(Process));
	pq->size = 0;
	return pq;
}

// Function to check if the queue is empty
int isEmpty(PriorityQueue* pq) {
	return pq->size == 0;
}

// Function to maintain the min - heap property from a given index
void minHeapify(PriorityQueue * pq, int idx) {
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

// Function to extract the minimum process safely using a mutex lock
static Process threadSafeHeapPop(PriorityQueue* pq) {
	pthread_mutex_lock(&mutex);
	Process p = heapPop(pq);
	pthread_mutex_unlock(&mutex);
	return p;
}

// Thread functions for CPU core 1
static int* CPU0(void* arg) {
	PriorityQueue* pq = (PriorityQueue*)arg;

	// Loop to continuously try to extract processes until the queue is empty or a stop condition is met
	while (1) {
		pthread_mutex_lock(&time_mutex);
		int now = CPU1_time;
		pthread_mutex_unlock(&time_mutex);

		check_for_arrivals(pq, now);

		Process current = threadSafePop(pq);
		if (current.index == -1) 
			break;	// Queue was empty, all processes are done, breaking the loop

		pthread_mutex_lock(&time_mutex);
		if (CPU1_time < cur.arrival_time)
			CPU1_time = current.arrival_time;
		current.waiting_time = CPU1_time - current.arrival_time;
		CPU1_time += current.burst_time;
		current.completed_time = CPU1_time;
		pthread_mutex_unlock(&time_mutex);

		// Recording process metrics by index
		int idx = current.index;
		waiting_times[idx] = current.waiting_time;
		turnaround_times[idx] = current.waiting_time + current.burst_time;
		completion_times[idx] = current.completed_time;
		core_allocated[idx] = 0;
	}

	return NULL;
}

// Thread functions for CPU core 2
static int* CPU1(void* arg) {
	PriorityQueue* pq = (PriorityQueue*)arg;

	// Loop to continuously try to extract processes until the queue is empty or a stop condition is met
	while (1) {
		pthread_mutex_lock(&time_mutex);
		int now = CPU2_time;
		pthread_mutex_unlock(&time_mutex);

		check_for_arrivals(pq, now);

		Process current = threadSafePop(pq);
		if (current.index == -1)
			break;	// Queue was empty, all processes are done, breaking the loop

		pthread_mutex_lock(&time_mutex);
		if (CPU2_time < cur.arrival_time)
			CPU2_time = current.arrival_time;
		current.waiting_time = CPU2_time - current.arrival_time;
		CPU2_time += current.burst_time;
		current.completed_time = CPU2_time;
		pthread_mutex_unlock(&time_mutex);

		// Recording process metrics by index
		int idx = current.index;
		waiting_times[idx] = current.waiting_time;
		turnaround_times[idx] = current.waiting_time + current.burst_time;
		completion_times[idx] = current.completed_time;
		core_allocated[idx] = 1;
	}

	return NULL;
}


int main(int argc, char* argv[]) {

	pthread_mutex_init(&mutex, NULL);
	pthread_mutex_init(&queue_mutex, NULL);
	pthread_mutex_init(&time_mutex, NULL);

	initialize_master_list(); //initializing master process list
	PriorityQueue* pq = createPriorityQueue(N); //creating priority queue of size N for eligible processes

	pthread_t t0, t1;
	pthread_create(&t0, NULL, CPU0, (void*)pq);
	pthread_create(&t1, NULL, CPU1, (void*)pq);
	pthread_join(t0, NULL);
	pthread_join(t1, NULL);

	// Initialize the mutex
	pthread_mutex_init(&mutex, NULL);



	//logic to call the functions

	// Calculate turnaround times
	for (int i = 0; i < N; i++) {
		processes[i].turnaround_time = completion_times[i] - waiting_times[i];
	}

	// Calculate average waiting time and turnaround time
	double avg_waiting_time = 0;
	double avg_turnaround_time = 0;

	for (int i = 0; i < N; i++) {
		avg_waiting_time += (double)processes[i].waiting_time;
		avg_turnaround_time += (double)processes[i].turnaround_time;
	}
	avg_waiting_time /= N;
	avg_turnaround_time /= N;


	for (int i = 0; i < N; i++) {
		printf("Process: %s Arrival: %d Burst: %d CPU: %d Waiting Time: %d Turnaround Time: %d\n", processes[i].name, processes[i].arrival_time, processes[i].burst_time, processes[i].core_allocated, processes[i].waiting_time, processes[i].turnaround_time);
	}
	printf("Average waiting time: %.2f\n", avg_waiting_time / N);
	printf("Average turnaround time: %.2f\n", avg_turnaround_time / N);

	// Destroy the mutex
	pthread_mutex_destroy(&mutex);

	return 0;
}
