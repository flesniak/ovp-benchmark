imodelnewperipheral -name bench -vendor itiv.kit.edu -library peripheral -version 1.0 -constructor constructor -destructor destructor
iadddocumentation   -name Licensing   -text "Open Source Apache 2.0"
iadddocumentation   -name Description -text "OVP guest-to-host benchmark peripheral"

  imodeladdbusslaveport -name CFGBUS -size 4 -mustbeconnected
  imodeladdaddressblock -name AB0  -width 32 -offset 0 -size 4
  imodeladdmmregister   -name CONTROL   -offset 0  -width 32 -access rw -readfunction readReg -writefunction writeReg -userdata 0
    imodeladdfield        -name CALIBRATE     -bitoffset 0 -width 1 -access rw
      iadddocumentation -name Description -text "Calibrate by measuring time for interception"
    imodeladdfield        -name TEST_CALLBACK -bitoffset 1 -width 1 -access rw
      iadddocumentation -name Description -text "Test callback copying"
    imodeladdfield        -name TEST_RTCOPY   -bitoffset 2 -width 1 -access rw
      iadddocumentation -name Description -text "Test vmiRt copying"
    imodeladdfield        -name TEST_NATIVE   -bitoffset 3 -width 1 -access rw
      iadddocumentation -name Description -text "Test native memory"
    imodeladdfield        -name ISSUE_COPY    -bitoffset 31 -width 1 -access rw
      iadddocumentation -name Description -text "Issue a vmirt copy"

  imodeladdbusslaveport -name DATABUS -size 0x4000000 -mustbeconnected
    iadddocumentation -name Description -text "64MiB of test memory"
  imodeladdaddressblock -name AB1  -width 32 -offset 0 -size 0x4000000
  imodeladdlocalmemory  -name DATA -size 0x4000000

