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

#include "cpi.hh"
#include "builtins.hh"

// gc.cc: OZ_Propagator * OZ_Propagator::gc(void)

// propagators use free list heap memory
void * OZ_Propagator::operator new(size_t s)
{
  return freeListMalloc(s);
}

void OZ_Propagator::operator delete(void * p, size_t s)
{
  freeListDispose(p, s);
}

static void outputArgsList(ostream& o, OZ_Term args, Bool not_top)
{
  Bool not_first = FALSE;
  if (not_top) o << '[';

  for (; OZ_isCons(args); args = OZ_tail(args)) {
    OZ_Term h = OZ_head(args);
    if (not_first) o << ' ';

    DEREF(h, hptr, htag);
    switch (htag) {

    case LITERAL:
      o << tagged2Literal(h)->getPrintName();
      break;

    case LTUPLE:
      outputArgsList(o, h, TRUE);
      break;

    case SRECORD:
      {
        SRecord * st = tagged2SRecord(h);
        if (st->isTuple()) {
          int width = st->getWidth();
          o << tagged2Literal(st->getLabel())->getPrintName() << '/' << width;
        }
      }
      break;
    case UVAR: case SVAR:
      o << '_';
      break;

    case SMALLINT:
      o << smallIntValue(h);
      break;

    case FSETVALUE:
      o << tagged2FSetValue(h)->toString();
      break;

    case CVAR:
      {
        o << getVarName(makeTaggedRef(hptr));

        GenCVariable * cv = tagged2CVar(h);

        if (cv->testReifiedFlag()) {
          if (cv->isBoolPatched()) goto bool_lbl;
          if (cv->isFDPatched()) goto fd_lbl;
          if (cv->isFSetPatched()) goto fs_lbl;
          /*Assert(cv->isFDPatched()); goto ri_lbl;*/
        } else if (cv->getType() == FDVariable) {
        fd_lbl:
          o << ((GenFDVariable *) cv)->getDom().toString();
        } else if (cv->getType() == BoolVariable) {
        bool_lbl:
          o << "{0#1}";
        } else if (cv->getType() == FSetVariable) {
        fs_lbl:
          o << ((GenFSetVariable *) cv)->getSet().toString();
        } else {
          goto problem;
        }
      }
      break;

    default:
      goto problem;
    }

    not_first = TRUE;
  }

  if (!OZ_isNil(args)) goto problem;
  if (not_top) o << ']' << flush;
  return;

problem:
  OZ_warning("Unexpected term found in argument list "
             "of propagator while printing %x, %x.", args, tagTypeOf(args));
}

ostream& operator << (ostream& o, const OZ_Propagator &p)
{
  const char * func_name = builtinTab.getName((void *) p.getHeader()->getHeaderFunc());
  OZ_Term args = p.getParameters();


  /*
#ifdef DEBUG_CHECK
  o << "cb(" << (void *) am.currentBoard << "), p(" << (void *) &p << ") ";
#endif
*/

  if (!p.isMonotonic())
    o << p.getOrder() << '#' << flush;

  o << '{' << func_name << ' ';
  outputArgsList(o, args, FALSE);
  o << '}' << flush;

  return o;
}

char *OZ_Propagator::toString() const
{
  ozstrstream str;
  str << (*this);
  return ozstrdup(str.str());
}


OZ_Boolean OZ_Propagator::mayBeEqualVars(void)
{
  return am.currentThread()->isUnifyThread();
}


OZ_Return OZ_Propagator::replaceBy(OZ_Propagator * p)
{
  am.currentThread()->setPropagator(p);
  return am.runPropagator(am.currentThread());
}

OZ_Return OZ_Propagator::replaceBy(OZ_Term a, OZ_Term b)
{
  return OZ_unify(a, b); // mm_u
}

OZ_Return OZ_Propagator::replaceByInt(OZ_Term v, int i)
{
  return OZ_unify(v, newSmallInt(i)); // mm_u
}

OZ_Return OZ_Propagator::postpone(void)
{
  return SCHEDULED;
}

OZ_Boolean OZ_Propagator::imposeOn(OZ_Term t)
{
  DEREF(t, tptr, ttag);
  if (isAnyVar(ttag)) {
    addSuspAnyVar(tptr, am.currentThread());
    return OZ_TRUE;
  }
  return OZ_FALSE;
}

OZ_Boolean OZ_Propagator::addImpose(OZ_FDPropState ps, OZ_Term v)
{
  DEREF(v, vptr, vtag);
  if (!isAnyVar(vtag))
    return FALSE;
  Assert(vptr);

  staticAddSpawnProp(ps, vptr);
  return TRUE;
}

void OZ_Propagator::impose(OZ_Propagator * p, int prio)
{
  Thread * thr = am.mkPropagator(am.currentBoard(), prio, p);
  ozstat.propagatorsCreated.incf();

  am.suspendPropagator(thr);
  am.propagatorToRunnable(thr);
  am.scheduleThreadInline(thr, thr->getPriority());

  OZ_Boolean all_local = OZ_TRUE;

  for (int i = staticSpawnVarsNumberProp; i--; ) {
    OZ_Term v = makeTaggedRef(staticSpawnVarsProp[i].var);
    DEREF(v, vptr, vtag);

    Assert(isAnyVar(vtag));

    Bool isStorePatched = 0, isReifiedPatched = 0, isBoolPatched = 0;
    OZ_FiniteDomain * tmp_fd = NULL;

    if (isCVar(vtag)) {
      isStorePatched = testResetStoreFlag(v);
      isReifiedPatched = testResetReifiedFlag(v);
      if (isReifiedPatched) {
        isBoolPatched = testBoolPatched(v);
        tmp_fd = unpatchReified(v, isBoolPatched);
      }
    }

    if (isGenFDVar(v, vtag)) {
      addSuspFDVar(v, thr, staticSpawnVarsProp[i].state.fd);
      all_local &= am.isLocalSVar(v);
    } else if (isGenOFSVar(v, vtag)) {
      addSuspOFSVar(v, thr);
      all_local &= am.isLocalSVar(v);
    } else if (isGenBoolVar(v, vtag)) {
      addSuspBoolVar(v, thr);
      all_local &= am.isLocalSVar(v);
    } else if (isSVar(vtag)) {
      addSuspSVar(v, thr);
      all_local &= am.isLocalSVar(v);
    } else {
      Assert(isUVar(vtag));
      addSuspUVar(vptr, thr);
      all_local &= am.isLocalUVar(v,vptr);
    }

    if (isCVar(vtag)) {
      if (isStorePatched)
        setStoreFlag(v);
      if (isReifiedPatched)
        patchReified(tmp_fd, v, isBoolPatched);
    }
  }

  if (all_local)
    thr->markLocalThread();

  staticSpawnVarsNumberProp = 0;
}

//-----------------------------------------------------------------------------
// class NonMonotonic

OZ_NonMonotonic::order_t OZ_NonMonotonic::_next_order = 1;

OZ_NonMonotonic::OZ_NonMonotonic(void) : _order(_next_order++)
{
  Assert(_next_order);

#ifdef DEBUG_NONMONOTONIC
    printf("New nonmono order: %d\n", _next_order-1); fflush(stdout);
#endif

}

// End of File
//-----------------------------------------------------------------------------
