#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <vmi/vmiMessage.h>
#include <vmi/vmiOSAttrs.h>
#include <vmi/vmiOSLib.h>
#include <vmi/vmiPSE.h>
#include <vmi/vmiTypes.h>

#include "../bench.h"

typedef struct vmiosObjectS {
  //register descriptions for get/return arguments
  vmiRegInfoCP resultLow;
  vmiRegInfoCP resultHigh;
  vmiRegInfoCP stackPointer;

  struct timeval start;
  struct timeval end;
  Uns32 calibration;
  Uns32 calibCount;

  memDomainP guestDomain;
  unsigned char* buffer;
  Uns32 bytesWritten;
  Uns32 bufAddr;
} vmiosObject;

static void getArg(vmiProcessorP processor, vmiosObjectP object, Uns32 *index, void* result, Uns32 argSize) {
  memDomainP domain = vmirtGetProcessorDataDomain(processor);
  Uns32 spAddr;
  vmirtRegRead(processor, object->stackPointer, &spAddr);
  vmirtReadNByteDomain(domain, spAddr+*index+4, result, argSize, 0, MEM_AA_FALSE);
  *index += argSize;
}

#define GET_ARG(_PROCESSOR, _OBJECT, _INDEX, _ARG) getArg(_PROCESSOR, _OBJECT, &_INDEX, &_ARG, sizeof(_ARG))

inline static void retArg(vmiProcessorP processor, vmiosObjectP object, Uns64 result) {
  vmirtRegWrite(processor, object->resultLow, &result);
  result >>= 32;
  vmirtRegWrite(processor, object->resultHigh, &result);
}

Uns32 timevalDiff(struct timeval* start, struct timeval* end) {
  Uns32 res = 0;
  res  = (Uns32)end->tv_sec  *1000000 + end->tv_usec;
  res -= (Uns32)start->tv_sec*1000000 + start->tv_usec;
  //vmiMessage("I", "BENCH_SH", "diff %ld %ld to %ld %ld is %d", start->tv_sec, start->tv_usec, end->tv_sec, end->tv_usec, res);
  return res;
}

memDomainP getSimulatedVmemDomain(vmiProcessorP processor, char* name) {
  Addr lo, hi;
  Bool isMaster, isDynamic;
  memDomainP simDomain = vmipsePlatformPortAttributes(processor, BENCH_DATA_BUS_NAME, &lo, &hi, &isMaster, &isDynamic);
  if( !simDomain )
    vmiMessage("F", "BENCH_SH", "Failed to obtain %s platform port", BENCH_DATA_BUS_NAME);
  vmiMessage("I", "BENCH_SH", "port lo 0x%08llx hi 0x%08llx isMaster %d isDynamic %d", lo, hi, isMaster, isDynamic);
  return simDomain;
}

void mapMemory(vmiosObjectP object, bool mapOrUnmap) {
  if( mapOrUnmap ) {
    vmiMessage("I", "BENCH_SH", "Mapping external vmem at addr 0x%08x", BENCH_DATA_DEFAULT_ADDRESS);
    vmipseAliasMemory(object->guestDomain, BENCH_DATA_BUS_NAME, BENCH_DATA_DEFAULT_ADDRESS, BENCH_DATA_DEFAULT_ADDRESS+BENCH_DATA_SIZE-1);
  } else {
    vmiMessage("I", "BENCH_SH", "Unaliasing previously mapped memory at 0x%08x", BENCH_DATA_DEFAULT_ADDRESS);
    vmirtUnaliasMemory(object->guestDomain, BENCH_DATA_DEFAULT_ADDRESS, BENCH_DATA_DEFAULT_ADDRESS+BENCH_DATA_SIZE-1);
    vmirtMapMemory(object->guestDomain, BENCH_DATA_DEFAULT_ADDRESS, BENCH_DATA_DEFAULT_ADDRESS+BENCH_DATA_SIZE-1, MEM_RAM);
  }
}

static VMIOS_INTERCEPT_FN(calibrate) {
  Uns32 enable = 0, index = 0;
  GET_ARG(processor, object, index, enable);

  if( enable ) { //start test
    gettimeofday(&object->start, 0);
  } else { //end test
    gettimeofday(&object->end, 0);
    if( object->calibCount < 100 )
      vmiMessage("W", "BENCH_SH", "Calibration end with less than 100 dummy calls (%d). Re-run with more calls!", object->calibCount);
    object->calibration = timevalDiff(&object->start, &object->end);
    Uns32 msecs = ceil((float)object->calibration / object->calibCount);
    vmiMessage("I", "BENCH_SH", "Bench calibrated to %d microseconds (%d calls took %d us)", msecs, object->calibCount, object->calibration);
    object->calibration = msecs;
  }
}

static VMIOS_INTERCEPT_FN(dummyCallback) {
  //vmiMessage("I", "BENCH_SH", "Dummy Callback!");
  object->calibCount++;
  return;
}

static VMIOS_INTERCEPT_FN(copyCallback) {
  //vmiMessage("I", "BENCH_SH", "Copy Callback!");
  vmirtReadNByteDomain(object->guestDomain, BENCH_DATA_DEFAULT_ADDRESS, object->buffer, BENCH_DATA_SIZE, 0, MEM_AA_FALSE);
  object->bytesWritten += BENCH_DATA_SIZE;
}

static VMIOS_INTERCEPT_FN(testCallback) {
  Uns32 enable = 0, index = 0;
  GET_ARG(processor, object, index, enable);
  GET_ARG(processor, object, index, object->bufAddr);
  if( enable ) { //start test
    object->bytesWritten = 0;
    gettimeofday(&object->start, 0);
  } else { //end test
    gettimeofday(&object->end, 0);
    Uns32 diff = timevalDiff(&object->start, &object->end);
    vmiMessage("I", "BENCH_SH", "Wrote %d bytes in %f seconds, equals %f MiB/s", object->bytesWritten, (double)(diff-object->calibration)/1000000, (double)1.04858*object->bytesWritten/(diff-object->calibration));
    mapMemory(object, 0);
  }
}

static VMIOS_INTERCEPT_FN(testRtcopy) {
  Uns32 enable = 0, index = 0;
  GET_ARG(processor, object, index, enable);
  if( enable ) { //start test
    object->bytesWritten = 0;
    gettimeofday(&object->start, 0);
  } else { //end test
    gettimeofday(&object->end, 0);
    Uns32 diff = timevalDiff(&object->start, &object->end);
    vmiMessage("I", "BENCH_SH", "Wrote %d bytes in %f seconds, equals %f MiB/s", object->bytesWritten, (double)(diff-object->calibration)/1000000, (double)1.04858*object->bytesWritten/(diff-object->calibration));
  }
}

static VMIOS_INTERCEPT_FN(testNative) {
  Uns32 enable = 0, index = 0;
  GET_ARG(processor, object, index, enable);
  if( enable ) { //start test
    object->bytesWritten = 0;
    mapMemory(object, 1);
    gettimeofday(&object->start, 0);
  } else { //end test
    gettimeofday(&object->end, 0);
    mapMemory(object, 0);
    object->bytesWritten = 10*BENCH_DATA_SIZE;
    Uns32 diff = timevalDiff(&object->start, &object->end);
    vmiMessage("I", "BENCH_SH", "Wrote %d bytes in %f seconds, equals %f MiB/s", object->bytesWritten, (double)(diff-object->calibration)/1000000, (double)1.04858*object->bytesWritten/(diff-object->calibration));
  }
}

static VMIOS_INTERCEPT_FN(writeData) {
  Uns32 addr = 0, data = 0, index = 0;
  GET_ARG(processor, object, index, addr);
  GET_ARG(processor, object, index, data);
  index = addr-object->bufAddr; //just re-use index variable
  /*if( index >= 16750000 )
    vmiMessage("I", "BENCH_SH", "writeData addr 0x%08x bufAddr 0x%08x data 0x%08x index 0x%08x", addr, object->bufAddr, data, index);*/
  if( index >= BENCH_DATA_SIZE )
    vmiMessage("F", "BENCH_SH", "writeData out of bounds index 0x%08x", index);
  *((unsigned int*)(object->buffer+index)) = data;
  object->bytesWritten += 4;
}

static VMIOS_CONSTRUCTOR_FN(constructor) {
  vmiMessage("I" ,"BENCH_SH", "Constructing");

  const char *procType = vmirtProcessorType(processor);
  memEndian endian = vmirtGetProcessorDataEndian(processor);

  if( strcmp(procType, "pse") )
      vmiMessage("F", "BENCH_SH", "Processor must be a PSE (is %s)\n", procType);
  if( endian != MEM_ENDIAN_NATIVE )
      vmiMessage("F", "BENCH_SH", "Host processor must same endianity as a PSE\n");

  //get register description and store to object for further use
  object->resultLow = vmirtGetRegByName(processor, "eax");
  object->resultHigh = vmirtGetRegByName(processor, "edx");
  object->stackPointer = vmirtGetRegByName(processor, "esp");

  object->guestDomain = getSimulatedVmemDomain(processor, BENCH_DATA_BUS_NAME);
  //vmirtMapMemory(object->guestDomain, BENCH_DATA_DEFAULT_ADDRESS, BENCH_DATA_DEFAULT_ADDRESS+BENCH_DATA_SIZE-1, MEM_RAM);

  object->calibration = 0;
  object->calibCount = 0,
  object->buffer = calloc(1, BENCH_DATA_SIZE);
}

static VMIOS_CONSTRUCTOR_FN(destructor) {
  vmiMessage("I" ,"BENCH_SH", "Shutting down");
  if( object->buffer )
    free(object->buffer);
  vmiMessage("I", "BENCH_SH", "Shutdown complete");
}

vmiosAttr modelAttrs = {
    .versionString  = VMI_VERSION,                // version string (THIS MUST BE FIRST)
    .modelType      = VMI_INTERCEPT_LIBRARY,      // type
    .packageName    = "videoin",                  // description
    .objectSize     = sizeof(vmiosObject),        // size in bytes of object

    .constructorCB  = constructor,                // object constructor
    .destructorCB   = destructor,                 // object destructor

    .morphCB        = 0,                          // morph callback
    .nextPCCB       = 0,                          // get next instruction address
    .disCB          = 0,                          // disassemble instruction

    // -------------------          -------- ------  -----------------
    // Name                         Address  Opaque  Callback
    // -------------------          -------- ------  -----------------
    .intercepts = {
        {"calibrate",          0,       True,   calibrate         },
        {"dummyCallback",      0,       True,   dummyCallback     },
        {"testCallback",       0,       True,   testCallback      },
        {"testRtcopy",         0,       True,   testRtcopy        },
        {"testNative",         0,       True,   testNative        },
        {"writeData",          0,       True,   writeData         },
        {"copyCallback",       0,       True,   copyCallback      },
        {0}
    }
};
