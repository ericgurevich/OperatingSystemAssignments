/*
 ============================================================================
 Name        : discrete.c
 Author      : Eric Gurevich
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//Sim structs------------------------
struct Event {
	int id;
	int type;	//0 arrives, 1 finishes CPU, 2 finishes disk 1
	//3 finishes disk 2
	int time;
};

struct Device {
	int status;	//0 free 1 busy
	struct FIFO *f;
};

//FIFO structs----------------------
struct FIFO {
	struct FIFOnode *front, *rear;
};

struct FIFOnode {
	struct Event *e;
	struct FIFOnode *next;
};

//PriorityQ structs-----------------------
struct PQ {
	struct PQnode *front;
};

struct PQnode {
	struct Event *e;
	struct PQnode *next;
};

/*
 *
 *
 *
 *
 * function declarations-------------------------
 */
//
struct FIFO *newFIFO();
void qFIFO(struct FIFO *fifo, struct Event *Event);
struct Event *dqFIFO(struct FIFO *fifo);
struct Event *peekFIFO(struct FIFO *fifo);
struct Event *peekFIFO(struct FIFO *fifo);
struct PQ *newPQ();
void qPQ(struct PQ *pq, struct Event *event);
struct Event *dqPQ(struct PQ *pq);
struct Event *peekPQ(struct PQ *pq);

//get config from file
void readSetConfigVars(char *filename);

//Handlers
void handleEvent(struct Event *e);

void processCPU(struct Event *e);
void processDisk(int diskno, struct Event *e);


void eventArrives(struct Event *e);
void eventFinishesCPU(struct Event *e);
void eventFinishesD1(struct Event *e);
void eventFinishesD2(struct Event *e);
void eventExits(struct Event *e);

void printLine(char *s);
void loopSim();

int* getQSize();

/*
 *
 *
 *
 *
 * actual code ------------------------------
 */

//global struct pointers
struct FIFO *cpufifo;
struct Device cpu;

struct FIFO *disk1fifo;
struct Device disk1;

struct FIFO *disk2fifo;
struct Device disk2;

struct PQ *pq;

//global config vars
int SEED, INIT_TIME,FIN_TIME, ARRIVE_MIN, ARRIVE_MAX, CPU_MIN, CPU_MAX, DISK1_MIN, DISK1_MAX, DISK2_MIN, DISK2_MAX;
float QUIT_PROB;

//global current time
int currtime;

int main(void) {
	readSetConfigVars("src/RUNS.txt");
	srand(SEED);

	cpufifo = newFIFO();
	cpu.status = 0;
	cpu.f = cpufifo;

	disk1fifo = newFIFO();
	disk1.status = 0;
	disk1.f = disk1fifo;

	disk2fifo = newFIFO();
	disk1.status = 0;
	disk2.f = disk2fifo;

	pq = newPQ();

	loopSim();
}

//FIFO functions-----------------------

struct FIFO *newFIFO() {
	struct FIFO *fifo = (struct FIFO*) malloc(sizeof(struct FIFO));
	fifo->front = NULL;
	fifo->rear = NULL;
	return fifo;
}

void qFIFO(struct FIFO *fifo, struct Event *Event) {
	struct FIFOnode *node = (struct FIFOnode*) malloc(sizeof(struct FIFOnode));
	node->e = Event;
	node->next = NULL;

	if (fifo->rear == NULL) {
		fifo->front = fifo->rear = node;
	} else {
		fifo->rear->next = node;
		fifo->rear = node;
	}
}

struct Event *dqFIFO(struct FIFO *fifo) {

	if (fifo->front == NULL) {
		return NULL;
	}

	struct FIFOnode *temp = fifo->front;
	fifo->front = temp->next;

	if (fifo->front == NULL) {
		fifo->rear = NULL;
	}
	struct Event *toreturn = temp->e;
	free(temp);
	return toreturn;
}

struct Event *peekFIFO(struct FIFO *fifo) {

	if (fifo->front == NULL) {
			return NULL;
	}

	return fifo->front->e;
}

//PQ functions-----------------------------

struct PQ *newPQ() {
	struct PQ *pq = (struct PQ*) malloc(sizeof(struct PQ));
	pq->front = NULL;
	return pq;
}

void qPQ(struct PQ *pq, struct Event *event) {
	struct PQnode *pqnode = (struct PQnode*) malloc(sizeof(struct PQnode));
	pqnode->e = event;
	pqnode->next = NULL;

	if(pq->front == NULL) {
		pq->front = pqnode;
		return;
	}

	struct PQnode *current = pq->front;
	//if new node has higher priority before front (is smaller time)
	if (current->e->time > event->time) {
		pqnode->next = pq->front;
		pq->front = pqnode;

	} else {
		//traverse to find position to insert new node
		while (current->next != NULL && current->next->e->time < event->time) {
			current = current->next;
		}

		pqnode->next = current->next;
		current->next = pqnode;
	}
}

struct Event *dqPQ(struct PQ *pq) {		//deletes and returns highest priority (lowest time)
	if (pq->front == NULL) {
		return NULL;
	}

	struct PQnode *temp = pq->front;

	struct Event *toreturn = temp->e;

	pq->front = pq->front->next;
	free(temp);
	return toreturn;
}

struct Event *peekPQ(struct PQ *pq) {

	if (pq->front == NULL) {
			return NULL;
	}

	return pq->front->e;
}


//file stuff-----------------------------
void readSetConfigVars(char *filename) {
	FILE *f;

	char line[1024];

	if ((f = fopen(filename,"r")) != NULL) {
		while (fgets(line, sizeof(line), f) != NULL) {
			sscanf(line, "SEED %d\n", &SEED);
			sscanf(line, "INIT_TIME %d\n", &INIT_TIME);
			sscanf(line, "FIN_TIME %d\n", &FIN_TIME);
			sscanf(line, "ARRIVE_MIN %d\n", &ARRIVE_MIN);
			sscanf(line, "ARRIVE_MAX %d\n", &ARRIVE_MAX);
			sscanf(line, "QUIT_PROB %f\n", &QUIT_PROB);
			sscanf(line, "CPU_MIN %d\n", &CPU_MIN);
			sscanf(line, "CPU_MAX %d\n", &CPU_MAX);
			sscanf(line, "DISK1_MIN %d\n", &DISK1_MIN);
			sscanf(line, "DISK1_MAX %d\n", &DISK1_MAX);
			sscanf(line, "DISK2_MIN %d\n", &DISK2_MIN);
			sscanf(line, "DISK2_MAX %d\n", &DISK2_MAX);
		}
	} else {
		printf("File open error.\n");
	}

	fclose(f);

	//print to output file
	char buffer[100];
	sprintf(buffer,"SEED %d\n", SEED); printLine(buffer);
	sprintf(buffer,"INIT_TIME %d\n", INIT_TIME); printLine(buffer);
	sprintf(buffer,"FIN_TIME %d\n", FIN_TIME); printLine(buffer);
	sprintf(buffer,"ARRIVE_MIN %d\n", ARRIVE_MIN); printLine(buffer);
	sprintf(buffer,"ARRIVE_MAX %d\n", ARRIVE_MAX); printLine(buffer);
	sprintf(buffer,"QUIT_PROB %f\n", QUIT_PROB); printLine(buffer);
	sprintf(buffer,"CPU_MIN %d\n", CPU_MIN); printLine(buffer);
	sprintf(buffer,"CPU_MAX %d\n", CPU_MAX); printLine(buffer);
	sprintf(buffer,"DISK1_MIN %d\n", DISK1_MIN); printLine(buffer);
	sprintf(buffer,"DISK1_MAX %d\n", DISK1_MAX); printLine(buffer);
	sprintf(buffer,"DISK2_MIN %d\n", DISK2_MIN); printLine(buffer);
	sprintf(buffer,"DISK2_MAX %d\n", DISK2_MAX); printLine(buffer);

}

void printLine(char *s) {	//prints to output file
	FILE *f;

	f = fopen("src/output.txt", "a");
	fprintf(f, "%s", s);
	fclose(f);
}

//loop simulation
void loopSim() {
	struct Event *new = (struct Event*) malloc(sizeof(struct Event));

	new->id = 0;	//first event
	new->type = 0;
	new->time = INIT_TIME;

	qPQ(pq, new);

	currtime = INIT_TIME;

	//stuff for calculations after program
	int cpuqsum = 0;
	int disk1qsum = 0;
	int disk2qsum = 0;
	int pqsum = 0;

	int cpuqmax = 0;
	int disk1qmax = 0;
	int disk2qmax = 0;
	int pqmax = 0;

	int cpubusytime = 0;
	int disk1busytime = 0;
	int disk2busytime = 0;

	int loopcounter = 0;

	while(peekPQ(pq)!= NULL && peekPQ(pq)->time <= FIN_TIME) {	//while pq isnt empty and sim not finished
		handleEvent(dqPQ(pq));

		//calculationstuff
		cpuqsum += getQSize()[0];
		disk1qsum += getQSize()[1];
		disk2qsum += getQSize()[3];
		pqsum += getQSize()[4];

		if (getQSize()[0] > cpuqmax) {
			cpuqmax = getQSize()[0];
		}
		if (getQSize()[1] > disk1qmax) {
			disk1qmax = getQSize()[1];
		}
		if (getQSize()[2] > disk2qmax) {
			disk2qmax = getQSize()[2];
		}
		if (getQSize()[3] > pqmax) {
			pqmax = getQSize()[3];
		}

		++loopcounter;

		if (cpu.status) {	//if device busy,add to busy counter
			cpubusytime += (peekPQ(pq)->time - currtime);
		}

		if (disk1.status) {	//if device busy,
			disk1busytime += (peekPQ(pq)->time - currtime);
		}

		if (disk2.status) {	//if device busy,
			disk2busytime += (peekPQ(pq)->time - currtime);
		}

	}
	//print to output
	char buffer[100];
	sprintf(buffer,"%d Simulation finished\n", FIN_TIME); printLine(buffer);


	//print calculations
	printf("cpu q size average: %f\n", (float) cpuqsum / loopcounter);
	printf("disk1 q size average: %f\n", (float) disk1qsum / loopcounter);
	printf("disk2 q size average: %f\n", (float) disk2qsum / loopcounter);
	printf("priority q size average: %f\n", (float) pqsum / loopcounter);

	printf("cpu q size max: %d\n", cpuqmax);
	printf("disk1 q size max: %d\n", disk1qmax);
	printf("disk2 q size max: %d\n", disk2qmax);
	printf("priority q size max: %d\n", pqmax);

	printf("cpu utilization %f\n", (float) cpubusytime / (FIN_TIME - INIT_TIME));
	printf("disk1 utilization %f\n", (float) disk1busytime / (FIN_TIME - INIT_TIME));
	printf("disk2 utilization %f\n", (float) disk2busytime / (FIN_TIME - INIT_TIME));

}


//Handlers
void handleEvent(struct Event *e) {
	//printf("time %d, job %d, type %d\n",e->time, e->id,e->type); //debugging porpoises

	//0 arrives, 1 finishes CPU, 2 finishes disk 1
		//3 finishes disk 2
	switch (e->type) {
		case 0:
			eventArrives(e); break;
		case 1:
			eventFinishesCPU(e); break;
		case 2:
			eventFinishesD1(e); break;
		case 3:
			eventFinishesD2(e); break;
	}
}

void eventArrives(struct Event *e) {
	currtime = e->time;

	//print to output
	char buffer[100];
	sprintf(buffer,"%d Job %d arrives\n", currtime, e->id); printLine(buffer);

	//determine arrival time for next job and add to pq
	struct Event *new = (struct Event*) malloc(sizeof(struct Event));

	new->id = e->id + 1;
	new->type = 0;

	//not exactly uniformly distributed but close enough for me
	int arrivetime = rand()%(ARRIVE_MAX - ARRIVE_MIN + 1) + currtime;

	new->time = arrivetime;

	qPQ(pq, new);

	//send job to cpu
	qFIFO(cpu.f, e);

	//check if busy, if not generate cpu finish event
	if(!cpu.status) {
		processCPU(dqFIFO(cpu.f));
	}
}

void processCPU(struct Event *e) {
	//add cpu finsh event to pq
	cpu.status = 1;
	int finishtime = rand()%(CPU_MAX - CPU_MIN + 1) + currtime;

	e->time = finishtime;
	e->type = 1;

	qPQ(pq, e);
}

void eventFinishesCPU(struct Event *e) {
	currtime = e->time;

	cpu.status = 0;

	//print to output
	char buffer[100];
	sprintf(buffer,"%d Job %d finishes CPU\n", currtime, e->id); printLine(buffer);

	/*//add next event in fifo to pq
	struct Event *next = dqFIFO(cpu.f);

	if (next != NULL) {
		processCPU(e);
	}*/


	//check if job quits or goes to disk
	int quitbool = (rand() / (double)RAND_MAX) < QUIT_PROB;

	if (quitbool) {
		eventExits(e);
	} else {
		//decide which disk to send
		//test how long each disks fifo is
		struct FIFOnode *current1 = disk1.f->front;
		int i = 0;
		while (current1 != NULL) {
			current1 = current1->next;
			i++;
		}

		struct FIFOnode *current2 = disk2.f->front;
		int j = 0;
		while (current2 != NULL) {
			current2 = current2->next;
			j++;
		}

		if (i < j) {	//if disk 1 fifo shorter, send to disk 1 fifo
			qFIFO(disk1.f, e);

			if (!disk1.status) {	//disk 1 free
				processDisk(1, dqFIFO(disk1.f));
			}

		} else if (j > i) {
			qFIFO(disk2.f, e);

			if (!disk2.status) {	//disk 2 free
				processDisk(2, dqFIFO(disk2.f));
			}

		} else {	//both equal length, decide randomly
			if (rand() & 1) {
				qFIFO(disk1.f, e);

				if (!disk1.status) {	//disk 1 free
					processDisk(1, dqFIFO(disk1.f));
				}

			} else {
				qFIFO(disk2.f, e);

				if (!disk2.status) {	//disk 2 free
					processDisk(2, dqFIFO(disk2.f));
				}
			}
		}




	}
}

void processDisk(int diskno, struct Event *e) {
	//add disk finsh event to pq

	if(diskno == 1) {
		disk1.status = 1;
		int finishtime = rand()%(DISK1_MAX - DISK1_MIN + 1) + currtime;
		e->time = finishtime;
		e->type = 2;

		qPQ(pq, e);
	} else {
		disk2.status = 1;
		int finishtime = rand()%(DISK2_MAX - DISK2_MIN + 1) + currtime;
		e->time = finishtime;
		e->type = 3;

		qPQ(pq, e);
	}
}

void eventFinishesD1(struct Event *e) {
	currtime = e->time;

	disk1.status = 0;

	//print to output
	char buffer[100];
	sprintf(buffer,"%d Job %d finishes Disk 1\n", currtime, e->id); printLine(buffer);



	qFIFO(cpu.f, e);
	//send job to cpu
	if (!cpu.status) {	//check if busy, if not generate cpu finish event
		processCPU(dqFIFO(cpu.f));
	}


}

void eventFinishesD2(struct Event *e) {
	currtime = e->time;

	disk2.status = 0;

	//print to output
	char buffer[100];
	sprintf(buffer,"%d Job %d finishes Disk 2\n", currtime, e->id); printLine(buffer);



	qFIFO(cpu.f, e);
	//send job to cpu
	if (!cpu.status) {	//check if busy, if not generate cpu finish event
		processCPU(dqFIFO(cpu.f));
	}


}

void eventExits(struct Event *e) {
	currtime = e->time;
	char buffer[100];
	sprintf(buffer,"%d Job %d exits\n", currtime, e->id); printLine(buffer);
	free(e);
}

int* getQSize() {
	int *qsizes = (int*)malloc(sizeof(int) * 4);

	struct FIFOnode *c = cpu.f->front;
	qsizes[0] = 0;
	while (c != NULL) {
		c = c->next;
		qsizes[0]++;
	}

	struct FIFOnode *d1 = disk1.f->front;
	qsizes[1] = 0;
	while (d1 != NULL) {
		d1 = d1->next;
		qsizes[1]++;
	}

	struct FIFOnode *d2 = disk2.f->front;
	qsizes[2] = 0;
	while (d2 != NULL) {
		d2 = d2->next;
		qsizes[2]++;
	}

	struct PQnode *p = pq->front;
	qsizes[3] = 0;
	while (p!= NULL) {
		p = p->next;
		qsizes[3]++;
	}

	return qsizes;
}

