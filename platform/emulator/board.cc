/*
 *  Authors:
 *    Kostja Popow (popow@ps.uni-sb.de)
 *    Michael Mehl (mehl@dfki.de)
 *    Christian Schulte <schulte@ps.uni-sb.de>
 * 
 *  Contributors:
 * 
 *  Copyright:
 *    Kostja Popow, 1997-1999
 *    Michael Mehl, 1997-1999
 *    Christian Schulte, 1997-1999
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

#if defined(INTERFACE)
#pragma implementation "board.hh"
#endif

#include "board.hh"
#include "thr_int.hh"
#include "prop_int.hh"
#include "builtins.hh"
#include "value.hh"
#include "var_base.hh"
#include "var_future.hh"
#include "var_opt.hh"
#include "os.hh"
#include "trail.hh"

#ifdef OUTLINE
#include "board.icc"
#endif

/*
 * Static members
 *
 */

Bool Board::_ignoreWakeUp = NO;

/*
 * Generic operations
 *
 */

Board::Board() 
  : suspCount(0), dist(0),
    crt(0), suspList(0), nonMonoSuspList(0),
    status(taggedVoidValue), rootVar(taggedVoidValue),
    script(taggedVoidValue), parent(NULL), flags(BoTag_Root)
{
  Assert((int) OddGCStep == 0x0 && (int) EvenGCStep == (int) BoTag_EvenGC);
  optVar = makeTaggedVar(new OptVar(this));
  lpq.init();
  setGCStep(oz_getGCStep());
}


Board::Board(Board * p) 
  : suspCount(0), dist(0),
    crt(0), suspList(0), nonMonoSuspList(0),
    script(taggedVoidValue), parent(p), flags(0)
{
  Assert(!p->isCommitted());
  status  = oz_newFuture(p);
  optVar = makeTaggedVar(new OptVar(this));
  rootVar = makeTaggedRef(newTaggedOptVar(optVar));
  setGCStep(oz_getGCStep());
  lpq.init();
#ifdef CS_PROFILE
  orig_start  = (int32 *) NULL;
  copy_start  = (int32 *) NULL;
  copy_size   = 0;
#endif
}

TaggedRef Board::genBlocked(TaggedRef arg) {
  SRecord *stuple = SRecord::newSRecord(AtomBlocked, 1);
  stuple->setArg(0, arg);
  return makeTaggedSRecord(stuple);
}

void Board::bindStatus(TaggedRef t) {
  TaggedRef s = getStatus();
  DEREF(s, sPtr);
  if (oz_isFuture(s))
    oz_bindFuture(sPtr, t);
}

void Board::clearStatus() {
  if (oz_isFuture(oz_deref(getStatus())))
    return;
  status = oz_newFuture(getParent());
}


/*
 * Propagator queue
 *
 */

void Board::wakeServeLPQ(void) {
  Assert(lpq.isEmpty());

  if (board_served == this)
    return;
  
  Thread * thr = oz_newThreadInject(this);

  // Push run lpq builtin
  thr->pushCall(BI_PROP_LPQ, 0, 0);

}

inline
void Board::killServeLPQ(void) {
  board_served = NULL;
  lpq.reset();
}

Board * Board::board_served = NULL;

OZ_Return Board::scheduleLPQ(void) {
  Assert(!board_served);

  board_served = this;

  unsigned int starttime = 0;
	 
  if (ozconf.timeDetailed)
    starttime = osUserTime();
	
  while (!lpq.isEmpty() && !am.isSetSFlag()) {
    Propagator * prop = SuspToPropagator(lpq.dequeue());
    // this is needed by first-class propagators when they are
    // explicitly discarded. I am sorry that I have to destroy this
    // invariant
    // Assert(!prop->isDead());
    if (prop->isDead())
      continue;
    Propagator::setRunningPropagator(prop);
    	   
    switch (oz_runPropagator(prop)) {
    case SLEEP:
      oz_sleepPropagator(prop);
      break;
    case PROCEED:
      oz_closeDonePropagator(prop);
      break;
    case SCHEDULED:
      oz_preemptedPropagator(prop);
      break;
    case FAILED:
      
#ifdef NAME_PROPAGATORS
      // this is experimental: a top-level failure with set
      // property 'internal.propLocation',  
      if (am.isPropagatorLocation()) {
	if (!am.hf_raise_failure()) {
	  if (ozconf.errorDebug) 
	    am.setExceptionInfo(OZ_mkTupleC("apply",2,
					    OZ_atom((prop->getPropagator()->getProfile()->getPropagatorName())),
					    prop->getPropagator()->getParameters()));	
	  oz_sleepPropagator(prop);
	  prop->setFailed();
	  killServeLPQ();
	  return RAISE;
	}
      }
#endif

      if (ozconf.timeDetailed)
	ozstat.timeForPropagation.incf(osUserTime()-starttime);
	     
      // check for top-level and if not, prepare raising of an
      // exception (`hf_raise_failure()')
      if (am.hf_raise_failure()) {
	oz_closeDonePropagator(prop);
	killServeLPQ();
	return FAILED;
      }
      
      if (ozconf.errorDebug) 
	am.setExceptionInfo(OZ_mkTupleC("apply",2,
					OZ_atom((prop->getPropagator()->getProfile()->getPropagatorName())),
					prop->getPropagator()->getParameters()));	
      
      oz_closeDonePropagator(prop);
      killServeLPQ();
      return RAISE;
    }

    Assert(prop->isDead() || !prop->isRunnable());
    
  }
  
  if (ozconf.timeDetailed)
    ozstat.timeForPropagation.incf(osUserTime()-starttime);

  if (lpq.isEmpty()) {
    killServeLPQ();
    return PROCEED;
  } else {
    board_served = NULL;
    am.prepareCall(BI_PROP_LPQ, (RefsArray *) NULL);
    return BI_REPLACEBICALL;
  }

}

/*
 * Routines for checking external suspensions
 *
 */

void Board::checkExtSuspension(Suspendable * susp) {

  Board * varHome = derefBoard();

  Board * bb = oz_currentBoard();

  Bool wasFound = NO;
    
  while (bb != varHome) {
    Assert(!bb->isRoot() && !bb->isCommitted() && !bb->isFailed());
    
    bb->addSuspension(susp);
    wasFound = OK;
    
    bb = bb->getParent();
  }
  
  if (wasFound) 
    susp->setExternal();
  
}

static
Bool extParameters(TaggedRef list, Board * solve_board) {

  list = oz_deref(list);
  
  while (oz_isCons(list)) {
    TaggedRef h = oz_head(list);
    
    Bool found = FALSE;

    DEREF(h, hptr);

    if (oz_isVar(h)) {

      Assert(!oz_isOptVar(h));

      Board * home = GETBOARD(tagged2Var(h)); 
      Board * tmp  = solve_board;

      // from solve board go up to root; if you step over home 
      // then the variable is external otherwise it must be a local one
      do {
	tmp = tmp->getParent();

	if (tmp->isFailed())
	  return FALSE;
	
	if (tmp == home) { 
	  found = TRUE;
	  break;
	}
      } while (!tmp->isRoot());
      
    } else if (oz_isCons(h)) {
      found = extParameters(h, solve_board);
    }

    if (found) return TRUE;

    list = oz_tail(oz_deref(list));
  } // while
  return FALSE;
}


void Board::clearSuspList(Suspendable * killSusp) {
  Assert(!isRoot());
  
  SuspList * fsl = getSuspList();
  SuspList * tsl = (SuspList *) 0;

  while (fsl) {
    // Traverse suspension list and copy all valid suspensions
    Suspendable * susp = fsl->getSuspendable();

    fsl = fsl->dispose();

    if (susp->isDead() ||
	killSusp == susp ||
	(susp->isRunnable() && !susp->isPropagator())) {
      continue;
    }

    Board * bb = GETBOARD(susp);

    Bool isAlive = OK;
    
    // find suspensions, which occured in a failed nested search space
    while (1) {
      Assert(!bb->isCommitted() && !bb->isRoot());
      
      if (bb->isFailed()) {
	isAlive = NO;
	break;
      }
		     
      if (bb == this)
	break;

      bb = bb->getParent();
    }

    if (susp->isPropagator()) {
      
      if (isAlive) {
	// if propagator suspends on external variable then keep its
	// thread in the list to avoid stability
	if (extParameters(SuspToPropagator(susp)->getPropagator()->getParameters(), this)) {
	  tsl = new SuspList(susp, tsl);
	} 

      }

    } else {
      Assert(susp->isThread());
      
      if (isAlive) {
	tsl = new SuspList(susp, tsl);
      } else {
	oz_disposeThread(SuspToThread(susp));
      }
      
    }
  }

  setSuspList(tsl);

}


/*
 * Stability checking
 *
 */

void Board::checkStability(void) {
  Assert(!isRoot() && !isFailed() && !isCommitted());
  Assert(this == oz_currentBoard());

  crt--;
  
  Board * pb = getParent();
  
  if (isStable()) {

    pb->decRunnableThreads();

    if (getNonMono()) {
      scheduleNonMono();
    } else {
      Distributor * d = getDistributor();

      if (d) {
	int n = d->getAlternatives();
      
	if (n == 1) {
	  if (d->commit(this,1) == 0)
	    setDistributor(NULL);
	} else {
	  trail.popMark();
	  Assert(!oz_onToplevel() || trail.isEmptyChunk());
	  am.setCurrent(pb, pb->getOptVar());
	  bindStatus(genAlt(n));
	}
	Assert(!oz_onToplevel() || trail.isEmptyChunk());
      
      } else {
	// succeeded
	trail.popMark();
	Assert(!oz_onToplevel() || trail.isEmptyChunk());
	am.setCurrent(pb, pb->getOptVar());
	
	bindStatus(genSucceeded(getSuspCount() == 0));
	Assert(!oz_onToplevel() || trail.isEmptyChunk());
      }
    }

  } else {
    int n = crt;

    setScript(trail.unwind(this));
    Assert(!oz_onToplevel() || trail.isEmptyChunk());
    am.setCurrent(pb, pb->getOptVar());
  
    if (n == 0) {
      // No runnable threads: blocked
      TaggedRef newVar = oz_newFuture(pb);
      
      bindStatus(genBlocked(newVar));
      setStatus(newVar);
      pb->decRunnableThreads();
    }
    Assert(!oz_onToplevel() || trail.isEmptyChunk());    
  }
} 

void Board::fail(void) {
  Board * pb = getParent();

  Assert(!isRoot());
      
  setFailed();
      
  pb->decRunnableThreads();

  trail.unwindFailed();
      
  am.setCurrent(pb, pb->getOptVar());
      
  bindStatus(genFailed());
     
}

/*
 * Script installation
 *
 */

OZ_Return Board::installScript(Bool isMerging)
{
  TaggedRef xys = oz_deref(script);

  setScript(oz_nil());
  
  while (oz_isCons(xys)) {
    TaggedRef xy = oz_deref(oz_head(xys));
    Assert(oz_isCons(xy));
    TaggedRef x = oz_head(xy);
    TaggedRef y = oz_tail(xy);

    xys = oz_deref(oz_tail(xys));
    
    if (!isMerging) {
      /*
       * This is very important! Normally it is okay to not
       * wake during installation, since all wakeups will
       * happen anyway and otherwise no termination is achieved.
       *
       * However, if there is the possibility that a new speculative
       * is realized (e.g. <f(X),f(a)> with X global is in the script)
       * we _have_ to wake: that's okay, because the next time the
       * script will be simplified!
       *
       */
      if (!oz_isVar(oz_deref(x)) && !oz_isVar(oz_deref(y)))
	Board::ignoreWakeUp(NO);
      else
	Board::ignoreWakeUp(OK);
    }

    int res = oz_unify(x, y);

    Board::ignoreWakeUp(NO);

    if (res != PROCEED) {
      // NOTE: in case of a failure the remaining constraints in the
      //       script are discarded
      if (res == SUSPEND) {
	res = BI_REPLACEBICALL;
	am.prepareCall(BI_Unify,x,y);
      }
      if (res == BI_REPLACEBICALL) {
	while (oz_isCons(xys)) {
	  TaggedRef xy = oz_deref(oz_head(xys));
	  Assert(oz_isCons(xy));
	  TaggedRef x = oz_head(xy);
	  TaggedRef y = oz_tail(xy);
	  xys = oz_deref(oz_tail(xys));
	  am.prepareCall(BI_Unify,x,y);
	}
      }
      return res;
    }
  }
  return PROCEED;
}




/*
 * Constraint installation
 *
 */


Bool Board::installDown(Board * frm) {

  if (frm == this)
    return OK;

  if (!getParent()->installDown(frm))
    return NO;

  am.setCurrent(this, optVar);
  trail.pushMark();

  OZ_Return ret = installScript(NO);

  if (ret != PROCEED) {
    Assert(ret==FAILED);
    fail();
    return NO;
  }

  return OK;

}


Bool Board::install(void) {
  // Tries to install "this".
  // If installation of a script fails, NO is returned and
  // the highest space for which installation is possible gets installed.
  // Otherwise, OK is returned.

  Board * frm = oz_currentBoard();

  Assert(!frm->isCommitted() && !this->isCommitted());

  if (frm == this)
    return OK;

  if (!isAlive())
    return NO;

  // Step 1: Mark all spaces including root as installed
  {
    Board * s;

    for (s = frm; !s->isRoot(); s=s->getParent()) {
      Assert(!s->hasMark());
      s->setMark();
    }
    Assert(!s->hasMark());
    s->setMark();
  }

  // Step 2: Find ancestor
  Board * ancestor = this;

  while (!ancestor->hasMark())
    ancestor = ancestor->getParent();

  // Step 3: Deinstall from "frm" to "ancestor", also purge marks
  {
    Board * s = frm;

    while (s != ancestor) {
      Assert(s->hasMark());
      s->unsetMark();
      s->setScript(trail.unwind(s));
      s=s->getParent();
      am.setCurrent(s, s->getOptVar());
    }

    am.setCurrent(ancestor, ancestor->getOptVar());

    // Purge remaining marks
    for ( ; !s->isRoot() ; s=s->getParent()) {
      Assert(s->hasMark());
      s->unsetMark();
    }
    Assert(s->hasMark());
    s->unsetMark();

  }

  // Step 4: Install from "ancestor" to "this"

  return installDown(ancestor);

}

/*
 * Before copying all spaces but the space to be copied get marked.
 * 
 * Important: even committed boards must be marked, since the globality
 * test does not do a dereference!
 *
 */

void Board::setGlobalMarks(void) {
  Board * b = this;

  while (!b->isRoot()) {
    b = b->getParentInternal(); 
    Assert(!b->hasMark());
    b->setMark();
  }
  
}

/*
 * Purge marks after copying
 */

void Board::unsetGlobalMarks(void) {
  Board * b = this;

  while (!b->isRoot()) {
    b = b->getParentInternal(); 
    Assert(b->hasMark());
    b->unsetMark();
  }

}

OZ_Return (*OZ_checkSituatedness)(Board *,TaggedRef *);


