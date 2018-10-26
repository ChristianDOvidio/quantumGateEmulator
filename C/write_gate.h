#ifndef WRITE_GATE_H_
#define WRITE_GATE_H_
#include "stdint.h"

void FPGAInit(void);
void writeGate(int column, int row, int gate);

void* mmap_ptr;
size_t mmap_length;
volatile u_int* write_en_ptr;
volatile u_int* read_en_ptr;
volatile u_int* reset_ptr;
volatile u_int* qin_ptr;
volatile u_int* qout_ptr;
#endif
