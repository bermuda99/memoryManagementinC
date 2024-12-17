/* Implementation of the log functions */
/* for comments on the functions see the associated .h-file */

/* ---------------------------------------------------------------- */
/* Include required external definitions */
#include "globals.h"
#include <math.h>
#include "bs_types.h"
#include "log.h"

/* ---------------------------------------------------------------- */
/*                Declarations of local helper functions            */

/* ---------------------------------------------------------------- */
/* Declarations of global variables visible only in this file 		*/
// array with strings associated to scheduling events for log outputs
char eventString[3][12] = { "completed", "io", "quantumOver" };

/* ---------------------------------------------------------------- */
/*                Externally available functions					*/
/* ---------------------------------------------------------------- */

void logGeneric(char* message)
{
	printf("%6u : %s\n", systemTime, message);
}

void logPid(unsigned pid, char* message)
{
	printf("%6u : PID %3u : %s\n", systemTime, pid, message);
}

void logPidCompleteness(unsigned pid, unsigned done, unsigned length,
	char* message)
{
	printf("%6u : PID %3u : completeness: %u/%u | %s\n", systemTime,
		pid, done, length, message);
}

void logPidMem(unsigned pid, char* message)
{
	printf("%6u : PID %3u : Used memory: %6u | %s\n", systemTime,
		pid, usedMemory, message);
}


void logLoadedProcessData(PCB_t* pProcess)
{
	char processTypeStr[21] = ""; 	// buffer process type-string

	switch (pProcess->type) {
	case os:
		strcpy(processTypeStr, "os");
		break;
	case interactive:
		strcpy(processTypeStr, "interactive");
		break;
	case batch:
		strcpy(processTypeStr, "batch");
		break;
	case background:
		strcpy(processTypeStr, "background");
		break;
	case foreground:
		strcpy(processTypeStr, "foreground");
		break;
	default:
		strcpy(processTypeStr, "no type");
	}

	printf("%6u : Sim: Loaded process properties: %u %u %u %u %s\n", systemTime,
		pProcess->ownerID, pProcess->start, pProcess->duration,
		pProcess->size, processTypeStr);
}

void logMemoryState(void)
{
	FreeBlock_t* current = freeList;
	printf("\n========================================\n");
	printf(" Memory State at Time %u\n", systemTime);
	printf("========================================\n");

	// Zeige freie Speicherblöcke an
	printf("Free Memory Blocks:\n");
	printf("----------------------------------------\n");
	while (current != NULL) {
		printf("Start: %6u | Size: %6u\n", current->start, current->size);
		current = current->next;
	}

	// Zeige belegte Speicherbereiche pro Prozess an
	printf("----------------------------------------\n");
	printf("Allocated Memory (Processes):\n");
	printf("----------------------------------------\n");
	for (unsigned i = 0; i < MAX_PROCESSES; i++) {
		if (processTable[i].valid && processTable[i].status == running) {
			printf("PID: %3u | Start: %6u | Size: %6u | Status: %s\n",
				processTable[i].pid,
				processTable[i].start,
				processTable[i].size,
				(processTable[i].status == running) ? "RUNNING" : "UNKNOWN");
		}
	}

	printf("========================================\n\n");
}


/* ----------------------------------------------------------------- */
/*                       Local helper functions                      */
/* ----------------------------------------------------------------- */
