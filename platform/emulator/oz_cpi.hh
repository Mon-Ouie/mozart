/*
 *  Authors:
 *    Tobias Mueller (tmueller@ps.uni-sb.de)
 * 
 *  Contributors:
 *    Christian Schulte (schulte@dfki.de)
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

#ifndef __OZ_CPI_HH__
#define __OZ_CPI_HH__

#include <stddef.h>
#include <stdio.h>
#include "oz.h"

//#define DEBUG_FSET
//#define DEBUG_FD
//#define DEBUG_NONMONOTONIC

#if defined(DEBUG_FSET) || defined(DEBUG_FD)
//#define CPI_FILE_PRINT
#endif




//-----------------------------------------------------------------------------
// misc macros

#define OZ_EXPECTED_TYPE(S) char * expectedType = S

#define OZ_EXPECT(O, P, F)                                                  \
  {                                                                         \
    OZ_expect_t r = O.F(OZ_args[P]);                                        \
    if (O.isFailing(r)) {                                                   \
      O.fail();                                                             \
      return OZ_typeError(expectedType, P, "");                             \
    } else if (O.isSuspending(r))                                           \
      return O.suspend(OZ_makeSuspendedThread(OZ_self,OZ_args,OZ_arity));   \
  }

#define OZ_EXPECT_SUSPEND(O, P, F, SC)                                      \
  {                                                                         \
    OZ_expect_t r = O.F(OZ_args[P]);                                        \
    if (O.isFailing(r)) {                                                   \
      O.fail();                                                             \
      return OZ_typeError(expectedType, P, "");                             \
    } else if (O.isSuspending(r))                                           \
      SC += 1;                                                              \
  }

#define _OZ_EM_FDINF    "0"
#define _OZ_EM_FDSUP    "134 217 726"
#define _OZ_EM_FSETINF  "0"
#define _OZ_EM_FSETSUP  "63"
#define _OZ_EM_INTMAX   "134 217 727"

#define OZ_EM_LIT       "literal"
#define OZ_EM_INT       "integer in [~"_OZ_EM_INTMAX"\\,...\\,"_OZ_EM_INTMAX"]"
#define OZ_EM_FD        "finite domain integer in {"_OZ_EM_FDINF"\\,...\\,"_OZ_EM_FDSUP"}"
#define OZ_EM_FDDESCR   "description of finite domain integer"
#define OZ_EM_FSETVAL   "finite set of integers"
#define OZ_EM_FSET      "finite set of integers constraint"
#define OZ_EM_FSETDESCR "description of finite set of integers"
#define OZ_EM_VECT      "vector of "
#define OZ_EM_TNAME     "truth name"
#define OZ_EM_STREAM    "stream"

//-----------------------------------------------------------------------------
// OZ_FiniteDomain 

class OZ_FSetValue;

enum OZ_FDState {fd_empty, fd_full, fd_bool, fd_singl};

class OZ_FiniteDomain {
protected:
  int min_elem, max_elem, size;
  void * descr;

public:

  OZ_FiniteDomain(void) : descr((void *) 0) {}
  OZ_FiniteDomain(OZ_FDState state);
  OZ_FiniteDomain(const OZ_FiniteDomain &);
  OZ_FiniteDomain(OZ_Term);
  OZ_FiniteDomain(const OZ_FSetValue &);

  int initRange(int, int);
  int initSingleton(int);
  int initDescr(OZ_Term);
  int initFull(void);
  int initEmpty(void);
  int initBool(void);

  int getMidElem(void) const; 
  int getNextSmallerElem(int v) const;
  int getNextLargerElem(int v) const;
  int getLowerIntervalBd(int v) const;
  int getUpperIntervalBd(int v) const;
  int getSize(void) const { return size; }
  int getMinElem(void) const { return min_elem; }
  int getMaxElem(void) const { return max_elem; }
  int getSingleElem(void) const;
  OZ_Term getDescr(void) const;

  const OZ_FiniteDomain &operator = (const OZ_FiniteDomain &fd);
  OZ_Boolean operator == (const OZ_FDState) const;
  OZ_Boolean operator == (const int) const;
  OZ_Boolean operator != (const OZ_FDState) const;
  OZ_Boolean operator != (const int) const;

  OZ_FiniteDomain operator & (const OZ_FiniteDomain &) const; 
  OZ_FiniteDomain operator | (const OZ_FiniteDomain &) const; 
  OZ_FiniteDomain operator ~ (void) const;                    

  int operator &= (const OZ_FiniteDomain &);
  int operator &= (const int);
  int operator += (const int); 
  int operator -= (const int); 
  int operator -= (const OZ_FiniteDomain &);
  int operator <= (const int);    
  int operator >= (const int);

  int constrainBool(void);
  int intersectWithBool(void);
  OZ_Boolean isIn(int i) const;
  void copyExtension(void);
  void disposeExtension(void);

  char * toString(void) const;
};   


//-----------------------------------------------------------------------------
// OZ_FSetValue

enum OZ_FSetState {fs_empty, fs_full};

const int fset_high = 2;
//const int fset_high = 220;
const int fsethigh32 = 32*fset_high;

class OZ_FSetConstraint;

class OZ_FSetValue {
protected:
  int _card;
  int _in[fset_high];

public:
  OZ_FSetValue(void) {}
  OZ_FSetValue(const OZ_FSetConstraint&);
  OZ_FSetValue(const OZ_Term);
  OZ_FSetValue(const OZ_FSetState);
  OZ_FSetValue(int, int);
  OZ_FSetValue(const OZ_FiniteDomain &);

  int getCard(void) const { return _card; }
  int getKnownNotIn(void) const { return fsethigh32 - _card; }
  OZ_Boolean isIn(int) const;
  OZ_Boolean isNotIn(int) const;
  int getMinElem(void) const;
  int getMaxElem(void) const;
  int getNextLargerElem(int) const;
  int getNextSmallerElem(int) const;
  OZ_Term getKnownInList(void) const;
  OZ_Term getKnownNotInList(void) const;
  char * toString(void);

  // comparison
  OZ_Boolean operator == (const OZ_FSetValue &) const;
  OZ_Boolean operator <= (const OZ_FSetValue &) const;

  OZ_FSetValue operator & (const OZ_FSetValue &) const;
  OZ_FSetValue operator | (const OZ_FSetValue &) const;
  OZ_FSetValue operator - (const OZ_FSetValue &) const;
  OZ_FSetValue operator &= (const OZ_FSetValue &);
  OZ_FSetValue operator |= (const OZ_FSetValue &);
  OZ_FSetValue operator &= (const int);
  OZ_FSetValue operator += (const int);
  OZ_FSetValue operator -= (const int);
  OZ_FSetValue operator - (void) const;

  static void * operator new(size_t);
  static void operator delete(void *, size_t);
};


//-----------------------------------------------------------------------------
// OZ_FSetConstraint 


enum OZ_FSetPropState {fs_prop_glb = 0, fs_prop_lub, fs_prop_val, 
		       fs_prop_any, fs_prop_bounds};

class OZ_FSetConstraint {
protected:
  int _card_min, _card_max; 
  int _known_not_in, _known_in;
  int _in[fset_high], _not_in[fset_high];

public:
  OZ_FSetConstraint(void) {}
  OZ_FSetConstraint(const OZ_FSetValue &);
  OZ_FSetConstraint(OZ_FSetState);

  OZ_FSetConstraint(const OZ_FSetConstraint &);
  OZ_FSetConstraint &operator = (const OZ_FSetConstraint &);

  int getKnownIn(void) const { return _known_in; }
  int getKnownNotIn(void) const { return _known_not_in; }
  int getUnknown(void) const { 
    return fsethigh32 - _known_in - _known_not_in; 
  }

  OZ_FSetValue getGlbSet(void) const;
  OZ_FSetValue getLubSet(void) const;
  OZ_FSetValue getUnknownSet(void) const;
  OZ_FSetValue getNotInSet(void) const;

  int getGlbCard(void) const;
  int getLubCard(void) const;
  int getNotInCard(void) const;
  int getUnknownCard(void) const;

  int getGlbMinElem(void) const;
  int getLubMinElem(void) const;
  int getNotInMinElem(void) const;
  int getUnknownMinElem(void) const;

  int getGlbMaxElem(void) const;
  int getLubMaxElem(void) const;
  int getNotInMaxElem(void) const;
  int getUnknownMaxElem(void) const;

  int getGlbNextSmallerElem(int) const;
  int getLubNextSmallerElem(int) const;
  int getNotInNextSmallerElem(int) const;
  int getUnknownNextSmallerElem(int) const;

  int getGlbNextLargerElem(int) const;
  int getLubNextLargerElem(int) const;
  int getNotInNextLargerElem(int) const;
  int getUnknownNextLargerElem(int) const;

  int getCardSize(void) const { return _card_max - _card_min + 1; }
  int getCardMin(void) const { return _card_min; } 
  int getCardMax(void) const { return _card_max; } 

  OZ_Boolean putCard(int, int);
  OZ_Boolean isValue(void) const;

  void init(void);
  void init(const OZ_FSetValue &);
  void init(OZ_FSetState);

  OZ_Boolean isIn(int) const;
  OZ_Boolean isNotIn(int) const;
  OZ_Boolean isEmpty(void) const;
  OZ_Boolean isFull(void) const;
  OZ_Boolean isSubsumedBy(const OZ_FSetConstraint &) const;
  OZ_Term getKnownInList(void) const;
  OZ_Term getKnownNotInList(void) const;
  OZ_Term getUnknownList(void) const;
  OZ_Term getLubList(void) const;
  OZ_Term getCardTuple(void) const;
  OZ_FSetConstraint operator - (void) const;
  OZ_Boolean operator += (int);
  OZ_Boolean operator -= (int);
  OZ_Boolean operator <<= (const OZ_FSetConstraint &);
  OZ_Boolean operator %= (const OZ_FSetConstraint &);
  OZ_FSetConstraint operator & (const OZ_FSetConstraint &) const;
  OZ_FSetConstraint operator | (const OZ_FSetConstraint &) const;
  OZ_FSetConstraint operator - (const OZ_FSetConstraint &) const;
  OZ_Boolean operator <= (const OZ_FSetConstraint &);
  OZ_Boolean operator >= (const OZ_FSetConstraint &);
  OZ_Boolean operator != (const OZ_FSetConstraint &);
  OZ_Boolean operator == (const OZ_FSetConstraint &) const;
  OZ_Boolean operator <= (const int);
  OZ_Boolean operator >= (const int);
  char * toString(void);
};   


//-----------------------------------------------------------------------------
// class OZ_Propagator

class OZ_NonMonotonic {
public:
  typedef unsigned order_t;
private:
  order_t _order; 
  static order_t _next_order;
public:
  OZ_NonMonotonic(void);
  order_t getOrder(void) const { return _order; }
};

class OZ_CFunHeader {
private:
  OZ_CFunHeader * _next;
  static OZ_CFunHeader * _all_headers;
  OZ_CFun _header;
  unsigned _calls, _samples, _heap;

public:
  OZ_CFunHeader(OZ_CFun header);
  
  OZ_CFun getHeaderFunc(void) { return _header; }
  void incSamples()           { _samples++; }
  void incCalls()             { _calls++; }
  unsigned getSamples()       { return _samples; }
  unsigned getCalls()         { return _calls; }
  void incHeap(unsigned inc)  { _heap += inc; }
  unsigned getHeap()          { return _heap; }

  static OZ_CFunHeader *getFirst() { return _all_headers; }
  OZ_CFunHeader *getNext()         { return _next; }

  static void profileReset();
};

enum OZ_FDPropState {fd_prop_singl = 0, fd_prop_bounds, fd_prop_any};

// virtual base class; never create an object from this class
class OZ_Propagator {
  friend class Thread;
private:
  OZ_Propagator * gc(void); 
  
public:
  OZ_Propagator(void) {}
  virtual ~OZ_Propagator(void) {}

  static void * operator new(size_t);
  static void operator delete(void *, size_t);

  OZ_Boolean mayBeEqualVars(void);
  OZ_Return replaceBy(OZ_Propagator *);
  OZ_Return replaceBy(OZ_Term, OZ_Term);
  OZ_Return replaceByInt(OZ_Term, int);
  OZ_Return postpone(void);
  OZ_Boolean imposeOn(OZ_Term);
  OZ_Boolean addImpose(OZ_FDPropState s, OZ_Term v);
  OZ_Boolean addImpose(OZ_FSetPropState s, OZ_Term v);
  void impose(OZ_Propagator * p, int prio = OZ_getMediumPrio());
  virtual size_t sizeOf(void) = 0;
  virtual void updateHeapRefs(OZ_Boolean duplicate) = 0;
  virtual OZ_Return propagate(void) = 0;
  virtual OZ_Term getParameters(void) const = 0;
  virtual OZ_CFunHeader * getHeader(void) const = 0;

  // support for nonmonotonic propagator
  virtual OZ_Boolean isMonotonic(void) const { return OZ_TRUE; }
  virtual OZ_NonMonotonic::order_t getOrder(void) const { 
    return 0; 
  }

  char * toString(void) const;
};

//-----------------------------------------------------------------------------
// class OZ_Expect, etc.

struct OZ_expect_t {
  int size, accepted; 
  OZ_expect_t(int s, int a) : size(s), accepted(a) {}
};

enum OZ_PropagatorFlags {NULL_flag, OFS_flag};

class OZ_Expect;

typedef OZ_expect_t (OZ_Expect::*OZ_ExpectMeth) (OZ_Term);

class OZ_Expect {
private:
  OZ_Boolean collect;

  OZ_expect_t _expectFSetDescr(OZ_Term descr, int level);
protected:
  void addSpawn(OZ_FDPropState, OZ_Term *);
  void addSpawn(OZ_FSetPropState, OZ_Term *);
  void addSuspend(OZ_Term *);
  void addSuspend(OZ_FDPropState, OZ_Term *);
  void addSuspend(OZ_FSetPropState, OZ_Term *);
public:
  OZ_Expect(void); 
  ~OZ_Expect(void); 

  void collectVarsOn(void);
  void collectVarsOff(void);

  OZ_expect_t expectDomDescr(OZ_Term descr, int level = 4);
  OZ_expect_t expectFSetDescr(OZ_Term descr) {
    return _expectFSetDescr(descr, 3);
  }
  OZ_expect_t expectVar(OZ_Term t);
  OZ_expect_t expectRecordVar(OZ_Term);
  OZ_expect_t expectIntVar(OZ_Term, OZ_FDPropState = fd_prop_any);
  OZ_expect_t expectFSetVar(OZ_Term, OZ_FSetPropState = fs_prop_any);
  OZ_expect_t expectInt(OZ_Term);
  OZ_expect_t expectFSetValue(OZ_Term);
  OZ_expect_t expectLiteral(OZ_Term);
  OZ_expect_t expectVector(OZ_Term, OZ_ExpectMeth);
  OZ_expect_t expectProperRecord(OZ_Term, OZ_ExpectMeth);
  OZ_expect_t expectProperTuple(OZ_Term, OZ_ExpectMeth);
  OZ_expect_t expectList(OZ_Term, OZ_ExpectMeth);
  OZ_expect_t expectStream(OZ_Term st); 

  OZ_Return impose(OZ_Propagator * p, 
		   int prio = OZ_getMediumPrio(),
		   OZ_PropagatorFlags flags=NULL_flag);
  OZ_Return suspend(OZ_Thread);
  OZ_Return fail(void);
  OZ_Boolean isSuspending(OZ_expect_t r) {
    return (r.accepted == 0 || (0 < r.accepted && r.accepted < r.size));
  }
  OZ_Boolean isFailing(OZ_expect_t r) {
    return (r.accepted == -1);
  }
};


//-----------------------------------------------------------------------------
// class OZ_FDIntVar

class OZ_FDIntVar {
private:
  OZ_FiniteDomain dom;
  OZ_FiniteDomain * domPtr;
  OZ_Term var;
  OZ_Term * varPtr;
  int initial_size, initial_width;
  enum Sort_e {sgl_e = 1, bool_e = 2, int_e  = 3} sort;
  enum State_e {loc_e = 1, glob_e = 2, encap_e = 3} state;
  OZ_Boolean isSort(Sort_e s) const {return s == sort;}
  void setSort(Sort_e s) {sort = s;}
  OZ_Boolean isState(State_e s) const {return s == state;}
  void setState(State_e s) {state = s;}

  OZ_Boolean tell(void);
public:
  OZ_FDIntVar(void) {}
  OZ_FDIntVar(OZ_Term v) { read(v); }
		      
  static void * operator new(size_t);
  static void operator delete(void *, size_t);

#ifdef __GNUC__
  // mm2: portability ?
  static void * operator new[](size_t);
  static void operator delete[](void *, size_t);
#endif

  OZ_FiniteDomain &operator * (void) {return *domPtr;}
  OZ_FiniteDomain * operator -> (void) {return domPtr;}

  OZ_Boolean isTouched(void) const {return initial_size > domPtr->getSize();}

  void ask(OZ_Term);
  int read(OZ_Term);
  int readEncap(OZ_Term);
  OZ_Boolean leave(void) { return isSort(sgl_e) ? OZ_FALSE : tell(); }
  void fail(void);
};


//-----------------------------------------------------------------------------
// class OZ_FSetVar
// the whole class is not documented

class OZ_FSetVar {
private:
  OZ_FSetConstraint set;
  OZ_FSetConstraint * setPtr;
  OZ_Term var;
  OZ_Term * varPtr;
  int known_in, known_not_in, card_size;
  enum Sort_e {val_e = 1, var_e = 2} sort;
  enum State_e {loc_e = 1, glob_e = 2, encap_e = 3} state;
  OZ_Boolean isSort(Sort_e s) const {return s == sort;}
  void setSort(Sort_e s) {sort = s;}
  OZ_Boolean isState(State_e s) const {return s == state;}
  void setState(State_e s) {state = s;}

  OZ_Boolean tell(void);
public:
  OZ_FSetVar(void) {}
  OZ_FSetVar(OZ_Term v) { read(v); }
		      
  static void * operator new(size_t);
  static void operator delete(void *, size_t);

#ifdef __GNUC__
  // mm2: portability ?
  static void * operator new[](size_t);
  static void operator delete[](void *, size_t);
#endif

  OZ_FSetConstraint &operator * (void) {return *setPtr;}
  OZ_FSetConstraint * operator -> (void) {return setPtr;}

  OZ_Boolean isTouched(void) const;

  void ask(OZ_Term);
  void read(OZ_Term);
  void readEncap(OZ_Term);
  OZ_Boolean leave(void) { return isSort(val_e) ? OZ_FALSE : tell(); }
  void fail(void);
};


//-----------------------------------------------------------------------------
// class OZ_Stream

class OZ_Stream {
private:
  OZ_Boolean closed, eostr, valid;
  OZ_Term tail;

  void setFlags(void);
public:
  OZ_Stream(OZ_Term st) : tail(st) { setFlags(); }
  OZ_Boolean isEostr(void) { return eostr; }
  OZ_Boolean isClosed(void) { return closed; }
  OZ_Boolean isValid(void) { return valid; }

  OZ_Term get(void);
  OZ_Term getTail(void) { return tail; }
  OZ_Term put(OZ_Term, OZ_Term);

  OZ_Boolean leave(void);
  void fail(void);
};


//-----------------------------------------------------------------------------
// Miscellaneous

extern void   OZ_collectHeapBlock(OZ_Term *, OZ_Term *, int);
extern void   OZ_collectHeapTerm(OZ_Term &, OZ_Term &);
extern void * OZ_hrealloc(void *, size_t);

inline void OZ_updateHeapTerm(OZ_Term & to) {
  OZ_collectHeapTerm(to,to);
}

OZ_Boolean OZ_isPosSmallInt(OZ_Term val);

OZ_Term * OZ_hallocOzTerms(int);
int *     OZ_hallocCInts(int);
char *    OZ_hallocChars(int);
OZ_Term * OZ_copyOzTerms(int, OZ_Term *);
inline int * OZ_copyCInts(int n, int * frm) {
  return (n>0) ? ((int *) OZ_hrealloc(frm, n*sizeof(int))) : ((int *) 0);
}
char *    OZ_copyChars(int, char *);
void      OZ_hfreeOzTerms(OZ_Term *, int);
void      OZ_hfreeCInts(int *, int);
void      OZ_hfreeChars(char *, int);

int * OZ_findEqualVars(int, OZ_Term *); // static return value
int * OZ_findSingletons(int, OZ_Term *); // static return value

OZ_Boolean OZ_isEqualVars(OZ_Term, OZ_Term);

OZ_Return OZ_typeError(char *, int, char *);

int OZ_getFDInf(void);
int OZ_getFDSup(void);
int OZ_getFSetInf(void);
int OZ_getFSetSup(void);

int OZ_vectorSize(OZ_Term);

OZ_Term * OZ_getOzTermVector(OZ_Term, OZ_Term *);
int * OZ_getCIntVector(OZ_Term, int *);

#endif // __OZ_CPI_HH__
