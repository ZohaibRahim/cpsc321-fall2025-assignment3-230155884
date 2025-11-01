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

pthread_mutex_t mutex; // Mutex for thread safety

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

// Structure to represent the Priority Queue (Min-Heap)
typedef struct PriorityQueue {
	Process* arr; // Array to store heap elements
	int size;      // Current number of elements in the heap
	int capacity;  // Maximum capacity of the heap
} PriorityQueue;

// Helper function to swap two processes
void swap(Process* p1, Process* p2) {
	Process temp = *p1;
	*p1 = *p2;
	*p2 = temp;
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

// Function to extract the process with the minimum burst time (the top of the heap)
Process extractMin(PriorityQueue* pq) {
	if (isEmpty(pq)) {
		Process empty = { -1, -1 }; // Return an indicator of an empty queue
		return empty;
	}

	// Store the minimum process
	Process root = pq->arr[0];

	// Replace root with the last element
	pq->arr[0] = pq->arr[pq->size - 1];
	pq->size--;

	// Maintain the min-heap property from the root
	minHeapify(pq, 0);

	return root;
}

// Function to extract the minimum process safely using a mutex lock
// Acquires the global mutex before calling extractMin, ensuring thread safety,
// then releases the lock after the operation is complete.
Process threadSafeExtractMin(PriorityQueue* pq, int CPU) {
	pthread_mutex_lock(&mutex);
	Process p = extractMin(pq);
	pthread_mutex_unlock(&mutex);
	return p;
}

void executeProcess(Process p) {
	// Simulate process execution
}

// Thread functions for CPU core 1
void* CPU0(void* arg) {
	threadSafeExtractMin(NULL, 0); // Extract the minimum process safely

	return NULL;
}

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


	struct Process processes[N];

	for (int i = 0; i < N; i++) {
		processes[i].id = i;
		processes[i].name = NAMES[i];
		processes[i].arrival_time = ARRIVAL[i];
		processes[i].burst_time = BURST[i];
	}

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
