# cpsc321-fall2025-assignment3-230155884

# CPSC 321 - Assignment 3  
**University of Northern British Columbia**  
**Course:** Operating Systems (Fall 2025)  
**Instructor:** Dr. Sajal Saha  
**Student:** Zohaib Rahim 

---

## 🧠 Overview
This project implements a **multi-CPU scheduling simulation** using the **Shortest Job First (SJF)** algorithm with **thread synchronization** (mutexes/semaphores).  
Each CPU is represented by a thread that selects and executes processes from a shared ready queue while ensuring safe concurrent access.

---

## 🎯 Objectives
- Extend CPU scheduling algorithms to support synchronization.
- Simulate multi-core scheduling using threads.
- Prevent race conditions with mutex/semaphore protection.
- Calculate process waiting and turnaround times.
- Demonstrate average performance metrics.

---

## ⚙️ Features
- Two worker threads represent two CPU cores.
- Shared ready queue protected by mutex.
- Non-preemptive **Shortest Job First (SJF)** scheduling.
- Dynamic handling of any number of processes (`N`).
- Detailed per-process output with assigned CPU, waiting time, and turnaround time.
- Calculation of average waiting and turnaround times.

---

## 🧩 Input Format
All process data is defined statically inside the program:

```c
static const int N = 5;
static const char* names[]  = {"P1","P2","P3","P4","P5"};
static const int   arrival[] = {0, 1, 2, 3, 4};
static const int   burst[]   = {10, 5, 8, 6, 3};x 
```
## 🧩 Output Format
Process: P1 Arrival: 0 Burst: 10 CPU: 0 Waiting Time: 0 Turnaround Time: 10
Process: P2 Arrival: 1 Burst: 5  CPU: 1 Waiting Time: 0 Turnaround Time: 5
Process: P3 Arrival: 2 Burst: 8  CPU: 0 Waiting Time: 8 Turnaround Time: 16
Process: P4 Arrival: 3 Burst: 6  CPU: 1 Waiting Time: 6 Turnaround Time: 12
Process: P5 Arrival: 4 Burst: 3  CPU: 1 Waiting Time: 2 Turnaround Time: 5
Average waiting time = 3.20  
Average turnaround time = 9.60

