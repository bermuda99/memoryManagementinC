#include "globals.h"

/* ---------------------------------------------------------------- */
/* Declarations of global variables visible only in this file       */

PCB_t process;      // the only user process used for batch and FCFS
PCB_t* pNewProcess; // pointer for new process read from batch

FreeBlock_t* freeList = NULL;
BlockedProcess_t* blockedQueue = NULL;
//FLO & Ben
/* ---------------------------------------------------------------- */
/*                Externally available functions                    */
/* ---------------------------------------------------------------- */

void initOS(void)
{
    unsigned i; // iteration variable

    /* init the status of the OS */
    // mark all process entries invalid
    for (i = 0; i < MAX_PROCESSES; i++) processTable[i].valid = FALSE;
    process.pid = 0; // reset pid
    freeList = (FreeBlock_t*)malloc(sizeof(FreeBlock_t));
    if (freeList == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    freeList->start = 0;
    freeList->size = MEMORY_SIZE;
    freeList->next = NULL;
}

FreeBlock_t* findFreeBlock(unsigned size)
{
    FreeBlock_t* current = freeList;
    FreeBlock_t* previous = NULL;

    while (current != NULL)
    {
        if (current->size >= size)
        {
            // Found a suitable block
            if (current->size == size)
            {
                // Exact match, remove the block from the list
                if (previous == NULL)
                {
                    freeList = current->next;
                }
                else
                {
                    previous->next = current->next;
                }
            }
            else
            {
                // Allocate part of the block
                current->start += size;
                current->size -= size;
            }
            return current;
        }
        previous = current;
        current = current->next;
    }
    return NULL; // No suitable block found
}

void freeMemory(unsigned start, unsigned size)
{
    FreeBlock_t* newBlock = (FreeBlock_t*)malloc(sizeof(FreeBlock_t));
    newBlock->start = start;
    newBlock->size = size;
    newBlock->next = NULL;

    // Insert the new block into the free list in sorted order
    FreeBlock_t* current = freeList;
    FreeBlock_t* previous = NULL;

    while (current != NULL && current->start < newBlock->start)
    {
        previous = current;
        current = current->next;
    }

    newBlock->next = current;

    if (previous == NULL)
    {
        freeList = newBlock;
    }
    else
    {
        previous->next = newBlock;
    }

    // Merge adjacent free blocks
    if (newBlock->next != NULL && newBlock->start + newBlock->size == newBlock->next->start)
    {
        newBlock->size += newBlock->next->size;
        FreeBlock_t* temp = newBlock->next;
        newBlock->next = newBlock->next->next;
        free(temp);
    }

    if (previous != NULL && previous->start + previous->size == newBlock->start)
    {
        previous->size += newBlock->size;
        previous->next = newBlock->next;
        free(newBlock);
    }
}

void enqueueBlockedProcess(PCB_t* process) {
    BlockedProcess_t* newBlocked = (BlockedProcess_t*)malloc(sizeof(BlockedProcess_t));
    newBlocked->process = process;
    newBlocked->next = NULL;

    if (blockedQueue == NULL) {
        blockedQueue = newBlocked;
    }
    else {
        BlockedProcess_t* current = blockedQueue;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = newBlocked;
    }
}

PCB_t* dequeueBlockedProcess() {
    if (blockedQueue == NULL) {
        return NULL;
    }
    BlockedProcess_t* temp = blockedQueue;
    PCB_t* process = temp->process;
    blockedQueue = blockedQueue->next;
    free(temp);
    return process;
}

void compactMemory() {
    FreeBlock_t* current = freeList;
    unsigned nextFreeStart = 0;

    while (current != NULL) {
        if (current->start > nextFreeStart) {
            // Verschiebe alle Prozesse nach unten
            for (unsigned i = 0; i < MAX_PROCESSES; i++) {
                if (processTable[i].valid && processTable[i].start >= current->start) {
                    processTable[i].start = nextFreeStart;
                    nextFreeStart += processTable[i].size;
                }
            }
            current->start = nextFreeStart;
        }
        nextFreeStart += current->size;
        current = current->next;
    }
}

void coreLoop(void) {
    SchedulingEvent_t nextEvent;        // scheduling event to process
    unsigned int eventPid;              // pid of a process triggering an event
    PCB_t* candidateProcess = NULL;     // next process to start, already stored in process table
    pid_t newPid;                       // pid of new process to start 
    PCB_t* nextReady = NULL;            // pointer to process that finishes next    
    unsigned delta = 0;                 // time interval by which the systemTime is advanced
    Boolean isLaunchable = FALSE;       // indicates if the next process in batch is launchable 

    do {    // loop until no more process to run 
        do {    // loop until no currenlty launchable process is in the batch file
            if (checkForProcessInBatch())
            {   // there is a process pending
                if (isNewProcessReady())    // test if the process is ready to be started
                {   // the process is ready to be started
                    isLaunchable = TRUE;
                    newPid = getNextPid();                  // get next valid pid
                    initNewProcess(newPid, getNewPCBptr()); // Info on new process provided by simulation
                    FreeBlock_t* block = findFreeBlock(processTable[newPid].size);
                    if (block != NULL)
                    {   // enough memory available, and location in memory found: start process
                        processTable[newPid].start = block->start;
                        processTable[newPid].status = running;  // mark new process as running
                        runningCount++;                         // and add to number of running processes
                        usedMemory = usedMemory + processTable[newPid].size;  // update amount of used memory
                        systemTime = systemTime + LOADING_DURATION;  // account for time used by OS
                        logPidMem(newPid, "Process started and memory allocated");
                        flagNewProcessStarted();    // process is now a running process, not a candidate any more 
                    }
                    else
                    {
                        processTable[newPid].status = blocked;  // not enough memory --> blocked due to "no ressources available"
                        logPidMem(newPid, "Process too large, not started");
                        enqueueBlockedProcess(&processTable[newPid]);
                    }
                }
                else
                {
                    isLaunchable = FALSE;
                    logGeneric("Sim: Process read but it is not yet ready to run");
                }
            }
        } while ((!batchComplete) && (isLaunchable));

        delta = runToNextEvent(&nextEvent, &eventPid);      // run the existing processes until a new arrives or one terminates

        updateAllVirtualTimes(delta);           // update all processes according to elapsed time

        systemTime = systemTime + delta;        // update system time by elapsed physical time
        if (nextEvent == completed) // check if a process needs to be terminated
        {
            usedMemory = usedMemory - processTable[eventPid].size;  // mark memory of the process free
            logPidMem(eventPid, "Process terminated, memory freed");
            freeMemory(processTable[eventPid].start, processTable[eventPid].size); // free the memory
            deleteProcess(&processTable[eventPid]); // terminate process
            runningCount--;             // one running process less 

            PCB_t* blockedProcess = dequeueBlockedProcess();
            while (blockedProcess != NULL) {
                FreeBlock_t* block = findFreeBlock(blockedProcess->size);
                if (block != NULL) {
                    blockedProcess->start = block->start;
                    blockedProcess->status = running;
                    runningCount++;
                    usedMemory += blockedProcess->size;
                    logPidMem(blockedProcess->pid, "Blocked process started and memory allocated");
                }
                else {
                    enqueueBlockedProcess(blockedProcess);
                    break;
                }
                blockedProcess = dequeueBlockedProcess();
            }
        }

        if (usedMemory < MEMORY_SIZE / 2) {
            compactMemory();
        }

        logMemoryState();

    } while ((runningCount > 0) || (batchComplete == FALSE));
}

unsigned getNextPid()
{
    static unsigned pidCounter = 1;
    unsigned i = 0; // iteration variable;
    // determine next available pid make sure not to search infinitely
    while ((processTable[pidCounter].valid) && (i < MAX_PID))
    {
        // determine next available pid 
        pidCounter = (pidCounter + 1) % MAX_PID;
        if (pidCounter == 0) pidCounter++; // pid=0 is invalid
        i++; // count the checked entries
    }
    if (i == MAX_PID) return 0; // indicate error
    else           return pidCounter;
}

int initNewProcess(pid_t newPid, PCB_t* pProcess)
/* Initialised the PCB at the given index of the process table with the        */
/* process information provided in the PCB-struct giben by the pointer        */
{
    if (pProcess == NULL)
        return 0;
    else {    /* PCB correctly passed, now initialise it */
        processTable[newPid].pid = newPid;
        processTable[newPid].ppid = pProcess->ppid;
        processTable[newPid].ownerID = pProcess->ownerID;
        processTable[newPid].start = pProcess->start;
        processTable[newPid].duration = pProcess->duration;
        processTable[newPid].size = pProcess->size;
        processTable[newPid].usedCPU = pProcess->usedCPU;
        processTable[newPid].type = pProcess->type;
        processTable[newPid].status = init;
        processTable[newPid].valid = TRUE;
        // new process is initialised, now invalidate the PCB passed to this funtion
        pProcess->valid = FALSE;
        return 1;
    }
}

int deleteProcess(PCB_t* pProcess)
/* Voids the PCB handed over in pProcess, this includes setting the valid-    */
/* flag to invalid and setting other values to invalid values.                */
/* retuns 0 on error and 1 on success                                        */
{
    if (pProcess == NULL)
        return 0;
    else {    /* PCB correctly passed, now delete it */
        pProcess->valid = FALSE;
        pProcess->pid = 0;
        pProcess->ppid = 0;
        pProcess->ownerID = 0;
        pProcess->start = 0;
        pProcess->duration = 0;
        pProcess->size = 0;
        pProcess->usedCPU = 0;
        pProcess->type = os;
        pProcess->status = ended;
        return 1;
    }
}

/* ----------------------------------------------------------------- */
/*                       Local helper functions                      */
/* ----------------------------------------------------------------- */
