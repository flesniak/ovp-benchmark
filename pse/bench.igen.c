
////////////////////////////////////////////////////////////////////////////////
//
//                W R I T T E N   B Y   I M P E R A S   I G E N
//
//                             Version 20131018.0
//                          Sat Oct 25 01:27:59 2014
//
////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////// Licensing //////////////////////////////////

// Open Source Apache 2.0

////////////////////////////////// Description /////////////////////////////////

// OVP guest-to-host benchmark peripheral


#include "bench.igen.h"
/////////////////////////////// Port Declarations //////////////////////////////

CFGBUS_AB0_dataT CFGBUS_AB0_data;

handlesT handles;

/////////////////////////////// Diagnostic level ///////////////////////////////

// Test this variable to determine what diagnostics to output.
// eg. if (diagnosticLevel > 0) bhmMessage("I", "bench", "Example");

Uns32 diagnosticLevel;

/////////////////////////// Diagnostic level callback //////////////////////////

static void setDiagLevel(Uns32 new) {
    diagnosticLevel = new;
}

///////////////////////////// MMR Generic callbacks ////////////////////////////

//////////////////////////////// View callbacks ////////////////////////////////

static PPM_VIEW_CB(view_CFGBUS_AB0_CONTROL) {
    *(Uns32*)data = CFGBUS_AB0_data.CONTROL.value;
}
//////////////////////////////// Bus Slave Ports ///////////////////////////////

static void installSlavePorts(void) {
    handles.CFGBUS = ppmCreateSlaveBusPort("CFGBUS", 4);
    if (!handles.CFGBUS) {
        bhmMessage("E", "PPM_SPNC", "Could not connect port 'CFGBUS'");
    }

}

//////////////////////////// Memory mapped registers ///////////////////////////

static void installRegisters(void) {

    ppmCreateRegister("CONTROL",
        0,
        handles.CFGBUS,
        0,
        4,
        readReg,
        writeReg,
        view_CFGBUS_AB0_CONTROL,
        (void*)0x0,
        True
    );


}

////////////////////////////////// Constructor /////////////////////////////////

PPM_CONSTRUCTOR_CB(periphConstructor) {
    installSlavePorts();
    installRegisters();
}

///////////////////////////////////// Main /////////////////////////////////////

int main(int argc, char *argv[]) {
    diagnosticLevel = 0;
    bhmInstallDiagCB(setDiagLevel);
    constructor();

    bhmWaitEvent(bhmGetSystemEvent(BHM_SE_END_OF_SIMULATION));
    destructor();
    return 0;
}

