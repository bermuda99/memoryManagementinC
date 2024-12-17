/* Implementation of system componentes required to simulate the	*/
/* execution of processes.											*/
/* for comments on the functions see the associated .h-file */

/* ---------------------------------------------------------------- */
/* Include required external definitions */
#include <math.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "bs_types.h"
#include "globals.h"
#include "core.h"
#include "loader.h"
#include "executer.h"



/* ----------------------------------------------------------------	*/
/* Declarations of global variables visible only in this file 		*/

/* ---------------------------------------------------------------- */
/*                Declarations of local functions					*/



/* ---------------------------------------------------------------- */
/*                Externally available functions                    */
/* ---------------------------------------------------------------- */


Boolean isNewProcessReady() {
/* predicate returning if the next process listed in the batch file is ready to be executed */
/* i.e. if the system time has passed its start time */
	return (systemTime >= candidateProcess.start);
}

void flagNewProcessStarted(void) {
	deleteProcess(&candidateProcess);		// clean up candidate-variable
	candidateProcess.valid = FALSE; // process is now a running process, not a candidate any more 
}

unsigned int runToNextEvent(SchedulingEvent_t* nextEvent, pid_t* eventPid) {
	// advances the system time until the next scheduling event occurs
	unsigned int remaining;					// time to termination of a precess 
	unsigned int minRemaining = UINT_MAX;	// time until the next process terminates
	unsigned int delta= UINT_MAX;			// time interval by which the system time needs to be advanced
	pid_t pid = 1;							// start search with lowest valid pid
	unsigned int i = 0;						// counter for running processes 
	unsigned int nextReadyPID = 0;			// pid of found process to terminate next 

	*nextEvent = none;						// make sure a defined return happens
	*eventPid = 0; 
	while ((i<runningCount) && (pid<MAX_PROCESSES))
	{	// search entire process table or until all current provcesses are checked
		if ((processTable[pid].valid == TRUE) && (processTable[pid].status == running))
		{
			remaining = processTable[pid].duration - processTable[pid].usedCPU;
			if (remaining<minRemaining)
			{	// found a process finishing earlier than last match
				nextReadyPID = pid;			// remember that process
				minRemaining = remaining;	// update minimum
				delta = minRemaining * runningCount;		// all processes advance until first to quit is finished
				*nextEvent = completed;
				*eventPid = nextReadyPID;
			}
			i++;	// one more running process used
		}
		pid++;		// next process entry
	}
	// the next process to end is found. 
	// unless a new process is started before that, so check for it: 
	// check if the waiting candidate (if any) will start earlier
	if (candidateProcess.valid)
	{
		if ((candidateProcess.start >= systemTime)
			&& (candidateProcess.start - systemTime < delta))
		{	// the waiting process can start before any running one is complete
			delta = (candidateProcess.start - systemTime);
			nextReadyPID = 0;		// no process to terminate now
			*nextEvent = start;		// but there is a candidate to start
			*eventPid = 0;			// the candidate does not have a pid yet
		}
	}
	if (runningCount == 0)
	{	// notify the user in case of idle time 
		//systemTime = candidateProcess.start;			// BUG korrigiert: Diese Funktion berechnet nur die Zeitspanne, die Aktualisierung der Systemzeit erfolgt in der Core-loop
		logGeneric("Sim: CPU is turning idle now.");	// BUG korrigiert: Log-Ausgabe auch angepasst, das neue Systemzeit hier nicht bekannt.
	}

	return max(1U, delta);			// make sure time advances, avoid zero delta due to integer arithmetics
}

void updateAllVirtualTimes(unsigned int delta)
{
	// update all processes according to elapsed time
	// the parameter specifies the elapsed physical time, so each running process is 
	// executed by its equal share. 
	unsigned int pid = 1;			// start search with lowest valid pid and 
	unsigned int i = 0;				// init counter for running processes 
	
	while ((i < runningCount) && (pid < MAX_PROCESSES))
	{	// search entire process table or until all current provcesses are checked
		if ((processTable[pid].valid == TRUE) && (processTable[pid].status == running))
		{
			// update time already spent in process
			processTable[pid].usedCPU = processTable[pid].usedCPU + delta / runningCount;
			i++;	// one more running process updated
		}
		pid++;		// next process entry
	}
}
/* ---------------------------------------------------------------- */
/*                       Local functions							*/
/* ---------------------------------------------------------------- */


