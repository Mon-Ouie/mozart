/*
 *  Authors:
 *    Tobias Mueller (tmueller@ps.uni-sb.de)
 * 
 *  Contributors:
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

// I M P O R T A N T:
// This file defines the interface between the abstract machine
// and generic variables, which provide the basic functionality
// for concrete generic variables. The implementor of subclasses
// of GenCVariable is encouraged to include only this file and 
// files related to the constraint system concerned.

#ifndef __GENVAR__H__
#define __GENVAR__H__

#if defined(INTERFACE)
#pragma interface
#endif

#include "am.hh"
#include "variable.hh"
#include "pointer-marks.hh"

//#define DEBUG_TELLCONSTRAINTS

//-----------------------------------------------------------------------------
//                       Generic Constrained Variable
//-----------------------------------------------------------------------------


enum TypeOfGenCVariable {
  FDVariable,
  OFSVariable,
  MetaVariable, 
  BoolVariable,
  FSetVariable,
  PerdioVariable,
  LazyVariable,
  NonGenCVariable
};

#define GenVarCheckType(t)				\
    Assert(t == FDVariable || t == OFSVariable || 	\
	   t == MetaVariable || t == BoolVariable || 	\
	   t==PerdioVariable || t == FSetVariable || \
	   t ==LazyVariable)

class GenCVariable: public SVariable {

friend class GenFDVariable;
friend class GenFSetVariable;

private:
  union {
    TypeOfGenCVariable var_type;
    OZ_FiniteDomain *patchDomain;
    OZ_FSetConstraint *patchFSet;
  } u;

  enum u_mask_t {u_fd=0, u_bool=1, u_fset=2, u_ri=3, u_mask=3};
   
protected:
  
  void propagate(TaggedRef, SuspList * &, PropCaller);

public:
  USEFREELISTMEMORY;
  NO_DEFAULT_CONSTRUCTORS(GenCVariable);
  // the constructor creates per default a local variable (wrt curr. node)
  GenCVariable(TypeOfGenCVariable, Board * = NULL);

  TypeOfGenCVariable getType(void){ return u.var_type; }
  void setType(TypeOfGenCVariable t){
    GenVarCheckType(t);
    u.var_type = t;
  }  
    
  // methods relevant for term copying (gc and solve)  
  GenCVariable * gc(void);
  void gcRecurse(void);

  // unifies a generic variable with another generic variable
  // or a non-variable
  // invariant: left term == *this
  OZ_Return unify(TaggedRef *, TaggedRef *, TaggedRef, ByteCode *);
  OZ_Return unifyOutline(TaggedRef *, TaggedRef *, TaggedRef, ByteCode *);

  int getSuspListLength(void);

  // is X=val still valid
  Bool valid(TaggedRef *varPtr, TaggedRef val);
  int hasFeature(TaggedRef fea,TaggedRef *out);

  OZPRINTLONG;

  void installPropagators(GenCVariable *);

  void addDetSusp (Thread *thr, TaggedRef *tptr);

  void dispose(void);

  // needed to catch multiply occuring reified vars in propagators
  void patchReified(OZ_FiniteDomain * d, Bool isBool) { 
    u.patchDomain = d; 
    if (isBool) {
      u.patchDomain =  
	(OZ_FiniteDomain*) ToPointer(ToInt32(u.patchDomain) | u_bool);
    }
    setReifiedFlag();
  }
  void unpatchReified(Bool isBool) { 
    setType(isBool ? BoolVariable : FDVariable); 
    resetReifiedFlag();
  }
  OZ_Boolean isBoolPatched(void) { return (u.var_type & u_mask) == u_bool; }
  OZ_Boolean isFDPatched(void) { return (u.var_type & u_mask) == u_fd; }
  OZ_Boolean isFSetPatched(void) { return (u.var_type & u_mask) == u_fset; }
  OZ_Boolean isRIPatched(void) { return (u.var_type & u_mask) == u_ri; }
  OZ_FiniteDomain * getReifiedPatch(void) { 
    return (OZ_FiniteDomain *)  (u.var_type & ~u_mask); 
  }
  Bool isKinded() { return true; }
};


// only SVar and their descendants can be exclusive
inline
void setStoreFlag(OZ_Term t) 
{
  Assert(!isUVar(t) && oz_isVariable(t) && !oz_isRef(t));

  ((SVariable *) tagValueOf(t))->setStoreFlag();
}

inline
void setReifiedFlag(OZ_Term t) 
{
  Assert(!isUVar(t) && oz_isVariable(t) && !oz_isRef(t));

  ((SVariable *) tagValueOf(t))->setReifiedFlag();
}

inline
OZ_Boolean testReifiedFlag(OZ_Term t) 
{
  Assert(!isUVar(t) && oz_isVariable(t) && !oz_isRef(t));

  return ((SVariable *) tagValueOf(t))->testReifiedFlag();
}

inline
void patchReified(OZ_FiniteDomain * fd, OZ_Term t, Bool isBool)
{
  ((GenCVariable *) tagValueOf(t))->patchReified(fd, isBool);
}

inline
OZ_Boolean testBoolPatched(OZ_Term t) 
{
  Assert(!isUVar(t) && oz_isVariable(t) && !oz_isRef(t));

  return ((GenCVariable *) tagValueOf(t))->isBoolPatched();
}

inline
OZ_Boolean testResetStoreFlag(OZ_Term t) 
{
  Assert(!isUVar(t) && oz_isVariable(t) && !oz_isRef(t));

  return ((SVariable *) tagValueOf(t))->testResetStoreFlag();
}

inline
OZ_Boolean testStoreFlag(OZ_Term t) 
{
  Assert(!isUVar(t) && oz_isVariable(t) && !oz_isRef(t));

  return ((SVariable *) tagValueOf(t))->testStoreFlag();
}

inline
OZ_Boolean testResetReifiedFlag(OZ_Term t) 
{
  Assert(!isUVar(t) && oz_isVariable(t) && !oz_isRef(t));

  return ((SVariable *) tagValueOf(t))->testResetReifiedFlag();
}

inline
OZ_FiniteDomain * unpatchReified(OZ_Term t, Bool isBool) 
{
  Assert(!isUVar(t) && oz_isVariable(t) && !oz_isRef(t));
  GenCVariable * v = ((GenCVariable *) tagValueOf(t));
  
  v->unpatchReified(isBool);
  return v->getReifiedPatch();
}

void addSuspCVarOutline(TaggedRef *v, Thread *el, int unstable=TRUE);

#include "fsgenvar.hh"
#include "fdgenvar.hh"
#include "fdbvar.hh"
#include "ofgenvar.hh"
#include "metavar.hh"
#include "perdiovar.hh"
#include "lazyvar.hh"
#include "promise.hh"

#ifdef OUTLINE
void addSuspCVar(TaggedRef *v, Thread *el, int unstable=TRUE);
#else
#include "genvar.icc"
#endif


#endif

