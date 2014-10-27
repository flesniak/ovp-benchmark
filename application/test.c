#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "../bench.h"

#define TIMES(x) for(int count=0; count<x; count++)

void writeData(volatile unsigned int* data, int value) {
  for( int i = 0; i<BENCH_DATA_SIZE; i++ )
    *(data++) = value;
}

int main() {
  volatile unsigned int* ctrl = (unsigned int*)BENCH_CONTROL_DEFAULT_ADDRESS;
  volatile unsigned int* data = (unsigned int*)BENCH_DATA_DEFAULT_ADDRESS;

  printf("Initializing memory...\n");
  //writeData(data, 0xDEADBEEF); //write out data once to allocate icm memory

  TIMES(100000) asm("nop");

  printf("Benchmark calibrating...\n");
  *ctrl = BENCH_CALIBRATE_MASK;
  TIMES(10000) *ctrl = 0;
  *ctrl = BENCH_CALIBRATE_MASK;

  TIMES(100000) asm("nop");

  printf("Callback test\n"); //to callback test last because callbacks can't be closed
  *ctrl = BENCH_TEST_CALLBACK_MASK;
  writeData(data, 0xB00B1E5);
  *ctrl = BENCH_TEST_CALLBACK_MASK;

  TIMES(100000) asm("nop");

  printf("Runtime copy test\n");
  *ctrl = BENCH_TEST_RTCOPY_MASK;
  TIMES(10) { putchar('c'); fflush(stdout); writeData(data, count); *ctrl = BENCH_ISSUE_COPY_MASK; };
  putchar('\n'); fflush(stdout);
  *ctrl = BENCH_TEST_RTCOPY_MASK;

  TIMES(100000) asm("nop");

  printf("Native mapping test\n");
  *ctrl = BENCH_TEST_NATIVE_MASK;
  TIMES(10) { putchar('c'); fflush(stdout); writeData(data, count); }
  putchar('\n'); fflush(stdout);
  *ctrl = BENCH_TEST_NATIVE_MASK;

  printf("Done. Looping.\n");
  while( 1 );

  return 0;
}

