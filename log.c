/* Implementation of the log functions */
#include "globals.h"
#include <math.h>
#include "bs_types.h"
#include "log.h"

/* Global variables */
char eventString[3][12] = { "completed", "io", "quantumOver" };

void logGeneric(const char* message)
{
    printf("%6u : %s\n", systemTime, message);
}

void logPid(unsigned pid, const char* message)
{
    printf("%6u : PID %3u : %s\n", systemTime, pid, message);
}

void logPidCompleteness(unsigned pid, unsigned done, unsigned length, const char* message)
{
    printf("%6u : PID %3u : completeness: %u/%u | %s\n",
        systemTime, pid, done, length, message);
}

void logPidMem(unsigned pid, const char* message)
{
    printf("%6u : PID %3u : Used memory: %6u | %s\n",
        systemTime, pid, usedMemory, message);
}

void logMemoryAllocation(unsigned pid, unsigned size, unsigned start)
{
    printf("%6u : PID %3u : Allocated memory block - Start: %6u, Size: %6u\n",
        systemTime, pid, start, size);
}

void logMemoryDeallocation(unsigned pid, unsigned size, unsigned start)
{
    printf("%6u : PID %3u : Freed memory block - Start: %6u, Size: %6u\n",
        systemTime, pid, start, size);
}

void logMemoryCompaction(unsigned movedBytes)
{
    printf("%6u : Memory compaction completed - Moved %u bytes\n",
        systemTime, movedBytes);
}

void logLoadedProcessData(PCB_t* pProcess)
{
    const char* processTypeStr;

    switch (pProcess->type) {
    case os:          processTypeStr = "os"; break;
    case interactive: processTypeStr = "interactive"; break;
    case batch:       processTypeStr = "batch"; break;
    case background:  processTypeStr = "background"; break;
    case foreground:  processTypeStr = "foreground"; break;
    default:         processTypeStr = "no type";
    }

    printf("%6u : Sim: Loaded process properties: %u %u %u %u %s\n",
        systemTime, pProcess->ownerID, pProcess->start, pProcess->duration,
        pProcess->size, processTypeStr);
}

void logMemoryState() {
    static unsigned lastLoggedSystemTime = 0;

    if (systemTime == lastLoggedSystemTime) {
        return;
    }
    lastLoggedSystemTime = systemTime;

    printf("\n========================================\n");
    printf(" Memory State at Time %u\n", systemTime);
    printf("========================================\n");

    // Free blocks
    printf("Free Memory Blocks:\n");
    printf("----------------------------------------\n");
    FreeBlock_t* current = freeList;
    unsigned totalFree = 0;
    while (current != NULL) {
        printf("Start: %6u | Size: %6u\n", current->start, current->size);
        totalFree += current->size;
        current = current->next;
    }

    // Running processes
    printf("----------------------------------------\n");
    printf("Allocated Memory (Processes):\n");
    printf("----------------------------------------\n");
    unsigned totalUsed = 0;
    for (unsigned i = 0; i < MAX_PROCESSES; i++) {
        if (processTable[i].valid && processTable[i].status == running) {
            printf("PID: %3u | Start: %6u | Size: %6u | Status: RUNNING\n",
                processTable[i].pid,
                processTable[i].start,
                processTable[i].size);
            totalUsed += processTable[i].size;
        }
    }

    // Summary
    printf("----------------------------------------\n");
    printf("Memory Usage Summary:\n");
    printf("Total Used: %6u | Total Free: %6u | Fragmentation: %6u\n",
        totalUsed, totalFree,
        (totalFree > 0) ? (FreeBlock_t*)freeList->next != NULL : 0);
    printf("========================================\n\n");
}