
////////////////////////////////////////////////////////////////////////////////
//
//                W R I T T E N   B Y   I M P E R A S   I G E N
//
//                             Version 20131018.0
//                          Mon Oct 13 14:00:51 2014
//
////////////////////////////////////////////////////////////////////////////////

#ifndef BENCH_MACROS_IGEN_H
#define BENCH_MACROS_IGEN_H
/////////////////////////////////// Licensing //////////////////////////////////

// Open Source Apache 2.0

////////////////////////////////// Description /////////////////////////////////

// OVP guest-to-host benchmark peripheral

// Before including this file in the application, define the indicated macros
// to fix the base address of each slave port.
// Set the macro 'CFGBUS' to the base of port 'CFGBUS'
#ifndef CFGBUS
    #error CFGBUS is undefined.It needs to be set to the port base address
#endif
#define CFGBUS_AB0_CONTROL    (CFGBUS + 0x0)

#define CFGBUS_AB0_CONTROL_CALIBRATE   0x1
#define CFGBUS_AB0_CONTROL_TEST_CALLBACK   (0x1 << 1)
#define CFGBUS_AB0_CONTROL_TEST_RTCOPY   (0x1 << 2)
#define CFGBUS_AB0_CONTROL_TEST_NATIVE   (0x1 << 3)


#endif
