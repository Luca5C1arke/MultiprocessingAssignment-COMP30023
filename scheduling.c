#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <getopt.h>
#include <signal.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/signalfd.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <unistd.h>

#include "scheduling.h"
#include "memory.h"

/* ----------------------------------- CREATE LIST FUNCTIONS ------------------------------------*/

/* reads processes from a .txt file, puts them in a linked list and returns a pointer to the list */
node_t* buildProcesses(FILE* fp, int* num_processes) {
    node_t* head;
    // vars for process
    process_t* new_process;
    unsigned long int temp_time_arrived;
    char temp_process_name[MAXPROCESSNAMELEN];
    unsigned long int temp_service_time;
    unsigned int temp_memory_requirement;
    // create list
    head = createLinkedList();
    // open the file, read each line
    if (fp == NULL) {
            exit(EXIT_FAILURE);
    }
    while (fscanf(fp, "%10lu %8s %10lu %4u", &temp_time_arrived, (char*)&temp_process_name, &temp_service_time, &temp_memory_requirement) != -1) {
        // create a new process_t struct for this line
        // DEBUGGING printf("Read process %lu %s %lu %u\n", temp_time_arrived, (char*)&temp_process_name, temp_service_time, temp_memory_requirement);
        new_process = createProcess(temp_time_arrived, temp_process_name, temp_service_time, temp_memory_requirement);
        insertProcess(new_process, head);
        (*num_processes)++;
    }
    fclose(fp);
    // return the pointer to the process list
    return head;
}

/* creates a linked list and returns the pointer to the head of the list */
node_t* createLinkedList() {
    node_t* head = NULL;
    head = (node_t*) malloc(sizeof(node_t));
    if (head == NULL) {
        exit(EXIT_FAILURE);
    }
    head->next = NULL;
    head->prev = NULL;
    return head;
}

/* ---------------------------------- ALTER PROCESS FUNCTIONS -----------------------------------*/

/* reads a line of input and creates a new process, returning a pointer to its struct */
process_t* createProcess(unsigned long int new_time_arrived, char new_process_name[MAXPROCESSNAMELEN], unsigned long int new_service_time, unsigned int new_memory_requirement) {
    process_t* ptr = (process_t*) malloc(sizeof(process_t));
    if (ptr == NULL) {
        exit(EXIT_FAILURE);
    }
    ptr->time_arrived = new_time_arrived;
    for (int i=0; i<MAXPROCESSNAMELEN; i++) {
            ptr->process_name[i] = new_process_name[i];
    }
    ptr->service_time = new_service_time;
    ptr->time_remaining = new_service_time;
    ptr->memory_requirement = new_memory_requirement;
    ptr->status = 0;
    ptr->mem_loc = -1;
    ptr->child_process = -1;
    ptr->has_run = 0;
    return ptr;
}

/* inserts a process into the linked list */
void insertProcess(process_t* a_process, node_t* head) {
    node_t* temp;
    node_t* new_node = (node_t*)malloc(sizeof(node_t));
    if (new_node ==NULL) {
        exit(EXIT_FAILURE);
    }
    new_node->next = NULL;
    new_node->prev = NULL;
    new_node->process = a_process;
    if (head->next == NULL) {
        // then this is the first entry into the list
        head->next = new_node;
        new_node->prev = head;
    } else {
        // insert it at the end
        temp = head->prev;
        temp->next = new_node;
        new_node->prev = temp;
    }
    head->prev = new_node; // new end of the list
    //DEBUGGING printf("%p -> [  %p  ][  %p  ][  %p  ]\nta:%lu pn:%s st:%lu mr:%u\n\n", new_node, new_node->prev, new_node->process, new_node->next, (new_node->process)->time_arrived, (new_node->process)->process_name, (new_node->process)->service_time, (new_node->process)->memory_requirement);
}

/* removes a process struct from the list */
void deleteProcess(process_t* process, node_t* list) {
    // finds the overall pointer of the process in the list
    if (list->next == NULL) {
        // then this list was empty
        printf("Error, this list was empty\n");
        exit(EXIT_FAILURE);
    }
    node_t* temp = list->next;
    node_t* del_node = NULL;
    int esc=0;
    while ((temp->next != NULL) && (esc==0)) {
        if ((temp->process) == process) {
            // then this is the process we are trying to delete
            del_node = temp;
            esc=1;
        } else {
            temp = temp->next;
        }
    }
    if (esc == 0) {
        if ((temp->process) == process) {
            // then this is the process we are trying to delete
            del_node = temp;
        } else {
            printf("ERROR: Process was not present in the list\n");
            exit(EXIT_FAILURE);
        }
    }
    if ((list->next)->next == NULL) {
        // then list only had one node
        list->next = NULL;
        list->prev = NULL;
    } else if (list->prev == del_node) {
        // then this was the last node in the list
        list->prev = del_node->prev;
        (del_node->prev)->next = NULL;
    } else {
        // otherwise is just a normal node
        (del_node->prev)->next = del_node->next;
        (del_node->next)->prev = del_node->prev;
    }
    //DEBUGGING printf("Deleted process %p at %p\n", del_node->process, del_node);
}

/* finds a given process in another list, returning its alternative address */
process_t* findProcess(process_t* process, node_t* head) {
     if (head->next == NULL) {
        // then this list was empty
        return NULL;
    }
    node_t* temp_node = head->next;
    while (head->next != NULL) {
        if (strcmp(process->process_name, (temp_node->process)->process_name) == 0){
            // then we have found the process
            return (temp_node->process);
        } else {
            temp_node = temp_node->next;
        }
    }
    // and once more
    if (strcmp(process->process_name, (temp_node->process)->process_name) == 0) {
        // then we have found the process
        return (temp_node->process);
    } else {
        return NULL;
    }
}

/* -------------------------------------- SCHEDULING LOGIC --------------------------------------*/

/* checks how many processes are newly-ready, allocates them to memory and returns a boolean of found(1)/none found(0) */
int checkReadyProcesses(node_t* head, node_t* READY_list, int* READY_count, int simtime, int mem_alloc, int* memory) {
    node_t* temp_node = head->next;
    int l=0;
    int r=0;
    if (temp_node->next == NULL) {
        printf("ERROR: No processes are available");
        exit(EXIT_FAILURE);
    }
    while (l == 0) {
        // if the proc
        if ((((temp_node->process)!=NULL) && (((temp_node->process)->status == 0) || ((temp_node->process)->status == 5)) && (((temp_node->process)->time_arrived) <= simtime))) {
            // then we have found a new READY or a WAITING process
            if (mem_alloc == 1) {
                int alloc_start = -1;
                alloc_start = allocateMemory(temp_node->process, memory);
                if (alloc_start > MEMORY_CAPACITY) {
                    printf("Error, memory was allocated beyond space\n");
                    exit(EXIT_FAILURE);
                } else if (alloc_start == -1) {
                    if ((temp_node->process)->status != 5) {
                        (*READY_count)++;;
                    }
                    (temp_node->process)->status = 5;
                } else {
                    if ((temp_node->process)->status != 5) {
                        (*READY_count)++;;
                    }
                    (temp_node->process)->status = 1;
                    insertProcess(temp_node->process, READY_list);
                    r=1;
                    insertMem(temp_node->process, memory, alloc_start);
                    printf("%d,READY,process_name=%s,assigned_at=%d\n", simtime, (temp_node->process)->process_name, alloc_start);
                }
            } else {
                (temp_node->process)->status = 1;
                insertProcess(temp_node->process, READY_list);
                r=1;
                (*READY_count)++;
            }
        }
        if (temp_node->next != NULL) {
            temp_node = temp_node->next;
        } else {
            // break here
            l=1;
        }
    } 
    return r;
}

/* deletes and frees up an entire list from memory*/
void freeList(node_t* list) {
    node_t* temp_node;
    if (list->next == NULL) {
        free(list);
        return;
    } else {
        temp_node = list->next;
    }
    while (temp_node->next == NULL) {
        free((temp_node->prev)->process);
        free((temp_node->prev));
        temp_node = temp_node -> next;
    }
    free((temp_node->prev)->process);
    free((temp_node->prev));
    free(temp_node->process);
    free(temp_node);
}

/* --------------------------------------- SJF FUNCTIONS ----------------------------------------*/

/* SJF runtime algorithm */
void runtimeSJF(node_t* head, int mem_strategy, int q, int* num_proc_left) {
    int simtime = 0;
    int temp_turnaround_time = 0;
    int total_turnaround_time = 0;
    float temp_overhead = 0.0;
    float total_overhead = 0.0;
    int init_processes = (*num_proc_left);
    float max_time_overhead = 0.0;
    process_t* curr_process = NULL;
    process_t* original_process = NULL;
    int READY_count = 0;
    node_t* READY_head = createLinkedList(); // a list of nodes that are READY
    // the memory declarations for task 3
    int memory[MEMORY_CAPACITY] = {0}; // the simulated 'memory'
    /* debugging
    printf("\n********** RUNNING ************\n");
    printf("Init: q=%d, num_procesess=%d, READY_list=%p\n", q, *num_proc_left, READY_head);
    */
    // while there are processes not finished
    while ((*num_proc_left) != 0) {
        // initialisation phase
        if (simtime == 0) {
            // increment until a process is added to the READY queue
            while (checkReadyProcesses(head, READY_head, &READY_count, simtime, mem_strategy, memory) == 0) {
                simtime+=q;
            }
            // then there are newly ready processes!
            if (curr_process == NULL) {
                // nothing is currently running (which there shouldn't be)
                // select the SJF process
                curr_process = selectProcessSJF(READY_head);
                original_process = findProcess(curr_process, head);
                if (original_process == NULL) {
                    printf("Error, tried to get address from list where process wasn't present\n");
                    exit(EXIT_FAILURE);
                }
                if (!curr_process) {
                    // then something went wrong with SJF
                    printf("ERROR: No process was allocated when expected\n");
                    exit(EXIT_FAILURE);
                } else {
                     // update the process' status and print that it is running
                    curr_process->status = 2;
                    READY_count--;
                    // execute the process
                    initialRunProcess(simtime, original_process);
                    // as initial process, needs extra loop
                    printf("%d,RUNNING,process_name=%s,remaining_time=%ld\n", simtime, curr_process->process_name, (curr_process->service_time));
                }
            } else {
                printf("Error, a process was assigned before initialisation\n");
                exit(EXIT_FAILURE);
            }
        }
        // RUNNING phase
        // if process is currently running
        if (((signed long)(curr_process->time_remaining)) > 0) {
            // take off q
            if (simtime != 0) {
                continueRunProcess(simtime, original_process);
            }
            (curr_process->time_remaining)-=q;
            // whether there are any new READY processes
            checkReadyProcesses(head, READY_head, &READY_count, simtime, mem_strategy, memory);
        } else {
        // then process now FINISHED
            (curr_process->time_remaining)=0;
            printf("%d,FINISHED,process_name=%s,proc_remaining=%d\n", simtime, curr_process->process_name, READY_count);
            (*num_proc_left)--;
            curr_process->status = 3;
            endRunProcess(simtime, original_process);
            // do some calculations for results
            temp_turnaround_time = simtime-(curr_process->time_arrived);
            total_turnaround_time += temp_turnaround_time;
            temp_overhead = (float)temp_turnaround_time/(curr_process->service_time);
            total_overhead += temp_overhead;
            if (temp_overhead > (max_time_overhead)) {
                max_time_overhead = temp_overhead;
            }
            // and remove the process from the READY queue
            if (mem_strategy == 1) {
                freeMem(curr_process, memory);
            }
            deleteProcess(curr_process, READY_head);
            // now we need to run a new process
            if ((*num_proc_left) != 0) {
                // check if any READY's coincided
                checkReadyProcesses(head, READY_head, &READY_count, simtime, mem_strategy, memory);
                // otherwise loop to find processes that are are READY
                while (READY_count == 0) {
                    // increment
                    simtime += q;
                    // and check if there are any ready processes
                    checkReadyProcesses(head, READY_head, &READY_count, simtime, mem_strategy, memory);
                }
                // and now that there is at least one READY process
                // select the SJF process
                curr_process = selectProcessSJF(READY_head);
                original_process = findProcess(curr_process, head);
                if (original_process == NULL) {
                    printf("Error, tried to get address from list where process wasn't present\n");
                    exit(EXIT_FAILURE);
                }
                if (!curr_process) {
                    printf("ERROR: No process was allocated when expected\n");
                    exit(EXIT_FAILURE);
                } else {
                     // update the process' status and print that it is running
                    curr_process->status = 2;
                    READY_count--;
                    initialRunProcess(simtime, original_process);
                    printf("%d,RUNNING,process_name=%s,remaining_time=%ld\n", simtime, curr_process->process_name, curr_process->service_time);
                    // and remove one quantum to start with
                    (curr_process->time_remaining)-=q;
                }
            }
        }
        // and with each cycle increment simtime by 1 quantum
        simtime += q;
    }
    // calculate and print out the results
    printf("Turnaround time %d\nTime overhead %.2f %.2f\nMakespan %d\n", roundup((float)total_turnaround_time/init_processes), 
      max_time_overhead, (float)total_overhead/init_processes, (simtime-q));
    freeList(head);
    freeList(READY_head);
}


/* selects the smallest runtime out of the READY list and returns that process */
process_t* selectProcessSJF(node_t* READY_list) {
    node_t* temp = READY_list->next;
    process_t* best = ((READY_list->next)->process);
    // sorts by service time, then arrival time, then by lexographical process name order
    while (temp->next != NULL) {
        if ((temp->process)->service_time < (best->service_time)) {
            // then temp's process has the new best service time
            best = temp->process;
        } else if ((temp->process)->service_time == (best->service_time)) {
            // tiebreaker: compare arrival time
            if ((temp->process)->time_arrived < (best->time_arrived)) {
                best = temp->process;
            } else if ((temp->process)->service_time == (best->service_time)) {
                //tiebreaker: compare process name
                if ((strcmp(((temp->process)->process_name), (best->process_name))) < 0) {
                    best = temp->process;
                }
            }
        }
        temp = temp->next;
    }
    // and then one more time
    if ((temp->process)->service_time < (best->service_time)) {
        // then temp's process has the new best service time
        best = temp->process;
    } else if ((temp->process)->service_time == (best->service_time)) {
        // tiebreaker: compare arrival time
        if ((temp->process)->time_arrived < (best->time_arrived)) {
            best = temp->process;
        } else if ((temp->process)->service_time == (best->service_time)) {
            //tiebreaker: compare process name
            if ((strcmp(((temp->process)->process_name), (best->process_name))) < 0) {
                best = temp->process;
            }
        }
    }
    return best;
}

/* --------------------------------------- RR FUNCTIONS -----------------------------------------*/

/* RR runtime algorithm */
void runtimeRR(node_t* head, int mem_strategy, int q, int* num_proc_left) {
    int simtime = 0;
    int temp_turnaround_time = 0;
    int total_turnaround_time = 0;
    float temp_overhead = 0.0;
    float total_overhead = 0.0;
    int init_processes = (*num_proc_left);
    float max_time_overhead = 0.0;
    int swapped=0;
    process_t* curr_process = NULL;
    int READY_count = 0;
    node_t* READY_head = createLinkedList(); // a list of nodes that are READY
    // the memory declarations for task 3
    int memory[MEMORY_CAPACITY] = {0}; // the simulated 'memory'
    while ((*num_proc_left) > 0) {  
        // INITIALISATION
        if (simtime == 0) {
            while (checkReadyProcesses(head, READY_head, &READY_count, simtime, mem_strategy, memory) != 1) {
                // increment simtime by q until the first process(es) are encountered
                simtime += q;
            }
            // do an initial run
            curr_process = (READY_head->next)->process;
            curr_process->status = 2;
            initialRunProcess(simtime, curr_process);
            curr_process->has_run = 1;
            printf("%d,RUNNING,process_name=%s,remaining_time=%ld\n", simtime, curr_process->process_name, curr_process->time_remaining);
            simtime+=q;
            curr_process->time_remaining-=q;
        }
        // MAIN LOOP, should here have a READY_list with >=1 processes at arbitrary simtime.
        checkReadyProcesses(head, READY_head, &READY_count, simtime, mem_strategy, memory);
        // if multiple processes are READY:
        if (READY_count > 1) {
            // set the current process back to READY state
            curr_process->status=1;
            // rotate the list
            if (swapped==0) {
                pauseRunProcess(simtime, curr_process);
                swapRRProcesses(READY_head);
            } else {
                swapped=0;
            }
            // run the new head process for a quantum
            curr_process = (READY_head->next)->process;
            curr_process->status = 2;
            // see if this is a brand new process or has already run before
            if (curr_process->has_run == 0) {
                initialRunProcess(simtime, curr_process);
                curr_process->has_run = 1;
            } else {
                continueRunProcess(simtime, curr_process);
            }
            printf("%d,RUNNING,process_name=%s,remaining_time=%ld\n", simtime, curr_process->process_name, curr_process->time_remaining);
            simtime+=q;
            curr_process->time_remaining-=q;
            if ((int)curr_process->time_remaining <=0) {
                // FINISHED LOOP
                (curr_process->time_remaining)=0;
                (*num_proc_left)--;
                (READY_count)--;
                printf("%d,FINISHED,process_name=%s,proc_remaining=%d\n", simtime, curr_process->process_name, READY_count);
                curr_process->status = 3;
                endRunProcess(simtime, curr_process);
                //notify that a new process will run
                swapped = 1;
                // do some calculations for results
                temp_turnaround_time = simtime-(curr_process->time_arrived);
                total_turnaround_time += temp_turnaround_time;
                temp_overhead = (float)temp_turnaround_time/(curr_process->service_time);
                total_overhead += temp_overhead;
                if (temp_overhead > (max_time_overhead)) {
                    max_time_overhead = temp_overhead;
                }
                // and remove the process from the READY queue
                if (mem_strategy == 1) {
                    freeMem(curr_process, memory);
                }
                deleteProcess(curr_process, READY_head);
                // if there was only one left, make that the new currprocess
                if (READY_count == 1) {
                    curr_process = (READY_head->next)->process;
                    if (curr_process->has_run == 0) {
                        initialRunProcess(simtime, curr_process);
                        curr_process->has_run = 1;
                    }
                }
                // FINISHED LOOP END
            }
        } // if just the one READY process:
        else if (READY_count == 1) {
            // run the process for a quantum
            if (curr_process->has_run == 0) {
                initialRunProcess(simtime, curr_process);
                curr_process->has_run = 1;
            } else {
                continueRunProcess(simtime, curr_process);
            }
            if (swapped == 1) {
                printf("%d,RUNNING,process_name=%s,remaining_time=%ld\n", simtime, curr_process->process_name, curr_process->time_remaining);
                swapped = 0;
            }
            simtime+=q;
            curr_process->time_remaining-=q;
            if ((int)curr_process->time_remaining <= 0) {
                // FINISHED LOOP
                (curr_process->time_remaining)=0;
                (*num_proc_left)--;
                (READY_count)--;
                printf("%d,FINISHED,process_name=%s,proc_remaining=%d\n", simtime, curr_process->process_name, READY_count);
                curr_process->status = 3;
                endRunProcess(simtime, curr_process);
                //notify that a new process will run
                swapped = 1;
                // do some calculations for results
                temp_turnaround_time = simtime-(curr_process->time_arrived);
                total_turnaround_time += temp_turnaround_time;
                temp_overhead = (float)temp_turnaround_time/(curr_process->service_time);
                total_overhead += temp_overhead;
                if (temp_overhead > (max_time_overhead)) {
                    max_time_overhead = temp_overhead;
                }
                // and remove the process from the READY queue
                if (mem_strategy == 1) {
                    freeMem(curr_process, memory);
                }
                deleteProcess(curr_process, READY_head);
                // now run until there is another process waiting and assign that one
                if ((*num_proc_left) != 0) {
                    while (checkReadyProcesses(head, READY_head, &READY_count, simtime, mem_strategy, memory) != 1) {
                        // increment simtime by q until process(es) are encountered
                        simtime += q;
                    }
                }
                // if there was only one process left, make that the curr process
                if (READY_count == 1) {
                    curr_process = (READY_head->next)->process;
                    if (curr_process->has_run == 0) {
                        initialRunProcess(simtime, curr_process);
                        curr_process->has_run = 1;
                    }
                }
                // FINISHED LOOP END
            }
        } // if no READY processes
        else {
            while (checkReadyProcesses(head, READY_head, &READY_count, simtime, mem_strategy, memory) != 1) {
                // increment simtime by q until process(es) are encountered
                simtime += q;
            }
            // now have got one
            curr_process = (READY_head->next)->process;
            initialRunProcess(simtime, curr_process);
        }
        // END OF MAIN LOOP
    }
    // calculate and print out the results
    printf("Turnaround time %d\nTime overhead %.2f %.2f\nMakespan %d\n", roundup((float)total_turnaround_time/init_processes), 
      max_time_overhead, (float)total_overhead/init_processes, (simtime));
    freeList(head);
    freeList(READY_head);
}

/* moves the current head of the list to the end of the list */
void swapRRProcesses(node_t* list) {
    node_t *firstn, *secondn, *lastn;
    firstn = (list->next);
    secondn = (list->next)->next;
    lastn = (list->prev);
    firstn->prev = lastn;
    firstn->next = NULL;
    lastn->next = firstn;
    secondn->prev = list;
    list->prev = firstn;
    list->next = secondn;
}

/* ---------------------------------- TASK 4 FUNCTIONS ------------------------------------------*/

void initialRunProcess(int curr_time, process_t* ptr) {
    int in_fd[2];
    int out_fd[2];
    if (pipe(in_fd) == -1 || pipe(out_fd) == -1) {
        printf("ERROR: Pipe in an initial process run failed\n");
        exit(EXIT_FAILURE);
    }
    ptr->child_process = fork();
    if (ptr->child_process == -1) {
        printf("ERROR: Process could not be forked\n");
        exit(EXIT_FAILURE);
    } else if (ptr->child_process == 0) {
        close(in_fd[1]);
        dup2(in_fd[0], STDIN_FILENO);
        close(out_fd[0]);
        dup2(out_fd[1], STDOUT_FILENO);
        char* args[] = {"./process", ptr->process_name, NULL};
        if (execv(args[0], args) == -1) {
            printf("ERROR: Execv failed");
            exit(EXIT_FAILURE);
        }
    }
    close(in_fd[0]);
    close(out_fd[1]);
    ptr->in_fd[1] = in_fd[1];
    ptr->out_fd[0] = out_fd[0];
    // convert the time to big endian
    uint32_t big_e_time = htonl(curr_time);
    if (write(in_fd[1], &big_e_time, sizeof(big_e_time)) == -1) {
        printf("ERROR: Could not write\n");
        exit(EXIT_FAILURE);
    }
    uint8_t byte;
    if(read(out_fd[0], &byte, sizeof(byte)) == -1) {
        printf("ERROR: Could not read byte in init\n");
        exit(EXIT_FAILURE);
    }   
    if (byte != ((uint8_t*)&curr_time)[0]) {
        printf("ERROR: Byte did not match in init\n");
    }
}


void continueRunProcess(int curr_time, process_t* ptr) {
    uint32_t big_e_time = htonl(curr_time);
    if (write(ptr->in_fd[1], &big_e_time, sizeof(big_e_time)) == -1) {
        printf("ERROR: Could not write\n");
        exit(EXIT_FAILURE);
    }
    kill(ptr->child_process, SIGCONT);
    uint8_t byte;
    if(read(ptr->out_fd[0], &byte, sizeof(byte)) == -1) {
        printf("ERROR: Could not read byte in continue\n");
        exit(EXIT_FAILURE);
    }   
    if (byte != ((uint8_t*)&curr_time)[0]) {
        printf("ERROR: byte did not match in continue\n");
    }
}

void pauseRunProcess(int curr_time, process_t* ptr) {
    int loop = 1;
    while (loop == 1) {
        uint32_t big_e_time = htonl(curr_time);
        write(ptr->in_fd[1], &big_e_time, sizeof(big_e_time));
        kill(ptr->child_process, SIGTSTP);
        int wait_status;
        waitpid(ptr->child_process, &wait_status, WUNTRACED);
        if (WIFSTOPPED(wait_status)) {
            loop = 0;
            break;
        } else {
            continue;
        }
    }
}

void endRunProcess(int curr_time, process_t* ptr) {
    uint32_t big_e_time = htonl(curr_time);
    write(ptr->in_fd[1], &big_e_time, sizeof(big_e_time));
    kill(ptr->child_process, SIGTERM);
    char sha[65];
    sha[64] = '\0';
    if (read(ptr->out_fd[0], &sha, 64) == -1) {
        printf("ERROR: The sha failed to generate, seen during the array allocation\n");
        exit(EXIT_FAILURE);
    }
    printf("%d,FINISHED-PROCESS,process_name=%s,sha=%s\n", curr_time, ptr->process_name, sha);
}


/* ----------------------------------- OTHER MISC FUNCTIONS -------------------------------------*/

/* roundup function, returns the rounded up float*/
int roundup(float num) {
    int temp_num;
    temp_num = (int)num;
    if ((float)temp_num < num) {
        temp_num++;
    }
    return temp_num;
}


/* DEBUGGING FUNCTION - prints out a list */
void printList(node_t* head) {
    if (head->next == NULL) {
        printf("List was empty\n");
        return;
    }
    node_t* temp = head->next;
    printf("LIST:");
    while (temp->next != NULL) {
        printf(" %s (%d)", (temp->process)->process_name, (temp->process)->status);
        temp = temp->next;
    }
    printf(" %s (%d)\n", (temp->process)->process_name, (temp->process)->status);
}
