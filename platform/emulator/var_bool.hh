/*
  Hydra Project, DFKI Saarbruecken,
  Stuhlsatzenhausweg 3, D-66123 Saarbruecken, Phone (+49) 681 302-5312
  Author: tmueller
  Last modified: $Date$ from $Author$
  Version: $Revision$
  State: $State$

  ------------------------------------------------------------------------
*/

#ifndef __GENBOOLVAR__H__
#define __GENBOOLVAR__H__

#if defined(__GNUC__)
#pragma interface
#endif

#include "genvar.hh"
#include "fdomn.hh"
#include "fdhook.hh"

#if defined(OUTLINE) || defined(FDOUTLINE)
#define inline
#endif

//-----------------------------------------------------------------------------
//                           class GenBoolVariable
//-----------------------------------------------------------------------------

class GenBoolVariable : public GenCVariable {

  friend class GenCVariable;
  friend inline void addSuspBoolVar(TaggedRef, SuspList *);

public:
  GenBoolVariable(void) : GenCVariable(BoolVariable) { }

  // methods relevant for term copying (gc and solve)
  size_t getSize(void){return sizeof(GenBoolVariable);}
  void dispose(void);

  Bool unifyBool(TaggedRef *, TaggedRef, TaggedRef *, TaggedRef,
                 Bool, Bool = TRUE);

  // is X=val still valid, i.e. is val an smallint and either 0 ro 1.
  Bool valid(TaggedRef val);

  void becomesSmallIntAndPropagate(TaggedRef * trPtr, FiniteDomain & fd);

  int getSuspListLength(void) { return suspList->length(); }

  void installPropagators(GenFDVariable *, Board *);

  void propagate(TaggedRef var, PropCaller prop_eq = pc_propagator) {
    if (suspList) GenCVariable::propagate(var, suspList, prop_eq);
  }
};

inline Bool isGenBoolVar(TaggedRef term);
inline Bool isGenBoolVar(TaggedRef term, TypeOfTerm tag);
inline GenBoolVariable * tagged2GenBoolVar(TaggedRef term);
inline void addSuspBoolVar(TaggedRef, SuspList *);


#if !defined(OUTLINE) && !defined(FDOUTLINE)
#include "fdbvar.icc"
#else
#undef inline
#endif

#endif
