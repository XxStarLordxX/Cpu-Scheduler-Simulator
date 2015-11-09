#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include"sch-helpers.h"


void toReadyQueue(void);
void timeShift(void);
void cpuToDeviceQueue(void);
void ReadyQueueToCPU(void);
void computeResult(void);
int checkBurstFinished(process *p);

process_queue *readyQueue;
process_queue *deviceQueue;
process processes[MAX_PROCESSES+1];
struct processor CPU[NUMBER_OF_PROCESSORS];
int numberOfProcesses = 0; 
int time = 0;
int nextArrivingProcess = 0;
int numberOfRunningProcessors = 0;
int contextSwitch = 0;


int main(){
	int i;
	int status;
	while ((status = readProcess(&processes[numberOfProcesses])))  {
         if(status==1)  numberOfProcesses ++;
    }   // it reads pid, arrival_time, bursts to one element of the above struct array
    
    qsort(processes, numberOfProcesses, sizeof(process), compareByArrival);  // Sort the processes by the arrival time.
    
    readyQueue = (process_queue *)malloc(sizeof(process_queue));
    deviceQueue = (process_queue *)malloc(sizeof(process_queue));
    
    initializeProcessQueue(readyQueue);  				// initialize the ready queue.
    initializeProcessQueue(deviceQueue);				// initialize the device queue.
    /*
    CPU[0].process = &processes[nextArrivingProcess];
    CPU[0].process->startTime = time;
    CPU[0].isOccupied = 1;
    CPU[0].currentPid = CPU[0].process->pid;
    numberOfRunningProcessors++;
    nextArrivingProcess++;
    */
    
    while(!(readyQueue->size == 0 && deviceQueue->size == 0 && numberOfRunningProcessors == 0 && nextArrivingProcess == numberOfProcesses)){
    	
    	
    	/* If a process finished on its CPU burst and does not terminated, move it into ready queue. Eles terminates this process. */
    	cpuToDeviceQueue();
    	    
    	/* If there is a process finished IO burst or the arrrival time of that process is reached, put it into ready queue. */
    	toReadyQueue();
    	
    	/* If the ready queue is not empty, send the first process in ready queue to a idle CPU */
    	ReadyQueueToCPU();
    	
    	/* Shift one time unit. */
    	timeShift();
    	
    	//printf("Time: %d, Process #%d, Current=%d, Total=%d, step=%d, running processors: %d\n", 
    		//time, processes[16].pid, processes[16].currentBurst, processes[16].numberOfBursts,  processes[16].bursts[processes[16].currentBurst].step, numberOfRunningProcessors);
    }
    for (i = 0; i < numberOfProcesses; i++)
    	printf("Process #%d: Arrival=%d, Start=%d, End=%d, Waiting=%d\n", 
    		processes[i].pid, processes[i].arrivalTime, processes[i].startTime, processes[i].endTime, processes[i].waitingTime);
    computeResult();
    
    return 0;
}


void ReadyQueueToCPU(void){
	int i = 0;
	while (!(readyQueue->size == 0 || numberOfRunningProcessors == NUMBER_OF_PROCESSORS) && i < 4){
		
		if(!CPU[i].isOccupied){
			CPU[i].process = readyQueue->front->data;
			if(readyQueue->front->data->currentBurst == 0)
				readyQueue->front->data->startTime = time;
			CPU[i].isOccupied = 1;
			numberOfRunningProcessors++;
			dequeueProcess(readyQueue);
		}
		i++;
	}
}

void cpuToDeviceQueue(void){
	int i;
	
	process *temp;
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
					enqueueProcess(deviceQueue, temp);
				}
				/* Process terminates. */
				else{
					temp->endTime = time;
				}
			}
		}
	}
}

void toReadyQueue(void){;
	/* When a process is going to be put into ready queue, it will first be put into a temperary queue. This queue will be sorted based on the pid of its elements. */
	process *tempQueue[10];
	int nextSlot = 0;                            // index of next avaliable slot of the tempQueue.
	int i;
	int tempSize = deviceQueue->size;			  // current size of the deviceQueue.
	/* Check is there a arriving process. if yes, put it into temp queue. */
	if((processes[nextArrivingProcess].arrivalTime == time)){
    		tempQueue[nextSlot] = &processes[nextArrivingProcess];
    		nextSlot++;
    		nextArrivingProcess++;
    		
    }
    
    for(i = 0; i < tempSize; i++){
    	/* If a process finished its IO burst, put it into temp queue. */
    	if(checkBurstFinished(deviceQueue->front->data)){
    		tempQueue[nextSlot] = deviceQueue->front->data;
    		nextSlot++;
    		dequeueProcess(deviceQueue);
    	}
    	/* Else pop it out first then push it back to the deviceQueue. */
    	else{
    		enqueueProcess(deviceQueue, deviceQueue->front->data);
    		dequeueProcess(deviceQueue);
    	}
    }
    
    /* If there are more than one process in the temp queue, sort the temp queue by pid and then put those processes into ready queue in order. */
    if(nextSlot > 1){
    	 qsort(tempQueue, nextSlot, sizeof(process *), compareByPid);
    }
    for(i = 0; i < nextSlot; i++){
    	enqueueProcess(readyQueue, tempQueue[i]);
    	tempQueue[i] = NULL;
    	
    }
    	
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
			CPU[i].runningTime++;
		}
	}
	
	process_node *temp = deviceQueue->front;
	while(temp != NULL){
		temp->data->bursts[temp->data->currentBurst].step++;
		temp = temp->next;
	}
	
	temp = readyQueue->front;
	while(temp != NULL){
		temp->data->waitingTime++;
		temp = temp->next;
	}
}

void computeResult(void){
	int i;
	int max = 0;
	int totalWaitingTime = 0;
	double avgWaitingTime;
	int totalTurnaroundTime;
	double avgTurnaroundTime;
	int totalCPURunningTime;
	double cpuUtilization;
	int lastProcess;
	
	for (i = 0; i < numberOfProcesses; i++){;
		if(max < processes[i].endTime){
			max = processes[i].endTime;
			lastProcess = processes[i].pid;
		}
		totalWaitingTime += processes[i].waitingTime;
		totalTurnaroundTime += processes[i].endTime - processes[i].startTime;
	}
	avgWaitingTime = (double) totalWaitingTime / numberOfProcesses;
	avgTurnaroundTime = (double) totalTurnaroundTime / numberOfProcesses;
	for (i = 0; i < NUMBER_OF_PROCESSORS; i++){
		totalCPURunningTime += CPU[i].runningTime;
	}
	printf("totalWaitingTime = %d\nnumberOfProcesses = %d\n", totalWaitingTime, numberOfProcesses);
	cpuUtilization = (double) totalCPURunningTime / time;
	printf("Average waiting time = %f\nAverage turnaround time = %f\nFinish time = %d\nAverage CPU utilization = %f\%\nContext switch = %d\nPid of last process = %d\n", 
		avgWaitingTime, avgTurnaroundTime, time, cpuUtilization * 100, contextSwitch, lastProcess);
	
}
