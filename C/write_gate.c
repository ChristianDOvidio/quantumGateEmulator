#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <error.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <termios.h>
#include <sys/utsname.h>
#include <stdbool.h>

#include "hps_0_arm_a9_0.h"
#include "write_gate.h"

/*

  Author - James Kiessling

  Writes the specified value to the selecting MUX of the specified row/col

  Requires the COLUMN, ROW, and MUXVAL as args from the terminal:

  COLUMN - The column number of the current mux that is being written
  ROW - The row number of the current mux that is being written. This essentially 
        specifies the gate that is currently being analyzed.
  MUXVAL - The row number of the previous column so that the output of the previous 
           column can be connected to the input of the current column

  Note - This program should only be run by the python webserver. The print statements
         and error handling are specifically designed to be run by the webserver.

*/

//Main function opens the memory map and writes to the row/col specified
int main(int argc, char* argv[])
{
  if(argc != 4){
    printf("-1");
    return(0);
  }

  //extract info
  int col = atoi(argv[1]);
  int row = atoi(argv[2]);
  int gate = atoi(argv[3]);

  FPGAInit(); //open memory map
  writeGate(col, row, gate);

  //close memory map
  int result = munmap(mmap_ptr, mmap_length);
  if(result < 0)
    printf("-2");

  return(0);
}

//opens memory map
void FPGAInit(void)
{
  int dev_mem_fd;
  void* mmap_addr;
  int mmap_prot;
  int mmap_flags;
  int mmap_fd;
  off_t mmap_offset;

  dev_mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
  if(dev_mem_fd < 0){
    printf("-2");
    return;
  }

  mmap_addr = NULL;
  mmap_length = WRITE_ENABLE_SPAN + READ_ENABLE_SPAN + RESET_SPAN + QIN_SPAN + QOUT_SPAN;
  mmap_prot = PROT_READ | PROT_WRITE;
  mmap_flags = MAP_SHARED;
  mmap_fd = dev_mem_fd;
  mmap_offset = WRITE_ENABLE_BASE & ~(sysconf(_SC_PAGE_SIZE) -1);
  mmap_ptr = mmap(mmap_addr, mmap_length, mmap_prot, mmap_flags,
                  mmap_fd, mmap_offset);

  if(mmap_ptr == MAP_FAILED){
    printf("-2");
    return;
  }

  write_en_ptr = (u_int*)((u_int)mmap_ptr + (WRITE_ENABLE_BASE & (sysconf(_SC_PAGE_SIZE)-1)));
  read_en_ptr = (u_int*)((u_int)mmap_ptr + (READ_ENABLE_BASE & (sysconf(_SC_PAGE_SIZE)-1)));
  reset_ptr = (u_int*)((u_int)mmap_ptr + (RESET_BASE & (sysconf(_SC_PAGE_SIZE)-1)));
  qin_ptr = (u_int*)((u_int)mmap_ptr + (QIN_BASE & (sysconf(_SC_PAGE_SIZE)-1)));
  qout_ptr = (u_int*)((u_int)mmap_ptr + (QOUT_BASE & (sysconf(_SC_PAGE_SIZE)-1)));

  close(dev_mem_fd);
}

//Write the specified gate mux value to the row/col
void writeGate(int column, int row, int gate)
{
  //Format is '1cccrrrr'
  int writeIdx = 128 + (column << 4) + row;

  //Choose (col, row)
  write_en_ptr[0] = 0;
  qin_ptr[0] = writeIdx;
  write_en_ptr[0] = 1;

  usleep(1);

  //Select gate from previous stage
  write_en_ptr[0] = 0;
  qin_ptr[0] = gate;
  write_en_ptr[0] = 1;

  usleep(1);  
}
