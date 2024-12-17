/* Include-file defining functions of the OS-core 	 		*/
#ifndef __EXEC__
#define __EXEC__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


Boolean awaitNextProcess(void);
/* check the batch file for processes to start and advance simulation time */
/* until the next process can be started */

Boolean isNewProcessReady(); 
/* predicate returning if the next process listed in the batch file is ready to be executed */
/* i.e. if the system time has passed its start time */


void flagNewProcessStarted(void);
/* clean up the PCB of the waiting candidate of a new process */
/* used after the candidate became a running process  */			

unsigned int runToNextEvent(SchedulingEvent_t* nextEvent, pid_t* eventPid);
// advance the system time until the next scheduling event occurs
// the event and the pid of the process causing it are updated in the passed parameters
// the time interval to pass until this event occurs is given by the return value


void updateAllVirtualTimes(unsigned int delta); 
// update the virtual time of all currently running processes
// the paratmeter specifies the elapdes physical time, i.e. each process
// is executes by only its part (assumed to be equal shares)
// zero updates may occure due to integer arithmetics

#endif /* __EXEC__ */