Eric Gurevich. 3207 Lab 1. Discrete Event Simulator.

1. The average and the maximum size of each queue.

per event handle loop
--------------------------
cpu q size average: 643.016663
disk1 q size average: 149.395264
disk2 q size average: 3.322229
priority q size average: 0.000000

^ found by diving sum queue size of each device by number of event handling loops

per event handle loop
--------------------------
cpu q size max: 1287
disk1 q size max: 288
disk2 q size max: 288
priority q size max: 4



2. The utilization of each server (component). This would be: time_the_server_is_busy/total_time where total_time = FIN_TIME-INIT_TIME.

cpu utilization 0.683900
disk1 utilization 0.810000
disk2 utilization 0.819500

3. The average and maximum response time of each server (response time will be the difference in time between the job arrival at a server and the completion of the job at the server)

idk how to do this :)

4. The throughput (number of jobs completed per unit of time) of each server.

CPU - 844 Jobs Completed 
	------------------------ = 0.0844 jobs/unittime
		10,000 Time

Disk 1 - 169 Jobs Completed 
	------------------------ = 0.0169 jobs/unittime
		10,000 Time
		
Disk 2 - 198 Jobs Completed 
	------------------------ = 0.0198 jobs/unittime
		10,000 Time
		
		
***ACTUAL README*****

The program begins by reading config variables from a text file and storing them in global int/float variables for easy access 
throughout the program. 

Structs are used to represent events, containing JOB ID, time of the event, and type of the event where 0 arrives, 1 finishes CPU, 2 finishes disk 1
	3 finishes disk 2. 
	
Structs are used to represent the FIFO and its nodes, as well as the Priority Queue and its nodes. These queues are based on
linked lists. Functions are set up to return these queues, to queue new events, to dequeue and return head events, and to peek at the head events.
The priority queue is set up to base priority on the node->event->time, where lower time, means higher priority, so that the 
queue launches events that are of lower time, first. 

Structs are used to represent the devices, CPU, Disk1, and Disk2, containing the status as a boolean, and the associated FIFO.

***pseudocode***

Add initial arrival event to Priority Queue (PQ). Add event representing FIN_TIME.

While (PQ not empty && current time < FIN_TIME)
	Dequeue PQ and send to appropriate event handler
	Set current time of handler to time of event
		
		Handler 0: Event Arrrives
			Generate new event, add to CPU queue 
			If CPU not busy
				Mark CPU busy
				Dequeue CPU FIFO, generate event using rand between CPU_MAX and CPU_MIN + currtime 
				Send event to PQ
				
		Handler 1: CPU finishes Event 
		 	Mark CPU free
		 	Choose if Event quits based on QUIT_PROB
		 	If Not
		 		Choose which disk to send to (based on length of their queues)
		 		Add to disk queue
		 		If disk not busy
		 			Mark disk busy
		 			Dequeue Disk FIFO, generate event using rand between DISKx_MAX and DISKx_MIN + currtime
		 			Send event to PQ
		 		
		Handler 2/3: Disk finishes Event 
			Mark disk free
			Generate new event, add to CPU queue
			If CPU not busy
				Mark CPU busy
				Dequeue CPU FIFO, generate event using rand between CPU_MAX and CPU_MIN + currtime 
				Send event to PQ
				
I chose these choices because they were the only ones that worked. I also chose to free some mallocs sometimes, e.g. after
an Event quits, or node is dequeued so the program wouldnt crash and/or heat up my laptop. I chose to use a linked list queue
system because it's much easier to deal with than arrays or heaps. I chose the config variables because they seemed to look right
and the program works. The device utilization could be better, but hey.
		 	