#ifndef WRITE_INPUT_H_
#define WRITE_INPUT_H_
#include "stdint.h"

void FPGAInit(void);
void initSignals(void);
void writeInput(int state, int input);

void* mmap_ptr;
size_t mmap_length;
volatile u_int* write_en_ptr;
volatile u_int* read_en_ptr;
volatile u_int* reset_ptr;
volatile u_int* qin_ptr;
volatile u_int* qout_ptr;
#endif /*TEST_PROG_H_*/
