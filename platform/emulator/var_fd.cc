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

#if defined(INTERFACE) && !defined(PEANUTS)
#pragma implementation "fdgenvar.hh"
#endif

#include "fdgenvar.hh"
#include "fdbvar.hh"
#include "fdomn.hh"
#include "am.hh"
#include "threadInterface.hh"

// unify expects either two GenFDVariables or at least one
// GenFDVariable and one non-variable
// invariant: left term (ie var)  == *this
// Only if a local variable is bound relink its suspension list, since
// global variables are trailed.(ie. their suspension lists are
// implicitely relinked.)
OZ_Return GenFDVariable::unify(TaggedRef * vPtr, TaggedRef term, ByteCode *scp)
{
#ifdef SCRIPTDEBUG
  printf(am.isInstallingScript()
         ? "fd installing script\n"
         : "fd NOT installing script\n");
  fflush(stdout);
#endif

  if (oz_isSmallInt(term)) {
    if (! finiteDomain.isIn(OZ_intToC(term))) {
      return FALSE;
    }

    Bool isLocalVar = am.isLocalSVar(this);
    Bool isNotInstallingScript = !am.isInstallingScript();

#ifdef SCRIPTDEBUG
    printf("fd-int %s\n", isLocalVar ? "local" : "global"); fflush(stdout);
#endif

    if (scp==0 && (isNotInstallingScript || isLocalVar))
      propagate(fd_prop_singl);

    if (isLocalVar) {
      doBind(vPtr, term);
      dispose();
    } else {
      am.doBindAndTrail(vPtr, term);
    }

    return TRUE;
  }

  if (oz_isRef(term)) {
    TaggedRef *tPtr = tagged2Ref(term);
    term = *tPtr;
    GenCVariable *cv=tagged2CVar(term);
    if (cv->getType()!=FDVariable) return FAILED;

    // compute intersection of domains ...
    GenFDVariable * termVar = (GenFDVariable *)cv;
    OZ_FiniteDomain &termDom = termVar->finiteDomain;
    OZ_FiniteDomain intsct;

    if ((intsct = finiteDomain & termDom) == fd_empty) {
      return FALSE;
    }

    // bind - trail - propagate
    Bool varIsLocal =  am.isLocalSVar(this);
    Bool termIsLocal = am.isLocalSVar(termVar);

    Bool isNotInstallingScript = !am.isInstallingScript();
    Bool varIsConstrained = isNotInstallingScript ||
      (intsct.getSize() < finiteDomain.getSize());
    Bool termIsConstrained = isNotInstallingScript ||
      (intsct.getSize() < termDom.getSize());

    switch (varIsLocal + 2 * termIsLocal) {
    case TRUE + 2 * TRUE: // var and term are local
      {
#ifdef SCRIPTDEBUG
        printf("fd-fd local local\n"); fflush(stdout);
#endif
        if (intsct == fd_singl) {
          TaggedRef int_var = OZ_int(intsct.getSingleElem());
          termVar->propagateUnify();
          propagateUnify();
          doBind(vPtr, int_var);
          doBind(tPtr, int_var);
          dispose();
          termVar->dispose();
        } else if (heapNewer(vPtr, tPtr)) { // bind var to term
          if (intsct == fd_bool) {
            GenBoolVariable * tbvar = termVar->becomesBool();
            propagateUnify();
            tbvar->propagateUnify();
            relinkSuspListTo(tbvar);
            doBind(vPtr, makeTaggedRef(tPtr));
          } else {
            termVar->setDom(intsct);
            propagateUnify();
            termVar->propagateUnify();
            relinkSuspListTo(termVar);
            doBind(vPtr, makeTaggedRef(tPtr));
          }
          dispose();
        } else { // bind term to var
          if (intsct == fd_bool) {
            GenBoolVariable * bvar = becomesBool();
            termVar->propagateUnify();
            bvar->propagateUnify();
            termVar->relinkSuspListTo(bvar);
            doBind(tPtr, makeTaggedRef(vPtr));
          } else {
            setDom(intsct);
            termVar->propagateUnify();
            propagateUnify();
            termVar->relinkSuspListTo(this);
            doBind(tPtr, makeTaggedRef(vPtr));
          }
          termVar->dispose();
        }
        break;
      }
    case TRUE + 2 * FALSE: // var is local and term is global
      {
#ifdef SCRIPTDEBUG
        printf("fd-fd local global\n"); fflush(stdout);
#endif
        if (intsct.getSize() != termDom.getSize()){
          if (intsct == fd_singl) {
            TaggedRef int_var = OZ_int(intsct.getSingleElem());
            if (isNotInstallingScript) termVar->propagateUnify();
            if (varIsConstrained) propagateUnify();
            doBind(vPtr, int_var);
            am.doBindAndTrail(tPtr, int_var);
            dispose();
          } else {
            if (intsct == fd_bool) {
              GenBoolVariable * bvar = becomesBool();
              if (isNotInstallingScript) termVar->propagateUnify();
              if (varIsConstrained) bvar->propagateUnify();
              DoBindAndTrailAndIP(tPtr, makeTaggedRef(vPtr),
                                  bvar, termVar);
            } else {
              setDom(intsct);
              if (isNotInstallingScript) termVar->propagateUnify();
              if (varIsConstrained) propagateUnify();
              DoBindAndTrailAndIP(tPtr, makeTaggedRef(vPtr),
                                  this, termVar);
            }
          }
        } else {
          if (isNotInstallingScript) termVar->propagateUnify();
          if (varIsConstrained) propagateUnify();
          relinkSuspListTo(termVar, TRUE);
          doBind(vPtr, makeTaggedRef(tPtr));
          dispose();
        }
        break;
      }
    case FALSE + 2 * TRUE: // var is global and term is local
      {
#ifdef SCRIPTDEBUG
        printf("fd-fd global local\n"); fflush(stdout);
#endif
        if (intsct.getSize() != finiteDomain.getSize()){
          if(intsct == fd_singl) {
            TaggedRef int_term = OZ_int(intsct.getSingleElem());
            if (isNotInstallingScript) propagateUnify();
            if (termIsConstrained) termVar->propagateUnify();
            doBind(tPtr, int_term);
            am.doBindAndTrail(vPtr, int_term);
            termVar->dispose();
          } else {
            if (intsct == fd_bool) {
              GenBoolVariable * tbvar = termVar->becomesBool();
              if (isNotInstallingScript) propagateUnify();
              if (termIsConstrained) tbvar->propagateUnify();
              DoBindAndTrailAndIP(vPtr, makeTaggedRef(tPtr),
                                     tbvar, this);
            } else {
              termVar->setDom(intsct);
              if (isNotInstallingScript) propagateUnify();
              if (termIsConstrained) termVar->propagateUnify();
              DoBindAndTrailAndIP(vPtr, makeTaggedRef(tPtr),
                                  termVar, this);
            }
          }
        } else {
          if (termIsConstrained) termVar->propagateUnify();
          if (isNotInstallingScript) propagateUnify();
          termVar->relinkSuspListTo(this, TRUE);
          doBind(tPtr, makeTaggedRef(vPtr));
          termVar->dispose();
        }
        break;
      }
    case FALSE + 2 * FALSE: // var and term is global
      {
#ifdef SCRIPTDEBUG
        printf("fd-fd global global\n"); fflush(stdout);
#endif
        if (intsct == fd_singl){
          TaggedRef int_val = OZ_int(intsct.getSingleElem());
          if (scp==0) {
            if (varIsConstrained) propagateUnify();
            if (termIsConstrained) termVar->propagateUnify();
          }
          am.doBindAndTrail(vPtr, int_val);
          am.doBindAndTrail(tPtr, int_val);
        } else {
          if (intsct == fd_bool) {
            GenBoolVariable * c_var
              = new GenBoolVariable(oz_currentBoard());
            TaggedRef * var_val = newTaggedCVar(c_var);
            if (scp==0) {
              if (varIsConstrained) propagateUnify();
              if (termIsConstrained) termVar->propagateUnify();
            }
            DoBindAndTrailAndIP(vPtr, makeTaggedRef(var_val),
                                c_var, this);
            DoBindAndTrailAndIP(tPtr, makeTaggedRef(var_val),
                                c_var, termVar);
          } else {
            GenFDVariable * c_var
              = new GenFDVariable(intsct,oz_currentBoard());
            TaggedRef * var_val = newTaggedCVar(c_var);
            if (scp==0) {
              if (varIsConstrained) propagateUnify();
              if (termIsConstrained) termVar->propagateUnify();
            }
            DoBindAndTrailAndIP(vPtr, makeTaggedRef(var_val),
                                c_var, this);
            DoBindAndTrailAndIP(tPtr, makeTaggedRef(var_val),
                                c_var, termVar);
          }
        }
        break;
      }
    default:
      error("unexpected case in FD::unify");
      break;
    } // switch (varIsLocal + 2 * termIsLocal) {
    return TRUE;
  }

  return FALSE;
} // GenFDVariable::unify

Bool GenFDVariable::valid(TaggedRef val)
{
  Assert(!oz_isRef(val));
  return (oz_isSmallInt(val) && finiteDomain.isIn(OZ_intToC(val)));
}

void GenFDVariable::relinkSuspListTo(GenBoolVariable * lv, Bool reset_local)
{
  GenCVariable::relinkSuspListTo(lv, reset_local); // any
  for (int i = 0; i < fd_prop_any; i += 1)
    fdSuspList[i] =
      fdSuspList[i]->appendToAndUnlink(lv->suspList, reset_local);
}


void GenFDVariable::relinkSuspListToItself(Bool reset_local)
{
  for (int i = 0; i < fd_prop_any; i += 1)
    fdSuspList[i]->appendToAndUnlink(suspList, reset_local);
}


void GenFDVariable::becomesBoolVarAndPropagate(TaggedRef * trPtr)
{
  if (isGenBoolVar(*trPtr)) return;

  propagate(fd_prop_bounds);
  becomesBool();
}

int GenFDVariable::intersectWithBool(void)
{
  return ((OZ_FiniteDomainImpl *) &finiteDomain)->intersectWithBool();
}

OZ_Return tellBasicConstraint(OZ_Term v, OZ_FiniteDomain * fd)
{
#ifdef DEBUG_TELLCONSTRAINTS
  cout << "tellBasicConstraint - in - : ";
  oz_print(v);
  if (fd) cout << " , " << *fd;
  cout << endl <<flush;
#endif

  DEREF(v, vptr, vtag);

  if (fd && (*fd == fd_empty))
    goto failed;


// tell finite domain constraint to unconstrained variable
  if (oz_isFree(v)) {
    if (! fd) goto fdvariable;

    // fd is singleton domain --> v becomes integer
    if (fd->getSize() == 1) {
      if (am.isLocalVariable(v, vptr)) {
        // mm2: was isSVar
        if (!isUVar(vtag))
          oz_checkSuspensionList(tagged2SVarPlus(v));
        doBind(vptr, OZ_int(fd->getSingleElem()));
      } else {
        am.doBindAndTrail(vptr, OZ_int(fd->getSingleElem()));
      }
      goto proceed;
    }

    GenCVariable * cv;

    // create appropriate constrained variable
    if (*fd == fd_bool) {
      cv = (GenCVariable *) new GenBoolVariable(oz_currentBoard());
    } else {
    fdvariable:
      cv = (GenCVariable *) fd ? new GenFDVariable(*fd,oz_currentBoard())
        : new GenFDVariable(oz_currentBoard());
    }
    OZ_Term *  tcv = newTaggedCVar(cv);

    if (am.isLocalVariable(v, vptr)) {
      // mm2: was isSVar
      if (!isUVar(vtag)) {
        oz_checkSuspensionList(tagged2SVarPlus(v));
        cv->setSuspList(tagged2SVarPlus(v)->getSuspList());
      }
      doBind(vptr, makeTaggedRef(tcv));
    } else {
      am.doBindAndTrail(vptr, makeTaggedRef(tcv));
    }

    goto proceed;
// tell finite domain constraint to finite domain variable
  } else if (isGenFDVar(v, vtag)) {
    if (! fd) goto proceed;

    GenFDVariable * fdvar = tagged2GenFDVar(v);
    OZ_FiniteDomain dom = (fdvar->getDom() & *fd);

    if (dom == fd_empty)
      goto failed;

    if (dom.getSize() == fdvar->getDom().getSize())
      goto proceed;

    if (dom == fd_singl) {
      if (am.isLocalSVar(v)) {
        fdvar->getDom() = dom;
        fdvar->becomesSmallIntAndPropagate(vptr);
      } else {
        int singl = dom.getSingleElem();
        fdvar->propagate(fd_prop_singl);
        am.doBindAndTrail(vptr, OZ_int(singl));
      }
    } else if (dom == fd_bool) {
      if (am.isLocalSVar(v)) {
        fdvar->becomesBoolVarAndPropagate(vptr);
      } else {
        fdvar->propagate(fd_prop_bounds);
        GenBoolVariable * newboolvar = new GenBoolVariable(oz_currentBoard());
        OZ_Term * newtaggedboolvar = newTaggedCVar(newboolvar);
        DoBindAndTrailAndIP(vptr, makeTaggedRef(newtaggedboolvar),
                            newboolvar, tagged2GenBoolVar(v));
      }
    } else {
      fdvar->propagate(fd_prop_bounds);
      if (am.isLocalSVar(v)) {
        fdvar->getDom() = dom;
      } else {
        GenFDVariable * locfdvar = new GenFDVariable(dom,oz_currentBoard());
        OZ_Term * loctaggedfdvar = newTaggedCVar(locfdvar);
        DoBindAndTrailAndIP(vptr, makeTaggedRef(loctaggedfdvar),
                            locfdvar, tagged2GenFDVar(v));
      }
    }
    goto proceed;
// tell finite domain constraint to boolean finite domain variable
  } else if (isGenBoolVar(v, vtag)) {
    if (! fd) goto proceed;

    int dom = fd->intersectWithBool();

    if (dom == -2) goto failed;
    if (dom == -1) goto proceed;

    GenBoolVariable * boolvar = tagged2GenBoolVar(v);
    if (am.isLocalSVar(v)) {
      boolvar->becomesSmallIntAndPropagate(vptr, dom);
    } else {
      boolvar->propagate();
      am.doBindAndTrail(vptr, OZ_int(dom));
    }
    goto proceed;
// tell finite domain constraint to integer, i.e. check for compatibility
  } else if (isSmallIntTag(vtag)) {
    if (! fd) goto proceed;

    if (fd->isIn(smallIntValue(v)))
      goto proceed;
  }

failed:

  return FAILED;

proceed:

#ifdef DEBUG_TELLCONSTRAINTS
  cout << "tellBasicConstraint - out - : ";
  if (vptr) oz_print(*vptr); else oz_print(v);
  if (fd) cout << " , " << *fd;
  cout << endl <<flush;
#endif

  return PROCEED;
}

// inline DISABLED CS
void GenFDVariable::propagate(OZ_FDPropState state,
                              PropCaller prop_eq)
{
  if (prop_eq == pc_propagator) {
    switch (state) {
    case fd_prop_singl: // no break
      if (fdSuspList[fd_prop_singl])
        GenCVariable::propagate(fdSuspList[fd_prop_singl], prop_eq);
    case fd_prop_bounds: // no break
      if (fdSuspList[fd_prop_bounds])
        GenCVariable::propagate(fdSuspList[fd_prop_bounds], prop_eq);
    default:
      break;
    }
  } else {
    GenCVariable::propagate(fdSuspList[fd_prop_singl], prop_eq);
    GenCVariable::propagate(fdSuspList[fd_prop_bounds], prop_eq);
  }
  if (suspList)
    GenCVariable::propagate(suspList, prop_eq);
}


#ifdef OUTLINE
#define inline
#include "fdgenvar.icc"
#undef inline
#endif
