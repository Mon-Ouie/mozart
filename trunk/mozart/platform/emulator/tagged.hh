/*
 *  Authors:
 *    Ralf Scheidhauer (Ralf.Scheidhauer@ps.uni-sb.de)
 * 
 *  Contributors:
 *    Michael Mehl (mehl@dfki.de)
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

/* classes:
     TaggedRef
     RefsArray
     */


#ifndef __TAGGEDH
#define __TAGGEDH

#ifdef INTERFACE
#pragma interface
#endif

#include <stdio.h>

#include "machine.hh"
#include "types.hh"
#include "error.hh"
#include "mem.hh"
#include "gc.hh"

// ---------------------------------------------------------------------------
// ---------- TAGGED REF -----------------------------------------------------
// ---------------------------------------------------------------------------

// --- TaggedRef: Type Declaration

enum TypeOfTerm {
  REFTAG1          =  0,   // 0000
  REFTAG2          =  4,   // 0100
  REFTAG3          =  8,   // 1000
  REFTAG4          = 12,   // 1100
  
  UVAR             =  1,   // 0001
  SVAR             =  9,   // 1001
  CVAR             =  5,   // 0101
  
  GCTAG            =  13,  // 1101    --> !!! isAnyVar(GCTAG) = 1 !!!
	    
  LTUPLE           =  2,   // 0010
  FSETVALUE        = 14,   // 1110
  SRECORD          =  3,   // 0011
		  
  LITERAL          = 15,   // 1111

  OZCONST          = 10,   // 1010

  SMALLINT         =  6,   // 0110
  BIGINT           =  7,   // 0111
  OZFLOAT          = 11    // 1011
};  

// ---------------------------------------------------------------------------
// --- TaggedRef: CLASS / BASIC ACCESS FUNCTIONS


typedef TaggedRef *TaggedRefPtr;

#define tagSize 4
#define tagMask 0xF


// ------------------------------------------------------
// Debug macros for debugging outside of gc

extern int gcing;

#define _tagTypeOf(ref) ((TypeOfTerm)(ref&tagMask))

#ifdef DEBUG_GC  
#define GCDEBUG(X)                            \
  if ( gcing && (_tagTypeOf(X)==GCTAG ) )     \
    error("GcTag unexpectedly found.");
#else
#define GCDEBUG(X)
#endif


#define TaggedToPointer(t) ((void*) (mallocBase|t))


#ifdef LARGEADRESSES
#define _tagValueOf(ref)         TaggedToPointer((ref >> (tagSize-2))&~3)
#define _tagValueOfVerbatim(ref) ((void*)((ref >> (tagSize-2))&~3))
#define _makeTaggedRef2(tag,i)   ((i << (tagSize-2)) | tag)

#else
#define _tagValueOf(ref)         TaggedToPointer(ref >> tagSize)
#define _tagValueOfVerbatim(ref) (ref >> tagSize)
#define _makeTaggedRef2(tag,i)   ((i << tagSize) | (tag))

#endif

#define _isNullPtr(p)             ((p&~(tagMask)) == 0)
#define _makeTaggedRef(s)         ((TaggedRef) ToInt32(s))
#define _makeTaggedRef2i(tag,ptr) _makeTaggedRef2(tag,(int32)ToInt32(ptr))


#ifdef DEBUG_CHECK
inline
void *tagValueOf(TaggedRef ref) 
{ 
  GCDEBUG(ref);
  return _tagValueOf(ref);
}

inline
void *tagValueOfVerbatim(TaggedRef ref) 
{ 
  GCDEBUG(ref);
  return _tagValueOfVerbatim(ref);
}

inline
TypeOfTerm tagTypeOf(TaggedRef ref) 
{ 
  GCDEBUG(ref);
  return _tagTypeOf(ref);
}

inline
TaggedRef makeTaggedRef2i(TypeOfTerm tag, int32 i)
{
#ifdef LARGEADRESSES
  Assert((i&3) == 0);
#endif
  return _makeTaggedRef2(tag,i);
}

inline
TaggedRef makeTaggedRef2p(TypeOfTerm tag, void *ptr)
{
  return _makeTaggedRef2i(tag,ptr);
}

inline
Bool isNullPtr(TaggedRef p) { return _isNullPtr(p); }

#else

#define tagValueOf(ref)         _tagValueOf(ref)
#define tagValueOfVerbatim(ref) _tagValueOfVerbatim(ref)
#define tagTypeOf(ref)          _tagTypeOf(ref)
#define makeTaggedRef2i(tag,i)  _makeTaggedRef2(tag,i)
#define makeTaggedRef2p(tag,i)  _makeTaggedRef2i(tag,i)
#define isNullPtr(p)            _isNullPtr(p)
#define makeTaggedRef(s)        _makeTaggedRef(s)

#endif


// ---------------------------------------------------------------------------
// --- TaggedRef: useful functions --> print.C

char *tagged2String(TaggedRef ref, int depth, int offset = 0);
void taggedPrint(TaggedRef ref,int depth = 10, int offset = 0);
void taggedPrintLong(TaggedRef ref, int depth = 10, int offset = 0);
void tagged2Stream(TaggedRef, ostream & = cout, int = 10,int = 0);

// ---------------------------------------------------------------------------
// --- TaggedRef: CHECK_xx

// Philosophy:
//   Arguments which are passed around are never variables, but only
//     REF or bound data
#define CHECK_NONVAR(term) Assert(isRef(term) || !isAnyVar(term))
#define CHECK_ISVAR(term)  Assert(isAnyVar(term))
#define CHECK_DEREF(term)  Assert(!isRef(term) && !isAnyVar(term))
#define CHECK_POINTER(s)   Assert(!(ToInt32(s) & 3))
#define CHECK_POINTER_N(s)  Assert(s != NULL && !(ToInt32(s) & 3))
#define CHECK_STRPTR(s)    Assert(s != NULL)
#define CHECKTAG(Tag)      Assert(tagTypeOf(ref) == Tag)


// ---------------------------------------------------------------------------
// --- TaggedRef: BASIC TYPE TESTS

// if you want to test a term:
//   1. if you have the tag: is<Type>(tag)
//   2. if you have the term: is<Type>(term)
//   3. if you need many tests:
//        switch(typeOf(term)) { ... }
//    or  switch(tag) { ... }


#define IsRef(term) ((term & 3) == 0)
inline
Bool isRef(TaggedRef term) {
  GCDEBUG(term);
  return IsRef(term);
}


// ---------------------------------------------------------------------------
// Tests for variables and their semantics:
//                           tag
// unconstrained var         0001 (UVAR:1)  isUVar   isNotCVar  isAnyVar
// unconstr. suspending var  1001 (SVAR:9)  isSVar   isNotCVar  isAnyVar
// constrained-susp. var     0101 (CVAR:5)           isCVar    isAnyVar
// ---------------------------------------------------------------------------


inline
Bool isSVar(TypeOfTerm tag) {
  return (tag == SVAR);
}

inline
Bool isSVar(TaggedRef term) {
  GCDEBUG(term);
  Assert(!isRef(term));
  return isSVar(tagTypeOf(term));
}


inline
Bool isCVar(TypeOfTerm tag) {
  return (tag == CVAR);
}

inline
Bool isCVar(TaggedRef term) {
  GCDEBUG(term);
  Assert(!isRef(term));
  return isCVar(tagTypeOf(term));
}


/*
 * Optimized tests for some most often used types: no untagging needed
 * for type tests!
 * Use macros if not DEBUG_CHECK, since gcc creates awful code
 * for inline function version
 */

#define _isAnyVar(val)    (((TaggedRef) val&2)==0)       /* mask = 0010 */
#define _isDirectVar(val) (((TaggedRef) val&3)==1)       /* mask = 0011 */
#define _isNotCVar(val)   (((TaggedRef) val&6)==0)       /* mask = 0110 */
#define _isUVar(val)      (((TaggedRef) val&14)==0)      /* mask = 1110 */
#define _isLTuple(val)    (((TaggedRef) val&13)==0)      /* mask = 1101 */

#ifdef DEBUG_CHECK

inline
TaggedRef makeTaggedRef(TaggedRef *s)
{
  CHECK_POINTER_N(s);
  DebugGC(gcing == 0 && !MemChunks::list->inChunkChain ((void *)s),
	  error ("making TaggedRef pointing to 'from' space"));
  return _makeTaggedRef(s);
}


inline Bool isLTuple(TypeOfTerm tag) { return _isLTuple(tag);}

inline
Bool isLTuple(TaggedRef term) {
  GCDEBUG(term);
  Assert(!isRef(term));
  return _isLTuple(term);
}


inline Bool isUVar(TypeOfTerm tag) { return _isUVar(tag);}

inline
Bool isUVar(TaggedRef term) {
  GCDEBUG(term);
  Assert(!isRef(term));
  return _isUVar(term);
}

inline Bool isAnyVar(TypeOfTerm tag) { return _isAnyVar(tag); }

inline
Bool isAnyVar(TaggedRef term) {
  GCDEBUG(term);
  Assert(!isRef(term));
  return _isAnyVar(term);
}

inline Bool isDirectVar(TypeOfTerm tag) { return _isDirectVar(tag); }

inline
Bool isDirectVar(TaggedRef term) {
  return _isDirectVar(term);
}

inline Bool isNotCVar(TypeOfTerm tag) { return _isNotCVar(tag);}

inline
Bool isNotCVar(TaggedRef term) {
  GCDEBUG(term);
  Assert(!isRef(term));
  return _isNotCVar(term);
}


#else

#define isAnyVar(term)    _isAnyVar(term)
#define isDirectVar(term) _isDirectVar(term)
#define isNotCVar(term)   _isNotCVar(term)
#define isUVar(term)      _isUVar(term)
#define isLTuple(term)    _isLTuple(term)

#endif



inline
Bool isFSetValue(TypeOfTerm tag) {
  return tag == FSETVALUE;
}

inline
Bool isFSetValue(TaggedRef term) {
  GCDEBUG(term);
  return isFSetValue(tagTypeOf(term));
}

inline
Bool isLiteral(TypeOfTerm tag) {
  return tag == LITERAL;
}

inline
Bool isLiteral(TaggedRef term) {
  GCDEBUG(term);
  return isLiteral(tagTypeOf(term));
}

inline
Bool isSRecord(TypeOfTerm tag) {
  return tag == SRECORD;
}

inline
Bool isSRecord(TaggedRef term) {
  GCDEBUG(term);
  return isSRecord(tagTypeOf(term));
}

inline
Bool isFloat(TypeOfTerm tag) {
  return (tag == OZFLOAT);
}

inline
Bool isFloat(TaggedRef term) {
  GCDEBUG(term);
  return isFloat(tagTypeOf(term));
}

inline
Bool isSmallInt(TypeOfTerm tag) {
  return (tag == SMALLINT);
}

inline
Bool isSmallInt(TaggedRef term) {
  return isSmallInt(tagTypeOf(term));
}

inline
Bool isBigInt(TypeOfTerm tag) {
  return (tag == BIGINT);
}

inline
Bool isBigInt(TaggedRef term) {
  GCDEBUG(term);
  return isBigInt(tagTypeOf(term));
}

inline
Bool isInt(TypeOfTerm tag) {
  return (isSmallInt(tag) || isBigInt(tag));
}

inline
Bool isInt(TaggedRef term) {
  GCDEBUG(term);
  return isInt(tagTypeOf(term));
}

inline
Bool isNumber(TypeOfTerm tag) {
  return (isInt(tag) || isFloat(tag));
}

inline
Bool isNumber(TaggedRef term) {
  GCDEBUG(term);
  return isNumber(tagTypeOf(term));
}

inline
Bool isConst(TypeOfTerm tag) {
  return (tag == OZCONST);
}

inline
Bool isConst(TaggedRef term) {
  GCDEBUG(term);
  return isConst(tagTypeOf(term));
}

// ---------------------------------------------------------------------------
// --- TaggedRef: create: makeTagged<Type>

// this function should be used, if tagged references are to be initialized
#ifdef DEBUG_CHECK
inline
TaggedRef makeTaggedNULL()
{
  return makeTaggedRef2p((TypeOfTerm)0,(void*)NULL);
}

inline
TaggedRef makeTaggedMiscp(void *s)
{
  return makeTaggedRef2p((TypeOfTerm)0,s);
}

inline
TaggedRef makeTaggedMisci(int32 s)
{
  return makeTaggedRef2i((TypeOfTerm)0,s);
}

inline
TaggedRef makeTaggedUVar(Board *s)
{
  CHECK_POINTER_N(s);
  return makeTaggedRef2p(UVAR,s);
}

inline
TaggedRef makeTaggedSVar(SVariable *s)
{
  CHECK_POINTER_N(s);
  return makeTaggedRef2p(SVAR,s);
}

inline
TaggedRef makeTaggedCVar(GenCVariable *s) {
  CHECK_POINTER_N(s);
  return makeTaggedRef2p(CVAR, s);
}

inline
TaggedRef makeTaggedFSetValue(OZ_FSetValue * s)
{
  CHECK_POINTER_N(s);
  return makeTaggedRef2p(FSETVALUE, s);
}

inline
TaggedRef makeTaggedLTuple(LTuple *s)
{
  CHECK_POINTER_N(s);
  return makeTaggedRef2p(LTUPLE,s);
}

inline
TaggedRef makeTaggedSRecord(SRecord *s)
{
  CHECK_POINTER_N(s);
  return makeTaggedRef2p(SRECORD,s);
}


inline
TaggedRef makeTaggedLiteral(Literal *s)
{
  CHECK_POINTER_N(s);
  return makeTaggedRef2p(LITERAL,s);
}

inline
TaggedRef makeTaggedSmallInt(int32 s)
{
#ifdef LARGEADRESSES
  /* small ints are the only TaggedRefs that do not
   * contain a pointer in the value part */
  return (s << tagSize) | SMALLINT;
#else
  return makeTaggedRef2p(SMALLINT,(void*)s);
#endif
}

inline
TaggedRef makeTaggedBigInt(BigInt *s)
{
  CHECK_POINTER_N(s);
  return makeTaggedRef2p(BIGINT,s);
}

inline
TaggedRef makeTaggedFloat(Float *s)
{
  CHECK_POINTER_N(s);
  return makeTaggedRef2p(OZFLOAT,s);
}


inline
TaggedRef makeTaggedConst(ConstTerm *s)
{
  CHECK_POINTER_N(s);
  return makeTaggedRef2p(OZCONST,s);
}

inline
TaggedRef makeTaggedTert(Tertiary *s)
{
  CHECK_POINTER_N(s);
  return makeTaggedRef2p(OZCONST,s);
}

#else

#define makeTaggedNULL()       ((TaggedRef) 0)
#define makeTaggedMiscp(s)     makeTaggedRef2p((TypeOfTerm)0,s)
#define makeTaggedMisci(s)     makeTaggedRef2i((TypeOfTerm)0,s)
#define makeTaggedUVar(s)      makeTaggedRef2p(UVAR,s)
#define makeTaggedSVar(s)      makeTaggedRef2p(SVAR,s)
#define makeTaggedCVar(s)      makeTaggedRef2p(CVAR,s)
#define makeTaggedFSetValue(s) makeTaggedRef2p(FSETVALUE,s)
#define makeTaggedLTuple(s)    makeTaggedRef2p(LTUPLE,s)
#define makeTaggedSRecord(s)   makeTaggedRef2p(SRECORD,s)
#define makeTaggedLiteral(s)   makeTaggedRef2p(LITERAL,s)
#define makeTaggedBigInt(s)    makeTaggedRef2p(BIGINT,s)
#define makeTaggedFloat(s)     makeTaggedRef2p(OZFLOAT,s)
#define makeTaggedConst(s)     makeTaggedRef2p(OZCONST,s)
#define makeTaggedTert(s)      makeTaggedRef2p(OZCONST,s)

#ifdef LARGEADRESSES
#define makeTaggedSmallInt(s) ((s << tagSize) | SMALLINT)
#else
#define makeTaggedSmallInt(s) makeTaggedRef2p(SMALLINT,(void*)s)
#endif


#endif


extern Literal *addToAtomTab(const char *str);
extern Literal *addToNameTab(const char *str);
inline
TaggedRef makeTaggedAtom(const char *s)
{
  CHECK_STRPTR(s);
  return makeTaggedLiteral(addToAtomTab(s));
}

// getArg() and the like may never return variables
inline
TaggedRef tagged2NonVariable(TaggedRef *term)
{
  GCDEBUG(*term);
  TaggedRef ret = *term;
#ifdef OPT_VAR_IN_STRUCTURE
  if (!IsRef(ret) && isAnyVar(ret)) {
    ret = makeTaggedRef(term);
  }
#else
  Assert(IsRef(ret) || !isAnyVar(ret));
#endif
  return ret;
}


// ---------------------------------------------------------------------------
// --- TaggedRef: allocate on heap, an return a ref to it

inline
TaggedRef *newTaggedSVar(SVariable *c)
{
  TaggedRef *ref = (TaggedRef *) int32Malloc(sizeof(TaggedRef));
  *ref = makeTaggedSVar(c);
  return ref;
}

inline
TaggedRef *newTaggedUVar(TaggedRef proto)
{
  TaggedRef *ref = (TaggedRef *) int32Malloc(sizeof(TaggedRef));
  *ref = proto;
  return ref;
}

inline
TaggedRef *newTaggedUVar(Board *c)
{
  return newTaggedUVar(makeTaggedUVar(c));
}

inline
TaggedRef *newTaggedCVar(GenCVariable *c) {
  TaggedRef *ref = (TaggedRef *) int32Malloc(sizeof(TaggedRef));
  *ref = makeTaggedCVar(c);
  return ref;
}


// ---------------------------------------------------------------------------
// --- TaggedRef: conversion: tagged2<Type>


inline
void *tagValueOf2(TypeOfTerm tag, TaggedRef ref) 
{
  GCDEBUG(ref);
#ifdef LARGEADRESSES
  return TaggedToPointer((ref>>(tagSize-2)) - (tag>>2));
#else
  return tagValueOf(ref);
#endif
}

#define _tagged2Ref(ref) ((TaggedRef *) ToPointer(ref))

#ifdef DEBUG_CHECK
inline
TaggedRef *tagged2Ref(TaggedRef ref)
{
  GCDEBUG(ref);
// cannot use CHECKTAG(REF); because only last two bits must be zero
  Assert((ref & 3) == 0);
  return _tagged2Ref(ref);
}
#else
/* macros are faster */
#define tagged2Ref(ref) _tagged2Ref(ref)
#endif

/* does not deref home pointer! */
inline
Board *tagged2VarHome(TaggedRef ref)
{
  GCDEBUG(ref);
  CHECKTAG(UVAR);
  return (Board *) tagValueOf2(UVAR,ref);
}

inline
OZ_FSetValue *tagged2FSetValue(TaggedRef ref)
{
  GCDEBUG(ref);
  CHECKTAG(FSETVALUE);
  return (OZ_FSetValue *) tagValueOf2(FSETVALUE,ref);
}

inline
SRecord *tagged2SRecord(TaggedRef ref)
{
  GCDEBUG(ref);
  CHECKTAG(SRECORD);
  return (SRecord *) tagValueOf2(SRECORD,ref);
}

inline
LTuple *tagged2LTuple(TaggedRef ref)
{
  GCDEBUG(ref);
  CHECKTAG(LTUPLE);
  return (LTuple *) tagValueOf2(LTUPLE,ref);
}

inline
Literal *tagged2Literal(TaggedRef ref)
{
  GCDEBUG(ref);
  CHECKTAG(LITERAL);
  return (Literal *) tagValueOf2(LITERAL,ref);
}

inline
Float *tagged2Float(TaggedRef ref)
{
  GCDEBUG(ref);
  CHECKTAG(OZFLOAT);
  return (Float *) tagValueOf2(OZFLOAT,ref);
}

inline
BigInt *tagged2BigInt(TaggedRef ref)
{
  GCDEBUG(ref);
  CHECKTAG(BIGINT);
  return (BigInt *) tagValueOf2(BIGINT,ref);
}


inline
ConstTerm *tagged2Const(TaggedRef ref)
{
  GCDEBUG(ref);
  CHECKTAG(OZCONST);
  return (ConstTerm *) tagValueOf2(OZCONST,ref);
}

inline
Tertiary *tagged2Tert(TaggedRef ref)
{
  GCDEBUG(ref);
  CHECKTAG(OZCONST);
  return (Tertiary*) tagValueOf2(OZCONST,ref);
}

inline
SVariable *tagged2SVar(TaggedRef ref)
{
  GCDEBUG(ref);
  CHECKTAG(SVAR);
  return (SVariable *) tagValueOf2(SVAR,ref);
}


inline
SVariable *tagged2SVarPlus(TaggedRef ref) {
  GCDEBUG(ref);
  Assert(isCVar(ref)||isSVar(ref));
  return (SVariable *) tagValueOf(ref);
}

inline
GenCVariable *tagged2CVar(TaggedRef ref) {
  GCDEBUG(ref);
  CHECKTAG(CVAR);
  return (GenCVariable *) tagValueOf2(CVAR,ref);
}


// ---------------------------------------------------------------------------
// --- TaggedRef: DEREF

// DEREF - Philosophy:
//   Arguments are never deref'ed
// especially: builtins cannot immediately access the tag of their arguments,
//   but must first call DEREF


// DEREF MACRO:
//  - declares 'termPtr','tag' as local variables
//  - destructively changes 'term'
//  - 'tag' is the tag of the deref'ed 'term'
//  - 'termPtr' is the pointer to the last REF-Cell in a chain of REF's
//    and is NULL, if no deref step was necessary (needed for destructive
//    changes of variables)

// Usage:
// void test(TaggedRef a) {
//   DEREF(a,ptr,tag);
//   if (isLiteral(tag)) { ... }
//   if (isAnyVar(tag) { *ptr = ... }
//   ....
// }


#define __DEREF(term, termPtr, tag)			\
  ProfileCode(ozstat.lenDeref=0);			\
  while(IsRef(term)) {					\
    COUNT(lenDeref);					\
    termPtr = tagged2Ref(term);				\
    term = *termPtr;					\
  }							\
  ProfileCode(ozstat.derefChain(ozstat.lenDeref));	\
  tag = tagTypeOf(term);

#define _DEREF(term, termPtr, tag)		\
  register TypeOfTerm tag;			\
   __DEREF(term, termPtr, tag);


#define DEREF(term, termPtr, tag)		\
  register TaggedRef *termPtr = NULL;		\
  _DEREF(term,termPtr,tag);

#define DEREF0(term, termPtr, tag)		\
  register TaggedRef *termPtr;			\
  _DEREF(term,termPtr,tag);

#define DEREFPTR(term, termPtr, tag)		\
  register TaggedRef term = *termPtr;		\
  _DEREF(term,termPtr,tag);


inline
TaggedRef deref(TaggedRef t) {
  DEREF(t,_1,_2);
  return t;
}

inline
TaggedRef deref(TaggedRef &tr, TaggedRef * &ptr, TypeOfTerm &tag)
{
  TaggedRef tr1=tr;
  DEREF(tr1,ptr1,tag1);
  tr=tr1;
  ptr=ptr1;
  tag=tag1;
  return tr1;
}

#define OZ_getCArgDeref(N, V, VPTR, VTAG) \
  OZ_Term V = OZ_getCArg(N); \
  DEREF(V, VPTR, VTAG);


// ---------------------------------------------------------------------------
// Binding
// ---------------------------------------------------------------------------


inline
TaggedRef *derefPtr(TaggedRef t) {
  DEREF(t,tPtr,_1);
  return tPtr;
}

inline
void doBind(TaggedRef *p, TaggedRef t)
{
  CHECK_NONVAR(t);
  Assert(p!=derefPtr(t));
  *p = t;
}

inline
void doBindCVar(TaggedRef *p, GenCVariable *cvar)
{
  *p = makeTaggedCVar(cvar);
}

inline
void doBindSVar(TaggedRef *p, SVariable *svar)
{
  *p = makeTaggedSVar(svar);
}

inline
void unBind(TaggedRef *p, TaggedRef t)
{
  Assert(isAnyVar(t));
  *p = t;
}


// ---------------------------------------------------------------------------
// ------- RefsArray ----------------------------------------------------------
// ---------------------------------------------------------------------------


// RefsArray is an array of TaggedRef
// a[-1] = LL...LLLTTTT,
//     L = length
//     T = tag (RADirty/RAFreed) or GCTAG during GC


typedef TaggedRef *RefsArray;

/* any combination of the following must not be equal to the GCTAG */
#ifdef DEBUG_CHECK
const int RAFreed = 1; // means has been already deallocated

inline
Bool isFreedRefsArray(RefsArray a)
{
  return (a && a[-1]&RAFreed);
}

inline
void markFreedRefsArray(RefsArray a)
{
  if (a) a[-1] |= RAFreed;
}

#define RAtagSize tagSize

#else

#define RAtagSize 0

#endif



inline
void setRefsArraySize(RefsArray a, int32 n)
{
  a[-1] = (n<<RAtagSize);
}

inline
int getRefsArraySize(RefsArray a)
{
  return (a[-1]>>RAtagSize);
}


inline
Bool initRefsArray(RefsArray a, int size, Bool init)
{
  setRefsArraySize(a,size);
  if (init) {
    switch (size) {
    case 10: a[9] = makeTaggedNULL();       
    case  9: a[8] = makeTaggedNULL();       
    case  8: a[7] = makeTaggedNULL();       
    case  7: a[6] = makeTaggedNULL();       
    case  6: a[5] = makeTaggedNULL();       
    case  5: a[4] = makeTaggedNULL();       
    case  4: a[3] = makeTaggedNULL();       
    case  3: a[2] = makeTaggedNULL();       
    case  2: a[1] = makeTaggedNULL();       
    case  1: a[0] = makeTaggedNULL();
      break;
    default:
      {
	for(int i = size-1; i >= 0; i--) 
	  a[i] = makeTaggedNULL();
      }
      break;
    }
  }

  return OK;  /* due to stupid CC */
}

inline
RefsArray allocateRefsArray(int n, Bool init=OK)
{
  Assert(n > 0);
  RefsArray a = ((RefsArray) freeListMalloc((n+1) * sizeof(TaggedRef)));
  a += 1;
  initRefsArray(a,n,init);
  return a;  
}

inline
RefsArray allocateRefsArray(int n, TaggedRef initRef)
{
  Assert(n > 0);
  RefsArray a = ((RefsArray) freeListMalloc((n+1) * sizeof(TaggedRef)));
  a += 1;
  // Initialize with given TaggedRef:
  setRefsArraySize(a,n);
  for(int i = n-1; i >= 0; i--) 
    a[i] = initRef;
  return a;  
}

inline
void disposeRefsArray(RefsArray a)
{
  if (a) {
    int sz = getRefsArraySize(a);
    a -= 1;
    freeListDispose(a, (sz+1) * sizeof(TaggedRef));
  }
}

inline
RefsArray allocateY(int n)
{
  COUNT(numEnvAllocs);

  int sz = (n+1) * sizeof(TaggedRef);
  COUNT1(sizeEnvs,sz);
  CountMax(maxEnvSize,sz);
  RefsArray a = (RefsArray) freeListMalloc(sz);
  a += 1;
  initRefsArray(a,n,OK);
  return a;  
}

inline
void deallocateY(RefsArray a, int sz)
{
  Assert(getRefsArraySize(a)==sz);
  Assert(!isFreedRefsArray(a));
#ifdef DEBUG_CHECK
  markFreedRefsArray(a);
#else
  freeListDispose(a-1,(sz+1) * sizeof(TaggedRef));
#endif
}

inline
void deallocateY(RefsArray a)
{
  deallocateY(a,getRefsArraySize(a));
}

inline
RefsArray allocateStaticRefsArray(int n) 
{
  RefsArray a = new TaggedRef[n + 1];
  a += 1;
  initRefsArray(a,n,OK);
  return a;
}


inline
RefsArray copyRefsArray(RefsArray a) 
{
  int n = getRefsArraySize(a);
  RefsArray r = allocateRefsArray(n,NO);
  for (int i = n-1; i >= 0; i--) {
    r[i] = tagged2NonVariable(&a[i]);
  }
  return r;
}

inline
RefsArray copyRefsArray(RefsArray a,int n,Bool init=NO) 
{
  RefsArray r = allocateRefsArray(n,init);
  for (int i = n-1; i >= 0; i--) {
    CHECK_NONVAR(a[i]);
    r[i] = a[i];
  }
  return r;
}


inline
RefsArray resize(RefsArray r, int s)
{
  int size = getRefsArraySize(r);
  if (s < size){
    setRefsArraySize(r,s);
    return r;
  }
  
  if (s > size){
    RefsArray aux = allocateRefsArray(s);
    for(int j = size-1; j >= 0; j--)
      aux[j] = r[j];
    return aux;
  }
  return r;
} // resize


// 
// identity test
//
inline
Bool termEq(TaggedRef t1, TaggedRef t2)
{
  DEREF(t1,t1Ptr,_1);
  DEREF(t2,t2Ptr,_2);
  if (isAnyVar(t1) || isAnyVar(t2)) {
    return t1Ptr==t2Ptr;
  }
  return t1==t2;
}

inline
OZ_Term mkTuple(int from, int to) {
  return OZ_pair2(OZ_int(from), OZ_int(to));
}


/*
 * using 32 bit for pointer + 2 tag bits
 */
class TaggedPtr {
  int32 tagged;
public:
  TaggedPtr(void *p,int t) {
    Assert(t >= 0 && t <=3)
    tagged = (ToInt32(p)<<2) || t;
  }
  void init()         { tagged = 0; }
  TaggedPtr()         { init(); }
  int *getRef()       { return &tagged; }
  int getType()       { return (tagged&3); }
  void setType(int t) { Assert(t >=0 && t <=3); tagged = (tagged&~3)|t; }
  int getIndex()      { return tagged>>2; }
  void *getPtr()      { return ToPointer(tagged&~3); }

  void setIndex(int i) {
    Assert(i>=0 && i < 1<<30);
    int oldtype = getType();
    tagged = i<<2;
    setType(oldtype);
  }
   
  void setPtr(void *p) {
    CHECK_POINTER(p);
    int oldtype = getType();
    tagged = ToInt32(p);
    setType(oldtype);
  }

};

inline 
int nextPowerOf2(int n)
{
  for(int i=2;; i*=2) {
    if (i>=n) return i;
  }
}

#define DerefIfVarDo(v,v1,Block)		\
 if (isRef(v)) {				\
   TaggedRef v1;				\
   while (1) {					\
     v1 = v;					\
     v = *tagged2Ref(v);			\
     if (!isRef(v)) break;			\
   }						\
   if (isAnyVar(v)) { Block; }			\
 }

#define DerefReturnVar(v)     DerefIfVarDo(v,_v,return _v);
#define DerefReturnSuspend(v) DerefIfVarDo(v,_v, return SUSPEND);

#endif
