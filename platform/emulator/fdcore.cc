/*
 *  Authors:
 *    Tobias M�ller (tmueller@ps.uni-sb.de)
 *
 *  Contributors:
 *    Christian Schulte <schulte@ps.uni-sb.de>
 *
 *  Copyright:
 *    Tobias M�ller, 1999
 *    Christian Schulte, 1999
 *
 *  Last change:
 *    $Date$ by $Author$
 *    $Revision$
 *
 *  This file is part of Mozart, an implementation
 *  of Oz 3:
 *     http://www.mozart-oz.org
 *
 *  See the file "LICENSE" or
 *     http://www.mozart-oz.org/LICENSE.html
 *  for information on usage and redistribution
 *  of this file, and for a DISCLAIMER OF ALL
 *  WARRANTIES.
 *
 */

#include "cpi.hh"
#include "var_fd.hh"
#include "var_bool.hh"


OZ_BI_define(BIisFdVar, 1,0)
{
  return isGenFDVar(oz_deref(OZ_in(0)))
    || isGenBoolVar(oz_deref(OZ_in(0))) ? PROCEED : FAILED;
} OZ_BI_end

OZ_BI_define(BIisFdVarB, 1,1)
{
  OZ_RETURN(oz_bool(isGenFDVar(oz_deref(OZ_in(0))) ||
                    isGenBoolVar(oz_deref(OZ_in(0)))));
} OZ_BI_end

OZ_BI_define(BIgetFDLimits, 0,2)
{
  OZ_out(0) = oz_int(0);
  OZ_out(1) = oz_int(fd_sup);
  return PROCEED;
} OZ_BI_end

OZ_BI_define(BIfdIs, 1, 1)
{
  OZ_getINDeref(0, fd, fdptr, fdtag);

  if (oz_isNonKinded(fd))
    oz_suspendOnPtr(fdptr);

  OZ_RETURN(oz_bool(isPosSmallFDInt(fd) ||
                    isGenFDVar(fd, fdtag) ||
                    isGenBoolVar(fd, fdtag)));
} OZ_BI_end


//-----------------------------------------------------------------------------
// reflective stuff

OZ_BI_define(BIfdMin, 1, 1)
{
  ExpectedTypes(OZ_EM_FD "," OZ_EM_INT);

  OZ_getINDeref(0, var, varptr, vartag);

  if(isSmallIntTag(vartag)) {
    OZ_RETURN(var);
  } else if (isGenFDVar(var,vartag)) {
    OZ_RETURN(newSmallInt(tagged2GenFDVar(var)->getDom().getMinElem()));
  } else if (isGenBoolVar(var,vartag)) {
    OZ_RETURN(newSmallInt(0));
  } else if (oz_isNonKinded(var)) {
    oz_suspendOnPtr(varptr);
  } else {
    TypeError(0, "");
  }
} OZ_BI_end

OZ_BI_define(BIfdMax, 1, 1)
{
  ExpectedTypes(OZ_EM_FD "," OZ_EM_INT);

  OZ_getINDeref(0, var, varptr, vartag);

  if(isSmallIntTag(vartag)) {
    OZ_RETURN(var);
  } else if (isGenFDVar(var,vartag)) {
    OZ_RETURN(newSmallInt(tagged2GenFDVar(var)->getDom().getMaxElem()));
  } else if (isGenBoolVar(var,vartag)) {
    OZ_RETURN(newSmallInt(1));
  } else if (oz_isNonKinded(var)) {
    oz_suspendOnPtr(varptr);
  } else {
    TypeError(0, "");
  }
}
OZ_BI_end

OZ_BI_define(BIfdMid, 1, 1)
{
  ExpectedTypes(OZ_EM_FD "," OZ_EM_INT);

  OZ_getINDeref(0, var, varptr, vartag);

  if(isSmallIntTag(vartag)) {
    OZ_RETURN(var);
  } else if (isGenFDVar(var,vartag)) {
    OZ_RETURN(newSmallInt(tagged2GenFDVar(var)->getDom().getMidElem()));
  } else if (isGenBoolVar(var,vartag)) {
    OZ_RETURN(newSmallInt(0));
  } else if (oz_isNonKinded(var)) {
    oz_suspendOnPtr(varptr);
  } else {
    TypeError(0, "");
  }
}
OZ_BI_end

OZ_BI_define(BIfdNextSmaller, 2, 1)
{
  ExpectedTypes(OZ_EM_FD "," OZ_EM_INT "," OZ_EM_INT);

  OZ_getINDeref(1, val, valptr, valtag);

  int value = -1;
  if (isVariableTag(valtag)) {
    oz_suspendOnPtr(valptr);
  } else if (isSmallIntTag(valtag)) {
    value = smallIntValue(val);
  } else {
    TypeError(1, "");
  }

  OZ_getINDeref(0, var, varptr, vartag);

  if(isSmallIntTag(vartag)) {
    if (value > smallIntValue(var))
      OZ_RETURN(var);
  } else if (isGenFDVar(var,vartag)) {
    int nextSmaller = tagged2GenFDVar(var)->getDom().getNextSmallerElem(value);
    if (nextSmaller != -1)
      OZ_RETURN(newSmallInt(nextSmaller));
  } else if (isGenBoolVar(var,vartag)) {
    if (value > 1)
      OZ_RETURN(newSmallInt(1));
    else if (value > 0)
      OZ_RETURN(newSmallInt(0));
  } else if (oz_isNonKinded(var)) {
    oz_suspendOnPtr(varptr);
  } else {
    TypeError(0, "");
  }
  return FAILED;
}
OZ_BI_end

OZ_BI_define(BIfdNextLarger, 2, 1)
{
  ExpectedTypes(OZ_EM_FD "," OZ_EM_INT "," OZ_EM_INT);

  OZ_getINDeref(1, val, valptr, valtag);

  int value = -1;
  if (oz_isVariable(valtag)) {
    oz_suspendOnPtr(valptr);
  } else if (isSmallIntTag(valtag)) {
    value = smallIntValue(val);
  } else {
    TypeError(1, "");
  }

  OZ_getINDeref(0, var, varptr, vartag);

  if(isSmallIntTag(vartag)) {
    if (value < smallIntValue(var))
      OZ_RETURN(var);
  } else if (isGenFDVar(var,vartag)) {
    int nextLarger = tagged2GenFDVar(var)->getDom().getNextLargerElem(value);
    if (nextLarger != -1)
      OZ_RETURN(newSmallInt(nextLarger));
  } else if (isGenBoolVar(var,vartag)) {
    if (value < 0)
      OZ_RETURN(newSmallInt(0));
    else if (value < 1)
      OZ_RETURN(newSmallInt(1));
  } else if (oz_isNonKinded(var)) {
    oz_suspendOnPtr(varptr);
  } else {
    TypeError(0, "");
  }
  return FAILED;
}
OZ_BI_end

OZ_BI_define(BIfdGetAsList, 1, 1)
{
  ExpectedTypes(OZ_EM_FD "," OZ_EM_FDDESCR);

  OZ_getINDeref(0, var, varptr, vartag);

  if(isSmallIntTag(vartag)) {
    OZ_RETURN(makeTaggedLTuple(new LTuple(var, AtomNil)));
  } else if (isGenFDVar(var,vartag)) {
    OZ_FiniteDomain &fdomain = tagged2GenFDVar(var)->getDom();
    OZ_RETURN(fdomain.getDescr());
  } else if (isGenBoolVar(var,vartag)) {
    OZ_RETURN(makeTaggedLTuple(new LTuple(oz_pair2(newSmallInt(0),
                                                   newSmallInt(1)),
                                          AtomNil)));
  } else if (oz_isNonKinded(var)) {
    oz_suspendOnPtr(varptr);
  } else {
    TypeError(0, "");
  }
}
OZ_BI_end

OZ_BI_define(BIfdGetCardinality, 1, 1)
{
  ExpectedTypes(OZ_EM_FD "," OZ_EM_INT);

  OZ_getINDeref(0, var, varptr, vartag);

  if(isSmallIntTag(vartag)) {
    OZ_RETURN(newSmallInt(1));
  } else if (isGenFDVar(var,vartag)) {
    OZ_FiniteDomain &fdomain = tagged2GenFDVar(var)->getDom();
    OZ_RETURN(newSmallInt(fdomain.getSize()));
  } else if (isGenBoolVar(var,vartag)) {
    OZ_RETURN(newSmallInt(2));
  } else if (oz_isNonKinded(var)) {
    oz_suspendOnPtr(varptr);
  } else {
    TypeError(0, "");
  }
}
OZ_BI_end

//-----------------------------------------------------------------------------
// tell finite domain constraint

OZ_BI_define(BIfdTellConstraint, 2, 0)
{
  ExpectedTypes(OZ_EM_FDDESCR "," OZ_EM_FD);

  ExpectOnly pe;
  EXPECT_BLOCK(pe, 0, expectDomDescr,
               "The syntax of a " OZ_EM_FDDESCR " is:\n"
               "   dom_descr   ::= simpl_descr | compl(simpl_descr)\n"
               "   simpl_descr ::= range_descr | [range_descr+]\n"
               "   range_descr ::= integer | integer#integer\n"
               "   integer     ::= {" _OZ_EM_FDINF ",...," _OZ_EM_FDSUP "}");

  OZ_FiniteDomain aux(OZ_in(0));

  return tellBasicConstraint(OZ_in(1), &aux);
}
OZ_BI_end

OZ_BI_define(BIfdBoolTellConstraint, 1, 0)
{
  return tellBasicBoolConstraint(OZ_in(0));
}
OZ_BI_end


OZ_BI_define(BIfdDeclTellConstraint, 1, 0)
{
  return tellBasicConstraint(OZ_in(0), NULL);
}
OZ_BI_end

//-----------------------------------------------------------------------------
// watches

OZ_BI_define(BIfdWatchSize, 3, 0)
{
  ExpectedTypes(OZ_EM_FD "," OZ_EM_INT "," OZ_EM_TNAME);

  OZ_getINDeref(2, t, tptr, ttag);
  if (!isVariableTag(ttag)) {
    if (oz_isBool(t))
      return PROCEED;
    return FAILED;
  }

  OZ_getINDeref(0, v, vptr, vtag);
  int vsize = 0;

// get the current size of the domain
  if(isSmallIntTag(vtag)) {
    vsize = 1;
  } else if (isGenFDVar(v,vtag)) {
    vsize = tagged2GenFDVar(v)->getDom().getSize();
  } else if (isGenBoolVar(v, vtag)) {
    vsize = 2;
  } else if (oz_isNonKinded(v)) {
    oz_suspendOnPtr(vptr);
  } else {
    TypeError(0, "");
  }

// get the value to compare with
  OZ_getINDeref(1, vs, vsptr, vstag);
  int size = 0;

  if (isVariableTag(vstag)) {
    oz_suspendOnPtr(vsptr);
  } else if (isSmallIntTag(vstag)) {
    size = smallIntValue(vs);
  } else {
    TypeError(1, "");
  }

// compute return value
  if (vsize < size) return OZ_unify (OZ_in(2), oz_true());
  if (size < 1) return (OZ_unify (OZ_in(2), oz_false()));

  if (isVariableTag(vtag)){
    //  must return SUSPEND;
    if (isVariableTag(ttag))
      oz_suspendOn2(makeTaggedRef(vptr), makeTaggedRef(tptr));
    oz_suspendOnPtr(vptr);
  }

  return (OZ_unify (OZ_in(2), oz_false()));
} OZ_BI_end


OZ_BI_define(BIfdWatchMin, 3, 0)
{
  ExpectedTypes(OZ_EM_FD "," OZ_EM_INT "," OZ_EM_TNAME);

  OZ_getINDeref(2, t, tptr, ttag);
  if (!isVariableTag(ttag)) {
    if (oz_isBool(t))
      return PROCEED;
    return FAILED;
  }

  OZ_getINDeref(0, v, vptr, vtag);
  int vmin = -1, vmax = -1;

// get the current lower bound of the domain
  if(isSmallIntTag(vtag)) {
    vmin = vmax = smallIntValue(v);
  } else if (isGenFDVar(v,vtag)) {
    vmin = tagged2GenFDVar(v)->getDom().getMinElem();
    vmax = tagged2GenFDVar(v)->getDom().getMaxElem();
  } else if (isGenBoolVar(v, vtag)) {
    vmin = 0;
    vmax = 1;
  } else if (oz_isNonKinded(v)) {
    oz_suspendOnPtr(vptr);
  } else {
    TypeError(0, "");
  }

// get the value to compare with
  OZ_getINDeref(1, vm, vmptr, vmtag);
  int min = -1;

  if (isVariableTag(vmtag)) {
    oz_suspendOnPtr(vmptr);
  } else if (isSmallIntTag(vmtag)) {
    min = smallIntValue(vm);
  } else {
    TypeError(1, "");
  }

  if (min < 0) return (OZ_unify (OZ_in(2), oz_false()));
  if (vmin > min) return OZ_unify (OZ_in(2), oz_true());

  if (isVariableTag(vtag) && min < vmax){
    //  must return SUSPEND;
    if (isVariableTag(ttag))
      oz_suspendOn2(makeTaggedRef(vptr), makeTaggedRef(tptr));
    oz_suspendOnPtr(vptr);
  }

  return (OZ_unify (OZ_in(2), oz_false()));
} OZ_BI_end

OZ_BI_define(BIfdWatchMax, 3, 0)
{
  ExpectedTypes(OZ_EM_FD "," OZ_EM_INT "," OZ_EM_TNAME);

  OZ_getINDeref(2, t, tptr, ttag);
  if (!isVariableTag(ttag)) {
    if (oz_isBool(t))
      return PROCEED;
    return FAILED;
  }

  OZ_getINDeref(0, v, vptr, vtag);
  int vmin = -1, vmax = -1;

// get the current lower bound of the domain
  if(isSmallIntTag(vtag)) {
    vmin = vmax = smallIntValue(v);
  } else if (isGenFDVar(v,vtag)) {
    vmin = tagged2GenFDVar(v)->getDom().getMinElem();
    vmax = tagged2GenFDVar(v)->getDom().getMaxElem();
  } else if (isGenBoolVar(v, vtag)) {
    vmin = 0;
    vmax = 1;
  } else if (oz_isNonKinded(v)) {
    oz_suspendOnPtr(vptr);
  } else {
    TypeError(0, "");
  }

// get the value to compare with
  OZ_getINDeref(1, vm, vmptr, vmtag);
  int max = -1;

  if (isVariableTag(vmtag)) {
    oz_suspendOnPtr(vmptr);
  } else if (isSmallIntTag(vmtag)) {
    max = smallIntValue(vm);
  } else {
    TypeError(1, "");
  }

  if (vmax < max) return OZ_unify (OZ_in(2), oz_true());
  if (max < 0) return (OZ_unify (OZ_in(2), oz_false()));

  if (isVariableTag(vtag) && vmin < max){
    //  must return SUSPEND;
    if (isVariableTag(ttag))
      oz_suspendOn2(makeTaggedRef(vptr), makeTaggedRef(tptr));
    oz_suspendOnPtr(vptr);
  }

  return (OZ_unify (OZ_in(2), oz_false()));
} OZ_BI_end
