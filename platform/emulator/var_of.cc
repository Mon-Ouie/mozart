
/*
 *  Authors:
 *    Peter van Roy (pvr@info.ucl.ac.be)
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
  
#if defined(INTERFACE) && !defined(VAR_ALL)
#pragma implementation "var_of.hh"
#endif

#include "var_of.hh"

//-------------------------------------------------------------------------
//                               for class OzOFVariable
//-------------------------------------------------------------------------


// Check if there exists an S_ofs (Open Feature Structure) suspension
// in the suspList (Used only for monitorArity)
static
Bool hasOFSSuspension(SuspList * suspList)
{
  while (suspList) {
    Suspension susp = suspList->getSuspension();
    
    if (!susp.isDead() && susp.isPropagator() && susp.isOFSPropagator()) 
      return TRUE;

    suspList = suspList->getNext();
  }
  return FALSE;
}

OZ_Return OzOFVariable::bind(TaggedRef *vPtr, TaggedRef term, ByteCode *scp)
{
  Assert(!oz_isRef(term));
  TaggedRef bindInRecordCaseHack = term;
  TaggedRef var = *vPtr;
  TypeOfTerm tTag = tagTypeOf(term);
  switch (tTag) {
  case LITERAL:
    {
      // Literals have no features:
      if (getWidth()>0) return FALSE;

      // Get local/global flag:
      Bool vLoc=oz_isLocalVar(this);

      // Bind OFSVar to the Literal:
      if (vLoc) doBind(vPtr, term);
      else doBindAndTrail(vPtr, term);

      // Unify the labels:
      if (!oz_unify(term,label,scp)) return FALSE; // mm_u

      // Update the OFS suspensions:
      if (vLoc) addFeatOFSSuspensionList(var,suspList,makeTaggedNULL(),TRUE);

      // Propagate changes to the suspensions:
      // (this routine is actually OzVariable::propagate)
      if (scp==0) propagate(suspList, pc_cv_unif);

      // Take care of linking suspensions
      if (!vLoc) {
	// Add a suspension to the OZ_VAR_OF if it is global:
	// Suspension* susp=new Suspension(am.currentBoard);
	// Assert(susp!=NULL);
	// addSuspension(susp);
      }
      return TRUE;
    }

  case LTUPLE:
    {
      // Get the LTuple corresponding to term:
      LTuple* termLTup=tagged2LTuple(term);

      // Get local/global flag:
      Bool vLoc=oz_isLocalVar(this);

      // Check that var features are subset of {1,2}
      TaggedRef arg1=getFeatureValue(makeTaggedSmallInt(1));
      TaggedRef arg2=getFeatureValue(makeTaggedSmallInt(2));
      if ((arg1!=makeTaggedNULL())+(arg2!=makeTaggedNULL()) != getWidth())
	return FALSE;

      // Take care of OFS suspensions:
      if (vLoc && hasOFSSuspension(suspList)) {
	if (getWidth()<2) {
	  // Calculate feature or list of features 'flist' that are
	  // in LTUPLE and not in OFS.
	  TaggedRef flist=AtomNil;
	  if (!arg2) flist=oz_cons(makeTaggedSmallInt(2),flist);
	  if (!arg1) flist=oz_cons(makeTaggedSmallInt(1),flist);
	  // Add the extra features to S_ofs suspensions:
	  addFeatOFSSuspensionList(var,suspList,flist,TRUE);
	} else {
	  addFeatOFSSuspensionList(var,suspList,makeTaggedNULL(),TRUE);
	}
      }

      // Bind OFSVar to the LTuple:
      if (vLoc) doBind(vPtr, bindInRecordCaseHack);
      else doBindAndTrail(vPtr, bindInRecordCaseHack);

      // Unify the labels:
      if (!oz_unify(AtomCons,label,scp)) return FALSE; // mm_u

      // Unify corresponding feature values:
      if (arg1 && !oz_unify(termLTup->getHead(),arg1,scp)) return FALSE; // mm_u
      if (arg2 && !oz_unify(termLTup->getTail(),arg2,scp)) return FALSE; // mm_u

      // Propagate changes to the suspensions:
      // (this routine is actually OzVariable::propagate)
      if (scp==0) propagate(suspList, pc_cv_unif);
      return TRUE;
    }

  case SRECORD:
  Record:
  {
    // For all features of var, term should contain the feature.
    // Unify the values of corresponding features.
    // If success, bind the var to the SRECORD (with local/global distinction).

    // Get the SRecord corresponding to term:
    SRecord* termSRec=tagged2SRecord(term);
    Assert(termSRec!=NULL);

    // Get local/global flag:
    Bool vLoc=oz_isLocalVar(this);
  
    // Check that all features of the OFSVar exist in the SRecord:
    // (During the check, calculate the list of feature pairs that correspond.)
    PairList* pairs;
    Bool success=dynamictable->srecordcheck(*termSRec, pairs);
    if (!success) { pairs->free(); return FALSE; }

    // Take care of OFS suspensions:
    if (vLoc && hasOFSSuspension(suspList)) {
      if (termSRec->getWidth()>getWidth()) {
	// Calculate feature or list of features 'flist' that are in SRECORD
	// and not in OFS.
	TaggedRef flist = dynamictable->extraSRecFeatures(*termSRec);
	// Add the extra features to S_ofs suspensions:
	addFeatOFSSuspensionList(var,suspList,flist,TRUE);
      } else {
	addFeatOFSSuspensionList(var,suspList,makeTaggedNULL(),TRUE);
      }
    }

    // Bind OFSVar to the SRecord:
    if (vLoc) doBind(vPtr, bindInRecordCaseHack);
    else doBindAndTrail(vPtr, bindInRecordCaseHack);
  
    // Unify the labels:
    if (!oz_unify(termSRec->getLabel(),label,scp))  // mm_u
      { pairs->free(); return FALSE; }

    // Unify corresponding feature values:
    PairList* p=pairs;
    TaggedRef t1, t2;
    while (p->getpair(t1, t2)) {
      Assert(!p->isempty());
      if (oz_unify(t1, t2,scp)) { // mm_u
	// Unification successful
      } else {
	// Unification failed
	success=FALSE;
	break;
      }
      p->nextpair();
    }
    Assert(!success || p->isempty());
    pairs->free();
    if (!success) return FALSE;
    // At this point, unification is successful

    // Propagate changes to the suspensions:
    // (this routine is actually OzVariable::propagate)
    if (scp==0) propagate(suspList, pc_cv_unif);

    // Take care of linking suspensions
    if (!vLoc) {
      // Add a suspension to the OZ_VAR_OF if it is global:
      // Suspension* susp=new Suspension(am.currentBoard);
      // Assert(susp!=NULL);
      // addSuspension(susp);
    }
    return TRUE;
  }
  default:
    return FALSE;
  }
}

// (Arguments are dereferenced)
OZ_Return OzOFVariable::unify(TaggedRef *vPtr, TaggedRef *tPtr, ByteCode *scp)
{
  // var - var unification
  TaggedRef var = *vPtr;
  TaggedRef term = *tPtr;
  OzVariable *cv = tagged2CVar(term);
  if (cv->getType()!=OZ_VAR_OF) {
    return FALSE;
  }

  Assert(*tPtr!=*vPtr);

  // Get the OzOFVariable corresponding to term:
  OzOFVariable* termVar=(OzOFVariable *)cv;
  Assert(termVar!=NULL);

  // Get local/global flags:
  Bool vLoc=oz_isLocalVar(this);
  Bool tLoc=oz_isLocalVar(termVar);
  
  OzOFVariable* newVar=NULL;
  OzOFVariable* otherVar=NULL;
  TaggedRef* nvRefPtr=NULL;
  TaggedRef* otherPtr=NULL;
  long varWidth=getWidth();
  long termWidth=termVar->getWidth();
  if (vLoc && tLoc) {
    // Reuse the largest table (optimization to improve unification speed):
    if (varWidth>termWidth) {
      newVar=this;
      nvRefPtr=vPtr;
      otherVar=termVar; // otherVar must be smallest
      otherPtr=tPtr;
    } else {
      newVar=termVar;
      nvRefPtr=tPtr;
      otherVar=this; // otherVar must be smallest
      otherPtr=vPtr;
    }
  } else if (vLoc && !tLoc) {
    // Reuse the var:
    newVar=this;
    nvRefPtr=vPtr;
    otherVar=termVar;
  } else if (!vLoc && tLoc) {
    // Reuse the term:
    newVar=termVar;
    nvRefPtr=tPtr;
    otherVar=this;
  } else if (!vLoc && !tLoc) {
    // Reuse the largest table (this improves unification speed):
    if (varWidth>termWidth) {
      // Make a local copy of the var's DynamicTable:
      DynamicTable* dt=dynamictable->copyDynamicTable();
      // Make a new OzOFVariable with the new DynamicTable:
      newVar=new OzOFVariable(*dt,oz_currentBoard());
      nvRefPtr=newTaggedCVar(newVar);
      otherVar=termVar; // otherVar must be smallest
    } else {
      // Same as above, but in opposite order:
      DynamicTable* dt=termVar->getTable()->copyDynamicTable();
      newVar=new OzOFVariable(*dt,oz_currentBoard());
      nvRefPtr=newTaggedCVar(newVar);
      otherVar=this; // otherVar must be smallest
    }
  } else Assert(FALSE);
  Assert(nvRefPtr!=NULL);
  Assert(newVar!=NULL);
  Assert(otherVar!=NULL);
    
  // Take care of OFS suspensions, part 1/2 (before merging tables):
  Bool vOk=vLoc && hasOFSSuspension(suspList);
  TaggedRef vList = 0;
  if (vOk) {
    // Calculate the extra features in var:
    vList=termVar->dynamictable->extraFeatures(dynamictable);
  }
  Bool tOk=tLoc && hasOFSSuspension(termVar->suspList);
  TaggedRef tList = 0;
  if (tOk) {
    // Calculate the extra features in term:
    tList=dynamictable->extraFeatures(termVar->dynamictable);
  }

  // Merge otherVar's DynamicTable into newVar's DynamicTable.
  // (During the merge, calculate the list of feature pairs that correspond.)
  PairList* pairs;
  otherVar->dynamictable->merge(newVar->dynamictable, pairs);
  long mergeWidth=newVar->getWidth();

  // Take care of OFS suspensions, part 2/2 (after merging tables):
  if (vOk && (vList!=AtomNil /*mergeWidth>termWidth*/)) {
    // Add the extra features to S_ofs suspensions:
    addFeatOFSSuspensionList(var,suspList,vList,FALSE);
  }
  if (tOk && (tList!=AtomNil /*mergeWidth>varWidth*/)) {
    // Add the extra features to S_ofs suspensions:
    addFeatOFSSuspensionList(term,termVar->suspList,tList,FALSE);
  }

  // Bind both var and term to the (possibly reused) newVar:
  // Because of cycles, these bindings must be done _before_ the unification
  // If in glob/loc unification, the global is not constrained, then bind
  // the local to the global and relink the local's suspension list
  if (vLoc && tLoc) {
    // bind to var without trailing:
    doBind(otherPtr, makeTaggedRef(nvRefPtr));
  } else if (vLoc && !tLoc) {
    // Global term is constrained if result has more features than term:
    if (mergeWidth>termWidth) {
      DoBindAndTrailAndIP(tPtr, makeTaggedRef(vPtr),
			  newVar, otherVar);
    } else {
      doBind(vPtr, makeTaggedRef(tPtr));
    }
  } else if (!vLoc && tLoc) {
    // Global var is constrained if result has more features than var:
    if (mergeWidth>varWidth) {
      DoBindAndTrailAndIP(vPtr, makeTaggedRef(tPtr),
			  newVar, otherVar);
    } else {
      doBind(tPtr, makeTaggedRef(vPtr));
    }
  } else if (!vLoc && !tLoc) {
    // bind to new term with trailing:
    DoBindAndTrailAndIP(vPtr, makeTaggedRef(nvRefPtr),
			newVar, this);
    DoBindAndTrailAndIP(tPtr, makeTaggedRef(nvRefPtr),
			newVar, termVar);
  } else Assert(FALSE);

  // Unify the labels:
  if (!oz_unify(termVar->label,label,scp))  // mm_u
    { pairs->free(); return FALSE; }
  // Must be literal or variable:
  TaggedRef tmp=label;
  DEREF(tmp,_1,_2);
  if (!oz_isLiteral(tmp) && !oz_isVariable(tmp))
    { pairs->free(); return FALSE; }
  
  // Unify the corresponding feature values in the two variables:
  // Return FALSE upon encountering the first failing unification
  // Return TRUE if all unifications succeed
  PairList* p=pairs;
  Bool success=TRUE;
  TaggedRef t1, t2;
  while (p->getpair(t1, t2)) {
    Assert(!p->isempty());
    if (oz_unify(t1, t2, scp)) { // CAN ARGS BE _ANY_ TAGGEDREF* ?  // mm_u
      // Unification successful
    } else {
      // Unification failed
      success=FALSE;
      break;
    }
    p->nextpair();
  }
  Assert(!success || p->isempty());
  pairs->free();
  if (!success) return FALSE;
  // At this point, unification is successful

  // Propagate changes to the suspensions:
  // (this routine is actually OzVariable::propagate)
  if (scp==0) {
    propagate(suspList, pc_cv_unif);
    termVar->propagate(termVar->suspList, pc_cv_unif);
  }

  // Take care of linking suspensions
  if (vLoc && tLoc) {
    otherVar->relinkSuspListTo(newVar);
  } else if (vLoc && !tLoc) {
    if (mergeWidth>termWidth) {
      // Suspension* susp=new Suspension(am.currentBoard);
      // Assert(susp!=NULL);
      // termVar->addSuspension(susp);
    } else {
      relinkSuspListTo(termVar);
    }
  } else if (!vLoc && tLoc) {
    if (mergeWidth>varWidth) {
      // Suspension* susp=new Suspension(am.currentBoard);
      // Assert(susp!=NULL);
      // addSuspension(susp);
    } else {
      termVar->relinkSuspListTo(this);
    }
  } else if (!vLoc && !tLoc) {
    if (scp==0) {
      // Suspension* susp=new Suspension(am.currentBoard);
      // Assert(susp!=NULL);
      // termVar->addSuspension(susp);
      // addSuspension(susp);
    }
  } else Assert(FALSE);

  return TRUE;
}


// Return TRUE if OFS can't be constrained to l+tupleArity
Bool OzOFVariable::disentailed(Literal *l, int tupleArity) {
    TaggedRef tmp=label;
    DEREF(tmp,_1,_2);
    if (oz_isLiteral(tmp) && !literalEq(makeTaggedLiteral(l),tmp)) return TRUE;
    return (dynamictable->hasExtraFeatures(tupleArity));
}


// Return TRUE if OFS can't be constrained to l+recordArity
Bool OzOFVariable::disentailed(Literal *l, Arity *recordArity) {
    TaggedRef tmp=label;
    DEREF(tmp,_1,_2);
    if (oz_isLiteral(tmp) && !literalEq(makeTaggedLiteral(l),tmp)) return TRUE;
    return (dynamictable->hasExtraFeatures(recordArity));
}


Bool OzOFVariable::valid(TaggedRef val)
{
    if (!oz_isLiteral(val)) return FALSE;
    if (getWidth()>0) return FALSE;
    TaggedRef tmp=label;
    DEREF(tmp,_1,_2);
    if (oz_isLiteral(tmp) && !literalEq(tmp,val)) return FALSE;
    return TRUE;
}

TaggedRef OzOFVariable::getOpenArityList(TaggedRef* ftail, Board* hoome)
{
    return dynamictable->getOpenArityList(ftail,hoome);
}

TaggedRef OzOFVariable::getArityList()
{
    return dynamictable->getArityList();
}



// ---------------------------------------------------------------------
