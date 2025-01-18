#include "globals.h"

/* ---------------------------------------------------------------- */
/* Declarations of global variables visible only in this file       */

PCB_t process;      // the only user process used for batch and FCFS
PCB_t* pNewProcess; // pointer for new process read from batch

FreeBlock_t* freeList = NULL;
BlockedProcess_t* blockedQueue = NULL;

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
    logGeneric("New consolidated free block created with total size: ");
    if (freeList == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    freeList->start = 0;
    freeList->size = MEMORY_SIZE;
    freeList->next = NULL;
}

FreeBlock_t* findFreeBlock(unsigned size) {
    FreeBlock_t* current = freeList;
    FreeBlock_t* previous = NULL;


    while (current != NULL) {
        if (current->size >= size) {
            logGeneric("Suitable block found during search: Start = " + current->start + current->size);
            if (current->size == size) {
                if (previous == NULL) {
                    freeList = current->next;
                }
                else {
                    previous->next = current->next;
                }
            }
            else {
                current->start += size;
                current->size -= size;
            }
            return current;
        }
        previous = current;
        current = current->next;
    }
    logGeneric("No suitable block found for size: " + size);
    return NULL;
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

    char buffer[100];
    sprintf(buffer, "Freeing memory block - Start: %u, Size: %u", start, size);
    logGeneric(buffer);

    FreeBlock_t* current = freeList;
    FreeBlock_t* previous = NULL;

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

    if (newBlock->next != NULL && newBlock->start + newBlock->size == newBlock->next->start) {
        newBlock->size += newBlock->next->size;
        FreeBlock_t* temp = newBlock->next;
        newBlock->next = temp->next;
        free(temp);
        logGeneric("Adjacent blocks merged (next)");
    }
    
    if (previous != NULL && previous->start + previous->size == newBlock->start) {
        previous->size += newBlock->size;
        previous->next = newBlock->next;
        free(newBlock);
        logGeneric("Adjacent blocks merged (previous)");
    }

    logMemoryState();
}
void enqueueBlockedProcessWithPriority(PCB_t* process) {
    BlockedProcess_t* newBlocked = (BlockedProcess_t*)malloc(sizeof(BlockedProcess_t));
    newBlocked->process = process;
    newBlocked->next = NULL;

    if (blockedQueue == NULL || process->size < blockedQueue->process->size) {
        newBlocked->next = blockedQueue;
        blockedQueue = newBlocked;
    }
    else {
        BlockedProcess_t* current = blockedQueue;
        while (current->next != NULL && current->next->process->size <= process->size) {
            current = current->next;
        }
        newBlocked->next = current->next;
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

void compactMemoryWithSimulation() {
    logGeneric("Starting memory compaction...");
    logMemoryState();

    if (freeList == NULL || freeList->next == NULL) {
        logGeneric("Compaction skipped: no fragmentation");
        return;
    }

    unsigned nextFreeStart = 0;
    unsigned totalCopyCost = 0;

    for (unsigned i = 0; i < MAX_PROCESSES; i++) {
        if (processTable[i].valid && processTable[i].status == running) {
            if (processTable[i].start != nextFreeStart) {
                char buffer[100];
                sprintf(buffer, "Moving process %u from %u to %u",
                    processTable[i].pid, processTable[i].start, nextFreeStart);
                logGeneric(buffer);

                totalCopyCost += processTable[i].size;
                processTable[i].start = nextFreeStart;
            }
            nextFreeStart += processTable[i].size;
        }
    }

    // Create new consolidated free block
    while (freeList != NULL) {
        FreeBlock_t* temp = freeList;
        freeList = freeList->next;
        free(temp);
    }

    freeList = (FreeBlock_t*)malloc(sizeof(FreeBlock_t));
    if (!freeList) {
        fprintf(stderr, "Memory allocation failed during compaction.\n");
        exit(1);
    }
    freeList->start = nextFreeStart;
    freeList->size = MEMORY_SIZE - nextFreeStart;
    freeList->next = NULL;

    char buffer[100];
    sprintf(buffer, "Compaction complete - Moved %u bytes, new free block at %u",
        totalCopyCost, nextFreeStart);
    logGeneric(buffer);

    logMemoryState();
}



void coreLoop(void) {
    pid_t newPid;
    SchedulingEvent_t nextEvent;
    unsigned delta;
    unsigned eventPid;
    Boolean isLaunchable = FALSE;

    // 1. Initialize
    initOS();
    logGeneric("Process info file opened");
    logGeneric("System initialized, starting batch");

    do {
        // 2. Check for new process
        if (checkForProcessInBatch()) {
            logGeneric("Reading next process from batch");

            if (isNewProcessReady()) {
                isLaunchable = TRUE;
                newPid = getNextPid();
                PCB_t* pNewProcess = getNewPCBptr();

                if (pNewProcess != NULL) {
                    logLoadedProcessData(pNewProcess);

                    if (initNewProcess(newPid, pNewProcess)) {
                        // Memory checks and allocation
                        if (processTable[newPid].size <= MEMORY_SIZE) {
                            if (usedMemory + processTable[newPid].size <= MEMORY_SIZE) {
                                FreeBlock_t* block = findFreeBlock(processTable[newPid].size);

                                if (block == NULL) {
                                    logGeneric("No suitable block found - attempting compaction");
                                    compactMemoryWithSimulation();
                                    block = findFreeBlock(processTable[newPid].size);
                                }

                                if (block != NULL) {
                                    processTable[newPid].start = block->start;
                                    processTable[newPid].status = running;
                                    usedMemory += processTable[newPid].size;
                                    runningCount++;
                                    systemTime += LOADING_DURATION;
                                    logPidMem(processTable[newPid].pid, "Process started and memory allocated");
                                    flagNewProcessStarted();
                                }
                                else {
                                    processTable[newPid].status = blocked;
                                    enqueueBlockedProcessWithPriority(&processTable[newPid]);
                                    logPid(processTable[newPid].pid, "Process blocked - no suitable memory block");
                                }
                            }
                            else {
                                logPid(processTable[newPid].pid, "Process blocked - insufficient memory");
                                processTable[newPid].status = blocked;
                                enqueueBlockedProcessWithPriority(&processTable[newPid]);
                            }
                        }
                        else {
                            logPid(processTable[newPid].pid, "Process rejected - exceeds total memory size");
                            deleteProcess(&processTable[newPid]);
                        }
                    }
                }
            }
        }

        delta = runToNextEvent(&nextEvent, &eventPid);
        if (delta > 0) {
            updateAllVirtualTimes(delta);
            systemTime += delta;
        }

        if (nextEvent == completed) {
            logPid(eventPid, "Process completed, freeing memory");

            usedMemory -= processTable[eventPid].size;
            freeMemory(processTable[eventPid].start, processTable[eventPid].size);
            deleteProcess(&processTable[eventPid]);
            runningCount--;

            PCB_t* blockedProcess;
            while ((blockedProcess = dequeueBlockedProcess()) != NULL) {
                FreeBlock_t* block = findFreeBlock(blockedProcess->size);
                if (block != NULL) {
                    blockedProcess->start = block->start;
                    blockedProcess->status = running;
                    runningCount++;
                    usedMemory += blockedProcess->size;
                    logPid(blockedProcess->pid, "Blocked process started");
                }
                else {
                    enqueueBlockedProcessWithPriority(blockedProcess);
                    break;
                }
            }
            logMemoryState();
        }

    } while ((runningCount > 0) || (batchComplete == FALSE));

    logGeneric("Batch processing complete, shutting down");
}
unsigned getNextPid() {
    static unsigned pidCounter = 0;
    unsigned i = 0;

    do {
        pidCounter++;
        if (pidCounter >= MAX_PID) pidCounter = 1;
        i++;
    } while (processTable[pidCounter].valid && i < MAX_PID);

    if (i >= MAX_PID) return 0;
    return pidCounter;
}

int initNewProcess(pid_t newPid, PCB_t* pProcess)
{
    if (pProcess == NULL)
        return 0;
    else {
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
        pProcess->valid = FALSE;
        return 1;
    }
}

int deleteProcess(PCB_t* pProcess)
{
    if (pProcess == NULL)
        return 0;
    else {
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
