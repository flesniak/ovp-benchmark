#include <peripheral/bhm.h>

#include "byteswap.h"
#include "bench.igen.h"
#include "../bench.h"

unsigned char buffer[BENCH_DATA_SIZE];

void calibrate(int enable) {
  bhmMessage("F", "BENCH_PSE", "Failed to intercept : calibrate");
}

void dummyCallback() {
  bhmMessage("F", "BENCH_PSE", "Failed to intercept : dummyCallback");
}

void testCallback(int enable, unsigned int bufAddr) {
  bhmMessage("F", "BENCH_PSE", "Failed to intercept : testCallback");
}

void testRtcopy(int enable) {
  bhmMessage("F", "BENCH_PSE", "Failed to intercept : testRtcopy");
}

void testNative(int enable) {
  bhmMessage("F", "BENCH_PSE", "Failed to intercept : testNative");
}

void copyCallback() {
  bhmMessage("F", "BENCH_PSE", "Failed to intercept : copyCallback");
}

void writeData(void* addr, Uns32 data) {
  bhmMessage("F", "BENCH_PSE", "Failed to intercept : writeData");
}

PPM_WRITE_CB(changeCallback) {
  //bhmMessage("I", "BENCH_PSE", "WRITE_CB at 0x%08x data 0x%08x buffer 0x%08x", (Uns32)addr, data, (unsigned int)buffer);
  writeData(addr, data);
}

PPM_REG_READ_CB(readReg) {
  bhmMessage("W", "BENCH_PSE", "Unsupported read on CONTROL for %d bytes at 0x%08x", bytes, (Uns32)addr);
  return 0;
}

PPM_REG_WRITE_CB(writeReg) {
  Uns32 reg = (Uns32)user;
  //bhmMessage("I", "BENCH_PSE", "Write to CONTROL at 0x%08x data 0x%08x", (Uns32)addr, data);
  if( reg )
    bhmMessage("W", "BENCH_PSE", "Unhandled write to CONTROL at 0x%08x data 0x%08x", (Uns32)addr, data);
  else
    switch( bswap_32(data) ) {
      case 0:
        dummyCallback();
        break;
      case BENCH_CALIBRATE_MASK:
        if( !CFGBUS_AB0_data.CONTROL.bits.CALIBRATE ) { //start test
          CFGBUS_AB0_data.CONTROL.bits.CALIBRATE = 1; //memorize current test
          calibrate(1);
        } else { //end test
          calibrate(0);
          CFGBUS_AB0_data.CONTROL.bits.CALIBRATE = 0;
        }
        break;
      case BENCH_TEST_CALLBACK_MASK: bhmMessage("I", "BENCH_PSE", "Callback test");
        if( !CFGBUS_AB0_data.CONTROL.bits.TEST_CALLBACK ) { //start test
          CFGBUS_AB0_data.CONTROL.bits.TEST_CALLBACK = 1; //memorize current test
          ppmCreateDynamicSlavePort(BENCH_DATA_BUS_NAME, BENCH_DATA_DEFAULT_ADDRESS, BENCH_DATA_SIZE, buffer);
          ppmInstallChangeCallback(changeCallback, (void*)0, buffer, BENCH_DATA_SIZE);
          testCallback(1, (unsigned int)buffer);
        } else { //end test
          ppmDeleteDynamicSlavePort(BENCH_DATA_BUS_NAME, BENCH_DATA_DEFAULT_ADDRESS, BENCH_DATA_SIZE);
          testCallback(0, 0);
          CFGBUS_AB0_data.CONTROL.bits.TEST_CALLBACK = 0;
        }
        break;
      case BENCH_TEST_RTCOPY_MASK:
        if( !CFGBUS_AB0_data.CONTROL.bits.TEST_RTCOPY ) { //start test
          CFGBUS_AB0_data.CONTROL.bits.TEST_RTCOPY = 1; //memorize current test
          testRtcopy(1);
        } else { //end test
          testRtcopy(0);
          CFGBUS_AB0_data.CONTROL.bits.TEST_RTCOPY = 0;
        }
        break;
      case BENCH_TEST_NATIVE_MASK:
        if( !CFGBUS_AB0_data.CONTROL.bits.TEST_NATIVE ) { //start test
          CFGBUS_AB0_data.CONTROL.bits.TEST_NATIVE = 1; //memorize current test
          testNative(1);
        } else { //end test
          testNative(0);
          CFGBUS_AB0_data.CONTROL.bits.TEST_NATIVE = 0;
        }
        break;
      case BENCH_ISSUE_COPY_MASK:
        copyCallback();
        break;
      default:
        bhmMessage("W", "BENCH_PSE", "Unhandled write to CONTROL at 0x%08x data 0x%08x", (Uns32)addr, data);
    }
}

PPM_CONSTRUCTOR_CB(constructor) {
  bhmMessage("I", "BENCH_PSE", "Constructing");
  periphConstructor();
}

PPM_DESTRUCTOR_CB(destructor) {
  bhmMessage("I", "BENCH_PSE", "Destructing");
}

