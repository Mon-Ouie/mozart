/*
  Hydra Project, DFKI Saarbruecken,
  Stuhlsatzenhausweg 3, D-66123 Saarbruecken, Phone (+49) 681 302-5312
  Author: tmueller
  Last modified: $Date$ from $Author$
  Version: $Revision$
  State: $State$

  ------------------------------------------------------------------------
*/


#if defined(__GNUC__) && !defined(NOPRAGMA)
#pragma implementation "fdgenvar.hh"
#endif

#include "am.hh"
#include "genvar.hh"
#include "fdprofil.hh"

// unify expects either two GenFDVariables or at least one
// GenFDVariable and one non-variable
// invariant: left term (ie var)  == *this
// Only if a local variable is bound relink its suspension list, since
// global variables are trailed.(ie. their suspension lists are
// implicitely relinked.)
Bool GenFDVariable::unifyFD(TaggedRef *vPtr, TaggedRef var,
			    TaggedRef *tPtr, TaggedRef term,
			    Bool prop, Bool disp)
{
  TypeOfTerm tTag = tagTypeOf(term);
  
  switch (tTag){
  case SMALLINT:
    {
      if (! finiteDomain.contains(smallIntValue(term))) {
	PROFILE_CODE1(FDProfiles.inc_item(no_failed_fdunify_vars);)
	return FALSE;
      }
      if (prop) propagate(var, fd_det, pc_propagator);

      if (prop && am.isLocalSVar(this)) {
	doBind(vPtr, term);
	if (disp) dispose();
      } else {
	am.doBindAndTrail(var, vPtr, term);
      }
      
      PROFILE_CODE1(if (FDVarsTouched.add(term))
		      FDProfiles.inc_item(no_touched_vars);
		    FDProfiles.inc_item(no_succ_fdunify_vars);
		    )
      return TRUE;
    }
  case CVAR:
    {
      switch(tagged2CVar(term)->getType()) {
      case FDVariable: 
	{
	  // compute intersection of domains ...
	  GenFDVariable * termVar = tagged2GenFDVar(term);
	  FiniteDomain &termDom = termVar->finiteDomain;
	  LocalFD intsct;
	  
	  if ((intsct = finiteDomain & termDom) == fd_empty) {
	    PROFILE_CODE1(FDProfiles.inc_item(no_failed_fdunify_vars);)
	      return FALSE;
	  }
	  FDPropState l_dom = intsct.checkAgainst(finiteDomain);
	  FDPropState r_dom = intsct.checkAgainst(termDom);
	  
	  PROFILE_CODE1(if (l_dom != fd_any)
			if (FDVarsTouched.add(var))
			FDProfiles.inc_item(no_touched_vars);
			if (r_dom != fd_any)
			if (FDVarsTouched.add(term))
			FDProfiles.inc_item(no_touched_vars);
			FDProfiles.inc_item(no_succ_fdunify_vars);
			)
	    // bind - trail - propagate
	    Bool varIsLocal =  (prop && am.isLocalSVar(this));
	    Bool termIsLocal = (prop && am.isLocalSVar(termVar));
	    switch (varIsLocal + 2 * termIsLocal) {
	    case TRUE + 2 * TRUE: // var and term are local
	      {
		if (intsct == fd_singleton) {
		  TaggedRef int_var = newSmallInt(intsct.singl());
		  termVar->propagate(term, r_dom, pc_cv_unif);
		  propagate(var, l_dom, pc_cv_unif);
		  doBind(vPtr, int_var);
		  doBind(tPtr, int_var);
		  if (disp) { dispose(); termVar->dispose(); }
		} else if (heapNewer(vPtr, tPtr)) { // bind var to term
		  termVar->setDom(intsct);
		  propagate(var, l_dom, pc_cv_unif);
		  termVar->propagate(term, r_dom, pc_cv_unif);
		  relinkSuspListTo(termVar);
		  doBind(vPtr, makeTaggedRef(tPtr));
		  if (disp) dispose();
		} else { // bind term to var
		  setDom(intsct);
		  termVar->propagate(term, r_dom, pc_cv_unif);
		  propagate(var, l_dom, pc_cv_unif);
		  termVar->relinkSuspListTo(this);
		  doBind(tPtr, makeTaggedRef(vPtr));
		  if (disp) termVar->dispose();
		}
		break;
	      }
	    case TRUE + 2 * FALSE: // var is local and term is global
	      {
		if (intsct.getSize() != termDom.getSize()){
		  if (intsct == fd_singleton) {
		    TaggedRef int_var = newSmallInt(intsct.singl());
		    termVar->propagate(term, r_dom, pc_cv_unif);
		    propagate(var, l_dom, pc_cv_unif);
		    doBind(vPtr, int_var);
		    am.doBindAndTrail(term, tPtr, int_var);
		    if (disp) dispose();
		  } else {
		    setDom(intsct);
		    termVar->propagate(term, r_dom, pc_cv_unif);
		    propagate(var, l_dom, pc_cv_unif);
		    am.doBindAndTrailAndIP(term, tPtr, makeTaggedRef(vPtr),
					   this, termVar, prop);
		  }
		} else {
		  termVar->propagate(term, r_dom, pc_cv_unif);
		  propagate(var, l_dom, pc_cv_unif);
		  relinkSuspListTo(termVar, TRUE);
		  doBind(vPtr, makeTaggedRef(tPtr));
		  if (disp) dispose();
		}
		break;
	      }
	    case FALSE + 2 * TRUE: // var is global and term is local
	      {
		if (intsct.getSize() != finiteDomain.getSize()){
		  if(intsct == fd_singleton) {
		    TaggedRef int_term = newSmallInt(intsct.singl());
		    propagate(var, l_dom, pc_cv_unif);
		    termVar->propagate(term, r_dom, pc_cv_unif);
		    doBind(tPtr, int_term);
		    am.doBindAndTrail(var, vPtr, int_term);
		    if (disp) termVar->dispose();
		  } else {
		    termVar->setDom(intsct);
		    propagate(var, l_dom, pc_cv_unif);
		    termVar->propagate(term, r_dom, pc_cv_unif);
		    am.doBindAndTrailAndIP(var, vPtr, makeTaggedRef(tPtr),
					   termVar, this, prop);
		  }
		} else {
		  termVar->propagate(term, r_dom, pc_cv_unif);
		  propagate(var, l_dom, pc_cv_unif);
		  termVar->relinkSuspListTo(this, TRUE);
		  doBind(tPtr, makeTaggedRef(vPtr));
		  if (disp) termVar->dispose();
		}
		break;
	      }
	    case FALSE + 2 * FALSE: // var and term is global
	      {
		if (intsct == fd_singleton){
		  TaggedRef int_val = newSmallInt(intsct.singl());
		  if (prop) {
		    propagate(var, l_dom, pc_cv_unif);
		    termVar->propagate(term, r_dom, pc_cv_unif);
		  }
		  am.doBindAndTrail(var, vPtr, int_val);
		  am.doBindAndTrail(term, tPtr, int_val);
		} else {
		  GenCVariable * c_var = (intsct == fd_bool) ? new GenBoolVariable() : new GenFDVariable(intsct);
		  TaggedRef * var_val = newTaggedCVar(c_var);
		  if (prop) {
		    propagate(var, l_dom, pc_cv_unif);
		    termVar->propagate(term, r_dom, pc_cv_unif);
		  }
		  am.doBindAndTrailAndIP(var, vPtr, makeTaggedRef(var_val),
					 c_var, this, prop);
		  am.doBindAndTrailAndIP(term, tPtr, makeTaggedRef(var_val),
					 c_var, termVar, prop);
		}
		break;
	      }
	    default:
	      error("unexpected case in unifyFD");
	      break;
	    } // switch
	    return TRUE;
	} 
      case BoolVariable:
	{
	  return tagged2GenBoolVar(term)->unifyBool(tPtr, term, 
						    vPtr, var, 
						    prop, disp);
	}
      default:
	return FALSE;
      }
    default:
      break;
    }
  } // switch
  return FALSE;  
} // GenFDVariable::unify



Bool GenFDVariable::valid(TaggedRef val)
{
  Assert(!isRef(val));
  return (isSmallInt(val) && finiteDomain.contains(smallIntValue(val)));
}


void GenFDVariable::relinkSuspListTo(GenBoolVariable * lv, Bool reset_local)
{
  GenCVariable::relinkSuspListTo(lv, reset_local); // any
  for (int i = 0; i < fd_any; i += 1)
    fdSuspList[i] =
      fdSuspList[i]->appendToAndUnlink(lv->suspList, reset_local);
}


void GenFDVariable::relinkSuspListToItself(Bool reset_local)
{
  for (int i = 0; i < fd_any; i += 1)
    fdSuspList[i]->appendToAndUnlink(suspList, reset_local);
}


void GenFDVariable::becomesBoolVarAndPropagate(TaggedRef * trPtr)
{
  if (isGenBoolVar(*trPtr)) return;

  Assert(this == tagged2SuspVar(*trPtr));

  propagate(*trPtr, fd_bounds);
  becomesBool();
}


#if defined(OUTLINE) || defined(FDOUTLINE)
#define inline
#include "fdgenvar.icc"
#undef inline
#endif

