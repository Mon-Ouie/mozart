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

#include "cd.hh"
#include "sum.hh"

#ifdef FDCD
//-----------------------------------------------------------------------------

OZ_C_proc_begin(fdp_sumCD, 4)
{
  OZ_EXPECTED_TYPE(OZ_EM_VECT OZ_EM_FD","OZ_EM_LIT","OZ_EM_FD","OZ_EM_FD);

  PropagatorExpect pe;

  OZ_EXPECT(pe, 0, expectVectorIntVarMinMax);
  OZ_EXPECT(pe, 1, expectLiteral);
  OZ_EXPECT(pe, 2, expectIntVarMinMax);
  OZ_EXPECT(pe, 3, expectIntVarMinMax);

  const char * op = OZ_atomToC(OZ_args[1]);
  if (!strcmp(SUM_OP_EQ, op)) {
    return pe.impose(new CDSuppl(new SumEqPropagator(OZ_args[0], OZ_args[2]), OZ_args[3]));
  } else if (!strcmp(SUM_OP_NEQ, op)) {
    return pe.impose(new CDSuppl(new SumNeqPropagator(OZ_args[0], OZ_args[2]), OZ_args[3]));
  } else if (!strcmp(SUM_OP_LEQ, op)) {
    return pe.impose(new CDSuppl(new SumLeqPropagator(OZ_args[0], OZ_args[2]), OZ_args[3]));
  } else if (!strcmp(SUM_OP_LT, op)) {
    return pe.impose(new CDSuppl(new SumLtPropagator(OZ_args[0], OZ_args[2]), OZ_args[3]));
  } else if (!strcmp(SUM_OP_GEQ, op)) {
    return pe.impose(new CDSuppl(new SumGeqPropagator(OZ_args[0], OZ_args[2]), OZ_args[3]));
  } else if (!strcmp(SUM_OP_GT, op)) {
    return pe.impose(new CDSuppl(new SumGtPropagator(OZ_args[0], OZ_args[2]), OZ_args[3]));
  }

  ERROR_UNEXPECTED_OPERATOR(1);
}
OZ_C_proc_end

//-----------------------------------------------------------------------------

OZ_C_proc_begin(fdp_sumCCD, 5)
{
  OZ_EXPECTED_TYPE(OZ_EM_VECT OZ_EM_INT","OZ_EM_VECT OZ_EM_FD","OZ_EM_LIT","OZ_EM_FD","OZ_EM_FD);

  PropagatorExpect pe;

  OZ_EXPECT(pe, 0, expectVectorInt);
  OZ_EXPECT(pe, 1, expectVectorIntVarMinMax);
  OZ_EXPECT(pe, 2, expectLiteral);
  OZ_EXPECT(pe, 3, expectIntVarMinMax);
  OZ_EXPECT(pe, 4, expectIntVarMinMax);

  const char * op = OZ_atomToC(OZ_args[2]);
  if (!strcmp(SUM_OP_EQ, op)) {
    return pe.impose(new CDSuppl(new SumCEqPropagator(OZ_args[0], OZ_args[1], OZ_args[3]), OZ_args[4]));
  } else if (!strcmp(SUM_OP_NEQ, op)) {
    return pe.impose(new CDSuppl(new SumCNeqPropagator(OZ_args[0], OZ_args[1], OZ_args[3]), OZ_args[4]));
  } else if (!strcmp(SUM_OP_LEQ, op)) {
    return pe.impose(new CDSuppl(new SumCLeqPropagator(OZ_args[0], OZ_args[1], OZ_args[3]), OZ_args[4]));
  } else if (!strcmp(SUM_OP_LT, op)) {
    return pe.impose(new CDSuppl(new SumCLtPropagator(OZ_args[0], OZ_args[1], OZ_args[3]), OZ_args[4]));
  } else if (!strcmp(SUM_OP_GEQ, op)) {
    return pe.impose(new CDSuppl(new SumCGeqPropagator(OZ_args[0], OZ_args[1], OZ_args[3]), OZ_args[4]));
  } else if (!strcmp(SUM_OP_GT, op)) {
    return pe.impose(new CDSuppl(new SumCGtPropagator(OZ_args[0], OZ_args[1], OZ_args[3]), OZ_args[4]));
  }

  ERROR_UNEXPECTED_OPERATOR(2);
}
OZ_C_proc_end

//-----------------------------------------------------------------------------

OZ_C_proc_begin(fdp_sumCNCD, 5)
{
  OZ_EXPECTED_TYPE(OZ_EM_VECT OZ_EM_INT","OZ_EM_VECT OZ_EM_VECT OZ_EM_FD","OZ_EM_LIT","OZ_EM_FD","OZ_EM_FD);

  PropagatorExpect pe;

  OZ_EXPECT(pe, 0, expectVectorInt);
  OZ_EXPECT(pe, 1, expectVectorVectorIntVarMinMax);
  OZ_EXPECT(pe, 2, expectLiteral);
  OZ_EXPECT(pe, 3, expectIntVarMinMax);
  OZ_EXPECT(pe, 4, expectIntVarMinMax);

  const char * op = OZ_atomToC(OZ_args[2]);
  if (!strcmp(SUM_OP_EQ, op)) {
    return pe.impose(new CDSuppl(new SumCN_EqPropagator(OZ_args[0], OZ_args[1], OZ_args[3]), OZ_args[4]));
  } else if (!strcmp(SUM_OP_NEQ, op)) {
    return pe.impose(new CDSuppl(new SumCNNeqPropagator(OZ_args[0], OZ_args[1], OZ_args[3]), OZ_args[4]));
  } else if (!strcmp(SUM_OP_LEQ, op)) {
    return pe.impose(new CDSuppl(new SumCNLeqPropagator(OZ_args[0], OZ_args[1], OZ_args[3]), OZ_args[4]));
  } else if (!strcmp(SUM_OP_LT, op)) {
    return pe.impose(new CDSuppl(new SumCNLtPropagator(OZ_args[0], OZ_args[1], OZ_args[3]), OZ_args[4]));
  } else if (!strcmp(SUM_OP_GEQ, op)) {
    return pe.impose(new CDSuppl(new SumCNGeqPropagator(OZ_args[0], OZ_args[1], OZ_args[3]), OZ_args[4]));
  } else if (!strcmp(SUM_OP_GT, op)) {
    return pe.impose(new CDSuppl(new SumCNGtPropagator(OZ_args[0], OZ_args[1], OZ_args[3]), OZ_args[4]));
  }

  ERROR_UNEXPECTED_OPERATOR(2);
}
OZ_C_proc_end

//-----------------------------------------------------------------------------

#endif
