/*
 *  Authors:
 *    Denys Duchier <duchier@ps.uni-sb.de>
 *
 *  Contributors:
 *    Christian Schulte <schulte@ps.uni-sb.de>
 *
 *  Copyright:
 *    Denys Duchier, 1997
 *    Christian Schulte, 1997
 *
 *  Last change:
 *    $Date$ by $Author$
 *    $Revision$
 *
 *  This file is part of Mozart, an implementation
 *  of Oz 3:
 *     http://www.mozart-oz.org
 *
 *  See the file "LICENSE" or
 *     http://www.mozart-oz.org/LICENSE.html
 *  for information on usage and redistribution
 *  of this file, and for a DISCLAIMER OF ALL
 *  WARRANTIES.
 *
 */




// EMULATOR PROPERTIES
//
// We define here a single interface to all emulator properties.  This
// is intended to put an end to the proliferation of specialized builtins.
// It will also permit an optimized representation of emulator properties
// as Virtual Properties (see later): as ints rather than as instances of
// class VirtualProperty.
//

#include "vprops.hh"
#include "dictionary.hh"
#include "fdomn.hh"
#include "am.hh"
#include "os.hh"
#include "codearea.hh"
#include "OZCONF.h"
#include "builtins.hh"

#include <stdarg.h>

extern char *AMVersion, *AMDate;

// "ozplatform" (defined in version.cc) has the form <osname-cpu>,
extern char *ozplatform;

enum EmulatorPropertyIndex {
  // THREADS
  PROP_THREADS_CREATED,
  PROP_THREADS_RUNNABLE,
  PROP_THREADS_MIN,
  PROP_THREADS_MAX,
  PROP_THREADS,
  // PRIORITIES
  PROP_PRIORITIES_HIGH,
  PROP_PRIORITIES_MEDIUM,
  PROP_PRIORITIES,
  // TIME
  PROP_TIME_COPY,
  PROP_TIME_GC,
  PROP_TIME_PROPAGATE,
  PROP_TIME_RUN,
  PROP_TIME_SYSTEM,
  PROP_TIME_TOTAL,
  PROP_TIME_USER,
  PROP_TIME_IDLE,
  PROP_TIME_DETAILED,
  PROP_TIME,
  // GC
  PROP_GC_MIN,
  PROP_GC_MAX,
  PROP_GC_FREE,
  PROP_GC_TOLERANCE,
  PROP_GC_ON,
  PROP_GC_THRESHOLD,
  PROP_GC_SIZE,
  PROP_GC_ACTIVE,
  PROP_GC_CODE_CYCLES,
  PROP_GC,
  // PRINT
  PROP_PRINT_DEPTH,
  PROP_PRINT_WIDTH,
  PROP_PRINT,
  // FD
  PROP_FD_VARIABLES,
  PROP_FD_PROPAGATORS,
  PROP_FD_INVOKED,
  PROP_FD_THRESHOLD,
  PROP_FD,
  // SPACES
  PROP_SPACES_COMMITTED,
  PROP_SPACES_CLONED,
  PROP_SPACES_CREATED,
  PROP_SPACES_FAILED,
  PROP_SPACES_SUCCEEDED,
  PROP_SPACES,
  // ERRORS
  PROP_ERRORS_HANDLER,
  PROP_ERRORS_DEBUG,
  PROP_ERRORS_THREAD,
  PROP_ERRORS_DEPTH,
  PROP_ERRORS_WIDTH,
  PROP_ERRORS,
  // MESSAGES
  PROP_MESSAGES_GC,
  PROP_MESSAGES_IDLE,
  PROP_MESSAGES,
  // MEMORY
  PROP_MEMORY_ATOMS,
  PROP_MEMORY_NAMES,
  PROP_MEMORY_FREELIST,
  PROP_MEMORY_CODE,
  PROP_MEMORY_HEAP,
  PROP_MEMORY,
  // LIMITS
  PROP_LIMITS_INT_MIN,
  PROP_LIMITS_INT_MAX,
  PROP_LIMITS_BYTECODE_XREGISTERS,
  PROP_LIMITS,
  // APPLICATION
  PROP_APPLICATION_ARGS,
  PROP_APPLICATION_URL,
  PROP_APPLICATION_GUI,
  PROP_APPLICATION,
  // PLATFORM
  PROP_PLATFORM_NAME,
  PROP_PLATFORM_OS,
  PROP_PLATFORM_ARCH,
  PROP_PLATFORM,
  // MISC
  PROP_STANDALONE,
  PROP_OZ_CONFIGURE_HOME,
  PROP_OZ_EMULATOR_HOME,
  PROP_OZ_VERSION,
  PROP_OZ_DATE,
  // DISTRIBUTION
  PROP_DISTRIBUTION_VIRTUALSITES,
  // INTERNAL
  PROP_INTERNAL_DEBUG,
  PROP_INTERNAL_PROPLOCATION,
  PROP_INTERNAL_SUSPENSION,
  PROP_INTERNAL_STOP,
  PROP_INTERNAL_DEBUG_IP,
  PROP_INTERNAL,
  // PERDIO
  PROP_PERDIO_SEIFHANDLER,
  PROP_PERDIO_FLOWBUFFERSIZE,
  PROP_PERDIO_FLOWBUFFERTIME,
  PROP_PERDIO_DEBUG,
  PROP_PERDIO_MINIMAL,
  PROP_PERDIO_VERSION,
  PROP_PERDIO_USEALTVARPROTOCOL,
  PROP_PERDIO_TIMEOUT,
  PROP_PERDIO_TEMPRETRYCEILING,
  PROP_PERDIO_TEMPRETRYFLOOR,
  PROP_PERDIO_TEMPRETRYFACTOR,
  PROP_PERDIO_MAXTCPCACHE,
  PROP_PERDIO_CHECKALIVEINTERVAL,
  PROP_PERDIO,
  // DPTABLE
  PROP_DPTABLE_DEFAULTOWNERTABLESIZE,
  PROP_DPTABLE_DEFAULTBORROWTABLESIZE,
  PROP_DPTABLE_LOWLIMIT,
  PROP_DPTABLE_EXPANDFACTOR,
  PROP_DPTABLE_BUFFER,
  PROP_DPTABLE_WORTHWHILEREALLOC,
  PROP_DPTABLE,

  PROP_CLOSE_TIME,

  PROP_OZ_STYLE_USE_FUTURES,
  // this must remain last
  PROP__LAST
};


static OZ_Term getApplicationArgs(void) {
  TaggedRef out = oz_nil();
  for(int i=ozconf.argC-1; i>=0; i--)
    out = oz_cons(oz_atomNoDup(ozconf.argV[i]),out);
  return out;
}

// Handle the case of indexed property P whose value can be
// found at location L.  Return the corresponding Oz term.

#define CASE_INT( P,L) case P: return OZ_int( L)
#define CASE_BOOL(P,L) case P: return oz_bool(L)
#define CASE_ATOM(P,L) case P: return oz_atomNoDup(L)

// Construct an Arity given `n' atoms.  First argument is n
// i.e. the number of features, the following arguments are
// the n atoms.  Creating an arity is an expensive operation,
// but we are going to cache the required arity in local
// static variables.  Each arity is computed only the 1st time
// it is needed.

static
OZ_Arity mkArity(int n,...)
{
  va_list(ap);
  va_start(ap,n);
  OZ_Term list = oz_nil();
  for (int i=0;i<n;i++) list = oz_cons(va_arg(ap,OZ_Term),list);
  va_end(ap);
  return OZ_makeArity(list);
}

// DEFINE_REC(L,A) puts in REC__ a record with label L and
// arity A.  Both the record and arity are cached in local
// static variables and are computed only the 1st time they
// are needed.  L is specified as a C string, and A is of
// the form (n,F1,...,Fn) where F1 to Fn are Atoms which we
// assume are already initialized (they are expected to have
// been created in value.cc)

#define DEFINE_REC(L,A)                         \
static OZ_Term  LAB__ = 0;                      \
static OZ_Arity ARY__;                          \
if (LAB__==0) {                                 \
  LAB__ = oz_atomNoDup(L);                      \
  ARY__ = mkArity A ;                           \
}                                               \
REC__ = SRecord::newSRecord(LAB__,(Arity*)ARY__);

// tag and return the result record REC__
#define RETURN_REC return makeTaggedSRecord(REC__);

// set feature F of the result record REC__
#define SET_REC(F,V) REC__->setFeature(F,V)

// create a result record REC__, DO something, an return
// the tagged result

#define DO_REC(L,A,DO) { DEFINE_REC(L,A); DO; RETURN_REC; }

// Handle the case of indexed property P, whose result is
// a record with label L and arity A, and DO something to
// initialize its features before returning it.

#define CASE_REC(P,L,A,DO) case P: DO_REC(L,A,DO);

// set feature F or REC__ to appropriate term

#define SET_INT( F,I) SET_REC(F,OZ_int( I))
#define SET_BOOL(F,B) SET_REC(F,oz_bool(B))
#define SET_ATOM(F,A) SET_REC(F,oz_atom(A))

OZ_Term GetEmulatorProperty(EmulatorPropertyIndex prop) {
  SRecord * REC__;
  switch (prop) {
    // THREADS
    CASE_INT(PROP_THREADS_CREATED,ozstat.createdThreads.total);
    CASE_INT(PROP_THREADS_RUNNABLE,am.threadsPool.getRunnableNumber());
    CASE_INT(PROP_THREADS_MIN,ozconf.stackMinSize / TASKFRAMESIZE);
    CASE_INT(PROP_THREADS_MAX,ozconf.stackMaxSize / TASKFRAMESIZE);
    CASE_REC(PROP_THREADS,"threads",
             (4,AtomCreated,AtomRunnable,AtomMin,AtomMax),
             SET_INT(AtomCreated ,ozstat.createdThreads.total);
             SET_INT(AtomRunnable,am.threadsPool.getRunnableNumber());
             SET_INT(AtomMin     ,ozconf.stackMinSize/TASKFRAMESIZE);
             SET_INT(AtomMax     ,ozconf.stackMaxSize/TASKFRAMESIZE););
    // PRIORITIES
    CASE_INT(PROP_PRIORITIES_HIGH,ozconf.hiMidRatio);
    CASE_INT(PROP_PRIORITIES_MEDIUM,ozconf.midLowRatio);
    CASE_REC(PROP_PRIORITIES,"priorities",(2,AtomHigh,AtomMedium),
             SET_INT(AtomHigh,ozconf.hiMidRatio);
             SET_INT(AtomMedium,ozconf.midLowRatio););
    // TIME
    CASE_INT(PROP_TIME_COPY,ozconf.timeDetailed?ozstat.timeForCopy.total:0);
    CASE_INT(PROP_TIME_GC,ozconf.timeDetailed?ozstat.timeForGC.total:0);
    CASE_INT(PROP_TIME_PROPAGATE,ozconf.timeDetailed?ozstat.timeForPropagation.total:0);
    CASE_INT(PROP_TIME_RUN,ozconf.timeDetailed ?
             (osUserTime() - (ozstat.timeForCopy.total +
                              ozstat.timeForGC.total +
                              ozstat.timeForPropagation.total)):0);
    CASE_INT(PROP_TIME_SYSTEM,osSystemTime());
    CASE_INT(PROP_TIME_TOTAL,osTotalTime());
    CASE_INT(PROP_TIME_USER,osUserTime());
    CASE_INT(PROP_TIME_IDLE,(int) ozstat.timeIdle);
    CASE_BOOL(PROP_TIME_DETAILED,ozconf.timeDetailed);
    CASE_REC(PROP_TIME,"time",
             (9,AtomCopy,AtomGC,AtomPropagate,AtomRun,
              AtomSystem,AtomTotal,AtomUser,AtomDetailed,AtomIdle),
             unsigned int timeNow = osUserTime();
             unsigned int copy = 0;
             unsigned int gc   = 0;
             unsigned int prop = 0;
             unsigned int run  = 0;
             if (ozconf.timeDetailed) {
               copy = ozstat.timeForCopy.total;
               gc   = ozstat.timeForGC.total;
               prop = ozstat.timeForPropagation.total;
               run  = timeNow - (copy + gc + prop);
             }
             SET_INT(AtomCopy,copy);
             SET_INT(AtomGC,gc);
             SET_INT(AtomPropagate,prop);
             SET_INT(AtomRun,run);
             SET_INT(AtomSystem,osSystemTime());
             SET_INT(AtomTotal,osTotalTime());
             SET_INT(AtomUser,timeNow);
             SET_INT(AtomIdle,(int) ozstat.timeIdle);
             SET_BOOL(AtomDetailed,ozconf.timeDetailed););
    // GC
    CASE_INT(PROP_GC_MIN,ozconf.heapMinSize*KB);
    CASE_INT(PROP_GC_MAX,ozconf.heapMaxSize*KB);
    CASE_INT(PROP_GC_FREE,ozconf.heapFree);
    CASE_INT(PROP_GC_TOLERANCE,ozconf.heapTolerance);
    CASE_INT(PROP_GC_CODE_CYCLES,ozconf.codeGCcycles);
    CASE_BOOL(PROP_GC_ON,ozconf.gcFlag);
    CASE_INT(PROP_GC_THRESHOLD,ozconf.heapThreshold*KB);
    CASE_INT(PROP_GC_SIZE,getUsedMemory()*KB);
    CASE_INT(PROP_GC_ACTIVE,ozstat.gcLastActive*KB);
    CASE_REC(PROP_GC,"gc",
             (9,AtomCodeCycles,AtomMin,AtomMax,AtomFree,AtomTolerance,
              AtomOn,AtomThreshold,AtomSize,AtomActive),
             SET_INT(AtomMin,       ozconf.heapMinSize*KB);
             SET_INT(AtomMax,       ozconf.heapMaxSize*KB);
             SET_INT(AtomFree,      ozconf.heapFree);
             SET_INT(AtomTolerance, ozconf.heapTolerance);
             SET_BOOL(AtomOn,       ozconf.gcFlag);
             SET_INT(AtomThreshold, ozconf.heapThreshold*KB);
             SET_INT(AtomSize,      getUsedMemory()*KB);
             SET_INT(AtomCodeCycles, ozconf.codeGCcycles);
             SET_INT(AtomActive,    ozstat.gcLastActive*KB););
    // PRINT
    CASE_INT(PROP_PRINT_DEPTH,ozconf.printDepth);
    CASE_INT(PROP_PRINT_WIDTH,ozconf.printWidth);
    CASE_REC(PROP_PRINT,"print",(2,AtomDepth,AtomWidth),
             SET_INT(AtomDepth, ozconf.printDepth);
             SET_INT(AtomWidth, ozconf.printWidth););
    // FD
    CASE_INT(PROP_FD_VARIABLES,ozstat.fdvarsCreated.total);
    CASE_INT(PROP_FD_PROPAGATORS,ozstat.propagatorsCreated.total);
    CASE_INT(PROP_FD_INVOKED,ozstat.propagatorsInvoked.total);
    CASE_INT(PROP_FD_THRESHOLD,32 * fd_bv_max_high);
    CASE_REC(PROP_FD,"fd",
             (4,AtomVariables,AtomPropagators,AtomInvoked,AtomThreshold),
             SET_INT(AtomVariables,   ozstat.fdvarsCreated.total);
             SET_INT(AtomPropagators, ozstat.propagatorsCreated.total);
             SET_INT(AtomInvoked,     ozstat.propagatorsInvoked.total);
             SET_INT(AtomThreshold,   32 * fd_bv_max_high););
    // SPACES
    CASE_INT(PROP_SPACES_COMMITTED,ozstat.solveAlt.total);
    CASE_INT(PROP_SPACES_CLONED,ozstat.solveCloned.total);
    CASE_INT(PROP_SPACES_CREATED,ozstat.solveCreated.total);
    CASE_INT(PROP_SPACES_FAILED,ozstat.solveFailed.total);
    CASE_INT(PROP_SPACES_SUCCEEDED,ozstat.solveSolved.total);
    CASE_REC(PROP_SPACES,"spaces",
             (5,AtomCommitted,AtomCloned,AtomCreated,AtomFailed,AtomSucceeded),
             SET_INT(AtomCommitted,ozstat.solveAlt.total);
             SET_INT(AtomCloned,ozstat.solveCloned.total);
             SET_INT(AtomCreated,ozstat.solveCreated.total);
             SET_INT(AtomFailed,ozstat.solveFailed.total);
             SET_INT(AtomSucceeded,ozstat.solveSolved.total););
    // ERRORS
  case PROP_ERRORS_HANDLER: {
    TaggedRef ehdl = am.getDefaultExceptionHdl();
    return ehdl ? ehdl : oz_nil();
  }
    CASE_BOOL(PROP_ERRORS_DEBUG,ozconf.errorDebug);
    CASE_INT(PROP_ERRORS_THREAD,ozconf.errorThreadDepth);
    CASE_INT(PROP_ERRORS_DEPTH,ozconf.errorPrintDepth);
    CASE_INT(PROP_ERRORS_WIDTH,ozconf.errorPrintWidth);
    CASE_REC(PROP_ERRORS,"errors",
             (4,AtomDebug,AtomThread,
              AtomDepth,AtomWidth),
             SET_BOOL(AtomDebug,ozconf.errorDebug);
             SET_INT(AtomThread,ozconf.errorThreadDepth);
             SET_INT(AtomDepth,ozconf.errorPrintDepth);
             SET_INT(AtomWidth,ozconf.errorPrintWidth););
    // MESSAGES
    CASE_BOOL(PROP_MESSAGES_GC,ozconf.gcVerbosity);
    CASE_BOOL(PROP_MESSAGES_IDLE,ozconf.showIdleMessage);
    CASE_REC(PROP_MESSAGES,"messages",
             (2,AtomGC,AtomIdle),
             SET_BOOL(AtomGC,ozconf.gcVerbosity);
             SET_BOOL(AtomIdle,ozconf.showIdleMessage););
    // MEMORY
    CASE_INT(PROP_MEMORY_ATOMS,ozstat.getAtomMemory());
    CASE_INT(PROP_MEMORY_NAMES,ozstat.getNameMemory());
    CASE_INT(PROP_MEMORY_FREELIST,FL_Manager::getSize());
    CASE_INT(PROP_MEMORY_CODE,CodeArea::totalSize);
    CASE_INT(PROP_MEMORY_HEAP,ozstat.heapUsed.total+getUsedMemory());
    CASE_REC(PROP_MEMORY,"memory",
             (5,AtomAtoms,AtomNames,AtomFreelist,
              AtomCode,AtomHeap),
             SET_INT(AtomAtoms,ozstat.getAtomMemory());
             SET_INT(AtomNames,ozstat.getNameMemory());
             SET_INT(AtomFreelist,FL_Manager::getSize());
             SET_INT(AtomCode,CodeArea::totalSize);
             SET_INT(AtomHeap,ozstat.heapUsed.total+getUsedMemory()););
    // LIMITS
    CASE_INT(PROP_LIMITS_INT_MIN,OzMinInt);
    CASE_INT(PROP_LIMITS_INT_MAX,OzMaxInt);
    CASE_INT(PROP_LIMITS_BYTECODE_XREGISTERS,NumberOfXRegisters);
    CASE_REC(PROP_LIMITS,"limits",
             (3,AtomIntMin,AtomIntMax,AtomBytecodeXRegisters),
             SET_INT(AtomIntMin,OzMinInt);
             SET_INT(AtomIntMax,OzMaxInt);
             SET_INT(AtomBytecodeXRegisters,NumberOfXRegisters););
    // APPLICATION
  case PROP_APPLICATION_ARGS: { return getApplicationArgs(); }
    CASE_ATOM(PROP_APPLICATION_URL,ozconf.url);
    CASE_BOOL(PROP_APPLICATION_GUI,ozconf.gui==1);
    CASE_REC(PROP_APPLICATION,"application",
             (3,AtomArgs,AtomURL,AtomGUI),
             SET_BOOL(AtomGUI,ozconf.gui==1);
             SET_ATOM(AtomURL,ozconf.url);
             SET_REC(AtomArgs,getApplicationArgs()););
    // PLATFORM
    CASE_ATOM(PROP_PLATFORM_NAME, ozplatform);
    CASE_ATOM(PROP_PLATFORM_OS,   ozconf.osname);
    CASE_ATOM(PROP_PLATFORM_ARCH, ozconf.cpu);
    CASE_REC(PROP_PLATFORM,"platform",
             (3,AtomName, AtomOs, AtomArch),
             SET_ATOM(AtomName,ozplatform);
             SET_ATOM(AtomOs,ozconf.osname);
             SET_ATOM(AtomArch,ozconf.cpu););
    // MISC
  CASE_BOOL(PROP_STANDALONE,!ozconf.runningUnderEmacs);
  CASE_ATOM(PROP_OZ_CONFIGURE_HOME,OZ_CONFIGURE_PREFIX);
  CASE_ATOM(PROP_OZ_EMULATOR_HOME,ozconf.emuhome);
  CASE_ATOM(PROP_OZ_VERSION,AMVersion);
  CASE_ATOM(PROP_OZ_DATE,AMDate);
  // DISTRIBUTION
#ifdef VIRTUALSITES
  CASE_BOOL(PROP_DISTRIBUTION_VIRTUALSITES,OK);
#else
  CASE_BOOL(PROP_DISTRIBUTION_VIRTUALSITES,NO);
#endif
  // INTERNAL
  CASE_BOOL(PROP_INTERNAL_DEBUG,am.debugmode());
  CASE_BOOL(PROP_INTERNAL_PROPLOCATION,am.isPropagatorLocation());
  CASE_BOOL(PROP_INTERNAL_SUSPENSION,ozconf.showSuspension);
  CASE_BOOL(PROP_INTERNAL_STOP,ozconf.stopOnToplevelFailure);
  CASE_INT(PROP_INTERNAL_DEBUG_IP,ozconf.debugIP);
  CASE_INT(PROP_PERDIO_DEBUG,ozconf.debugPerdio);
  CASE_BOOL(PROP_PERDIO_SEIFHANDLER,ozconf.perdioSeifHandler);
  CASE_INT(PROP_PERDIO_FLOWBUFFERSIZE,ozconf.perdioFlowBufferSize);
  CASE_INT(PROP_PERDIO_FLOWBUFFERTIME,ozconf.perdioFlowBufferTime);
  CASE_INT(PROP_PERDIO_TIMEOUT,ozconf.perdioTimeout);
  CASE_INT(PROP_PERDIO_TEMPRETRYCEILING,ozconf.perdioTempRetryCeiling);
  CASE_INT(PROP_PERDIO_TEMPRETRYFLOOR,ozconf.perdioTempRetryFloor);
  CASE_INT(PROP_PERDIO_TEMPRETRYFACTOR,ozconf.perdioTempRetryFactor);
  CASE_INT(PROP_PERDIO_MAXTCPCACHE,ozconf.perdioMaxTCPCache);
  CASE_INT(PROP_PERDIO_CHECKALIVEINTERVAL,ozconf.perdioCheckAliveInterval);
  CASE_BOOL(PROP_PERDIO_MINIMAL,ozconf.perdioMinimal);

  case PROP_PERDIO_VERSION: return OZ_pair2(oz_int(PERDIOMAJOR),
                                            oz_int(PERDIOMINOR));

  CASE_BOOL(PROP_PERDIO_USEALTVARPROTOCOL,ozconf.perdioUseAltVarProtocol);
  CASE_REC(PROP_PERDIO,"perdio",
           (12,oz_atomNoDup("useAltVarProtocol"),oz_atomNoDup("minimal"),
            oz_atomNoDup("seifHandler"),oz_atomNoDup("debug"),
            oz_atomNoDup("flowbuffersize"),oz_atomNoDup("flowbuffertime"),
            oz_atomNoDup("version"),oz_atomNoDup("tempRetryCeiling"),
            oz_atomNoDup("tempRetryFloor"),oz_atomNoDup("tempRetryFactor"),
            oz_atomNoDup("maxTCPCache"),oz_atomNoDup("checkAliveInterval")),
           SET_BOOL(oz_atomNoDup("useAltVarProtocol"),
                    ozconf.perdioUseAltVarProtocol);
           SET_BOOL(oz_atomNoDup("minimal"), ozconf.perdioMinimal);
           SET_BOOL(oz_atomNoDup("seifHandler"), ozconf.perdioSeifHandler);
           SET_INT(oz_atomNoDup("debug"), ozconf.debugPerdio);
           SET_INT(oz_atomNoDup("flowbuffersize"),ozconf.perdioFlowBufferSize);
           SET_INT(oz_atomNoDup("flowbuffertime"),ozconf.perdioFlowBufferTime);
           SET_INT(oz_atomNoDup("tempRetryCeiling"),
                   ozconf.perdioTempRetryCeiling);
           SET_INT(oz_atomNoDup("tempRetryFloor"),ozconf.perdioTempRetryFloor);
           SET_INT(oz_atomNoDup("tempRetryFactor"),
                   ozconf.perdioTempRetryFactor);
           SET_INT(oz_atomNoDup("maxTCPCache"), ozconf.perdioMaxTCPCache);
           SET_INT(oz_atomNoDup("checkAliveInterval"),
                   ozconf.perdioCheckAliveInterval);
           SET_REC(oz_atomNoDup("version"), OZ_pair2(oz_int(PERDIOMAJOR),
                                                oz_int(PERDIOMINOR)));
           );
  CASE_INT(PROP_DPTABLE_DEFAULTOWNERTABLESIZE,
           ozconf.dpTableDefaultOwnerTableSize);
  CASE_INT(PROP_DPTABLE_DEFAULTBORROWTABLESIZE,
           ozconf.dpTableDefaultBorrowTableSize);
  CASE_INT(PROP_DPTABLE_LOWLIMIT, ozconf.dpTableLowLimit);
  CASE_INT(PROP_DPTABLE_EXPANDFACTOR, ozconf.dpTableExpandFactor);
  CASE_INT(PROP_DPTABLE_BUFFER, ozconf.dpTableBuffer);
  CASE_INT(PROP_DPTABLE_WORTHWHILEREALLOC, ozconf.dpTableWorthwhileRealloc);
  CASE_REC(PROP_DPTABLE,"dpTable",
           (6,oz_atomNoDup("defaultOwnerTableSize"),
            oz_atomNoDup("defaultBorrowTableSize"),
            oz_atomNoDup("lowLimit"),oz_atomNoDup("expandFactor"),oz_atomNoDup("buffer"),
            oz_atomNoDup("worthwhileRealloc")),
           SET_INT(oz_atomNoDup("defaultOwnerTableSize"),
                   ozconf.dpTableDefaultOwnerTableSize);
           SET_INT(oz_atomNoDup("defaultBorrowTableSize"),
                   ozconf.dpTableDefaultBorrowTableSize);
           SET_INT(oz_atomNoDup("lowLimit"), ozconf.dpTableLowLimit);
           SET_INT(oz_atomNoDup("expandFactor"), ozconf.dpTableExpandFactor);
           SET_INT(oz_atomNoDup("buffer"), ozconf.dpTableBuffer);
           SET_INT(oz_atomNoDup("worthwhileRealloc"),
                   ozconf.dpTableWorthwhileRealloc);
           );
  CASE_INT(PROP_CLOSE_TIME,ozconf.closetime);
  CASE_BOOL(PROP_OZ_STYLE_USE_FUTURES,ozconf.useFutures);
  default:
    return 0; // not readable. 0 ok because no OZ_Term==0
  }
}

#undef CASE_INT
#undef CASE_BOOL
#undef CASE_ATOM
#undef DEFINE_REC
#undef RETURN_REC
#undef SET_REC
#undef DO_REC
#undef CASE_REC
#undef SET_INT
#undef SET_BOOL

// Macros for manipulating the `val' argument of SetEmulatorProperty
// val has been DEREFed and there is also val_ptr and val_tag, and it
// is guaranteed that val is determined.
//
// Here we check that it is a boolean.  And we put its value in the
// local integer variable INT__

#define CHECK_BOOL                              \
if      (oz_isTrue(val )) INT__ = 1;            \
else if (oz_isFalse(val)) INT__ = 0;            \
else oz_typeError(1,"Bool");

// Handle a particular indexed property P, check that the specified
// val is a boolean, and do something (presumably using variable INT__)
// CASE_BOOL(P,L) is a specialization to update location L

#define CASE_BOOL_DO(P,DO) case P: CHECK_BOOL; DO; return PROCEED;
#define CASE_BOOL(P,L) CASE_BOOL_DO(P,L=INT__);

// Check that the value is a non-negative small integer

#define CHECK_NAT                               \
if (!isSmallIntTag(val_tag) ||                  \
    (INT__=smallIntValue(val))<0)               \
  oz_typeError(1,"Int>=0");

// Handle the case of indexed property P that should be an int>=0

#define CASE_NAT_DO(P,DO) case P: CHECK_NAT; DO; return PROCEED;
#define CASE_NAT(P,L) CASE_NAT_DO(P,L=INT__);

// Check that the value is an integer in [1..100], i.e. a percentage

#define CHECK_PERCENT                           \
if (!isSmallIntTag(val_tag) ||                  \
    (INT__=smallIntValue(val))<1 ||             \
    (INT__>100))                                \
  oz_typeError(1,"Int[1..100]");

// Handle the case of indexed property P that should a percentage

#define CASE_PERCENT_DO(P,DO) case P: CHECK_PERCENT; DO; return PROCEED;
#define CASE_PERCENT(P,L) CASE_PERCENT_DO(P,L=INT__);

// Check that the value is a record, if so untag it into REC__

#define CHECK_REC                               \
if (!isSRecordTag(val_tag))                     \
{oz_typeError(1,"SRecord")}                     \
else REC__=tagged2SRecord(val);

// Handle the case of an indexed property P that should be a record,
// and DO something (presumably using REC__)

#define CASE_REC(P,DO)                          \
case P: { CHECK_REC; DO; return PROCEED; }

// Signal that feature F on the record value is not of the
// expected type T

#define BAD_FEAT(F,T)                           \
return oz_raise(E_ERROR,E_SYSTEM,"putProperty",2,F,oz_atom(T));

// Lookup feature F.  If it exists, make sure that it is a
// determined integer, then DO something (presumably with INT__)
// Note: we are using the equivalence between OZ_Term and int
// to reuse variable INT__ both as the term which is the value
// of the feature and as the corresonding integer value obtained
// by untagging it.

#define DO_INT(F,DO)                            \
INT__ = REC__->getFeature(F);                   \
if (INT__) {                                    \
  DEREF(INT__,PTR__,TAG__);                     \
  if (oz_isVariable(TAG__)) oz_suspendOnPtr(PTR__); \
  if (oz_isSmallInt(TAG__)) {                   \
    INT__=smallIntValue(INT__);                 \
  } else if (oz_isBigInt(INT__)) {              \
    INT__=tagged2BigInt(INT__)->getInt();       \
  } else {                                      \
    BAD_FEAT(F,"Int");                          \
  }                                             \
  DO;                                           \
}

// set location L to integer value on feature F
#define SET_INT(F,L) DO_INT(F,L=INT__);

// Feature F should be a boolean, then DO something
#define DO_BOOL(F,DO)                                   \
INT__ = REC__->getFeature(F);                           \
if(INT__) {                                             \
  DEREF(INT__,PTR__,TAG__);                             \
  if (oz_isVariable(TAG__)) oz_suspendOnPtr(PTR__);     \
  if (!isLiteralTag(TAG__)) BAD_FEAT(F,"Bool");         \
  if      (oz_isTrue(INT__)) INT__=1;                   \
  else if (oz_isFalse(INT__)) INT__=0;                  \
  else BAD_FEAT(F,"Bool");                              \
  DO;                                                   \
}

// set location L to boolean value on feature F
#define SET_BOOL(F,L) DO_BOOL(F,L=INT__);

// Feature F should be a non-negative integer, then DO something
#define DO_NAT(F,DO)                            \
DO_INT(F,if (INT__<0) {oz_typeError(1,"Int>=0");}; DO);

// set location L to non-negative integer value on feature F
#define SET_NAT(F,L) DO_NAT(F,L=INT__);

// Feature F should be a percentage, then DO something
#define DO_PERCENT(F,DO)                        \
DO_INT(F,if (INT__<1||INT__>100) {oz_typeError(1,"Int[1..100]");}; DO);

// set location L to percentage value on feature F
#define SET_PERCENT(F,L) DO_PERCENT(F,L=INT__);

// val is guaranteed to be determined and derefed
OZ_Return SetEmulatorProperty(EmulatorPropertyIndex prop,OZ_Term val) {
  DEREF(val,val_ptr,val_tag);
  int      INT__;
  SRecord* REC__;
  switch (prop) {
    // TIME
    CASE_BOOL(PROP_TIME_DETAILED,ozconf.timeDetailed);
    CASE_REC(PROP_TIME,SET_BOOL(AtomDetailed,ozconf.timeDetailed););
    // THREADS
    CASE_NAT_DO(PROP_THREADS_MIN,{
      ozconf.stackMinSize=INT__/TASKFRAMESIZE;
      if (ozconf.stackMinSize > ozconf.stackMaxSize)
        ozconf.stackMaxSize = ozconf.stackMinSize;});
    CASE_NAT_DO(PROP_THREADS_MAX,{
      ozconf.stackMaxSize=INT__/TASKFRAMESIZE;
      if (ozconf.stackMinSize > ozconf.stackMaxSize)
        ozconf.stackMinSize = ozconf.stackMaxSize;});
    CASE_REC(PROP_THREADS,
             DO_NAT(AtomMin,
                    ozconf.stackMinSize=INT__/TASKFRAMESIZE;
                    if (ozconf.stackMinSize > ozconf.stackMaxSize)
                    ozconf.stackMinSize = ozconf.stackMaxSize;);
             DO_NAT(AtomMax,
                    ozconf.stackMaxSize=INT__/TASKFRAMESIZE;
                    if (ozconf.stackMinSize > ozconf.stackMaxSize)
                    ozconf.stackMaxSize = ozconf.stackMinSize;););
    // PRIORITIES
    CASE_PERCENT(PROP_PRIORITIES_HIGH,ozconf.hiMidRatio);
    CASE_PERCENT(PROP_PRIORITIES_MEDIUM,ozconf.midLowRatio);
    CASE_REC(PROP_PRIORITIES,
             SET_PERCENT(AtomHigh,ozconf.hiMidRatio);
             SET_PERCENT(AtomMedium,ozconf.midLowRatio););
    // GC
    CASE_NAT_DO(PROP_GC_MAX,{
      ozconf.heapMaxSize=INT__/KB;
      if (ozconf.heapMinSize > ozconf.heapMaxSize)
        ozconf.heapMinSize = ozconf.heapMaxSize;
      if (ozconf.heapThreshold > ozconf.heapMaxSize) {
        am.setSFlag(StartGC);
        return BI_PREEMPT;}});
    CASE_NAT_DO(PROP_GC_MIN,{
      ozconf.heapMinSize=INT__/KB;
      if (ozconf.heapMinSize > ozconf.heapMaxSize)
        ozconf.heapMaxSize = ozconf.heapMinSize;
      if (ozconf.heapMinSize > ozconf.heapThreshold)
        ozconf.heapThreshold = ozconf.heapMinSize;
      if (ozconf.heapThreshold > ozconf.heapMaxSize) {
        am.setSFlag(StartGC);
        return BI_PREEMPT;}});
    CASE_PERCENT(PROP_GC_FREE,ozconf.heapFree);
    CASE_PERCENT(PROP_GC_TOLERANCE,ozconf.heapTolerance);
    CASE_NAT(PROP_GC_CODE_CYCLES,ozconf.codeGCcycles);
    CASE_BOOL(PROP_GC_ON,ozconf.gcFlag);
    CASE_REC(PROP_GC,
             DO_NAT(AtomMin,ozconf.heapMinSize=INT__/KB);
             DO_NAT(AtomMax,ozconf.heapMaxSize=INT__/KB);
             SET_NAT(AtomCodeCycles,ozconf.codeGCcycles);
             if (ozconf.heapMinSize > ozconf.heapMaxSize)
               ozconf.heapMaxSize = ozconf.heapMinSize;
             SET_PERCENT(AtomFree,ozconf.heapFree);
             SET_PERCENT(AtomTolerance,ozconf.heapTolerance);
             SET_BOOL(AtomOn,ozconf.gcFlag);
             if (ozconf.heapThreshold > ozconf.heapMaxSize) {
               am.setSFlag(StartGC);
               return BI_PREEMPT;
             });
    // PRINT
    CASE_NAT(PROP_PRINT_WIDTH,ozconf.printWidth);
    CASE_NAT(PROP_PRINT_DEPTH,ozconf.printDepth);
    CASE_REC(PROP_PRINT,
             SET_NAT(AtomWidth,ozconf.printWidth);
             SET_NAT(AtomDepth,ozconf.printDepth););
    // FD
    CASE_NAT_DO(PROP_FD_THRESHOLD,reInitFDs(INT__));
    CASE_REC(PROP_FD,
             DO_NAT(AtomThreshold,reInitFDs(INT__)););
    // ERRORS
  case PROP_ERRORS_HANDLER: {
    if (oz_isVariable(val))
      return SUSPEND;

    if (!oz_isProcedure(val) || tagged2Const(val)->getArity()!=1) {
      oz_typeError(0,"Procedure/1");
    }

    am.setDefaultExceptionHdl(val);
    return PROCEED;
  }
    CASE_BOOL(PROP_ERRORS_DEBUG,ozconf.errorDebug);
    CASE_NAT(PROP_ERRORS_THREAD,ozconf.errorThreadDepth);
    CASE_NAT(PROP_ERRORS_WIDTH,ozconf.errorPrintWidth);
    CASE_NAT(PROP_ERRORS_DEPTH,ozconf.errorPrintDepth);
    CASE_REC(PROP_ERRORS,
             SET_BOOL(AtomDebug,ozconf.errorDebug);
             SET_NAT(AtomThread,ozconf.errorThreadDepth);
             SET_NAT(AtomWidth,ozconf.errorPrintWidth);
             SET_NAT(AtomDepth,ozconf.errorPrintDepth););
    // MESSAGES
    CASE_BOOL(PROP_MESSAGES_GC,ozconf.gcVerbosity);
    CASE_BOOL(PROP_MESSAGES_IDLE,ozconf.showIdleMessage);
    CASE_REC(PROP_MESSAGES,
             SET_BOOL(AtomGC,ozconf.gcVerbosity);
             SET_BOOL(AtomIdle,ozconf.showIdleMessage););
    // INTERNAL
    CASE_BOOL_DO(PROP_INTERNAL_DEBUG,
                 if (INT__) am.setdebugmode(OK);
                 else       am.setdebugmode(NO));
    CASE_BOOL_DO(PROP_INTERNAL_PROPLOCATION,
                 if (INT__) am.setPropagatorLocation(OK);
                 else       am.setPropagatorLocation(NO));
    CASE_BOOL(PROP_INTERNAL_SUSPENSION,ozconf.showSuspension);
    CASE_BOOL(PROP_INTERNAL_STOP,ozconf.stopOnToplevelFailure);
    CASE_NAT(PROP_INTERNAL_DEBUG_IP,ozconf.debugIP);
    CASE_REC(PROP_INTERNAL,
             DO_BOOL(AtomDebug, am.setdebugmode((INT__)?OK:NO));
             SET_BOOL(AtomShowSuspension,ozconf.showSuspension);
             SET_BOOL(AtomStopOnToplevelFailure,ozconf.stopOnToplevelFailure);
             SET_NAT(AtomDebugIP,ozconf.debugIP));


    CASE_NAT(PROP_PERDIO_DEBUG,ozconf.debugPerdio);

    CASE_BOOL(PROP_PERDIO_USEALTVARPROTOCOL,ozconf.perdioUseAltVarProtocol);
    CASE_BOOL_DO(PROP_PERDIO_MINIMAL,
                 if ((*isPerdioInitialized)())
                   return OZ_raiseDebug(OZ_makeException(E_ERROR,oz_atomNoDup("dp"),
                                                         "modelChoose",0));
                 ozconf.perdioMinimal=INT__);
    CASE_BOOL(PROP_PERDIO_SEIFHANDLER,ozconf.perdioSeifHandler);
    CASE_NAT(PROP_PERDIO_FLOWBUFFERSIZE,ozconf.perdioFlowBufferSize);
    CASE_NAT(PROP_PERDIO_FLOWBUFFERTIME,ozconf.perdioFlowBufferTime);
    CASE_NAT(PROP_PERDIO_TEMPRETRYCEILING,ozconf.perdioTempRetryCeiling);
    CASE_NAT(PROP_PERDIO_TEMPRETRYFLOOR,ozconf.perdioTempRetryFloor);
    CASE_NAT(PROP_PERDIO_TEMPRETRYFACTOR,ozconf.perdioTempRetryFactor);
    CASE_NAT(PROP_PERDIO_MAXTCPCACHE,ozconf.perdioMaxTCPCache);
    CASE_NAT(PROP_PERDIO_CHECKALIVEINTERVAL,ozconf.perdioCheckAliveInterval);
    CASE_NAT(PROP_PERDIO_TIMEOUT,ozconf.perdioTimeout);
    // PERDIO
    CASE_REC(PROP_PERDIO,
             SET_NAT(AtomDebugPerdio,ozconf.debugPerdio);
             SET_NAT(oz_atomNoDup("flowbuffersize"),ozconf.perdioFlowBufferSize);
             SET_NAT(oz_atomNoDup("flowbuffertime"),ozconf.perdioFlowBufferTime);
             SET_NAT(oz_atomNoDup("seifHandler"),ozconf.perdioSeifHandler);
             SET_NAT(oz_atomNoDup("tempRetryCeiling"),
                      ozconf.perdioTempRetryCeiling);
             SET_NAT(oz_atomNoDup("tempRetryFloor"),
                      ozconf.perdioTempRetryFloor);
             SET_NAT(oz_atomNoDup("tempRetryFactor"),
                      ozconf.perdioTempRetryFactor);
             SET_NAT(oz_atomNoDup("maxTCPCache"),ozconf.perdioMaxTCPCache);
             SET_NAT(oz_atomNoDup("checkAliveInterval"),
                      ozconf.perdioCheckAliveInterval);
             SET_BOOL(oz_atomNoDup("useAltVarProtocol"),
                      ozconf.perdioUseAltVarProtocol);
             DO_BOOL(oz_atomNoDup("minimal"),
                 if ((*isPerdioInitialized)())
                   return OZ_raiseDebug(OZ_makeException(E_ERROR,oz_atomNoDup("dp"),
                                                         "modelChoose",0));
                 ozconf.perdioMinimal=INT__));
    // DPTABLE
    CASE_NAT_DO(PROP_DPTABLE_DEFAULTOWNERTABLESIZE,{
      ozconf.dpTableDefaultOwnerTableSize=INT__;
      am.setSFlag(StartGC);
      return BI_PREEMPT;});
    CASE_NAT_DO(PROP_DPTABLE_DEFAULTBORROWTABLESIZE,{
      ozconf.dpTableDefaultBorrowTableSize=INT__;
      am.setSFlag(StartGC);
      return BI_PREEMPT;});
    CASE_PERCENT_DO(PROP_DPTABLE_LOWLIMIT,{
      ozconf.dpTableLowLimit=INT__;
      am.setSFlag(StartGC);
      return BI_PREEMPT;});
    CASE_NAT_DO(PROP_DPTABLE_EXPANDFACTOR,{
      ozconf.dpTableExpandFactor=INT__;
      am.setSFlag(StartGC);
      return BI_PREEMPT;});
    CASE_NAT_DO(PROP_DPTABLE_BUFFER, {
      ozconf.dpTableBuffer=INT__;
      am.setSFlag(StartGC);
      return BI_PREEMPT;});
    CASE_NAT_DO(PROP_DPTABLE_WORTHWHILEREALLOC,{
      ozconf.dpTableWorthwhileRealloc=INT__;
      am.setSFlag(StartGC);
      return BI_PREEMPT;});
    CASE_REC(PROP_DPTABLE,
             SET_NAT(oz_atomNoDup("defaultOwnerTableSize"),
                   ozconf.dpTableDefaultOwnerTableSize);
             SET_NAT(oz_atomNoDup("defaultBorrowTableSize"),
                     ozconf.dpTableDefaultBorrowTableSize);
             SET_NAT(oz_atomNoDup("lowLimit"), ozconf.dpTableLowLimit);
             SET_NAT(oz_atomNoDup("expandFactor"),
                     ozconf.dpTableExpandFactor);
             SET_NAT(oz_atomNoDup("buffer"), ozconf.dpTableBuffer);
             SET_NAT(oz_atomNoDup("worthwhileRealloc"),
                     ozconf.dpTableWorthwhileRealloc);
             );

    CASE_NAT(PROP_CLOSE_TIME,ozconf.closetime);
    CASE_BOOL_DO(PROP_STANDALONE,ozconf.runningUnderEmacs=!INT__);
    CASE_BOOL(PROP_OZ_STYLE_USE_FUTURES,ozconf.useFutures);
default:
    return PROP__NOT__WRITABLE;
  }
}

// VIRTUAL PROPERTIES
//

// The idea is to make system information available through a
// dictionary-like interface, in a manner that is trivially extensible,
// and such that we avoid the proliferation of specialized builtins.
//
// A virtual property is an object with virtual functions get and set.
// get() returns an OZ_Term as the (current) value of the property.
// set(V) sets the value of the property to the value described by the
// OZ_Term V.  It returns an OZ_Return status that indicate how the
// operation fared: it could succeed, fail, suspend, or raise an error.
//
// Virtual properties will be recorded as foreign pointers in
// dictionary vprop_registry.  Emulator properties will enjoy an
// optimized representation: they will simply be represented as
// integers in vprop_registry, to be interpreted by GetEmulatorProperty
// and SetEmulatorProperty.

OZ_Term   VirtualProperty::get()        { Assert(0); return NameUnit; }
OZ_Return VirtualProperty::set(OZ_Term) { Assert(0); return FAILED  ; }

static OZ_Term vprop_registry;
OZ_Term system_registry;        // eventually make it static [TODO]

inline
void VirtualProperty::add(const char * s, const int p) {
  tagged2Dictionary(vprop_registry)->setArg(oz_atomNoDup(s),
                                            newSmallInt(p));
}

// in addition to the usual OZ_Return values, the following
// may also return PROP__NOT__READABLE and PROP__NOT__FOUND

OZ_Return GetProperty(TaggedRef k,TaggedRef& val)
{
  TaggedRef key = k;
  DEREF(key,key_ptr,key_tag);
  if (oz_isVariable(key_tag)) oz_suspendOnPtr(key_ptr);
  if (!oz_isAtom(key)) oz_typeError(0,"Atom");
  OzDictionary* dict;
  TaggedRef entry;
  dict = tagged2Dictionary(vprop_registry);
  if (dict->getArg(key,entry)==PROCEED)
    if (oz_isInt(entry)) {
      entry = GetEmulatorProperty((EmulatorPropertyIndex)
                                  oz_IntToC(entry));
      if (entry) { val=entry; return PROCEED; }
      else return PROP__NOT__READABLE;
    } else {
      val = ((VirtualProperty*)
             OZ_getForeignPointer(entry))->get();
      return PROCEED;
    }
  dict = tagged2Dictionary(system_registry);
  if (dict->getArg(key,entry)==PROCEED) {
    val=entry; return PROCEED;
  }
  return PROP__NOT__FOUND;
}

// in addition to the usual OZ_Return values, the following
// may also return PROP__NOT__WRITABLE and PROP__NOT__GLOBAL.

OZ_Return PutProperty(TaggedRef k,TaggedRef v)
{
  if (!oz_onToplevel()) return PROP__NOT__GLOBAL;
  TaggedRef key = k;
  DEREF(key,key_ptr,key_tag);
  if (oz_isVariable(key_tag)) oz_suspendOnPtr(key_ptr);
  if (!oz_isAtom(key)) oz_typeError(0,"Atom");
  OzDictionary* dict;
  TaggedRef entry;
  dict = tagged2Dictionary(vprop_registry);
  if (dict->getArg(key,entry)==PROCEED)
    if (OZ_isInt(entry)) {
      return SetEmulatorProperty((EmulatorPropertyIndex)
                                 oz_IntToC(entry),v);
    } else
      return ((VirtualProperty*)
              OZ_getForeignPointer(entry))->set(v);
  dict = tagged2Dictionary(system_registry);
  dict->setArg(key,v);
  return PROCEED;
}

OZ_BI_define(BIgetProperty,1,1)
{
  OZ_declareTerm(0,key);
  OZ_Return status = GetProperty(key,OZ_out(0));
  if (status == PROP__NOT__READABLE)
    return oz_raise(E_ERROR,E_SYSTEM,"getProperty",1,key);
  else if (status == PROP__NOT__FOUND)
    return oz_raise(E_SYSTEM,E_SYSTEM,"getProperty",1,key);
  else return status;
} OZ_BI_end

OZ_BI_define(BIcondGetProperty,2,1)
{
  OZ_declareTerm(0,key);
  OZ_declareTerm(1,def);
  OZ_Return status = GetProperty(key,OZ_out(0));
  if (status == PROP__NOT__READABLE)
    return oz_raise(E_ERROR,E_SYSTEM,"condGetProperty",1,key);
  else if (status == PROP__NOT__FOUND)
    OZ_RETURN(def);
  else return status;
} OZ_BI_end

OZ_BI_define(BIputProperty,2,0)
{
  OZ_declareTerm(0,key);
  OZ_declareTerm(1,val);
  OZ_Return status = PutProperty(key,val);
  if (status == PROP__NOT__WRITABLE)
    return oz_raise(E_ERROR,E_SYSTEM,"putProperty",1,key);
  else if (status == PROP__NOT__GLOBAL)
    return oz_raise(E_ERROR,E_KERNEL,"globalState",
                    1,oz_atomNoDup("putProperty"));
  else return status;
} OZ_BI_end

struct prop_entry {
  const char * name;
  enum EmulatorPropertyIndex epi;
};

static const struct prop_entry prop_entries[] = {
  // THREADS
  {"threads.created",PROP_THREADS_CREATED},
  {"threads.runnable",PROP_THREADS_RUNNABLE},
  {"threads.min",PROP_THREADS_MIN},
  {"threads.max",PROP_THREADS_MAX},
  {"threads",PROP_THREADS},
  // PRIORITIES
  {"priorities.high",PROP_PRIORITIES_HIGH},
  {"priorities.medium",PROP_PRIORITIES_MEDIUM},
  {"priorities",PROP_PRIORITIES},
  // TIME
  {"time.copy",PROP_TIME_COPY},
  {"time.gc",PROP_TIME_GC},
  {"time.propagate",PROP_TIME_PROPAGATE},
  {"time.run",PROP_TIME_RUN},
  {"time.system",PROP_TIME_SYSTEM},
  {"time.total",PROP_TIME_TOTAL},
  {"time.user",PROP_TIME_USER},
  {"time.idle",PROP_TIME_IDLE},
  {"time.detailed",PROP_TIME_DETAILED},
  {"time",PROP_TIME},
  // GC
  {"gc.min",PROP_GC_MIN},
  {"gc.max",PROP_GC_MAX},
  {"gc.free",PROP_GC_FREE},
  {"gc.tolerance",PROP_GC_TOLERANCE},
  {"gc.on",PROP_GC_ON},
  {"gc.codeCycles",PROP_GC_CODE_CYCLES},
  {"gc.threshold",PROP_GC_THRESHOLD},
  {"gc.size",PROP_GC_SIZE},
  {"gc.active",PROP_GC_ACTIVE},
  {"gc",PROP_GC},
  // PRINT
  {"print.depth",PROP_PRINT_DEPTH},
  {"print.width",PROP_PRINT_WIDTH},
  {"print",PROP_PRINT},
  // FD
  {"fd.variables",PROP_FD_VARIABLES},
  {"fd.propagators",PROP_FD_PROPAGATORS},
  {"fd.invoked",PROP_FD_INVOKED},
  {"fd.threshold",PROP_FD_THRESHOLD},
  {"fd",PROP_FD},
  // SPACES
  {"spaces.committed",PROP_SPACES_COMMITTED},
  {"spaces.cloned",PROP_SPACES_CLONED},
  {"spaces.created",PROP_SPACES_CREATED},
  {"spaces.failed",PROP_SPACES_FAILED},
  {"spaces.succeeded",PROP_SPACES_SUCCEEDED},
  {"spaces",PROP_SPACES},
  // ERRORS
  {"errors.handler",PROP_ERRORS_HANDLER},
  {"errors.debug",PROP_ERRORS_DEBUG},
  {"errors.thread",PROP_ERRORS_THREAD},
  {"errors.depth",PROP_ERRORS_DEPTH},
  {"errors.width",PROP_ERRORS_WIDTH},
  {"errors",PROP_ERRORS},
  // MESSAGES
  {"messages.gc",PROP_MESSAGES_GC},
  {"messages.idle",PROP_MESSAGES_IDLE},
  {"messages",PROP_MESSAGES},
  // MEMORY
  {"memory.atoms",PROP_MEMORY_ATOMS},
  {"memory.names",PROP_MEMORY_NAMES},
  {"memory.freelist",PROP_MEMORY_FREELIST},
  {"memory.code",PROP_MEMORY_CODE},
  {"memory.heap",PROP_MEMORY_HEAP},
  {"memory",PROP_MEMORY},
  // LIMITS
  {"limits.int.min",PROP_LIMITS_INT_MIN},
  {"limits.int.max",PROP_LIMITS_INT_MAX},
  {"limits.bytecode.xregisters",
                       PROP_LIMITS_BYTECODE_XREGISTERS},
  {"limits",PROP_LIMITS},
  // APPLICATION
  {"application.args",PROP_APPLICATION_ARGS},
  {"application.url",PROP_APPLICATION_URL},
  {"application.gui",PROP_APPLICATION_GUI},
  {"application",PROP_APPLICATION},
  // PLATFORM
  {"platform.name", PROP_PLATFORM_NAME},
  {"platform.os",   PROP_PLATFORM_OS},
  {"platform.arch", PROP_PLATFORM_ARCH},
  {"platform",      PROP_PLATFORM},
  // MISC
  {"oz.standalone",PROP_STANDALONE},
  {"oz.configure.home",PROP_OZ_CONFIGURE_HOME},
  {"oz.emulator.home",PROP_OZ_EMULATOR_HOME},
  {"oz.version",PROP_OZ_VERSION},
  {"oz.date",PROP_OZ_DATE},
  // Suspending on SimpleVars raises an exception
  {"oz.style.useFutures",PROP_OZ_STYLE_USE_FUTURES},
  // Distribution
  {"distribution.virtualsites",
                       PROP_DISTRIBUTION_VIRTUALSITES},
  // INTERNAL
  {"internal",PROP_INTERNAL},
  {"internal.debug",PROP_INTERNAL_DEBUG},
  {"internal.propLocation",PROP_INTERNAL_PROPLOCATION},
  {"internal.suspension",PROP_INTERNAL_SUSPENSION},
  {"internal.stop",PROP_INTERNAL_STOP},
  {"internal.ip.debug",PROP_INTERNAL_DEBUG_IP},
  // PERDIO
  {"perdio.debug",PROP_PERDIO_DEBUG},
  {"perdio.useAltVarProtocol",
                       PROP_PERDIO_USEALTVARPROTOCOL},
  {"perdio.minimal",PROP_PERDIO_MINIMAL},
  {"perdio.version",PROP_PERDIO_VERSION},
  {"perdio.flowbuffersize",PROP_PERDIO_FLOWBUFFERSIZE},
  {"perdio.flowbuffertime",PROP_PERDIO_FLOWBUFFERTIME},
  {"perdio.timeout",PROP_PERDIO_TIMEOUT},
  {"perdio.seifHandler",PROP_PERDIO_SEIFHANDLER},
  {"perdio.tempRetryCeiling",PROP_PERDIO_TEMPRETRYCEILING},
  {"perdio.tempRetryFloor",PROP_PERDIO_TEMPRETRYFLOOR},
  {"perdio.tempRetryFactor",PROP_PERDIO_TEMPRETRYFACTOR},
  {"perdio.maxTCPCache",PROP_PERDIO_MAXTCPCACHE},
  {"perdio.checkAliveInterval",PROP_PERDIO_CHECKALIVEINTERVAL},
  {"perdio",PROP_PERDIO},
  // DPTABLE
  {"dpTable.defaultOwnerTableSize",
                       PROP_DPTABLE_DEFAULTOWNERTABLESIZE},
  {"dpTable.defaultBorrowTableSize",
                       PROP_DPTABLE_DEFAULTBORROWTABLESIZE},
  {"dpTable.lowLimit", PROP_DPTABLE_LOWLIMIT},
  {"dpTable.expandFactor", PROP_DPTABLE_EXPANDFACTOR},
  {"dpTable.buffer", PROP_DPTABLE_BUFFER},
  {"dpTable.worthwhileRealloc",
                       PROP_DPTABLE_WORTHWHILEREALLOC},
  {"dpTable", PROP_DPTABLE},
  //CLOSE
  {"close.time",PROP_CLOSE_TIME},
  {0,PROP__LAST},
};

void initVirtualProperties()
{
  vprop_registry  = makeTaggedConst(new OzDictionary(oz_rootBoard()));
  system_registry = makeTaggedConst(new OzDictionary(oz_rootBoard()));
  OZ_protect(&vprop_registry);
  OZ_protect(&system_registry);
  // POPULATE THE SYSTEM REGISTRY
  {
    OzDictionary * dict = tagged2Dictionary(system_registry);
    dict->setArg(oz_atomNoDup("oz.home"),oz_atom(ozconf.ozHome));
  }
  for (const struct prop_entry * pe = prop_entries; pe->name; pe++)
    VirtualProperty::add(pe->name,pe->epi);
}
