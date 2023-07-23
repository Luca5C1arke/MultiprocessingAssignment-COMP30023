/* definitions */
#define MAXPROCESSNAMELEN 8
#define MEMORY_CAPACITY 2048
#include <wait.h>

/* process struct */

typedef struct process_t process_t;

struct process_t {
    unsigned long int time_arrived;
    char process_name[MAXPROCESSNAMELEN];
    unsigned long int service_time;
    unsigned int memory_requirement;
    unsigned long int time_remaining;
    int status; // 0=NULL, 1=READY, 2=RUNNING, 3=FINISHED, 4=WAITINGTOPRINT, 5=WAITINGONMEMORY
    int mem_loc; // the starting address of its location in memory
    int has_run; // 0=FALSE, 1=TRUE (for task 4 RR, to determine if initialised or not) 
    pid_t child_process;
    int in_fd[2];
    int out_fd[2];
};

/* linked list structs */
typedef struct node_t node_t;

struct node_t {
    node_t* prev;
    process_t* process;
    node_t* next;
};

/* function headers */
node_t* buildProcesses(FILE* fp, int* num_processes);
process_t* createProcess(unsigned long int time_arrived, char process_name[MAXPROCESSNAMELEN], unsigned long int service_time, unsigned int memory_requirement);
node_t* createLinkedList();
void insertProcess(process_t* a_process, node_t* head);
process_t* findProcess(process_t* process, node_t* head);
void runtimeSJF(node_t* head, int mem_strategy, int q, int* num_proc_left);
void runtimeRR(node_t* head, int mem_strategy, int q, int* num_proc_left);
process_t* selectProcessSJF(node_t* READY_list);
int checkReadyProcesses(node_t* head, node_t* READY_list, int* READY_count, int simtime, int mem_alloc, int* memory);
void swapRRProcesses(node_t* list);
int roundup(float num);
void printList(node_t* head);
void freeList(node_t* head);
void initialRunProcess(int simtime, process_t* ptr);
void continueRunProcess(int simtime, process_t* ptr);
void pauseRunProcess(int curr_time, process_t* ptr);
void endRunProcess(int simtime, process_t* ptr);