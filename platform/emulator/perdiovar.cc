/*
 *  Authors:
 *    Michael Mehl (mehl@dfki.de)
 * 
 *  Contributors:
 *    optional, Contributor's name (Contributor's email address)
 * 
 *  Copyright:
 *    Michael Mehl (1997)
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
#pragma implementation "perdiovar.hh"
#endif

#include "am.hh"
#include "genvar.hh"
#include "perdio.hh"

// bind and inform sites

// from perdio.cc
void bindPerdioVar(PerdioVar *pv,TaggedRef *lPtr,TaggedRef v);
int compareNetAddress(PerdioVar *lVar,PerdioVar *rVar);

void PerdioVar::primBind(TaggedRef *lPtr,TaggedRef v)
{
  setSuspList(am.checkSuspensionList(this, getSuspList(), pc_std_unif));

  TaggedRef vv=deref(v);
  if (isAnyVar(vv)) {
    Assert(isPerdioVar(vv));
    PerdioVar *pv=tagged2PerdioVar(vv);
    if (pv==this) return;
    pv->setSuspList(am.checkSuspensionList(pv, pv->getSuspList(),
					   pc_std_unif));
    relinkSuspListTo(pv);
  }
  doBind(lPtr, v);
  if (isObjectGName()) {
    deleteGName(u.gnameClass);
  }
}

GName *getGNameForUnify(TaggedRef val) {
  if (!isConst(val)) return 0;
  ConstTerm *c=tagged2Const(val);
  switch(c->getType()) {
  case Co_Abstraction:
  case Co_Class:
  case Co_Chunk:
    return ((ConstTermWithHome *)c)->getGName1();
  default:
    return 0;
  }
}

Bool PerdioVar::unifyPerdioVar(TaggedRef *lPtr, TaggedRef *rPtr, ByteCode *scp)
{
  TaggedRef rVal = *rPtr;
  TaggedRef lVal = *lPtr;

  Assert(!isNotCVar(rVal));
  
  PerdioVar *lVar = this;

  if (isPerdioVar(rVal)) {
    PerdioVar *rVar = tagged2PerdioVar(rVal);

    if (isObject() || isObjectGName()) {
      if (rVar->isObject() || rVar->isObjectGName()) {
	if (getGName() != rVar->getGName()) {
	  // the following is completely legal
	  // warning("mm2:gname mismatch (var-var)");
	  return FALSE;
	}
      }
      if (!rVar->isObject() && !rVar->isObjectGName()) {
	/*
	 * binding preferences
	 * bind perdiovar -> proxy
	 * bind url proxy -> object proxy
	 */
	Swap(rVal,lVal,TaggedRef);
	Swap(rPtr,lPtr,TaggedRef*);
      }
    }

    PD((PD_VAR,"unify i:%d i:%d",lVar->getIndex(),rVar->getIndex()));

    // Note: for perdio variables: am.isLocal == am.onToplevel
    if (scp!=0 || !am.isLocalSVar(lVar)) {
      // in any kind of guard then bind and trail
      am.checkSuspensionList(lVal,pc_std_unif);
      am.doBindAndTrail(lVal, lPtr,makeTaggedRef(rPtr));
      return TRUE;
    } else {
      // not in guard: distributed unification
      Assert(am.isLocalSVar(rVar));
      int cmp = compareNetAddress(lVar,rVar);
      Assert(cmp!=0);
      if (cmp<0) {
	bindPerdioVar(lVar,lPtr,makeTaggedRef(rPtr));
      } else {
	bindPerdioVar(rVar,rPtr,makeTaggedRef(lPtr));
      }
      return TRUE;
    }
  } // both PVARs


  // PVAR := non PVAR
  Assert(!isAnyVar(rVal));

  if (!valid(lPtr,rVal)) return FALSE;

  if (am.isLocalSVar(lVar)) {
    // onToplevel: distributed unification
    bindPerdioVar(lVar,lPtr,rVal);
    return TRUE;
  } else {
    // in guard: bind and trail
    am.checkSuspensionList(lVal,pc_std_unif);
    am.doBindAndTrail(lVal, lPtr,rVal);
    return TRUE;
  }
}

Bool PerdioVar::valid(TaggedRef *varPtr, TaggedRef v)
{
  Assert(!isRef(v) && !isAnyVar(v));

  if (isObject() || isObjectGName()) {
    if (getGName() != getGNameForUnify(v)) {
      return FALSE;
    }
  }
  return TRUE;
}

//-----------------------------------------------------------------------------
// Implementation of interface functions

OZ_BI_define(PerdioVar_is, 1,1)
{
  OZ_RETURN(isPerdioVar(deref(OZ_in(0)))?NameTrue:NameFalse);
}
	  
// ---------------------------------------------------------------------
// Distributed stuff
// ---------------------------------------------------------------------


static
BIspec pvarSpec[] = {
  {"PerdioVar.is",	      2, PerdioVar_is},

  {0,0,0,0}
};

void BIinitPerdioVar()
{
  BIaddSpec(pvarSpec);
}

