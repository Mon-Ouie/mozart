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

#include "var_ct.hh"
#include "am.hh"
#include "builtins.hh"

OZ_BI_define(BIIsGenCtVarB, 1,1)
{
  OZ_getINDeref(0, v, vptr, vtag);

  OZ_RETURN(oz_bool(isGenCtVar(v, vtag)));
} OZ_BI_end

OZ_BI_define(BIGetCtVarConstraintAsAtom, 1, 1)
{ 
  ExpectedTypes("OzCtVariable<ConstraintData>,Atom");
  
  OZ_getINDeref(0, var, varptr, vartag);

  if (!oz_isVariable(vartag)) {
    OZ_RETURN(var);
  } else if (isGenCtVar(var, vartag)) {
    OZ_RETURN(oz_atom(((OzCtVariable *) tagged2CVar(var))->getConstraint()->toString(ozconf.printDepth)));
  } else if (oz_isNonKinded(var)) {
    oz_suspendOnPtr(varptr);
  } else {
    TypeError(0, "");
  }
}
OZ_BI_end

OZ_BI_define(BIGetCtVarNameAsAtom, 1, 1)
{ 
  ExpectedTypes("OzCtVariable<ConstraintData>,Atom");
  
  OZ_getINDeref(0, var, varptr, vartag);

  if (!oz_isVariable(vartag)) {
    OZ_RETURN(var);
  } else if (isGenCtVar(var, vartag)) {
    OZ_RETURN(oz_atom(((OzCtVariable*)tagged2CVar(var))->getDefinition()->getName()));
  } else if (oz_isNonKinded(var)) {
    oz_suspendOnPtr(varptr);
  } else {
    TypeError(0, "");
  }
}
OZ_BI_end

/*
 * The builtin table
 */

#ifndef MODULES_LINK_STATIC

#include "modCTB-if.cc"

#endif
