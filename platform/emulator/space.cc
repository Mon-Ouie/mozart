/*
 *  Authors:
 *    Kostja Popow (popow@ps.uni-sb.de)
 *    Michael Mehl (mehl@dfki.de)
 *    Christian Schulte <schulte@ps.uni-sb.de>
 *
 *  Contributors:
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

#include "space.hh"
#include "thr_int.hh"
#include "var_base.hh"

inline
void unBind(TaggedRef *p, TaggedRef t)
{
  Assert(oz_isVariable(t));
  *p = t;
}

OZ_Return oz_installScript(Script &script)
{
  OZ_Return ret = PROCEED;
  am.setInstallingScript(); // mm2: special hack ???
  for (int index = 0; index < script.getSize(); index++) {
    int res = oz_unify(script[index].getLeft(),script[index].getRight());
    if (res == PROCEED) continue;
    if (res == FAILED) {
      ret = FAILED;
      if (!oz_onToplevel()) {
        break;
      }
    } else {
      // mm2: instead of failing, this should corrupt the space
      (void) am.emptySuspendVarList();
      ret = FAILED;
      if (!oz_onToplevel()) {
        break;
      }
    }
  }
  am.unsetInstallingScript();

  script.dealloc();

  return ret;
}

/*
 *
 *  Install every board from the currentBoard to 'n'
 * and move cursor to 'n'
 *
 *  Algorithm:
 *   find common parent board of 'to' and 'currentBoard'
 *   deinstall until common parent (go upward)
 *   install (go downward)
 *
 *  Pre-conditions:
 *  - 'to' ist not deref'd;
 *  - 'to' may be committed, failed or discarded;
 *
 *  Return values and post-conditions:
 *  - INST_OK:
 *      installation successful, currentBoard == 'to';
 *  - INST_FAILED:
 *      installation of *some* board on the "down" path has failed,
 *      'am.currentBoard' points to that board;
 *  - INST_REJECTED:
 *      *some* board on the "down" path is already failed or discarded,
 *      'am.currentBoard' stays unchanged;
 *
 */

// only used in deinstall
// Three cases may occur:
// any global var G -> ground ==> add susp to G
// any global var G -> constrained local var ==> add susp to G
// unconstrained global var G1 -> unconstrained global var G2
//    ==> add susp to G1 and G2

void oz_reduceTrailOnSuspend() {

  if (!am.trail.isEmptyChunk()) {

    int numbOfCons = am.trail.chunkSize();

    Board * bb = oz_currentBoard();

    bb->newScript(numbOfCons);

    // one single suspended thread for all;
    Thread *thr = oz_newThreadPropagate(bb);

    for (int index = 0; index < numbOfCons; index++) {
      TaggedRef * refPtr, value;

      switch (am.trail.getTeType()) {
      case Te_Bind: {
        am.trail.popBind(refPtr, value);
        Assert(oz_isRef(*refPtr) || !oz_isVariable(*refPtr));
        Assert(oz_isVariable(value));

        bb->setScript(index,refPtr,*refPtr);

        TaggedRef vv= *refPtr;
        DEREF(vv,vvPtr,_vvTag);
        if (oz_isVariable(vv)) {
          oz_var_addSusp(vvPtr,thr,NO);  // !!! Makes space *not* unstable !!!
        }

        unBind(refPtr, value);

        // value is always global variable, so add always a thread;
        if (oz_var_addSusp(refPtr,thr)!=SUSPEND) {
          Assert(0);
        }

        break;
      }
      case Te_Variable:
        am.trail.popVariable();
        break;
      case Te_Cast:
        am.trail.popCast();
        break;
      default:
        break;
      }
    }

  }

  am.trail.popMark();
}


void oz_reduceTrailOnFail(void) {

  while(!am.trail.isEmptyChunk()) {

    switch (am.trail.getTeType()) {
    case Te_Bind: {
      TaggedRef *refPtr;
      TaggedRef value;
      am.trail.popBind(refPtr,value);
      unBind(refPtr,value);
      break;
    }
    case Te_Variable:
      am.trail.popVariable();
      break;
    case Te_Cast:
      am.trail.popCast();
      break;
    default:
      break;
    }

  }

  am.trail.popMark();
}

void oz_reduceTrailOnEqEq(void) {
  am.emptySuspendVarList();

  while(!am.trail.isEmptyChunk()) {

    switch (am.trail.getTeType()) {
    case Te_Bind: {
      TaggedRef *refPtr;
      TaggedRef value;
      am.trail.popBind(refPtr,value);

      Assert(oz_isVariable(value));

      TaggedRef oldVal = makeTaggedRef(refPtr);
      DEREF(oldVal,ptrOldVal,_1);

      unBind(refPtr,value);

      if (oz_isVariable(oldVal))
        am.addSuspendVarList(ptrOldVal);

      am.addSuspendVarList(refPtr);
    }
    case Te_Variable:
      am.trail.popVariable();
      break;
    case Te_Cast:
      am.trail.popCast();
      break;
    default:
      break;
    }

  }

  am.trail.popMark();
}


static
Board * installOnly(Board * frm, Board * to) {

  if (frm == to)
    return frm;

  Board * r = installOnly(frm, to->getParent());

  if (r != frm)
    return r;

  am.setCurrent(to);
  am.trail.pushMark();

  OZ_Return ret = oz_installScript(to->getScriptRef());
  if (ret != PROCEED) {
    Assert(ret==FAILED);
    return to;
  }

  return r;

}


Bool oz_installPath(Board * to) {
  // Tries to install "to".
  // If installation of a script fails, NO is returned and
  // the highest space for which installation is possible gets installed.
  // Otherwise, OK is returned.

  Board * frm = oz_currentBoard();

  Assert(!frm->isCommitted() && !to->isCommitted());

  if (frm == to)
    return OK;

  Assert(to->isAlive());

  // Step 1: Mark all spaces including root as installed
  {
    Board * s;

    for (s = frm; !s->isRoot(); s=s->getParent()) {
      Assert(!s->hasMarkOne());
      s->setMarkOne();
    }
    Assert(!s->hasMarkOne());
    s->setMarkOne();
  }

  // Step 2: Find ancestor
  Board * ancestor = to;

  while (!ancestor->hasMarkOne())
    ancestor = ancestor->getParent();

  // Step 3: Deinstall from "frm" to "ancestor", also purge marks
  {
    Board * s = frm;

    while (s != ancestor) {
      Assert(s->hasMarkOne());
      s->unsetMarkOne();
      oz_reduceTrailOnSuspend();
      s=s->getParent();
      am.setCurrent(s);
    }

    am.setCurrent(ancestor);

    // Purge remaining marks
    for ( ; !s->isRoot() ; s=s->getParent()) {
      Assert(s->hasMarkOne());
      s->unsetMarkOne();
    }
    Assert(s->hasMarkOne());
    s->unsetMarkOne();

  }

  // Step 4: Install from "ancestor" to "to"

  return installOnly(ancestor, to) == ancestor;

}
