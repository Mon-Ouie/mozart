/*
  Hydra Project, DFKI Saarbruecken,
  Stuhlsatzenhausweg 3, D-66123 Saarbruecken, Phone (+49) 681 302-5312
  Author: mehl
  Last modified: $Date$ from $Author$
  Version: $Revision$
  State: $State$

  implementation of threads and queues of threads with priorities
  ------------------------------------------------------------------------
*/

#if defined(INTERFACE) && !defined(PEANUTS)
#pragma implementation "thread.hh"
#endif

/* some random comments on threads:

   threads are 'switch'ed, if
     - 'timed out' by alarm clock
     - 'finish'ed
     - 'displace'd by higher priority
     - 'freeze'd

   need for thread switching is signaled by 'ThreadSwitch' S-Flag

   "current thread"
     - the currently running thread

   */

#include "am.hh"

#ifdef OUTLINE
#define inline
#include "thread.icc"
#undef inline
#endif

// --------------------------------------------------------------------------

/*
 *  Threads;
 *
 */
void Thread::setExtThreadOutlined (Board *varHome)
{
  Board *bb = am.currentBoard;
  Bool wasFound = NO;
  Assert (!(varHome->isCommitted ()));

  while (bb != varHome) {
    Assert (!(bb->isRoot ()));
    Assert (!(bb->isCommitted ()) && !(bb->isFailed ()));
    if (bb->isSolve ()) {
      SolveActor *sa = SolveActor::Cast (bb->getActor ());
      sa->addSuspension (this);
      wasFound = OK;
    }
    bb = bb->getParent();
  }

  if (wasFound) {
    setExtThread ();

    //  ... and again to the 'SolveActor::checkExtSuspList':
    // we should not produce 'external' threads for propagators,
    // because stability can be achieved only when the last such 
    // a propagator dissapears;
    if (isPropagator()) {
      warning ("'External' propagator thread: "
	       "stability check is implemented *partially*!");
      warning ("(Explanation: no global variables may occur in propagators within a search problem)");
    }
  }
}

//
void Thread::checkExtThreadOutlined ()
{
  Assert (wasExtThread ());
  
  Board *sb = (getBoard())->getSolveBoard ();
  AM *e = &am;
  
  while (sb) {
    Assert (sb->isSolve());
    
    SolveActor *sa = SolveActor::Cast (sb->getActor ());
    if (e->isStableSolve (sa)) {
      e->scheduleThread (e->mkRunnableThread (sa->getPriority (),sb));
    }
    sb = (sa->getBoard())->getSolveBoard ();
  }
}

//
void Thread::removeExtThreadOutlined ()
{
  Assert (wasExtThread ());
  
  Board *sb = (getBoard())->getSolveBoard ();
  AM *e = &am;
  
  while (sb) {
    Assert (sb->isSolve());
    
    SolveActor *sa = SolveActor::Cast (sb->getActor ());
    sa->clearSuspList(this);
    sb = (sa->getBoard())->getSolveBoard ();
  }
}

/*
 * check if a thread's board is below a failed board
 */
Bool Thread::isBelowFailed (Board *top)
{
  Assert (isRunnable ());

  Board *bb=getBoard();
  while (bb!=top) {
    if (bb->isFailed()) {
      return TRUE;
    }
    bb=bb->getParent();
  }
  return FALSE;
}

/*
 * terminate a thread
 *  if not in deep guard: discard all tasks, return NO
 *  if in deep guard: don't discard any task, return OK
 */
Bool Thread::terminate()
{
  TaskStack *ts;
  TaskStackEntry *tos;
  Assert(hasStack());

  ts = &(item.threadBody->taskStack);
  tos = ts->getTop ();
  while (TRUE) {
    TaskStackEntry entry=*(--tos);
    if (ts->isEmpty (entry)) {
      ts->setTop(tos+1);
      return NO;
    }

    ContFlag cFlag = getContFlag(ToInt32(entry));

    switch (cFlag) {
    case C_ACTOR:
      return OK;

    default:
      tos = tos - ts->frameSize(cFlag) + 1;
      break;
    }
  }
}

void Thread::propagatorToNormal()
{
  Assert(isPropagator());
  delete item.propagator;
  state.flags &= ~S_PR_THR;

  setBody(am.allocateBody());
  state.flags |= S_RTHREAD;
}

int Thread::getRunnableNumber()
{
  switch (getThrType()) {
  case S_RTHREAD:
    {
      TaskStack * taskstack = getTaskStackRef();
      if (taskstack->isEmpty()) return 0;
      TaskStackEntry * topCache = taskstack->getTop();
      TaskStackEntry topElem = TaskStackPop(topCache-1);
      int cFlag   = getContFlag (ToInt32 (topElem));
      if (cFlag != C_LTQ) return 1;

      topElem = TaskStackPop(topCache-2);
      SolveActor *sa = (SolveActor *) topElem;
      ThreadQueueImpl *ltq = sa->getLocalThreadQueue();
      return ltq->getSize();
    }
  case S_WAKEUP:
    return 0;
  case S_PR_THR:
    return 1;
  default:
    error("Thread::getRunnableNumber()");
    return 0;
  }
}
