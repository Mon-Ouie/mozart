/*
 *  Authors:
 *    Kostja Popow (popow@ps.uni-sb.de)
 *    Michael Mehl (mehl@dfki.de)
 *    Ralf Scheidhauer (Ralf.Scheidhauer@ps.uni-sb.de)
 *
 *  Contributors:
 *    Benjamin Lorenz (lorenz@ps.uni-sb.de)
 *    Leif Kornstaedt (kornstae@ps.uni-sb.de)
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
 *     http://www.mozart-oz.org
 *
 *  See the file "LICENSE" or
 *     http://www.mozart-oz.org/LICENSE.html
 *  for information on usage and redistribution
 *  of this file, and for a DISCLAIMER OF ALL
 *  WARRANTIES.
 *
 */

#include "value.hh"
#include "trace.hh"
#include "thr_int.hh"
#include "prop_int.hh"
#include "builtins.hh"
#include "debug.hh"
#include "space.hh"

# define CBB (oz_currentBoard())
# define CTT (oz_currentThread())


static
TaggedRef formatError(TaggedRef info,TaggedRef val,
                      OZ_Term traceBack,OZ_Term loc)
{
  OZ_Term d = OZ_record(OZ_atom("d"),
                        oz_cons(OZ_atom("info"),
                             oz_cons(OZ_atom("stack"),
                                  oz_cons(OZ_atom("loc"),
                                       oz_nil()))));
  OZ_putSubtree(d,OZ_atom("stack"),traceBack);
  OZ_putSubtree(d,OZ_atom("loc"),loc);
  OZ_putSubtree(d,OZ_atom("info"),info);

  return OZ_adjoinAt(val,OZ_atom("debug"),d);
}

// check if failure has to be raised as exception on thread
static
int canOptimizeFailure(Thread *tt)
{
  if (tt->hasCatchFlag() || oz_onToplevel()) { // catch failure
    if (tt->isSuspended()) {
      tt->pushCall(BI_fail,0,0);
      oz_wakeupThread(tt);
    } else {
      printf("WEIRD: failure detected twice");
#ifdef DEBUG_CHECK
      PopFrame(tt->getTaskStackRef(),PC,Y,G);
      Assert(PC==C_CALL_CONT_Ptr);
      Assert((TaggedRef)ToInt32(Y)==BI_fail);
      tt->pushCall(BI_fail,0,0);
#endif
    }
    return NO;
  } else {
    return OK;
  }
}

void scheduler() {
  register AM * const e = &am;
  goto LBLstart;

  /* -----------------------------------------------------------------------
   * Get thread from queue
   * ----------------------------------------------------------------------- */
LBLstart:
  //  Assert(CTT==0); // TMUELLER

  // check status register
  e->checkStatus(OK);

  if (am.threadsPool.threadQueuesAreEmpty()) {
    e->suspendEngine();
  }
  am.threadsPool.setCurrentThread(am.threadsPool.getFirstThread());
  Assert(CTT);

  DebugTrace(ozd_trace("run thread"));

  // source level debugger & Thread.suspend
  if (CTT->getStop()) {
    am.threadsPool.unsetCurrentThread();  // byebye...
    goto LBLstart;
  }

  //  Every runnable thread must be terminated through
  // 'LBL{discard,kill}Thread', and it should not appear
  // more than once in the threads pool;
  Assert(!CTT->isDeadThread() && CTT->isRunnable());

  // Install board
  switch (oz_installPath(GETBOARD(CTT))) {
  case INST_OK:
    break;
  case INST_REJECTED:
    goto LBLdiscardThread;
  case INST_FAILED:
    goto LBLfailure;
  }

  Assert(CTT->isRunnable());

  e->restartThread(); // start a new time slice
  // fall through

  /* -----------------------------------------------------------------------
   * Running a thread
   * ----------------------------------------------------------------------- */
LBLrunThread:
  {

    ozstat.leaveCall(CTT->abstr);
    CTT->abstr = 0;
    e->cachedStack = CTT->getTaskStackRef();
    e->cachedSelf  = CTT->getSelf();
    CTT->setSelf(0);

    int ret=engine(NO);

    CTT->setAbstr(ozstat.currAbstr);
    ozstat.leaveCall(NULL);
    e->saveSelf();
    Assert(!e->cachedSelf);

    switch (ret) {
    case T_PREEMPT:
      goto LBLpreemption;
    case T_SUSPEND:
      goto LBLsuspend;
    case T_RAISE:
      goto LBLraise;
    case T_TERMINATE:
      goto LBLterminate;
    case T_FAILURE:
      goto LBLfailure;
    case T_ERROR:
    default:
      goto LBLerror;
    }
  }


  /* -----------------------------------------------------------------------
   * preemption
   * ----------------------------------------------------------------------- */
LBLpreemption:
  DebugTrace(ozd_trace("thread preempted"));
  Assert(GETBOARD(CTT)==CBB);
  /*  Assert(CTT->isRunnable()|| (CTT->isStopped())); ATTENTION */
  am.threadsPool.scheduleThreadInline(CTT, CTT->getPriority());
  am.threadsPool.unsetCurrentThread();
  goto LBLstart;


  /* -----------------------------------------------------------------------
   * An error occured
   * ----------------------------------------------------------------------- */

LBLerror:
  fprintf(stderr,"scheduler: An error has occurred.\n");
  goto LBLstart;


  /* -----------------------------------------------------------------------
   * Thread is terminated
   * ----------------------------------------------------------------------- */
  /*
   *  Kill the thread - decrement 'suspCounter'"s and counters of
   * runnable threads in solve if any
   */
LBLterminate:
  {
    DebugTrace(ozd_trace("thread terminated"));
    Assert(CTT);
    Assert(!CTT->isDeadThread());
    Assert(CTT->isRunnable());
    Assert(CTT->isEmpty());

    //  Note that during debugging the thread does not carry
    // the board pointer (== NULL) wenn it's running;
    // Assert (CBB == CTT->getBoard());

    Assert(CBB != (Board *) NULL);
    Assert(!CBB->isFailed());

    CBB->decSuspCount();

    oz_disposeThread(CTT);
    //am.threadsPool.unsetCurrentThread(); // TMUELLER

    // fall through to checkEntailmentAndStability
  }

  /* -----------------------------------------------------------------------
   * Check entailment and stability
   * ----------------------------------------------------------------------- */

LBLcheckEntailmentAndStability:
  {
    /*     *  General comment - about checking for stability:
     *  In the case when the thread was originated in a solve board,
     * we have to update the (runnable) threads counter there manually,
     * check stability there ('oz_checkStability ()'), and proceed
     * with further solve upstairs by means of
     * 'AM::decSolveThreads ()' as usually.
     *  This is because the 'AM::decSolveThreads ()' just generates
     * wakeups for solve boards where stability is suspected. But
     * finally the stability check for them should be performed,
     * and this (and 'LBLsuspendThread') labels are exactly the
     * right places where it should be done!
     *  Note also that the order of decrementing (runnable) threads
     * counters in solve  is also essential: if some solve
     * can be reduced, solve above it are getting *instable*
     * because of a new thread!
     *
     */

    // maybe optimize?
    if (oz_onToplevel())
      goto LBLstart;

    //  First, look at the current board, and if it's a solve one,
    // decrement the (runnable) threads counter manually _and_
    // skip the 'AM::decSolveThreads ()' for it;

    Assert(!CBB->isRoot() && !CBB->isFailed() && !CBB->isCommitted());

    //  'nb' points to some board above the current one,
    Board * nb = CBB->getParent();

    //  kost@ : optimize the most probable case!

    if (CBB->decThreads () != 0) {
      nb->decSolveThreads();
      goto LBLstart;
    }

    //
    //  ... and now, check the entailment here!
    //  Note again that 'decSolveThreads' should be done from
    // the 'nb' board which is probably modified above!
    //
    DebugCode(am.threadsPool.unsetCurrentThread());

    Assert(!CBB->isRoot());

    CBB->checkStability();

    //  deref nb, because it maybe just committed!

    Assert(nb);

    if (nb)
      nb->derefBoard()->decSolveThreads();

    goto LBLstart;
  }

  /* -----------------------------------------------------------------------
   * Discard Thread
   * ----------------------------------------------------------------------- */

  /*
   *  Discard the thread, i.e. just decrement solve thread counters
   * everywhere it is needed, and dispose the body;
   *  The main difference to the 'LBLterminateThread' is that no
   * entailment can be reached here, because it's tested already
   * when the failure was processed;
   *
   *  Invariants:
   *  - a runnable thread must be there;
   *  - the task stack must be empty (for proper ones), or
   *    it must be already marked as not having the propagator
   *    (in dedug mode, for propagators);
   *  - the home board of the thread must be failed;
   *
   */
LBLdiscardThread:
  {
    Assert(CTT);
    Assert(!CTT->isDeadThread());
    Assert(CTT->isRunnable());

    Board *tmpBB = GETBOARD(CTT);

    if (!tmpBB->isRoot())
      tmpBB->getParent()->decSolveThreads();

    oz_disposeThread(CTT);
    am.threadsPool.unsetCurrentThread();

    goto LBLstart;
  }

  /* -----------------------------------------------------------------------
   * Suspend Thread
   * ----------------------------------------------------------------------- */

  /*
   *  Suspend the thread in a generic way. It's used when something
   * suspends in the sequential mode;
   *  Note that the thread stack should already contain the
   * "suspended" task;
   *
   *  Note that this code might be entered not only from within
   * the toplevel, so, it has to handle everything correctly ...
   *   *  Invariants:
   *  - CBB must be alive;
   *
   */
LBLsuspend:
  {
    DebugTrace(ozd_trace("thread suspended"));
    if (e->debugmode() && CTT->getTrace()) {
      debugStreamBlocked(CTT);
    }

    CTT->unmarkRunnable();

    Assert(CBB);
    Assert(!CBB->isFailed());

    //  First, set the board and self, and perform special action for
    // the case of blocking the root thread;
    Assert(GETBOARD(CTT)==CBB);

    am.threadsPool.unsetCurrentThread();

    //  No counter decrement 'cause the thread is still alive!

    if (oz_onToplevel()) {
      //
      //  Note that in this case no (runnable) thread counters
      // in solve can be affected, just because this is
      // a top-level thread;
      goto LBLstart;
    }

    //
    //  So, from now it's somewhere in a deep guard;
    Assert (!oz_onToplevel());

    goto LBLcheckEntailmentAndStability;
  }

  /* -----------------------------------------------------------------------
   * Fail Thread
   * ----------------------------------------------------------------------- */
  /*
   *  kost@ : There are now the following invariants:
   *  - Can be entered only in a deep guard;
   *  - current thread must be runnable.
   */
LBLfailure:
   {
     DebugTrace(ozd_trace("thread failed"));

     Assert(CTT);
     Assert(CTT->isRunnable());

     Board * b = CBB;
     Board * p = b->getParent();

     Assert(!b->isRoot());

     b->setFailed();

     oz_reduceTrailOnFail();

     am.setCurrent(p);

     if (!oz_unify(b->getStatus(),b->genFailed())) { // mm_u
       // this should never happen?
       Assert(0);
     }

     p->decSolveThreads();

     // tmueller: this experimental
#ifdef NAME_PROPAGATORS
     if (!e->isPropagatorLocation()) {
       oz_disposeThread(CTT);
     }
#endif

     am.threadsPool.unsetCurrentThread();

     goto LBLstart;
   }

  /* -----------------------------------------------------------------------
   * Raise exception on thread
   * ----------------------------------------------------------------------- */

LBLraise:
  {
    DebugCode(if (ozconf.stopOnToplevelFailure) {DebugTrace(ozd_tracerOn());});
    DebugTrace(ozd_trace("exception raised"));

    Assert(CTT);

    Bool foundHdl;

    if (e->exception.debug) {
      OZ_Term traceBack;
      foundHdl =
        CTT->getTaskStackRef()->findCatch(CTT,e->exception.pc,
                                          e->exception.y, e->exception.cap,
                                          &traceBack,e->debugmode());

      OZ_Term loc = oz_getLocation(CBB);
      e->exception.value = formatError(e->exception.info,e->exception.value,
                                       traceBack,loc);

    } else {
      foundHdl = CTT->getTaskStackRef()->findCatch(CTT);
    }

    if (foundHdl) {
      if (e->debugmode() && CTT->getTrace())
        debugStreamUpdate(CTT);
      e->xRegs[0] = e->exception.value; // mm2: use pushX
      goto LBLrunThread;  // execute task with no preemption!
    }

    if (!oz_onToplevel() &&
        OZ_eq(OZ_label(e->exception.value),OZ_atom("failure"))) {
      goto LBLfailure;
    }

    if (e->debugmode()) {
      OZ_Term exc = e->exception.value;
      // ignore system(kernel(terminate)) exception:
      if (OZ_isRecord(exc) &&
          OZ_eq(OZ_label(exc),OZ_atom("system")) &&
          OZ_subtree(exc,OZ_int(1)) != makeTaggedNULL() &&
          OZ_eq(OZ_label(OZ_subtree(exc,OZ_int(1))),OZ_atom("kernel")) &&
          OZ_eq(OZ_subtree(OZ_subtree(exc,OZ_int(1)),OZ_int(1)),
                OZ_atom("terminate")))
        ;
      else {
        CTT->setTrace(OK);
        CTT->setStep(OK);
        debugStreamException(CTT,e->exception.value);
        goto LBLpreemption;
      }
    }
    // else
    if (e->defaultExceptionHdl) {
      CTT->pushCall(e->defaultExceptionHdl,e->exception.value);
    } else {
      prefixError();
      fprintf(stderr,"Exception raise:\n   %s\n",toC(e->exception.value));
      fflush(stderr);
    }
    goto LBLrunThread; // changed from LBLpopTaskNoPreempt; -BL 26.3.97
  }
}
