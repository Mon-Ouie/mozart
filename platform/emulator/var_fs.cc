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

#if defined(INTERFACE)
#pragma implementation "fsgenvar.hh"
#endif

#include "ozostream.hh"
#include "fddebug.hh"
#include "am.hh"
#include "genvar.hh"

Bool GenFSetVariable::valid(TaggedRef val)
{
  Assert(!oz_isRef(val));
  return (oz_isFSetValue(val) && ((FSetConstraint *) &_fset)->valid(*(FSetValue *)tagged2FSetValue(val)));
}

void GenFSetVariable::dispose(void) {
  suspList->disposeList();
  freeListDispose(this, sizeof(GenFSetVariable));
}

#ifdef DEBUG_FSET
//#define DEBUG_FSUNIFY
//#define DEBUG_TELLCONSTRAINTS
#endif


Bool GenFSetVariable::unifyFSet(OZ_Term * vptr, OZ_Term var,
                                OZ_Term * tptr, OZ_Term term,
                                ByteCode * scp, /* propagate */
                                Bool disp  /* dispose */)
{
  TypeOfTerm ttag = tagTypeOf(term);

  switch (ttag) {
  case FSETVALUE:
    {
#ifdef DEBUG_FSUNIFY
      (*cpi_cout) << "fsunify(value): (" << _fset.toString() << " = "
                << *((FSetValue *)tagged2FSetValue(term)) << " )";
#endif

      if (! ((FSetConstraint *) &_fset)->valid(*(FSetValue *)tagged2FSetValue(term)))
        goto f;

      Bool isLocalVar = am.isLocalSVar(this);
      Bool isNotInstallingScript = !am.isInstallingScript();

      if (scp==0 && (isNotInstallingScript || isLocalVar))
        propagate(var, fs_prop_val);

      if (isLocalVar) {
        doBind(vptr, term);
        if (disp) dispose();
      } else {
        am.doBindAndTrail(var, vptr, term);
      }

#ifdef DEBUG_FSUNIFY
      (*cpi_cout) << " -> " <<  _fset.toString();
#endif

      goto t;
    } // case FSETVALUE:
  case CVAR:
    {
      switch(tagged2CVar(term)->getType()) {
      case FSetVariable:
        {
          GenFSetVariable * term_var = tagged2GenFSetVar(term);
          OZ_FSetConstraint * t_fset = (OZ_FSetConstraint *) &term_var->getSet();
          OZ_FSetConstraint * fset = (OZ_FSetConstraint *) &getSet();
          OZ_FSetConstraint new_fset;

#ifdef DEBUG_FSUNIFY
          (*cpi_cout) << "fsunify(var): (" << *fset << " = " << *t_fset << " )";
#endif

          if ((new_fset = ((FSetConstraint *) t_fset)->unify(*(FSetConstraint *) fset)).getCardMin() == -1)
            goto f;

#ifdef DEBUG_FSUNIFY
          (*cpi_cout) << " -> " << new_fset << " " << new_fset.isValue();
#endif

          Bool var_is_local  = am.isLocalSVar(this);
          Bool term_is_local = am.isLocalSVar(term_var);
          Bool is_not_installing_script = !am.isInstallingScript();
          Bool var_is_constrained = (is_not_installing_script ||
                                     ((FSetConstraint *) fset)->isWeakerThan(*((FSetConstraint *) &new_fset)));
          Bool term_is_constrained = (is_not_installing_script ||
                                      ((FSetConstraint *) t_fset)->isWeakerThan(*((FSetConstraint *) &new_fset)));


          switch (var_is_local + 2 * term_is_local) {
          case TRUE + 2 * TRUE: // var and term are local
            {
              if (new_fset.isValue()) {
                OZ_Term new_fset_var = makeTaggedFSetValue(new FSetValue(*((FSetConstraint *) &new_fset)));
                term_var->propagateUnify(term);
                propagateUnify(var);
                doBind(vptr, new_fset_var);
                doBind(tptr, new_fset_var);
                if (disp) {
                  dispose();
                  term_var->dispose();
                }
              } else if (heapNewer(vptr, tptr)) { // bind var to term
                term_var->setSet(new_fset);
                propagateUnify(var);
                term_var->propagateUnify(term);
                relinkSuspListTo(term_var);
                doBind(vptr, makeTaggedRef(tptr));
                if (disp)
                  dispose();
              } else { // bind term to var
                setSet(new_fset);
                term_var->propagateUnify(term);
                propagateUnify(var);
                term_var->relinkSuspListTo(this);
                doBind(tptr, makeTaggedRef(vptr));
                if (disp)
                  term_var->dispose();
              }
              break;
            } // TRUE + 2 * TRUE:
          case TRUE + 2 * FALSE: // var is local and term is global
            {
              if (((FSetConstraint *) t_fset)->isWeakerThan(*((FSetConstraint *) &new_fset))) {
                if (new_fset.isValue()) {
                  OZ_Term new_fset_var = makeTaggedFSetValue(new FSetValue(*((FSetConstraint *) &new_fset)));
                  if (is_not_installing_script) term_var->propagateUnify(term);
                  if (var_is_constrained) propagateUnify(var);
                  doBind(vptr, new_fset_var);
                  am.doBindAndTrail(term, tptr, new_fset_var);
                  if (disp)
                    dispose();
                } else {
                  setSet(new_fset);
                  if (is_not_installing_script) term_var->propagateUnify(term);
                  if (var_is_constrained) propagateUnify(var);
                  am.doBindAndTrailAndIP(term, tptr, makeTaggedRef(vptr),
                                         this, term_var);
                }
              } else {
                if (is_not_installing_script) term_var->propagateUnify(term);
                if (var_is_constrained) propagateUnify(var);
                relinkSuspListTo(term_var, TRUE);
                doBind(vptr, makeTaggedRef(tptr));
                if (disp) dispose();
              }
              break;
            } // TRUE + 2 * FALSE:
          case FALSE + 2 * TRUE: // var is global and term is local
            {
              if (((FSetConstraint *) fset)->isWeakerThan(*((FSetConstraint *) &new_fset))) {
                if(new_fset.isValue()) {
                  OZ_Term new_fset_var = makeTaggedFSetValue(new FSetValue(*((FSetConstraint *) &new_fset)));
                  if (is_not_installing_script) propagateUnify(var);
                  if (term_is_constrained) term_var->propagateUnify(term);
                  doBind(tptr, new_fset_var);
                  am.doBindAndTrail(var, vptr, new_fset_var);
                  if (disp)
                    term_var->dispose();
                } else {
                  term_var->setSet(new_fset);
                  if (is_not_installing_script) propagateUnify(var);
                  if (term_is_constrained) term_var->propagateUnify(term);
                  am.doBindAndTrailAndIP(var, vptr, makeTaggedRef(tptr),
                                         term_var, this);
                }
              } else {
                if (term_is_constrained) term_var->propagateUnify(term);
                if (is_not_installing_script) propagateUnify(var);
                term_var->relinkSuspListTo(this, TRUE);
                doBind(tptr, makeTaggedRef(vptr));
                if (disp)
                  term_var->dispose();
              }
              break;
            } // FALSE + 2 * TRUE:
          case FALSE + 2 * FALSE: // var and term is global
            {
              if (new_fset.isValue()){
                OZ_Term new_fset_var = makeTaggedFSetValue(new FSetValue(*((FSetConstraint *) &new_fset)));
                if (scp==0) {
                  if (var_is_constrained) propagateUnify(var);
                  if (term_is_constrained) term_var->propagateUnify(term);
                }
                am.doBindAndTrail(var, vptr, new_fset_var);
                am.doBindAndTrail(term, tptr, new_fset_var);
              } else {
                GenCVariable *c_var = new GenFSetVariable(new_fset);
                TaggedRef * var_val = newTaggedCVar(c_var);
                if (scp==0) {
                  if (var_is_constrained) propagateUnify(var);
                  if (term_is_constrained) term_var->propagateUnify(term);
                }
                am.doBindAndTrailAndIP(var, vptr, makeTaggedRef(var_val),
                                       c_var, this);
                am.doBindAndTrailAndIP(term, tptr, makeTaggedRef(var_val),
                                       c_var, term_var);
              }
              break;
            } // FALSE + 2 * FALSE:
          default:
            error("unexpected case in unifyFSet");
            break;
          } // switch (varIsLocal + 2 * termIsLocal)
          goto t;
        } // case FSetVariable:
      default:
        goto f;
      } // switch(tagged2CVar(term)->getType())
    } // case CVAR;
  default:
    goto f;
  } // switch switch (ttag)
t:
#ifdef DEBUG_FSUNIFY

  (*cpi_cout) << toC(*vptr) << " true" << endl << flush;
#endif
  return TRUE;

f:
#ifdef DEBUG_FSUNIFY
  (*cpi_cout) << "false" << endl << flush;
#endif
  return FALSE;
}

OZ_Return tellBasicConstraint(OZ_Term v, OZ_FSetConstraint * fs)
{
#ifdef DEBUG_TELLCONSTRAINTS
  cout << "tellBasicConstraint - in - : ";
  oz_print(v);
  if (fs) cout << " , " << *fs;
  cout << endl <<flush;
#endif

  DEREF(v, vptr, vtag);

  if (fs && !((FSetConstraint *) fs)->isValid())
    goto failed;


// tell finite set constraint to unconstrained variable
  if (isNotCVar(vtag)) {
    if (! fs) goto fsvariable;

    // fs denotes a set value --> v becomes set value
    if (fs->isValue()) {
      if (am.isLocalVariable(v, vptr)) {
        if (isSVar(vtag))
          am.checkSuspensionList(v);
        doBind(vptr, makeTaggedFSetValue(new FSetValue(*(FSetConstraint *) fs)));
      } else {
        am.doBindAndTrail(v, vptr, makeTaggedFSetValue(new FSetValue(*(FSetConstraint *) fs)));
      }
      goto proceed;
    }

    // create finite set variable
  fsvariable:
    GenFSetVariable * fsv =
      fs ? new GenFSetVariable(*fs) : new GenFSetVariable();

    OZ_Term *  tfsv = newTaggedCVar(fsv);

    if (am.isLocalVariable(v, vptr)) {
      if (isSVar(vtag)) {
        am.checkSuspensionList(v);
        fsv->setSuspList(tagged2SVar(v)->getSuspList());
      }
      doBind(vptr, makeTaggedRef(tfsv));
    } else {
      am.doBindAndTrail(v, vptr, makeTaggedRef(tfsv));
    }

    goto proceed;
// tell finite set constraint to finite set variable
  } else if (isGenFSetVar(v, vtag)) {
    if (! fs) goto proceed;

    GenFSetVariable * fsvar = tagged2GenFSetVar(v);
    OZ_FSetConstraint set = ((FSetConstraint *) ((OZ_FSetConstraint *) &fsvar->getSet()))->unify(* (FSetConstraint *) fs);

    if (!((FSetConstraint *) &set)->isValid())
      goto failed;

    if (!((FSetConstraint *) &fsvar->getSet())->isWeakerThan(*((FSetConstraint *) &set)))
      goto proceed;

    if (set.isValue()) {
      if (am.isLocalSVar(v)) {
        fsvar->getSet() = set;
        fsvar->becomesFSetValueAndPropagate(vptr);
      } else {
        fsvar->propagate(v, fs_prop_val);
        am.doBindAndTrail(v, vptr, makeTaggedFSetValue(new FSetValue(*((FSetConstraint *) &set))));
      }
    } else {
      fsvar->propagate(v, fs_prop_bounds);
      if (am.isLocalSVar(v)) {
        fsvar->getSet() = set;
      } else {
        GenFSetVariable * locfsvar = new GenFSetVariable(set);
        OZ_Term * loctaggedfsvar = newTaggedCVar(locfsvar);
        am.doBindAndTrailAndIP(v, vptr,
                               makeTaggedRef(loctaggedfsvar),
                               locfsvar, tagged2GenFSetVar(v));
      }
    }
    goto proceed;
  } else if (isFSetValueTag(vtag)) {
    if (!fs) goto proceed;

    if (((FSetConstraint *) fs)->valid(*(FSetValue *) tagged2FSetValue(v)))
      goto proceed;
    goto failed;
  }

failed:

  return FAILED;

proceed:

#ifdef DEBUG_TELLCONSTRAINTS
  cout << "tellBasicConstraint - out - : ";
  if (vptr) oz_print(*vptr); else oz_print(v);
  if (fs) cout << " , " << *fs;
  cout << endl <<flush;
#endif

  return PROCEED;
}

#if defined(OUTLINE)
#define inline
#include "fsgenvar.icc"
#undef inline
#endif
