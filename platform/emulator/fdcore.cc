/*
  Hydra Project, DFKI Saarbruecken,
  Stuhlsatzenhausweg 3, D-66123 Saarbruecken, Phone (+49) 681 302-5312
  Author: tmueller
  Last modified: $Date$ from $Author$
  Version: $Revision$
  State: $State$

  ------------------------------------------------------------------------
*/

#include "fdbuilti.hh"


// ---------------------------------------------------------------------
//                  Finite Domains Core Built-ins
// ---------------------------------------------------------------------


OZ_C_proc_begin(BIisFdVar, 1)
{
  return isGenFDVar(deref(OZ_getCArg(0))) || isGenBoolVar(deref(OZ_getCArg(0))) ? PROCEED : FAILED;
}
OZ_C_proc_end


OZ_C_proc_begin(BIgetFDLimits,2)
{
  return (OZ_unify(newSmallInt(0), OZ_getCArg(0)) &&
    OZ_unify(newSmallInt(fd_sup), OZ_getCArg(1))) ? PROCEED : FAILED;
}
OZ_C_proc_end

State BIfdIsInline(TaggedRef fd) {
  DEREF(fd, fdptr, fdtag);

  if (isNotCVar(fdtag)) return SUSPEND;

  return (isPosSmallInt(fd) || isGenFDVar(fd, fdtag) || isGenBoolVar(fd, fdtag)) ? PROCEED : FAILED;
}

DECLAREBI_USEINLINEREL1(BIfdIs, BIfdIsInline)

OZ_C_proc_begin(BIfdMin, 2)
{
  ExpectedTypes("FiniteDomain,SmallInt");

  OZ_getCArgDeref(0, var, varptr, vartag);

  if(isSmallInt(vartag)) {
    return OZ_unify(var, OZ_getCArg(1));
  } else if (isGenFDVar(var,vartag)) {
    int minVal = tagged2GenFDVar(var)->getDom().minElem();
    return OZ_unify(newSmallInt(minVal), OZ_getCArg(1));
  } else if (isGenBoolVar(var,vartag)) {
    return OZ_unify(newSmallInt(0), OZ_getCArg(1));
  } else if (isNotCVar(vartag)) {
    return OZ_suspendOnVar(TaggedRef(varptr));
  } else {
    TypeError(0, "");
  }
}
OZ_C_proc_end


OZ_C_proc_begin(BIfdMax,2)
{
  ExpectedTypes("FiniteDomain,SmallInt");

  OZ_getCArgDeref(0, var, varptr, vartag);

  if(isSmallInt(vartag)) {
    return OZ_unify(var, OZ_getCArg(1));
  } else if (isGenFDVar(var,vartag)) {
    int maxVal = tagged2GenFDVar(var)->getDom().maxElem();
    return OZ_unify(newSmallInt(maxVal), OZ_getCArg(1));
  } else if (isGenBoolVar(var,vartag)) {
    return OZ_unify(newSmallInt(1), OZ_getCArg(1));
  } else if (isNotCVar(vartag)) {
    return OZ_suspendOnVar(TaggedRef(varptr));
  } else {
    TypeError(0, "");
  }
}
OZ_C_proc_end


OZ_C_proc_begin(BIfdGetAsList, 2)
{
  ExpectedTypes("FiniteDomain,List");

  OZ_getCArgDeref(0, var, varptr, vartag);

  if(isSmallInt(vartag)) {
    LTuple * ltuple = new LTuple(var, AtomNil);
    return OZ_unify(makeTaggedLTuple(ltuple), OZ_getCArg(1));
  } else if (isGenFDVar(var,vartag)) {
    FiniteDomain &fdomain = tagged2GenFDVar(var)->getDom();
    return OZ_unify(fdomain.getAsList(), OZ_getCArg(1));
  } else if (isGenBoolVar(var,vartag)) {
    return OZ_unify(makeTaggedLTuple(new LTuple(mkTuple(0, 1), AtomNil)),
                    OZ_getCArg(1));
  } else if (isNotCVar(vartag)) {
    return OZ_suspendOnVar(TaggedRef(varptr));
  } else {
    TypeError(0, "");
  }
}
OZ_C_proc_end


OZ_C_proc_begin(BIfdGetCardinality,2)
{
  ExpectedTypes("FiniteDomain,SmallInt");

  OZ_getCArgDeref(0, var, varptr, vartag);

  if(isSmallInt(vartag)) {
    return OZ_unify(newSmallInt(1), OZ_getCArg(1));
  } else if (isGenFDVar(var,vartag)) {
    FiniteDomain &fdomain = tagged2GenFDVar(var)->getDom();
    return OZ_unify(newSmallInt(fdomain.getSize()), OZ_getCArg(1));
  } else if (isGenBoolVar(var,vartag)) {
    return OZ_unify(newSmallInt(2), OZ_getCArg(1));
  } else if (isNotCVar(vartag)) {
    return OZ_suspendOnVar(TaggedRef(varptr));
  } else {
    TypeError(0, "");
  }
}
OZ_C_proc_end



OZ_C_proc_begin(BIfdNextTo, 3)
{
  ExpectedTypes("FiniteDomain,SmallInt,SmallInt or Tuple");

  OZ_getCArgDeref(1, n, nptr, ntag);

  if (isAnyVar(ntag)) {
    return OZ_suspendOnVar(TaggedRef(nptr));
  } else if (! isSmallInt(ntag)) {
    TypeError(1, "");
  }

  OZ_getCArgDeref(0, var, varptr, vartag);

  if (isPosSmallInt(var)) {
    return OZ_unify(OZ_getCArg(2), var);
  } else if (isGenFDVar(var,vartag)) {
    int next_val, n_val = smallIntValue(n);
    return (tagged2GenFDVar(var)->getDom().next(n_val, next_val))
      ? OZ_unify(OZ_getCArg(2), mkTuple(next_val, 2 * n_val - next_val))
      : OZ_unify(OZ_getCArg(2), newSmallInt(next_val));
  } else if (isGenBoolVar(var,vartag)) {
    int val = smallIntValue(n);
    return OZ_unify(OZ_getCArg(2), newSmallInt(val >= 1 ? 1 : 0));
  } else if (isNotCVar(vartag)) {
    return OZ_suspendOnVar(TaggedRef(varptr));
  } else {
    TypeError(0, "");
  }
}
OZ_C_proc_end


OZ_C_proc_begin(BIfdPutLe, 2)
{
  Assert(!FDcurrentTaskSusp);

  ExpectedTypes("FiniteDomain,SmallInt");

  OZ_getCArgDeref(1, n, nptr, ntag);

  if (isAnyVar(ntag)) {
    return OZ_suspendOnVar(TaggedRef(nptr));
  } else if (! isSmallInt(ntag)) {
    TypeError(1, "");
  }

  OZ_getCArgDeref(0, var, varptr, vartag);

  if (! (isGenFDVar(var,vartag) || isGenBoolVar(var,vartag) ||
         isSmallInt(vartag))) {
    if (isNotCVar(vartag)) {
      return OZ_suspendOnVar(TaggedRef(varptr));
    } else {
      TypeError(0, "");
    }
  }

  BIfdBodyManager x;

  if (! x.introduce(OZ_getCArg(0))) {
    error("Should never happen.");
    return FAILED;
  }

  FailOnEmpty(*x <= smallIntValue(n));

  return x.releaseNonRes();
}
OZ_C_proc_end // BIfdPutLe


OZ_C_proc_begin(BIfdPutGe, 2)
{
  Assert(!FDcurrentTaskSusp);

  ExpectedTypes("FiniteDomain,SmallInt");

  OZ_getCArgDeref(1, n, nptr, ntag);

  if (isAnyVar(ntag)) {
    return OZ_suspendOnVar(TaggedRef(nptr));
  } else if (! isSmallInt(ntag)) {
    TypeError(1, "");
  }

  OZ_getCArgDeref(0, var, varptr, vartag);

  if (! (isGenFDVar(var,vartag) || isGenBoolVar(var,vartag) ||
         isSmallInt(vartag))) {
    if (isNotCVar(vartag)) {
      return OZ_suspendOnVar(TaggedRef(varptr));
    } else {
      TypeError(0, "");
    }
  }

  BIfdBodyManager x;

  if (! x.introduce(OZ_getCArg(0))) {
    error("Should never happen.");
    return FAILED;
  }

  FailOnEmpty(*x >= smallIntValue(n));

  return x.releaseNonRes();
}
OZ_C_proc_end // BIfdPutGe


OZ_C_proc_begin(BIfdPutList, 3)
{
  Assert(!FDcurrentTaskSusp);

  ExpectedTypes("FiniteDomain,List of SmallInts or Tuples,SmallInt");

  OZ_getCArgDeref(2, s, sptr, stag); // sign

  if (isAnyVar(stag)) {
    return OZ_suspendOnVar(TaggedRef(sptr));
  } else if (! isSmallInt(stag)) {
    TypeError(2, "");
  }

  switch (checkDomDescr(OZ_getCArg(1), OZ_self, OZ_args, OZ_arity)) {
  case SUSPEND: return SUSPEND;
  case FAILED:  return FAILED;
  case PROCEED: break;
  default:      error("Unexpected value"); break;
  }

  BIfdBodyManager x;

  if (! x.introduce(OZ_getCArg(0))) return FAILED;

  LocalFD aux; aux.init(OZ_getCArg(1));

  if (smallIntValue(s) != 0) aux = ~aux;

  FailOnEmpty(*x &= aux);

  return x.releaseNonRes();
}
OZ_C_proc_end // BIfdPutList


OZ_C_proc_begin(BIfdPutInterval, 3)
{
  Assert(!FDcurrentTaskSusp);

  ExpectedTypes("FiniteDomain,SmallInt,SmallInt");

  OZ_getCArgDeref(1, l, lptr, ltag); // lower bound

  if (isAnyVar(ltag)) {
    return OZ_suspendOnVar(TaggedRef(lptr));
  } else if (! isSmallInt(ltag)) {
    TypeError(1, "");
  }

  OZ_getCArgDeref(2, u, uptr, utag); // upper bound

  if (isAnyVar(utag)) {
    return OZ_suspendOnVar(TaggedRef(uptr));
  } else if (! isSmallInt(utag)) {
    TypeError(2, "");
  }

  OZ_getCArgDeref(0, var, varptr, vartag);

  if (! (isGenFDVar(var, vartag) || isGenBoolVar(var, vartag) ||
         isNotCVar(vartag) || isSmallInt(vartag))) {
    TypeError(0, "");
  }

  BIfdBodyManager x;

  if (! x.introduce(OZ_getCArg(0))) return FAILED;

  LocalFD aux;

  FailOnEmpty(aux.init(smallIntValue(l), smallIntValue(u)));
  FailOnEmpty(*x &= aux);

  return x.releaseNonRes();
}
OZ_C_proc_end // BIfdPutInterval


OZ_C_proc_begin(BIfdPutNot, 2)
{
  Assert(!FDcurrentTaskSusp);

  ExpectedTypes("FiniteDomain,SmallInt");

  OZ_getCArgDeref(1, n, nptr, ntag);

  if (isAnyVar(ntag)) {
    return OZ_suspendOnVar(TaggedRef(nptr));
  } else if (! isSmallInt(ntag)) {
    TypeError(1, "");
  }

  OZ_getCArgDeref(0, var, varptr, vartag);

  if (! (isGenFDVar(var, vartag) || isGenBoolVar(var, vartag) ||
         isSmallInt(vartag))) {
    if (isNotCVar(vartag)) {
      return OZ_suspendOnVar(TaggedRef(varptr));
    } else {
      TypeError(0, "");
    }
  }

  BIfdBodyManager x;

  if (! x.introduce(OZ_getCArg(0))) {
    error("Should never happen.");
    return FAILED;
  }

  FailOnEmpty(*x -= smallIntValue(n));

  return x.releaseNonRes();
}
OZ_C_proc_end // BIfdPutNot
