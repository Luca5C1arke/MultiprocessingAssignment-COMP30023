#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scheduling.h"
#include "memory.h"

/* --------------------------------------- MAIN FUNCTION ----------------------------------------*/

int main(int argc, char** argv) {
    // variable declarations
    int clis[4] = {0};
    int* num_processes = (int*)malloc(sizeof(int));
    *num_processes = 0;
    FILE* fp = NULL;
    int mem_strategy=0; // 0=infinite, 1=best-fit...

    // reads from cli and stores arguments as index of argv's
    // [-filename (0), -scheduler (1), -memory-strategy (2), -quantum(3)]
    for (int i=0; i<argc; i++) {
        if (!strcmp(argv[i],"-f")) {
            clis[0]=i+1;
        } else if (!strcmp(argv[i],"-s")) {
            clis[1]=i+1;
        } else if (!strcmp(argv[i],"-m")) {
            clis[2]=i+1;
        } else if (!strcmp(argv[i],"-q")) {
            clis[3]=i+1;
        } else {
            /* do nothing */
        }
    }
    // determine if best fit is being implemented
    if (!strcmp(argv[clis[2]], "best-fit")) {
        mem_strategy = 1;
    }
    // printf("PROCESSES\n-f: %s\n-s: %s\n-m: %s\n-q: %s\n", argv[clis[0]], argv[clis[1]], argv[clis[2]], argv[clis[3]]);
    // open the file and create the input process list
    fp = fopen(argv[clis[0]], "r");
    if (fp == NULL) {
        printf("Error, could not find an input file of that name\n");
        exit(EXIT_FAILURE);
    }
    node_t* process_list = buildProcesses(fp, num_processes);
    // determine which scheduler and run
     if (!strcmp(argv[clis[1]],"SJF")) {
        runtimeSJF(process_list, mem_strategy, atoi(argv[clis[3]]), num_processes);
    } else if (!strcmp(argv[clis[1]],"RR")) {
        runtimeRR(process_list, mem_strategy, atoi(argv[clis[3]]), num_processes);
    } else {
        printf("Error, unknown scheduler\n");
        exit(EXIT_FAILURE);
    }

    return 0;
}