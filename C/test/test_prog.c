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
#include "test_prog.h"

/*  Expected System Environment  */
#define SYSFS_FPGA0_STATE_PATH "/sys/class/fpga_manager/fpga0/state"
#define SYSFS_FPGA0_STATE "operating"

#define SYSFS_LWH2F_BRIDGE_NAME_PATH "/sys/class/fpga_bridge/br0/name"
#define SYSFS_LWH2F_BRIDGE_NAME "lwhps2fpga"

#define SYSFS_LWH2F_BRIDGE_STATE_PATH "/sys/class/fpga_bridge/br0/state"
#define SYSFS_LWH2F_BRIDGE_STATE "enabled"

#define SYSFS_H2F_BRIDGE_NAME_PATH "/sys/class/fpga_bridge/br1/name"
#define SYSFS_H2F_BRIDGE_NAME "hps2fpga"

#define SYSFS_H2F_BRIDGE_STATE_PATH "/sys/class/fpga_bridge/br1/state"
#define SYSFS_H2F_BRIDGE_STATE "enabled"

#define LED_DELAY_US 250000

#define STATE0 2130706432
#define IMAG0 8323072
#define STATE1 32512
#define IMAG1 127
#define BYPASS0 0
#define BYPASS1 1
#define BYPASS2 2
#define BYPASS3 3
#define BYPASS4 4
#define BYPASS5 5
#define PAULIX 6
#define PAULIY 7
#define PAULIZ 8
#define SQRTNOT 9
#define HADAMARD 10
#define CNOT0 11
#define CNOT1 12
#define TOFFOLI0 13
#define TOFFOLI1 14
#define TOFFOLI2 15

/*   Main function  */
int main(int argc, char* argv[])
{
  if(argc > 1){
    printf("All CLI arguments are being ignored.\n");
    printf("The following CLI arguments will be ignored: ");
    int i;
    for(i = 1; i < argc; i++){
      printf("%s ", argv[i]);
    }
    printf("\n");
  }
  
  //disp welcome message
  printf("\n");
  printf("Test app started...\n");
  printf("\n");

  validate_system_status();
  FPGAInit();

  //Initialize all signals
  initSignals();

  //Assign gate multiplexors
  writeInput(STATE1, 0);
  writeGate(0, PAULIX, 0);
  writeGate(1, 0, PAULIX);
  writeGate(2, 0, 0);
  writeGate(3, 0, 0);
  writeGate(4, 0, 0);
  writeGate(5, 0, 0);
  writeGate(6, 0, 0);
  writeGate(7, 0, 0);
  
  /*
  writeGate(0, TOFFOLI1, 1);
  writeGate(1, 1, TOFFOLI1);
  writeGate(2, 1, 1);
  writeGate(3, 1, 1);
  writeGate(4, 1, 1);
  writeGate(5, 1, 1);
  writeGate(6, 1, 1);
  writeGate(7, 1, 1);

  writeGate(0, TOFFOLI2, 2);
  writeGate(1, 2, TOFFOLI2);
  writeGate(2, 2, 2);
  writeGate(3, 2, 2);
  writeGate(4, 2, 2);
  writeGate(5, 2, 2);
  writeGate(6, 2, 2);
  writeGate(7, 2, 2);
  */
  
  //Write inputs
  //writeInput(STATE0, 0);
  //writeInput(STATE0, 1);
  //writeInput(STATE0, 2);

  //Read outputs
  uint32_t output0 = readOutput(0);
  //uint32_t output1 = readOutput(1);
  //uint32_t output2 = readOutput(2);

  //Print output result to console in hex
  printf("Final result: Qubit 0: %x\n", output0);
  //printf("Final result: Qubit 1: %x\n", output1);
  //printf("Final result: Qubit 2: %x\n", output2);
  
  //termination message  printf("Test app finished\n");

  //close memory map
  int result = munmap(mmap_ptr, mmap_length);
  if(result < 0)
    error(1, errno, "munmap");
  
  return(0);
}

void validate_system_status(void)
{
  int result;
  int sysfs_fd;
  char sysfs_str[256];

  //Validate the FPGA state
  sysfs_fd = open(SYSFS_FPGA0_STATE_PATH, O_RDONLY);
  if(sysfs_fd < 0){
    error(1, errno, "Could not open FPGA state");
  }
  result = read(sysfs_fd, sysfs_str, strlen(SYSFS_FPGA0_STATE));
  if(result < 0){
    error(1, errno, "Could not read FPGA state");
  }
  close(sysfs_fd);
  if(strncmp(SYSFS_FPGA0_STATE, sysfs_str, strlen(SYSFS_FPGA0_STATE))){
    error(1, 0, "FPGA not in operate state");
  }

  //Validate the LWH2F bridge name
  sysfs_fd = open(SYSFS_LWH2F_BRIDGE_NAME_PATH, O_RDONLY);
  if(sysfs_fd < 0){
    error(1, errno, "open sysfs LWH2F bridge name");
  }
  result = read(sysfs_fd, sysfs_str, strlen(SYSFS_LWH2F_BRIDGE_NAME));
  if(result < 0){
    error(1, errno, "read sysfs LWH2F bridge name");
  }
  close(sysfs_fd);
  if(strncmp(SYSFS_LWH2F_BRIDGE_NAME, sysfs_str, strlen(SYSFS_LWH2F_BRIDGE_NAME))){
    error(1, 0, "bad LWH2F bridge name");
  }

  //Validate the LWH2F bridge state
  sysfs_fd = open(SYSFS_LWH2F_BRIDGE_STATE_PATH, O_RDONLY);
  if(sysfs_fd < 0){
    error(1, errno, "open sysfs LWH2F bridge state");
  }
  result = read(sysfs_fd, sysfs_str, strlen(SYSFS_LWH2F_BRIDGE_STATE));
  if(result < 0){
    error(1, errno, "read sysfs LWH2F bridge state");
  }
  close(sysfs_fd);
  if(strncmp(SYSFS_LWH2F_BRIDGE_STATE, sysfs_str, strlen(SYSFS_LWH2F_BRIDGE_STATE))){
    error(1, 0, "LWH2F bridge not enabled");
  }

  //Validate the H2F bridge name
  sysfs_fd = open(SYSFS_H2F_BRIDGE_NAME_PATH, O_RDONLY);
  if(sysfs_fd < 0){
    error(1, errno, "open sysfs H2F bridge name");
  }
  result = read(sysfs_fd, sysfs_str, strlen(SYSFS_H2F_BRIDGE_NAME));
  if(result < 0){
    error(1, errno, "read sysfs H2F bridge name");
  }
  close(sysfs_fd);
  if(strncmp(SYSFS_H2F_BRIDGE_NAME, sysfs_str, strlen(SYSFS_H2F_BRIDGE_NAME))){
    error(1, 0, "bad H2F bridge name");
  }

  //Validate the H2F bridge state
  sysfs_fd = open(SYSFS_H2F_BRIDGE_STATE_PATH, O_RDONLY);
  if(sysfs_fd < 0){
    error(1, errno, "open sysfs H2F bridge state");
  }
  result = read(sysfs_fd, sysfs_str, strlen(SYSFS_H2F_BRIDGE_STATE));
  if(result < 0){
    error(1, errno, "read sysfs H2F bridge state");
  }
  close(sysfs_fd);
  if(strncmp(SYSFS_H2F_BRIDGE_STATE, sysfs_str, strlen(SYSFS_H2F_BRIDGE_STATE))){
    error(1, 0, "H2F bridge not enabled");
  }

  printf("FPGA appears to be configured and bridges are not in reset\n");
}

//Initialize the FPGA
void FPGAInit(void)
{
  int dev_mem_fd;
  void* mmap_addr;
  int mmap_prot;
  int mmap_flags;
  int mmap_fd;
  off_t mmap_offset;

  dev_mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
  if(dev_mem_fd < 0)
    error(1, errno, "open /dev/mem");

  mmap_addr = NULL;
  mmap_length = WRITE_ENABLE_SPAN + READ_ENABLE_SPAN + RESET_SPAN + QIN_SPAN + QOUT_SPAN;
  mmap_prot = PROT_READ | PROT_WRITE;
  mmap_flags = MAP_SHARED;
  mmap_fd = dev_mem_fd;
  mmap_offset = WRITE_ENABLE_BASE & ~(sysconf(_SC_PAGE_SIZE) -1);
  mmap_ptr = mmap(mmap_addr, mmap_length, mmap_prot, mmap_flags,
                  mmap_fd, mmap_offset);

  if(mmap_ptr == MAP_FAILED)
    error(1, errno, "mmap /dev/mem");

  write_en_ptr = (u_int*)((u_int)mmap_ptr + (WRITE_ENABLE_BASE & (sysconf(_SC_PAGE_SIZE)-1)));
  read_en_ptr = (u_int*)((u_int)mmap_ptr + (READ_ENABLE_BASE & (sysconf(_SC_PAGE_SIZE)-1)));
  reset_ptr = (u_int*)((u_int)mmap_ptr + (RESET_BASE & (sysconf(_SC_PAGE_SIZE)-1)));
  qin_ptr = (u_int*)((u_int)mmap_ptr + (QIN_BASE & (sysconf(_SC_PAGE_SIZE)-1)));
  qout_ptr = (u_int*)((u_int)mmap_ptr + (QOUT_BASE & (sysconf(_SC_PAGE_SIZE)-1)));

  close(dev_mem_fd);
}

//Initialize all of the signals / soft reset of circuit
void initSignals(void)
{
  reset_ptr[0] = 0; //Reset is active low
  qin_ptr[0] = 0;
  read_en_ptr[0] = 0;
  write_en_ptr[0] = 0;
  reset_ptr[0] = 1;
}

//Write the specified gate value to the specified column/row
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

/*
void demo_leds(void)
{
  int result;
  int dev_mem_fd;
  void *mmap_addr;
  size_t mmap_length;
  int mmap_prot;
  int mmap_flags;
  int mmap_fd;
  off_t mmap_offset;
  void *mmap_ptr;
  volatile u_int *led_pio_ptr;
  volatile u_int *qout_pio_ptr;
  //u_int32_t led_pio_value;
  //u_int32_t led = 0;

  //  Map the peripheral span through /dev/mem  
  dev_mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
  if(dev_mem_fd < 0)
    error(1, errno, "open /dev/mem");

  mmap_addr = NULL;
  //mmap_length = QIN_SPAN;
  mmap_length = QIN_SPAN + QOUT_SPAN;
  mmap_prot = PROT_READ | PROT_WRITE;
  mmap_flags = MAP_SHARED;
  mmap_fd = dev_mem_fd;
  mmap_offset = QIN_BASE & ~(sysconf(_SC_PAGE_SIZE) -1);
  mmap_ptr = mmap(mmap_addr, mmap_length, mmap_prot, mmap_flags,
                  mmap_fd, mmap_offset);

  if(mmap_ptr == MAP_FAILED)
    error(1, errno, "mmap /dev/mem");

  led_pio_ptr = (u_int*)((u_int)mmap_ptr + (QIN_BASE & (sysconf(_SC_PAGE_SIZE)-1)));
  qout_pio_ptr = (u_int*)((u_int)mmap_ptr + (QOUT_BASE & (sysconf(_SC_PAGE_SIZE)-1)));

  //Print qin value
  printf("Value of Qin: %d\n", led_pio_ptr[0]);
  printf("Value of Qout: %d\n", qout_pio_ptr[0]);

  //Change qin value
  led_pio_ptr[0] = 12345;

  //Print qin value
  printf("Value of Qin after change: %d\n", led_pio_ptr[0]);
  printf("Value of Qout after change: %d\n", qout_pio_ptr[0]);

  //  Read the LED PIO value
  led_pio_value = led_pio_ptr[0];
  
  //  Blink the LEDs for 10s
  printf("Blinking LEDs for 10 seconds\n");
 
  int sim_time = (1000000 / LED_DELAY_US) * 10;
  int i;
  for(i = 0; i < sim_time; i++){
    led_pio_ptr[0] = 1L << led;
    usleep(LED_DELAY_US);
    led = (led + 1) % PIO_0_DATA_WIDTH;
  }

  //  restore the LED PIO value
  led_pio_ptr[0] = led_pio_value;

  //  unmap /dev/mem mappings  
  result = munmap(mmap_ptr, mmap_length);
  if(result < 0)
    error(1, errno, "munmap /dev/mem");

  close(dev_mem_fd);
  printf("\n");
}
*/
