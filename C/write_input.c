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
#include "write_input.h"

/*

  Author - James Kiessling
  
  Updates the specified input register of the circuit with the state given

  Requires two arguments from the terminal - STATE and REGISTER:

  STATE - The state of the qubit. The state should either be |0} or |1}, where
          |0} = 0x7F000000 == 2130706432
          |1} = 0x00007F00 == 32512

  REGISTER - An integer indicating which input register should be written to. I.e.,
             if changing the initial state of the first qubit, REGISTER = 0.

  Note - this program should only be run from the python webserver. All print
         statements and error checking are specifically implemented for the webserver.
*/

//Parses CLI args to determine STATE/REGISTER, initializes memory map and writes to
//input register
int main(int argc, char* argv[])
{
  if(argc != 3){
    printf("-1"); //Checked in python script --> -1 corresponds to error
    return(0);
  }

  int state = atoi(argv[1]);
  int input = atoi(argv[2]);

  if(input > 15){
    printf("-1");
    return(0);
  }

  FPGAInit(); //Open memory map
  initSignals(); //Soft reset for top-level entity in circuit
  writeInput(state, input);

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

//Initialize signals / soft reset of top-level entity in circuit
void initSignals(void)
{
  reset_ptr[0] = 0; //Reset is active low
  qin_ptr[0] = 0;
  read_en_ptr[0] = 0;
  write_en_ptr[0] = 0;
  reset_ptr[0] = 1;  
}

//Write the specified state to the specified input register
void writeInput(int state, int input)
{
  //Choose the Input register
  write_en_ptr[0] = 0;
  qin_ptr[0] = input;
  write_en_ptr[0] = 1;

  usleep(1);

  //Assign initial state
  write_en_ptr[0] = 0;
  qin_ptr[0] = state;
  write_en_ptr[0] = 1;

  usleep(1);
}
