/*
 *  Authors:
 *    Kostja Popov (popow@ps.uni-sb.de)
 *    Christian Schulte (schulte@dfki.de)
 *
 *  Contributors:
 *    Michael Mehl (mehl@dfki.de)
 *
 *  Copyright:
 *    Kostja Popov, 1997
 *    Christian Schulte, 1997, 1998
 *
 *  Last change:
 *    $Date$ by $Author$
 *    $Revision$
 *
 *  This file is part of Mozart, an implementation
 *  of Oz 3:
 *     http://mozart.ps.uni-sb.de
 *
 *  See the file "LICENSE" or
 *     http://mozart.ps.uni-sb.de/LICENSE.html
 *  for information on usage and redistribution
 *  of this file, and for a DISCLAIMER OF ALL
 *  WARRANTIES.
 *
 */

// Solver

#ifndef __SOLVEH
#define __SOLVEH

#ifdef INTERFACE
#pragma interface
#endif

#include "actor.hh"
#include "cpbag.hh"
#include "value.hh"

// ------------------------------------------------------------------------

class SolveActor : public Actor {
public:

  void printLongStreamSolve(ostream &stream, int depth, int offset);

  static SolveActor *Cast(Actor *a) {
    Assert(a->isSolve());
    return ((SolveActor *) a);
  }
private:
  Board     *solveBoard;
  CpBag     *cpb;
  TaggedRef solveVar;
  TaggedRef result;
  SuspList  *suspList;
  int threads;

#ifdef CS_PROFILE
public:
  int32 * orig_start;
  int32 * copy_start;
  int     copy_size;
#endif

public:
  NO_DEFAULT_CONSTRUCTORS(SolveActor);
  SolveActor(Board *bb);

  Board *getSolveBoard() {
    return solveBoard;
  }

  // mm2: ask christian why &solveVar
  TaggedRef getSolveVar() {
    return makeTaggedRef(&solveVar);
  }

  void gcRecurse();

  void incThreads(int n = 1) {
    threads += n;
  }
  int decThreads()  {
    Assert (threads > 0);
    return (--threads);
  }

  int getThreads(void) {
    return threads;
  }

  void addSuspension(Suspension);
  void addSuspension(SuspList *);
  Bool isEmptySuspList() { return suspList==0; }
  void setSuspList(SuspList *sl) { suspList=sl; }
  SuspList *unlinkSuspList() {
    SuspList *sl = suspList;
    suspList=0;
    return sl;
  }
  WaitActor * select(int left, int right);

  void mergeCPB(Board *bb, int siblings);
  void mergeNonMono(Board *bb);

  void clearResult(Board *bb);
  void patchChoiceResult(int i) {
    SRecord *stuple = SRecord::newSRecord(AtomAlt, 1);

    stuple->setArg(0, makeTaggedSmallInt(i));

    result = makeTaggedSRecord(stuple);
  }

  TaggedRef getResult() { return result; }
  void setResult(TaggedRef v) { result = v; }

  void addChoice(WaitActor *wa) {
    cpb = cpb->add(wa);
  }
  void mergeChoices(CpBag *pcpb) {
    cpb = cpb->merge(pcpb);
  }
  CpBag *getCpb() { return cpb; }

  WaitActor *getChoice() {
    WaitActor * wa;
    cpb = cpb->get(&wa);
    return wa;
  }
  void removeChoice() {
    cpb = cpb->remove();
  }

  void setBoard(Board *bb) { board = bb; }

  TaggedRef genSolved();
  TaggedRef genStuck();
  TaggedRef genChoice(int noOfClauses);
  TaggedRef genFailed();
  TaggedRef genUnstable(TaggedRef arg);

#ifdef CS_PROFILE
  TaggedRef getCloneDiff(void);
#endif

//-----------------------------------------------------------------------------
// support for nonmonotonic propagators
private:
  OrderedSuspList * nonMonoSuspList;
public:
  void addToNonMonoSuspList(Propagator *);
  void mergeNonMonoSuspListWith(OrderedSuspList *);
  void setNonMonoSuspList(OrderedSuspList * l) { nonMonoSuspList = l; }
  OrderedSuspList *getNonMonoSuspList() { return nonMonoSuspList; }

//-----------------------------------------------------------------------------
};

#ifndef OUTLINE
#include "solve.icc"
#endif

#endif
