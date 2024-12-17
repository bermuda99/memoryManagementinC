/* Implementation of the loader. Responsible for reading batch,		*/
/* generating and destroying of processes  */
/* for comments on the functions see the associated .h-file */

#define	_CRT_SECURE_NO_WARNINGS		// suppress legacy warnings 

/* ---------------------------------------------------------------- */
/* Include required external definitions */
#include <math.h>
#include "loader.h"
#include "bs_types.h"
#include "globals.h"
#include "log.h"

/* ----------------------------------------------------------------	*/
/* Declarations of global variables visible only in this file 		*/
extern PCB_t candidateProcess;		// only for simulation purposes

/* ---------------------------------------------------------------- */
/*                Declarations of local helper functions            */
// none

/* ---------------------------------------------------------------- */
/*                Externally available functions                    */
/* ---------------------------------------------------------------- */

FILE* openConfigFile (FILE *file, const char * filename)
{

	char linebuffer[129]="x"; // read buffer for file-input
	char charbuffer='x';		// read buffer for file-input
	unsigned ownerID=0xFF, start=0xFF, duration=0xFF; // local buffer for reading proccess properties
	char ownerIDStr[21]="", startStr[21]=""; // buffers for reading process type-string
	char durationStr[21]="", processTypeStr [21]=""; 	// buffers for reading process type-string

	// try to open file for read
	file = fopen (filename, "r");
	// test for success and error handling
	if (file == NULL)
		{
		logGeneric("Fehler beim Oeffnen der Datei mit Prozessinformationen.");
		return NULL;
		}
	// skip first line, only a comment
	if (!feof(file))
		fgets(linebuffer, 128, file);
		// printf("Read from File: %s ", linebuffer);	// Debug-Output showing skipped first line in the file
	return file;
}


Boolean readNextProcess (FILE* f, PCB_t* pProcess)
{
	char linebuffer[129]="";		// read buffer for file-input
	char processTypeStr [21]=""; 	// buffers for reading process type-string
	int readOK; 
	if (f==NULL) return FALSE;		// error: file handle not initialised
	if (feof(f))
		{
		fclose (f);			// close the file on reaching EOF
		return FALSE;		// error occured (EOF reached)
		}
	else 
	{	
		fgets(linebuffer, 128, f);
		if (strcmp(linebuffer, "")==0)
			return FALSE;			// error occured: line is empty
		else
		{
			readOK=sscanf (linebuffer, "%u %u %u %u %s", &pProcess->ownerID, &pProcess->start, 
				&pProcess->duration, &pProcess->size, processTypeStr);

			if (strcmp(processTypeStr, "os")==0)
				pProcess->type=os;
			else if (strcmp(processTypeStr, "interactive")==0)
				pProcess->type=interactive;
			else if (strcmp(processTypeStr, "batch")==0)
				pProcess->type=batch;
			else if (strcmp(processTypeStr, "background")==0)
				pProcess->type=background;
			else if (strcmp(processTypeStr, "foreground")==0)
				pProcess->type=foreground;
			else pProcess->type=os;
		}
		// Debug-output to show the data read in the line of the processes.txt-file
		//printf( "Sim: New process read from File: %u %u %u %u %s\n", pProcess->ownerID, pProcess->start, pProcess->duration, pProcess->size, processTypeStr);

	}
	return TRUE; 
}

Boolean checkForProcessInBatch() {
	// select and run a process
	if (!candidateProcess.valid)	// no candidate read from file yet
	{
		logGeneric("Sim: Reading next process from file");
		// read the next process for the file and store in process table
		candidateProcess.valid = readNextProcess(processFile, &candidateProcess);
		if (candidateProcess.valid)
		{	// there are still jobs listed in the file   
			logGeneric("Sim: Next process loaded from file");
			logLoadedProcessData(&candidateProcess);
		}
		else	// no more processes to be started 
		{
			logGeneric("Sim: No further process listed for execution.");
			batchComplete = TRUE;
		}
	}
	return candidateProcess.valid;
}

PCB_t* getNewPCBptr(void)
/* returns a pointer to the PCB of the process to start */
{
	return &candidateProcess;
}
