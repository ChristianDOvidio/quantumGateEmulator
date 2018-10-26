#ifndef TEST_PROG_H_
#define TEST_PROG_H_
#include "stdbool.h"
#include "stdint.h"

void validate_system_status(void);
void FPGAInit(void);
void initSignals(void);
void writeGate(int column, int row, int gate);
void writeInput(int state, int input);
uint32_t readOutput(int reg);

void* mmap_ptr;
size_t mmap_length;
volatile u_int* write_en_ptr;
volatile u_int* read_en_ptr;
volatile u_int* reset_ptr;
volatile u_int* qin_ptr;
volatile u_int* qout_ptr;
#endif /*TEST_PROG_H_*/
