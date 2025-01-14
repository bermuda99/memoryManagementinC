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

void freeMemory(unsigned start, unsigned size) {
    FreeBlock_t* newBlock = (FreeBlock_t*)malloc(sizeof(FreeBlock_t));
    if (!newBlock) {
        fprintf(stderr, "Memory allocation failed in freeMemory.\n");
        exit(1);
    }
    newBlock->start = start;
    newBlock->size = size;
    newBlock->next = NULL;

    FreeBlock_t* current = freeList;
    FreeBlock_t* previous = NULL;

    // In die Liste einfügen
    while (current != NULL && current->start < start) {
        previous = current;
        current = current->next;
    }

    newBlock->next = current;
    if (previous == NULL) {
        freeList = newBlock;
    }
    else {
        previous->next = newBlock;
    }

    // Konsolidierung benachbarter Blöcke
    if (newBlock->next != NULL && newBlock->start + newBlock->size == newBlock->next->start) {
        newBlock->size += newBlock->next->size;
        FreeBlock_t* temp = newBlock->next;
        newBlock->next = newBlock->next->next;
        free(temp);
        logGeneric("Adjacent free blocks merged (next).");
    }
    if (previous != NULL && previous->start + previous->size == newBlock->start) {
        previous->size += newBlock->size;
        previous->next = newBlock->next;
        free(newBlock);
        logGeneric("Adjacent free blocks merged (previous).");
    }

    logGeneric("Memory freed and consolidated.");
    logMemoryState();
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
    // Prüfen, ob eine Kompaktierung notwendig ist
    if (freeList == NULL || freeList->next == NULL) {
        logGeneric("Memory compaction skipped: No fragmentation detected.");
        return;
    }

    unsigned nextFreeStart = 0;

    // Prozesse verschieben
    for (unsigned i = 0; i < MAX_PROCESSES; i++) {
        if (processTable[i].valid && processTable[i].status == running) {
            if (processTable[i].start != nextFreeStart) {
                logPid(processTable[i].pid, "Process moved during memory compaction");
                processTable[i].start = nextFreeStart;
            }
            nextFreeStart += processTable[i].size;
        }
    }

    // Einen einzigen konsolidierten freien Block erstellen
    FreeBlock_t* newFreeBlock = (FreeBlock_t*)malloc(sizeof(FreeBlock_t));
    if (!newFreeBlock) {
        fprintf(stderr, "Memory allocation failed in compactMemory.\n");
        exit(1);
    }
    newFreeBlock->start = nextFreeStart;
    newFreeBlock->size = MEMORY_SIZE - nextFreeStart;
    newFreeBlock->next = NULL;

    freeList = newFreeBlock;

    logGeneric("Memory compacted successfully.");
    logMemoryState();
}





void coreLoop(void) {
    SchedulingEvent_t nextEvent;
    unsigned int eventPid;
    PCB_t* candidateProcess = NULL;
    pid_t newPid;
    PCB_t* nextReady = NULL;
    unsigned delta = 0;
    Boolean isLaunchable = FALSE;

    // Initialize
    initOS();

    do {
        // Check for new process
        do {
            if (checkForProcessInBatch()) {
                if (isNewProcessReady()) {
                    isLaunchable = TRUE;
                    newPid = getNextPid();
                    initNewProcess(newPid, getNewPCBptr());

                    // First check: Does it fit in total memory?
                    if (processTable[newPid].size <= MEMORY_SIZE) {
                        // Second check: Is a suitable memory block available?
                        FreeBlock_t* block = findFreeBlock(processTable[newPid].size);

                        if (block != NULL) {
                            // Memory block available - assign and start process
                            processTable[newPid].start = block->start;
                            processTable[newPid].status = running;
                            runningCount++;
                            usedMemory = usedMemory + processTable[newPid].size;
                            systemTime = systemTime + LOADING_DURATION;
                            logPidMem(newPid, "Process started and memory allocated");
                            flagNewProcessStarted();
                        }
                        else {
                            // No suitable block - block process and add to queue
                            processTable[newPid].status = blocked;
                            logPidMem(newPid, "No suitable memory block, process blocked");
                            enqueueBlockedProcess(&processTable[newPid]);

                            // Check if compaction would help
                            if (usedMemory + processTable[newPid].size <= MEMORY_SIZE) {
                                compactMemory();
                                // Try again after compaction
                                block = findFreeBlock(processTable[newPid].size);
                                if (block != NULL) {
                                    PCB_t* blockedProcess = dequeueBlockedProcess();
                                    blockedProcess->start = block->start;
                                    blockedProcess->status = running;
                                    runningCount++;
                                    usedMemory += blockedProcess->size;
                                    logPidMem(blockedProcess->pid, "Process started after compaction");
                                }
                            }
                        }
                    }
                    else {
                        // Process too large for total memory - reject
                        logPidMem(newPid, "Process rejected - exceeds total memory size");
                        deleteProcess(&processTable[newPid]);
                    }
                }
                else {
                    isLaunchable = FALSE;
                    logGeneric("Sim: Process read but it is not yet ready to run");
                }
            }
        } while ((!batchComplete) && (isLaunchable));

        delta = runToNextEvent(&nextEvent, &eventPid);
        updateAllVirtualTimes(delta);
        systemTime = systemTime + delta;

        if (nextEvent == completed) {
            // Process termination
            usedMemory = usedMemory - processTable[eventPid].size;
            logPidMem(eventPid, "Process terminated");

            // Free memory
            freeMemory(processTable[eventPid].start, processTable[eventPid].size);
            deleteProcess(&processTable[eventPid]);
            runningCount--;

            // After freeing memory, check blocked queue
            PCB_t* blockedProcess = dequeueBlockedProcess();
            while (blockedProcess != NULL) {
                FreeBlock_t* block = findFreeBlock(blockedProcess->size);
                if (block != NULL) {
                    blockedProcess->start = block->start;
                    blockedProcess->status = running;
                    runningCount++;
                    usedMemory += blockedProcess->size;
                    logPidMem(blockedProcess->pid, "Blocked process started");
                }
                else {
                    enqueueBlockedProcess(blockedProcess);
                    break;
                }
                blockedProcess = dequeueBlockedProcess();
            }

            // Log the current memory state
            logMemoryState();
        }
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
