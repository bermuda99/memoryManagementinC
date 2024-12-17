// main file, top level of simulation/test environment for the		*/
/* memory management component of the operating system				*/
/* under development												*/
#include "bs_types.h"
#include "globals.h"
#include "loader.h"
#include "core.h"
#include <time.h>



/* ----------------------------------------------------------------	*/
/* Declare global variables according to definition in globals.h	*/
PCB_t processTable[MAX_PROCESSES]; 	// the process table
unsigned systemTime=0; 			// the current system time (up time)
extern unsigned usedMemory=0;	// amount of used physical memory
unsigned runningCount = 0;			// counter of currently running processes
Boolean batchComplete = FALSE;		// end of pending processes in the file indicator
FILE* processFile;			// file containing the processes to simulate
PCB_t candidateProcess;		// only for simulation purposes

/* ----------------------------------------------------------------	*/
/* Declarations of global variables visible only in this file 		*/

/* ---------------------------------------------------------------- */
/*                Declarations of local functions                   */
int initSim();		// initialises the simulation environment


int main(int argc, char *argv[])
{	// starting point, all processing is done in called functions
	printf ("Starting system. Available memory: %u\n", MEMORY_SIZE );
	initSim();		// initialise the simulation
	initOS();		// initialise OS itself
	logGeneric("System Initialised, starting batch");
	coreLoop();		// start scheduling loop
	logGeneric("Batch complete, shutting down");
	fflush(stdout);			// make sure the output on the console is complete 
	return 1; 
 }

/* ---------------------------------------------------------------- */
/*                Implementation of local functions                 */

int initSim()
{	// initialises the simulation environment

	char filename[128] = PROCESS_FILENAME;
	systemTime = 0;				// reset the system time to zero
								// open the file with process definitions
	processFile = openConfigFile(processFile, filename);
	logGeneric("Process info file opened");
	srand((unsigned)time(NULL));	// init the random number generator

	return TRUE;
}
