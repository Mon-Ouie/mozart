/*
 * FBPS Saarbr"ucken
 * Author: mehl
 * Last modified: $Date$ from $Author$
 * Version: $Revision$
 * State: $State$
 *
 * Values: literal, list, records
 */

#ifndef __VALUEHH
#define __VALUEHH

#ifdef INTERFACE
#pragma interface
#endif

#include "perdiotables.hh"

/*===================================================================
 * global names and atoms
 *=================================================================== */
extern TaggedRef AtomNil, AtomCons, AtomPair, AtomVoid,
       AtomSucceeded, AtomAlt, AtomMerged, AtomFailed,
       AtomEntailed, AtomSuspended, AtomBlocked,
       AtomEmpty, AtomUpper, AtomLower, AtomDigit, 
       AtomCharSpace, AtomPunct, AtomOther,
       NameTrue, NameFalse, AtomBool, AtomSup, AtomCompl, AtomUnknown,
       AtomMin, AtomMax, AtomMid,
       AtomNaive, AtomSize, AtomConstraints,
       AtomDistributed, AtomMobile, AtomFetched,
       NameOoAttr,NameOoFreeFeatR,NameOoFreeFlag,
       NameOoDefaultVar,NameOoRequiredArg,
       NameUnit,
       // Atoms for System.get and System.set
       AtomActive, AtomAtoms, AtomBuiltins, AtomCellHack, AtomChosen, 
       AtomCloned, AtomCode, AtomCopy, AtomCreated, AtomDebug, AtomDepth, 
       AtomFeed, AtomForeign, AtomFree, AtomFreelist, AtomGC, AtomHigh, 
       AtomHints, AtomIdle, AtomInt, AtomInvoked, AtomLimits, AtomLoad, 
       AtomLocation, AtomMiddle, AtomNames, AtomOn, AtomPropagate, 
       AtomPropagators, AtomRun, AtomRunnable, AtomRuns, AtomShowSuspension, 
       AtomStackMaxSize, AtomStopOnToplevelFailure, AtomSystem, AtomThread, 
       AtomThreshold, AtomTolerance, AtomUser, AtomVariables, AtomWidth,
       AtomHeap, AtomDebugIP, AtomDebugPerdio;

/*===================================================================
 * Literal
 *=================================================================== */

class Literal {
private:
  static int LiteralCurrentNumber;
  // 
  char *printName;
  Board *home;
  // home can be: NULL: an atom;
  //              ALLBITS: optimized (static) top-level name;
  //              <board-addr>: non-top-level name;
  int seqNumber; 
public:
  Literal (char *str = "", Bool flag = NO);
  Literal (Board *hb); 

  isAtom()     { return home == (Board *) NULL; }
  isOptName()  { return home == (Board *) ToPointer(ALLBITS); }
  isDynName()  { return !isAtom() && !isOptName(); }

  Board *getBoardFast(); // see am.icc

  Literal *gc ();       // for non-top-level names only;
  void gcRecurse (); // too;

// atoms are the only terms which are not allocated on heap and that's why, 
// its new-operator has to be overloaded to allocate global memory for atoms
  void *operator new(size_t size) {return ::new char[size]; }
  void operator  delete(void *p, size_t) { ::delete(p); }

// term functions

  OZPRINT;
  OZPRINTLONG;
  
  /* It's an assumption: assembler produces only one entry.   */
  /* OK means that this is equal constants.       */

  // name access
  char* getPrintName() { return printName; }
  int getSize() { return strlen(printName); }

  int getSeqNumber() { return (seqNumber); } 

  int hash() { return getSeqNumber(); }
};

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

  // res == 0

  return (((a->getSeqNumber ()) < (b->getSeqNumber ())) ? -1 : 1);
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
#include "gmp.h"
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


#ifdef GNUWIN32
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
TaggedRef makeTaggedFloat(double i)
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
  BigInt(int i) {
    mpz_init_set_si(&value,i);
  }
  BigInt(char *s) {
    if(mpz_init_set_str(&value, s, 10)) {
      Assert(0);
    }
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

#define MKOP(op,mpop)							      \
  TaggedRef op(BigInt *b) {						      \
    BigInt *n = new BigInt();						      \
    mpop(&n->value,&value,&b->value);					      \
    return n->shrink();							      \
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
protected:
// DATA
  TaggedRef args[2];   // head, tail
 public:
  USEHEAPMEMORY;

  LTuple(void) {;} // called by putlist and the like
  LTuple(TaggedRef head, TaggedRef tail) 
  { args[0] = head; args[1] = tail; }

  TaggedRef getHead() { return tagged2NonVariable(args); }
  TaggedRef getTail() { return tagged2NonVariable(args+1); }
  void setHead(TaggedRef term) { args[0] = term;}
  void setTail(TaggedRef term) { args[1] = term;}
  TaggedRef getLabel() { return AtomCons; }
  Literal *getLabelLiteral() { return tagged2Literal(AtomCons); }
  TaggedRef *getRef() { return &args[0]; }
  TaggedRef *getRefHead() { return &args[0]; }
  TaggedRef *getRefTail() { return &args[1]; }
  void initArgs(TaggedRef val) { args[0]=val; args[1]=val;}

  OZPRINT;
  OZPRINTLONG;
  
  void gcRecurse();
  LTuple *gc();
};


// functions for builtins and features
//   with explicit deref


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
TaggedRef * headRef(TaggedRef list)
{
  Assert(isLTuple(list));
  return tagged2LTuple(list)->getRefHead();
}

inline
TaggedRef tail(TaggedRef list)
{
  Assert(isLTuple(list));
  return tagged2LTuple(list)->getTail();
}

inline
TaggedRef * tailRef(TaggedRef list)
{
  Assert(isLTuple(list));
  return tagged2LTuple(list)->getRefTail();
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
  Co_Board,
  Co_Actor,
  Co_HeapChunk,
  Co_Thread,

  Co_Abstraction,
  Co_Builtin,  
  Co_Cell,
  Co_Space,
  Co_Port,

  /* chunks must stay together and the first one
   * must be Co_Object
   * otherwise you'll have to change the "isChunk" test
   * NOTE: update the builtins: subtree and chunkArity !
   */
  Co_Object,   
  Co_Chunk,
  Co_Array,
  Co_Dictionary,
  Dummy      // GCTAG
};


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
  ConstTerm();
  ConstTerm(const ConstTerm& ct);

  ConstTerm *gcConstTerm(void);
  void gcConstRecurse(void);

  void setTagged(TypeOfConst t, void *p) { ctu.tagged = makeTaggedRef((TypeOfTerm)t,p); }
  ConstTerm(TypeOfConst t)     { setTagged(t,NULL); }
  TypeOfConst getType() { return (TypeOfConst) tagTypeOf(ctu.tagged); }
  //  TypeOfConst typeOf()  { return getType(); }
  char *getPrintName();
  int getArity();
  void *getPtr()        { return isNullPtr(ctu.tagged) ? NULL : tagValueOf(ctu.tagged); }
  void setPtr(void *p)  { setTagged(getType(),p); }

  OZPRINT;
  OZPRINTLONG;

  Bool unify(TaggedRef, Bool) { return NO; }
  Bool install(TaggedRef t) { return unify(t,OK); };  
  Bool deinstall(TaggedRef) { return OK; };

  /* optimized isChunk test */
  Bool isChunk() { return (int) getType() >= (int) Co_Object; }
};


class ConstTermWithHome: public ConstTerm {
protected:
  Board *home;
public:
  ConstTermWithHome();
  ~ConstTermWithHome();
  ConstTermWithHome(ConstTermWithHome&);
  ConstTermWithHome(Board *b, TypeOfConst t) : ConstTerm(t), home(b) {}
  Board *getBoardFast();
};


/*===================================================================
 * HeapChunk
 *=================================================================== */

class HeapChunk: public ConstTerm {
private:
  size_t chunk_size;
  char * chunk_data;
  char * copyChunkData(void) {
    char * data = allocate(chunk_size);
    for (int i = chunk_size; i--; )
      data[i] = chunk_data[i];
    return data;
  }
  char * allocate(int size) {
    return (char *) alignedMalloc(size, sizeof(double));
  }
public:
  HeapChunk();
  ~HeapChunk();
  HeapChunk(HeapChunk&);
  HeapChunk(int size)
  : ConstTerm(Co_HeapChunk), chunk_size(size), chunk_data(allocate(size)) {
    }

  size_t getChunkSize(void) { return chunk_size; }

  char * getChunkData(void) { return chunk_data; }

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

inline
int featureEq(TaggedRef a,TaggedRef b)
{
  CHECK_FEATURE(a);
  CHECK_FEATURE(b);
  if (isLiteral(a)) {
    // Note: if b is no literal this also returns NO
    return literalEq(a,b);
  }
  TypeOfTerm tagA = tagTypeOf(a);
  TypeOfTerm tagB = tagTypeOf(b);
  if (tagA != tagB) return NO;
  switch(tagA) {
  case SMALLINT: return smallIntEq(a,b);
  case BIGINT:   return bigIntEq(a,b);
  default:       return NO;
  }
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
int featureHash(TaggedRef a)
{
  CHECK_FEATURE(a);
  TypeOfTerm tag = tagTypeOf(a);
  switch (tag) {
  case LITERAL:
    return tagged2Literal(a)->hash();
  case SMALLINT:
    return (int) a;
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

  int size;		// size is always a power of 2
  int hashmask;		// size-1, used as mask for hashing
  int width;	        // next unused index in RefsArray (for add())
  DebugCheckT(int numberofentries);
  DebugCheckT(int numberofcollisions);
  Bool isTupleFlag;

  TaggedRef *keytable;
  int *indextable;
  int scndhash(TaggedRef a) { return ((featureHash(a)&7)<<1)|1; }
  int hashfold(int i) { return(i&hashmask); }
  void add (TaggedRef);

public:
  Bool isTuple() {
    return isTupleFlag;
  }
  int find(TaggedRef entry)   // return -1, if argument not found.
  {
    if (isTuple()) return isSmallInt(entry) ? smallIntValue(entry)-1 : -1;

    int i=hashfold(featureHash(entry));
    int step=scndhash(entry);
    while (OK) {
      if ( keytable[i] == makeTaggedNULL()) return -1;
      if ( featureEq(keytable[i],entry) ) return indextable[i];
      i = hashfold(i+step);
    }
  }
public:
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

  void initArgs(TaggedRef val)
  {
    for (int i = getWidth(); i--; )
      args[i] = val;
  }
  
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
    SRecord *ret = (SRecord *) int32Malloc(memSize);
    ret->label = lab;
    ret->recordArity = arity;
    return ret;
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
  
  TaggedRef adjoinAt(TaggedRef feature, TaggedRef value);

  SRecord *replaceLabel(TaggedRef newlabel)
  {	
    SRecord *copy = newSRecord(this);
    copy->label = newlabel;
    return copy;
  }

  TaggedRef adjoin(SRecord* highstr);
  TaggedRef adjoinList(TaggedRef arity, TaggedRef proplist);
  void setFeatures(TaggedRef proplist);
  
  TaggedRef getLabel() { return label; }
  Literal *getLabelLiteral() { return tagged2Literal(label); }
  void setLabel(TaggedRef newLabel) { label = newLabel; }
  
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
    return getArity()->find(feature);
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
};

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
  return isLTuple(term) || isSTuple(term);
}

inline
TaggedRef left(TaggedRef pair)
{
  Assert(OZ_isPair2(pair));
  return tagged2SRecord(pair)->getArg(0);
}

inline
TaggedRef right(TaggedRef pair)
{
  Assert(OZ_isPair(pair));
  return tagged2SRecord(pair)->getArg(1);
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
  return (0);			// ???
}


/*===================================================================
 * ObjectOrClass incl. ObjectClass, DeepObjectOrClass
 *=================================================================== */

/* Internal representation of Oz classes */

class ObjectClass {
private:
  OzDictionary *fastMethods;
  Literal *printName;
  OzDictionary *slowMethods;
  OzDictionary *defaultMethods;
  Abstraction *send;
  SRecord *unfreeFeatures;
  TaggedRef ozclass;    /* the class as seen by the Oz user */

public:
  USEHEAPMEMORY;

  ObjectClass(OzDictionary *fm, Literal *pn, OzDictionary *sm, 
	      Abstraction *snd, SRecord *uf,
	      OzDictionary *dm)
  {
    fastMethods    = fm;
    printName      = pn;
    slowMethods    = sm;
    send           = snd;
    unfreeFeatures = uf;
    defaultMethods = dm;
    ozclass        = AtomNil;

  }
  OzDictionary *getSlowMethods() { return slowMethods; }
  OzDictionary *getDefMethods()  { return defaultMethods; }
  OzDictionary *getfastMethods() { return fastMethods; }
  Abstraction *getAbstraction()  { return send; }
  char *getPrintName()           { return printName->getPrintName(); }
  TaggedRef getOzClass()         { return ozclass; }
  void setOzClass(TaggedRef cl)  { ozclass = cl; }

  TaggedRef getFeature(TaggedRef lit) 
  {
    return unfreeFeatures
      ? unfreeFeatures->getFeature(lit)
      : makeTaggedNULL();
  }

  SRecord *getUnfreeRecord() { return unfreeFeatures; }

  ObjectClass *gcClass();
  OZPRINT;
  OZPRINTLONG;
};


/*
 * Object
 */

typedef enum {
  OFlagClosed = 0x1,
  OFlagDeep   = 0x2,
  OFlagClass  = 0x4
} OFlag;

#define DeepnessShift 3

class Object: public ConstTerm {
  friend void ConstTerm::gcConstRecurse(void);
protected:
  int32 state;  // was: SRecord *state, but saves memory on the Alpha
  int32 aclass; // was: ObjectClass *aclass
  TaggedRef threads;  /* list of variables with threads attached to them */
  int32 deepness;     /* deepnes plus OFlag */
public:
  Object();
  ~Object();
  Object(Object&);

  Object(SRecord *s,ObjectClass *ac,SRecord *feat,Bool iscl):
    ConstTerm(Co_Object)
  {
    setFreeRecord(feat);
    deepness = 0;
    threads = AtomNil;
    setClass(ac);
    setState(s);
    if (iscl) setClass();
  };

  int getDeepness()     { return (deepness >> DeepnessShift);}
  int incDeepness()     { deepness += (1<<DeepnessShift); return getDeepness();}
  int decDeepness()     { deepness -= (1<<DeepnessShift); return getDeepness();}

  Bool isClosedOrClassOrDeepOrLocked() { return (deepness!=0); }

  void setFlag(OFlag f) { deepness |= (int) f; } 

  Bool isClass()        { return (deepness)&OFlagClass; }
  void setClass()       { setFlag(OFlagClass); }
  Bool isDeep()         { return (deepness)&OFlagDeep; }
  void setIsDeep()      { setFlag(OFlagDeep); }
  Bool isClosed()       { return (deepness)&OFlagClosed; }
  void close()          { setFlag(OFlagClosed); }

  void setClass(ObjectClass *c) { aclass = ToInt32(c); }

  TaggedRef attachThread();
  inline void release();

  ObjectClass *getClass() { return (ObjectClass*) ToPointer(aclass); }

  char *getPrintName()          { return getClass()->getPrintName(); }
  OzDictionary *getMethods()    { return getClass()->getfastMethods(); }
  Abstraction *getMethod(TaggedRef label, SRecordArity arity, RefsArray X,
			 Bool &defaultsUsed);

  Bool lookupDefault(TaggedRef label, SRecordArity arity, RefsArray X);
  SRecord *getState()           { return (SRecord*) ToPointer(state); }
  void setState(SRecord *s)     { state = ToInt32(s); }
  Abstraction *getAbstraction() { return getClass()->getAbstraction(); }
  OzDictionary *getSlowMethods() { return getClass()->getSlowMethods(); }
  OzDictionary *getDefMethods()  { return getClass()->getDefMethods(); }
  TaggedRef getOzClass()        { return getClass()->getOzClass(); }
  Board *getBoardFast();
  SRecord *getFreeRecord()          { return (SRecord *) getPtr(); }
  SRecord *getUnfreeRecord() { 
    return isClass() ? (SRecord*) NULL : getClass()->getUnfreeRecord(); 
  }
  void setFreeRecord(SRecord *aRec) { setPtr(aRec); }

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

  TaggedRef getArityList();
  int getWidth ();

  Object *gcObject();

  OZPRINT;
  OZPRINTLONG;
};


inline
void Object::release()
{
  Assert(getDeepness()>0);
  if (decDeepness()==0) {
    /* wake threads */
    Assert(!isRef(threads));
    if (!literalEq(threads,AtomNil)) {
      incDeepness();
      TaggedRef var = head(threads);
      if (OZ_unify(var, isClosed() ? NameTrue : NameFalse)==FAILED) {
	warning("Object::wakeThreads: unify failed");
      }
      threads = tail(threads);
    }
  }
}

/* objects not created on toplevel need a home pointer */

class DeepObject: public Object {
  friend void ConstTerm::gcConstRecurse(void);
  friend class Object;
private:
  Board *home;
public:
  DeepObject();
  ~DeepObject();
  DeepObject(DeepObject&);
  DeepObject(SRecord *s,ObjectClass *cl,
	     SRecord *feat,Bool iscl, Board *bb):
    Object(s,cl,feat,iscl)
  {
    setIsDeep();
    home=bb;
  };
};



inline
Bool isObject(TaggedRef term)
{
  return isConst(term) && tagged2Const(term)->getType()==Co_Object;
}

inline
Object *tagged2Object(TaggedRef term)
{
  Assert(isObject(term));
  return (Object *)tagged2Const(term);
}

/*===================================================================
 * SChunk
 *=================================================================== */

class SChunk: public ConstTerm {
friend void ConstTerm::gcConstRecurse(void);
private:
  TaggedRef value;
public:
  SChunk();
  ~SChunk();
  SChunk(SChunk&);
  SChunk(Board *b,TaggedRef v) : ConstTerm(Co_Chunk), value(v) {
    Assert(isRecord(v));
    Assert(b);
    setPtr(b);
  };

  OZPRINT;
  OZPRINTLONG;

  TaggedRef getValue() { return value; }
  TaggedRef getFeature(TaggedRef fea) { return OZ_subtree(value,fea); }
  TaggedRef getArityList() { return ::getArityList(value); }
  int getWidth () { return ::getWidth(value); }
  Board *getBoardFast();
};


inline
Bool isSChunk(TaggedRef term)
{
  return isConst(term) && tagged2Const(term)->getType() == Co_Chunk;
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
  OzArray();
  ~OzArray();
  OzArray(OzArray&);
  OzArray(Board *b, int low, int high, TaggedRef initvalue) : ConstTermWithHome(b,Co_Array) 
  {
    Assert(isRef(initvalue) || !isAnyVar(initvalue));

    offset = low;
    width = high-low+1;
    if (width <= 0) {
      width = 0;
      setPtr(NULL);
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

  OZ_Return getArg(int n, TaggedRef &out) 
  { 
    n -= offset;
    if (n>=getWidth() || n<0)
      return OZ_raiseC("array",2,makeTaggedConst(this),OZ_int(n));

    out = getArgs()[n];
    Assert(isRef(out) || !isAnyVar(out));

    return PROCEED;
  }

  OZ_Return setArg(int n,TaggedRef val) 
  { 
    Assert(isRef(val) || !isAnyVar(val));

    n -= offset;
    if (n>=getWidth() || n<0)
      return OZ_raiseC("array",2,makeTaggedConst(this),OZ_int(n));

    getArgs()[n] = val;
    return PROCEED;
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
  AssRegArray () { numbOfGRegs = 0; first = (AssReg *)NULL; }

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


class PrTabEntry {
private:
  TaggedRef printname;
  unsigned short arity;
  unsigned short spyFlag;
  SRecordArity methodArity;

public:
  AssRegArray gRegs;
  ProgramCounter PC;

  PrTabEntry (TaggedRef name, SRecordArity arityInit, int numbOfGRegs)
  : printname(name), gRegs (numbOfGRegs), spyFlag(NO)
  {
      Assert(isLiteral(name));
      methodArity = arityInit;
      arity =  (unsigned short) getWidth(arityInit);
      Assert((int)arity == getWidth(arityInit));
  }

  OZPRINTLONG;
  
  int getArity () { return (int) arity; }
  SRecordArity getMethodArity() { return methodArity; }
  int getGSize () { return gRegs.getSize(); }
  char *getPrintName () { return tagged2Literal(printname)->getPrintName(); }
  TaggedRef getName () { return printname; }
  ProgramCounter getPC() { return PC; }
  Bool getSpyFlag()   { return (Bool) spyFlag; }
  void setSpyFlag()   { spyFlag = OK; }
  void unsetSpyFlag() { spyFlag = NO; }
};



#define Mobile      0x1
#define Distributed 0x2    /* exported to some other site */
#define Fetched     0x4    /* was already brought to local site */

class DistObject: public ConstTermWithHome {
private:
  int type;
public:
  DistObject();
  ~DistObject();
  DistObject(DistObject&);
  DistObject(Board *b, TypeOfConst tc) : ConstTermWithHome(b,tc), type(0) {}

  void setDistFlag(int f)   { type |= f;  }
  void unsetDistFlag(int f) { type &= ~f; }

  Bool getDistFlag(int f) { return (type&f); }

  Bool isMobile()         { return getDistFlag(Mobile); }
  Bool isDistributed()    { return getDistFlag(Distributed); }
  Bool isFetched()        { return getDistFlag(Fetched); }

  void distribute() { setDistFlag(Distributed); }
};


class Abstraction: public DistObject {
  friend void ConstTerm::gcConstRecurse(void);
private:
// DATA
  RefsArray gRegs;
  PrTabEntry *pred;
public:
  Abstraction();
  ~Abstraction();
  Abstraction(Abstraction&);
  Abstraction(PrTabEntry *prd, RefsArray gregs, Board *b, Bool mobile=OK)
  : DistObject(b,Co_Abstraction), gRegs(gregs), pred(prd) 
  {
    setDistFlag(Fetched);
    if (mobile)
      setDistFlag(Mobile);
  }

  /* receiving a distributed procedure: */
  Abstraction(Bool mobile);

  OZPRINT;
  OZPRINTLONG;

  RefsArray &getGRegs()  { return gRegs; }
  ProgramCounter getPC() { return pred->getPC(); }
  int getArity()         { return pred->getArity(); }
  SRecordArity getMethodArity()   { return pred->getMethodArity(); }
  int getGSize()         { return pred->getGSize(); }
  char *getPrintName()   { return pred->getPrintName(); }
  PrTabEntry *getPred()  { return pred; }
  TaggedRef getName()    { return pred->getName(); }

  void makeStationary() { unsetDistFlag(Mobile); }
  void nowFetched(RefsArray g, PrTabEntry *pte) 
  {
    setDistFlag(Fetched);
    gRegs = g;
    pred  = pte;
  }

  TaggedRef DBGgetGlobals();
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

// special builtins known in emulate
enum BIType {
  BIDefault,
  BIraise
};

class BuiltinTabEntry {
  friend class Debugger;
public:
  BuiltinTabEntry (Literal *name,int arty,OZ_CFun fn,
		   IFOR infun=NULL)
  : printname(makeTaggedLiteral(name)), arity(arty),fun(fn),
    inlineFun(infun), type(BIDefault)
  {
    Assert(isAtom(printname));
  }
  BuiltinTabEntry (char *s,int arty,OZ_CFun fn,
		   IFOR infun=NULL)
  : arity(arty),fun(fn), inlineFun(infun), type(BIDefault)
  {
    printname = makeTaggedAtom(s);
    Assert(isAtom(printname));
  }
  BuiltinTabEntry (char *s,int arty,OZ_CFun fn,BIType t,
		   IFOR infun=NULL)
    : arity(arty),fun(fn), inlineFun(infun), type(t) {
      printname = makeTaggedAtom(s);
    }
  BuiltinTabEntry (char *s,int arty,BIType t, IFOR infun=(IFOR)NULL)
    : arity(arty),fun(NULL), inlineFun(infun), type(t)
  {
    printname = makeTaggedAtom(s);
    Assert(isAtom(printname));
  }

  ~BuiltinTabEntry () {}

  OZPRINT;
  OZ_CFun getFun() { return fun; }
  int getArity() { return arity; }
  char *getPrintName() { return tagged2Literal(printname)->getPrintName(); }
  TaggedRef getName() { return printname; }
  IFOR getInlineFun() { return inlineFun; } 
  BIType getType() { return type; } 

private:

  TaggedRef printname; //must be atom
  int arity;
  OZ_CFun fun;
  IFOR inlineFun;
  BIType type;
};



class Builtin: public ConstTerm {
friend void ConstTerm::gcConstRecurse(void);
private:
  BuiltinTabEntry *fun;
  TaggedRef suspHandler; // this one is called, when it must suspend
public:
  Builtin();
  ~Builtin();
  Builtin(Builtin&);
  Builtin(BuiltinTabEntry *fn, TaggedRef handler)
    : suspHandler(handler), fun(fn), ConstTerm(Co_Builtin)
    {}

  OZPRINT;
  OZPRINTLONG;

  int getArity()                    { return fun->getArity(); }
  OZ_CFun getFun()                  { return fun->getFun(); }
  char *getPrintName()              { return fun->getPrintName(); }
  TaggedRef getName()               { return fun->getName(); }
  BIType getType()                  { return fun->getType(); } 
  TaggedRef getSuspHandler()        { return suspHandler; }
  BuiltinTabEntry *getBITabEntry()  { return fun; }
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
Builtin *tagged2Builtin(TaggedRef term)
{
  Assert(isBuiltin(term));
  return (Builtin *)tagged2Const(term);
}

/*===================================================================
 * Cell
 *=================================================================== */

class Cell: public DistObject {
friend void ConstTerm::gcConstRecurse(void);
private:
  TaggedRef val;
public:
  Cell();
  ~Cell();
  Cell(Cell&);
  Cell(Board *b,TaggedRef v, Bool mobile=NO) : 
    DistObject(b, Co_Cell), val(v) 
  {
    if (mobile)
      setDistFlag(Mobile);
  }

  /* receiving a cell */
  Cell(TaggedRef v, Bool mobile);

  OZPRINT;
  OZPRINTLONG;

  TaggedRef getValue() { return val; }
  TaggedRef exchangeValue(TaggedRef v) {
    TaggedRef ret = val;
    val = v;
    return ret;
  }
  TaggedRef distribute(TaggedRef v)
  {
    DistObject::distribute();
    return exchangeValue(v);
  }
};

inline Bool isCell(TaggedRef term)
{
  return isConst(term) && tagged2Const(term)->getType() == Co_Cell;
}

inline
Cell *tagged2Cell(TaggedRef term)
{
  Assert(isCell(term));
  return (Cell *) tagged2Const(term);
}


/*===================================================================
 * Ports
 *=================================================================== */

class Port: public DistObject {
friend void ConstTerm::gcConstRecurse(void);
private:
  TaggedRef strm;
  NetAddress *addr;
public:
  Port();
  ~Port();
  Port(Port&);

  Port(Board *b,TaggedRef s) : DistObject(b, Co_Port), strm(s), addr(0) {}
  Port(NetAddress *na);

  TaggedRef exchangeStream(TaggedRef newStream)
  { 
    TaggedRef ret = strm;
    strm = newStream;
    return ret; 
  }

  TaggedRef getStream() { return strm; }

  TaggedRef *getStreamRef() { return &strm; }

  NetAddress *getAddress() { return addr; }

  NetAddress *export();

  Bool isLocal() { return (addr==NULL || addr->isLocal()); }

  OZPRINT;
  OZPRINTLONG;
};


inline Bool isPort(TaggedRef term)
{
  return isConst(term) && tagged2Const(term)->getType() == Co_Port;
}

inline
Port *tagged2Port(TaggedRef term)
{
  Assert(isPort(term));
  return (Port *) tagged2Const(term);
}


/*===================================================================
 * Space
 *=================================================================== */

class Space: public ConstTermWithHome {
friend void ConstTerm::gcConstRecurse(void);
private:
  Board *solve;
  // The solve pointer can be:
  // - 0 (the board is failed and has been discarded by the garbage 
  //      collector)
  // - 1 (the space has been merged)
  // or a valid pointer
public:
  Space();
  ~Space();
  Space(Space&);
  Space(Board *h, Board *s) : ConstTermWithHome(h,Co_Space), solve(s) {};

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
 * 
 *=================================================================== */

char *toC(OZ_Term);
TaggedRef reverseC(TaggedRef l);
TaggedRef appendI(TaggedRef x,TaggedRef y);

#endif
