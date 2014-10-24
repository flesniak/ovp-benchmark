#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <icm/icmCpuManager.h>

#include "../bench.h"

#define INSTRUCTION_COUNT_DEFAULT 100000

static struct optionsS {
  int verbosity;
  icmNewProcAttrs processorAttributes;
  int instructionCount;
  double wallclockFactor;
  char* program;
} options = { 1, ICM_ATTR_DEFAULT, INSTRUCTION_COUNT_DEFAULT, 0, 0 };

void usage( char* argv0 ) {
  fprintf( stderr, "Usage: %s [-v] [-t simple|count|regs] [-m memsize|[-d|-c]] [-i instructions] <program.elf>\n", argv0 );
  fprintf( stderr, "  -v                   | increase verbosity (1 -> OVP stats, 2 -> stack analysis debug\n" );
  fprintf( stderr, "  -t simple|count|regs | enable instruction tracing, with instruction cound or register dump\n" );
  fprintf( stderr, "  -i instructions      | simulate instructions at once (default 100000)\n" );
  fprintf( stderr, "  -w factor            | set wallclock factor \n" );
  fprintf( stderr, "  <program.elf>        | program file to simulate\n" );
  exit( 1 );
}

void parseOptions( int argc, char** argv ) {
  int c;
  opterr = 0;

  while( ( c = getopt( argc, argv, "vt:m:i:w:" ) ) != -1 )
    switch( c ) {
      case 'v':
        options.verbosity++;
        break;
      case 't':
        if( !strcmp( optarg, "simple" ) ) {
          options.processorAttributes |= ICM_ATTR_TRACE;
          break;
        }
        if( !strcmp( optarg, "count" ) ) {
          options.processorAttributes |= ICM_ATTR_TRACE_ICOUNT;
          break;
        }
        if( !strcmp( optarg, "regs" ) ) {
          options.processorAttributes |= ICM_ATTR_TRACE_REGS_BEFORE;
          break;
        }
        fprintf( stderr, "Invalid argument to option -t.\n" );
        usage( argv[0] );
        break;
      case 'i':
        sscanf( optarg, "%d", &options.instructionCount );
        if( !options.instructionCount ) {
          fprintf( stderr, "Invalid argument to option -i.\n" );
          usage( argv[0] );
        }
        break;
      case 'w':
        sscanf( optarg, "%lf", &options.wallclockFactor );
        if( !options.wallclockFactor ) {
          fprintf( stderr, "Invalid argument to option -w.\n" );
          usage( argv[0] );
        }
        break;
      case '?':
        if( optopt == 't' || optopt == 'm' || optopt == 'i' )
          fprintf( stderr, "Option -%c requires an argument.\n", optopt );
        else if( isprint( optopt ) )
          fprintf( stderr, "Unknown option `-%c'.\n", optopt);
        else
          fprintf( stderr, "Unknown option character `\\x%x'.\n", optopt );
      default:
        usage( argv[0] );
    }

   if( optind+1 != argc ) {
    fprintf( stderr, "Invalid number of arguments\n" );
    usage( argv[0] );
  }

  options.program = argv[optind];
}

int main( int argc, char** argv ) {
  parseOptions( argc, argv );

  if( options.verbosity > 1 ) {
    icmInit( ICM_VERBOSE | ICM_STOP_ON_CTRLC, 0, 0 );
  } else
    icmInit( ICM_INIT_DEFAULT | ICM_STOP_ON_CTRLC, 0, 0 );

  const char* model = icmGetVlnvString( 0, "xilinx.ovpworld.org", "processor", "microblaze", "1.0", "model" );
  const char* semihosting = icmGetVlnvString( 0, "xilinx.ovpworld.org", "semihosting", "microblazeNewlib", "1.0", "model" );

  icmAttrListP userAttrs = icmNewAttrList();
  icmAddDoubleAttr(userAttrs, "mips", 800.0);
  //icmAddUns32Attr(userAttrs, "C_ENDIANNESS", 0); //microblaze is big-endian by default
  //icmAddStringAttr(userAttrs, "endian", "little"); //or1k toolchain does not seem to support little endian

  icmProcessorP processor1 = icmNewProcessor(
      "cpu1",             // CPU name
      "microblaze",       // CPU type
      0,                  // CPU cpuId
      0,                  // CPU model flags
      32,                 // address bits
      model,              // model file
      "",                 // morpher attributes
      options.processorAttributes, // enable tracing or register values
      userAttrs,          // user-defined attributes
      semihosting,        // semi-hosting file
      ""                  // semi-hosting attributes
  );

  //bus
  icmBusP bus1 = icmNewBus("bus1", 32);
  icmConnectProcessorBusses(processor1, bus1, bus1);

  icmMemoryP mem[2] = {};
  mem[0] = icmNewMemory("mem1", ICM_PRIV_RWX, BENCH_CONTROL_DEFAULT_ADDRESS-1);
  mem[1] = icmNewMemory("mem2", ICM_PRIV_RWX, UINT_MAX-BENCH_CONTROL_DEFAULT_ADDRESS-BENCH_CONTROL_REGS_SIZE-1);
  icmConnectMemoryToBus(bus1, "port1", mem[0], 0);
  icmConnectMemoryToBus(bus1, "port2", mem[1], BENCH_CONTROL_DEFAULT_ADDRESS+BENCH_CONTROL_REGS_SIZE);

  icmAttrListP pseAttrs = icmNewAttrList();
  //icmAddBoolAttr(pseAttrs, "bigEndianGuest", 1);
  //icmAddStringAttr(pseAttrs, "output", options.display);
  //icmAddUns32Attr(pseAttrs, "polledRedraw", DVI_REDRAW_PSE);

  icmPseP bench = icmNewPSE( "bench", "../pse/pse.pse", pseAttrs, 0, 0 );
  icmConnectPSEBus( bench, bus1, BENCH_CONTROL_BUS_NAME, 0, BENCH_CONTROL_DEFAULT_ADDRESS, BENCH_CONTROL_DEFAULT_ADDRESS+BENCH_CONTROL_REGS_SIZE-1 );
  icmConnectPSEBusDynamic( bench, bus1, BENCH_DATA_BUS_NAME, 0 );

  //load semihost library
  icmAddPseInterceptObject( bench, "bench", "../model/model.so", 0, 0);

  if( options.wallclockFactor )
    icmSetWallClockFactor(options.wallclockFactor);

  icmLoadProcessorMemory(processor1, options.program, ICM_LOAD_DEFAULT, False, True);

  //do by-instruction simulation if necessary
  if( options.instructionCount != INSTRUCTION_COUNT_DEFAULT ) {
    icmStopReason stopReason = 0;
    while( ( stopReason = icmSimulate( processor1, options.instructionCount ) ) != ICM_SR_EXIT ) {
      switch( stopReason ) {
        case ICM_SR_SCHED : {
          //instructionCount instructions done, nothing special to do
          fprintf( stdout, "Simulation scheduling interrupt\n" );
          //TODO schedule PSE
          break;
        }
        case ICM_SR_WATCHPOINT : {
          icmWatchPointP triggeredWatchpoint = 0;
          while( ( triggeredWatchpoint = icmGetNextTriggeredWatchPoint() ) != 0 ) {
            fprintf( stderr, "Unhandled icmWatchPoint, ignoring!\n" );
          }
          break;
        }
        case ICM_SR_FINISH :
          fprintf( stdout, "Simulation finished\n" );
          stopReason = ICM_SR_EXIT; //to exit loop
          break;
        case ICM_SR_EXIT :
          fprintf( stdout, "Simulation exited\n" );
          break;
        case ICM_SR_INTERRUPT : {
          fprintf( stdout, "Simulation interrupted\n" );
          stopReason = ICM_SR_EXIT; //to exit loop
          break;
        }
        default :
          fprintf( stderr, "Unhandled icmStopReason!\n" );
      }
    }
  }
  else
    icmSimulatePlatform();

  icmTerminate();
  return 0;
}
