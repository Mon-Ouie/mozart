/*
 *  Authors:
 *    Michael Mehl (mehl@dfki.de)
 *    Kostja Popow (popow@ps.uni-sb.de)
 * 
 *  Contributors:
 *    Tobias Mueller (tmueller@ps.uni-sb.de)
 * 
 *  Copyright:
 *    Organization or Person (Year(s))
 * 
 *  Last change:
 *    $Date$ by $Author$
 *    $Revision$
 * 
 *  This file is part of Mozart, an implementation 
 *  of Oz 3:
 *     $MOZARTURL$
 * 
 *  See the file "LICENSE" or
 *     $LICENSEURL$
 *  for information on usage and redistribution 
 *  of this file, and for a DISCLAIMER OF ALL 
 *  WARRANTIES.
 *
 */

// interface of threads
// including propagators and LTQ

#ifndef __THREADHH
#define __THREADHH


#ifdef INTERFACE
#pragma interface
#endif

#include "taskstk.hh"
#include "value.hh"

//
// (kp) On Sparc (v8), most efficient flags are (strictly) between 0x0 and 
// 0x1000. Flags up to 0x1000 and above 0x1000 should not be mixed, 
// because then three instructions are required (for testing, i mean);
enum ThreadFlag {
  T_null   = 0x000000,   // no flag is set;
  T_dead   = 0x000001,   // the thread is dead;
  T_runnable=0x000002,   // the thread is runnable;
  T_p_thr  = 0x000004,   // 'real' propagators by Tobias;
  T_stack  = 0x000008,   // it has an (allocated) stack;
  T_catch  = 0x000010,   // has or has had an exception handler
  T_solve  = 0x000020,   // it was created in a search CS
  			 // (former notificationBoard);
  T_ext    = 0x000040,   // an external suspension wrt current search problem;
  T_unif   = 0x000080,   // the thread is due to a (proper) unification 
                         // of fd variables; 
  T_loca   = 0x000100,   // all variables of this propagator are local;
  T_tag    = 0x000200,   // a special stuff for fd (Tobias, please comment?);
  T_ofs    = 0x000400,   // the OFS thread (needed for DynamicArity);

  T_ltq    = 0x000800,   // designates local thread queue
  T_nmo    = 0x001000,   // designates nonmonotonic propagator

  T_noblock= 0x002000,   // if this thread blocks, raise an exception

  // debugger
  T_G_trace= 0x010000,   // thread is being traced
  T_G_step = 0x020000,   // step mode turned on
  T_G_stop = 0x040000,   // no permission to run

  T_max    = 0x800000    // MAXIMAL FLAG;
};


#define  S_TYPE_MASK  (T_stack|T_p_thr)
#define  S_WAKEUP     T_null
#define  S_RTHREAD    T_stack
#define  S_PR_THR     T_p_thr


class RunnableThreadBody {
friend class AM;
friend class Thread;
private:
  TaskStack taskStack;
  RunnableThreadBody *next;  /* for linking in the freelist */

  RunnableThreadBody(int sz) : taskStack(sz) {}
public:
  //  gc methods;
  RunnableThreadBody *gcRTBody();
  USEHEAPMEMORY;
  NO_DEFAULT_CONSTRUCTORS(RunnableThreadBody);

  void reInit() {		// for the root thread only;
    taskStack.init();
  }
};

union ThreadBodyItem {
  OZ_Propagator *propagator;
  RunnableThreadBody *threadBody;
};  


//  Every thread can be in the following states - 
// suspended, runnable, running and dead:
//
//  Moreover, only the following transactions are possible:
//
//                    .------------> dead <-----------.
//                    |                               |
//                    |                               |
//  (create) ---> suspended -----> runnable <----> running
//                    ^                               |
//                    |                               |
//                    `-------------------------------'
//
// memory layout
// <Secondary Tags>               from class ConstTerm
// <board|index> | <Tertiary Tag> from class Tertiary
// <prio> | <flags>
// <id>    (debugger)
// <abstr> (profiler)
// <stack>

class Thread : public Tertiary {
  friend int engine(Bool);
  friend void scheduler();
  friend void ConstTerm::gcConstRecurse(void);
private:
  //  Sparc, for instance, has a ldsb/stb instructions - 
  // so, this is exactly as efficient as just two integers;
  Object *self;
  struct {
    int pri:    sizeof(char) * 8;
    int flags:  (sizeof(int) - sizeof(char)) * sizeof(char) * 8;
  } state;

  unsigned int id;              // unique identity for debugging
  PrTabEntry *abstr;            // for profiler
  ThreadBodyItem item;		// NULL if it's a deep 'unify' suspension;
public:
  NO_DEFAULT_CONSTRUCTORS(Thread);

  Thread(int flags, int prio, Board *bb, int id1)
    : Tertiary(bb,Co_Thread,Te_Local), id(id1)
  {
    state.flags = flags;
    state.pri = prio;

    item.threadBody = 0;

    setAbstr(NULL);
    setSelf(NULL);

    if (flags & T_stack)
      ozstat.createdThreads.incf();
  }

  Thread(int i, TertType tertType)
    : Tertiary(0,Co_Thread,tertType)
  {
    setIndex(i);
  }

  USEHEAPMEMORY;
  OZPRINTLONG;

  Thread *gcThread();
  Thread *gcThreadInline();
  Thread *gcDeadThread();
  void gcRecurse();

  void freeThreadBodyInternal() {
    Assert(isDeadThread());
    item.threadBody = 0;
  }

  void freePropBody() {
    delete item.propagator;
    item.propagator = 0;
  }

  void markDeadThread() { 
    state.flags = state.flags | T_dead;
  }

  int getFlags() { return state.flags; }

  void markPropagatorThread () { 
    Assert(!isDeadThread());
    state.flags = state.flags | T_p_thr;
  }
  void unmarkPropagatorThread() { 
    Assert (!isDeadThread());
    state.flags &= ~T_p_thr;
  }
  Bool isPropagator() { 
    Assert(!isDeadThread());
    return state.flags & T_p_thr;
  }

  Bool isWakeup() { 
    Assert(!isDeadThread());
    return getThrType() == S_WAKEUP;
  }
  
  void setBody(RunnableThreadBody *rb) { item.threadBody=rb; }
  RunnableThreadBody *getBody()        { return item.threadBody; }

  void setInitialPropagator(OZ_Propagator * pro) { 
    item.propagator = pro; 
    state.flags = T_p_thr | T_runnable | T_unif;
    if (!pro->isMonotonic()) markNonMonotonicPropagatorThread();
  }

  unsigned int getID() { return id; }
  void setID(unsigned int newId) { id = newId; }
  
  void setAbstr(PrTabEntry *a) { abstr = a; }
  PrTabEntry *getAbstr()       { return abstr; }

  void setSelf(Object *o) { self = o; }
  Object *getSelf()       { return self; }

  int getPriority() { 
    Assert(state.pri >= OZMIN_PRIORITY && state.pri <= OZMAX_PRIORITY);
    return state.pri;
  }
  void setPriority(int newPri) { 
    Assert(state.pri >= OZMIN_PRIORITY && state.pri <= OZMAX_PRIORITY);
    state.pri = newPri;
  }

  /* check if thread has a stack */
  Bool isRThread() { return (state.flags & S_TYPE_MASK) == S_RTHREAD; }

  Bool isDeadThread() { return state.flags & T_dead; }

  Bool isSuspended() { 
    Assert(!isDeadThread());
    return !(state.flags & T_runnable);
  }
  Bool isRunnable() { 
    Assert(!isDeadThread());
    return state.flags & T_runnable;
  }

  //  For reinitialisation; 
  void setRunnable() { 
    state.flags = (state.flags & ~T_dead) | T_runnable;
  }

  Bool isInSolve() { 
    Assert (!isDeadThread());
    return state.flags & T_solve;
  }
  void setInSolve() { 
    Assert(isRunnable());
    state.flags =  state.flags | T_solve;
  }

  //  non-runnable threads;
  void markRunnable() {
    Assert(isSuspended() && !isDeadThread());
    state.flags = state.flags | T_runnable;
  }
  void unmarkRunnable() { 
    Assert((isRunnable () && !isDeadThread ()) || getStop());
    state.flags &= ~T_runnable;
  }

  void setExtThread() { 
    Assert (!isDeadThread());
    state.flags = state.flags | T_ext;
  }
  Bool isExtThread() { 
    Assert(isRunnable());
    return state.flags & T_ext;
  }
  void clearExtThread() {
    state.flags &= ~T_ext;
  }

  Bool wasExtThread() {
    return state.flags & T_ext;
  }

  void markNonMonotonicPropagatorThread(void) { 
    Assert(!isDeadThread() && isPropagator());
    state.flags = state.flags | T_nmo;
  }
  void unmarkNonMonotonicPropagatorThread(void) { 
    Assert(!isDeadThread() && isPropagator());
    state.flags &= ~T_nmo;
  }
  Bool isNonMonotonicPropagatorThread(void) { 
    Assert(!isDeadThread() && isPropagator());
    return state.flags & T_nmo;
  }
  
  void setNoBlock(Bool yesno) {
    state.flags = yesno ? state.flags | T_noblock : state.flags & ~T_noblock;
  }
  Bool getNoBlock() {
    return (state.flags & T_noblock);
  }

  // source level debugger
  // set/delete some bits...
  void setTrace(Bool yesno) {
    state.flags = yesno ? state.flags | T_G_trace : state.flags & ~T_G_trace;
  }
  void setStep(Bool yesno) {
    state.flags = yesno ? state.flags | T_G_step  : state.flags & ~T_G_step;
  }
  void setStop(Bool yesno) {
    state.flags = yesno ? state.flags | T_G_stop  : state.flags & ~T_G_stop;
  }

  // ...and check them
  Bool getTrace() { return (state.flags & T_G_trace); }
  Bool getStep()  { return (state.flags & T_G_step); }
  Bool getStop()  { return (state.flags & T_G_stop); }

  
  int getThrType() { return (state.flags & S_TYPE_MASK); }

  // 
  //  Convert a thread of any type to a 'wakeup' thread (without a stack).
  //  That's used in GC because thread with a dead home board
  // might not dissappear during GC, but moved to a first alive board,
  // and killed in the emulator.
  void setWakeUpTypeGC() {
    state.flags = (state.flags & ~S_TYPE_MASK) | S_WAKEUP;
  }

  void markUnifyThread() { 
    Assert(isPropagator () && !isDeadThread());
    state.flags = state.flags | T_unif;
  }
  void unmarkUnifyThread () { 
    Assert(isPropagator() && !isDeadThread());
    state.flags = state.flags & ~T_unif;
  }
  Bool isUnifyThread() {
    Assert(isPropagator() && !isDeadThread());
    return (state.flags & T_unif);
  }

  void setOFSThread() {
    Assert((isPropagator ()) && !(isDeadThread ()));
    state.flags = state.flags | T_ofs;
  }
  Bool isOFSThread() {
    Assert(!isDeadThread());
    return (state.flags & T_ofs);
  }

  // the following six member functions operate on dead threads too
  void markLocalThread() {
    if (isDeadThread()) return;
    state.flags = state.flags | T_loca;
  }
  void unmarkLocalThread() {
    state.flags = state.flags & ~T_loca;
  }
  Bool isLocalThread() {
    return (state.flags & T_loca);
  }

  void markTagged() { 
    if (isDeadThread ()) return;
    state.flags = state.flags | T_tag;
  }
  void unmarkTagged() { 
    state.flags = state.flags & ~T_tag;
  }
  Bool isTagged() { 
    return (state.flags & T_tag);
  }

  Bool hasStack() { 
    Assert(!isDeadThread());
    return (state.flags & T_stack);
  }
  Bool hadStack() { 
    Assert(isDeadThread ());
    return (state.flags & T_stack);
  }
  void setHasStack() { 
    Assert(isRunnable());
    Assert(!isPropagator());
    state.flags = state.flags | T_stack; 
  }

  void reInit() {  // for the root thread only;
    setRunnable();
    item.threadBody->reInit();
  }

  TaggedRef getStreamTail();
  void setStreamTail(TaggedRef v);

  OZ_Propagator * swapPropagator(OZ_Propagator * p) {
    OZ_Propagator * r = item.propagator;
    item.propagator = p;
    return r;
  }

  void setPropagator(OZ_Propagator * p) {
    Assert(isPropagator());
    delete item.propagator; 
    item.propagator = p;
  }

  OZ_Propagator * getPropagator(void) {
    Assert(isPropagator()); 
    return item.propagator;
  }
  
  void pushLTQ(Board * sb) {
    item.threadBody->taskStack.pushLTQ(sb);
  }
  void pushDebug(OzDebug *dbg, OzDebugDoit dothis) {
    item.threadBody->taskStack.pushDebug(dbg,dothis);
  }
  void popDebug(OzDebug *&dbg, OzDebugDoit &dothis) {
    PopFrame(&item.threadBody->taskStack,pc,y,cap);
    if (pc == C_DEBUG_CONT_Ptr) {
      dbg = (OzDebug *) y;
      dothis = (OzDebugDoit) (int) cap;
    } else {
      item.threadBody->taskStack.restoreFrame();
      dbg = (OzDebug *) NULL;
      dothis = DBG_EXIT;
    }
  }
  void pushCall(TaggedRef pred, TaggedRef arg0=0, TaggedRef arg1=0, 
		TaggedRef arg2=0, TaggedRef arg3=0, TaggedRef arg4=0)
  {
    item.threadBody->taskStack.pushCall(pred, arg0,arg1,arg2,arg3,arg4);
  }

  void pushCall(TaggedRef pred, RefsArray  x, int n) {
    item.threadBody->taskStack.pushCall(pred, x, n);
  }
  void pushCallNoCopy(TaggedRef pred, RefsArray  x) {
    item.threadBody->taskStack.pushCallNoCopy(pred, x);
  }
  void pushCFun(OZ_CFun f, RefsArray  x, int n, Bool copyF) {
    item.threadBody->taskStack.pushCFun(f, x, n, copyF);
  }

  Bool hasCatchFlag() { return (state.flags & T_catch); }
  void setCatchFlag() { state.flags = state.flags|T_catch; }
  void pushCatch() {
    setCatchFlag();
    item.threadBody->taskStack.pushCatch();
  }

  Bool isEmpty() {
    return hasStack() ? item.threadBody->taskStack.isEmpty() : NO;
  }

  void printTaskStack(int depth) {
    if (!isDeadThread() && hasStack()) {
      item.threadBody->taskStack.printTaskStack(depth);
    } else {
      message("\tEMPTY\n");
      message("\n");
    }
  }


  TaskStack *getTaskStackRef() {
    Assert(hasStack());
    return &(item.threadBody->taskStack);
  }


  DebugCode 
  (void removePropagator () { 
    Assert (isPropagator ());
    item.propagator = (OZ_Propagator *) NULL;
  })

  int getRunnableNumber();
};

inline 
Bool oz_isThread(TaggedRef term)
{
  return oz_isConst(term) && tagged2Const(term)->getType() == Co_Thread;
}

inline
Thread *tagged2Thread(TaggedRef term)
{
  Assert(oz_isThread(term));
  return (Thread *) tagged2Const(term);
}

#endif
