/*
 *  Authors:
 *    Konstantin Popov (popow@ps.uni-sb.de)
 *    Tobias M�ller (tmueller@ps.uni-sb.de)
 *    Christian Schulte <schulte@ps.uni-sb.de>
 * 
 *  Copyright:
 *    Konstantin Popov, 1999
 *    Tobias M�ller, 1999
 *    Christian Schulte, 1999
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

#include "suspendable.hh"
#include "thr_class.hh"
#include "prop_class.hh"
#include "thr_int.hh"
#include "prop_int.hh"
#include "am.hh"

inline
Bool Suspendable::_wakeup(Board * home, PropCaller calledBy) {
  // Returns OK, if suspension can go away
  // This can happen, if either the suspension is dead
  // or gets woken
  
  if (isDead())
    return OK;

  Board * sb = getBoardInternal()->derefBoard();

  oz_BFlag between = oz_isBetween(sb, home);
  
  if (isRunnable()) {

    if (isThread()) {
      return OK;
    } else {
      if (calledBy && !isUnify()) {
	switch (between) {
	case B_BETWEEN:
	  setUnify(); 
	  return NO;
	case B_DEAD:
	  return OK;
	case B_NOT_BETWEEN:
	  return NO;
	}
      }
    } 

    return NO;

  } else {

    if (isThread()) {

      switch (between) {
      case B_BETWEEN:
	oz_wakeupThread(SuspToThread(this));
	return OK;
      case B_NOT_BETWEEN:
	if (calledBy==pc_all) {
	  oz_wakeupThread(SuspToThread(this));
	  return OK;
	}
	return NO;
      case B_DEAD:
	setDead();

	if (isExternal())
	  sb->checkSolveThreads();

	SuspToThread(this)->disposeStack();
	return OK;
      }
      
    } else {

      switch (between) {
      case B_BETWEEN:
	if (calledBy)
	  setUnify();
	setRunnable();
	if (isNMO() && !oz_onToplevel()) {
	  Assert(!SuspToPropagator(this)->getPropagator()->isMonotonic());

	  sb->addToNonMono(SuspToPropagator(this));
	} else {
	  sb->addToLPQ(SuspToPropagator(this));
	}
	return NO;
      case B_NOT_BETWEEN:
	return NO;
      case B_DEAD:
	setDead();
	if (isExternal())
	  sb->checkSolveThreads();
	SuspToPropagator(this)->dispose();
	return OK;
      }

    } 
    
  }
  
  return NO;

}

Bool Suspendable::wakeup(Board * bb, PropCaller calledBy) {
  return _wakeup(bb, calledBy);
}


void oz_checkAnySuspensionList(SuspList ** suspList,
			       Board * home,
			       PropCaller calledBy) {
  if (am.inEqEq())
    return;

  home = home->derefBoard();
 
  SuspList ** p  = suspList;

  SuspList * sl = *suspList; 

  while (sl) {
    
    SuspList ** n = sl->getNextRef();
    
    if (sl->getSuspendable()->_wakeup(home,calledBy)) {
      *p = *n;
      sl->dispose();
      sl = *p;
    } else {
      sl = *n;
      p  = n;
    }
    
    
  }
    
}


inline
Bool Suspendable::_wakeupLocal(Board * sb, PropCaller calledBy) {
  if (isDead())
    return OK;
  
  if (calledBy)
    setUnify();
    
  if (!isRunnable()) {
    setRunnable();
    
    if (isNMO() && !oz_onToplevel()) {
      Assert(!SuspToPropagator(this)->getPropagator()->isMonotonic());
      
      sb->addToNonMono(SuspToPropagator(this));
    } else {
      sb->addToLPQ(SuspToPropagator(this));
    }
    
  }

  return NO;
  
}

void oz_checkLocalSuspensionList(SuspList ** suspList,
				 PropCaller calledBy) {
  if (am.inEqEq())
    return;


  SuspList ** p = suspList;

  SuspList * sl = *p; 

  if (!sl)
    return;

  Board * sb = sl->getSuspendable()->getBoardInternal()->derefBoard();

  do {
    
    SuspList ** n = sl->getNextRef();

    Assert(sb == sl->getSuspendable()->getBoardInternal()->derefBoard());
  
    if (sl->getSuspendable()->_wakeupLocal(sb,calledBy)) {
      *p = *n;
      sl->dispose();
      sl = *p;
    } else {
      sl = *n;
      p  = n;
    }
    
    
  } while (sl);
    
}

