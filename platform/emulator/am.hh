/*
  Hydra Project, DFKI Saarbruecken,
  Stuhlsatzenhausweg 3, D-66123 Saarbruecken, Phone (+49) 681 302-5312
  Author: popow, mehl
  Last modified: $Date$ from $Author$
  Version: $Revision$
  State: $State$

  ------------------------------------------------------------------------
*/

#ifndef __AMH
#define __AMH

#ifdef __GNUC__
#pragma interface
#endif

#include <setjmp.h>

#include "tagged.hh"
#include "term.hh"
#include "constter.hh"

#include "actor.hh"
#include "board.hh"

#include "hashtbl.hh"

#include "suspensi.hh"
#include "variable.hh"

#include "opcodes.hh"
# include "codearea.hh"

#include "statisti.hh"

#include "stack.hh"
#  include "taskstk.hh"

#include "trail.hh"
#include "bignum.hh"

#include "records.hh"

#include "builtins.hh"
#include "compiler.hh"
#include "debug.hh"
#include "os.hh"
#include "thread.hh"
#include "thrspool.hh"
#include "verbose.hh"
#include "cell.hh"
#include "objects.hh"

#include "dllstack.hh"
# include "solve.hh"

// -----------------------------------------------------------------------

typedef enum {
  ThreadSwitch  = 1 << 2, // choose a new process
  IOReady       = 1 << 3, // IO handler has signaled IO ready
  UserAlarm     = 1 << 4, // Alarm handler has signaled User Alarm
  StartGC       = 1 << 5, // need a GC
  DebugMode     = 1 << 6
} StatusBit;


// isBetween returns
enum BFlag {
  B_BETWEEN,
  B_NOT_BETWEEN,
  B_DEAD
};

enum JumpReturns {
  NOEXCEPTION = 0,
  SEGVIO = 1,
  BUSERROR = 2
};

enum InstType {
  INST_OK,
  INST_FAILED,
  INST_REJECTED
};

// this class contains the central global data
class AM : public ThreadsPool {
friend void engine();
public:
  int threadSwitchCounter;
  int userCounter;

  int statusReg;
  Trail trail;
  RebindTrail rebindTrail;
  RefsArray xRegs;
  RefsArray toplevelVars;

  Board *currentBoard;
  TaggedRef currentUVarPrototype; // opt: cache
  Board *rootBoard;

  Board *currentSolveBoard;       // current 'solve' board or NULL if none;
  Bool wasSolveSet;

  CompStream *compStream;
  Bool isStandaloneF;
  Bool isStandalone() { return isStandaloneF; }

  jmp_buf engineEnvironment;

  TaggedRef *ioNodes;           // node that must be waked up on io

#ifdef DEBUG_CHECK
  Bool dontPropagate;
  // is used by consistency checking of a copy of a search tree;
#endif

  TaggedRef suspCallHandler;
  TaggedRef suspendVarList;
  void emptySuspendVarList(void) { suspendVarList = makeTaggedNULL(); }
  void addSuspendVarList(TaggedRef * t);
  void suspendOnVarList(Suspension *susp);

  void suspendInline(Board *bb,int prio,OZ_CFun fun,int n,
                     OZ_Term A,OZ_Term B=makeTaggedNULL(),
                     OZ_Term C=makeTaggedNULL(),OZ_Term D=makeTaggedNULL());

  Toplevel *toplevelQueue;

  void printBoards();

  //
  //  schedule various kinds of jobs;
  void scheduleSuspCont(Board *bb, int prio, Continuation *c,
                        Bool wasExtSusp);
  void scheduleSuspCCont(Board *bb, int prio,
                         CFuncContinuation *c, Bool wasExtSusp,
                         Suspension *s=0);
  void scheduleSolve(Board *b);
  void scheduleWakeup(Board *b, Bool wasExtSusp);

  void pushToplevel(ProgramCounter pc);
  void checkToplevel();
  void addToplevel(ProgramCounter pc);
  Thread *createThread(int prio);

  int catchError() { return setjmp(engineEnvironment); }
public:
  AM() {};
  void init(int argc,char **argv);
  void checkVersion();
  void exitOz(int status);
  void suspendEngine();

  Bool criticalFlag;  // if this is true we will NOT set Sflags
                      // from within signal handlers

  Bool isCritical() { return criticalFlag; }

  void setSFlag(StatusBit flag)
  {
    criticalFlag = OK;
    statusReg = (flag | statusReg);
    criticalFlag = NO;
  }

  void unsetSFlag(StatusBit flag)
  {
    criticalFlag = OK;
    statusReg = (~flag & statusReg);
    criticalFlag = NO;
  }

  Bool isSetSFlag(StatusBit flag) { return ( statusReg & flag ) ? OK : NO; }
  Bool isSetSFlag() { return statusReg ? OK : NO; }

  void print();

  void setCurrent(Board *c, Bool checkNotGC=OK);
  InstType installPath(Board *to); // ###
  Bool installScript(Script &script);
  Bool install(Board *bb);
  void deinstallPath(Board *top);
  void deinstallCurrent();
  void reduceTrailOnUnitCommit();
  void reduceTrailOnSuspend();
  void reduceTrailOnFail();
  void reduceTrailOnShallow(int numbOfCons);

  // in emulate.cc
  Bool emulateHookOutline(Abstraction *def=NULL,
                          int arity=0, TaggedRef *arguments=NULL);
  Bool hookCheckNeeded();
  Suspension *mkSuspension(Board *b, int prio, ProgramCounter PC,
                           RefsArray Y, RefsArray G,
                           RefsArray X, int argsToSave);
  Suspension *mkSuspension(Board *b, int prio, OZ_CFun bi,
                           RefsArray X, int argsToSave);
  TaggedRef createNamedVariable(int regIndex, TaggedRef name);
  void handleToplevelBlocking(ProgramCounter PC);


  Bool isToplevel();

  void gc(int msgLevel);  // ###
  void doGC();
  Bool idleGC();
// coping of trees (and terms);
  Board* copyTree (Board* node, Bool *isGround);

  void awakeIOVar(TaggedRef var);

  // entailment check
  Bool entailment();
  Bool isEmptyTrailChunk();
  int checkEntailment(Continuation *&contAfter,int &prio);
  int checkStable(Continuation *&contAfter,int &prio);  // mm2 todo

  // Unification
  Bool unify(TaggedRef ref1, TaggedRef ref2, Bool prop = OK);
  Bool fastUnify(TaggedRef ref1, TaggedRef ref2, Bool prop);
  Bool fastUnifyOutline(TaggedRef ref1, TaggedRef ref2, Bool prop);
  Bool performUnify(TaggedRef *termPtr1, TaggedRef *termPtr2, Bool prop);
  void bindToNonvar(TaggedRef *varPtr, TaggedRef var, TaggedRef term, Bool prop);

  void rebind(TaggedRef *ref, TaggedRef ptr);
  void doBindAndTrail(TaggedRef v, TaggedRef * vp, TaggedRef t);
  void doBindAndTrailAndIP(TaggedRef v, TaggedRef * vp, TaggedRef t,
                               GenCVariable * lv, GenCVariable * gv,
                               Bool prop);

  Bool isLocalUVar(TaggedRef var);
  Bool isLocalSVar(TaggedRef var);
  Bool isLocalSVar(SVariable *var);
  Bool isLocalCVar(TaggedRef var);
  Bool isLocalVariable(TaggedRef var);

  void pushCall(Board *b, Chunk *def, int arity, RefsArray args);
  void pushDebug(Board *n, Chunk *def, int arity, RefsArray args);
  void pushTask(Board *n,ProgramCounter pc,
                RefsArray y,RefsArray g,RefsArray x=0,int i=0);
  void pushTaskOutline(Board *n,ProgramCounter pc,
                       RefsArray y,RefsArray g,RefsArray x=0,int i=0);
  void pushCFun(Board *n, OZ_CFun f, RefsArray x=0, int i=0);
  void pushNervous(Board *n);

  void genericBind(TaggedRef *varPtr, TaggedRef var,
                   TaggedRef *termPtr, TaggedRef term, Bool prop);
  void bind(TaggedRef *varPtr, TaggedRef var, TaggedRef *termPtr, Bool prop);
  void checkSuspensionList(TaggedRef taggedvar,
                           PropCaller calledBy = pc_propagator);
  Bool hasOFSSuspension(SuspList *suspList);
  void addFeatOFSSuspensionList(TaggedRef var, SuspList* suspList,
                                TaggedRef flist, Bool determined);
  SuspList * checkSuspensionList(SVariable * var, TaggedRef taggedvar,
                                 SuspList * suspList, PropCaller calledBy);
  BFlag isBetween(Board * to, Board * varHome);
  void setExtSuspension (Board *varHome, Suspension *susp);
private:
  Bool _checkExtSuspension(Suspension * susp);
public:
  Bool checkExtSuspension(Suspension * susp) {
    if (susp->isExtSusp())
      return _checkExtSuspension(susp);
    return NO;
  }
  void incSolveThreads (Board *bb,int n=1);
  void decSolveThreads (Board *bb);
  Board *findStableSolve(Board *bb);

// debugging --> see file ../builtins/debug.C
  State getValue(TaggedRef feature, TaggedRef out);
  State setValue(TaggedRef feature, TaggedRef value);

  void restartThread();

  void handleIO();
  Bool loadQuery(CompStream *fd);
  OZ_Bool readSelect(int fd,TaggedRef l,TaggedRef r);
  void checkIO();

  void handleAlarm();
  void handleUser();
  int setUserAlarmTimer(int ms);

  Sleep *sleepQueue;
  void insertUser(int t,TaggedRef node);
  int wakeUser();

  static OZ_Bool SolveActorWaker(int n, TaggedRef *args);
  Bool isStableSolve(SolveActor *sa);

};

extern AM am;

#ifdef OUTLINE
void updateExtSuspension(Board *varHome, Suspension *susp);
#else
#include "am.icc"
#endif

#endif
