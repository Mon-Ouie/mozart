#include "standard.hh"

OZ_C_proc_begin(fsp_intersection, 3)
{
  OZ_EXPECTED_TYPE(OZ_EM_FSET "," OZ_EM_FSET "," OZ_EM_FSET);

  PropagatorExpect pe;

  int susp_count = 0;

  OZ_EXPECT_SUSPEND(pe, 0, expectFSetVarBounds, susp_count);
  OZ_EXPECT_SUSPEND(pe, 1, expectFSetVarBounds, susp_count);
  OZ_EXPECT_SUSPEND(pe, 2, expectFSetVarBounds, susp_count);
  
  if (susp_count > 1) return pe.suspend(OZ_makeSelfSuspendedThread());

  return pe.spawn(new FSetIntersectionPropagator(OZ_args[0],
						 OZ_args[1],
						 OZ_args[2]));
} 
OZ_C_proc_end

OZ_CFun FSetIntersectionPropagator::spawner = fsp_intersection;

OZ_C_proc_begin(fsp_union, 3)
{
  OZ_EXPECTED_TYPE(OZ_EM_FSET "," OZ_EM_FSET "," OZ_EM_FSET);

  PropagatorExpect pe;

  int susp_count = 0;

  OZ_EXPECT_SUSPEND(pe, 0, expectFSetVarBounds, susp_count);
  OZ_EXPECT_SUSPEND(pe, 1, expectFSetVarBounds, susp_count);
  OZ_EXPECT_SUSPEND(pe, 2, expectFSetVarBounds, susp_count);
  
  if (susp_count > 1) return pe.suspend(OZ_makeSelfSuspendedThread());

  return pe.spawn(new FSetUnionPropagator(OZ_args[0],
					  OZ_args[1],
					  OZ_args[2]));
} 
OZ_C_proc_end

OZ_CFun FSetUnionPropagator::spawner = fsp_union;

OZ_C_proc_begin(fsp_subsume, 2)
{
  OZ_EXPECTED_TYPE(OZ_EM_FSET "," OZ_EM_FSET);

  PropagatorExpect pe;

  int susp_count = 0;

  OZ_EXPECT_SUSPEND(pe, 0, expectFSetVarBounds, susp_count);
  OZ_EXPECT_SUSPEND(pe, 1, expectFSetVarBounds, susp_count);
  
  if (susp_count > 1) return pe.suspend(OZ_makeSelfSuspendedThread());

  return pe.spawn(new FSetSubsumePropagator(OZ_args[0],
					    OZ_args[1]));
} 
OZ_C_proc_end

OZ_CFun FSetSubsumePropagator::spawner = fsp_subsume;


OZ_C_proc_begin(fsp_disjoint, 2)
{
  OZ_EXPECTED_TYPE(OZ_EM_FSET "," OZ_EM_FSET);

  PropagatorExpect pe;

  int susp_count = 0;

  OZ_EXPECT_SUSPEND(pe, 0, expectFSetVarBounds, susp_count);
  OZ_EXPECT_SUSPEND(pe, 1, expectFSetVarBounds, susp_count);
  
  if (susp_count > 1) return pe.suspend(OZ_makeSelfSuspendedThread());

  return pe.spawn(new FSetDisjointPropagator(OZ_args[0],
					     OZ_args[1]));
} 
OZ_C_proc_end

OZ_CFun FSetDisjointPropagator::spawner = fsp_disjoint;

//*****************************************************************************

OZ_Return FSetIntersectionPropagator::run(void) 
{
  _OZ_DEBUGPRINT("in " << *this);

  OZ_FSetVar x(_x), y(_y), z(_z);
  PropagatorController_S_S_S P(x, y, z);
  FSetTouched xt, yt, zt;

loop:
  xt = x;  yt = y;  zt = z;

  if (z->isEmpty()) {
    _OZ_DEBUGPRINT("replace: (z empty)" << *this);
    P.vanish();
    return replaceBy(new FSetDisjointPropagator(_x, _y));
  }
  if (x->isSubsumedBy(*y)) {
    _OZ_DEBUGPRINT("replace: (x subsumbed by y)" << *this);
    P.vanish();
    return replaceBy(_x, _z);
  }
  if (y->isSubsumedBy(*x)) {
    _OZ_DEBUGPRINT("replace: (y subsumbed by xy)" << *this);
    P.vanish();
    return replaceBy(_y, _z);
  }
  
  FailOnInvalid(*x <= -(- *z & *y)); // lub
  OZ_DEBUGPRINT("x=" << *x);
  FailOnInvalid(*y <= -(- *z & *x)); // lub
  OZ_DEBUGPRINT("y=" << *y);

  FailOnInvalid(*z <<= (*x & *y)); // glb
  OZ_DEBUGPRINT("z=" << *z);
  FailOnInvalid(*x >= *z); // glb
  OZ_DEBUGPRINT("x=" << *x);
  FailOnInvalid(*y >= *z); // glb
  OZ_DEBUGPRINT("y=" << *y);

  if (xt <= x || yt <= y || zt <= z) 
    goto loop;

  _OZ_DEBUGPRINT("leave " << *this);
  return P.leave1();

failure:
  _OZ_DEBUGPRINT("failed " << *this);
  return P.fail();
}

OZ_Return FSetUnionPropagator::run(void) 
{
  _OZ_DEBUGPRINT("in " << *this);

  OZ_FSetVar x(_x), y(_y), z(_z);
  PropagatorController_S_S_S P(x, y, z);
  FSetTouched xt, yt, zt;

loop:
  xt = x;  yt = y;  zt = z;

  if (z->isEmpty()) {
    OZ_FSetConstraint aux(fs_empty);
    FailOnInvalid(*x <<= aux);
    FailOnInvalid(*y <<= aux);
    P.vanish();
    return ENTAILED;
  }
  if (x->isSubsumedBy(*y)) {
    P.vanish();
    return replaceBy(_y, _z);
  }
  if (y->isSubsumedBy(*x)) {
    P.vanish();
    return replaceBy(_x, _z);
  }

  FailOnInvalid(*x >= (*z & - *y)); // glb
  OZ_DEBUGPRINT("x=" << *x);
  FailOnInvalid(*y >= (*z & - *x)); // glb
  OZ_DEBUGPRINT("y=" << *y);

  FailOnInvalid(*z <<= (*x | *y)); // lub
  OZ_DEBUGPRINT("z=" << *z);
  FailOnInvalid(*x <= *z); // lub
  OZ_DEBUGPRINT("x=" << *x);
  FailOnInvalid(*y <= *z); // lub
  OZ_DEBUGPRINT("y=" << *y);

  if (xt <= x || yt <= y || zt <= z) 
    goto loop;

  _OZ_DEBUGPRINT("out " << *this);
  return P.leave1();

failure:
  _OZ_DEBUGPRINT("failed " << *this);
  return P.fail();
}

OZ_Return FSetSubsumePropagator::run(void) 
{
  OZ_DEBUGPRINT("int " << *this);
  OZ_FSetVar x(_x), y(_y);
  PropagatorController_S_S P(x, y);

  FailOnInvalid(*x <= *y);
  FailOnInvalid(*y >= *x);

  OZ_DEBUGPRINT("out " << *this);
  return P.leave1(); /* is entailed if only 
			one var is left */
  
failure:
  OZ_DEBUGPRINT("failed " << *this);
  return P.fail();
}

OZ_Return FSetDisjointPropagator::run(void) 
{
  OZ_DEBUGPRINT("in " << *this);
  OZ_FSetVar x(_x), y(_y);
  PropagatorController_S_S P(x, y);

  FailOnInvalid(*x != *y);
  FailOnInvalid(*y != *x);

  OZ_DEBUGPRINT("out " << *this);
  return P.leave1(); /* is entailed if only 
			one var is left */
  
failure:
  OZ_DEBUGPRINT("failed " << *this);
  return P.fail();
}
