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

#ifndef __FDBUILTIN_H__
#define __FDBUILTIN_H__

#ifdef INTERFACE
#pragma interface
#endif

#include "fdomn.hh"


enum Recalc_e {lower, upper};

//-----------------------------------------------------------------------------
// Debug Macros
//-----------------------------------------------------------------------------

#if defined(DEBUG_CHECK)
#else
#endif

//-----------------------------------------------------------------------------
// Macros

#define MAXFDBIARGS 1000

#define FailFD FAILED
#define SuspendFD SLEEP
#define EntailFD PROCEED

#define FailOnEmpty(X)				\
{						\
  if((X) == 0) {				\
    BIfdBodyManager::restoreDomainOnToplevel();	\
    return FailFD;				\
  }						\
}

#include "fdhook.hh"
#include "genvar.hh"

#define SimplifyOnUnify(EQ01, EQ02, EQ12) \
  if (isUnifyCurrentPropagator ()) { \
    OZ_getCArgDeref(0, x, xPtr, xTag); \
    OZ_getCArgDeref(1, y, yPtr, yTag); \
    if (xPtr == yPtr && isVariableTag(xTag)) { \
      return (EQ01); \
    } \
    OZ_getCArgDeref(2, z, zPtr, zTag); \
    if (xPtr == zPtr && isVariableTag(xTag)) { \
      return (EQ02); \
    } \
    if (yPtr == zPtr && isVariableTag(yTag)) { \
      return (EQ12); \
    } \
  }

//-----------------------------------------------------------------------------
// pm_  

enum pm_term_type {pm_none = 0x0, pm_singl = 0x1, 
		   pm_bool = 0x2, pm_fd = 0x4, 
		   pm_svar = 0x8, pm_uvar = 0x10, 
		   pm_tuple = 0x20, pm_literal = 0x40};

inline 
OZ_Boolean pm_is_var(pm_term_type t) {
  return t & (pm_bool | pm_fd | pm_svar | pm_uvar);
}

inline 
OZ_Boolean pm_is_noncvar(pm_term_type t) {
  return t & (pm_svar | pm_uvar);
}

//-----------------------------------------------------------------------------
// Global Variables relenvant for FD Built-ins

extern double * static_coeff_double;
extern int * static_coeff_int;
extern OZ_Boolean * static_sign_bit;
extern OZ_Term * static_var;
extern OZ_Term ** static_varptr;
extern pm_term_type * static_vartag;
extern OZ_Boolean * static_bool_a;
extern OZ_Boolean * static_bool_b;
extern int * static_int_a;
extern int * static_int_b;
extern double * static_double_a;
extern double * static_double_b;
extern int * static_index_offset;
extern int * static_index_size;


//-----------------------------------------------------------------------------
// Auxiliary stuff
//-----------------------------------------------------------------------------

// return TRUE if i is not negative
OZ_Boolean getSign(int i);
OZ_Boolean getSign(double  i);
void getSignbit(int i, int n);
void getDoubleCoeff(int i, OZ_Term v);
void getIntCoeff(int i, OZ_Term v);

//-----------------------------------------------------------------------------
//                          class BIfdHeadManager
//-----------------------------------------------------------------------------

class BIfdHeadManager {
private:
  static OZ_Term * bifdhm_var;
  static OZ_Term ** bifdhm_varptr;
  static pm_term_type * bifdhm_vartag;
  static int * bifdhm_coeff;
  static int curr_num_of_items;
  
  int global_vars;

public:
  BIfdHeadManager(int s);

  void increaseSizeBy(int s);
  
  static void initStaticData(void);
  
  OZ_Return addSuspFDish(OZ_CFun, OZ_Term *, int);
  OZ_Return addSuspSingl(OZ_CFun, OZ_Term *, int);
  OZ_Boolean addSuspXorYdet(OZ_CFun, OZ_Term *, int);
  
  int getCoeff(int i);

  pm_term_type getTag(int i);

  int getCurrNumOfItems(void) {return curr_num_of_items;}
  
  OZ_Return spawnPropagator(OZ_FDPropState, OZ_CFun, int, OZ_Term *);
  OZ_Return spawnPropagator(OZ_FDPropState, OZ_FDPropState, OZ_CFun, int, OZ_Term *);
  OZ_Return spawnPropagator(OZ_FDPropState, OZ_CFun, int, OZ_Term, ...);

  OZ_Boolean areIdentVar(int a, int b);

  void printDebug(void);
  void printDebug(int i);
};


//-----------------------------------------------------------------------------
//                           class BIfdBodyManager
//-----------------------------------------------------------------------------

enum fdbm_var_state {fdbm_local, fdbm_global, fdbm_speculative};
inline
char * fdbm_var_stat2char(fdbm_var_state s) {
  static char * fdbm_var_state_names[3] = {"local", "global", "speculative"};
  return fdbm_var_state_names[s];
}

inline
int idx(int i, int j) {
  AssertFD(0 <= j && j < static_index_size[i]);
  AssertFD(0 <= static_index_offset[i] + j &&
	   static_index_offset[i] + j < MAXFDBIARGS);

  return static_index_offset[i] + j;
}

inline int idx_b(int i) { return idx(0, i); }
inline int idx_v(int i) { return idx(1, i); }
inline int idx_vp(int c, int v) { return idx(2+c, v); }

extern OZ_FiniteDomain __CDVoidFiniteDomain;

class BIfdBodyManager {
private:
// data slots in charge  
  static OZ_Term * bifdbm_var;
  static OZ_Term ** bifdbm_varptr;
  static pm_term_type * bifdbm_vartag;

  static OZ_FiniteDomain ** bifdbm_dom;
  static OZ_FiniteDomain * bifdbm_domain;

  static int curr_num_of_vars;
  static int * bifdbm_init_dom_size;
  static int * cache_from;
  static int * cache_to;
  static int * index_offset;
  static int * index_size;
  
  static OZ_Boolean vars_left;
  static OZ_Boolean only_local_vars;
  static fdbm_var_state * bifdbm_var_state;

// backup data slots
  int backup_count;
  int backup_curr_num_of_vars1;
  OZ_Boolean backup_vars_left1;
  OZ_Boolean backup_only_local_vars1;

// private methods
  OZ_Boolean isTouched(int i);
  
  void process(void);
  void processFromTo(int, int);
  void processLocal(void) {processLocalFromTo(0, curr_num_of_vars);}

  void processLocalFromTo(int, int);
  void processNonRes(void);
  
  void _introduce(int i, OZ_Term v);
  void introduceLocal(int i, OZ_Term v);
  void saveDomainOnTopLevel(int i);
  void _propagate_unify_cd(int clauses, int variables, SRecord &st);

  enum {cache_slot_size = 4};

  void setSpeculative(int i);
public:
  BIfdBodyManager(int s);

  void add(int i, int size);

  int getCurrNumOfVars(void) {return curr_num_of_vars;}

  int initCache(void);
  int getCacheSlotFrom(int i) {return cache_from[i];}
  int getCacheSlotTo(int i) {return cache_to[i];}

  OZ_Boolean allVarsAreLocal(void) {return only_local_vars;}
  
  static void initStaticData(void);
  
  void backup(void);
  void restore(void);
  
  OZ_FiniteDomain &operator [](int i);

  OZ_FiniteDomain &operator ()(int i, int j);

  void printDebug(void);
  void printDebug(int i);
  void printTerm(void);
  void printTerm(int i);
  
  void introduceDummy(int i);
  void introduce(int i, OZ_Term v);
  void introduce(int i, int j, OZ_Term v);
  void introduceSpeculative(int i, OZ_Term v);
  
  void reintroduce(int i, OZ_Term v);

  void process(int i) {processFromTo(i, i+1);}

  OZ_Return entailment(void);

  OZ_Return entailmentClause(int from_b, int to_b,
			   int from, int to,
			   int from_p, int to_p);
  
  OZ_Return entailmentClause(int from_b, int to_b);
  
  OZ_Return release(int from, int to);
    
  OZ_Return releaseReify(int from_b, int to_b, int from, int to);
  
  OZ_Return release(void) {return release(0, curr_num_of_vars - 1);}

  // used by square and twice
  OZ_Return release1(void);
  // used by putList, putNot, putLe, putGe
  OZ_Return releaseNonRes(void);

  OZ_Boolean _unifiedVars(void);
  OZ_Boolean unifiedVars(void);

  void propagate_unify_cd(int cl, int vars, SRecord &st);

  OZ_Boolean isNotCDVoid(int i) {return bifdbm_dom[i] != &__CDVoidFiniteDomain;}
  
  OZ_Boolean areIdentVar(int a, int b);
  
  OZ_FiniteDomain ** getDoms(void) {return bifdbm_dom;}

// exactly one variable is regarded
  BIfdBodyManager(void) {backup_count = 0; curr_num_of_vars = 1;}

  OZ_Boolean introduce(OZ_Term v);

  OZ_FiniteDomain &operator *(void) {return *bifdbm_dom[0];}

  static void restoreDomainOnToplevel(void);

}; // BIfdBodyManager


#ifndef OUTLINE
#include "fdbuilti.icc"
#endif


#endif
