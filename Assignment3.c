#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

pthread_mutex_t mutex;

// Structure to represent a process
typedef struct Process {
	static const int N = 5;
	static const char* names[] = { "P1","P2","P3","P4","P5" }; // the process identifiers 
	static const int   arrival[] = { 0,   1,   2,   3,   4 }; // the arrival time of each process (in time units). 
	static const int   burst[] = { 10,   5,   8,   6,   3 }; // the burst time (execution time in time units) of each process. 
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
PriorityQueue* createPriorityQueue(int capacity) {
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

void insertProcess(PriorityQueue* pq, Process p) {
	if (pq->size == pq->capacity) {
		// Optional: resize the array if needed
		printf("Priority queue is full. Cannot insert.\n");
		return;
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

int main(int argc, char* argv[]) {

	pthread_t threads[2]; //creating 2 threads for 2 cores
	int thread_index[2];

	pthread_create(&threads[0], NULL, increment, NULL);
	pthread_create(&threads[1], NULL, decrement, NULL);

	for (int i = 0; i < 2; i++) {
		thread_index[i] = i;
		pthread_create(&threads[i], NULL, NULL, &thread_index[i]);
	}

	for (int i = 0; i < 2; i++)
		pthread_join(threads[i], NULL);

	// Initialize the mutex
	pthread_mutex_init(&mutex, NULL);

	return 0;
}
