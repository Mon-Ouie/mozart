/*
  Hydra Project, DFKI Saarbruecken,
  Stuhlsatzenhausweg 3, D-66123 Saarbruecken, Phone (+49) 681 302-5312
  Author: mehl
  Last modified: $Date$ from $Author$
  Version: $Revision$
  State: $State$

  ------------------------------------------------------------------------
*/

#ifndef __STATISTICS_H__
#define __STATISTICS_H__

#ifdef INTERFACE
#pragma interface
#endif

#ifdef AM_PROFILE
#   define IncfProfCounter(C,N) ozstat.C += N
#else
#   define IncfProfCounter(C,N)
#endif

#ifdef HEAP_PROFILE
# define INITCOUNT() ozstat.initCount();
# define COUNT(WHAT) ozstat.WHAT += 1
# define COUNT1(WHAT,n) ozstat.WHAT += n
#else
# define INITCOUNT()
# define COUNT(WHAT)
# define COUNT1(WHAT,n)
#endif


class StatCounter {
public:
  unsigned long sinceIdle;
  unsigned long total;
  void reset()  { sinceIdle = total = 0; }
  StatCounter() { reset(); }
  void incf(int n=1) { total+=n; }
  void idle()   { sinceIdle = total; }
  unsigned int sinceidle()   { return total-sinceIdle; }
};


class Statistics {
private:
  unsigned int gcStarttime;
  unsigned int gcStartmem;

public:
  StatCounter gcCollected;
  StatCounter timeForCopy;
  StatCounter timeForLoading;
  StatCounter timeForGC;
  StatCounter timeUtime;

  StatCounter heapUsed; /* total == memory used not including getUsedMemory() */
                        /* sinceIdle == memory reported during last idle */

  // for the solve combinator
  StatCounter solveAlt;
  StatCounter solveClone;
  StatCounter solveSolved;
  StatCounter solveFailed;

  StatCounter propagatorsCreated;
  StatCounter fdvarsCreated;

  Statistics();
  void print(FILE *fd);
  void printIdle(FILE *fd);

  void reset();

  void initGcMsg(int level);
  void printGcMsg(int level);

  OZ_Term getStatistics();
  
#ifdef PROFILE
  /* these should alos use class StatCounter */
  int allocateCounter, deallocateCounter, procCounter,
    waitCounter, askCounter,
    localVariableCounter, protectedCounter;
#endif

  void incSolveAlt(void)    { solveAlt.incf();}
  void incSolveClone(void)  { solveClone.incf();}
  void incSolveSolved(void) { solveSolved.incf(); }
  void incSolveFailed(void) { solveFailed.incf(); }

#ifdef HEAP_PROFILE
  void initCount();
  void printCount();
  long literal;
  long ozfloat;
  long bigInt;
  long scriptLen; // length of all scripts
  long refsArray;
  long refsArrayLen; // length of all refsArrays
  long continuation;
  long suspCFun;
  long suspCont;
  long sTuple;
  long sTupleLen;
  long lTuple;
  long sRecord;
  long sRecordLen;
  long suspension;
  long suspList;
  long uvar;
  long svar;
  long cvar;
  long dynamicTable, dynamicTableLen;
  long taskStack,taskStackLen;
  long cLocal,cJob,cCont,cXCont,cSetCaa,cDebugCont,cExceptHandler;
  long cCallCont, cCFuncCont;
  long abstraction,deepObject,flatObject,cell,chunk;
  long oneCallBuiltin,solvedBuiltin,builtin;
  long heapChunk,thread;
  long board,objectClass;
  long askActor,waitActor,solveActor,waitChild;
  long solveDLLStack;
#endif
};

extern Statistics ozstat;

#endif
