/*
  PS Lab, DFKI, Saarbruecken
  Author: mehl

  Values: literal, list, records, ...
 */

#ifndef __VALUEHH
#define __VALUEHH

#ifdef INTERFACE
#pragma interface
#endif

#include "tagged.hh"

/*===================================================================
 * global names and atoms
 *=================================================================== */
extern TaggedRef AtomNil, AtomCons, AtomPair, AtomVoid,
       AtomSucceeded, AtomAlt, AtomMerged, AtomFailed,
       AtomEntailed, AtomSuspended, AtomBlocked,
       AtomEmpty, AtomUpper, AtomLower, AtomDigit,
       AtomCharSpace, AtomPunct, AtomOther,
       NameTrue, NameFalse, AtomBool, AtomSup, AtomCompl,
       AtomMin, AtomMax, AtomMid, AtomLow,
       AtomNaive, AtomSize, AtomNbSusps,
       AtomDebugCall, AtomDebugCond, AtomDebugHandler, AtomDebugLock,

       NameOoFreeFlag,NameOoAttr,NameOoFreeFeatR,NameOoUnFreeFeat,
       NameOoFastMeth,NameOoDefaults,NameOoRequiredArg,NameOoDefaultVar,
       NameOoPrintName,NameOoLocking,NameOoFallback,NameOoId,
       AtomNew, AtomSend, AtomApply,

       NameUnit,
       AtomKinded, AtomDet, AtomRecord, AtomFSet,
       // Atoms for System.get and System.set
       AtomActive, AtomAtoms, AtomBuiltins, AtomCommitted,
       AtomMoreInfo, AtomTotal, AtomCache,
       AtomCloned, AtomCode, AtomCopy, AtomCreated, AtomDebug, AtomDepth,
       AtomFeed, AtomForeign, AtomFree, AtomFreelist, AtomGC, AtomHigh,
       AtomHints, AtomIdle, AtomInt, AtomInvoked, AtomLimits, AtomLoad,
       AtomLocation, AtomMedium, AtomNames, AtomOn, AtomPropagate,
       AtomPropagators, AtomRun, AtomRunnable, AtomShowSuspension,
       AtomStopOnToplevelFailure, AtomSystem, AtomThread,
       AtomThreshold, AtomTolerance, AtomUser, AtomVariables, AtomWidth,
  AtomDetailed,
       AtomHeap, AtomDebugIP, AtomDebugPerdio,
  RecordFailure,
  E_ERROR, E_KERNEL, E_OBJECT, E_TK, E_OS, E_SYSTEM,
  BI_Unify,BI_Show,BI_send;


extern Board *ozx_rootBoard();

/*===================================================================
 * Literal
 *=================================================================== */


/* any combination of the following must be different from GCTAG,
 * otherwise getRef() will not work
 */
#define Lit_isName        2
#define Lit_isNamedName   4
#define Lit_hasGName      8
#define Lit_isUniqueName 16

const int sizeOfLitFlags = 5;
const int litFlagsMask   = (1<<sizeOfLitFlags)-1;

class Literal {
  int32 flagsAndOthers;
public:
  void init() { flagsAndOthers=0; }
  void setFlag(int flag) { flagsAndOthers |= flag; }
  int getFlags() { return (flagsAndOthers&litFlagsMask); }
  int getOthers() { return flagsAndOthers>>sizeOfLitFlags; }
  void setOthers(int value) { flagsAndOthers = getFlags()|(value<<sizeOfLitFlags); }

  Bool isName()       { return (getFlags()&Lit_isName); }
  Bool isNamedName()  { return (getFlags()&Lit_isNamedName); }
  Bool isUniqueName() { return (getFlags()&Lit_isUniqueName); }
  Bool isAtom()       { return !isName(); }

  Literal() { Assert(0); }

  const char *getPrintName();

  Literal *gc();

  TaggedRef *getRef() { return (TaggedRef*)&flagsAndOthers; }
  OZPRINT;
  OZPRINTLONG;

  inline unsigned int hash();
};

class Atom: public Literal {
private:
  const char *printName;
public:
  static Atom *newAtom(const char *str);
  const char* getPrintName() { return printName; }
  int getSize() { return getOthers(); }
  unsigned int hash() { return ToInt32(getPrintName()); } // == this!
};


/* This one goes onto the heap */
class Name: public Literal {
protected:
  static int NameCurrentNumber;
  int32 homeOrGName;
public:
  Name() { Assert(0); }
  static Name *newName(Board *b);

  Board *getBoardInternal() {
    return (hasGName() || isNamedName())
      ? ozx_rootBoard() : (Board*)ToPointer(homeOrGName);
  }

  int getSeqNumber() { return getOthers(); }
  unsigned int hash() { return getSeqNumber(); }

  Bool isOnHeap() { return (getFlags()&Lit_isNamedName)==0; }
  Bool hasGName() { return (getFlags()&Lit_hasGName); }

  Name *gcName();
  void gcRecurse();

  GName *getGName() {
    return hasGName() ? (GName*) ToPointer(homeOrGName) : globalize();
  }
  GName *globalize();
  void import(GName *);
};


/* This one managed via malloc */

class NamedName: public Name {
public:
  const char *printName;
  static NamedName *newNamedName(const char *str);
};


unsigned int Literal::hash()
{
  if (isAtom()) return ((Atom*)this)->hash();
  return ((Name*)this)->hash();
}

inline
Bool isAtom(TaggedRef term) {
  return isLiteral(tagTypeOf(term)) && tagged2Literal(term)->isAtom();
}


//mm2: one argument is guarantueed to be a literal ???
inline
Bool literalEq(TaggedRef a, TaggedRef b)
{
  Assert(isLiteral(a) || isLiteral(b));
  return (a==b);
}

/*
 * mm2: how should this function handle names ?
 *
 * atomcmp(a,b) is used to construct the arity lists of records.
 * It returns: 0   if   a == b
 *            -1   if   a < b
 *             1   if   a > b
 *
 * Note: if a and b are atoms (not names) than they MUST be compared via
 * strcmp, otherwise the compiler must be changed too!!!!!!
 *
 */

inline
int atomcmp(Literal *a, Literal *b)
{
  if (a==b) return 0;

  int res = strcmp(a->getPrintName(), b->getPrintName());
  if (res < 0) return -1;
  if (res > 0) return  1;

  Assert(a->isName() && b->isName());
  return (((Name*)a)->getSeqNumber() < ((Name*)b)->getSeqNumber()) ? -1 : 1;
}


inline
int atomcmp(TaggedRef a, TaggedRef b)
{
  if (a==b) return 0;

  return atomcmp(tagged2Literal(a), tagged2Literal(b));
}



/*===================================================================
 * Numbers
 *=================================================================== */

#include <math.h>
#include <limits.h>

extern "C" {
#include "../include/gmp.h"
}


/*===================================================================
 * SmallInt
 *=================================================================== */

/* ----------------------------------------------------------------------
   SmallInts represented as follows
   Gxx...xxTTTT

   G = GC bit, T = tag bits, x = value bits (left shifted value)
   ----------------------------------------------------------------------

   Search for string "INTDEP" to see procedures that depend on it !!

*/


#ifdef XGNUWIN32
#define INT_MAX    2147483647
#define INT_MIN    (-2147483647-1)
#endif

const int OzMaxInt = INT_MAX>>tagSize;
const int OzMinInt = -OzMaxInt;


inline
TaggedRef newSmallInt(int val)
{
  Assert(val >= OzMinInt && val <= OzMaxInt);

  return makeTaggedSmallInt((int32)val);
}


/* The C++ standard does not specify whether shifting right negative values
 * means shift logical or shift arithmetical. So we test what this C++ compiler
 * does.
 */
inline
int smallIntValue(TaggedRef t)
{
  int help = (int) t;

  if (1||(-1>>1) == -1) {  /* -1>>1  means SRA? */
    return (help>>tagSize);
  } else {
    return (help >= 0) ? help >> tagSize
                       : ~(~help >> tagSize);
  }
}

inline
Bool smallIntLess(TaggedRef A, TaggedRef B)
{
  Assert(isSmallInt(A) && isSmallInt(B));
  int ahelp = A;
  int bhelp = B;
  return ahelp < bhelp;
}

inline
Bool smallIntLE(TaggedRef A, TaggedRef B)
{
  Assert(isSmallInt(A) && isSmallInt(B));
  int ahelp = A;
  int bhelp = B;
  return ahelp <= bhelp;
}

inline
unsigned int smallIntHash(TaggedRef n)
{
  return (unsigned int) ToInt32(tagValueOf(n));
}


inline
Bool smallIntEq(TaggedRef a, TaggedRef b)
{
  Assert(isSmallInt(a) || isSmallInt(b));
  return (a == b);
}

inline
Bool smallIntCmp(TaggedRef a, TaggedRef b)
{
  Assert(isSmallInt(a) && isSmallInt(b));
  return smallIntLess(a,b) ? -1 : (smallIntEq(a,b) ? 0 : 1);
}

/*===================================================================
 * Float
 *=================================================================== */

class Float {
protected:
  double value;

public:
  Float() { error("use newFloat");; };
  static Float *newFloat(double val);
  double getValue() { return value; }
  OZPRINT;
  OZPRINTLONG;
  unsigned int hash() { return (unsigned int) value; }

  Float *gc();
};

inline
Float *Float::newFloat(double val)
{
  Float *ret = (Float *) alignedMalloc(sizeof(Float),sizeof(double));
  ret->value = val;
  return ret;
}

inline
double floatValue(TaggedRef t)
{
  return tagged2Float(t)->getValue();
}

inline
Bool floatEq(TaggedRef a, TaggedRef b)
{
  return (tagged2Float(a)->getValue() == tagged2Float(b)->getValue());
}

inline
TaggedRef newTaggedFloat(double i)
{
  return makeTaggedFloat(Float::newFloat(i));
}

/*===================================================================
 * BigInt
 *=================================================================== */

class BigInt {
public:
  USEFREELISTMEMORY;

private:
  MP_INT value;

public:
  BigInt() {
    mpz_init(&value);
  }

  BigInt(int i)          { mpz_init_set_si(&value,i); }
  BigInt(unsigned int i) { mpz_init_set_ui(&value,i); }

  BigInt(char *s) {
    if(mpz_init_set_str(&value, s, 10)) {
      Assert(0);
    }
  }
  BigInt(MP_INT *i) {
    mpz_init_set(&value, i);
  }
  void dispose()
  {
    mpz_clear(&value);
    freeListDispose(this,sizeof(BigInt));
  }
/* make a small int if <Big> fits into it, else return big int */
  TaggedRef shrink() {
    TaggedRef ret;
    if (mpz_cmp_si(&value,OzMaxInt) > 0 ||
        mpz_cmp_si(&value,OzMinInt) < 0)
      ret = makeTaggedBigInt(this);
    else {
      ret =  newSmallInt((int) mpz_get_si(&value));
      dispose();
    }
    return ret;
  }

  /* make an 'int' if <Big> fits into it, else return INT_MAX,INT_MIN */
  int getInt()
  {
    if (mpz_cmp_si(&value,INT_MAX) > 0) {
      return INT_MAX;
    } else if (mpz_cmp_si(&value,INT_MIN) < 0) {
      return INT_MIN;
    } else {
      return mpz_get_si(&value);
    }
  }


  Bool equal(BigInt *b) {
    return (mpz_cmp(&value, &b->value) == 0);
  }

#define MKOP(op,mpop)                                                         \
  TaggedRef op(BigInt *b) {                                                   \
    BigInt *n = new BigInt();                                                 \
    mpop(&n->value,&value,&b->value);                                         \
    return n->shrink();                                                       \
  }
  MKOP(div,mpz_div);
  MKOP(mod,mpz_mod);
  MKOP(mul,mpz_mul);
  MKOP(sub,mpz_sub);
  MKOP(add,mpz_add);
#undef MKOP

  TaggedRef neg() {
    BigInt *n = new BigInt();
    mpz_neg(&n->value,&value);
    return n->shrink();
  }
  int cmp(BigInt *b)      { return mpz_cmp(&value,&b->value); }
  int cmp(long int i)     { return mpz_cmp_si(&value,i); }
  Bool less(BigInt *b)    { return cmp(b) < 0; }
  Bool leq(BigInt *b)     { return cmp(b) <= 0; }
  int stringLength()      { return mpz_sizeinbase(&value,10)+2; }
  void getString(char *s) { mpz_get_str(s,10,&value); }
  OZPRINTLONG;
  unsigned int hash() { return 75; } // all BigInt hash to same value
  BigInt *gc();
};

inline
TaggedRef makeInt(int i)
{
  if (i > OzMaxInt || i < OzMinInt)
    return makeTaggedBigInt(new BigInt(i));
  else
    return newSmallInt(i);
}

inline
TaggedRef makeUnsignedInt(unsigned int i)
{
  if (i > (unsigned int) OzMaxInt)
    return makeTaggedBigInt(new BigInt(i));
  else
    return newSmallInt(i);
}

inline
Bool bigIntEq(TaggedRef a, TaggedRef b)
{
  return tagged2BigInt(a)->equal(tagged2BigInt(b));
}

inline
Bool numberEq(TaggedRef a, TaggedRef b)
{
  Assert(tagTypeOf(a)==tagTypeOf(b));

  TypeOfTerm tag = tagTypeOf(a);
  switch(tag) {
  case OZFLOAT:  return floatEq(a,b);
  case SMALLINT: return smallIntEq(a,b);
  case BIGINT:   return bigIntEq(a,b);
  default:       return NO;
  }
}

#define CHECK_LITERAL(lab) \
Assert(!isRef(lab) && !isAnyVar(lab) && isLiteral(lab));


/*===================================================================
 * LTuple
 *=================================================================== */

class LTuple {
private:
  TaggedRef args[2];

public:
  USEHEAPMEMORY32;

  LTuple(void) {
    COUNT1(sizeLists,sizeof(LTuple));
  }
  LTuple(TaggedRef head, TaggedRef tail) {
    COUNT1(sizeLists,sizeof(LTuple));
    args[0] = head; args[1] = tail;
  }

  OZPRINT;
  OZPRINTLONG;
  void gcRecurse();
  LTuple *gc();

  TaggedRef getHead()          { return tagged2NonVariable(args); }
  TaggedRef getTail()          { return tagged2NonVariable(args+1); }
  void setHead(TaggedRef term) { args[0] = term;}
  void setTail(TaggedRef term) { args[1] = term;}
  TaggedRef *getRef()          { return args; }
  TaggedRef *getRefTail()      { return args+1; }
  TaggedRef *getRefHead()      { return args; }
};

inline
Bool isCons(TaggedRef term) {
  return isLTuple(term);
}

inline
Bool isNil(TaggedRef term) {
  return literalEq(term,AtomNil);
}

inline
TaggedRef nil() { return AtomNil; }

inline
TaggedRef cons(TaggedRef head, TaggedRef tail)
{
  return makeTaggedLTuple(new LTuple(head,tail));
}

inline
TaggedRef cons(Literal *head, TaggedRef tail)
{
  return cons(makeTaggedLiteral(head),tail);
}

inline
TaggedRef cons(char *head, TaggedRef tail)
{
  return cons(makeTaggedAtom(head),tail);
}

inline
TaggedRef head(TaggedRef list)
{
  Assert(isLTuple(list));
  return tagged2LTuple(list)->getHead();
}

inline
TaggedRef tail(TaggedRef list)
{
  Assert(isLTuple(list));
  return tagged2LTuple(list)->getTail();
}

inline
int length(OZ_Term l)
{
  int len = 0;
  l = deref(l);
  while (isCons(l)) {
    len++;
    l = deref(tail(l));
  }
  return len;
}


/*===================================================================
 * ConstTerm
 *=================================================================== */

/* must not match GCTAG (ie <> 13 (1101) !!!! */
enum TypeOfConst {
  Co_UNUSED1,
  Co_UNUSED2,
  Co_Thread,
  Co_Abstraction,

  Co_Builtin,       /* 4 */
  Co_Cell,
  Co_Space,

  /* chunks must stay together and the first one
   * must be Co_Object
   * otherwise you'll have to change the "isChunk" test
   * NOTE: update the builtins: subtree and chunkArity !
   */
  Co_Object,
  Co_Port,
  Co_Chunk,
  Co_HeapChunk,
  Co_Array,
  Co_Dictionary,    /* 12 */
  Dummy = GCTAG,
  Co_Lock,
  Co_Class
};

enum TertType {
  Te_Local   = 0, // 0000
  Te_Manager = 1, // 0001
  Te_Proxy   = 2, // 0010
  Te_Frame   = 3  // 0011
};

#define DebugIndexCheck(IND) {Assert(IND< (1<<27));Assert(IND>=0);}

class ConstTerm {
protected:
  union {
    TaggedRef tagged;
    void *padForAlpha;   // otherwise we get lots of warnings
  } ctu;
public:
  /* where do we store forward reference */
  int32 *getGCField() { return (int32*) &ctu.tagged; }
public:
  USEHEAPMEMORY;

  ConstTerm *gcConstTerm(void);
  ConstTerm *gcConstTermSpec(void);
  void gcConstRecurse(void);

  void setTagged(TypeOfConst t, void *p) {
    ctu.tagged = makeTaggedRef2p((TypeOfTerm)t,p);
  }
  ConstTerm(TypeOfConst t)  { setTagged(t,NULL); }
  TypeOfConst getType()     { return (TypeOfConst) tagTypeOf(ctu.tagged); }

  const char *getPrintName();
  int getArity();
  void *getPtr() {
    return isNullPtr(ctu.tagged) ? NULL : tagValueOf(ctu.tagged);
  }
  void setPtr(void *p)  { setTagged(getType(),p); }
  TaggedRef *getRef()   { return &ctu.tagged; }

  OZPRINT;
  OZPRINTLONG;

  /* optimized isChunk test */
  Bool isChunk() { return (int) getType() >= (int) Co_Object; }
};


#define CWH_Board 0
#define CWH_GName 1


class ConstTermWithHome: public ConstTerm {
private:
  TaggedPtr boardOrGName;
  void setBoard(Board *b)
  {
    boardOrGName.setPtr(b);
    boardOrGName.setType(CWH_Board);
  }
public:
  ConstTermWithHome(Board *b, TypeOfConst t) : ConstTerm(t) { setBoard(b);  }

  Board *getBoardInternal() {
    return hasGName() ? ozx_rootBoard() : (Board*)boardOrGName.getPtr();
  }

  void gcConstTermWithHome();
  void setGName(GName *gn) {
    Assert(gn);
    boardOrGName.setPtr(gn);
    boardOrGName.setType(CWH_GName);
  }
  Bool hasGName() { return (boardOrGName.getType()&CWH_GName); }
  GName *getGName1() { return hasGName()?(GName *)boardOrGName.getPtr():(GName *)NULL; }
};


class Tertiary: public ConstTerm {
  TaggedPtr tagged;
public:

  TertType getTertType()       { return (TertType) tagged.getType(); }
  void setTertType(TertType t) { tagged.setType((int) t); }

  Tertiary(Board *b, TypeOfConst s,TertType t) : ConstTerm(s) {
    setTertType(t);
    setBoard(b);}
  Tertiary(int i, TypeOfConst s,TertType t) : ConstTerm(s)
  {
    setTertType(t);
    setIndex(i);
  }

  void setIndex(int i) { tagged.setIndex(i); }
  int getIndex() { return tagged.getIndex(); }
  void setPointer (void *p) { tagged.setPtr(p); }
  void *getPointer()        { return tagged.getPtr(); }

  Bool checkTertiary(TypeOfConst s,TertType t){
    return (s==getType() && t==getTertType());}

  Board *getBoardInternal() {
    return isLocal() ? (Board*)getPointer() : ozx_rootBoard();
  }
  void setBoard(Board *b);

  Bool isLocal()   { return (getTertType() == Te_Local); }
  Bool isManager() { return (getTertType() == Te_Manager); }
  Bool isProxy()   { return (getTertType() == Te_Proxy); }

  void globalizeTert();
  void localize();

  void gcProxy();
  void gcManager();
  void gcTertiary();
  void gcBorrowMark();
};



/*===================================================================
 * HeapChunk
 *=================================================================== */

class HeapChunk: public ConstTerm {
private:
  size_t chunk_size;
  void * chunk_data;
  void * copyChunkData(void) {
    char * data = (char *) allocate(chunk_size);
    for (int i = chunk_size; i--; )
      data[i] = ((char *) chunk_data)[i];
    return (void *) data;
  }
  void * allocate(int size) {
    COUNT1(sizeHeapChunks,size);
    return (void *) alignedMalloc(size, sizeof(double));
  }
public:
  HeapChunk(HeapChunk&);
  HeapChunk(int size)
  : ConstTerm(Co_HeapChunk), chunk_size(size), chunk_data(allocate(size))
  {
    COUNT1(sizeHeapChunks,sizeof(HeapChunk));
  }

  size_t getChunkSize(void) { return chunk_size; }

  void * getChunkData(void) { return chunk_data; }

  OZPRINT;
  OZPRINTLONG;

  HeapChunk * gc(void);
};

/*===================================================================
 * SRecord: incl. Arity, ArityTable
 *=================================================================== */


inline
Bool isFeature(TaggedRef lab) { return isLiteral(lab) || isInt(lab); }

#define CHECK_FEATURE(lab) \
Assert(!isRef(lab) && !isAnyVar(lab) && isFeature(lab));


int featureEqOutline(TaggedRef a, TaggedRef b);

inline
int featureEq(TaggedRef a,TaggedRef b)
{
  CHECK_FEATURE(a);
  CHECK_FEATURE(b);
  return a == b || featureEqOutline(a,b);
}

/*
 * for sorting the arity one needs to have a total order
 *
 * SMALLINT < BIGINT < LITERAL
 * return 0: if equal
 *       -1: a<b
 *        1: a>b
 */
inline
int featureCmp(TaggedRef a,TaggedRef b)
{
  CHECK_FEATURE(a);
  CHECK_FEATURE(b);
  TypeOfTerm tagA = tagTypeOf(a);
  TypeOfTerm tagB = tagTypeOf(b);
  if (tagA != tagB) {
    if (tagA==SMALLINT) return -1;
    if (tagA==BIGINT) {
      if (tagB==SMALLINT) return 1;
      Assert(tagB==LITERAL);
      return -1;
    }
    Assert(tagA==LITERAL);
    return 1;
  }
  switch (tagA) {
  case LITERAL:
    return atomcmp(tagged2Literal(a),tagged2Literal(b));
  case SMALLINT:
    return smallIntCmp(a,b);
  case BIGINT:
    return tagged2BigInt(a)->cmp(tagged2BigInt(b));
  default:
    error("featureCmp");
    return 0;
  }
}


/*
 * Hash function for Features:
 * NOTE: all bigints are hashed to the same value
 */
inline
unsigned int featureHash(TaggedRef a)
{
  CHECK_FEATURE(a);
  TypeOfTerm tag = tagTypeOf(a);
  switch (tag) {
  case LITERAL:
    return tagged2Literal(a)->hash();
  case SMALLINT:
    return (unsigned int) a;
  case BIGINT:
    return 75;
  default:
    error("featureHash");
    return 0;
  }
}

class Arity {
friend class ArityTable;
private:
  Arity ( TaggedRef, Bool );

  void gc();

  TaggedRef list;
  Arity *next;

  int size;             // size is always a power of 2
  int hashmask;         // size-1, used as mask for hashing and opt marker
  int width;            // next unused index in RefsArray (for add())
  DebugCheckT(int numberofentries;)
  DebugCheckT(int numberofcollisions;)
  Bool isTupleFlag;

  TaggedRef *keytable;
  int *indextable;
  int scndhash(TaggedRef a) { return ((featureHash(a)&7)<<1)|1; }
  int hashfold(int i) { return(i&hashmask); }

public:
  Bool isTuple() {
    return isTupleFlag;
  }

  // use SRecord::getIndex instead of this!
  int lookupInternal(TaggedRef entry);   // return -1, if argument not found.

  TaggedRef getList() { return list; }
  int getWidth() { return width; }

  OZPRINT;
};

Arity *mkArity(TaggedRef list);

#define ARITYTABLESIZE 8000

class ArityTable {
friend class Arity;
public:
  ArityTable ( unsigned int );
  Arity *find ( TaggedRef);
  void gc();
private:
  OZPRINT;

  Bool hashvalue( TaggedRef, unsigned int & );
  Arity* *table;

  int size;
  int hashmask;
  int hashfold(int i) { return(i&hashmask); }
};

extern ArityTable aritytable;

TaggedRef makeTupleArityList(int i);


/*
 * Abstract data type SRecordArity for records and tuples:
 * either an Arity* or an int
 */

// #define TYPECHECK_SRecordArity
#ifdef TYPECHECK_SRecordArity

class SRA;
typedef SRA* SRecordArity;
#define Body(X) ;
#define inline

#else

typedef int32 SRecordArity; /* do not want to use a pointer on the Alpha! */
#define Body(X) X

#endif

inline Bool sraIsTuple(SRecordArity a)
     Body({ return a&1; })

inline SRecordArity mkTupleWidth(int w)
     Body({ return (SRecordArity) ((w<<1)|1);})

inline int getTupleWidth(SRecordArity a)
     Body({ return a>>1; })

inline SRecordArity mkRecordArity(Arity *a)
     Body({ Assert(!a->isTuple()); return ToInt32(a); })

inline Arity *getRecordArity(SRecordArity a)
     Body({ return (Arity*) ToPointer(a); })

inline Bool sameSRecordArity(SRecordArity a, SRecordArity b)
     Body({ return a==b; })
inline int getWidth(SRecordArity a)
     Body({
       return sraIsTuple(a) ? getTupleWidth(a) : getRecordArity(a)->getWidth();
     })

#undef Body

inline
OZ_Term sraGetArityList(SRecordArity arity)
{
  return (sraIsTuple(arity))
    ? makeTupleArityList(getTupleWidth(arity))
    : getRecordArity(arity)->getList();
}

class SRecord {
private:
  TaggedRef label;
  SRecordArity recordArity;
  TaggedRef args[1];   // really maybe more

public:
  USEHEAPMEMORY;

  SRecord *gcSRecord();

  SRecord() { error("do not use SRecord"); }

  Bool isTuple() { return sraIsTuple(recordArity); }

  void setTupleWidth(int w) { recordArity = mkTupleWidth(w); }
  int getTupleWidth() {
    Assert(isTuple());
    return ::getTupleWidth(recordArity);
  }

  SRecordArity getSRecordArity() { return recordArity; }

  void setRecordArity(Arity *a) { recordArity = mkRecordArity(a);}
  Arity *getRecordArity() {
    Assert(!isTuple());
    return ::getRecordArity(recordArity);
  }

  TaggedRef normalize()
  {
    if (isTuple() && label == AtomCons && getWidth()==2) {
      return makeTaggedLTuple(new LTuple(getArg(0),getArg(1)));
    }
    return makeTaggedSRecord(this);
  }

  void initArgs();

  int getWidth() { return ::getWidth(getSRecordArity()); }

  void downSize(unsigned int s) { // TMUELLER
    Assert(isTuple());
    setTupleWidth(s);
  } // FD

  static SRecord *newSRecord(TaggedRef lab, SRecordArity arity, int width)
  {
    CHECK_LITERAL(lab);
    Assert(width > 0);
    int memSize = sizeof(SRecord) + sizeof(TaggedRef) * (width - 1);
    COUNT1(sizeRecords,memSize);
    SRecord *ret = (SRecord *) int32Malloc(memSize);
    ret->label = lab;
    ret->recordArity = arity;
    return ret;
  }

  int sizeOf()
  {
    return sizeof(SRecord) + sizeof(TaggedRef) * (getWidth() - 1);
  }

  static SRecord *newSRecord(TaggedRef lab, int i)
  {
    return newSRecord(lab,mkTupleWidth(i),i);
  }

  static SRecord *newSRecord(TaggedRef lab, Arity *arity)
  {
    if (arity->isTuple())
      return newSRecord(lab,arity->getWidth());

    return newSRecord(lab,mkRecordArity(arity),arity->getWidth());
  }

  // returns copy of tuple st (args are appropriately copied as well)
  static SRecord * newSRecord(SRecord * st)
  {
    SRecord *ret = newSRecord(st->label, st->getSRecordArity(),st->getWidth());
    for (int i = st->getWidth(); i--; ) {
      ret->args[i] = tagged2NonVariable((st->args)+i);
    }
    return ret;
  }

  TaggedRef getArg(int i) { return tagged2NonVariable(args+i); }
  void setArg(int i, TaggedRef t) { args[i] = t; }
  TaggedRef *getRef() { return args; }
  TaggedRef *getRef(int i) { return args+i; }
  TaggedRef &operator [] (int i) {return args[i];}

  void setFeatures(TaggedRef proplist);

  TaggedRef getLabel() { return label; }
  void setLabelInternal(TaggedRef l) { label=l; }
  Literal *getLabelLiteral() { return tagged2Literal(label); }
  void setLabelForAdjoinOpt(TaggedRef newLabel) { label = newLabel; }

  TaggedRef getArityList() {
    return sraGetArityList(getSRecordArity());
  }

  Arity* getArity () {
    return isTuple() ? aritytable.find(getArityList()) : getRecordArity();
  }

  int getIndex(TaggedRef feature)
  {
    CHECK_FEATURE(feature);
    if (isTuple()) {
      if (!isSmallInt(feature)) return -1;
      int f=smallIntValue(feature);
      return (1 <= f && f <= getWidth()) ? f-1 : -1;
    }
    return getRecordArity()->lookupInternal(feature);
  }

  Bool hasFeature(TaggedRef feature) { return getIndex(feature) >= 0; }
  TaggedRef getFeature(TaggedRef feature)
  {
    int i = getIndex(feature);
    return i < 0 ? makeTaggedNULL() : getArg(i);
  }

  Bool setFeature(TaggedRef feature,TaggedRef value);
  TaggedRef replaceFeature(TaggedRef feature,TaggedRef value);

  void gcRecurse();

  OZPRINT;
  OZPRINTLONG;

  Bool compareSortAndArity(TaggedRef lbl, SRecordArity arity) {
    return literalEq(getLabel(),lbl) &&
           sameSRecordArity(getSRecordArity(),arity);
  }

  Bool compareFunctor(SRecord* str) {
    return compareSortAndArity(str->getLabel(),str->getSRecordArity());
  }

  TaggedRef *getCycleAddr() { return &label; }
};

TaggedRef oz_adjoinAt(SRecord *, TaggedRef feature, TaggedRef value);
TaggedRef oz_adjoin(SRecord *, SRecord *);
TaggedRef oz_adjoinList(SRecord *, TaggedRef arity, TaggedRef proplist);

Bool isSorted(TaggedRef list);

TaggedRef duplist(TaggedRef list, int &len);
TaggedRef sortlist(TaggedRef list,int len);

inline
Bool isRecord(TaggedRef term) {
  GCDEBUG(term);
  TypeOfTerm tag = tagTypeOf(term);
  return isSRecord(tag) || isLTuple(tag) || isLiteral(tag);
}


SRecord *makeRecord(TaggedRef t);

inline
int isSTuple(TaggedRef term) {
  return isSRecord(term) && tagged2SRecord(term)->isTuple();
}

inline
int isTuple(TaggedRef term) {
  return isLTuple(term) || isSTuple(term) || isLiteral(term);
}

inline
OZ_Term getArityList(OZ_Term term)
{
  if (isSRecord(term)) {
    return tagged2SRecord(term)->getArityList();
  }
  if (isLTuple(term)) {
    return makeTupleArityList(2);
  }
  if (isLiteral(term)) {
    return nil();
  }
  return 0;
}

inline
int getWidth(OZ_Term term)
{
  if (isSRecord(term)) {
    return tagged2SRecord(term)->getWidth();
  }
  if (isLTuple(term)) {
    return (2);
  }
  if (isLiteral(term)) {
    return (0);
  }
  return (0);                   // ???
}


/*===================================================================
 * ObjectClass
 *=================================================================== */

/* Internal representation of Oz classes */

class ObjectClass: public ConstTermWithHome {
  friend void ConstTerm::gcConstRecurse(void);
private:
  SRecord *features;
  SRecord *unfreeFeatures;
  OzDictionary *fastMethods;
  OzDictionary *defaultMethods;
  Bool locking;
public:
  USEHEAPMEMORY;

  ObjectClass(SRecord *feat,OzDictionary *fm,SRecord *uf,OzDictionary *dm,
              Bool lck, Board *b)
    : ConstTermWithHome(b,Co_Class)
  {
    features       = feat;
    fastMethods    = fm;
    unfreeFeatures = uf;
    defaultMethods = dm;
    locking        = lck;
  }

  Bool supportsLocking() { return locking; }

  OzDictionary *getDefMethods()  { return defaultMethods; }
  OzDictionary *getfastMethods() { return fastMethods; }

  Abstraction *getMethod(TaggedRef label, SRecordArity arity, RefsArray X,
                         Bool &defaultsUsed);

  TaggedRef getFallbackNew();
  TaggedRef getFallbackSend();
  TaggedRef getFallbackApply();

  Bool lookupDefault(TaggedRef label, SRecordArity arity, RefsArray X);

  TaggedRef classGetFeature(TaggedRef lit) { return features->getFeature(lit); }

  SRecord *getUnfreeRecord() { return unfreeFeatures; }
  SRecord *getFeatures()     { return features; }

  const char *getPrintName();

  ObjectClass *gcClass();

  void import(SRecord *feat,OzDictionary *fm, SRecord *uf, OzDictionary *dm, Bool l)
  {
    features       = feat;
    fastMethods    = fm;
    unfreeFeatures = uf;
    defaultMethods = dm;
    locking        = l;
  }

  TaggedRef getArityList();
  int getWidth();

  GName *getGName() {
    GName *gn = getGName1();
    Assert(gn);
    return gn;
  }
  void globalize();

  OZPRINT;
  OZPRINTLONG;
};


/*
 * Object
 */


typedef int32 RecOrCell;

inline
Bool stateIsCell(RecOrCell rc)     { return rc&1; }

inline
Tertiary *getCell(RecOrCell rc)   {
  Assert(stateIsCell(rc)); return (Tertiary*) ToPointer(rc-1);
}

inline
SRecord *getRecord(RecOrCell rc) {
  Assert(!stateIsCell(rc)); return (SRecord*) ToPointer(rc);
}

inline
RecOrCell makeRecCell(Tertiary *c)    { return ToInt32(c)|1; }

inline
RecOrCell makeRecCell(SRecord *r) { return ToInt32(r); }


class Object: public Tertiary {
  friend void ConstTerm::gcConstRecurse(void);
protected:
  // ObjectClass *cl is in ptr field of ConstTerm
  RecOrCell state;
  OzLock *lock;
  SRecord *freeFeatures;
public:
  Object();
  ~Object();
  Object(Object&);

  Object(Board *bb,SRecord *s,ObjectClass *ac,SRecord *feat, OzLock *lck):
    Tertiary(bb, Co_Object,Te_Local)
  {
    setFreeRecord(feat);
    setClass(ac);
    setState(s);
    lock = lck;
  }

  Object(int i): Tertiary(i,Co_Object,Te_Proxy)
  {
    setFreeRecord(NULL);
    setClass(NULL);
    setState((Tertiary*)NULL);
    lock = 0;
  }

  void setClass(ObjectClass *c) { setPtr(c); }

  OzLock *getLock() { return lock; }
  void setLock(OzLock *l) { lock=l; }

  ObjectClass *getClass()       { return (ObjectClass*) getPtr(); }
  OzDictionary *getMethods()    { return getClass()->getfastMethods(); }
  const char *getPrintName()    { return getClass()->getPrintName(); }
  RecOrCell getState()          { return state; }
  void setState(SRecord *s)     { Assert(s!=0); state=makeRecCell(s); }
  void setState(Tertiary *c)    { state = makeRecCell(c); }
  OzDictionary *getDefMethods() { return getClass()->getDefMethods(); }

  SRecord *getFreeRecord()          { return freeFeatures; }
  SRecord *getUnfreeRecord() { return getClass()->getUnfreeRecord(); }
  void setFreeRecord(SRecord *aRec) { freeFeatures = aRec; }

  /* same functionality is also in instruction inlineDot */
  TaggedRef getFeature(TaggedRef lit)
  {
    SRecord *freefeat = getFreeRecord();
    if (freefeat) {
      TaggedRef ret = freefeat->getFeature(lit);
      if (ret!=makeTaggedNULL())
        return ret;
    }
    SRecord *fr = getUnfreeRecord();
    return fr ?  fr->getFeature(lit) : makeTaggedNULL();
  }

  TaggedRef replaceFeature(TaggedRef lit, TaggedRef value) {
    SRecord *freefeat = getFreeRecord();
    if (freefeat) {
      int ind = freefeat->getIndex(lit);
      if (ind != -1) {
        TaggedRef ret = freefeat->getArg(ind);
        freefeat->setArg(ind, value);
        return ret;
      }
    }
    SRecord *fr = getUnfreeRecord();

    if (!fr)
      return makeTaggedNULL();

    int ind = fr->getIndex(lit);

    if (ind == -1)
      return makeTaggedNULL();

    TaggedRef ret = fr->getArg(ind);
    fr->setArg(ind, value);

    return ret;
  }

  TaggedRef getArityList();
  int getWidth ();

  Object *gcObject();

  OZPRINT;
  OZPRINTLONG;
};

SRecord *getState(RecOrCell state, Bool isAssign, OZ_Term fea, OZ_Term &val);

inline
Bool isObject(ConstTerm *t)
{
  return (t->getType()==Co_Object);
}

inline
Bool isObject(TaggedRef term)
{
  return isConst(term) && isObject(tagged2Const(term));
}

inline
Object *tagged2Object(TaggedRef term)
{
  Assert(isObject(term));
  return (Object *)tagged2Const(term);
}

inline
Bool isObjectClass(ConstTerm *t)
{
  return (t->getType()==Co_Class);
}

inline
Bool isObjectClass(TaggedRef term)
{
  return isConst(term) && isObjectClass(tagged2Const(term));
}

inline
ObjectClass *tagged2ObjectClass(TaggedRef term)
{
  Assert(isObjectClass(term));
  return (ObjectClass *)tagged2Const(term);
}

inline
Bool isClass(TaggedRef term)
{
  return isObjectClass(term);
}

/*===================================================================
 * SChunk
 *=================================================================== */

class SChunk: public ConstTermWithHome {
friend void ConstTerm::gcConstRecurse(void);
private:
  TaggedRef value;
public:
  SChunk(Board *b,TaggedRef v)
    : ConstTermWithHome(b,Co_Chunk), value(v)
  {
    Assert(v==0||isRecord(v));
    Assert(b);
  };

  OZPRINT;
  OZPRINTLONG;

  TaggedRef getValue() { return value; }
  TaggedRef getFeature(TaggedRef fea) { return OZ_subtree(value,fea); }
  TaggedRef getArityList() { return ::getArityList(value); }
  int getWidth () { return ::getWidth(value); }

  void import(TaggedRef val) {
    Assert(!value);
    Assert(isRecord(val));
    Assert(getGName1());
    value=val;
  }

  GName *globalize();
  GName *getGName() {
    GName *gn = getGName1();
    return gn ? gn : globalize();
  }
};


inline
Bool isSChunk(ConstTerm *t)
{
  return t->getType() == Co_Chunk;
}

inline
Bool isSChunk(TaggedRef term)
{
  return isConst(term) && isSChunk(tagged2Const(term));
}

inline
SChunk *tagged2SChunk(TaggedRef term)
{
  Assert(isSChunk(term));
  return (SChunk *) tagged2Const(term);
}


inline
Bool isChunk(TaggedRef t)
{
  return isConst(t) && tagged2Const(t)->isChunk();
}

/*===================================================================
 * Arrays
 *=================================================================== */

class OzArray: public ConstTermWithHome {
  friend void ConstTerm::gcConstRecurse(void);
private:
  int offset, width;

  TaggedRef *getArgs() { return (TaggedRef*) getPtr(); }

public:

  OzArray(Board *b, int low, int high, TaggedRef initvalue) : ConstTermWithHome(b,Co_Array)
  {
    Assert(isRef(initvalue) || !isAnyVar(initvalue));

    offset = low;
    width = high-low+1;
    if (width <= 0) {
      width = 0;
      setPtr(NULL); // mm2: attention if globalize gname!
    } else {
      TaggedRef *args = (TaggedRef*) int32Malloc(sizeof(TaggedRef)*width);
      for(int i=0; i<width; i++) {
        args[i] = initvalue;
      }
      setPtr(args);
    }
  }

  int getLow()      { return offset; }
  int getHigh()     { return getWidth() + offset - 1; }
  int getWidth()    { return width; }

  OZ_Term getArg(int n)
  {
    n -= offset;
    if (n>=getWidth() || n<0)
      return 0;

    OZ_Term out = getArgs()[n];
    Assert(isRef(out) || !isAnyVar(out));

    return out;
  }

  int setArg(int n,TaggedRef val)
  {
    Assert(isRef(val) || !isAnyVar(val));

    n -= offset;
    if (n>=getWidth() || n<0) return FALSE;

    getArgs()[n] = val;
    return TRUE;
  }

  OZPRINT;
  OZPRINTLONG;

};


inline
Bool isArray(TaggedRef term)
{
  return isConst(term) && tagged2Const(term)->getType() == Co_Array;
}

inline
OzArray *tagged2Array(TaggedRef term)
{
  Assert(isArray(term));
  return (OzArray *) tagged2Const(term);
}


/*===================================================================
 * Abstraction (incl. PrTabEntry, AssRegArray, AssReg)
 *=================================================================== */

enum KindOfReg {
  XReg = XREG,
  YReg = YREG,
  GReg = GREG
};

class AssReg {
public:
  void print();

  PosInt number;
  KindOfReg kind;
};

inline
AssReg* allocAssRegBlock (int numb)
{
  return new AssReg[numb];
}

class AssRegArray  {
  friend void restore (int fileDesc, AssRegArray *array);
  /* we restore at the address ptr number of AssRegs;           */
public:
  AssRegArray (int sizeInit) : numbOfGRegs (sizeInit)
  {
    first = (sizeInit==0 ? (AssReg*) NULL : allocAssRegBlock(sizeInit));
  }
  ~AssRegArray () { Assert(0) };

  int getSize () { return (numbOfGRegs); }
  AssReg &operator[] (int elem) { return ( *(first + elem) ); }
  /* no bounds checking;    */

private:
  int numbOfGRegs;
  AssReg* first;
};


// Debugger ---------------------------------------------

class DbgInfo {
public:
  ProgramCounter PC;
  TaggedRef file;
  int line;
  DbgInfo *next;
  DbgInfo(ProgramCounter pc, TaggedRef f, int l, DbgInfo *nxt)
    : PC(pc), file(f), line(l), next(nxt) {};
};

extern DbgInfo *allDbgInfos;

// ---------------------------------------------


class PrTabEntry {
private:
  TaggedRef printname;
  unsigned short arity;
  SRecordArity methodArity;
  TaggedRef fileName;
  int lineno;
  TaggedRef info;
  TaggedRef names; // list of names for components: when loading
                   // theses names are replaced
                   // default: unit --> no replacements

public:
  PrTabEntry *next;
  unsigned int numClosures, numCalled, heapUsed, samples, lastHeap;
  static PrTabEntry *allPrTabEntries;
  static void printPrTabEntries();
  static TaggedRef getProfileStats();
  static void profileReset();

  ProgramCounter PC;

  PrTabEntry (TaggedRef name, SRecordArity arityInit,TaggedRef file, int line)
  : printname(name), fileName(file), lineno(line)
  {
    Assert(isLiteral(name));
    methodArity = arityInit;
    arity =  (unsigned short) getWidth(arityInit);
    Assert((int)arity == getWidth(arityInit)); /* check for overflow */
    PC = NOCODE;
    info = nil();
    names = NameUnit;
    numClosures = numCalled = heapUsed = samples = lastHeap =0;
    next = allPrTabEntries;
    allPrTabEntries = this;
  }

  OZPRINTLONG;

  int getArity () { return (int) arity; }
  TaggedRef getFileName() { return fileName; }
  int getLine() { return lineno; }
  SRecordArity getMethodArity() { return methodArity; }
  const char *getPrintName () { return tagged2Literal(printname)->getPrintName(); }
  TaggedRef getName () { return printname; }
  ProgramCounter getPC() { return PC; }
  void setPC(ProgramCounter pc) { PC = pc; }

  void setInfo(TaggedRef t) { info = t; }
  TaggedRef getInfo()       { return info; }

  void setNames(TaggedRef n) { names = n; }
  TaggedRef getNames()       { return names; }

  void patchFileAndLine();

  void gcPrTabEntry();
};



class Abstraction: public ConstTermWithHome {
  friend void ConstTerm::gcConstRecurse(void);
protected:
  // PrTabEntry *pred is in ptr field of ConstTerm
  RefsArray gRegs;
public:
  Abstraction(Abstraction&);
  Abstraction(PrTabEntry *prd, RefsArray gregs, Board *b)
    : ConstTermWithHome(b,Co_Abstraction), gRegs(gregs)
  {
    setPtr(prd);
  }

  OZPRINT;
  OZPRINTLONG;

  PrTabEntry *getPred()  { return (PrTabEntry *) getPtr(); }
  RefsArray &getGRegs()  { return gRegs; }
  ProgramCounter getPC() { return getPred()->getPC(); }
  int getArity()         { return getPred()->getArity(); }
  SRecordArity getMethodArity()   { return getPred()->getMethodArity(); }
  int getGSize()         { return getRefsArraySize(gRegs); }
  const char *getPrintName()   { return getPred()->getPrintName(); }
  TaggedRef getName()    { return getPred()->getName(); }

  TaggedRef DBGgetGlobals();

  GName *globalize();
  GName *getGName() {
    GName *gn = getGName1();
    return gn ? gn : globalize();
  }
  void import(RefsArray g, ProgramCounter pc) {
    gRegs = g;
    if (pc!=NOCODE) {
      getPred()->setPC(pc);
    }
  }
};

inline
Bool isProcedure(TaggedRef term)
{
  if (!isConst(term)) {
    return NO;
  }
  switch (tagged2Const(term)->getType()) {
  case Co_Abstraction:
  case Co_Builtin:
    return OK;
  default:
    return NO;
  }
}


inline
Bool isAbstraction(ConstTerm *s)
{
  return s->getType() == Co_Abstraction;
}

inline
Bool isAbstraction(TaggedRef term)
{
  return isConst(term) && isAbstraction(tagged2Const(term));
}

inline
Abstraction *tagged2Abstraction(TaggedRef term)
{
  Assert(isAbstraction(term));
  return (Abstraction *)tagged2Const(term);
}


/*===================================================================
 * Builtin (incl. BuiltinTabEntry)
 *=================================================================== */

class BuiltinTabEntry: public ConstTerm {
friend void ConstTerm::gcConstRecurse(void);
private:
  TaggedRef printname; //must be atom
  int arity;
  OZ_CFun fun;
  IFOR inlineFun;
#ifdef PROFILE_BI
  unsigned long counter;
#endif

public:

  /* use malloc to allocate memory */
  static void *operator new(size_t chunk_size)
  { return ::new char[chunk_size]; }

  BuiltinTabEntry(const char *s,int arty,OZ_CFun fn,IFOR infun)
  : arity(arty),fun(fn), inlineFun(infun), ConstTerm(Co_Builtin)
  {
    printname = makeTaggedAtom(s);
#ifdef PROFILE_BI
    counter = 0;
#endif
  }

  ~BuiltinTabEntry () {}

  OZPRINT;
  OZPRINTLONG;

  OZ_CFun getFun() { return fun; }
  int getArity() { return arity; }
  const char *getPrintName() { return tagged2Literal(printname)->getPrintName(); }
  TaggedRef getName() { return printname; }
  IFOR getInlineFun() { return inlineFun; }

#ifdef PROFILE_BI
  void incCounter() { counter++; }
  long getCounter() { return counter; }
#endif
};



inline
Bool isBuiltin(ConstTerm *s)
{
  return s->getType() == Co_Builtin;
}

inline
Bool isBuiltin(TaggedRef term)
{
  return isConst(term) && isBuiltin(tagged2Const(term));
}

inline
BuiltinTabEntry *tagged2Builtin(TaggedRef term)
{
  Assert(isBuiltin(term));
  return (BuiltinTabEntry *)tagged2Const(term);
}

/*===================================================================
 * Cell
 *=================================================================== */


class CellLocal:public Tertiary{
friend void ConstTerm::gcConstRecurse(void);
private:
  TaggedRef val;
public:

  CellLocal(Board *b,TaggedRef v) : Tertiary(b, Co_Cell,Te_Local), val(v) {}
  OZPRINT;
  OZPRINTLONG;

  TaggedRef getValue() { return val; }
  void setValue(TaggedRef v) { val=v; }
  TaggedRef exchangeValue(TaggedRef v) {
    TaggedRef ret = val;
    val = v;
    return ret;}

  void globalize(int);
};


#define Cell_Invalid     0
#define Cell_Requested   1
#define Cell_Next        2
#define Cell_Valid       4
#define Cell_Dump_Asked  8
#define Cell_Access_Bit 16

class CellSec{
friend class CellFrame;
friend class CellManager;
private:
  USEHEAPMEMORY;
  short state;
  short int readCtr;
  TaggedRef head;
  PendThread* pending;
  Site* next;
  TaggedRef contents;
  int accessNr;

public:
  CellSec(TaggedRef val){ // on globalize
    state=Cell_Valid;
    readCtr=0;
    pending=NULL;
    DebugCode(next=NULL);
    contents=val;
    accessNr= -1;
  }

  CellSec(int nr){ // on Proxy becoming Frame
    state=Cell_Invalid;
    readCtr=0;
    pending=NULL;
    accessNr=nr;

    DebugCode(head=makeTaggedNULL());
    DebugCode(contents=makeTaggedNULL());
    DebugCode(next=NULL);}
};

class CellManager:public Tertiary{
friend void ConstTerm::gcConstRecurse(void);
private:
  CellSec *sec;
public:
  OZPRINT;
  OZPRINTLONG;

  CellManager() : Tertiary(0,Co_Cell,Te_Manager){init();}

  void incManCtr(){
        Assert(getTertType()==Te_Manager);
        sec->readCtr++;}
  int getAndInitManCtr(){
        Assert(getTertType()==Te_Manager);
        int i=sec->readCtr;
        sec->readCtr=0;
        return i;}

  void setOwnCurrent();
  Bool isOwnCurrent();
  void init();
  void setCurrent(Site *, int);
  Site* getCurrent();
  void cellReceived(Site*,int);
  Site* cellSent(Site*,int, int&);
  void managerSesSiteCrash(Site*, int);
  Site* proxySesSiteCrash(Site*,Site*,int);
  void localize();
  void gcCellManager();
  void probeFault(Site*, int, int);
};

class CellProxy:public Tertiary{
friend void ConstTerm::gcConstRecurse(void);
private:
  int holder;
public:
  OZPRINT;
  OZPRINTLONG;

 CellProxy(int manager):Tertiary(manager,Co_Cell,Te_Proxy){  // on import
   holder = 0;
   DebugIndexCheck(manager);}

  void convertToFrame();
};

class CellFrame:public Tertiary{
friend void ConstTerm::gcConstRecurse(void);
private:
  CellSec *sec;
public:
  OZPRINT;
  OZPRINTLONG;

  CellFrame():Tertiary((Board*)NULL,Co_Cell,Te_Frame){Assert(0);}
  short getState(){
    Assert(sec!=NULL);
    return sec->state;}
  void setState(short s){
    Assert(sec!=NULL);
    sec->state=s;}

  Bool isAccessBit(){
    if(getState() & Cell_Access_Bit) return TRUE;
    return FALSE;}
  void setAccessBit(){
    setState(getState()| Cell_Access_Bit);}
  void resetAccessBit(){setState(getState() & (~Cell_Access_Bit));}

  void myStoreForward(void* forward){setPtr(forward);}
  void* getForward(){return getPtr();}

  void setPending(PendThread *pt){
    sec->pending=pt;}
  PendThread* getPending(){return sec->pending;}
  PendThread ** getPendBase(){return &(sec->pending);}

  void incCtr(){sec->readCtr++;}
  void decCtr(int i){sec->readCtr -= i;}
  int getCtr(){return sec->readCtr;}

  TaggedRef getHead(){return sec->head;}
  void setHead(TaggedRef val){sec->head=val;}

  void setNext(Site *s){
    sec->next=s;}
  Site* getNext(){return sec->next;}

  TaggedRef getContents(){
    Assert(getState()&(Cell_Valid|Cell_Requested));return sec->contents;}
  void setContents(TaggedRef tr){sec->contents=tr;}

  void initFromProxy(){
    int nr=(int)sec;
    sec=new CellSec(nr);}

  void initFromGlobalize(TaggedRef val){
    sec=new CellSec(val);}

  void convertToProxy(){
    setTertType(Te_Proxy);
    int nr = sec->accessNr;
    DebugCode(sec=NULL);
    sec = (CellSec*)nr;}

  void gcCellFrame();
  void gcCellFrameSec();

  int readAccessNr(){
    return sec->accessNr;}
  int incAccessNr(){
    sec->accessNr = sec->accessNr+1;
    return sec->accessNr;}
};


inline Bool isCell(TaggedRef term)
{
  return isConst(term) && tagged2Const(term)->getType() == Co_Cell;
}

/*===================================================================
 * Ports
 *=================================================================== */

class Port: public Tertiary {
friend void ConstTerm::gcConstRecurse(void);
public:
  Port(Board *b, TertType tt) : Tertiary(b,Co_Port,tt){}
};

class PortWithStream: public Port {
friend void ConstTerm::gcConstRecurse(void);
protected:
  TaggedRef strm;
public:
  TaggedRef exchangeStream(TaggedRef newStream)
  {
    TaggedRef ret = strm;
    strm = newStream;
    return ret;   }
  PortWithStream(Board *b, TaggedRef s) : Port(b,Te_Local)  {
    strm = s;}
};

class PortManager: public PortWithStream {
friend void ConstTerm::gcConstRecurse(void);
public:
  OZPRINTLONG;
  OZPRINT;
  PortManager() : PortWithStream(0,0) { Assert(0); };
};

/* ----------------------------------------------------
   PORTS    local               manager           proxy
lst word:   Co_Port:board       Co_Port:_         Co_Port:_
2nd word:   Te_Local:NO_ENTRY   Te_Manager:owner  Te_Proxy:borrow
3rd word    <<stream>>          <<stream>>        _
---------------------------------------------------- */

class PortLocal: public PortWithStream {
friend void ConstTerm::gcConstRecurse(void);
public:
  OZPRINTLONG;
  OZPRINT;
  PortLocal(Board *b, TaggedRef s) : PortWithStream(b,s) {};
};

class PortProxy: public Port {
friend void ConstTerm::gcConstRecurse(void);
public:
  PortProxy(int i): Port(0,Te_Proxy) { setIndex(i); }
  OZPRINTLONG;
  OZPRINT;
};

inline Bool isPort(TaggedRef term)
{ return isConst(term) && tagged2Const(term)->getType() == Co_Port;}

inline PortWithStream *tagged2PortWithStream(TaggedRef term)
{ return (PortWithStream *) tagged2Const(term);}

inline Port *tagged2Port(TaggedRef term)
{ return (Port*) tagged2Const(term);}

/*===================================================================
 * Space
 *=================================================================== */

class Space: public Tertiary {
friend void ConstTerm::gcConstRecurse(void);
private:
  Board *solve;
  // The solve pointer can be:
  // - 0 (the board is failed and has been discarded by the garbage
  //      collector)
  // - 1 (the space has been merged)
  // or a valid pointer
public:
  Space(Board *h, Board *s) : Tertiary(h,Co_Space,Te_Local), solve(s) {};
  Space(int i, TertType t) : Tertiary(i,Co_Space,t) {}

  OZPRINT;
  OZPRINTLONG;

  SolveActor *getSolveActor();
  Board *getSolveBoard() { return solve; }
  void  merge() { solve = (Board *) 1; }
  Bool isFailed();
  Bool isMerged();
};


inline
Bool isSpace(TaggedRef term)
{
  return isConst(term) && tagged2Const(term)->getType() == Co_Space;
}

inline
Space *tagged2Space(TaggedRef term)
{
  Assert(isSpace(term));
  return (Space *) tagged2Const(term);
}


/*===================================================================
 * Locks
 *=================================================================== */

class OzLock:public Tertiary{
public:
  OzLock(Board *b,TertType tt):Tertiary(b,Co_Lock,tt){}
  OzLock(int i,TertType tt):Tertiary(i,Co_Lock,tt){}
};

class LockLocal:public OzLock{
friend void ConstTerm::gcConstRecurse(void);
private:
  PendThread *pending;
public:
  LockLocal(Board *b) : OzLock(b,Te_Local){
    pending=NULL;
    setPtr(NULL);
    pending= NULL;}

  PendThread* getPending(){return pending;}
  void setPending(PendThread *pt){pending=pt;}
  PendThread** getPendBase(){return &pending;}

  Thread * getLocker(){return (Thread*) getPtr();}
  void setLocker(Thread *t){setPtr(t);}
  Bool hasLock(Thread *t){return (t==getLocker()) ? TRUE : FALSE;}

  void unlockComplex();
  void unlock(){
    Assert(getLocker()!=NULL);
    if(pending==NULL){
      setLocker(NULL);
      return;}
    unlockComplex();}

  Bool isLocked(Thread *t) { return (getLocker()==t); }

  void lockComplex(Thread *);
  void lock(Thread *t){
    if(t==getLocker()) {return;}
    if(getLocker()==NULL) {setLocker(t);return;}
    lockComplex(t);}
  Bool lockB(Thread *t){
    if(t==getLocker()) {return TRUE;}
    if(getLocker()==NULL) {setLocker(t);return TRUE;}
    lockComplex(t);
    return FALSE;}

  OZPRINT;
  OZPRINTLONG;
  void globalize(int);

  void convertToLocal(Thread *t,PendThread *pt){
    setLocker(t);
    pending=pt;}
};

#define Lock_Invalid     0
#define Lock_Requested   1
#define Lock_Next        2
#define Lock_Valid       4
#define Lock_Dump_Asked  8
#define Lock_Access     16

class LockSec{
friend class LockFrame;
friend class LockManager;
private:
  unsigned int state;
  PendThread* pending;
  Site* next;
  Thread *locker;

public:
  LockSec(Thread *t,PendThread *pt){ // on globalize
    state=Lock_Valid;
    pending=pt;
    locker=t;
    next=NULL; } // maybe not needed except for debug

  LockSec(){ // on Proxy becoming Frame
    state=Lock_Invalid;
    locker=NULL;
    pending=NULL;
    next=NULL;} // maybe not needed except for debug
};

class LockFrame:public OzLock{
friend void ConstTerm::gcConstRecurse(void);
private:
  LockSec *sec;
public:
  OZPRINT;
  OZPRINTLONG;

  LockFrame():OzLock((Board*)NULL,Te_Frame){Assert(0);}
  unsigned int getState(){
    Assert(sec!=NULL);
    return sec->state;}
  void setState(short s){
    Assert(sec!=NULL);
    sec->state=s;}

  void setLocker(Thread *t){
    sec->locker=t;}
  Thread *getLocker(){
    return sec->locker;}
  Bool hasLock(Thread *t){if(t==getLocker()) return TRUE;return FALSE;}

  Bool isAccessBit(){
    if(getState() & Cell_Access_Bit) return TRUE;
    return FALSE;}
  void setAccessBit(){
    setState(getState()| Cell_Access_Bit);}
  void resetAccessBit(){setState(getState() & (~Cell_Access_Bit));}

  void myStoreForward(void* forward){setPtr(forward);}
  void* getForward(){return getPtr();}

  void setPending(PendThread *pt){sec->pending=pt;}
  PendThread* getPending(){return sec->pending;}
  PendThread** getPendBase(){return &sec->pending;}

  void setNext(Site *s){sec->next=s;}
  Site* getNext(){return sec->next;}

  void initFromProxy(){
    sec=new LockSec();}
  void initFromGlobalize(Thread *t,PendThread *pt){
    sec=new LockSec(t,pt);}

  void convertToProxy(){
    setTertType(Te_Proxy);
    DebugCode(sec=NULL);}

  void gcLockFrame();
  void gcLockFrameSec();

  void lockComplex(Thread *);
  void lock(Thread *t){
    if(getLocker()==t) return;
    if((getLocker()==NULL) && (getState()==Lock_Valid)){
      Assert(getPending()==NULL);
      setLocker(t);
      return;}
    lockComplex(t);}
  Bool lockB(Thread *t){
    if(getLocker()==t) return TRUE;
    if((getLocker()==NULL) && (getState()==Lock_Valid)){
      Assert(getPending()==NULL);
      setLocker(t);
      return TRUE;}
    lockComplex(t);
    return FALSE;}

  void unlock(){
    setLocker(NULL);
    if((getState()==Lock_Valid) && (getPending()==NULL)){
      return;}
    unlockComplex();}

  void unlockComplex();
};

class LockManager:public OzLock{
friend void ConstTerm::gcConstRecurse(void);
private:
  LockSec *sec;
public:
  OZPRINT;
  OZPRINTLONG;

  LockManager() : OzLock((Board*)NULL,Te_Manager){Assert(0);}

  void setOwnCurrent(){
    setPtr(NULL);}
  Bool isOwnCurrent(){
    if(getPtr()==NULL) return TRUE;
    return FALSE;}

  void setCurrent(Site *s){
    setPtr(s);}
  Site* getCurrent(){
    return (Site*) getPtr();}

  void localize();
  void gcLockManager();

  Bool hasLock(Thread *t){if(t==((LockFrame*)this)->getLocker()) return TRUE;return FALSE;}

  void lockComplex(Thread *);
  void lock(Thread *t){
    LockFrame *lf=(LockFrame*)this;
    if(lf->getLocker()==t) return;
    if((lf->getLocker()==NULL) && (lf->getState()==Lock_Valid)){
      Assert(lf->getPending()==NULL);
      lf->setLocker(t);
      return;}
    lockComplex(t);}

  Bool lockB(Thread *t){
    LockFrame *lf=(LockFrame*)this;
    if(lf->getLocker()==t) return TRUE;
    if((lf->getLocker()==NULL) && (lf->getState()==Lock_Valid)){
      Assert(lf->getPending()==NULL);
      lf->setLocker(t);
      return TRUE;}
    lockComplex(t);
    return FALSE;}

  void unlock(){
    LockFrame *lf=(LockFrame*)this;
    lf->setLocker(NULL);
    if((lf->getState()==Lock_Valid) && (lf->getPending()==NULL)){
      return;}
    unlockComplex();}

  void unlockComplex();

};

class LockProxy:public OzLock{
friend void ConstTerm::gcConstRecurse(void);
private:
  int * dummy;
public:
  OZPRINT;
  OZPRINTLONG;

 LockProxy(int manager):OzLock(manager,Te_Proxy){  // on import
   DebugIndexCheck(manager);}

  void convertToFrame();
  void lock(Thread *);
  void unlock();
};



inline
Bool isLock(TaggedRef term)
{
  return isConst(term) && tagged2Const(term)->getType() == Co_Lock;
}

/*===================================================================
 *
 *=================================================================== */

char *toC(OZ_Term);
TaggedRef reverseC(TaggedRef l);
TaggedRef appendI(TaggedRef x,TaggedRef y);
Bool member(TaggedRef elem,TaggedRef list);
TaggedRef getUniqueName(const char *s);

#endif
