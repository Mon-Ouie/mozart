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
 *     http://www.mozart-oz.org
 * 
 *  See the file "LICENSE" or
 *     http://www.mozart-oz.org/LICENSE.html
 *  for information on usage and redistribution 
 *  of this file, and for a DISCLAIMER OF ALL 
 *  WARRANTIES.
 *
 */

#include "std_n.hh"
#include "standard.hh"

#ifdef PROFILE
#define inline
#endif

//-----------------------------------------------------------------------------

#ifdef EXPERIMENT
#include <stdlib.h>
int sortVarsOrder(const void * _a, const void  * _b) {
  OZ_FSetVar a, b;
  a.ask(* (const OZ_Term *) _a);
  b.ask(* (const OZ_Term *) _b);
  return a->getKnownNotIn() -  b->getKnownNotIn();
}

void sortVars(FSetUnionNPropagator  &p)
{
  qsort(p._vs, p._vs_size, sizeof(OZ_Term), sortVarsOrder);
}
#endif    

//-----------------------------------------------------------------------------

OZ_BI_define(fsp_disjointN, 1, 0)
{
  OZ_EXPECTED_TYPE(OZ_EM_VECT OZ_EM_FSET);

  PropagatorExpect pe;

  OZ_EXPECT(pe, 0, expectVectorFSetVarBounds);
  
  return pe.impose(new FSetDisjointNPropagator(OZ_in(0)));
} 
OZ_BI_end

OZ_Return FSetDisjointNPropagator::propagate(void)
{
  OZ_DEBUGPRINTTHIS("in ");

  DECL_DYN_ARRAY(OZ_FSetVar, vs, _vs_size);
  DECL_DYN_ARRAY(FSetTouchedGlb, vst, _vs_size);
  PropagatorController_VS P(_vs_size, vs);
  int i;


  for (i = _vs_size; i--; ) {
    vs[i].read(_vs[i]);
    vst[i] = vs[i];
  }

  if (hasEqualVars()) 
    goto failure;

  {
  OZ_FSetValue u = _u;
  
  for (i = _vs_size; i--; ) { 
    OZ_FSetValue vsi_glb(vs[i]->getGlbSet());
    
    if ((u & vsi_glb).getCard() > 0)
      goto failure;

    u |= vsi_glb;
  }
  
  if (u.getCard() > 0) {
    OZ_Boolean doagain;
    
    do {
      doagain = OZ_FALSE;
      
      for (i = _vs_size; i--; ) {
	OZ_FSetValue vsi_glb(vs[i]->getGlbSet());
	
	FailOnInvalid(*vs[i] != (u - vsi_glb));
	
	if (vst[i] <= vs[i]) {
	  doagain = OZ_TRUE;
	  vst[i] = vs[i];
	  u |= vsi_glb;
	}
      }
      
    } while (doagain);
  }
  }
  {
    OZ_Return r = P.leave();
    
    if (r == OZ_SLEEP) {
      
      int j = 0;
      for (i = 0; i < _vs_size; i += 1) {
        if (vs[i]->isValue()) {
          _u |= vs[i]->getGlbSet();
          continue;
        }
        _vs[j] = _vs[i];
        j += 1;
      }
      _vs_size = j;
    }
    
    OZ_DEBUGPRINTTHIS("out ");
    return r;
  }

failure:
  OZ_DEBUGPRINTTHIS("failed");
  return P.fail();  
}

//-----------------------------------------------------------------------------

OZ_BI_define(fsp_unionN, 2, o)
{
  OZ_EXPECTED_TYPE(OZ_EM_VECT OZ_EM_FSET "," OZ_EM_FSET);

  PropagatorExpect pe;

  int susp_count = 0;

  OZ_EXPECT_SUSPEND(pe, 0, expectVectorFSetVarBounds, susp_count);
  OZ_EXPECT_SUSPEND(pe, 1, expectFSetVarBounds, susp_count);
  
  if (susp_count > 1) 
    return pe.suspend();

  return pe.impose(new FSetUnionNPropagator(OZ_in(0),
					    OZ_in(1)));
} 
OZ_BI_end


inline
OZ_FSetConstraint unionN(OZ_FSetVar x[], int high, int except)
{
  OZ_FSetConstraint z(fs_empty);

  for (int i = high; i--; )
    if (i != except) 
      z = z | *x[i];

  OZ_DEBUGPRINT(("unionN: expect=%d u=%s\n", except, z.toString()));
  return z;
}

inline
OZ_Boolean union3(OZ_FSetConstraint &x, 
		  OZ_FSetConstraint &y, 
		  OZ_FSetConstraint &z,
		  OZ_Boolean &touched)
{
  OZ_DEBUGPRINT(("union3 in: x=%s ", x.toString()));
  OZ_DEBUGPRINT(("union3 in: y=%s ", y.toString()));
  OZ_DEBUGPRINT(("union3 in: z=%s ", z.toString()));
  OZ_DEBUGPRINT(("union3 in: t=%d\n", touched));

  FSetTouched xt, yt, zt;

  OZ_Boolean repeat;

  do {
    repeat = OZ_FALSE;

    xt = x;  yt = y;  zt = z;
    
    FailOnInvalid(x >= (z - y)); // glb
    OZ_DEBUGPRINT(("union3: x=%s",x.toString()));
    
    FailOnInvalid(y >= (z - x)); // glb
    OZ_DEBUGPRINT(("union3: y=%s",y.toString()));
    
    FailOnInvalid(z <<= (x | y)); // lub
    OZ_DEBUGPRINT(("union3: z=%s",z.toString()));
    
    FailOnInvalid(x <= z); // lub
    OZ_DEBUGPRINT(("union3: x=%s",x.toString()));
    
    FailOnInvalid(y <= z); // lub
    OZ_DEBUGPRINT(("union3: y=%s",y.toString()));
    
    touched |= (repeat = (xt <= x || yt <= y || zt <= z));
  } while (repeat);

  OZ_DEBUGPRINT(("union3 out: x=%s ", x.toString()));
  OZ_DEBUGPRINT(("union3 out: y=%s ", y.toString()));
  OZ_DEBUGPRINT(("union3 out: z=%s ", z.toString()));
  OZ_DEBUGPRINT(("union3 out: t=%d\n", touched));

  return OZ_TRUE;

failure: 

  OZ_DEBUGPRINT(("union3 failed: x=%s ", x.toString()));
  OZ_DEBUGPRINT(("union3 failed: y=%s ", y.toString()));
  OZ_DEBUGPRINT(("union3 failed: z=%s\n", z.toString()));

  return OZ_FALSE;
} 

OZ_Return FSetUnionNPropagator::propagate(void)
{
  _OZ_DEBUGPRINTTHIS("in ");
  
  DECL_DYN_ARRAY(OZ_FSetVar, vs, _vs_size);
  OZ_FSetVar s(_s);
  OZ_FSetConstraint * aux = _aux;
  DECL_DYN_ARRAY(FSetTouched, vst, _vs_size);
  PropagatorController_VS_S P(_vs_size, vs, s);
  int i;
  OZ_Boolean isAllValues = OZ_TRUE;
  
  for (i = _vs_size; i--; ) {
    vs[i].read(_vs[i]);
    isAllValues &= vs[i]->isValue();
  }

  OZ_Boolean doagain;

  if (_vs_size == 0) {
    FailOnInvalid(*s <<= OZ_FSetConstraint(fs_empty));

    OZ_DEBUGPRINTTHIS("_vs_size == 0 out ");

    return P.vanish();
  } else if (_vs_size == 1) {
    P.vanish();
    
    OZ_DEBUGPRINTTHIS("_vs_size == 1 out");
      
    return replaceBy(_s, _vs[0]);
  } 

  if (isAllValues) {
    OZ_FSetValue aux(fs_empty);

    for (int j = _vs_size; j--; )
      aux |= vs[j]->getGlbSet();

    FailOnInvalid(*s <<= aux);

    P.vanish();
    _OZ_DEBUGPRINTTHIS("out ");
    return OZ_ENTAILED;
  }

  do {
    doagain = OZ_FALSE;

    for (i = 0; i < _vs_size-1; i += 1)
      if (!union3(aux[i], *vs[i], aux[i+1], doagain))
        goto failure;
    
    if (!union3(*vs[_vs_size-1], aux[_vs_size-1], *s, doagain))
      goto failure;

  } while (doagain);
  
  {
    OZ_Return r = P.leave();
    
    if (r == OZ_SLEEP) {
      
      int j = 0;
      for (i = 0; i < _vs_size; i += 1) {
        if (vs[i]->isEmpty())
	  continue;

	if (i != j) {
	  _vs[j] = _vs[i];
	  _aux[j] = _aux[i];
	}
        j += 1;
      }

      _vs_size = j;
    }
    
    _OZ_DEBUGPRINTTHIS("out ");
    return r;
  }

failure:
  _OZ_DEBUGPRINTTHIS("failed");
  return P.fail();  
}

//-----------------------------------------------------------------------------

OZ_BI_define(fsp_partition, 2, 0)
{
  OZ_EXPECTED_TYPE(OZ_EM_VECT OZ_EM_FSET "," OZ_EM_FSET);

  PropagatorExpect pe;

  int susp_count = 0;

  OZ_EXPECT_SUSPEND(pe, 0, expectVectorFSetVarBounds, susp_count);
  OZ_EXPECT_SUSPEND(pe, 1, expectFSetVarBounds, susp_count);
  
  if (susp_count > 1) 
    return pe.suspend();

  return pe.impose(new FSetPartitionPropagator(OZ_in(0),
					       OZ_in(1)));
} 
OZ_BI_end


OZ_Return FSetPartitionPropagator::propagate(void)
{
  _OZ_DEBUGPRINTTHIS("in ");
  
  OZ_FSetConstraint * aux = _aux;
  OZ_FSetVar s(_s);
      
  {
    DECL_DYN_ARRAY(OZ_FSetVar, vss, _vs_size);
    int i;
    for (i = _vs_size; i--; ) 
      vss[i].ask(_vs[i]);

    i = 0;
    int j = 0;
    for (; i < _vs_size; i += 1) {
      if (vss[i]->isEmpty() && i < _vs_size - 1) {
	if (!((i < _vs_size - 1 ? _aux[i+1] :  *s) <<= _aux[i])) {
	  s.fail();
	  return FAILED;
	}
	continue;
      }
      if (i != j) {
	_vs[j] = _vs[i];
	_aux[j] = _aux[i];
      }
      j += 1;
    }
    OZ_DEBUGPRINT(("%d:%d", j, _vs_size));

    _vs_size = j;
  }
  
  DECL_DYN_ARRAY(OZ_FSetVar, vs, _vs_size);
  DECL_DYN_ARRAY(FSetTouched, vst, _vs_size);
  PropagatorController_VS_S P(_vs_size, vs, s);
  int i;
  
  for (i = _vs_size; i--; ) 
    vs[i].read(_vs[i]);
  
  OZ_Boolean doagain;
    
#ifdef OZ_DEBUG
  for (int i = 0 ; i < _vs_size; i += 1)
    printf("vs[%d]=%s   aux[%d]=%s\n", 
	   i, vs[i]->toString(),
	   i, _aux[i].toString());
#endif

  if (_vs_size == 0) {
    OZ_DEBUGPRINTTHIS("_vs_size == 0 out");
    FailOnInvalid(*s <<= OZ_FSetConstraint(fs_empty));
    return P.vanish();
  } else if (_vs_size == 1) {
    P.vanish();
    
    OZ_DEBUGPRINTTHIS("_vs_size == 1 out");
      
    return replaceBy(_s, _vs[0]);
  } 

  {
    OZ_FSetValue u(fs_empty);
    
    for (i = _vs_size; i--; ) { 
      OZ_DEBUGPRINT(("vs[%d]=%s", i, vs[i]->toString()));
      OZ_DEBUGPRINT(("vs[%d]->getGlbSet=%s", i, vs[i]->getGlbSet().toString()));
      OZ_FSetValue vsi_glb(vs[i]->getGlbSet());
      OZ_DEBUGPRINT(("glb(vs[%d])=%s", i, vsi_glb.toString()));
      if ((u & vsi_glb).getCard() > 0) {
	OZ_DEBUGPRINT(("fuck"));
	goto failure;
      }
      u |= vsi_glb;
      OZ_DEBUGPRINT(("u=%s",  u.toString()));
    }
    

    OZ_Boolean dodisjoint = u.getCard() > 0;
    int count = 0, start = 0;

    do {
      doagain = OZ_FALSE;
      
      OZ_DEBUGPRINT(("0=%d", doagain));
      if (dodisjoint)
	for (i = _vs_size; i--; ) {
	  OZ_FSetValue vsi_glb(vs[i]->getGlbSet());
	  
	  FailOnInvalid(*vs[i] != (u - vsi_glb));
	  
	  if (vst[i] <= vs[i]) {
	    doagain = OZ_TRUE;
	    vst[i] = vs[i];
	    u |= vsi_glb;
	  }
	}
      
#ifdef OZ_DEBUG
  for (i = 0 ; i < _vs_size; i += 1)
    printf("after disjoint: vs[%d]=%s   aux[%d]=%s\n", 
	   i, vs[i]->toString(),
	   i, _aux[i].toString());
#endif
      
      int first = doagain ? -1 : -2;
      

      for (i = start; i < _vs_size - 1; i += 1) {
	OZ_DEBUGPRINT(("i=%d", i));
	if (!union3(aux[i], *vs[i], aux[i+1], doagain))
	  goto failure;
	if (first  == -2 && doagain == OZ_TRUE)
	  start = first = i;
      }
      if (start > 0 && !doagain) {
	start = 0;
	doagain = OZ_TRUE;
	OZ_DEBUGPRINT(("final check"));
      }

      if (!union3(*vs[_vs_size-1], aux[_vs_size-1], *s, doagain))
	goto failure;

      OZ_DEBUGPRINT(("3=%d", doagain));
      count ++;
      OZ_DEBUGPRINT(("%d first(%d)", count, first));
    } while (doagain);
    OZ_DEBUGPRINT((" %d:%d", count, _vs_size));
  }
  {
    OZ_Return r = P.leave();
    
    if (r == OZ_SLEEP) {
      
      int j = 0;
      for (i = 0; i < _vs_size; i += 1) {
        if (vs[i]->isEmpty())
	  continue;
	if (i != j) {
	  _vs[j] = _vs[i];
	  _aux[j] = _aux[i];
	}
        j += 1;
      }
      _vs_size = j;
    }
    
    _OZ_DEBUGPRINTTHIS("out ");
    return r;
  }

failure:
  _OZ_DEBUGPRINTTHIS("failed");
  return P.fail();  

}

OZ_PropagatorProfile FSetUnionNPropagator::profile;
OZ_PropagatorProfile FSetDisjointNPropagator::profile;
OZ_PropagatorProfile FSetPartitionPropagator::profile;

// eof 
//-----------------------------------------------------------------------------
