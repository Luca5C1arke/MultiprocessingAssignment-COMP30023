#include <stdio.h>
#include <stdlib.h>

#include "scheduling.h"
#include "memory.h"

// put the memory-based functions in here, and their declaration in memory.h
int allocateMemory(process_t* process, int* memory) {
    int i=0, start=-1, end=-1, gap=-1, best_loc=-1, b_gap=(MEMORY_CAPACITY+1), size=0;
    size = process->memory_requirement;
    // run through the entire memory, find the smallest gap that is bigger than the memory requirement
    while (i<(MEMORY_CAPACITY-1)) {
        // if slot is free
        if (memory[i] == 0) {
            // we have found a gap
            start = i;
            // find the end of the gap
            while ((memory[(i+1)] == 0) && (i<(MEMORY_CAPACITY))) {
                i++;
            }
            // now have found the end
            end = i;
            gap = (end-start)+1;
            if ((gap < b_gap) && (gap >= size)) {
                // we have found the current best fit gap
                b_gap = gap;
                best_loc = start;
            }
            i++;
        } else {
            // slot is occupied
            i++;
        }
    }
    return best_loc; //will return -1 if no gap was found
}

void insertMem(process_t* process, int* memory, int alloc_start) {
    int end=(alloc_start+(process->memory_requirement)-1);
    process->mem_loc = alloc_start;
    //DEBUGGING printf("Memory for %s of size req %d allocated at address %d-%d\n", process->process_name, process->memory_requirement, alloc_start, end);
    for (int i=alloc_start; i<=end; i++) {
        if (memory[i] == 1) {
            printf("Error, tried to allocate to already reserved space\n");
            exit(EXIT_FAILURE);
        }
        memory[i] = 1;
    }
}

void freeMem(process_t* process, int* memory) {
    int start = process->mem_loc, end=(start+(process->memory_requirement)-1);
    //DEBUGGING printf("FREED UP Memory for %s of size req %d at address %d-%d\n", process->process_name, process->memory_requirement, start, end);
    for (int i=start; i<=end; i++) {
        if (memory[i] == 0) {
            printf("Error, tried to deallocate already free space\n");
            exit(EXIT_FAILURE);
        }
        memory[i] = 0;
    }
}