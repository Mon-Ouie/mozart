/*
 *  Authors:
 *    Kostja Popow (popow@ps.uni-sb.de)
 *    Christian Schulte (schulte@dfki.de)
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

// Solver

#if defined(INTERFACE) && !defined(PEANUTS)
#pragma implementation "solve.hh"
#endif

#include "am.hh"

#include "cpbag.hh"
#include "solve.hh"

#ifdef DEBUG_NONMONOTONIC
#include "runtime.hh"
#endif

/*
 * class SolveActor:
 *    solve actor;
 *    member data:
 *      solveBoard: 'home' board of a search problem;
 *      orActors: all actors that may be distributed;
 *      solveVar: root variable of a search problem;
 *      result: reference to the control variable
 *         (recall that there is single control variable, but not three
 *           as in kernel definition);
 *      suspList: list of external threads;
 *      threads: the number of *runnable* threads!
 */

/* ------------------------------------------------------------------------
   class SolveActor
   ------------------------------------------------------------------------ */

TaggedRef SolveActor::genSolved() {
  ozstat.incSolveSolved();
  SRecord *stuple = SRecord::newSRecord(AtomSucceeded, 1);

  Assert(solveBoard->isSolve());
  stuple->setArg(0, AtomEntailed);

  return makeTaggedSRecord(stuple);
}

TaggedRef SolveActor::genStuck() {
  SRecord *stuple = SRecord::newSRecord(AtomSucceeded, 1);

  Assert(solveBoard->isSolve());
  stuple->setArg(0, AtomSuspended);
  return makeTaggedSRecord(stuple);
}

TaggedRef SolveActor::genChoice(int noOfClauses) {
  SRecord *stuple = SRecord::newSRecord(AtomAlt, 1);

  Assert(!solveBoard->isCommitted());
  stuple->setArg(0, makeTaggedSmallInt(noOfClauses));

  return makeTaggedSRecord(stuple);
}

TaggedRef SolveActor::genFailed() {
  ozstat.incSolveFailed();
  return AtomFailed;
}

TaggedRef SolveActor::genUnstable(TaggedRef arg) {
  SRecord *stuple = SRecord::newSRecord(AtomBlocked, 1);
  stuple->setArg(0, arg);
  return makeTaggedSRecord(stuple);
}

Bool SolveActor::isBlocked() {
  return ((getThreads()==0) && !am.isStableSolve(this));
}

static
Bool extParameters(OZ_Term list, Board * solve_board)
{
  while (OZ_isCons(list)) {
    OZ_Term h = OZ_head(list);

    Bool found = FALSE;

    if (OZ_isVariable(h)) {

#ifdef DEBUG_PROP_STABILTY_TEST
      oz_print(h);
#endif

      DEREF(h, hptr, htag);

      Assert(!isUVar(htag));

      Board  * home = GETBOARD(tagged2SVarPlus(h));
      Board * tmp = solve_board;

      // from solve board go up to root; if you step over home
      // then the variable is external otherwise it must be a local one
      do {
        tmp = tmp->getParent();

        Assert (!(tmp->isCommitted()) && !(tmp->isFailed()));

        if (tmp == home) {
          found = TRUE;
          break;
        }
      } while (!tmp->_isRoot());

    } else if (OZ_isCons(h)) {
      found = extParameters(h, solve_board);
    }

    if (found) return TRUE;

    list = OZ_tail(list);
  } // while
  return FALSE;
}

void SolveActor::clearSuspList(Suspension killSusp) {
  SuspList * tmpSuspList = suspList;

  suspList = NULL;
  while (tmpSuspList) {
    Suspension susp = tmpSuspList->getSuspension();

    /*
     *  kost@
     *  Note that i've preserved here a limitation of stability
     * check: no propagators (i.e. former "resistant" suspensions")
     * might suspend on global variables; otherwise, no stability
     * will be reached (precisely speaking, they must go away -
     * then stability can be reached);
     *  This limitation was introduced with the "resistant"
     * suspensions (Hi, Tobias!).
     *
     */

    if (susp.isDead() ||
        killSusp == susp ||
        (susp.isRunnable() && !susp.isPropagator())) {
      tmpSuspList = tmpSuspList->dispose ();
      continue;
    }

    Board * bb = GETBOARDOBJ(susp);

    // find suspensions, which occured in a failed nested search space
    while (1) {
      bb = bb->getSolveBoard();
      if (bb == solveBoard) break;
      if (bb == 0) break;
      bb = bb->getParentAndTest();
      if (bb == 0) break;
    }

    if (susp.isPropagator()) {
      Propagator * prop = susp.getPropagator();

#ifdef DEBUG_PROP_STABILTY_TEST
      cout << "SolveActor::clearSuspList : Found propagator." << endl
           << prop->toString() << endl
           << "\tbb = " << bb << endl << flush;
#endif

      if (bb) {
        // if propagator suspends on external variable then keep its
        // thread in the list to avoid stability
        if (extParameters(prop->getPropagator()->getParameters(), solveBoard)) {
#ifdef DEBUG_PROP_STABILTY_TEST
          cout << "\tExt parameter found!" << endl << flush;
#endif
          SuspList * helpList = tmpSuspList;
          addSuspension (helpList);
        }
#ifdef DEBUG_PROP_STABILTY_TEST
        else {
          cout << "\tNo ext parameter found!" << endl << flush;
        }
#endif

      }
      tmpSuspList = tmpSuspList->getNext ();
    } else {
      Assert(susp.isThread());

      Thread * thr = susp.getThread();

      if (bb == 0) {
        am.disposeSuspendedThread(thr);
        tmpSuspList = tmpSuspList->dispose ();
      } else {
        SuspList *helpList = tmpSuspList;
        tmpSuspList = tmpSuspList->getNext();
        addSuspension (helpList);
      }
    }
  }
}

SolveActor::SolveActor(Board *bb)
 : Actor (Ac_Solve, bb), cpb(NULL), suspList (NULL), threads (0),
   nonMonoSuspList(NULL) {
  result     = makeTaggedRef(newTaggedUVar(bb));
  solveBoard = new Board(this, Bo_Solve);
  solveVar   = makeTaggedRef(newTaggedUVar(solveBoard));
#ifdef CS_PROFILE
  orig_start  = (int32 *) NULL;
  copy_start  = (int32 *) NULL;
  copy_size   = 0;
#endif
}

#ifdef CS_PROFILE
TaggedRef SolveActor::getCloneDiff(void) {
  TaggedRef l = nil();

  if (copy_start && orig_start && (copy_size>0)) {
    int n = 0;

    while (n < copy_size) {
      if (copy_start[n] != orig_start[n]) {
        int d = 0;

        while ((n < copy_size) && (copy_start[n] != orig_start[n])) {
          d++; n++;
        }

        LTuple *lt = new LTuple();
        lt->setHead(newSmallInt(d));
        lt->setTail(l);
        l = makeTaggedLTuple(lt);
      } else {
        n++;
      }

    }

    free(copy_start);

    TaggedRef ret = OZ_pair2(newSmallInt(copy_size),l);

    copy_start = (int32 *) 0;
    copy_size  = 0;
    orig_start = (int32 *) 0;

    return ret;

  } else {
    return OZ_pair2(newSmallInt(0),l);
  }

}
#endif

//-----------------------------------------------------------------------------
// support for nonmonotonic propagators

void SolveActor::addToNonMonoSuspList(Propagator * prop)
{
  nonMonoSuspList = nonMonoSuspList->insert(prop);
}

void SolveActor::mergeNonMonoSuspListWith(OrderedSuspList * p)
{
  for (; p != NULL; p = p->getNext())
    nonMonoSuspList = nonMonoSuspList->insert(p->getPropagator());
}

void SolveActor::scheduleNonMonoSuspList(void)
{
#ifdef DEBUG_NONMONOTONIC
  printf("------------------------------------------------------------------"
         "\nSolveActor::scheduleNonMonoSuspList\n"); fflush(stdout);
#endif

  for (OrderedSuspList * p = nonMonoSuspList; p != NULL; p = p->getNext()) {
    Propagator * prop = p->getPropagator();

#ifdef DEBUG_NONMONOTONIC
    OZ_CFunHeader * header = prop->getPropagator()->getHeader();
    OZ_CFun headerfunc = header->getHeaderFunc();
    printf("<%s %d>\n", builtinTab.getName((void *) headerfunc),
           prop->getPropagator()->getOrder());
#endif

    //am.updateSolveBoardPropagatorToRunnable(thr);
    GETBOARD(prop)->pushToLPQ(prop);
    //am.scheduleThreadInline(thr, thr->getPriority());
  }

  nonMonoSuspList = NULL;

#ifdef DEBUG_NONMONOTONIC
  printf("Done\n"); fflush(stdout);
#endif
}

//-----------------------------------------------------------------------------

#ifdef OUTLINE
#define inline
#include "solve.icc"
#undef inline
#endif
