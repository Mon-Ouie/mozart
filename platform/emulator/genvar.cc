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
 *     $MOZARTURL$
 *
 *  See the file "LICENSE" or
 *     $LICENSEURL$
 *  for information on usage and redistribution
 *  of this file, and for a DISCLAIMER OF ALL
 *  WARRANTIES.
 *
 */


#if defined(INTERFACE) && !defined(PEANUTS)
#pragma implementation "genvar.hh"
#endif

#include "genvar.hh"

GenCVariable::GenCVariable(TypeOfGenCVariable t, Board * n) :
SVariable(n == NULL ? am.currentBoard() : n)
{
  setType(t);
}


void GenCVariable::propagate(TaggedRef var, SuspList * &sl, PropCaller unifyVars)
{
  sl = am.checkSuspensionList(tagged2SVarPlus(var), sl, unifyVars);
}

OZ_Return GenCVariable::unifyOutline(TaggedRef *tptr1, TaggedRef *tptr2, TaggedRef term2, ByteCode *scp)
{
  return unify(tptr1,tptr2,term2, scp);
}

void addSuspCVarOutline(TaggedRef *v, Thread *el, int unstable)
{
  addSuspCVar(v, el, unstable);
}

#ifdef OUTLINE
#define inline
#include "genvar.icc"
#undef inline
#endif
