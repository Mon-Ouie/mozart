/*
 *  Authors:
 *    Author's name (Author's email address)
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
/*
  Hydra Project, DFKI Saarbruecken,
  Stuhlsatzenhausweg 3, D-66123 Saarbruecken, Phone (+49) 681 302-5312
  Author: mehl
  Last modified: $Date$ from $Author$
  Version: $Revision$
  State: $State$
  */

#ifndef __BUILTINSH
#define __BUILTINSH

#ifdef INTERFACE
#pragma interface
#endif

// specification for builtins
struct BIspec {
  char *name;
  int arity;
  OZ_CFun fun;
  IFOR ifun;
};


// add specification to builtin table
void BIaddSpec(BIspec *spec);
OZ_Term OZ_findBuiltin(char *name, OZ_Term handler);

BuiltinTabEntry *BIinit();
BuiltinTabEntry *BIadd(const char *name,int arity,OZ_CFun fun,IFOR infun=(IFOR) NULL);

void threadRaise(Thread *th,OZ_Term E);

// -----------------------------------------------------------------------
// tables

class BuiltinTab : public HashTable {
public:
  BuiltinTab(int sz) : HashTable(HT_CHARKEY,sz) {};
  ~BuiltinTab() {};
  unsigned memRequired(void) {
    return HashTable::memRequired(sizeof(BuiltinTabEntry));
  }
  const char * getName(void * fp) {
    HashNode * hn = getFirst();
    for (; hn != NULL; hn = getNext(hn)) {
      BuiltinTabEntry * abit = (BuiltinTabEntry *) hn->value;
      if (abit->getInlineFun() == (IFOR) fp ||
	  abit->getFun() == (OZ_CFun) fp)
	return hn->key.fstr;
    }
    return "???";
  }
  BuiltinTabEntry * getEntry(void * fp) {
    HashNode * hn = getFirst();
    for (; hn != NULL; hn = getNext(hn)) {
      BuiltinTabEntry * abit = (BuiltinTabEntry *) hn->value;
      if (abit->getInlineFun() == (IFOR) fp ||
	  abit->getFun() == (OZ_CFun) fp)
	return abit;
    }
    return (BuiltinTabEntry *) NULL;
  }

  BuiltinTabEntry *find(const char *name) { return (BuiltinTabEntry*) htFind(name); }
};

extern BuiltinTab builtinTab;
extern OZ_Return dotInline(TaggedRef term, TaggedRef fea, TaggedRef &out);
extern OZ_Return uparrowInlineBlocking(TaggedRef term, TaggedRef fea,
				       TaggedRef &out);

OZ_Return BIarityInline(TaggedRef, TaggedRef &);
OZ_Return adjoinPropList(TaggedRef t0, TaggedRef list, TaggedRef &out,
			     Bool recordFlag);

OZ_C_proc_proto(BIatWithState)
OZ_C_proc_proto(BIassignWithState)

// -----------------------------------------------------------------------
// propagators

class WidthPropagator : public OZ_Propagator {
private:
  static OZ_CFunHeader spawner;
protected:
  OZ_Term rawrec, rawwid;
public:
  WidthPropagator(OZ_Term r, OZ_Term w)
    : rawrec(r), rawwid(w) {}

  virtual void updateHeapRefs(OZ_Boolean) {
    OZ_updateHeapTerm(rawrec);
    OZ_updateHeapTerm(rawwid);
  }
  virtual size_t sizeOf(void) { return sizeof(WidthPropagator); }
  virtual OZ_Return propagate(void);
  virtual OZ_CFunHeader * getHeader(void) const {return &spawner; }
  virtual OZ_Term getParameters(void) const { return OZ_nil(); }
};

class MonitorArityPropagator : public OZ_Propagator {
private:
  static OZ_CFunHeader spawner;
protected:
  OZ_Term X, K, L, FH, FT;
public:
  MonitorArityPropagator(OZ_Term X1, OZ_Term K1, OZ_Term L1,
                         OZ_Term FH1, OZ_Term FT1)
    : X(X1), K(K1), L(L1), FH(FH1), FT(FT1) {}

  virtual void updateHeapRefs(OZ_Boolean) {
    OZ_updateHeapTerm(X);
    OZ_updateHeapTerm(K);
    OZ_updateHeapTerm(L);
    OZ_updateHeapTerm(FH);
    OZ_updateHeapTerm(FT);
  }
  virtual size_t sizeOf(void) { return sizeof(MonitorArityPropagator); }
  virtual OZ_Return propagate(void);
  virtual OZ_CFunHeader * getHeader(void) const {return &spawner; }
  virtual OZ_Term getParameters(void) const { return OZ_nil(); }
  
  TaggedRef getX(void) { return X; }
  TaggedRef getK(void) { return K; }
  TaggedRef getFH(void) { return FH; }
  TaggedRef getFT(void) { return FT; }
  void setFH(TaggedRef FH1) { FH=FH1; }
};

#endif

