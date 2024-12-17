/* header-file defining the interface of the scheduler */
/* the scheduler contains all functions required to decide which process	*/
/* to run next																*/
/* +++	this version only implements a batch-scheuling using FIFO without	*/
/* +++	SPOOLING. It needs to be extended to support multiprogramming		*/

#ifndef __LOG__
#define __LOG__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "bs_types.h"
#include "globals.h"
#include "loader.h"


void logGeneric(char* message);
/* print the given general string to stdout and/or a log file 				*/
/* The time stamp (systemTime) will be prepended automatically				*/
/* the linefeed is automatically appended									*/

void logPid(unsigned pid, char* message);
/* print the given  string to stdout and/or a log file 						*/
/* The time stamp (systemTime) and the  PID are prepended automatically		*/
/* the linefeed is automatically appended									*/

void logPidCompleteness(unsigned pid, unsigned done, unsigned length, 
						char * message);
/* print the given  string to stdout and/or a log file 						*/
/* The time stamp (systemTime) and the  PID are prepended automatically		*/
/* the fraction of used CPU over process duratiuon is also prepended		*/
/* the linefeed is automatically appended									*/

void logPidMem(unsigned pid, char * message);
/* print the given  string to stdout and/or a log file 						*/
/* The time stamp (systemTime) and the  PID are prepended automatically		*/
/* the currenty used amount of physical memory is also prepended		    */
/* the linefeed is automatically appended									*/

void logLoadedProcessData(PCB_t* pProcess); 
/* print the data of the new process read from file							*/
/* Intended and used only for the simulation/testenvironment				*/				
/* The time stamp (systemTime) is automatically, PID is not applicable		*/

void logMemoryState(void);
/* Print a detailed overview of the memory state, including:            */
/* - Free memory blocks (start and size)                                */
/* - Allocated memory with process details (PID, start, size, status)   */
/* The time stamp (systemTime) is automatically prepended.              */
/* Output is formatted for clear readability.                           */


#endif /* __LOG__ */