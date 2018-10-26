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
#include "read_output.h"

/*

  Author - James Kiessling

  Reads the specified output register of the circuit

  Requires the specific output register REG as input from the terminal:

  REG - Integer value corresponding to the index of the output register to read, i.e.
        if reading the output of qubit 1, REG = 0.

  Note - This program should only be run from the python webserver. All print statements
         and error handling are designed to be run from the webserver.

*/

//Main function parses CLI args, initializes memory map, and reads output
int main(int argc, char* argv[])
{
  if(argc != 2){
    printf("-1"); //Error code for python webserver
    return(0);
  }

  int reg = atoi(argv[1]);

  FPGAInit();
  uint32_t output = readOutput(reg);
  printf("%d", output);

  //close memory map
  int result = munmap(mmap_ptr, mmap_length);
  if(result < 0)
    printf("-2");

  return(0);
}

//Open memory map
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

//Reads the output at the specified output register and returns the value
uint32_t readOutput(int reg)
{
  write_en_ptr[0] = 0;
  qin_ptr[0] = reg;
  write_en_ptr[0] = 1;

  usleep(1);

  write_en_ptr[0] = 0;
  read_en_ptr[0] = 1;
  uint32_t output = qout_ptr[0];
  read_en_ptr[0] = 0;

  return output;  
}
