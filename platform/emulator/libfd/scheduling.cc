/*
  Hydra Project, DFKI Saarbruecken,
  Stuhlsatzenhausweg 3, D-66123 Saarbruecken, Phone (+49) 681 302-5312
  Author: wuertz
  Last modified: $Date$ from $Author$
  Version: $Revision$
  State: $State$

  ------------------------------------------------------------------------
*/

#define HOLES
//#define BOUNDS_OLD
//#define BOUNDS_NEW

#include "scheduling.hh"
#include "rel.hh"
#include "auxcomp.hh"
#include <stdlib.h>

static inline int intMin(int a, int b) { return a < b ? a : b; }
static inline int intMax(int a, int b) { return a > b ? a : b; }

//-----------------------------------------------------------------------------


OZ_C_proc_begin(sched_disjoint_card, 4)
{
  OZ_EXPECTED_TYPE(OZ_EM_FD "," OZ_EM_INT "," OZ_EM_FD "," OZ_EM_INT);
  
  PropagatorExpect pe;

  OZ_EXPECT(pe, 0, expectIntVarMinMax);
  OZ_EXPECT(pe, 1, expectInt);
  OZ_EXPECT(pe, 2, expectIntVarMinMax);
  OZ_EXPECT(pe, 3, expectInt);

  return pe.impose(new SchedCardPropagator(OZ_args[0], OZ_args[1], 
					   OZ_args[2], OZ_args[3]));
}
OZ_C_proc_end

OZ_Return SchedCardPropagator::propagate(void) 
{
  OZ_DEBUGPRINTTHIS("in: ");

  int &xd = reg_xd, &yd = reg_yd;

  OZ_FDIntVar x(reg_x), y(reg_y);
  PropagatorController_V_V P(x, y);
    
  int xl = x->getMinElem(), xu = x->getMaxElem();
  int yl = y->getMinElem(), yu = y->getMaxElem();
  
  if (xu + xd <= yl) return P.vanish();
  if (yu + yd <= xl) return P.vanish();
  
  if (xl + xd > yu) {
    P.vanish();
    return replaceBy(new LessEqOffPropagator(reg_y, reg_x, -yd));
  }

  if (yl + yd > xu) {
    P.vanish();
    return replaceBy(new LessEqOffPropagator(reg_x, reg_y, -xd));
  }
  
  
  OZ_DEBUGPRINTTHIS("out: ");

  return P.leave();

failure:
  OZ_DEBUGPRINTTHIS("fail");
  return P.fail();
}


//-----------------------------------------------------------------------------


static OZ_FDIntVar * xx;
static int * dd;

static int compareDescRel(const int &a, const int &b) {
  if (xx[a]->getMinElem() > xx[b]->getMinElem()) return 1;
  else return 0;
}

static int compareAscDue(const int &a, const int &b) {
  if (xx[a]->getMaxElem() + dd[a] < xx[b]->getMaxElem() + dd[b])
    return 1;
  else return 0;
}

// for cpIterate
struct StartDurTerms {
  OZ_Term start;
  int dur;
};


template <class T>
static void myqsort(T * my, int left, int right,   
	     int (*compar)(const T &a, const T &b))
{
  register int i = left, j = right;
  int middle = (left + right) / 2;
  T x = my[middle];

  do {
    while((*compar)(my[i], x) && (i < right)) i++;
    
    while((*compar)(x, my[j]) && j > left) j--;
    
    if (i <= j) {
      T aux = my[i];
      my[i] = my[j];
      my[j] = aux;
      i++; 
      j--;
    }
  } while(i <= j);
  
  if (left < j) myqsort(my, left, j, compar);
  if (i < right) myqsort(my, i, right, compar);
}

int compareDurs(const StartDurTerms &a, const StartDurTerms &b) {
  if ( a.dur > b.dur) return 1;
  else return 0;
}


// for cpIterateCap
struct StartDurUseTerms {
  OZ_Term start;
  int dur;
  int use;
};

int compareDursUse(const StartDurUseTerms &a, const StartDurUseTerms &b) {
  if (a.dur * a.use > b.dur * b.use) 
    return 1;
  else return 0;
}


CPIteratePropagator::CPIteratePropagator(OZ_Term tasks, 
					 OZ_Term starts, 
					 OZ_Term durs)
  : Propagator_VD_VI(OZ_vectorSize(tasks))
{
  VectorIterator vi(tasks);
  int i = 0;
		     
  DECL_DYN_ARRAY(StartDurTerms, sd, reg_sz);

  while (vi.anyLeft()) {
    OZ_Term task = vi.getNext();

    sd[i].start = OZ_subtree(starts, task);
    sd[i].dur = OZ_intToC(OZ_subtree(durs, task));
    i += 1;
  } // while

  OZ_ASSERT(i == reg_sz);

  myqsort(sd,0,reg_sz-1, compareDurs);

  for (i = reg_sz; i--; ) {
    reg_l[i]      = sd[i].start;
    reg_offset[i] = sd[i].dur;
  }
}

OZ_C_proc_begin(sched_cpIterate, 3)
{
  OZ_EXPECTED_TYPE(OZ_EM_VECT OZ_EM_VECT OZ_EM_LIT "," 
		   OZ_EM_RECORD OZ_EM_FD "," OZ_EM_RECORD OZ_EM_INT);
  
  {
    PropagatorExpect pe;
    
    OZ_EXPECT(pe, 0, expectVectorVectorLiteral);
    OZ_EXPECT(pe, 1, expectProperRecordIntVarMinMax);
    OZ_EXPECT(pe, 2, expectProperRecordInt);
    SAMELENGTH_VECTORS(1, 2);
  }
  
  OZ_Term starts = OZ_args[1], durs = OZ_args[2];

  VectorIterator vi(OZ_args[0]);


  for (int i = OZ_vectorSize(OZ_args[0]); i--; ) {
    OZ_Term tasks = vi.getNext();

    PropagatorExpect pe;

    VectorIterator vi_tasks(tasks);
    while (vi_tasks.anyLeft()) {
      OZ_Term task = vi_tasks.getNext();
      OZ_Term start_task = OZ_subtree(starts, task);
      OZ_Term dur_task = OZ_subtree(durs, task);
      if (!start_task || !dur_task)  
	return OZ_typeError(expectedType, 0, "Scheduling applications expect that all task symbols are features of the records denoting the start times and durations.");
      pe.expectIntVarMinMax(OZ_subtree(starts, task));
    }
    OZ_Return r = pe.impose(new CPIteratePropagator(tasks, starts, durs),
			    OZ_getLowPrio());

    if (r == FAILED) return FAILED;
  }
  return OZ_ENTAILED;
}
OZ_C_proc_end

//-----------------------------------------------------------------------------

class Min_max {
public:
  int min, max;
};

struct Set {
  int cSi, dSi, mSi, sUp, sLow, extSize, val;
  int min,max;
  int * ext;
};


OZ_Return CPIteratePropagator::propagate(void)
{
  int &ts  = reg_sz;
  int * dur = reg_offset;

  // if we have no tasks the prop returns trivially true
  if (ts == 0) return PROCEED;

  DECL_DYN_ARRAY(OZ_FDIntVar, x, ts);
  PropagatorController_VV P(ts, x);
  
  int i, j;
  for (i = ts; i--; )
    x[i].read(reg_l[i]);

  xx = x;
  dd = reg_offset;

  int upFlag = 0;
  int downFlag = 0;
  int disjFlag = 0;


  int kUp;
  int kDown;
  int dur0;
  int mSi;

  DECL_DYN_ARRAY(int, set0, ts);
  DECL_DYN_ARRAY(int, compSet0, ts);
  DECL_DYN_ARRAY(int, forCompSet0Up, ts);
  DECL_DYN_ARRAY(int, forCompSet0Down, ts);
  DECL_DYN_ARRAY(int, outSide, ts);

  DECL_DYN_ARRAY(Min_max, MinMax, ts);
  for (i=ts; i--;){
    MinMax[i].min = x[i]->getMinElem();
    MinMax[i].max = x[i]->getMaxElem();
    forCompSet0Up[i] = i;
    forCompSet0Down[i] = i;
  }

  int set0Size;
  int compSet0Size;
  int outSideSize;
  int mysup = OZ_getFDSup();
  
  DECL_DYN_ARRAY(Set, Sets, ts);


  for (i = ts; i--; )
    Sets[i].ext = (int *) OZ_FDIntVar::operator new(sizeof(int) * ts);

  // memory is automatically disposed when propagator is left

  //////////  
  // do the reified stuff for task pairs.
  //////////  
  for (i=0; i<ts; i++)
    for (j=i+1; j<ts; j++) {
      int xui = MinMax[i].max, di = dur[i], xlj = MinMax[j].min;
      if (xui + di <= xlj) continue;
      int xuj = MinMax[j].max, dj = dur[j], xli = MinMax[i].min;
      if (xuj + dj <= xli) continue;
      if (xli + di > xuj) {
	int val1 = xui - dj;
	if (xuj > val1) {
	  FailOnEmpty(*x[j] <= val1);
	  MinMax[j].max = x[j]->getMaxElem();
	}
	int val2 = xlj + dj;
	if (xli < val2) {
	  FailOnEmpty(*x[i] >= val2);
	  MinMax[i].min = x[i]->getMinElem();
	}
      }
      if (xlj + dj > xui) {
	int val1 = xuj - di;
	if (xui > val1) {
	  FailOnEmpty(*x[i] <= val1);
	  MinMax[i].max = x[i]->getMaxElem();
	}
	int val2 = xli + di;
	if (xlj < val2) {
	  FailOnEmpty(*x[j] >= val2);
	  MinMax[j].min = x[j]->getMinElem();
	}
      }
    }

cploop:

  //////////  
  // UPPER PHASE
  //////////  


  /*
  // it's not worth it
  // Cut a hole into tasks if it must be scheduled in the following interval
  for (i=0; i<ts; i++)
    if (x[i]->getMaxElem() < x[i]->getMinElem() + dur[i]) {
      OZ_FiniteDomain lb;
      lb.initRange(x[i]->getMinElem()+dur[i], mysup);
      for (j=0; j<ts; j++)
	if (i != j) {
	  OZ_FiniteDomain la, l1;
	  la.initRange(0, x[i]->getMaxElem()- dur[j]);
	  l1 = (la | lb);
	  FailOnEmpty(*x[j] &= l1);
	}
    }
    */

  //////////  
  // sort by descending release date; ie. min(s1) > min(s2) > min(s3) etc.
  //////////  
  myqsort(forCompSet0Up, 0, ts-1,compareDescRel );

  {
  for (int upTask=0; upTask < ts; upTask++) {
    
    kUp = MinMax[upTask].max + dur[upTask];
    kDown = MinMax[upTask].min;
    dur0 = 0;
    mSi = mysup;
    int maxSi = 0;
    set0Size = 0;
    compSet0Size = 0;
    outSideSize = 0;
    int maxEct = 0;
    int overlap = 0;

    //////////  
    // compute set S0 
    //////////  
    int l;
    for (l=0; l<ts; l++) {
      int dl = dur[l];
      int xlMin = MinMax[l].min;
      int xlMaxDL = MinMax[l].max + dl;
      if (( kDown <= xlMin) && ( xlMaxDL <= kUp)) {
	dur0 = dur0 + dl;
	maxEct = intMax(maxEct,xlMin+dl); 
	mSi = intMin( mSi, xlMin+dl );
	maxSi = intMax( maxSi, MinMax[l].max );
	set0[set0Size++] = l;
      }
      else {
	int overlapTmp = intMin(intMax(0,xlMin+dl-kDown),
				intMin(intMax(0,kUp-xlMaxDL+dl),
				       intMin(dl,kUp-kDown)));
	overlap += overlapTmp;
	if (xlMaxDL > kUp) {
	  outSide[outSideSize++] = l;
	}
      }
    }
    for (l=0; l<ts; l++) {
      int realL = forCompSet0Up[l];
      if ( (MinMax[realL].min < kDown) && 
	   (MinMax[realL].max + dur[realL] <= kUp) )
	compSet0[compSet0Size++] = realL;
    }

    if (kUp-kDown < dur0+overlap) goto failure;
    
    struct Set *oset = &Sets[0];	
    oset ->cSi = intMax(kDown +dur0, maxEct);
    oset->dSi = dur0;
    oset->mSi = mSi;
    oset->min = mSi;
    oset->max = maxSi;
    oset->sUp = kUp;
    oset->sLow = kDown;
    oset->extSize = set0Size;
    {
      for (int k = 0; k < ts; k++) {
	oset->ext[k] = set0[k];
      }
    }

    int setSize = 0;

    //////////  
    // compute the sets Si
    //////////  
    for (l=0; l<compSet0Size; l++) {
      int realL = compSet0[l];
      int dl    = dur[realL];
      if (MinMax[realL].max+dl <= kUp) {
	int setSizeBefore = setSize;	
	struct Set *bset = &Sets[setSizeBefore];	
	setSize++;
	int dSi = bset->dSi + dl;
	int minL = MinMax[realL].min;
	maxEct = intMax(maxEct, minL+dl);
	int newCSi = intMax(bset->cSi, intMax(minL+dSi, maxEct));
	if (newCSi > kUp) 
	  goto failure;
	else {
	  struct Set *cset = &Sets[setSize];	
	  cset->cSi = newCSi;
	  cset->dSi = dSi;
	  cset->mSi = intMin(bset->mSi, minL+dl);
	  cset->min = cset->mSi;
	  cset->max = intMax(bset->max, MinMax[realL].max);
	  cset->sUp = kUp;
	  cset->sLow = minL;
	  cset->extSize = bset->extSize+1;
	  cset->val = realL;
	}
      }
    }

    /*
    //////////  
    // edgepushUp
    //////////  

    for (int t1=setSize; t1>=0; t1--) { 
      for (int t2=0; t2<ts; t2++) {
	struct Set *s = &Sets[t1];	
	if (MinMax[t2].min > s->sLow) {
	  int tx;
	  if (MinMax[t2].max+dur[t2] <= kUp) {
	    tx = MinMax[t2].min;
	  }
	  else {
	    tx = MinMax[t2].min+dur[t2];
	  }
	  if ((tx > kUp - s->dSi) && (MinMax[t2].min < s->mSi)) {
	    upFlag = 1;
	    FailOnEmpty(*x[t2] >= s->mSi);
	  }
	}
      }
    }
    */


    //////////  
    // Do the edge-finding
    //////////  
    int lCount = 0;
    int setCount = setSize;
    while ( (setCount >= 0) && (lCount < outSideSize) ) {
      int l = outSide[lCount];
      struct Set *s = &Sets[setCount];
      int minL = MinMax[l].min;
      int maxL = MinMax[l].max;
      int durL = dur[l];

      if (maxL+durL > s->sUp) {
	if ( (s->sUp - s->sLow >= s->dSi + durL) &&
	     (minL+durL <= s->max) &&
	     (s->min <= maxL) )
	     {
	  // case 4: l may be inside
          if (s->sUp - minL >= s->dSi + durL) {
	    // case 5: L may be first
	    lCount++; 
	    setCount--;
	  }
	  else {
	    // it cannot be first
	    setCount--;
	    FailOnEmpty(*x[l] >= s->mSi);
	  }
	}
	else {
	  if (s->sUp - minL >= s->dSi + durL) {
	    // case 5: L may be first
	    lCount++;
	  }
	  else {

	    // l must be last
            if (minL < s->cSi) {
	      upFlag = 1;
	      FailOnEmpty(*x[l] >= s->cSi);
	      for (i=0; i < Sets[0].extSize; i++) {
		int sext = Sets[0].ext[i];
		int right = maxL-dur[sext];
		if (right < 0) 
		  goto failure;
		if (MinMax[sext].max > right) {
		  FailOnEmpty(*x[sext] <= right);
		}
	      }

	      for (i=1; i <= setCount; i++) {
		int sext = Sets[i].val;
		int right = maxL-dur[sext];
		if (right < 0) 
		  goto failure;
		if (MinMax[sext].max > right) {
		  FailOnEmpty(*x[sext] <= right);
		}
	      }

	      lCount++;
	    }
	    else lCount++;
	  }
	}
      }
      else lCount++;
    }

  }
  }

  for (i=0; i<ts; i++) {
    MinMax[i].min = x[i]->getMinElem();	
    MinMax[i].max = x[i]->getMaxElem();	
  }

  //////////  
  // DOWN PHASE
  //////////  

  
  //////////  
  // sort by ascending due date; ie. intMax(s1)+dur(s1) < max(s2)+dur(s2)
  //////////  
  myqsort(forCompSet0Down, 0, ts-1, compareAscDue);


  {
  for (int downTask=0; downTask < ts; downTask++) {
    
    kUp = MinMax[downTask].max + dur[downTask];
    kDown = MinMax[downTask].min;
    dur0 = 0;
    mSi = 0;
    int minSi = mysup;
    set0Size = 0;
    compSet0Size = 0;
    outSideSize = 0;
    int minLst =  mysup;
    int overlap = 0;
    
    //////////  
    // compute set S0 
    //////////  
    int l;
    for (l=0; l<ts; l++) {
      int dl = dur[l];
      int xlMin = MinMax[l].min;
      int xlMax = MinMax[l].max;
      int xlMaxDL = xlMax + dl;
      if (( kDown <= xlMin) && ( xlMaxDL <= kUp)) {
	dur0 = dur0 + dl;
	mSi = intMax( mSi, xlMax );
	minSi = intMin( minSi, xlMin+dl );
	minLst = intMin(minLst,xlMax);
	set0[set0Size++] = l;
      }
      else {
	int overlapTmp = intMin(intMax(0,xlMin+dl-kDown),
				intMin(intMax(0,kUp-xlMaxDL+dl),
				       intMin(dl,kUp-kDown)));
	overlap += overlapTmp;
	if (xlMin < kDown) {
	  outSide[outSideSize++] = l;
	}
      }
    }
    
    for (l=0; l<ts; l++) {
      int realL = forCompSet0Down[l];
      if ( (MinMax[realL].min >= kDown) && 
	   (MinMax[realL].max + dur[realL] > kUp) )
	compSet0[compSet0Size++] = realL;
    }

    if (kUp-kDown < dur0+overlap) goto failure;

    struct Set *oset = &Sets[0];
    oset->cSi = intMin(kUp - dur0, minLst);
    oset->dSi = dur0;
    oset->mSi = mSi;
    oset->max = mSi;
    oset->min = minSi;
    oset->sUp = kUp;
    oset->sLow = kDown;
    oset->extSize = set0Size;
    {
      for (int k = 0; k < ts; k++) {
	oset->ext[k] = set0[k];
      }
    }

    int setSize = 0;

    //////////  
    // compute the sets Si
    //////////  
    {
      int l;
      for (l=0; l<compSet0Size; l++) {
	int realL = compSet0[l];
	if (MinMax[realL].min >= kDown) {
	  int setSizeBefore = setSize;	
	  struct Set *bset = &Sets[setSizeBefore];	
	  int durL = dur[realL];
	  setSize++;
	  int dSi = bset->dSi + durL;
	  int maxL = MinMax[realL].max+durL;
	  minLst = intMin(minLst, maxL-durL);
	  int newCSi = intMin(bset->cSi, intMin(maxL-dSi, minLst));
	  if (newCSi < kDown) 
	    goto failure;
	  else {
	    struct Set *cset = &Sets[setSize];	
	    cset->cSi = newCSi;
	    cset->dSi = dSi;
	    cset->mSi = intMax(bset->mSi, maxL-durL);
	    cset->min = intMin(bset->min, MinMax[realL].min+dur[realL]);
	    cset->max = cset->mSi;
	    cset->sUp = maxL;
	    cset->sLow = kDown;
	    cset->extSize = bset->extSize+1;
	    cset->val = realL;
	  }
	}
      }
    }
    
    /*
    //////////  
    // edgepushDown
    //////////  
    for (int t1=setSize; t1>=0; t1--) {
      for (int t2=0; t2<ts; t2++) {
	struct Set *s = &Sets[t1];	
	int durT2 = dur[t2];
	int t2Max = MinMax[t2].max;
	if (t2Max+durT2 < s->sUp) {
	  int tx;
	  if (MinMax[t2].min >= kDown) {
	    tx = t2Max+durT2;
	  }
	  else {
	    tx = t2Max;
	  }
	  if ((tx < kDown + s->dSi) && (t2Max+durT2 > s->mSi)) {
	    downFlag = 1;
	    FailOnEmpty(*x[t2] <= s->mSi-durT2);
	  }
	}
      }
    }
    */

    //////////  
    // DO the edge finding
    //////////  
    int lCount = 0;
    int setCount = setSize;
    while ( (setCount >= 0) && (lCount < outSideSize) ) {
      int l = outSide[lCount];
      struct Set *s = &Sets[setCount];
      int minL = MinMax[l].min;
      int maxL = MinMax[l].max;
      int durL = dur[l];

      if (minL < s->sLow) {
	if ( (s->sUp - s->sLow >= s->dSi + durL) &&
	     (minL+durL <= s->max) &&
	     (s->min <= maxL) )
	     {
	  // case 4: l may be inside
	    if (maxL + durL - s->sLow >= s->dSi + durL) {
	    // case 5: L may be last
	    lCount++; 
	    setCount--;
	  }
	  else {
	    // it cannot be last
	      setCount--;
	      FailOnEmpty(*x[l] <= s->mSi - durL);
	  }
	}
	else {
	  if (maxL + durL - s->sLow >= s->dSi + durL) {
	    // case 5: L may be last
	    lCount++;
	  }
	  else {

	    // l must be first
            if (maxL+durL > s->cSi) {
	      downFlag = 1;
	      int right = s->cSi - durL;
	      if (right < 0)
	        goto failure;
	      FailOnEmpty(*x[l] <= right);

	      int minDL = minL + durL;
	      for (i=0; i < Sets[0].extSize; i++) {
		int element = Sets[0].ext[i];
		if (MinMax[element].min < minDL) {
		  FailOnEmpty(*x[element] >= minDL);
		}
	      }

	      for (i=1; i <= setCount; i++) {
		int element = Sets[i].val;
		if (MinMax[element].min < minDL) {
		  FailOnEmpty(*x[element] >= minDL);
		}

	      }

	      lCount++; 
	    }
	    else lCount++;

	  }
	}
      }
      else lCount++;
    }


  }
  }

  for (i=0; i<ts; i++) {
    MinMax[i].min = x[i]->getMinElem();	
    MinMax[i].max = x[i]->getMaxElem();	
  }

  
  if ((upFlag == 1)||(downFlag==1)) {
    upFlag = 0;
    downFlag = 0;
    goto cploop;
  }


reifiedloop:

   //////////
   // do the reification in a loop
   //////////
   for (i=0; i<ts; i++)
     for (j=i+1; j<ts; j++) {
       int xui = MinMax[i].max, di = dur[i], xlj = MinMax[j].min;
       if (xui + di <= xlj) continue;
       int xuj = MinMax[j].max, dj = dur[j], xli = MinMax[i].min;
       if (xuj + dj <= xli) continue;
       if (xli + di > xuj) {
	 if (xuj > xui - dj) {
	   disjFlag = 1;
	   FailOnEmpty(*x[j] <= xui - dj);
	   MinMax[j].max = x[j]->getMaxElem();
	 }
	 if (xli < xlj + dj) {
	   disjFlag = 1;
	   FailOnEmpty(*x[i] >= xlj + dj);
	   MinMax[i].min = x[i]->getMinElem();
	 }
       }
       if (xlj + dj > xui) {
	 if (xui > xuj - di) {
	   disjFlag = 1;
	   FailOnEmpty(*x[i] <= xuj - di);
	   MinMax[i].max = x[i]->getMaxElem();
	 }
	 if (xlj < xli + di) {
	   disjFlag = 1;
	   FailOnEmpty(*x[j] >= xli + di);
	   MinMax[j].min = x[j]->getMinElem();
	 }
       }
     }

  if (disjFlag == 1) {
    disjFlag = 0;
    goto reifiedloop;
  }

  return P.leave();

failure:
  return P.fail();
}

//--------------------------------------------------------------

OZ_C_proc_begin(sched_disjunctive, 3)
{
  OZ_EXPECTED_TYPE(OZ_EM_VECT OZ_EM_VECT OZ_EM_LIT "," 
		   OZ_EM_RECORD OZ_EM_FD "," OZ_EM_RECORD OZ_EM_INT);
    
  {
    PropagatorExpect pe;
    
    OZ_EXPECT(pe, 0, expectVectorVectorLiteral);
    OZ_EXPECT(pe, 1, expectProperRecordIntVarMinMax);
    OZ_EXPECT(pe, 2, expectProperRecordInt);
    SAMELENGTH_VECTORS(1, 2);
  }
  
  OZ_Term starts = OZ_args[1], durs = OZ_args[2];

  VectorIterator vi(OZ_args[0]);

  for (int i = OZ_vectorSize(OZ_args[0]); i--; ) {
    OZ_Term tasks = vi.getNext();

    PropagatorExpect pe;

    VectorIterator vi_tasks(tasks);
    while (vi_tasks.anyLeft()) {
      OZ_Term task = vi_tasks.getNext();
      OZ_Term start_task = OZ_subtree(starts, task);
      OZ_Term dur_task = OZ_subtree(durs, task);
      if (!start_task || !dur_task) 
	return OZ_typeError(expectedType, 0, "Scheduling applications expect that all task symbols are features of the records denoting the start times and durations.");
      pe.expectIntVarMinMax(OZ_subtree(starts, task));
    }

    OZ_Return r = pe.impose(new DisjunctivePropagator(tasks, starts, durs),
			    OZ_getLowPrio());

    if (r == FAILED) return FAILED;
  }
  return OZ_ENTAILED;

}
OZ_C_proc_end

DisjunctivePropagator::DisjunctivePropagator(OZ_Term tasks, 
					     OZ_Term starts, 
					     OZ_Term durs)
  : Propagator_VD_VI(OZ_vectorSize(tasks))
{
  VectorIterator vi(tasks);
  int i = 0;
		     
  while (vi.anyLeft()) {
    OZ_Term task = vi.getNext();

    reg_l[i]      = OZ_subtree(starts, task);
    reg_offset[i] = OZ_intToC(OZ_subtree(durs, task));
    i += 1;
  } // while
    
    OZ_ASSERT(i == reg_sz);

}


OZ_Return DisjunctivePropagator::propagate(void)
{
  int &ts  = reg_sz;
  int * dur = reg_offset;

  // if we have no tasks the prop returns trivially true
  if (ts == 0) return PROCEED;

  DECL_DYN_ARRAY(OZ_FDIntVar, x, ts);
  PropagatorController_VV P(ts, x);
  
  int i, j;
  for (i = ts; i--; )
    x[i].read(reg_l[i]);

  DECL_DYN_ARRAY(Min_max, MinMax, ts);
  for (i=ts; i--;){
    MinMax[i].min = x[i]->getMinElem();
    MinMax[i].max = x[i]->getMaxElem();
  }


  int disjFlag = 0;

reifiedloop:

   for (i=0; i<ts; i++)
     for (j=i+1; j<ts; j++) {
       int xui = MinMax[i].max, di = dur[i], xlj = MinMax[j].min;
       if (xui + di <= xlj) continue;
       int xuj = MinMax[j].max, dj = dur[j], xli = MinMax[i].min;
       if (xuj + dj <= xli) continue;
       if (xli + di > xuj) {
	 if (xuj > xui - dj) {
	   disjFlag = 1;
	   FailOnEmpty(*x[j] <= xui - dj);
	   MinMax[j].max = x[j]->getMaxElem();
	 }
	 if (xli < xlj + dj) {
	   disjFlag = 1;
	   FailOnEmpty(*x[i] >= xlj + dj);
	   MinMax[i].min = x[i]->getMinElem();
	 }
       }
       if (xlj + dj > xui) {
	 if (xui > xuj - di) {
	   disjFlag = 1;
	   FailOnEmpty(*x[i] <= xuj - di);
	   MinMax[i].max = x[i]->getMaxElem();
	 }
	 if (xlj < xli + di) {
	   disjFlag = 1;
	   FailOnEmpty(*x[j] >= xli + di);
	   MinMax[j].min = x[j]->getMinElem();
	 }
       }
     }

  if (disjFlag == 1) {
    disjFlag = 0;
    goto reifiedloop;
  }


  return P.leave();

failure:
  return P.fail();
}

//-----------------------------------------------------------------------------

struct Interval {
  int left, right, use;
};

int ozcdecl CompareIntervals(const Interval &Int1, const Interval &Int2) 
{
  int left1 = Int1.left;
  int left2 = Int2.left;
  if (left1 >= left2) return 0;
  else {
    if (left1 == left2) {
      if (Int1.right < Int2.right) return 1;
      else return 0;
    }
    else return 1;
  }
}

inline int EnergyFunct(int t1, int t2, int dura, int usea, int ra, int da)
{
  return intMax(0,
	     usea*intMin(dura, 
		      intMin(t2 - t1,
			  intMin(ra + dura - t1,
			      t2 - da + dura))));
}

int ozcdecl CompareBounds(const int &Int1, const int &Int2) {
  if (Int1 < Int2) return 1;
  else return 0;
}

//////////
// CONSTRUCTOR
//////////
CPIteratePropagatorCap::CPIteratePropagatorCap(OZ_Term tasks, 
					       OZ_Term starts, 
					       OZ_Term durs, 
					       OZ_Term use,
					       OZ_Term cap,
					       int flag)
  : Propagator_VD_VI_VI_I(OZ_vectorSize(tasks))
{

  reg_capacity = OZ_intToC(cap);

  VectorIterator vi(tasks);
  int i = 0;
		     
  DECL_DYN_ARRAY(StartDurUseTerms, sdu, reg_sz);

  while (vi.anyLeft()) {
    OZ_Term task = vi.getNext();

    sdu[i].start = OZ_subtree(starts, task);
    sdu[i].dur = OZ_intToC(OZ_subtree(durs, task));
    sdu[i].use = OZ_intToC(OZ_subtree(use, task));
    i += 1;
  } // while

  OZ_ASSERT(i == reg_sz);

  myqsort(sdu, 0, reg_sz-1, compareDursUse);

  for (i = reg_sz; i--; ) {
    reg_l[i]      = sdu[i].start;
    reg_offset[i] = sdu[i].dur;
    reg_use[i]    = sdu[i].use;
  }

  reg_flag = flag;
}

//////////
// BUILTIN
//////////
OZ_C_proc_begin(sched_cpIterateCap, 6)
{
  OZ_EXPECTED_TYPE(OZ_EM_VECT OZ_EM_VECT OZ_EM_LIT "," OZ_EM_RECORD OZ_EM_FD 
		   "," OZ_EM_RECORD OZ_EM_INT "," OZ_EM_RECORD OZ_EM_INT 
		   "," OZ_EM_VECT OZ_EM_INT "," OZ_EM_INT);
  
  {
    PropagatorExpect pe;
    
    OZ_EXPECT(pe, 0, expectVectorVectorLiteral);
    OZ_EXPECT(pe, 1, expectProperRecordIntVarMinMax);
    OZ_EXPECT(pe, 2, expectProperRecordInt);
    OZ_EXPECT(pe, 3, expectProperRecordInt);
    OZ_EXPECT(pe, 4, expectVectorInt);
    OZ_EXPECT(pe, 5, expectInt);
    SAMELENGTH_VECTORS(1, 2);
    SAMELENGTH_VECTORS(1, 3);
  }

  OZ_Term starts = OZ_args[1], durs = OZ_args[2], use = OZ_args[3], 
    caps = OZ_args[4];


  VectorIterator vi(OZ_args[0]);
  VectorIterator viCap(caps);

  for (int i = 0; i < OZ_vectorSize(OZ_args[0]); i++) {
    OZ_Term tasks    = vi.getNext();
    OZ_Term capacity = viCap.getNext();

    PropagatorExpect pe;

    VectorIterator vi_tasks(tasks);
    while (vi_tasks.anyLeft()) {
      OZ_Term task = vi_tasks.getNext();
      OZ_Term start_task = OZ_subtree(starts, task);
      OZ_Term dur_task = OZ_subtree(durs, task);
      OZ_Term use_task = OZ_subtree(use, task);
      if (!start_task || !dur_task || !use_task) 
	return OZ_typeError(expectedType, 0, "Scheduling applications expect that all task symbols are features of the records denoting the start times, the durations and the resource usages.");
      pe.expectIntVarMinMax(OZ_subtree(starts, task));
    }

    OZ_Return r = pe.impose(new CPIteratePropagatorCap(tasks, starts, durs, 
						       use, capacity,
						       OZ_intToC(OZ_args[5])),
			    OZ_getLowPrio());

    if (r == FAILED) return FAILED;
  }
  return OZ_ENTAILED;
  
}
OZ_C_proc_end


struct Set2 {
  int dSi, sUp, sLow, extSize;
  int * ext;
};

//////////
// RUN METHOD
//////////
OZ_Return CPIteratePropagatorCap::propagate(void)
{

  //////////
  // Cumulative constraint: lean version of edge-finding 
  // and some interval stuff
  //////////
  int &ts      = reg_sz;
  int * dur    = reg_offset;
  int * use    = reg_use;
  int capacity = reg_capacity;

  // if we have no tasks the prop returns trivially true
  if (ts == 0) return PROCEED;

  DECL_DYN_ARRAY(OZ_FDIntVar, x, ts);

  PropagatorController_VV P(ts, x);
  
  int i, j;
  for (i = ts; i--; )
    x[i].read(reg_l[i]);

  xx = x;
  dd = reg_offset;

  
  int upFlag = 0;
  int downFlag = 0;
  int disjFlag = 0;
  int cap_flag = 0;

  int kUp;
  int kDown;
  int dur0;
  int mSi;

  DECL_DYN_ARRAY(int, set0, ts);
  DECL_DYN_ARRAY(int, compSet0, ts);
  DECL_DYN_ARRAY(int, forCompSet0Up, ts);
  DECL_DYN_ARRAY(int, forCompSet0Down, ts);
  DECL_DYN_ARRAY(int, outSide, ts);

  DECL_DYN_ARRAY(Min_max, MinMax, ts);
  for (i=ts; i--;){
    MinMax[i].min = x[i]->getMinElem();
    MinMax[i].max = x[i]->getMaxElem();
    forCompSet0Up[i] = i;
    forCompSet0Down[i] = i;
  }

  int set0Size;
  int compSet0Size;
  int outSideSize;
  int mysup = OZ_getFDSup();
  
  DECL_DYN_ARRAY(Set2, Sets, ts);
  
  for (i = ts; i--; )
    Sets[i].ext = (int *) OZ_FDIntVar::operator new(sizeof(int) * ts);


  // memory is automatically disposed when propagator is left
       
    //////////  
    // do the reified stuff for task pairs.
    //////////  
    for (i=0; i<ts; i++)
      for (j=i+1; j<ts; j++) {
	if (use[i] + use[j] > capacity) {
	  int xui = MinMax[i].max, di = dur[i], xlj = MinMax[j].min;
	  if (xui + di <= xlj) continue;
	  int xuj = MinMax[j].max, dj = dur[j], xli = MinMax[i].min;
	  if (xuj + dj <= xli) continue;
	  if (xli + di > xuj) {
	    int val1 = xui - dj;
	    if (xuj > val1) {
	      FailOnEmpty(*x[j] <= val1);
	      MinMax[j].max = x[j]->getMaxElem();
	    }
	    int val2 = xlj + dj;
	    if (xli < val2) {
	      FailOnEmpty(*x[i] >= val2);
	      MinMax[i].min = x[i]->getMinElem();
	    }
	  }
	  if (xlj + dj > xui) {
	    int val1 = xuj - di;
	    if (xui > val1) {
	      FailOnEmpty(*x[i] <= val1);
	      MinMax[i].max = x[i]->getMaxElem();
	    }
	    int val2 = xli + di;
	    if (xlj < val2) {
	      FailOnEmpty(*x[j] >= val2);
	      MinMax[j].min = x[j]->getMinElem();
	    }
	  }
	}
      }

  /*
// it is not worth the effort
// energy from Baptistes thesis
for (i=0; i<ts; i++) {

  for (j=0; j<ts; j++) {
    int ra = x[i]->getMinElem();
    int sum1 = 0;
    int val = x[j]->getMinElem();
    for (int k = 0; k<ts; k++) {
      if (i != k)
	sum1 = sum1 + EnergyFunct(ra, val, dur[k], use[k], 
				  x[k]->getMinElem(),
				  x[k]->getMaxElem() + dur[k]);
    }
    int sum2 = sum1 + use[i] * intMin(dur[i], val - ra);
    if ( capacity*(val - ra) < sum2) {
      int up = (int) ceil( (double) sum1 / (double) capacity);
      if (ra + up > x[i]->getMinElem()) {
	FailOnEmpty(*x[i] >= ra + up);
      }
    }
    sum1 = 0;
    val = x[j]->getMaxElem() + dur[j];
    for (int k = 0; k<ts; k++) {
      if (i != k)
	sum1 = sum1 + EnergyFunct(ra, val, dur[k], use[k], 
				  x[k]->getMinElem(),
				  x[k]->getMaxElem() + dur[k]);
    }
    sum2 = sum1 + use[i] * intMin(dur[i], val - ra);
    if ( capacity*(val - ra) < sum2) {
      int up = (int) ceil( (double) sum1 / (double) capacity);
      if (ra + up > x[i]->getMinElem()) {
	FailOnEmpty(*x[i] >= ra + up);
      }
    }
  }

  for (j=0; j<ts; j++) {
    if (i != j) {
      int leftBound = x[i]->getMinElem();
      int rightBound = x[j]->getMaxElem()+dur[j];
      if (rightBound >= leftBound) {
	// test for failure; not sufficient space
	int sum = 0;
	for (int k = 0; k<ts; k++) {
	  int energy = EnergyFunct(leftBound, rightBound, dur[k], use[k], 
				   x[k]->getMinElem(),
				   x[k]->getMaxElem() + dur[k]);
	  sum = sum + energy;
	}
	if (sum > capacity * (rightBound - leftBound))
	  goto failure;
	
	}
    }
  }
}
*/
cploop:

  //////////  
  // UPPER PHASE
  //////////  

  //////////  
  // sort by descending release date; ie. min(s1) > min(s2) > min(s3) etc.
  //////////  
					
  if (reg_flag == 1) {
    myqsort(forCompSet0Up, 0, ts-1,compareDescRel );
  }

  for (int upTask=0; upTask < ts; upTask++) {

    kUp = MinMax[upTask].max + dur[upTask];
    kDown = MinMax[upTask].min;
    int use0 = 0;
    set0Size = 0;
    compSet0Size = 0;
    outSideSize = 0;
    int overlap = 0;

    // compute set S0 
    int l;
    for (l=0; l<ts; l++) {
      int dl = dur[l];
      int xlMin = MinMax[l].min;
      int xlMaxDL = MinMax[l].max + dl;
      if (( kDown <= xlMin) && ( xlMaxDL <= kUp)) {
	use0 = use0 + dl*use[l];
	set0[set0Size++] = l;
      }
      else {
	int overlapTmp = intMin(intMax(0,xlMin+dl-kDown),
				intMin(intMax(0,kUp-xlMaxDL+dl),
				       intMin(dl,kUp-kDown)));
	overlap += overlapTmp*use[l];
	if (reg_flag == 1) {
	  if (xlMaxDL > kUp) {
	    outSide[outSideSize++] = l;
	  }
	}
      }
    }

    if (reg_flag == 1) {
      for (l=0; l<ts; l++) {
	int realL = forCompSet0Up[l];
	if ( (MinMax[realL].min < kDown) && 
	     (MinMax[realL].max + dur[realL] <= kUp) )
	  compSet0[compSet0Size++] = realL;
      }
    }



    if ((kUp-kDown)*capacity < use0+overlap) {
      goto failure;
    }

    if (reg_flag == 0) goto endUp;

    {
    struct Set2 *oset = &Sets[0];	
    oset->dSi = use0;
    oset->sUp = kUp;
    oset->sLow = kDown;
    oset->extSize = set0Size;
    {
      for (int k = 0; k < ts; k++) {
	oset->ext[k] = set0[k];
      }
    }

    int setSize = 0;

    //////////  
    // compute the sets Si
    //////////  
    for (l=0; l<compSet0Size; l++) {
      int realL = compSet0[l];
      if (MinMax[realL].max+dur[realL] <= kUp) {
	int setSizeBefore = setSize;	
	struct Set2 *bset = &Sets[setSizeBefore];	
	setSize++;
	int dSi = bset->dSi + dur[realL]*use[realL];
	int minL = MinMax[realL].min;
	if ( (kUp - minL)*capacity < dSi) {
	  goto failure;
	}
	else {
	  struct Set2 *cset = &Sets[setSize];	
	  cset->dSi = dSi;
	  cset->sUp = kUp;
	  cset->sLow = minL;
	  cset->extSize = bset->extSize+1;
	}
      }
    }

    //////////  
    // Do the edge-finding
    //////////  
    int lCount = 0;
    int setCount = setSize;
    while ( (setCount >= 0) && (lCount < outSideSize) ) {
      int l = outSide[lCount];
      struct Set2 *s = &Sets[setCount];
      int minL = MinMax[l].min;
      int maxL = MinMax[l].max;
      int durL = dur[l];
      int useL = use[l];
      int sizeAll = s->dSi + durL*useL;
      if (maxL+durL > s->sUp) {
	if ( (s->sUp - s->sLow)*capacity >= sizeAll) {
	  // case 4: l may be inside
          if ( (s->sUp - minL)*capacity >= sizeAll) {
	    // case 5: L may be first
	    lCount++; 
	    setCount--;
	  }
	  else {
	    // it cannot be first
	    setCount--;
	  }
	}
	else {
	  if ( (s->sUp - minL)*capacity >= sizeAll) {
	    // case 5: L may be first
	    lCount++;
	  }
	  else {
	    int rest = s->dSi - (s->sUp - s->sLow)*(capacity - useL);
	    if (rest > 0) {
	      // l must be last
	      int val = s->sLow + (int) ceil((double) rest / (double) useL);
	      if (minL < val) {
		upFlag = 1;
		FailOnEmpty(*x[l] >= val);
	      }
	      lCount++;
	    }
	    else lCount++;
	  }
	}
      }
      else lCount++;
    }
    }
  endUp:
    ;
  }

  if (reg_flag == 1) {
    for (i=0; i<ts; i++) {
      MinMax[i].min = x[i]->getMinElem();	
      MinMax[i].max = x[i]->getMaxElem();	
    }
  }


  //////////
  // DOWN PHASE
  //////////

  

  //////////
  // sort by ascending due date; ie. max(s1)+dur(s1) < max(s2)+dur(s2)
  //////////

  if (reg_flag == 1) {		   
    myqsort(forCompSet0Down, 0, ts-1, compareAscDue);
  }


  for (int downTask=0; downTask < ts; downTask++) {
    
    kUp = MinMax[downTask].max + dur[downTask];
    kDown = MinMax[downTask].min;
    int use0 = 0;
    set0Size = 0;
    compSet0Size = 0;
    outSideSize = 0;
    int overlap=0;

    //////////
    // compute set S0 
    //////////
    int l;
    for (l=0; l<ts; l++) {
      int dl = dur[l];
      int xlMin = MinMax[l].min;
      int xlMaxDL = MinMax[l].max + dl;
      if (( kDown <= xlMin) && ( xlMaxDL <= kUp)) {
	use0 = use0 + dl*use[l];
	set0[set0Size++] = l;
      }
      else {
	int overlapTmp = intMin(intMax(0,xlMin+dl-kDown),
				intMin(intMax(0,kUp-xlMaxDL+dl),
				       intMin(dl,kUp-kDown)));
	overlap += overlapTmp*use[l];
	if (reg_flag == 1) {
	  if (xlMin < kDown) {
	    outSide[outSideSize++] = l;
	  }
	}
      }
    }

    if (reg_flag == 1) {
      for (l=0; l<ts; l++) {
	int realL = forCompSet0Down[l];
	if ( (MinMax[realL].min >= kDown) && 
	     (MinMax[realL].max + dur[realL] > kUp) )
	  compSet0[compSet0Size++] = realL;
      }
    }

    
    if ( (kUp-kDown)*capacity < use0+overlap) {
      goto failure;
    }

    if (reg_flag == 0) goto endDown;

    {
    struct Set2 *oset = &Sets[0];
    oset->dSi = use0;
    oset->sUp = kUp;
    oset->sLow = kDown;
    oset->extSize = set0Size;
    {
      for (int k = 0; k < ts; k++) {
	oset->ext[k] = set0[k];
      }
    }

    int setSize = 0;

    //////////
    // compute the sets Si
    //////////
    {
      int l;
      for (l=0; l<compSet0Size; l++) {
	int realL = compSet0[l];
	if (MinMax[realL].min >= kDown) {
	  int setSizeBefore = setSize;	
	  struct Set2 *bset = &Sets[setSizeBefore];	
	  int durL = dur[realL];
	  setSize++;
	  int dSi = bset->dSi + durL*use[realL];
	  int maxL = MinMax[realL].max+durL;
	  if ( (maxL - kDown)*capacity < dSi)
	    {
	      goto failure;
	    }
	  else {
	    struct Set2 *cset = &Sets[setSize];	
	    cset->dSi = dSi;
	    cset->sUp = maxL;
	    cset->sLow = kDown;
	    cset->extSize = bset->extSize+1;
	  }
	}
      }
    }

    //////////
    // Do the edge-finding
    //////////
    int lCount = 0;
    int setCount = setSize;
    while ( (setCount >= 0) && (lCount < outSideSize) ) {
      int l = outSide[lCount];
      struct Set2 *s = &Sets[setCount];
      int minL = MinMax[l].min;
      int maxL = MinMax[l].max;
      int durL = dur[l];
      int useL = use[l];
      int sizeAll = s->dSi + durL*useL;
      
      if (minL < s->sLow) {
	if ( (s->sUp - s->sLow)*capacity >= sizeAll) {
	  // case 4: l may be inside
	    if ( (maxL + durL - s->sLow)*capacity >= sizeAll) {
	    // case 5: L may be last
	    lCount++; 
	    setCount--;
	  }
	  else {
	    // it cannot be last
	    setCount--;
	  }
	}
	else {
	  if ((maxL + durL - s->sLow) * capacity >= sizeAll) {
	    // case 5: L may be last
	    lCount++;
	  }
	  else {
	    int rest = s->dSi - (s->sUp - s->sLow)*(capacity - useL);
	    if (rest > 0) {
	      int val = s->sUp - (int) ceil( (double) rest / (double) useL);
	      // l must be first
              if (maxL+durL > val) {
		downFlag = 1;
		int right = val - durL;
		if (right < 0)
		  goto failure;
		FailOnEmpty(*x[l] <= right);
	      }
	      lCount++;
	    }
	    else lCount++;
	  }
	}
      }
      else lCount++;
    }
    }
  endDown:
    ;
  }

  if (reg_flag == 1) {
    for (i=0; i<ts; i++) {
      MinMax[i].min = x[i]->getMinElem();	
      MinMax[i].max = x[i]->getMaxElem();	
    }
    
    if ((upFlag == 1)||(downFlag==1)) {
      upFlag = 0;
      downFlag = 0;
    goto cploop;
    }
  }
  else goto capLoop;

reifiedloop:

   for (i=0; i<ts; i++)
     for (j=i+1; j<ts; j++) {
       if (use[i] + use[j] > capacity) {
	 int xui = MinMax[i].max, di = dur[i], xlj = MinMax[j].min;
	 if (xui + di <= xlj) continue;
	 int xuj = MinMax[j].max, dj = dur[j], xli = MinMax[i].min;
	 if (xuj + dj <= xli) continue;
	 if (xli + di > xuj) {
	   if (xuj > xui - dj) {
	     disjFlag = 1;
	     FailOnEmpty(*x[j] <= xui - dj);
	     MinMax[j].max = x[j]->getMaxElem();
	   }
	   if (xli < xlj + dj) {
	     disjFlag = 1;
	     FailOnEmpty(*x[i] >= xlj + dj);
	     MinMax[i].min = x[i]->getMinElem();
	   }
	 }
	 if (xlj + dj > xui) {
	   if (xui > xuj - di) {
	     disjFlag = 1;
	     FailOnEmpty(*x[i] <= xuj - di);
	     MinMax[i].max = x[i]->getMaxElem();
	   }
	   if (xlj < xli + di) {
	     disjFlag = 1;
	     FailOnEmpty(*x[j] >= xli + di);
	     MinMax[j].min = x[j]->getMinElem();
	   }
	 }
       }
     }

  if (disjFlag == 1) {
    disjFlag = 0;
    goto reifiedloop;
  }


capLoop:

  //////////
  // do the capacity checking
  //////////
  {
    int interval_nb = 0;	  
    
    DECL_DYN_ARRAY(Interval, Intervals, ts);

    //////////
    // compute intervals, where the task occupies place in any case
    //////////
    for (i=ts; i--;){
      // latest start time < earliest completion?
      int lst = MinMax[i].max;
      int ect = MinMax[i].min + dur[i];
      if (lst < ect) {
	Intervals[interval_nb].left = lst;
	Intervals[interval_nb].right = ect;
	Intervals[interval_nb].use = use[i];
	interval_nb++;
      }
    }

    //////////
    // compute left and right bound for cumulative checking
    //////////
    int min_left = mysup;
    int max_right = 0;
    int sum = 0;
    for (i=0; i<ts; i++) {
      int iMin = MinMax[i].min;
      int iDue = MinMax[i].max + dur[i];
      sum = sum + use[i] * dur[i];
      if (iMin < min_left) min_left = iMin;
      if (iDue > max_right) max_right = iDue;
    }
    // test whether the capacity is sufficient for all tasks
    if (sum > capacity * (max_right - min_left))
      goto failure;

    Intervals[interval_nb].left = min_left;
    Intervals[interval_nb].right = max_right;
    Intervals[interval_nb].use = 0;
    interval_nb++;

    

    //////////
    // sort the intervals lexicographically
    //////////
    Interval * intervals = Intervals;
    myqsort(intervals, 0, interval_nb-1, CompareIntervals);

    //////////
    // compute the set of all bounds of intervals
    //////////
    int double_nb = interval_nb*2;
    DECL_DYN_ARRAY(int, IntervalBounds, double_nb);

    for (i=0; i<interval_nb; i++) {
      IntervalBounds[2*i] = Intervals[i].left;
      IntervalBounds[2*i+1] = Intervals[i].right;
    }
    
    //////////
    // sort the bounds in ascending order
    //////////
    int * intervalBounds = IntervalBounds;
    myqsort(intervalBounds, 0, double_nb-1, CompareBounds);

    //////////
    // compute the set of intervals, for which there is exclusion
    //////////
    DECL_DYN_ARRAY(Interval, ExclusionIntervals, double_nb);
    int exclusion_nb = 0;
    int low_counter  = 0;
    int left_pt      = 0;
    int right_pt     = 0;


    while (left_pt < double_nb) {
      int left_val = IntervalBounds[left_pt];
      while ((right_pt < double_nb) && (IntervalBounds[right_pt] == left_val))
	right_pt++;
      if (right_pt == double_nb)  break;
      int right_val = IntervalBounds[right_pt];
      int cum = 0;

      for (i=low_counter; i<interval_nb; i++) {

	int leftInt = Intervals[i].left;
	int rightInt = Intervals[i].right;
	if (leftInt > right_val) break;
	if ( (left_val >= leftInt) && (right_val <= rightInt) ) {
	  cum = cum + (right_val - left_val) * Intervals[i].use;
	}
      }

      /* 
      // counting the overlap is not worth the effort for cumulative reasoning
      int cumOver = 0;
      //this destroys the cumulative reasoning
      for (i = 0; i < ts; i++) {
	if (MinMax[i].max >= MinMax[i].min + dur[i]) {
	  int overlap = intMin(intMax(0,MinMax[i].min+dur[i]-left_val),
			       intMin(intMax(0,right_val-MinMax[i].max),
				      intMin(dur[i],right_val-left_val)));
	  cumOver = cumOver + overlap * use[i];
	}
      }
      */



      if (cum > (right_val - left_val) * capacity) {
	goto failure;
      }
      /* it's not worth the effort
      if (cum+cumOver > (right_val - left_val) * capacity) {
	goto failure;
      }
      */

      if (cum > 0) {
	ExclusionIntervals[exclusion_nb].left = left_val;
	ExclusionIntervals[exclusion_nb].right = right_val;
	ExclusionIntervals[exclusion_nb].use = cum;
	exclusion_nb++;
      }
      left_pt = right_pt;
    }

    int last = 0;
    /*
    int last_left = 0;
    int last_right = 0;
    */

    //////////
    // exclude from the tasks the intervals, which indicate that 
    // the task connot be scheduled here because of no sufficient place.
    //////////
    // do not use reg_flag anymore, it is not worth it.
    // the commented region does contain code which avoids the 
    // production of holes in domains
#ifdef HOLES
    // perhaps some tests before generalizing domains could improve
    for (i=0; i<ts; i++) {
      int lst = MinMax[i].max;
      int ect = MinMax[i].min + dur[i];
      int use_i = use[i];
      int dur_i = dur[i];
      for (j=0; j<exclusion_nb; j++) {
	Interval Exclusion = ExclusionIntervals[j];
	int span = Exclusion.right - Exclusion.left;
	if (Exclusion.use + span * use_i > span * capacity) {
	  int left = Exclusion.left;
	  int right = Exclusion.right;
	  if (lst < ect) {
	    if ( (lst <= left) && (right <= ect) ) continue;
	    else {
	      if (Exclusion.use + span * use_i > span * capacity) {
		OZ_FiniteDomain la;
		// The following holds because of construction of 
		// the exclusion intervals! They describe fulled columns!
		la.initRange(left-dur_i+1,right-1);
		FailOnEmpty(*x[i] -= la);
		
		// new
		// for capacity > 1 we must count the used resource. 
		// But this is too expensive. really???
		if ((left - last < dur_i) && (capacity == 1)) {
		  OZ_FiniteDomain la;
		  la.initRange(last,right-1);
		  FailOnEmpty(*x[i] -= la);
		}
		last = right;
		/*
		if ((left - last_right < dur_i)) {
		  OZ_FiniteDomain la;
		  la.initRange(last_left-dur_i+1,right-1);
		  FailOnEmpty(*x[i] -= la);
		}
		else 
		  last_left = left;
		last_right = right;
		*/
	      }
	    }
	  }
	  else {
	    if (Exclusion.use + span * use_i > span * capacity) {
	      OZ_FiniteDomain la;
	      la.initRange(left-dur_i+1,right-1);
	      FailOnEmpty(*x[i] -= la);
	      // new
	      // for capacity > 1 we must count the used resource. 
	      // But this is too expensive.
	      if ((left - last < dur_i) && (capacity == 1)) {
		OZ_FiniteDomain la;
		la.initRange(last,right-1);
		FailOnEmpty(*x[i] -= la);
	      }
	      last = right;
	      /*
		if ((left - last_right < dur_i)) {
		  OZ_FiniteDomain la;
		  la.initRange(last_left-dur_i+1,right-1);
		  FailOnEmpty(*x[i] -= la);
		  }
		  else 
		  last_left = left;
		  last = right;
		  */
	      }
	  }
	}
      }
    }
#endif

    /* is not useful */
    /*
    for (i=0; i<ts; i++) 
      for (int k=0; k<ts; k++) { 
      int left = MinMax[i].min;
      int right = MinMax[k].max+dur[k];
      int span = right-left;
      if (left <= right) {

	int cumL = 0;
	for (j = 0; j < ts; j++) {
	  int durJ = dur[j];
	  int lb = MinMax[j].min;
	  int lst = MinMax[j].max;
	  int ect = lb + durJ;
	  int rb = lst + durJ;
	  int add = intMax(0, intMin(durJ, 
			       intMin(right - left,
				   intMin(ect - left, right - lst))));
	  cumL = cumL + add;
	}
	if  (cumL > (right - left) * capacity) {
	  goto failure;
	}

	for (j = 0; j < ts; j++) {
	  int durJ = dur[j];
	  int lb   = MinMax[j].min;
	  int lst  = MinMax[j].max;
	  int ect  = lb + durJ;
	  int rb   = lst + durJ;
	  int useJ = use[j];
	  int add = intMax(0, intMin(durJ, 
			       intMin(right - left,
				   intMin(ect - left, right - lst))));
	  if (add == 0) {
	    int mdur = intMin(durJ, right-left);
	    if (cumL + useJ * mdur > (right - left) * capacity) {
	      int delta = (int) floor( (double) ((right - left) * capacity - cumL) / (double) useJ);
	      OZ_FiniteDomain la;
	      int lv = left-(durJ-delta)+1;
	      int rv = right-delta-1;
	      if ( lv <= rv) {
		la.initRange(lv, rv);
		FailOnEmpty(*x[j] -= la);
	      }
	    }
	  }
	}
      }
    }
    */


#ifdef BOUNDS_NEW
    /* for bounds reasoning more efficiently */
    for (i=0; i<ts; i++) { 
      int durI = dur[i];
      int useI = use[i];
      int est = MinMax[i].min;
      int ect = est + durI;
      int lst = MinMax[i].max;
      int lct = lst + durI;
      // from left to right
      for (j=0; j<exclusion_nb; j++) {
	Interval Exclusion = ExclusionIntervals[j];
	int low = Exclusion.left;
	int up  = Exclusion.right;
	if (low > est) 
	  break;
	else {
	  if ( (est >= low) && (est < up) ) {
	    int span = up - low;
	    if ( ((lst < ect) && ((lst > low) || (up > ect))) ||
		 (lst >= ect) )
	      if (Exclusion.use + span * useI > span * capacity) {	    
		FailOnEmpty(*x[i] >= up);
	      }
	  }
	}
      }
      // from right to left
      for (j=exclusion_nb-1; j>=0; j--) {
	Interval Exclusion = ExclusionIntervals[j];
	int low = Exclusion.left;
	int up  = Exclusion.right;
	if (up < lct)
	  break;
	else {
	  if ( (lct > low) && (lct <= up) ) {
	    int span = up - low;
	    if ( ((lst < ect) && ((lst > low) || (up > ect))) ||
		 (lst >= ect) )
	      if (Exclusion.use + span * useI > span * capacity) {	    
		FailOnEmpty(*x[i] <= low - durI);
	      }
	  }
	}
      }
    }
#endif

    //////////
    // update the min/max values
    //////////
    for (i=0; i<ts; i++) {
      int mini = x[i]->getMinElem();
      int maxi = x[i]->getMaxElem();
      if (mini > MinMax[i].min) {
	cap_flag = 1;
	MinMax[i].min = mini;
      }
      if (maxi < MinMax[i].max) {
	cap_flag = 1;
	MinMax[i].max = maxi;
      }
    }

    if (cap_flag == 1) {
      cap_flag = 0;
      goto capLoop;
    }

      
  }


finish:  

  return P.leave();


failure:

  return P.fail();
}



//-----------------------------------------------------------------------------

CPIteratePropagatorCapUp::CPIteratePropagatorCapUp(OZ_Term tasks, 
						   OZ_Term starts, 
						   OZ_Term durs, 
						   OZ_Term use,
						   OZ_Term cap)
  : Propagator_VD_VI_VI_I(OZ_vectorSize(tasks))
{

  reg_capacity = OZ_intToC(cap);

  VectorIterator vi(tasks);
  int i = 0;
		     
  DECL_DYN_ARRAY(StartDurUseTerms, sdu, reg_sz);

  while (vi.anyLeft()) {
    OZ_Term task = vi.getNext();

    sdu[i].start = OZ_subtree(starts, task);
    sdu[i].dur = OZ_intToC(OZ_subtree(durs, task));
    sdu[i].use = OZ_intToC(OZ_subtree(use, task));
    i += 1;
  } // while

  OZ_ASSERT(i == reg_sz);

  myqsort(sdu, 0, reg_sz-1, compareDursUse);

  for (i = reg_sz; i--; ) {
    reg_l[i]      = sdu[i].start;
    reg_offset[i] = sdu[i].dur;
    reg_use[i]    = sdu[i].use;
  }
}

OZ_C_proc_begin(sched_cpIterateCapUp, 5)
{
  OZ_EXPECTED_TYPE(OZ_EM_VECT OZ_EM_VECT OZ_EM_LIT "," OZ_EM_RECORD OZ_EM_FD 
		   "," OZ_EM_RECORD OZ_EM_INT "," OZ_EM_RECORD OZ_EM_INT 
		   "," OZ_EM_VECT OZ_EM_INT);
  
  {
    PropagatorExpect pe;
    
    OZ_EXPECT(pe, 0, expectVectorVectorLiteral);
    OZ_EXPECT(pe, 1, expectProperRecordIntVarMinMax);
    OZ_EXPECT(pe, 2, expectProperRecordInt);
    OZ_EXPECT(pe, 3, expectProperRecordInt);
    OZ_EXPECT(pe, 4, expectVectorInt);
    SAMELENGTH_VECTORS(1, 2);
    SAMELENGTH_VECTORS(1, 3);
  }

  OZ_Term starts = OZ_args[1], durs = OZ_args[2], use = OZ_args[3], 
    caps = OZ_args[4];


  VectorIterator vi(OZ_args[0]);
  VectorIterator viCap(caps);

  for (int i = 0; i < OZ_vectorSize(OZ_args[0]); i++) {
    OZ_Term tasks    = vi.getNext();
    OZ_Term capacity = viCap.getNext();

    PropagatorExpect pe;

    VectorIterator vi_tasks(tasks);
    while (vi_tasks.anyLeft()) {
      OZ_Term task = vi_tasks.getNext();
      OZ_Term start_task = OZ_subtree(starts, task);
      OZ_Term dur_task = OZ_subtree(durs, task);
      OZ_Term use_task = OZ_subtree(use, task);
      if (!start_task || !dur_task || !use_task) 
	return OZ_typeError(expectedType, 0, "Scheduling applications expect that all task symbols are features of the records denoting the start times, the durations and the resource usages.");
      pe.expectIntVarMinMax(OZ_subtree(starts, task));
    }

    OZ_Return r = pe.impose(new CPIteratePropagatorCapUp(tasks, starts, durs, 
							 use, capacity),
			    OZ_getLowPrio());

    if (r == FAILED) return FAILED;
  }
  return OZ_ENTAILED;
}
OZ_C_proc_end

OZ_Return CPIteratePropagatorCapUp::propagate(void)
{
  int &ts      = reg_sz;
  int * dur    = reg_offset;
  int * use    = reg_use;
  int capacity = reg_capacity;

  // if we have no tasks the prop returns trivially true
  if (ts == 0) return PROCEED;

  DECL_DYN_ARRAY(OZ_FDIntVar, x, ts);

  PropagatorController_VV P(ts, x);
  
  int i, j;
  for (i = ts; i--; )
    x[i].read(reg_l[i]);

  int cap_flag = 0;
  int mysup = OZ_getFDSup();

  struct Min_max {
    int min, max;
  };
  DECL_DYN_ARRAY(Min_max, MinMax, ts);
  for (i=ts; i--;){
    MinMax[i].min = x[i]->getMinElem();
    MinMax[i].max = x[i]->getMaxElem();
  }


capLoop:

  // do the capacity checking
  { 
    int interval_nb = 0;	  
    
    DECL_DYN_ARRAY(Interval, Intervals, ts+1);

    // compute intervals, where the task may occupy place 
    for (i=ts; i--;){
      // latest start time < earliest completion?
      Intervals[interval_nb].left = MinMax[i].min;
      Intervals[interval_nb].right = MinMax[i].max + dur[i];
      Intervals[interval_nb].use = use[i];
      interval_nb++;
    }

    // compute left and right bound for cumulative checking
    int min_left = mysup;
    int max_right = 0;
    for (i=0; i<ts; i++) {
      if (MinMax[i].min < min_left) min_left = MinMax[i].min;
      if (MinMax[i].max+dur[i] > max_right) max_right = MinMax[i].max+dur[i];
    }
    Intervals[interval_nb].left = min_left;
    Intervals[interval_nb].right = max_right;
    Intervals[interval_nb].use = 0;
    interval_nb++;
    
    

    Interval * intervals = Intervals;
    myqsort(intervals, 0, interval_nb-1, CompareIntervals);

    // compute the set of all bounds of intervals
    int double_nb = interval_nb*2;
    DECL_DYN_ARRAY(int, IntervalBounds, double_nb);

    for (i=0; i<interval_nb; i++) {
      IntervalBounds[2*i] = Intervals[i].left;
      IntervalBounds[2*i+1] = Intervals[i].right;
    }
    
    int * intervalBounds = IntervalBounds;
    myqsort(intervalBounds, 0, double_nb-1, CompareBounds);

    // compute the set of intervals, for which there is inclusion
    DECL_DYN_ARRAY(Interval, InclusionIntervals, double_nb);
    int inclusion_nb = 0;
    int low_counter  = 0;
    int left_pt      = 0;
    int right_pt     = 0;


    while (left_pt < double_nb) {
      int left_val = IntervalBounds[left_pt];
      while ((right_pt < double_nb) && (IntervalBounds[right_pt] == left_val))
	right_pt++;
      if (right_pt == double_nb)  break;
      int right_val = IntervalBounds[right_pt];
      // cum is the amount of possible usage
      int cum = 0;
      for (i=low_counter; i<interval_nb; i++) {
	int leftInt = Intervals[i].left;
	int rightInt = Intervals[i].right;
	// really for this???
	if (leftInt > right_val) break;
	if ( (leftInt <= left_val) && (right_val <= rightInt) ) {
	  cum = cum + (right_val - left_val) * Intervals[i].use;
	}
      }
      if (cum < (right_val - left_val) * capacity) {
	goto failure;
      }
      InclusionIntervals[inclusion_nb].left = left_val; 
      InclusionIntervals[inclusion_nb].right = right_val;
      InclusionIntervals[inclusion_nb].use = cum;
      inclusion_nb++;
      left_pt = right_pt;
    }


    // include places, where tasks must be scheduled.
    for (i=0; i<ts; i++) {
      int mini  = MinMax[i].min;
      int maxi  = MinMax[i].max + dur[i];
      int use_i = use[i];
      int dur_i = dur[i];
      for (j=0; j<inclusion_nb; j++) {
	Interval Inclusion = InclusionIntervals[j];
	int span = Inclusion.right - Inclusion.left;
	if (Inclusion.use - span * use_i < span * capacity) {
	  int left = Inclusion.left;
	  int right = Inclusion.right;
	  if ( (mini <= left) && (right <= maxi) ) {
	    if (Inclusion.use - span * use_i < span * capacity) {
	      if (dur_i < right - left) goto failure;
	      OZ_FiniteDomain la;
	      la.initRange(right-dur_i,left);
	      FailOnEmpty(*x[i] &= la);
	      int mini_new = x[i]->getMinElem();
	      int maxi_new = x[i]->getMaxElem();
	      if (mini_new > MinMax[i].min) {
		cap_flag = 1;
		MinMax[i].min = mini_new;
	      }
	      if (maxi_new < MinMax[i].max) {
		cap_flag = 1;
		MinMax[i].max = maxi_new;
	      }
	    }
	  }
	  else {
	    if ( (mini < left) && (left < maxi) && (maxi < right) )
	      printf("Fatal1: %d %d %d %d\n",mini,left,maxi,right);
	    else if ( (left < mini) && (mini < right) && (right < maxi) )
	      printf("Fatal2: %d %d %d %d\n",mini,left,maxi,right);
	    else if ( (left < mini) && (maxi < right) )
	      printf("Fatal3: %d %d %d %d\n",mini,left,maxi,right);
	    else;
	    // note that this can happen because of smaller intervals.
	    // But because we are looping, everything is fine.
	  }
	}
      }
    }




    if (cap_flag == 1) {
      cap_flag = 0;
      goto capLoop;
    }
      
    
  }
  
return P.leave();

failure:
  return P.fail();
}


 
  
//-----------------------------------------------------------------------------
// static member

OZ_CFun SchedCardPropagator::spawner = sched_disjoint_card;
OZ_CFun CPIteratePropagator::spawner = sched_cpIterate;
OZ_CFun CPIteratePropagatorCap::spawner = sched_cpIterateCap;
OZ_CFun CPIteratePropagatorCapUp::spawner = sched_cpIterateCapUp;
OZ_CFun DisjunctivePropagator::spawner = sched_disjunctive;

