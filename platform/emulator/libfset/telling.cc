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

#include "telling.hh"

OZ_C_proc_begin(fsp_include, 2)
{
  OZ_EXPECTED_TYPE(OZ_EM_FD "," OZ_EM_FSET);

  PropagatorExpect pe;

  OZ_EXPECT(pe, 0, expectIntVarAny);
  OZ_EXPECT(pe, 1, expectFSetVarBounds);

  return pe.impose(new IncludePropagator(OZ_args[1],
                                         OZ_args[0]));
}
OZ_C_proc_end


OZ_C_proc_begin(fsp_exclude, 2)
{
  OZ_EXPECTED_TYPE(OZ_EM_FD "," OZ_EM_FSET );

  PropagatorExpect pe;

  OZ_EXPECT(pe, 0, expectIntVarAny);
  OZ_EXPECT(pe, 1, expectFSetVarBounds);

  return pe.impose(new ExcludePropagator(OZ_args[1],
                                         OZ_args[0]));
}
OZ_C_proc_end


OZ_C_proc_begin(fsp_card, 2)
{
  OZ_EXPECTED_TYPE(OZ_EM_FSET "," OZ_EM_FD);

  PropagatorExpect pe;

  int susp_count = 0;

  OZ_EXPECT_SUSPEND(pe, 0, expectFSetVarAny, susp_count);
  OZ_EXPECT_SUSPEND(pe, 1, expectIntVarMinMax, susp_count);

  if (susp_count > 1) return pe.suspend(OZ_makeSelfSuspendedThread());

  return pe.impose(new FSetCardPropagator(OZ_args[0],
                                          OZ_args[1]));
}
OZ_C_proc_end


//*****************************************************************************

OZ_Return IncludePropagator::propagate(void)
{
  OZ_DEBUGPRINTTHIS("in: ");

  OZ_FSetVar s(_s);
  OZ_FDIntVar d(_d);
  PropagatorController_S_D P(s, d);

  FailOnEmpty(*d <= (fsethigh32 - 1));

  if (*d == fd_singl) {
    FailOnInvalid(*s += d->getSingleElem());
  } else {

    // all elements which are known _not_ to be in `s' are not in `d'
#ifdef FSET_HIGH
    for (int i = fsethigh32; i --; )
      if (s->isNotIn(i))
        FailOnEmpty(*d -= i);
#else
    OZ_FiniteDomain not_in(s->getNotInSet());
    FailOnEmpty(*d -= not_in);
#endif

    if (*d == fd_singl)
      FailOnInvalid(*s += d->getSingleElem());
  }

  {
    OZ_FSetValue d_set(*d);
    OZ_Boolean ent = (d_set <= s->getGlbSet());

    OZ_DEBUGPRINTTHIS("out: ");

    return ent ? P.vanish() : P.leave1();
  }
failure:
  OZ_DEBUGPRINTTHIS("fail: ");
  return P.fail();
}

OZ_Return ExcludePropagator::propagate(void)
{
  OZ_DEBUGPRINTTHIS("in: ");

  OZ_FSetVar s(_s);
  OZ_FDIntVar d(_d);
  PropagatorController_S_D P(s, d);

  if (*d == fd_singl) {
    FailOnInvalid(*s -= d->getSingleElem());
  } else {

    // all elements which are known to be in `s' are not in `d'
#ifdef FSET_HIGH
    for (int i = fsethigh32; i --; )
      if (s->isIn(i))
        FailOnEmpty(*d -= i);
#else
    OZ_FiniteDomain lb(s->getGlbSet());
    FailOnEmpty(*d -= lb);
#endif

    if (*d == fd_singl)
      FailOnInvalid(*s -= d->getSingleElem());
  }

  {
    OZ_Boolean ent;

    if (d->getMaxElem() >= fsethigh32) // TMUELLER: remove when new impl
      ent = OZ_FALSE;
    else {
      OZ_FSetValue d_set(*d);
      ent = (d_set <= s->getNotInSet());
    }
    //    printf("ent=%d\n", ent); fflush(stdout);

    OZ_DEBUGPRINTTHIS("out: ");

    return ent ? P.vanish() : P.leave1();
  }
failure:
  OZ_DEBUGPRINTTHIS("fail: ");
  return P.fail();
}

OZ_Return FSetCardPropagator::propagate(void)
{
  OZ_DEBUGPRINTTHIS("in: ");

  OZ_FSetVar s(_s);
  OZ_FDIntVar d(_d);
  PropagatorController_S_D P(s, d);


  FailOnEmpty(*d >= s->getCardMin());
  FailOnEmpty(*d <= s->getCardMax());

  FailOnInvalid(s->putCard(d->getMinElem(), d->getMaxElem()));

  if (*d == fd_singl) {
    OZ_DEBUGPRINT(("entailed: %s %s",d->toString(),this->toString()));
    return P.vanish();
  }

  OZ_DEBUGPRINTTHIS("out: ");

  return P.leave();

failure:
  OZ_DEBUGPRINTTHIS("fail: ");
  return P.fail();
}

OZ_PropagatorProfile IncludePropagator::profile = "fsp_include";
OZ_PropagatorProfile ExcludePropagator::profile = "fsp_exclude";
OZ_PropagatorProfile FSetCardPropagator::profile = "fsp_card";
