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
static int current_cpu_time = 0; // Shared current CPU time across cores

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
		if (arrival[i] <= current_cpu_time) {
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

static Process deleteElement(int indexToDelete) {
	// Check if the index is valid
	if (indexToDelete < 0 || indexToDelete >= processesLeftInMasterList) {
		return;
	}

	// Shift elements to the left
	for (int i = indexToDelete; i < processesLeftInMasterList - 1; i++) {
		master_process_list[i] = master_process_list[i + 1];
	}

	processesLeftInMasterList--;
	return master_process_list[indexToDelete];
}

Process eligibleProcesses[N];

// Function to check for newly arrived processes and add them to the eligible process list
static void check_for_arrivals() {
	pthread_mutex_lock(&queue_mutex);	// LOCK the shared queue before manipulation
	
	// Iterate through master list and move eligible processes
	int i = 0;	
	while (processesLeftInMasterList>0 && master_process_list[0].arrival_time <= current_cpu_time) {
		eligibleProcesses[i].index = master_process_list.i;
		eligibleProcesses[i].name = master_process_list.names[i];
		eligibleProcesses[i].arrival_time = master_process_list.arrival[i];
		eligibleProcesses[i].burst_time = master_process_list.burst[i];

		eligibleProcesses[i].waiting_time = 0;
		eligibleProcesses[i].turnaround_time = 0;
		eligibleProcesses[i].core_allocated = -1; // -1 means not allocated yet
		eligibleProcesses[i].completed_time = 0;

		processesLeftInMasterList++;
	
		deleteElement(i);

		i++;
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

// Function to extract the process with the minimum burst time (the top of the heap) for CPU1
Process extractMin_CPU1(PriorityQueue* pq) {
	if (isEmpty(pq)) {
		Process empty = { -1, -1 }; // Return an indicator of an empty queue
		return empty;
	}

	// Store the minimum process
	Process root = pq->arr[i];

	// Replace root with the last element
	pq->arr[0] = pq->arr[pq->size - 1];
	pq->size--;

	// Maintain the min-heap property from the root
	minHeapify(pq, 0);

	return root;
}

// Function to extract the process with the minimum burst time (the top of the heap) for CPU2
Process extractMin_CPU2(PriorityQueue* pq) {
	if (isEmpty(pq)) {
		Process empty = { -1, -1 }; // Return an indicator of an empty queue
		return empty;
	}

	// Store the minimum process
	int i = 0;
	while (CPU2_time < arr[1].arrival_time)
		i++;
	Process root = pq->arr[0];

	// Replace root with the last element
	pq->arr[0] = pq->arr[pq->size - 1];
	pq->size--;

	// Maintain the min-heap property from the root
	minHeapify(pq, 0);

	return root;
}

// Function to extract the minimum process safely using a mutex lock
Process threadSafeExtractMin(PriorityQueue* pq, int CPU) {
	pthread_mutex_lock(&mutex);
	Process p = extractMin(pq);
	pthread_mutex_unlock(&mutex);
	return p;
}

// Thread functions for CPU core 1
static int* CPU0(void* arg) {
	// Loop to continuously try to extract processes until the queue is empty or a stop condition is met
	while (1) {
		Process current_process = threadSafeExtractMin();	// Call the thread-safe function to get the next process

		if (current_process.index == -1) {
			break;// Queue was empty, all processes are done, breaking the loop
		}

		// If a valid process was returned, then accessing its members
		current_process.core_allocated = 0; // Process allocated to core 0 
		if (CPU1_time < current_process.arrival_time) {
			CPU1_time = current_process.arrival_time; // If CPU is idle, move time to process arrival so process only happens after arrival
		}
		current_process.waiting_time = CPU1_time - current_process.arrival_time;
		for (int i=1; i<=current_process.burst_time; i++) {
			CPU1_time++; // Increment CPU time for each unit of burst time

		}
		current_process.completed_time = CPU1_time; // Update completion time
	}
	return CPU1_time;
}2.

// Thread functions for CPU core 2
void* CPU1(void* arg) {
	threadSafeExtractMin(NULL,1); // Extract the minimum process safely
	return NULL;
}

int main(int argc, char* argv[]) {

	pthread_t threads[2]; //creating 2 threads for 2 cores
	int thread_index[2];

	pthread_create(&threads[0], NULL, CPU0, NULL);
	pthread_create(&threads[1], NULL, CPU1, NULL);

	for (int i = 0; i < 2; i++) {
		thread_index[i] = i;
		pthread_create(&threads[i], NULL, NULL, &thread_index[i]);
	}

	for (int i = 0; i < 2; i++)
		pthread_join(threads[i], NULL);

	// Initialize the mutex
	pthread_mutex_init(&mutex, NULL);

	initialize_master_list(); //initializing master process list

	//logic to call the functions
	createPriorityQueue(N); //creating priority queue of size N

	// Calculate turnaround times
	for (int i = 0; i < N; i++) {
		processes[i].turnaround_time = completion_times[i] - waiting_times[i];
	}

	// Calculate average waiting time and turnaround time
	float avg_waiting_time = 0;
	float avg_turnaround_time = 0;
	for (int i = 0; i < N; i++) {
		avg_waiting_time += (float)processes[i].waiting_time;
		avg_turnaround_time += (float)processes[i].turnaround_time;
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
