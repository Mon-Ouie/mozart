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

#if defined(__GNUC__)
#pragma interface
#endif

//-----------------------------------------------------------------------------
//                       Generic Constrained Variable
//-----------------------------------------------------------------------------


enum TypeOfGenCVariable {
  BeingTagged  = 0x1, // needs to occupy different bits than the rest
  FDVariable   = 0x2,
  OFSVariable  = 0x4,
  MetaVariable = 0x6
};

class GenCVariable: public SVariable {
private:
  TypeOfGenCVariable var_type;

protected:
  // takes the suspensionlist of var and  appends it to the
  // suspensionlist of leftVar
  void relinkSuspListTo(GenCVariable * lv, Bool reset_local = FALSE);

  void propagate(TaggedRef, SuspList * &, TaggedRef, PropCaller);

public:
  USEFREELISTMEMORY;

  // the constructor creates per default a local variable (wrt curr. node)
  GenCVariable(TypeOfGenCVariable, Board * = NULL);

  TypeOfGenCVariable getType(void){
    return TypeOfGenCVariable(var_type & ~BeingTagged);
  }
  void setType(TypeOfGenCVariable t){
    Assert(t == FDVariable || t == OFSVariable || t == MetaVariable);
    var_type = t;
  }

  void setTag(void) {
    var_type = TypeOfGenCVariable(var_type | BeingTagged);
  }
  Bool isTagged(void) {
    return (var_type & BeingTagged);
  }

  // methods relevant for term copying (gc and solve)
  void gc(void);
  size_t getSize(void);

  // unifies a generic variable with another generic variable
  // or a non-variable
  // invariant: left term == *this
  Bool unify(TaggedRef *, TaggedRef, TaggedRef *, TaggedRef, Bool);
  Bool unifyOutline(TaggedRef *, TaggedRef, TaggedRef *, TaggedRef, Bool);

  int getSuspListLength(void);

  // is X=val still valid
  Bool valid(TaggedRef val);

  void print(ostream &stream, int depth, int offset, TaggedRef v);
  void printLong(ostream &stream, int depth, int offset, TaggedRef v);

  void installPropagators(GenCVariable *, Bool prop);
};

#include "fdgenvar.hh"
#include "ofgenvar.hh"
#include "metavar.hh"

#ifndef OUTLINE
#include "genvar.icc"
#endif


#endif
