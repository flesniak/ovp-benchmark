
////////////////////////////////////////////////////////////////////////////////
//
//                W R I T T E N   B Y   I M P E R A S   I G E N
//
//                             Version 20131018.0
//                          Sat Oct 25 01:27:59 2014
//
////////////////////////////////////////////////////////////////////////////////

#ifndef BENCH_IGEN_H
#define BENCH_IGEN_H

#ifdef _PSE_
#    include "peripheral/impTypes.h"
#    include "peripheral/bhm.h"
#    include "peripheral/ppm.h"
#else
#    include "hostapi/impTypes.h"
#endif

//////////////////////////////////// Externs ///////////////////////////////////

    extern Uns32 diagnosticLevel;

/////////////////////////// Register data declaration //////////////////////////

typedef struct CFGBUS_AB0_dataS { 
    union { 
        Uns32 value;
        struct {
            unsigned CALIBRATE : 1;
            unsigned TEST_CALLBACK : 1;
            unsigned TEST_RTCOPY : 1;
            unsigned TEST_NATIVE : 1;
            unsigned __pad4 : 27;
            unsigned ISSUE_COPY : 1;
        } bits;
    } CONTROL;
} CFGBUS_AB0_dataT, *CFGBUS_AB0_dataTP;

/////////////////////////////// Port Declarations //////////////////////////////

extern CFGBUS_AB0_dataT CFGBUS_AB0_data;

#ifdef _PSE_
///////////////////////////////// Port handles /////////////////////////////////

typedef struct handlesS {
    void                 *CFGBUS;
} handlesT, *handlesTP;

extern handlesT handles;

////////////////////////////// Callback prototypes /////////////////////////////

PPM_REG_READ_CB(readReg);
PPM_REG_WRITE_CB(writeReg);
PPM_CONSTRUCTOR_CB(periphConstructor);
PPM_DESTRUCTOR_CB(periphDestructor);
PPM_CONSTRUCTOR_CB(constructor);
PPM_DESTRUCTOR_CB(destructor);

#endif

#endif
