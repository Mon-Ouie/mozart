/*
 *  Authors:
 *    Christian Schulte <schulte@ps.uni-sb.de>
 * 
 *  Copyright:
 *    Christian Schulte, 1998
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

#include "builtins.hh"
#include "var_base.hh"
#include "var_fd.hh"
#include "var_bool.hh"
#include "distributor.hh"
#include "thr_int.hh"
#include "fdomn.hh"

// ---------------------------------------------------------------------
//                  Finite Domains Distribution Built-ins
// ---------------------------------------------------------------------

inline
int getSize(TaggedRef var) {
  return isGenFDVar(var) ? tagged2GenFDVar(var)->getDom().getSize() : 2;
}

inline 
int getMin(TaggedRef var) {
  return isGenFDVar(var) ? tagged2GenFDVar(var)->getDom().getMinElem() : 0;
}

inline
int getMax(TaggedRef var) {
  return isGenFDVar(var) ? tagged2GenFDVar(var)->getDom().getMaxElem() : 1;
}

inline
int getMid(TaggedRef var) {
  if (isGenFDVar(var)) {
    OZ_FiniteDomain &dom = tagged2GenFDVar(var)->getDom();
    return dom.getMidElem();
  } else {
    return 0;
  }
}

inline
int getConstraints(TaggedRef var) {
  return oz_var_getSuspListLength(tagged2CVar(var));
}


OZ_C_proc_proto(BIfdTellConstraint);

TaggedRef BI_tell = makeTaggedConst(new  Builtin("FD.distributeTELL", 2, 0, 
						 BIfdTellConstraint, OK));


inline
void tell_dom(Board * bb, const TaggedRef a, const TaggedRef b) {
  RefsArray args = allocateRefsArray(2, NO);
  args[0] = b;
  args[1] = a;

  Thread * t = oz_newThreadInject(bb);
  t->pushCall(BI_tell,args,2);
}

class FdDistributor : public Distributor {
protected:
  TaggedRef sync;
  int sel_var;
  TaggedRef sel_val;
  TaggedRef * vars;
  int size;
public:

  FdDistributor(Board *bb, TaggedRef * vs, int n) {
    vars = vs;
    size = n;
    sync = oz_newVar(bb);
  }

  TaggedRef getSync() {
    return sync;
  }

  void normalize(void);

  void selectVarNaive(void);
  void selectVarSize(void);
  void selectVarMin(void);
  void selectVarMax(void);
  void selectVarNbSusps(void);

  void selectValMin(void);
  void selectValMid(void);
  void selectValMax(void);
  void selectValSplitMin(void);
  void selectValSplitMax(void);

  virtual int commit(Board * bb, int l, int r) {
    if (size > 0) {
      if (l==r) {
	TaggedRef dom;
	if (l == 1) {
	  dom = sel_val;
	} else {
	  SRecord * st = SRecord::newSRecord(AtomCompl, 1);
	  st->setArg(0, sel_val);
	  dom = makeTaggedSRecord(st);
	}
	tell_dom(bb,vars[sel_var],dom);
	return 1;
      } else {
	return 2;
      }
    } else {
      tell_dom(bb,sync,makeTaggedSmallInt(0));
      return 0;
    }
  }

  virtual void dispose(void) {
    freeListDispose(this, sizeof(FdDistributor));
  }

  virtual Distributor * gc(void) {
    FdDistributor * t = (FdDistributor *) 
      oz_hrealloc(this, sizeof(FdDistributor));
    OZ_collectHeapTerm(t->sync,t->sync);
    OZ_collectHeapTerm(t->sel_val,t->sel_val);
    t->vars = OZ_copyOzTerms(size, t->vars);
    return t;
  }

};


void FdDistributor::normalize(void) {
  // Discard all elements which are already bound
  int j = size;

  for (int i = size; i--;) {
    if (!oz_isSmallInt(oz_deref(vars[i])))
      vars[--j]=vars[i];
  }

  if (j > 0) {
    freeListDispose(vars, j * sizeof(TaggedRef));
    vars += j;
    size -= j; 
  }

}

/*
 * Variable selection strategies
 *
 */

void FdDistributor::selectVarNaive(void) {
  Assert(size > 0);
  sel_var = size-1;
}

void FdDistributor::selectVarSize(void) {
  Assert(size > 0);
  int minsize = getSize(oz_deref(vars[size-1]));
  sel_var = size-1;
  for (int i = size-1; i--; ) {
    int cursize = getSize(oz_deref(vars[i]));
    if (cursize < minsize) {
      minsize = cursize; sel_var = i; 
    }
  }
}

void FdDistributor::selectVarMin(void) {
  Assert(size > 0);
  int minmin = getMin(oz_deref(vars[size-1]));
  sel_var = size-1;
  for (int i = size-1; i--; ) {
    int curmin = getMin(oz_deref(vars[i]));
    if (curmin < minmin) {
      minmin = curmin; sel_var = i; 
    }
  }
}

void FdDistributor::selectVarMax(void) {
  Assert(size > 0);
  int maxmax = getMax(oz_deref(vars[size-1]));
  sel_var = size-1;
  for (int i = size-1; i--; ) {
    int curmax = getMax(oz_deref(vars[i]));
    if (curmax > maxmax) {
      maxmax = curmax; sel_var = i; 
    }
  }
}

void FdDistributor::selectVarNbSusps(void) {
  Assert(size > 0);
  TaggedRef d_var = oz_deref(vars[size-1]);
  int minsize = getSize(d_var);
  int maxnb   = getConstraints(d_var);
  sel_var = size-1;
  for (int i = size-1; i--; ) {
    d_var = oz_deref(vars[i]);
    int curnb = getConstraints(d_var);

    if (curnb < maxnb)
      continue;

    int cursize = getSize(d_var);
    
    if (curnb > maxnb || cursize < minsize) {
      maxnb   = curnb;
      minsize = cursize; 
      sel_var = i; 
    }
  }
}



/*
 * Value selection strategies
 *
 */

void FdDistributor::selectValMin(void) {
  sel_val = makeTaggedSmallInt(getMin(oz_deref(vars[sel_var])));
}

void FdDistributor::selectValMid(void) {
  sel_val = makeTaggedSmallInt(getMid(oz_deref(vars[sel_var])));
}

void FdDistributor::selectValMax(void) {
  sel_val = makeTaggedSmallInt(getMax(oz_deref(vars[sel_var])));
}

void FdDistributor::selectValSplitMin(void) {
  SRecord * st = SRecord::newSRecord(AtomPair, 2);
  st->setArg(0, makeTaggedSmallInt(0));
  st->setArg(1, makeTaggedSmallInt(getMid(oz_deref(vars[sel_var]))));
  sel_val = makeTaggedSRecord(st);
}

void FdDistributor::selectValSplitMax(void) {
  SRecord * st = SRecord::newSRecord(AtomPair, 2);
  st->setArg(0, makeTaggedSmallInt(getMid(oz_deref(vars[sel_var])) + 1));
  st->setArg(1, makeTaggedSmallInt(fd_sup));
  sel_val = makeTaggedSRecord(st);
}



/*
 * Create class for different combinations
 */

#define DefFdDistClass(CLASS,VARSEL,VALSEL) \
class CLASS : public FdDistributor {              \
  public:                                         \
  CLASS(Board * b, TaggedRef * v, int s) :        \
    FdDistributor(b,v,s) {}                       \
  virtual int getAlternatives(void) {             \
    normalize();                                  \
    if (size > 0) { VARSEL(); VALSEL(); return 2; \
    } else {                            return 1; \
    }                                             \
  };                                              \
}

DefFdDistClass(FdDist_Naive_Min,selectVarNaive,selectValMin);
DefFdDistClass(FdDist_Naive_Mid,selectVarNaive,selectValMid);
DefFdDistClass(FdDist_Naive_Max,selectVarNaive,selectValMax);
DefFdDistClass(FdDist_Naive_SplitMin,selectVarNaive,selectValSplitMin);
DefFdDistClass(FdDist_Naive_SplitMax,selectVarNaive,selectValSplitMax);

DefFdDistClass(FdDist_Size_Min,selectVarSize,selectValMin);
DefFdDistClass(FdDist_Size_Mid,selectVarSize,selectValMid);
DefFdDistClass(FdDist_Size_Max,selectVarSize,selectValMax);
DefFdDistClass(FdDist_Size_SplitMin,selectVarSize,selectValSplitMin);
DefFdDistClass(FdDist_Size_SplitMax,selectVarSize,selectValSplitMax);

DefFdDistClass(FdDist_Min_Min,selectVarMin,selectValMin);
DefFdDistClass(FdDist_Min_Mid,selectVarMin,selectValMid);
DefFdDistClass(FdDist_Min_Max,selectVarMin,selectValMax);
DefFdDistClass(FdDist_Min_SplitMin,selectVarMin,selectValSplitMin);
DefFdDistClass(FdDist_Min_SplitMax,selectVarMin,selectValSplitMax);

DefFdDistClass(FdDist_Max_Min,selectVarMax,selectValMin);
DefFdDistClass(FdDist_Max_Mid,selectVarMax,selectValMid);
DefFdDistClass(FdDist_Max_Max,selectVarMax,selectValMax);
DefFdDistClass(FdDist_Max_SplitMin,selectVarMax,selectValSplitMin);
DefFdDistClass(FdDist_Max_SplitMax,selectVarMax,selectValSplitMax);

DefFdDistClass(FdDist_NbSusps_Min,selectVarNbSusps,selectValMin);
DefFdDistClass(FdDist_NbSusps_Mid,selectVarNbSusps,selectValMid);
DefFdDistClass(FdDist_NbSusps_Max,selectVarNbSusps,selectValMax);
DefFdDistClass(FdDist_NbSusps_SplitMin,selectVarNbSusps,selectValSplitMin);
DefFdDistClass(FdDist_NbSusps_SplitMax,selectVarNbSusps,selectValSplitMax);



/*
 * Vector processing
 */

#define TestElement(v) \
  {                                         \
    DEREF(v, v_ptr, v_tag);                 \
    if (isGenFDVar(v) || isGenBoolVar(v)) { \
      n++;                                  \
    } else if (oz_isSmallInt(v)) {          \
      ;                                     \
    } else if (oz_isVariable(v)) {          \
      oz_suspendOnPtr(v_ptr);               \
    } else {                                \
      goto bomb;                            \
    }                                       \
  }
	
#define iVarNaive   0
#define iVarSize    1
#define iVarMin     2
#define iVarMax     3
#define iVarNbSusps 4

#define iValMin      0
#define iValMid      1
#define iValMax      2
#define iValSplitMin 3
#define iValSplitMax 4

#define PP(I,J) I*5+J

#define PPCL(I,J)                                  \
  case PP(iVar ## I,iVal ## J):                    \
    fdd = new FdDist_ ## I ## _ ## J(bb, vars, n); \
    break;

OZ_BI_define(fdd_distribute, 3, 1) {
  oz_declareIntIN(0,var_sel);
  oz_declareIntIN(1,val_sel);
  oz_declareNonvarIN(2,vv);

  int n = 0;
  TaggedRef * vars;
  
  if (oz_isLiteral(vv)) {
    ;
  } else if (oz_isCons(vv)) {
    
    TaggedRef vs = vv;

    while (oz_isCons(vs)) {
      TaggedRef v = oz_head(vs);
      TestElement(v);
      vs = oz_tail(vs);
      DEREF(vs, vs_ptr, vs_tag);
      if (isVariableTag(vs_tag))
	oz_suspendOnPtr(vs_ptr);
    }
    
    if (!oz_isNil(vs))
      goto bomb;
    
  } else if (oz_isSRecord(vv)) {
    
    for (int i = tagged2SRecord(vv)->getWidth(); i--; ) {
      TaggedRef v = tagged2SRecord(vv)->getArg(i);
      TestElement(v);
    }
    
  } else 
    goto bomb;
  
  if (n == 0)
    OZ_RETURN(NameUnit);

  // This is inverse order!
  vars = (TaggedRef *) freeListMalloc(sizeof(TaggedRef) * n);

  if (oz_isCons(vv)) {
    TaggedRef vs = vv;
    int i = n;
    while (oz_isCons(vs)) {
      TaggedRef v = oz_head(vs);
      if (!oz_isSmallInt(oz_deref(v)))
	vars[--i] = v;
      vs = oz_deref(oz_tail(vs));
    }
  } else {
    int j = 0;
    for (int i = tagged2SRecord(vv)->getWidth(); i--; ) {
      TaggedRef v = tagged2SRecord(vv)->getArg(i);
      if (!oz_isSmallInt(oz_deref(v)))
	vars[j++] = v;
    }
  }

  if (oz_onToplevel())
    OZ_RETURN(oz_newVar(oz_rootBoard()));

  {
    Board * bb = oz_currentBoard();
  
    FdDistributor * fdd;
    
    switch (PP(var_sel,val_sel)) {
      PPCL(Naive,Min);
      PPCL(Naive,Mid);
      PPCL(Naive,Max);
      PPCL(Naive,SplitMin);
      PPCL(Naive,SplitMax);

      PPCL(Size,Min);
      PPCL(Size,Mid);
      PPCL(Size,Max);
      PPCL(Size,SplitMin);
      PPCL(Size,SplitMax);

      PPCL(Min,Min);
      PPCL(Min,Mid);
      PPCL(Min,Max);
      PPCL(Min,SplitMin);
      PPCL(Min,SplitMax);

      PPCL(Max,Min);
      PPCL(Max,Mid);
      PPCL(Max,Max);
      PPCL(Max,SplitMin);
      PPCL(Max,SplitMax);

      PPCL(NbSusps,Min);
      PPCL(NbSusps,Mid);
      PPCL(NbSusps,Max);
      PPCL(NbSusps,SplitMin);
      PPCL(NbSusps,SplitMax);
    default:
      Assert(0);
    }
    bb->addToDistBag(fdd);
    
    OZ_RETURN(fdd->getSync());
  }
  
 bomb:
  oz_typeError(0,"vector of finite domains");
}
OZ_BI_end

