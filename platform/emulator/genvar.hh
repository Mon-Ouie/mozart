/*
  Hydra Project, DFKI Saarbruecken,
  Stuhlsatzenhausweg 3, D-66123 Saarbruecken, Phone (+49) 681 302-5312
  Author: tmueller
  Last modified: $Date$ from $Author$
  Version: $Revision$
  State: $State$

  ------------------------------------------------------------------------
*/

// I M P O R T A N T:
// This file defines the interface between the abstract machine
// and generic variables, which provide the basic functionality
// for concrete generic variables. The implementor of subclasses
// of GenCVariable is encouraged to include only this file and
// files related to the constraint system concerned.

#ifndef __GENVAR__H__
#define __GENVAR__H__

#ifdef __GNUC__
#pragma interface
#endif

#include "term.hh"
#include "indexing.hh"

//-----------------------------------------------------------------------------
//                       Generic Constrained Variable
//-----------------------------------------------------------------------------


#define ONLY_ONLY_FDVAR
enum TypeOfGenCVariable {
  FDVariable
};

class GenCVariable: public SVariable {
protected:
  TypeOfGenCVariable type;

  // takes the suspensionlist of var and  appends it to the
  // suspensionlist of leftVar
  inline void relinkSuspList(GenCVariable* leftVar);

  // moves appropriate suspension-list entries onto wake-up stack
  void propagate(TaggedRef, TaggedRef);
  void propagate(TaggedRef, TaggedRef*);
  SuspList* propagate(TaggedRef, SuspList* &, TaggedRef);
public:
  USEHEAPMEMORY;

  // the constructor creates per default a local variable (wrt curr. node)
  GenCVariable(TypeOfGenCVariable t,
               TaggedRef pn = AtomVoid,
               Board *n = NULL);

  TypeOfGenCVariable getType(void){return type;}

  // return OK, if var is local to the current node
  Bool isLocalVariable(void);

  // binds var to term and trails var, if it is global
  // var can be global/local GenCVariable
  // term can be local GenCVariable or an appropriate non-variable
  void bind(TaggedRef *vPtr, TaggedRef var,
            TaggedRef *tPtr, TaggedRef term);

  // methods relevant for term copying (gc and solve)
  void gc(void);
  size_t getSize(void);

  // unifies a generic variable with another generic variable or
  // or a non-variable
  // invariant: left term == *this
  Bool unify(TaggedRef*, TaggedRef, TypeOfTerm,
             TaggedRef*, TaggedRef, TypeOfTerm);

  // does indexing over constrained variables
  ProgramCounter index(ProgramCounter elseLabel, IHashTable* table);

  OZPRINT;
  OZPRINTLONG;

  static Bool unifyGenCVariables;
};


#ifndef OUTLINE
#include "genvar.icc"
#endif


#endif
