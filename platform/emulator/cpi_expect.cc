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

#include "cpi.hh"
#include "allgenvar.hh"
#include "prop_int.hh"

inline
OZ_expect_t expectFail(void) {
  return OZ_expect_t(0, -1);
}

inline
OZ_expect_t expectProceed(int s, int a) {
  return OZ_expect_t(s, a);
}

inline
OZ_expect_t expectSuspend(int s, int a)
{
  Assert(s > a);
  return OZ_expect_t(s, a);
}

inline
OZ_expect_t expectExceptional(void) {
  return OZ_expect_t(0, -2);
}

// expect_t.accepted:
//   -1 on failure, at least one term is inconsistent
//    0 on unsuffient constraints
// >= 1 number of terms sufficiently constrained
// args are sufficiently constrained if expect_t.size == expect_t.accepted

//-----------------------------------------------------------------------------

OZ_Expect::OZ_Expect(void)
{
  staticSpawnVarsNumber = staticSuspendVarsNumber = 0;
  collect = TRUE;
}

OZ_Expect::~OZ_Expect(void)
{
}

void OZ_Expect::collectVarsOn(void)
{
  collect = TRUE;
}

void OZ_Expect::collectVarsOff(void)
{
  collect = FALSE;
}

// variables are suffiently constrained
inline
void OZ_Expect::addSpawnBool(OZ_Term * v)
{
  if (collect)
    staticAddSpawnBool(v);
}

inline
void OZ_Expect::addSpawn(OZ_FDPropState ps, OZ_Term * v)
{
  if (collect)
    staticAddSpawn(ps, v);
}

inline
void OZ_Expect::addSpawn(OZ_FSetPropState ps, OZ_Term * v)
{
  if (collect)
    staticAddSpawn(ps, v);
}

inline
void OZ_Expect::addSpawn(OZ_CtDefinition * def, OZ_CtWakeUp w, OZ_Term * v)
{
  if (collect)
    staticAddSpawn(def, w, v);
}

// variables are insuffiently constrained

inline
void OZ_Expect::addSuspend(OZ_Term * v)
{
  if (collect) {
    staticSuspendVars[staticSuspendVarsNumber].var = v;
    DebugCode(staticSuspendVars[staticSuspendVarsNumber].expected_type = OZ_VAR_INVALID);
    staticSuspendVarsNumber++;

    staticSuspendVars.request(staticSuspendVarsNumber);
  }
}

// boolean finite domain constraints
inline
void OZ_Expect::addSuspendBool(OZ_Term * v)
{
  if (collect) {
    staticSuspendVars[staticSuspendVarsNumber].var = v;
    staticSuspendVars[staticSuspendVarsNumber].expected_type = BoolVariable;
    staticSuspendVarsNumber++;

    staticSuspendVars.request(staticSuspendVarsNumber);
  }
}

// finite domain constraints
inline
void OZ_Expect::addSuspend(OZ_FDPropState ps, OZ_Term * v)
{
  if (collect) {
    staticSuspendVars[staticSuspendVarsNumber].var = v;
    staticSuspendVars[staticSuspendVarsNumber].expected_type = FDVariable;
    staticSuspendVars[staticSuspendVarsNumber++].state.fd = ps;

    staticSuspendVars.request(staticSuspendVarsNumber);
  }
}

// finite set constraints
inline
void OZ_Expect::addSuspend(OZ_FSetPropState ps, OZ_Term * v)
{
  if (collect) {
    staticSuspendVars[staticSuspendVarsNumber].var = v;
    staticSuspendVars[staticSuspendVarsNumber].expected_type = FSetVariable;
    staticSuspendVars[staticSuspendVarsNumber++].state.fs = ps;

    staticSuspendVars.request(staticSuspendVarsNumber);
  }
}

// generic constraints
inline
void OZ_Expect::addSuspend(OZ_CtDefinition * def, OZ_CtWakeUp w, OZ_Term * v)
{;
  if (collect) {
    staticSuspendVars[staticSuspendVarsNumber].var = v;
    staticSuspendVars[staticSuspendVarsNumber].expected_type = CtVariable;
    staticSuspendVars[staticSuspendVarsNumber].state.ct.def = def;
    staticSuspendVars[staticSuspendVarsNumber++].state.ct.w = w;

    staticSuspendVars.request(staticSuspendVarsNumber);
  }
}

//-----------------------------------------------------------------------------
// generic constraint variable

OZ_expect_t OZ_Expect::expectGenCtVar(OZ_Term t,
                                      OZ_CtDefinition * def,
                                      OZ_CtWakeUp w)
{
  DEREF(t, tptr, ttag);

  if (def->isValidValue(t)) {
    return expectProceed(1, 1);
  } else if (isGenCtVar(t, ttag)) {
    GenCtVariable * ctvar = tagged2GenCtVar(t);

    // check the kind
    if (ctvar->getDefinition()->getKind() != def->getKind())
      goto fail;

    addSpawn(def, w, tptr);
    return expectProceed(1, 1);
  } else if (oz_isFree(t)) {
    addSuspend(def, w, tptr);
    return expectSuspend(1, 0);
  } else if (oz_isNonKinded(t)) {
    addSuspend(def, w, tptr);
    return expectExceptional();
  }

fail:
  return expectFail();
}



//*****************************************************************************
// member functions related to finite domain constraints
//*****************************************************************************

//-----------------------------------------------------------------------------
// An OZ term describing a finite domain is either:
// (1) a positive small integer <= FD.sup
// (2) a 2 tuple of (1)
// (3) a non-empty list of (1) and/or (2) or (1) or (2)
// (4) a tuple compl() of 4

OZ_expect_t OZ_Expect::expectDomDescr(OZ_Term descr, int level)
{
  DEREF(descr, descr_ptr, descr_tag);

  if (level >= 4) {
    if (oz_isFree(descr)||oz_isKinded(descr)) {
      addSuspend(descr_ptr);
      return expectSuspend(1, 0);
    } else if (oz_isSTuple(descr) && tagged2SRecord(descr)->getWidth() == 1 &&
               AtomCompl == tagged2SRecord(descr)->getLabel()) {
      //mm2: use tagged2Ref
      return expectDomDescr(makeTaggedRef(&(*tagged2SRecord(descr))[0]), 3);
    } else if (oz_isVariable(descr)) {
      addSuspend(descr_ptr);
      return expectExceptional();
    }
    level = 3;
  }

  if (isPosSmallFDInt(descr) && (level >= 1)) { // (1)
    return expectProceed(1, 1);
  } else if (isGenFDVar(descr, descr_tag) && (level >= 1)) {
    addSuspend(fd_prop_singl, descr_ptr);
    return expectSuspend(1, 0);
  } else if (isGenBoolVar(descr, descr_tag) && (level >= 1)) {
    addSuspend(descr_ptr);
    return expectSuspend(1, 0);
  } else if (oz_isSTuple(descr) && (level >= 2)) {
    SRecord &tuple = *tagged2SRecord(descr);
    if (tuple.getWidth() != 2)
      return expectFail();
    for (int i = 0; i < 2; i++) {
      //mm2: use tagged2Ref
      OZ_expect_t r = expectDomDescr(makeTaggedRef(&tuple[i]), 1);
      if (isSuspending(r) || isFailing(r) || isExceptional(r))
        return r;
    }
    return expectProceed(1, 1);
  } else if (isLTupleTag(descr_tag) && (level == 3)) {

    do {
      LTuple &list = *tagged2LTuple(descr);
      //mm2: use tagged2Ref
      OZ_expect_t r = expectDomDescr(makeTaggedRef(list.getRefHead()), 2);
      if (isSuspending(r) || isFailing(r) || isExceptional(r))
        return r;
      //mm2: use tagged2Ref
      descr = makeTaggedRef(list.getRefTail());

      __DEREF(descr, descr_ptr, descr_tag);
    } while (isLTupleTag(descr_tag));

    if (oz_isNil(descr)) return expectProceed(1, 1);
    return expectDomDescr(makeTaggedRef(descr_ptr), 0);
  } else if (oz_isFree(descr)||oz_isKinded(descr)) {
    addSuspend(descr_ptr);
    return expectSuspend(1, 0);
  } else if (oz_isVariable(descr)) {
    addSuspend(descr_ptr);
    return expectExceptional();
  }
  return expectFail();
}

// tmueller: not ready yet!
OZ_expect_t OZ_Expect::expectBoolVar(OZ_Term t)
{
  DEREF(t, tptr, ttag);

  if (isPosSmallBoolInt(t)) {
    return expectProceed(1, 1);
  } else if (isGenBoolVar(t, ttag)) {
    addSpawnBool(tptr);
    return expectProceed(1, 1);
  } else if (isGenFDVar(t, ttag)) {
    if (tellBasicBoolConstraint(makeTaggedRef(tptr)) == FAILED)
      return expectFail();
    addSpawnBool(tptr);
    return expectProceed(1, 1);
  } else if (oz_isFree(t) || oz_isKinded(t)) {
    addSuspendBool(tptr);
    return expectSuspend(1, 0);
  } else if (oz_isNonKinded(t)) {
    addSuspendBool(tptr);
    return expectExceptional();
  }
  return expectFail();
}

OZ_expect_t OZ_Expect::expectIntVar(OZ_Term t, OZ_FDPropState ps)
{
  DEREF(t, tptr, ttag);

  if (isPosSmallFDInt(t)) {
    return expectProceed(1, 1);
  } else if (isGenBoolVar(t, ttag) || isGenFDVar(t, ttag)) {
    addSpawn(ps, tptr);
    return expectProceed(1, 1);
  } else if (oz_isFree(t) || oz_isKinded(t)) {
    addSuspend(ps, tptr);
    return expectSuspend(1, 0);
  } else if (oz_isNonKinded(t)) {
    addSuspend(ps, tptr);
    return expectExceptional();
  }
  return expectFail();
}

OZ_expect_t OZ_Expect::expectInt(OZ_Term t)
{
  Assert(oz_isRef(t) || !oz_isVariable(t));

  DEREF(t, tptr, ttag);

  if (isSmallIntTag(ttag)) {
    return expectProceed(1, 1);
  } else if (oz_isFree(t)|| oz_isKinded(t)) {
    addSuspend(fd_prop_singl, tptr);
    return expectSuspend(1, 0);
  } else if (oz_isNonKinded(t)) {
    addSuspend(fd_prop_singl, tptr);
    return expectExceptional();
  }
  return expectFail();
}

OZ_expect_t OZ_Expect::expectFloat(OZ_Term t)
{
  Assert(oz_isRef(t) || !oz_isVariable(t));

  DEREF(t, tptr, ttag);

  if (isFloatTag(ttag)) {
    return expectProceed(1, 1);
  } else if (oz_isFree(t)|| oz_isKinded(t)) {
    addSuspend(tptr);
    return expectSuspend(1, 0);
  } else if (oz_isNonKinded(t)) {
    addSuspend(tptr);
    return expectExceptional();
  }
  return expectFail();
}

//*****************************************************************************
// member functions related to finite set constraints
//*****************************************************************************

//-----------------------------------------------------------------------------
// An OZ term describing a finite set is either:
// (1) a positive small integer <= FS.sup
// (2) a 2 tuple of (1)
// (3) a possibly empty list of (1) and/or (2)

OZ_expect_t OZ_Expect::_expectFSetDescr(OZ_Term descr, int level)
{
  DEREF(descr, descr_ptr, descr_tag);

  if (isPosSmallSetInt(descr) && (level == 1 || level == 2)) { // (1)
    return expectProceed(1, 1);
  } else if (isGenFDVar(descr, descr_tag) && (level == 1 || level == 2)) {
    addSuspend(fd_prop_singl, descr_ptr);
    return expectSuspend(1, 0);
  } else if (isGenBoolVar(descr, descr_tag) && (level == 1 || level == 2)) {
    addSuspend(descr_ptr);
    return expectSuspend(1, 0);
  } else if (oz_isSTuple(descr) && (level == 2)) {
    SRecord &tuple = *tagged2SRecord(descr);
    if (tuple.getWidth() != 2)
      return expectFail();
    for (int i = 0; i < 2; i++) {
      //mm2: use tagged2Ref
      OZ_expect_t r = expectDomDescr(makeTaggedRef(&tuple[i]), 1);
      if (isSuspending(r) || isFailing(r) || isExceptional(r))
        return r;
    }
    return expectProceed(1, 1);
  } else if (oz_isNil(descr) && (level == 3)) {
    return expectProceed(1, 1);
  } else if (isLTupleTag(descr_tag) && (level == 3)) {

    do {
      LTuple &list = *tagged2LTuple(descr);
      //mm2: use tagged2Ref
      OZ_expect_t r = expectDomDescr(makeTaggedRef(list.getRefHead()), 2);
      if (isSuspending(r) || isFailing(r) || isExceptional(r))
        return r;
      //mm2: use tagged2Ref
      descr = makeTaggedRef(list.getRefTail());

      __DEREF(descr, descr_ptr, descr_tag);
    } while (isLTupleTag(descr_tag));

    if (oz_isNil(descr)) return expectProceed(1, 1);
    return expectDomDescr(makeTaggedRef(descr_ptr), 0);
  } else if (oz_isFree(descr) || oz_isKinded(descr)) {
    addSuspend(descr_ptr);
    return expectSuspend(1, 0);
  } else if (oz_isNonKinded(descr)) {
    addSuspend(descr_ptr);
    return expectExceptional();
  }
  return expectFail();
}

OZ_expect_t OZ_Expect::expectFSetVar(OZ_Term t, OZ_FSetPropState ps)
{
  DEREF(t, tptr, ttag);

  if (isFSetValueTag(ttag)) {
    return expectProceed(1, 1);
  } else if (isGenFSetVar(t, ttag)) {
    addSpawn(ps, tptr);
    return expectProceed(1, 1);
  } else if (oz_isFree(t) || oz_isKinded(t)) {
    addSuspend(ps, tptr);
    return expectSuspend(1, 0);
  } else if (oz_isNonKinded(t)) {
    addSuspend(ps, tptr);
    return expectExceptional();
  }
  return expectFail();
}

OZ_expect_t OZ_Expect::expectFSetValue(OZ_Term t)
{
  Assert(oz_isRef(t) || !oz_isVariable(t));

  DEREF(t, tptr, ttag);

  if (isFSetValueTag(ttag)) {
    return expectProceed(1, 1);
  } else if (oz_isFree(t)||oz_isKinded(t)) {
    addSuspend(fs_prop_val, tptr);
    return expectSuspend(1, 0);
  } else if (oz_isNonKinded(t)) {
    addSuspend(fs_prop_val, tptr);
    return expectExceptional();
  }
  return expectFail();
}

//*****************************************************************************

OZ_expect_t OZ_Expect::expectVar(OZ_Term t)
{
  DEREF(t, tptr, ttag);

  if (oz_isFree(t)) {
    addSpawn(fd_prop_any, tptr);
    return expectProceed(1, 1);
  } else if (oz_isNonKinded(t)) {
    addSuspend(tptr);
    return expectExceptional();
  }
  return expectFail();
}

OZ_expect_t OZ_Expect::expectRecordVar(OZ_Term t)
{
  DEREF(t, tptr, ttag);

  if (oz_isRecord(t)) {
    return expectProceed(1, 1);
  } else if (isGenOFSVar(t, ttag)) {
    addSpawn(fd_prop_any, tptr);
    return expectProceed(1, 1);
  } else if (oz_isFree(t) || oz_isKinded(t)) {
    addSuspend(tptr);
    return expectSuspend(1, 0);
  } else if (oz_isNonKinded(t)) {
    addSuspend(tptr);
    return expectExceptional();
  }
  return expectFail();
}

OZ_expect_t OZ_Expect::expectLiteral(OZ_Term t)
{
  Assert(oz_isRef(t) || !oz_isVariable(t));

  DEREF(t, tptr, ttag);

  if (isLiteralTag(ttag)) {
    return expectProceed(1, 1);
  } else if (oz_isFree(t) || oz_isKinded(t)) {
    addSuspend(tptr);
    return expectSuspend(1, 0);
  } else if (oz_isNonKinded(t)) {
    addSuspend(tptr);
    return expectExceptional();
  }
  return expectFail();
}

OZ_expect_t OZ_Expect::expectLiteral(OZ_Term t, OZ_Term * one_of)
{
  OZ_expect_t r = expectLiteral(t);

  if (r.accepted == 1 && r.size == 1) {
    OZ_Term td = oz_deref(t);
    for (int i = 0; one_of[i] != (OZ_Term) 0; i += 1) {
      if (td == one_of[i])
        return expectProceed(1, 1);
    }
    return expectFail();
  }
  return r;
}

OZ_expect_t OZ_Expect::expectProperRecord(OZ_Term t,
                                          OZ_expect_t(OZ_Expect::* expectf)(OZ_Term))
{
  Assert(oz_isRef(t) || !oz_isVariable(t));

  DEREF(t, tptr, ttag);

  if (isLiteralTag(ttag)) { // subsumes nil
    return expectProceed(1, 1);
  } else if (oz_isSRecord(t) && !tagged2SRecord(t)->isTuple()) {

    SRecord & tuple = *tagged2SRecord(t);
    int width = tuple.getWidth(), acc = 1;

    for (int i = width; i--; ) {
      //mm2: use tagged2Ref
      OZ_expect_t r = (this->*expectf)(makeTaggedRef(&tuple[i]));
      if (r.accepted == -1) {
        return r;
      } else if (r.accepted == r.size) {
        acc += 1;
      }
    }
    return expectProceed(width + 1, acc);

  } else if (oz_isFree(t) || oz_isKinded(t)) {
    addSuspend(tptr);
    return expectSuspend(1, 0);
  } else if (oz_isNonKinded(t)) {
    addSuspend(tptr);
    return expectExceptional();
  }
  return expectFail();
}

OZ_expect_t OZ_Expect::expectProperRecord(OZ_Term t, OZ_Term * ar)
{
  Assert(oz_isRef(t) || !oz_isVariable(t));

  DEREF(t, tptr, ttag);

  if (isLiteralTag(ttag) && *ar == (OZ_Term) 0) { // subsumes nil
    return expectProceed(1, 1);
  } else if (oz_isSRecord(t) && !tagged2SRecord(t)->isTuple()) {
    SRecord & tuple = *tagged2SRecord(t);

    int i;

    for (i = 0; ar[i] != (OZ_Term) 0; i += 1)
      if (OZ_subtree(t, ar[i]) == (OZ_Term) 0)
        return expectFail();

    return expectProceed(i + 1, i + 1);
  } else if (oz_isFree(t) || oz_isKinded(t)) {
    addSuspend(tptr);
    return expectSuspend(1, 0);
  } else if (oz_isNonKinded(t)) {
    addSuspend(tptr);
    return expectExceptional();
  }
  return expectFail();
}

OZ_expect_t OZ_Expect::expectProperTuple(OZ_Term t,
                                         OZ_expect_t(OZ_Expect::* expectf)(OZ_Term))
{
  Assert(oz_isRef(t) || !oz_isVariable(t));

  DEREF(t, tptr, ttag);

  if (isLiteralTag(ttag)) { // subsumes nil
    return expectProceed(1, 1);
  } else if (oz_isSRecord(t) && tagged2SRecord(t)->isTuple()) {

    SRecord & tuple = *tagged2SRecord(t);
    int width = tuple.getWidth(), acc = 1;

    for (int i = width; i--; ) {
      //mm2: use tagged2Ref
      OZ_expect_t r = (this->*expectf)(makeTaggedRef(&tuple[i]));
      if (r.accepted == -1) {
        return r;
      } else if (r.accepted == r.size) {
        acc += 1;
      }
    }
    return expectProceed(width + 1, acc);

  } else if (oz_isFree(t) || oz_isKinded(t)) {
    addSuspend(tptr);
    return expectSuspend(1, 0);
  } else if (oz_isNonKinded(t)) {
    addSuspend(tptr);
    return expectExceptional();
  }
  return expectFail();
}

OZ_expect_t OZ_Expect::expectList(OZ_Term t,
                                    OZ_expect_t(OZ_Expect::* expectf)(OZ_Term))
{
  Assert(oz_isRef(t) || !oz_isVariable(t));

  DEREF(t, tptr, ttag);

  if (oz_isCons(ttag)) {

    int len = 0, acc = 0;

    do {
      len += 1;
      //mm2: use tagged2Ref
      OZ_expect_t r = (this->*expectf)(makeTaggedRef(tagged2LTuple(t)->getRefHead()));

      if (r.accepted == -1) {
        return r;
      } else if (r.accepted == r.size) {
        acc += 1;
      }

      t = oz_tail(t);
      __DEREF(t, tptr, ttag);
    } while (oz_isCons(ttag));

    if (oz_isNil(t)) {
      return expectProceed(len, acc);
    } else if (oz_isFree(t) || oz_isKinded(t)) {
      addSuspend(tptr);
      return expectSuspend(len+1, acc);
    } else if (oz_isNonKinded(t)) {
      addSuspend(tptr);
      return expectExceptional();
    }
  } else if (oz_isFree(t) || oz_isKinded(t)) {
    addSuspend(tptr);
    return expectSuspend(1, 0);
  } else if (oz_isNonKinded(t)) {
    addSuspend(tptr);
    return expectExceptional();
  }
  return expectFail();
}

OZ_expect_t OZ_Expect::expectVector(OZ_Term t,
                                    OZ_expect_t(OZ_Expect::* expectf)(OZ_Term))
{
  Assert(oz_isRef(t) || !oz_isVariable(t));

  DEREF(t, tptr, ttag);

  if (isLiteralTag(ttag)) { // subsumes nil
    return expectProceed(1, 1);
  } else if (oz_isSTuple(t) || oz_isSRecord(t)) {

    SRecord & tuple = *tagged2SRecord(t);
    int width = tuple.getWidth(), acc = 1;

    for (int i = width; i--; ) {
      OZ_expect_t r = (this->*expectf)(makeTaggedRef(&tuple[i]));
      if (r.accepted == -1) {
        return r;
      } else if (r.accepted == r.size) {
        acc += 1;
      }
    }
    return expectProceed(width + 1, acc);

  } else if (oz_isCons(ttag)) {

    int len = 0, acc = 0;

    do {
      len += 1;
      OZ_expect_t r = (this->*expectf)(makeTaggedRef(tagged2LTuple(t)->getRefHead()));

      if (r.accepted == -1) {
        return r;
      } else if (r.accepted == r.size) {
        acc += 1;
      }

      t = oz_tail(t);
      __DEREF(t, tptr, ttag);
    } while (oz_isCons(ttag));

    if (oz_isNil(t)) {
      return expectProceed(len, acc);
    } else if (oz_isFree(t) || oz_isKinded(t)) {
      addSuspend(tptr);
      return expectSuspend(len+1, acc);
    } else if (oz_isNonKinded(t)) {
      addSuspend(tptr);
      return expectExceptional();
    }

  } else if (oz_isFree(t) || oz_isKinded(t)) {
    addSuspend(tptr);
    return expectSuspend(1, 0);
  } else if (oz_isNonKinded(t)) {
    addSuspend(tptr);
    return expectExceptional();
  }
  return expectFail();
}

OZ_expect_t OZ_Expect::expectStream(OZ_Term st)
{
  Assert(oz_isRef(st) || !oz_isVariable(st));

  DEREF(st, stptr, sttag);

  if (oz_isNil(st)) {
    return expectProceed(1, 1);
  } else if (oz_isCons(sttag)) {

    int len = 0;

    do {
      len += 1;
      st = oz_tail(st);
      __DEREF(st, stptr, sttag);
    } while (oz_isCons(sttag));

    if (oz_isNil(st)) {
      return expectProceed(len, len);
    } else if (oz_isFree(st) || oz_isKinded(st)) {
      addSpawn(fd_prop_any, stptr);
      return expectProceed(len, len);
    } else if (oz_isNonKinded(st)) {
      addSuspend(stptr);
      return expectExceptional();
    }
  } else if (oz_isFree(st) || oz_isKinded(st)) {
    addSpawn(fd_prop_any, stptr);
    return expectProceed(1, 1);
  } else if (oz_isNonKinded(st)) {
    addSuspend(stptr);
    return expectExceptional();
  }

  return expectFail();
}

//-----------------------------------------------------------------------------

OZ_Return OZ_Expect::suspend(OZ_Thread th)
{
  Assert(staticSuspendVarsNumber > 0);

  if (th == NULL) {
    for (int i = staticSuspendVarsNumber; i--; )
      am.addSuspendVarList(staticSuspendVars[i].var);
    return SUSPEND;
  } else {
    for (int i = staticSuspendVarsNumber; i--; )
      OZ_addThread (makeTaggedRef(staticSuspendVars[i].var), th);
    return PROCEED;
  }
}

OZ_Return OZ_Expect::fail(void)
{
  return FAILED;
}


//*****************************************************************************
// member function to spawn a propagator
//*****************************************************************************

OZ_Return OZ_Expect::impose(OZ_Propagator * p, int prio,
                            OZ_PropagatorFlags flags)
{
  OZ_Boolean is_monotonic = p->isMonotonic();

  // do initial run with dummy thread

  // Constrain all SVARs and UVARs in staticSuspendVars to FDVARs before
  // OZ_Propagator::run is run.
  int i;
  for (i = staticSuspendVarsNumber; i--; ) {
    OZ_Term v = makeTaggedRef(staticSuspendVars[i].var);
    DEREF(v, vptr, vtag);
    TypeOfGenCVariable type = staticSuspendVars[i].expected_type;

    Assert(type == FDVariable || type == FSetVariable || type == CtVariable);

    if (oz_isFree(v)) {
      if (type == FDVariable) {
        tellBasicConstraint(makeTaggedRef(vptr), (OZ_FiniteDomain *) NULL);
      } else if (type == BoolVariable) {
        tellBasicBoolConstraint(makeTaggedRef(vptr));
      } else if (type == FSetVariable) {
        tellBasicConstraint(makeTaggedRef(vptr), (OZ_FSetConstraint *) NULL);
      } else {
        Assert(type == CtVariable);

        tellBasicConstraint(makeTaggedRef(vptr),
                            (OZ_Ct *) NULL,
                            staticSuspendVars[i].state.ct.def);

      }
    } else {
      Assert(oz_isKinded(v));
    }
  }

  Propagator * prop = oz_newPropagator(prio, p);
  ozstat.propagatorsCreated.incf();

  // only monotonic propagator are run on imposition
  if (is_monotonic) {
    ozstat.propagatorsInvoked.incf();

    Propagator::setRunningPropagator(prop);

    switch (oz_runPropagator(prop)) {
    case FAILED:
      oz_closeDonePropagator(prop);
      staticSpawnVarsNumber = staticSuspendVarsNumber = 0;
      return FAILED;
    case SLEEP:
      oz_suspendPropagator(prop);
      break;
    case SCHEDULED:
      oz_scheduledPropagator(prop);
      break;
    case PROCEED:
      oz_closeDonePropagator(prop);
      staticSpawnVarsNumber = staticSuspendVarsNumber = 0;
      return PROCEED;
    default:
      error("Unexpected return value.");
    }
  }

// only if a propagator survives its first run proper suspension are created
  OZ_Boolean all_local = OZ_TRUE;

  // caused by the first run of the run method variables may have
  // become determined
  for (i = staticSpawnVarsNumber; i--; ) {
    OZ_Term v = makeTaggedRef(staticSpawnVars[i].var);
    DEREF(v, vptr, vtag);

    if (isVariableTag(vtag)) {
      Assert(!isCVar(vtag) || (!testStoreFlag(v) && !testReifiedFlag(v)));

      if (isGenFDVar(v, vtag)) {
        addSuspFDVar(v, prop, staticSpawnVars[i].state.fd);
        all_local &= am.isLocalSVar(v);
      } else if (isGenFSetVar(v, vtag)) {
        addSuspFSetVar(v, prop, staticSpawnVars[i].state.fs);
        all_local &= am.isLocalSVar(v);
      } else if (isGenBoolVar(v, vtag)) {
        addSuspBoolVar(v, prop);
        all_local &= am.isLocalSVar(v);
      } else if (isGenCtVar(v, vtag)) {
        addSuspCtVar(v, prop, staticSpawnVars[i].state.ct.w);
        all_local &= am.isLocalSVar(v);
      } else {
        addSuspAnyVar(vptr, prop);
        all_local &= am.isLocalVariable(v,vptr);
      }
    }
  }

  // Note all SVARs and UVARs in staticSuspendVars are constrained to FDVARs.
  for (i = staticSuspendVarsNumber; i--; ) {
    OZ_Term v = makeTaggedRef(staticSuspendVars[i].var);
    DEREF(v, vptr, vtag);

    if (isVariableTag(vtag)) {
      Assert(isCVar(vtag));

      oz_cv_addSuspINLINE(tagged2CVar(v), vptr, prop);
      all_local &= am.isLocalSVar(v);
    }
  }

  if (all_local)
    prop->markLocalPropagator();

  switch (flags) {
  case OFS_flag:
    prop->markOFSPropagator();
    break;
  case NULL_flag:
    break;
  default:
    warning("Unrecognized flag found when spawning propagator");
  }

  staticSpawnVarsNumber = staticSuspendVarsNumber = 0;

  // only nonmonotonic propagator are set runnable on imposition
  if (! is_monotonic) {
#ifdef DEBUG_NONMONOTONIC
    printf("Setting nonmono prop runnable.\n"); fflush(stdout);
#endif
    oz_scheduledPropagator(prop);
  }
  return PROCEED;
}

// End of File
//-----------------------------------------------------------------------------
