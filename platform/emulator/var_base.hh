/*
 *  Authors:
 *    Michael Mehl (mehl@dfki.de)
 *    Kostja Popow (popow@ps.uni-sb.de)
 *    Ralf Scheidhauer (Ralf.Scheidhauer@ps.uni-sb.de)
 *
 *  Contributors:
 *    Tobias Mueller (tmueller@ps.uni-sb.de)
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
 *     http://mozart.ps.uni-sb.de
 *
 *  See the file "LICENSE" or
 *     http://mozart.ps.uni-sb.de/LICENSE.html
 *  for information on usage and redistribution
 *  of this file, and for a DISCLAIMER OF ALL
 *  WARRANTIES.
 *
 */

#ifndef __VARIABLEH
#define __VARIABLEH

#ifdef INTERFACE
#pragma interface
#endif

#include "tagged.hh"
#include "susplist.hh"
#include "board.hh"

#define AddSuspToList0(List, Susp, Home)                \
{                                                       \
  if ((List) && ((List)->getElem() == Susp)) {          \
  } else {                                              \
    List = new SuspList(Susp, List);                    \
    if (Home) oz_checkExtSuspension(Susp, Home);        \
  }                                                     \
}

#ifdef DEBUG_STABLE

#define AddSuspToList(List, Susp, Home)                         \
{                                                               \
  AddSuspToList0(List, Susp, Home);                             \
                                                                \
  if (board_constraints_thr != Susp) {                          \
    board_constraints_thr = Susp;                               \
    board_constraints = new SuspList(board_constraints_thr,     \
                                     board_constraints);        \
  }                                                             \
}

#else

#define AddSuspToList(List, Susp, Home) AddSuspToList0(List, Susp, Home)

#endif


#define STORE_FLAG 1
#define REIFIED_FLAG 2


#define SVAR_EXPORTED 1
#define SVAR_FLAGSMASK 0x3

class SVariable {
protected:
  SuspList * suspList;
  unsigned int homeAndFlags;
public:

  USEFREELISTMEMORY;

  Board *getHome1()        { return (Board *)(homeAndFlags&~SVAR_FLAGSMASK); }
  void setHome(Board *h) {
    homeAndFlags = (homeAndFlags&SVAR_FLAGSMASK)|((unsigned)h); }

  Bool isExported()   { return homeAndFlags&SVAR_EXPORTED; }
  void markExported() { homeAndFlags |= SVAR_EXPORTED; }

  SVariable() {}

  SVariable(Board * h) : suspList(NULL) { homeAndFlags=0; setHome(h); }

  void dispose(void) {
    suspList->disposeList();
    freeListDispose(this,sizeof(*this));
  }

  // get home node without deref, for faster isLocal
  Board *getHomeUpdate() {
    if (getHome1()->isCommitted()) {
      setHome(getHome1()->derefBoard());
    }
    return getHome1();
  }
  Board *getBoardInternal() { return getHome1(); }
  SuspList *getSuspList() { return suspList; }
  int getSuspListLengthS() { return suspList->length(); }

  void setSuspList(SuspList *inSuspList) { suspList = inSuspList; }
  void unlinkSuspList() { suspList = NULL; }

  // takes the suspensionlist of var and  appends it to the
  // suspensionlist of leftVar
  void relinkSuspListTo(SVariable * lv, Bool reset_local = FALSE) {
    suspList = suspList->appendToAndUnlink(lv->suspList, reset_local);
  }

  Bool gcIsMarked(void);
  void gcMark(Bool, TaggedRef *);
  TaggedRef * gcGetFwd(void);
  SVariable * gc();

  void setStoreFlag(void) {
    suspList = (SuspList *) (((long) suspList) | STORE_FLAG);
  }
  void resetStoreFlag(void) {
    suspList = (SuspList *) (((long) suspList) & ~STORE_FLAG);
  }
  OZ_Boolean testStoreFlag(void) {
    return ((long)suspList) & STORE_FLAG;
  }
  OZ_Boolean testResetStoreFlag(void) {
    OZ_Boolean r = testStoreFlag();
    resetStoreFlag();
    return r;
  }

  void setReifiedFlag(void) {
    suspList = (SuspList *) (((long) suspList) | REIFIED_FLAG);
  }
  void resetReifiedFlag(void) {
    suspList = (SuspList *) (((long) suspList) & ~REIFIED_FLAG);
  }
  OZ_Boolean testReifiedFlag(void) {
    return ((long)suspList) & REIFIED_FLAG;
  }
  OZ_Boolean testResetReifiedFlag(void) {
    OZ_Boolean r = testReifiedFlag();
    resetReifiedFlag();
    return r;
  }

  void addSuspSVar(Suspension susp, int unstable)
  {
    AddSuspToList(suspList, susp, unstable ? getHome1() : 0);
  }

  OZPRINTLONG;
};

// not yet inlined
void addSuspUVar(TaggedRef * v, Suspension susp, int unstable = TRUE);

void oz_cv_addSusp(GenCVariable *, TaggedRef *, Suspension, int = TRUE);

inline
void addSuspAnyVar(TaggedRef * v, Suspension susp,int unstable = TRUE)
{
  TaggedRef t = *v;
  // FUT
  if (isCVar(t)) {
    oz_cv_addSusp(tagged2CVar(*v), v, susp, unstable);
  } else {
    addSuspUVar(v, susp, unstable);
  }
}


/* ------------------------------------------------------------------------
 * Class VariableNamer: assign names to variables
 * ------------------------------------------------------------------------ */

class VariableNamer {
private:
  static VariableNamer *allnames;
  TaggedRef var;
  const char *name;
  VariableNamer *next;
  int length();
public:
  static void cleanup();
  static const char *getName(TaggedRef var);
  static void addName(TaggedRef var, const char *name);
};

const char *getVarName(TaggedRef v);

#endif
