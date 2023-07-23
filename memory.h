/* function headers */
int allocateMemory(process_t* process, int* memory);
void insertMem(process_t* process, int* memory, int alloc_start);
void freeMem(process_t* process, int* memory);