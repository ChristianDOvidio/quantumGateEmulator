#ifndef READ_OUTPUT_H_
#define READ_OUTPUT_H_
#include "stdint.h"

void FPGAInit(void);
uint32_t readOutput(int reg);

void* mmap_ptr;
size_t mmap_length;
volatile u_int* write_en_ptr;
volatile u_int* read_en_ptr;
volatile u_int* reset_ptr;
volatile u_int* qin_ptr;
volatile u_int* qout_ptr;
#endif
