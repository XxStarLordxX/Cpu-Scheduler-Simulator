/*
Family Name: Sun
Given Name: Ruojie
Section: A
Student Number: 213107313
CS Login: rogeysun
*/


#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<math.h>
#include"sch-helpers.h"

int minimum(int a, int b);
int maximum(int a, int b);
void toReadyQueue(void);
void timeShift(void);
void outOfCPU(void);
void ReadyQueueToCPU(void);
void computeResult(void);
int checkBurstFinished(process *p);
void toTempQueue(process *p, int *nextSlot, process *tempQueue[]);
void sortAndClearTempQueue(int *nextSlot, process *tempQueue[], process_queue *readyQueue);
int lowestPriority(process *p);

process_queue *readyQueue1;							 // ready queue with high priority.
process_queue *readyQueue2;							 // ready queue with medium priority.
process_queue *readyQueue3;							 // ready queue with low priority.
process_queue *deviceQueue;
process processes[MAX_PROCESSES+1];
struct processor CPU[NUMBER_OF_PROCESSORS];
int numberOfProcesses = 0;
int time = 0;
int nextArrivingProcess = 0;
int numberOfRunningProcessors = 0;
int contextSwitch = 0;
int timeQuantum1;							 // Time quantum of first ready queue.
int timeQuantum2;							 // Time quantum of second ready queue.
process *tempQueue1[20];
process *tempQueue2[20];
process *tempQueue3[20];
int nextSlot1 = 0;                            // index of next available slot of the tempQueue1.
int nextSlot2 = 0;                            // index of next available slot of the tempQueue2.
int nextSlot3 = 0;                            // index of next available slot of the tempQueue3.

int main(int argc, char* argv[]){
	int i;
	timeQuantum1 = atoi(argv[1]);
	timeQuantum2 = atoi(argv[2]);
	int status;

	while ((status = readProcess(&processes[numberOfProcesses])))  {
         if(status==1)  numberOfProcesses ++;
    }   // it reads pid, arrival_time, bursts to one element of the above struct array


    qsort(processes, numberOfProcesses, sizeof(process), compareByArrival);  // Sort the processes by the arrival time.

    readyQueue1 = (process_queue *)malloc(sizeof(process_queue));
    readyQueue2 = (process_queue *)malloc(sizeof(process_queue));
    readyQueue3 = (process_queue *)malloc(sizeof(process_queue));
    deviceQueue = (process_queue *)malloc(sizeof(process_queue));

    initializeProcessQueue(readyQueue1);  				// initialize the ready queue1, which contains the process with the highest priority.
    initializeProcessQueue(readyQueue2);  				// initialize the ready queue2, which contains the process with the medimum priority.
    initializeProcessQueue(readyQueue3);  				// initialize the ready queue3, which contains the process with the lowest priority.
    initializeProcessQueue(deviceQueue);				// initialize the device queue.
    while(!(readyQueue1->size == 0 && deviceQueue->size == 0 && readyQueue2->size == 0 && readyQueue3->size == 0 &&
    		numberOfRunningProcessors == 0 && nextArrivingProcess == numberOfProcesses)){

    	/* If a process finished on its CPU burst and does not terminated, move it into ready queue. Eles check if the current burst time reaches the time quantum.
    	Otherwise, terminates this process. */
    	outOfCPU();
    	
    	/* If there is a process finished IO burst or the arrrival time of that process is reached, put it into ready queue. */
    	toReadyQueue();

    	/* If the ready queue is not empty, send the first process in ready queue to a idle CPU. At the same time, check whether there is a new process
    	arrives at the ready queue with high priorities. If there is a running process which has a lower priority than the newly arrived process, the running 
    	process with lowest priority must be stopped and return to the head of the ready queue where it comes. Then the newly arrived process will be sent to 
    	that CPU to process.*/
    	ReadyQueueToCPU();

    	/* Shift one time unit. */
    	timeShift();
    }
    
    computeResult();

    return 0;
}


void ReadyQueueToCPU(void){
	int i = 0;
	int j = 0;

	for (i = 0; i < NUMBER_OF_PROCESSORS; i++){
		/* When there is a idle CPU and ready queue 1 is not empty. */
		if(readyQueue1->size != 0 && numberOfRunningProcessors != 4){
			if(!CPU[i].isOccupied){
				CPU[i].process = readyQueue1->front->data;
				CPU[i].process->quantumRemaining = timeQuantum1;
				if(readyQueue1->front->data->currentBurst == 0 && readyQueue1->front->data->hasStarted == 0){
					readyQueue1->front->data->startTime = time;
					readyQueue1->front->data->hasStarted = 1;
				}
				CPU[i].isOccupied = 1;
				numberOfRunningProcessors++;
				dequeueProcess(readyQueue1);
			}
		}
		/* When there is no idle CPUs and ready queue 1 is not empty, if there is a runnning process came from ready queue 2 or 3, preempt that process and insert 
		the first process of ready queue 1 to the available CPU. */
		else if(readyQueue1->size != 0 && numberOfRunningProcessors == 4){
			
			/* First, find out which running process has the lowest priority. */
			if(i == lowestPriority(readyQueue1->front->data)){
				/* Second, check which queue that process is in. */
				if (CPU[i].process->currentQueue == 2){
					/* Put this process back to the head of the ready queue which it came from. Then assign the process with higher priority to the current CPU. */
					CPU[i].process->interrupted = 1;
					enqueueProcess(readyQueue2, CPU[i].process);
					CPU[i].process = readyQueue1->front->data;
					CPU[i].process->quantumRemaining = timeQuantum1;
					if(readyQueue1->front->data->currentBurst == 0 && readyQueue1->front->data->hasStarted == 0){
						readyQueue1->front->data->startTime = time;
						readyQueue1->front->data->hasStarted = 1;
					}
					for (j = 0; j < readyQueue2->size - 1; j++){
						enqueueProcess(readyQueue2, readyQueue2->front->data);
						dequeueProcess(readyQueue2);
					}
					dequeueProcess(readyQueue1);
				}
				else{
					enqueueProcess(readyQueue3, CPU[i].process);
					CPU[i].process = readyQueue1->front->data;
					CPU[i].process->quantumRemaining = timeQuantum1;
					if(readyQueue1->front->data->currentBurst == 0 && readyQueue1->front->data->hasStarted == 0){
						readyQueue1->front->data->startTime = time;
						readyQueue1->front->data->hasStarted = 1;
					}
					for (j = 0; j < readyQueue3->size - 1; j++){
						enqueueProcess(readyQueue3, readyQueue3->front->data);
						dequeueProcess(readyQueue3);
					}
					dequeueProcess(readyQueue1);
				}
				contextSwitch++;
			}
		}
		/* When there is a idle CPU and ready queue 1 is empty but ready queue 2 is not empty. */
		else if(readyQueue2->size != 0 && numberOfRunningProcessors != 4){
			if(!CPU[i].isOccupied){
				CPU[i].process = readyQueue2->front->data;
				if(CPU[i].process->interrupted != 1)
					CPU[i].process->quantumRemaining = timeQuantum2;
				else
					CPU[i].process->interrupted = 0;
				CPU[i].isOccupied = 1;
				numberOfRunningProcessors++;
				dequeueProcess(readyQueue2);
			}
		}
		/* No idle CPU. Ready queue 1 is empty and ready queue 2 is not. */
		else if(readyQueue2->size != 0 && numberOfRunningProcessors == 4){
			if(i == lowestPriority(readyQueue2->front->data)){
				enqueueProcess(readyQueue3, CPU[i].process);
				CPU[i].process = readyQueue2->front->data;
				if(CPU[i].process->interrupted != 1)
					CPU[i].process->quantumRemaining = timeQuantum2;
				else
					CPU[i].process->interrupted = 0;

				for (j = 0; j < readyQueue3->size - 1; j++){
					enqueueProcess(readyQueue3, readyQueue3->front->data);
					dequeueProcess(readyQueue3);
				}
				dequeueProcess(readyQueue2);
				contextSwitch++;
			}
		}
		/* There is a idle CPU and both of ready queue 1 and ready queue 2 are empty, while ready queue is not empty. */
		else if(!CPU[i].isOccupied && readyQueue3->size != 0){
			CPU[i].process = readyQueue3->front->data;
			/* For processes in ready queue 3, just set the quantumRemaining to a positive number. */
			CPU[i].process->quantumRemaining = 1000000;
			CPU[i].isOccupied = 1;
			numberOfRunningProcessors++;
			dequeueProcess(readyQueue3);
		}
	}
}

void outOfCPU(void){
	int i;
	process *temp;
	process *tempQueue2[20];
	process *tempQueue3[20];
	int nextSlot2 = 0;			// index of next available slot of the tempQueue2.
	int nextSlot3 = 0;			// index of next available slot of the tempQueue3.
	for(i = 0; i < NUMBER_OF_PROCESSORS; i++){
		if(CPU[i].isOccupied){
			/* Check if the current process in cpu has finished its current cpu burst. */
			if(checkBurstFinished(CPU[i].process)){
				temp = CPU[i].process;
				CPU[i].process = NULL;
				CPU[i].isOccupied = 0;
				numberOfRunningProcessors--;
				/* Process does not terminate. */
				if(!(temp->currentBurst == temp->numberOfBursts)){
					if(temp->currentQueue != 1)
						/* Two ways of increase the priority of a process. */
						temp->currentQueue--;
						//temp->currentQueue = 1;
					enqueueProcess(deviceQueue, temp);
				}
				/* Process terminates. */
				else{
					temp->endTime = time;                                                                                                                                 
				}
			}
			/* Check if the current process in cpu has finished its time quantum. */
			else if (!CPU[i].process->quantumRemaining){
				temp = CPU[i].process;
				CPU[i].process = NULL;
				CPU[i].isOccupied = 0;
				numberOfRunningProcessors--;
				contextSwitch++;
				if(temp->currentQueue == 1){
					tempQueue2[nextSlot2] = temp;
					nextSlot2++;
					temp->currentQueue = 2;
			    }
				else if(temp->currentQueue == 2){
					tempQueue3[nextSlot3] = temp;
					nextSlot3++;
					temp->currentQueue = 3;
				}
			}
		}
    	}
    	
    /* Sort the processes by pid then put them into ready queue. */
	if(nextSlot2 > 1){
		qsort(tempQueue2, nextSlot2, sizeof(process *), compareByPid);
	}
	for(i = 0; i < nextSlot2; i++){
		enqueueProcess(readyQueue2, tempQueue2[i]);
		tempQueue2[i] = NULL;

	}
	nextSlot2 = 0;

	if(nextSlot3 > 1){
		qsort(tempQueue3, nextSlot3, sizeof(process *), compareByPid);
	}
	for(i = 0; i < nextSlot3; i++){
		enqueueProcess(readyQueue3, tempQueue3[i]);
		tempQueue3[i] = NULL;

	}
	nextSlot3 = 0;
}

void toReadyQueue(void){;
	/* When a process is going to be put into ready queue, it will first be put into a temperary queue. This queue will be sorted based on the pid of its elements. */
	int i;
	int tempSize = deviceQueue->size;			  // current size of the deviceQueue.
	process *tempQueue1[20];
	int nextSlot1 = 0;			// index of next available slot of the tempQueue1.
	/* Check is there an arriving process. if yes, put it into temp queue. */
	if((processes[nextArrivingProcess].arrivalTime == time)){
		tempQueue1[nextSlot1] = &processes[nextArrivingProcess];
		nextSlot1++;
    		nextArrivingProcess++;
    	}

    	for(i = 0; i < tempSize; i++){
		/* If a process finished its IO burst, put it into temp queue. */
		if(checkBurstFinished(deviceQueue->front->data)){
			tempQueue1[nextSlot1] = deviceQueue->front->data;
			nextSlot1++;
			dequeueProcess(deviceQueue);
		}
		/* Else pop it out first then push it back to the deviceQueue. */
		else{
			enqueueProcess(deviceQueue, deviceQueue->front->data);
			dequeueProcess(deviceQueue);
		}
	    }


    /* If there are more than one process in the temp queue, sort the temp queue by pid and then put those processes into ready queue in order. */
    	if(nextSlot1 > 1){
		qsort(tempQueue1, nextSlot1, sizeof(process *), compareByPid);
	}
	for(i = 0; i < nextSlot1; i++){
		enqueueProcess(readyQueue1, tempQueue1[i]);
		tempQueue1[i] = NULL;

	}
	nextSlot1 = 0;

}

int checkBurstFinished(process *p){

	/* If the current burst of process p has finished, return 1; else return 0. */
	if(p->bursts[p->currentBurst].step == p->bursts[p->currentBurst].length){
		p->currentBurst++;
		return 1;
	}
	else
		return 0;
}

/* The steps of current burst of all waiting and running processes increase by one, and time move forward by one time unit. */
void timeShift(void){
	int i;
	time++;
	for (i = 0; i < NUMBER_OF_PROCESSORS; i++){
		if(CPU[i].isOccupied){
			CPU[i].process->bursts[CPU[i].process->currentBurst].step++;
			if(CPU[i].process->currentQueue == 1 || CPU[i].process->currentQueue == 2)
				CPU[i].process->quantumRemaining--;
			CPU[i].runningTime++;
		}
	}

	process_node *temp = deviceQueue->front;
	while(temp != NULL){
		temp->data->bursts[temp->data->currentBurst].step++;
		temp = temp->next;
	}

	temp = readyQueue1->front;
	while(temp != NULL){
		temp->data->waitingTime++;
		temp = temp->next;
	}
	temp = readyQueue2->front;
	while(temp != NULL){
		temp->data->waitingTime++;
		temp = temp->next;
	}
	temp = readyQueue3->front;
	while(temp != NULL){
		temp->data->waitingTime++;
		temp = temp->next;
	}
}


/* To find out which running process has the lowest priority. */
int lowestPriority(process *p){
	int min = -1;
	int minQueue = p->currentQueue;
	int maxLength;
	int i;
	
	for (i = 0; i < NUMBER_OF_PROCESSORS; i++){
		if(CPU[i].isOccupied && minQueue < CPU[i].process->currentQueue){
				 minQueue = CPU[i].process->currentQueue;
				 if(minQueue == 2)
				 	 maxLength = CPU[i].process->quantumRemaining;
				 else if(minQueue == 3)
				 	 maxLength = CPU[i].process->bursts[CPU[i].process->currentBurst].length - CPU[i].process->bursts[CPU[i].process->currentBurst].step;
				 min = i;		
		}
	}
	
	for (i = 0; i < NUMBER_OF_PROCESSORS; i++){
		
		if(CPU[i].isOccupied && minQueue == 2 && minQueue == CPU[i].process->currentQueue && maxLength < CPU[i].process->quantumRemaining){
				maxLength = CPU[i].process->quantumRemaining;
				min = i;
		}
		else if(CPU[i].isOccupied && minQueue == 3 && minQueue == CPU[i].process->bursts[CPU[i].process->currentBurst].length - CPU[i].process->bursts[CPU[i].process->currentBurst].step){
			maxLength = CPU[i].process->bursts[CPU[i].process->currentBurst].length - CPU[i].process->bursts[CPU[i].process->currentBurst].step;
			min = i;
		}
		
	}
	/*
	for (i = 0; i < NUMBER_OF_PROCESSORS; i++){
		if(CPU[i].isOccupied && minQueue == 2 && minQueue == CPU[i].process->currentQueue && maxLength == CPU[i].process->quantumRemaining && CPU[i].process->pid > maxPid){
			maxPid = CPU[i].process->pid;
			min  = i;
		}
		else if(CPU[i].isOccupied && minQueue == 2 && minQueue == CPU[i].process->currentQueue && maxLength == CPU[i].process->bursts[CPU[i].process->currentBurst].length - CPU[i].process->bursts[CPU[i].process->currentBurst].step && CPU[i].process->pid > maxPid){
			maxPid = CPU[i].process->pid;
			min  = i;
		}
	}
	*/
	return min;
}
int minimum(int a, int b){
	int min;
	min = a < b ?  a: b;
	return min;
}

int maximum(int a, int b){
	int max;
	max = a > b ?  a: b;
	return max;
}
void computeResult(void){
	int i;
	int max = 0;
	int totalWaitingTime = 0;
	double avgWaitingTime = 0;
	int totalTurnaroundTime = 0;
	double avgTurnaroundTime = 0;
	int totalCPURunningTime = 0;
	double cpuUtilization = 0;
	process *lastProcess;

	for (i = 0; i < numberOfProcesses; i++){;

		if(max < processes[i].endTime){
			max = processes[i].endTime;
			lastProcess = &processes[i];
		}
		totalWaitingTime += processes[i].waitingTime;
		totalTurnaroundTime += processes[i].endTime - processes[i].startTime;
	}
	avgWaitingTime = (double) totalWaitingTime / numberOfProcesses;
	avgTurnaroundTime = (double) totalTurnaroundTime / numberOfProcesses;
	for (i = 0; i < NUMBER_OF_PROCESSORS; i++){
		totalCPURunningTime += CPU[i].runningTime;
	}
	cpuUtilization = (double) totalCPURunningTime / time;

	printf("Average waiting time = %f\n"
		"Average turnaround time = %f\n"
		"Finish time = %d\n"
		"Average CPU utilization = %f%%\n"
		"Context switch = %d\n"
		"Pid of last process = %d\n",
		avgWaitingTime, avgTurnaroundTime, lastProcess->endTime, cpuUtilization * 100, contextSwitch, lastProcess->pid);
}

