#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

pthread_mutex_t mutex;

int main(int argc, char* argv[]) {

	// Process data
	static const int N = 5;
	static const char* names[] = { "P1","P2","P3","P4","P5" }; // the process identifiers 
	static const int   arrival[] = { 0,   1,   2,   3,   4 }; // the arrival time of each process (in time units). 
	static const int   burst[] = { 10,   5,   8,   6,   3 }; // the burst time (execution time in time units) of each process. 


	pthread_t threads[2]; 
	int thread_index[2];

  //creating 2 threads for 2 cores
	pthread_create(&threads[0], NULL, NULL, NULL);
	pthread_create(&threads[1], NULL, NULL, NULL);

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
