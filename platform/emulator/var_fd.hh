/*
 *  Authors:
 *    Tobias Mueller (tmueller@ps.uni-sb.de)
 * 
 *  Contributors:
 *    optional, Contributor's name (Contributor's email address)
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

#ifndef __GENFDVAR__H__
#define __GENFDVAR__H__

#if defined(INTERFACE)
#pragma interface
#endif

#include "genvar.hh"
#include "fdomn.hh"
#include "fdhook.hh"

#ifdef OUTLINE 
#define inline
#endif

//-----------------------------------------------------------------------------
//                           class GenFDVariable
//-----------------------------------------------------------------------------

class GenFDVariable: public GenCVariable {

friend class GenCVariable;
friend class GenBoolVariable;
friend inline void addSuspFDVar(TaggedRef, Suspension, OZ_FDPropState);
  
private:
  OZ_FiniteDomain finiteDomain;
  SuspList * fdSuspList[fd_prop_any];
  
  void relinkSuspListToItself(Bool reset_local = FALSE);

  GenBoolVariable * becomesBool(void);
public:  
  GenFDVariable(OZ_FiniteDomain &fd) : GenCVariable(FDVariable) {
    ozstat.fdvarsCreated.incf();
    finiteDomain = fd;
    fdSuspList[fd_prop_singl] = fdSuspList[fd_prop_bounds] = NULL;
  }

  GenFDVariable(DummyClass *) : GenCVariable(FDVariable,(DummyClass*)0) {}

  GenFDVariable() : GenCVariable(FDVariable) {
    ozstat.fdvarsCreated.incf();
    finiteDomain.initFull();
    fdSuspList[fd_prop_singl] = fdSuspList[fd_prop_bounds] = NULL;
  }

  // methods relevant for term copying (gc and solve)
  void gc(GenFDVariable *); 
  inline void dispose(void);
  
  void becomesSmallIntAndPropagate(TaggedRef * trPtr);
  void becomesBoolVarAndPropagate(TaggedRef * trPtr);

  int intersectWithBool(void);

  // is X=val still valid, i.e. is val a smallint and is it still in the domain
  Bool valid(TaggedRef val);

  void setDom(OZ_FiniteDomain &fd) {
    Assert(fd != fd_bool);
    finiteDomain = fd;
  }
  OZ_FiniteDomain &getDom(void) {return finiteDomain;}

  void relinkSuspListTo(GenFDVariable * lv, Bool reset_local = FALSE);
  void relinkSuspListTo(GenBoolVariable * lv, Bool reset_local = FALSE);

  void propagate(OZ_FDPropState state,
		 PropCaller prop_eq = pc_propagator);  

  void propagateUnify() {
    propagate(fd_prop_singl, pc_cv_unif);
  }

  int getSuspListLength(void) {
    int len = suspList->length();
    for (int i = fd_prop_any; i--; )
      len += fdSuspList[i]->length();
    return len;
  }

  SuspList * getSuspList(int i) { return fdSuspList[i]; }

  void installPropagators(GenFDVariable *, Board *);



  OZ_Return unifyV(TaggedRef *, TaggedRef, ByteCode *);

  OZ_Return validV(TaggedRef* /* vPtr */, TaggedRef val ) {
    return valid(val);
  }
  GenCVariable* gcV() { error("not impl"); return 0; }
  void gcRecurseV() { error("not impl"); }
  void addSuspV(Suspension susp, TaggedRef* ptr, int state) {
    // mm2: addSuspFDVar(makeTaggedRef(ptr),susp,state);
  }
  void disposeV(void) { dispose(); }
  int getSuspListLengthV() { return getSuspListLength(); }
  void printStreamV(ostream &out,int depth = 10) {
    out << getDom().toString();
  }
  void printLongStreamV(ostream &out,int depth = 10,
			int offset = 0) {
    printStreamV(out,depth); out << endl;
  }
};

void addSuspFDVar(TaggedRef, Suspension, OZ_FDPropState = fd_prop_any);
OZ_Return tellBasicConstraint(OZ_Term, OZ_FiniteDomain *);

#ifndef OUTLINE 
#include "fdgenvar.icc"
#else
Bool isGenFDVar(TaggedRef term);
Bool isGenFDVar(TaggedRef term, TypeOfTerm tag);
GenFDVariable * tagged2GenFDVar(TaggedRef term);
#undef inline
#endif

#endif

// eof
//-----------------------------------------------------------------------------
