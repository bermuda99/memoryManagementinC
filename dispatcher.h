/* header-file defining the interface of the dispatcher */
/* the dispatcher contains all functions required to run the */
/* simulated processed */
/* The dispatcher is called in the central schgeduling loop. The scheduler	*/
/* must be called to determine, which process to run						*/

/* header-file defining the interface of the scheduler */
/* the scheduler contains all functions required to decide which process	*/
/* to run next																*/
/* +++	this version only implements a batch-scheuling using FIFO without	*/
/* +++	SPOOLING. It needs to be extended to support multiprogramming		*/

#ifndef __DISPATCHER__
#define __DISPATCHER__

#include "bs_types.h"
#include "globals.h"
#include "loader.h"


SchedulingEvent_t runProcess(PCB_t* pProcess, unsigned quantum); 
/* the process given by the PCB in parameter pProcess is run until			*/
/*		- the given quantum is over,										*/
/*		- an IO event occurs (simulated by random number), or				*/
/*		- the process is complete											*/
/* a quantum value of 0 indicates that no quantum is used (batch scheduling)*/
/* returns the reason for stopping the execution of the process				*/




#endif /* __DISPATCHER__ */
