/*
 *  Authors:
 *    Kostja Popow (popow@ps.uni-sb.de)
 *    Michael Mehl (mehl@dfki.de)
 *
 *  Contributors:
 *    Christian Schulte <schulte@ps.uni-sb.de>
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

// proper actors

#ifndef __ACTORH
#define __ACTORH

#ifdef INTERFACE
#pragma interface
#endif

#include "cpbag.hh"
#include "cont.hh"

// ------------------------------------------------------------------------
//  all 'proper' actors;

enum ActorFlags {
  Ac_None       = 0,
  Ac_Ask        = 0x01,
  Ac_Wait       = 0x02,
  Ac_Solve      = 0x04,
  Ac_Committed  = 0x08,
  Ac_Choice     = 0x10, // in disjunction with Ac_Wait
};

class Actor {
public:
protected:
  int flags;
  Board *board;
  Actor * gcField;// mm2: hack: flags and board seem to be needed for copying?
public:
  NO_DEFAULT_CONSTRUCTORS(Actor)

protected:
  Actor(int typ,Board *bb)
    : board(bb)
  {
    flags = typ;
    gcField = 0;
  }

public:
  USEHEAPMEMORY;

  Bool gcIsMarked(void);
  void gcMark(Actor * fwd);
  Actor * gcGetFwd(void);

  Bool isCommitted() { return flags & Ac_Committed;     }
  Bool isAsk()       { return flags & Ac_Ask;           }
  Bool isWait()      { return flags & Ac_Wait;          }
  Bool isAskWait()   { return flags & (Ac_Ask|Ac_Wait); }
  Bool isSolve()     { return flags & Ac_Solve;         }
  Bool isChoice()    { return flags & Ac_Choice;        }

  void setCommittedActor() { flags |= Ac_Committed; }

  void discardActor() { setCommittedActor(); }

  Actor * gcActor();
  void gcRecurse(void);
  OZPRINTLONG

  Board *getBoardInternal() { return board; }
};

// ------------------------------------------------------------------------
//  'ask' or 'wait' actors;

class AWActor : public Actor {
public:
  static AWActor *Cast(Actor *a) {
    DebugCheck((a->isAskWait () == NO), OZ_error ("AWActor::Cast"));
    return ((AWActor *) a);
  }
protected:
  Thread *thread;
  Continuation next;
  int childCount;
public:
  NO_DEFAULT_CONSTRUCTORS(AWActor)
  AWActor(int typ,Board *bb,Thread *tt,
          ProgramCounter p=NOCODE,RefsArray y=0,Abstraction *cap=0,
          RefsArray x=0,int i=0)
    : Actor (typ, bb)
  {
    thread=tt;
    childCount=0;
    next.setPC(p);
    next.setY(y);
    next.setCAP(cap);
    next.setX(x,i);
  }

  USEHEAPMEMORY;

  void gcRecurse(void);
  void addChild() {
    childCount++;
  }

  void failChild() {
    childCount--;
    Assert(childCount>=0);
  }
  Continuation *getNext() { return &next; }
  Bool hasNext() { return ((next.getPC() == NOCODE) ? NO : OK); }
  /* see also: hasOneChild() */
  Bool isLeaf() { return childCount == 0 && next.getPC() == NOCODE; }
  void lastClause() { next.setPC(NOCODE); }
  void nextClause(ProgramCounter pc) { next.setPC(pc); }
  void setThread(Thread *th) { thread = th; }
  Thread *getThread() { return thread; }
};

// ------------------------------------------------------------------------

class AskActor : public AWActor {
public:
  static AskActor *Cast(Actor *a)
  { DebugCheck(!a->isAsk(),OZ_error("AskActor::Cast")); return (AskActor *) a; }
private:
  ProgramCounter elsePC;
public:
  NO_DEFAULT_CONSTRUCTORS(AskActor)
  AskActor(Board *s,Thread *tt,
           ProgramCounter elsepc,
           ProgramCounter p, RefsArray y,Abstraction *cap, RefsArray x, int i)
    : AWActor(Ac_Ask,s,tt,p,y,cap,x,i)
  {
    elsePC = elsepc;
  }

  void addAskChild() {
    addChild();
  }
  void failAskChild() {
    failChild();
  }

  ProgramCounter getElsePC() { return elsePC; }

  // not possible because spaces are lazy marked as failed
  void disposeAsk(void) {
    // freeListDispose(this,sizeof(AskActor));
  }
};

// ------------------------------------------------------------------------

class WaitActor : public AWActor {
public:
  static WaitActor *Cast(Actor *a)
  {
    Assert(a->isWait()); return (WaitActor *) a;
  }
private:
  Board **children;
  CpBag *cpb;
public:
  NO_DEFAULT_CONSTRUCTORS(WaitActor)

  WaitActor(Board *s,Thread *tt,
            ProgramCounter p,RefsArray y,Abstraction *cap,
            Bool d)
    : AWActor((d ? (ActorFlags)(Ac_Wait | Ac_Choice) : Ac_Wait),s,tt,
              p,y,cap,0,0)
  {
    children  = NULL;
    cpb       = NULL;
  }

  USEFREELISTMEMORY;

  void gcRecurse();

  void addWaitChild(Board *n);
  void failWaitChild(Board *n);

  // returns the first created child; this child is unlinked from the actor;
  Board *getLastChild() { Board* b=children[0]; children[0] = NULL; return b; }

  // the same, but a child is not unlinked from the actor;
  Board *getChildRef() { return children[0]; }

  int getChildCount() { return childCount; };
  Bool hasOneChildNoChoice() { return
                                 childCount == 1 &&
                                 !isChoice() &&
                                 !hasNext(); }
  Bool hasNoChildren() { return ((childCount == 0 && !hasNext()) ? OK : NO); }
  void selectOrFailChildren(int l, int r);

  // maybe in some cpbag...
  void disposeWait(void) {
    // freeListDispose(children-1,(ToInt32(children[-1])+1)*sizeof(Board *));
    // freeListDispose(this,sizeof(WaitActor));
  }

  Bool isAliveUpToSolve(void);

  void addChoice(WaitActor *wa) {
    cpb = cpb->add(wa);
  }
  void mergeChoices(CpBag *mcpb) {
    cpb = cpb->merge(mcpb);
  }
  CpBag * getCpb() {
    return cpb;
  }

};

#endif
