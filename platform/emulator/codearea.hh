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
 *     http://www.mozart-oz.org
 *
 *  See the file "LICENSE" or
 *     http://www.mozart-oz.org/LICENSE.html
 *  for information on usage and redistribution
 *  of this file, and for a DISCLAIMER OF ALL
 *  WARRANTIES.
 *
 */

#ifndef __CODE_AREAH
#define __CODE_AREAH

#include <time.h>
#include "base.hh"
#include "hashtbl.hh"
#include "opcodes.hh"
#include "value.hh"
#include "am.hh"

#define AE_COPYABLE  1
#define AE_COLLECTED 2
#define AE_MASK      3

class AbstractionEntry {
private:
  TaggedRef      abstr;
  ProgramCounter pc;
  IHashTable *   indexTable;
  Tagged2        next_flags;

  static AbstractionEntry * allEntries;

public:

  AbstractionEntry * getNext(void) {
    return (AbstractionEntry *) next_flags.getPtr();
  }
  void setNext(AbstractionEntry * n) {
    next_flags.setPtr(n);
  }

  int isCopyable(void) {
    return next_flags.getTag() & AE_COPYABLE;
  }
  int isCollected(void) {
    return next_flags.getTag() & AE_COLLECTED;
  }
  void setCollected(void) {
    next_flags.borTag(AE_COLLECTED);
  }
  void unsetCollected(void) {
    next_flags.bandTag(~AE_COLLECTED);
  }
  IHashTable * getIndexTable(void) {
    return indexTable;
  }
  AbstractionEntry(Bool fc) {
    abstr      = makeTaggedNULL();
    pc         = NOCODE;
    indexTable = NULL;
    next_flags.set(allEntries,fc);
    allEntries = this;
  }

  Abstraction * getAbstr(void) {
    return abstr ? (Abstraction *) tagged2Const(abstr) : (Abstraction *) NULL;
  };

  ProgramCounter getPC(void)  {
    return pc;
  };


  void setPred(Abstraction * abs);
  void gCollectAbstractionEntry(void);

  static void freeUnusedEntries();
};

/*****************************************************************************/

#define getWord(PC) (*(PC))


#ifdef THREADED
  typedef uint32 AdressOpcode;
#else
  typedef Opcode AdressOpcode;
#endif


#define C_TAGGED      0
#define C_INLINECACHE 1
#define C_ABSTRENTRY  2
#define C_FREE        3

const int codeGCListBlockSize = 10;

class GCListEntry {
private:
  Tagged2 entry;
public:
  void set(void * p, int t) {
    entry.set(p,t);
  }
  int getTag(void) {
    return entry.getTag();
  }
  void * getPtr(void) {
    return entry.getPtr();
  }
};

class CodeGCList {
  CodeGCList *next;
  int nextFree;
  GCListEntry block[codeGCListBlockSize];

public:
  CodeGCList(CodeGCList *nxt) { nextFree=0; next=nxt; }

  void dispose() {
    CodeGCList *aux = this;
    while(aux) {
      CodeGCList *aux1 = aux->next;
      delete aux;;
      aux = aux1;
    }
  }


  CodeGCList *add(ProgramCounter ptr, int tag)
  {
    if (this==NULL || nextFree >= codeGCListBlockSize) {
      CodeGCList *aux = new CodeGCList(this);
      return aux->add(ptr,tag);
    }

    block[nextFree].set(ptr,tag);
    nextFree++;
    return this;
  }

  CodeGCList *addInlineCache(ProgramCounter ptr) { return add(ptr,C_INLINECACHE); }
  CodeGCList *addTagged(ProgramCounter ptr) { return add(ptr,C_TAGGED); }
  CodeGCList *addAbstractionEntry(ProgramCounter ptr) { return add(ptr,C_ABSTRENTRY); }

  void remove(int,TaggedRef *);

  void collectGClist();
};


class CodeArea {
  friend class AM;
  friend class TMapping;
  friend int engine(Bool init);
  friend class Statistics;
  static HashTable atomTab;
  static HashTable nameTab;
  friend TaggedRef OZ_atom(OZ_CONST char *str);
  friend TaggedRef oz_atomNoDup(OZ_CONST char *str);
  friend TaggedRef oz_uniqueName(const char *str);
  friend inline void printAtomTab();
  friend inline void printNameTab();

  static Bool getNextDebugInfoArgs(ProgramCounter from,
                                   TaggedRef &file, int &line, int &colum,
                                   TaggedRef &comment);

protected:
  ByteCode *codeBlock;    /* a block of abstract machine code */
  int size;               /* size of this block */
  CodeArea *nextBlock;
  ProgramCounter wPtr;    /* write pointer for the code block */
  ProgramCounter curInstr;/* start of current instruction */
  time_t timeStamp;       /* feed time */
  Bool referenced;        /* for GC */
public:
  CodeGCList *gclist;

#define CheckWPtr Assert(wPtr < codeBlock+size)

  static CodeArea *allBlocks;

  void allocateBlock(int sz);
  static void init(void **instrtab);

public:
  static time_t findTimeStamp(ProgramCounter PC)
  {

    CodeArea *aux = allBlocks;
    while (aux) {
      if (aux->codeBlock<=PC && PC<aux->codeBlock+aux->size)
        return aux->timeStamp;
      aux = aux->nextBlock;
    }
    return (time_t) 0;
  }

  Bool isReferenced(void) {
    return referenced;
  }
  ByteCode *getStart() { return codeBlock; }
  static int getTotalSize(void);

  CodeArea(int sz);
  ~CodeArea();

  void checkPtr(void *ptr) {
      Assert(getStart()<=ptr && ptr<getStart()+size);
  }

  void protectInlineCache(InlineCache *cache) {
    gclist = gclist->addInlineCache((ProgramCounter)cache);
  }

  static ProgramCounter printDef(ProgramCounter PC,FILE *out=stderr);
  static TaggedRef dbgGetDef(ProgramCounter PC, ProgramCounter definitionPC,
                             int frameId, RefsArray Y, Abstraction *G);
  static TaggedRef getFrameVariables(ProgramCounter, RefsArray, Abstraction *);
  static void getDefinitionArgs(ProgramCounter PC, XReg &reg, int &next,
                                TaggedRef &file, int &line, int &colum,
                                TaggedRef &predName);

  /* with one argument it means that we need the code till the "query"  */
  static void display (ProgramCounter from, int size = 1, FILE* = stderr,
                       ProgramCounter to=NOCODE);

private:
  static int livenessXInternal(ProgramCounter from, TaggedRef *X,int n, int*xUsage);
public:
  static int livenessX(ProgramCounter from, TaggedRef *X=0,int n=0);

  static ProgramCounter definitionStart(ProgramCounter from);
  static ProgramCounter definitionEnd(ProgramCounter from);

#ifdef THREADED
  static void **globalInstrTable;
  static HashTable *opcodeTable;
#endif

  static AdressOpcode getOP(ProgramCounter PC) {
    return (AdressOpcode) getWord(PC); }
  static Opcode adressToOpcode(AdressOpcode);
  static AdressOpcode opcodeToAdress(Opcode);
  static Opcode getOpcode(ProgramCounter PC) {
    return adressToOpcode(getOP(PC)); }

  void gCollectCodeBlock(void);
  static void gCollectCodeAreaStart(void);
  static void gCollectCollectCodeBlocks(void);

#ifdef RECINSTRFETCH
  static void writeInstr(void);
#else
  static void writeInstr(void) {};
#endif

private:
// data
#ifdef RECINSTRFETCH
  static int fetchedInstr;
  static ProgramCounter ops[RECINSTRFETCH];
  static void recordInstr(ProgramCounter PC);
#endif

  static ProgramCounter writeWord(ByteCode c, ProgramCounter ptr)
  {
    *ptr = c;
    return ptr+1;
  }

  static ProgramCounter writeWord(void *p, ProgramCounter ptr)
  {
    return writeWord((ByteCode)ToInt32(p),ptr);
  }

public:
  static ProgramCounter writeIHashTable(IHashTable *ht, ProgramCounter ptr)
  {
    return writeWord(ht,ptr);
  }

  static ProgramCounter writeLiteral(TaggedRef literal, ProgramCounter ptr)
  {
    Assert(oz_isLiteral(literal));
    return writeWord(literal,ptr);
  }

  // kost@ : TODO : with disappearance of the old marshaler, this must
  // become a procedure returning nothing;
  ProgramCounter writeTagged(TaggedRef t, ProgramCounter ptr);

  static CodeArea *findBlock(ProgramCounter PC);

  void unprotectTagged(TaggedRef* t);

  static ProgramCounter writeInt(TaggedRef i, ProgramCounter ptr)
  {
    Assert(oz_isNumber(i));
    return writeWord(i,ptr);
  }

  static ProgramCounter writeLabel(int label, int offset, ProgramCounter ptr)
  {
    return writeWord(ToPointer(label-offset), ptr);
  }

  static ProgramCounter writeBuiltin(Builtin *bi, ProgramCounter ptr)
  {
    return writeWord(bi,ptr);
  }

  static ProgramCounter writeOpcode(Opcode oc, ProgramCounter ptr)
  {
    return writeWord(opcodeToAdress(oc),ptr);
  }

  static ProgramCounter writeXRegIndex(int index, ProgramCounter ptr)
  {
#ifdef FASTREGACCESS
#ifdef FASTERREGACCESS
    return writeWord((ByteCode) &(XREGS[index]),ptr);
#else
    index *= sizeof(TaggedRef);
    return writeWord((ByteCode)index,ptr);
#endif
#else
#ifdef CHECKREGACCESS
    return writeWord((ByteCode) ((index << 2) | 1),ptr);
#else
    return writeWord((ByteCode)index,ptr);
#endif
#endif
  }

  static ProgramCounter writeYRegIndex(int index, ProgramCounter ptr)
  {
#ifdef FASTREGACCESS
    index *= sizeof(TaggedRef);
#else
#ifdef CHECKREGACCESS
    index = ((index << 2) | 2);
#endif
#endif
    return writeWord((ByteCode)index,ptr);
  }

  static ProgramCounter writeGRegIndex(int index, ProgramCounter ptr)
  {
#ifdef FASTREGACCESS
    index *= sizeof(TaggedRef);
#else
#ifdef CHECKREGACCESS
    index = ((index << 2) | 3);
#endif
#endif
    return writeWord((ByteCode)index,ptr);
  }

  static ProgramCounter writeSRecordArity(SRecordArity ar, ProgramCounter ptr)
  {
    return writeWord((ByteCode)ar,ptr);
  }
  static ProgramCounter writeArity(int ar, ProgramCounter ptr)
  {
    return writeWord((ByteCode)ar,ptr);
  }

  static ProgramCounter writeRecordArity(Arity *ar, ProgramCounter ptr)
  {
    return writeWord(ar,ptr);
  }

  static ProgramCounter writeInt(int i, ProgramCounter ptr)
  {
    return writeWord((ByteCode)i,ptr);
  }


  ProgramCounter writeAbstractionEntry(AbstractionEntry *p, ProgramCounter ptr);
  void writeAbstractionEntry(AbstractionEntry *p)
  {
    wPtr = writeAbstractionEntry(p,wPtr);
  }


  static void writeWordAllocated(ByteCode c, ProgramCounter ptr) {
    *ptr = c;
  }
  static void writeWordAllocated(void *p, ProgramCounter ptr) {
    writeWordAllocated((ByteCode)ToInt32(p),ptr);
  }

  static ProgramCounter writeAddress(void *p, ProgramCounter ptr)
  {
    return writeWord(p, ptr);
  }
  static void writeAddressAllocated(void *p, ProgramCounter ptr) {
    writeWordAllocated(p, ptr);
  }

  void writeCache() { wPtr = writeCache(wPtr); }
  ProgramCounter writeCache(ProgramCounter PC);

  void writeInt(int i)                   { CheckWPtr; wPtr=writeInt(i,wPtr); }
  void writeTagged(TaggedRef t)          { CheckWPtr; wPtr=writeTagged(t,wPtr); }
  void writeBuiltin(Builtin *bi)         { CheckWPtr; wPtr=writeBuiltin(bi,wPtr); }
  void writeOpcode(Opcode oc)            { CheckWPtr; curInstr=wPtr; wPtr=writeOpcode(oc,wPtr); }
  void writeSRecordArity(SRecordArity ar){ CheckWPtr; wPtr=writeSRecordArity(ar,wPtr); }
  void writeAddress(void *ptr)           { CheckWPtr; wPtr=writeWord(ptr,wPtr); }
  void writeXReg(int i)                   { CheckWPtr; wPtr=writeXRegIndex(i,wPtr); }
  void writeYReg(int i)                   { CheckWPtr; wPtr=writeYRegIndex(i,wPtr); }
  void writeGReg(int i)                   { CheckWPtr; wPtr=writeGRegIndex(i,wPtr); }
  void writeLabel(int lbl) { CheckWPtr; wPtr=writeLabel(lbl,curInstr-codeBlock,wPtr); }
  int computeLabel(int lbl) { return lbl-(curInstr-codeBlock); }
  void writeDebugInfo(TaggedRef file, int line) {
    CheckWPtr; allDbgInfos = new DbgInfo(wPtr,file,line,allDbgInfos);
  }
  static void writeDebugInfo(ProgramCounter PC, TaggedRef file, int line) {
    allDbgInfos = new DbgInfo(PC,file,line,allDbgInfos);
  }
  ProgramCounter getWritePtr(void)       { return wPtr; }

  //
  // kost@ : support for non-recursive (new) unmarshaler...
  // The builder expects the word to be word-aligned, so that it can
  // treat it as an s-pointer.
  static ProgramCounter allocateWord(ProgramCounter ptr) {
    Assert(((unsigned int) ptr) / sizeof(int) * sizeof(int) == (unsigned int) ptr);
    return (ptr+1);
  }
};



inline void printAtomTab()
{
  CodeArea::atomTab.print();
}


inline void printNameTab()
{
  CodeArea::nameTab.print();
}


/*
 * the following are not members of CodeArea: they are used
 * within the emulator, so we save prefixing them every time
 * with "CodeArea::"
 */

#ifdef CHECKREGACCESS
inline
XReg getXRegArg(ProgramCounter PC) {
  int w = getWord(PC);
  Assert((w & 3) == 1);
  return w >> 2;
}
inline
YReg getYRegArg(ProgramCounter PC) {
  int w = getWord(PC);
  Assert((w & 3) == 2);
  return w >> 2;
}
inline
GReg getGRegArg(ProgramCounter PC) {
  int w = getWord(PC);
  Assert((w & 3) == 3);
  return w >> 2;
}
#else
#define getXRegArg(PC)    ((XReg) getWord(PC))
#define getYRegArg(PC)    ((YReg) getWord(PC))
#define getGRegArg(PC)    ((GReg) getWord(PC))
#endif

#define getPosIntArg(PC) ((int) getWord(PC))
#define getTaggedArg(PC) ((TaggedRef) getWord(PC))
#define getNumberArg(PC)  getTaggedArg(PC)
#define getLiteralArg(PC) getTaggedArg(PC)
#define getAdressArg(PC)  (ToPointer(getWord(PC)))
#define getPredArg(PC)   ((PrTabEntry *) getAdressArg(PC))
#define getLabelArg(PC)  ((int) getWord(PC))
#define GetLoc(PC)       ((OZ_Location*) getAdressArg(PC))
#define GetBI(PC)        ((Builtin*) getAdressArg(PC))

/*
 * Inline caching
 */

class InlineCache {
  uint32 key;
  int32 value;

public:
  InlineCache() { key = value = 0; }

  int lookup(SRecord *rec, TaggedRef feature)
  {
    if (key!=(uint32)rec->getSRecordArity()) {
      int32 aux = rec->getIndex(feature);
      if (aux==-1)
        return aux;
      value = aux;
      key   = rec->getSRecordArity();
    }
    return value;
  }

  Abstraction *lookup(ObjectClass *c, TaggedRef meth,
                      SRecordArity arity)
  {
    if (ToInt32(c) != key) {
      Bool defaultsUsed = NO;
      Abstraction *ret = c->getMethod(meth,arity,OK,defaultsUsed);
      if (!defaultsUsed && ret) {
        value = ToInt32(ret);
        key   = ToInt32(c);
      }
      return ret;
    }
    return (Abstraction*) ToPointer(value);
  }

  void invalidate() { key = value = 0; }
};

class CallMethodInfo {
public:
  int regIndex;
  Bool isTailCall;
  TaggedRef mn;
  SRecordArity arity;

  CallMethodInfo(int ri, TaggedRef name, Bool ist, SRecordArity ar)
  {
    regIndex   = ri;
    isTailCall = ist;
    arity = ar;
    mn = name;
    OZ_protect(&mn);
  }

  ~CallMethodInfo() { OZ_unprotect(&mn); }
  void dispose()      { delete this; }
};

class OZ_LocList {
public:
  OZ_Location * loc;
  OZ_LocList  * next;
  OZ_LocList(OZ_Location * l,OZ_LocList *n) : loc(l), next(n) {}
};

class OZ_Location {
private:
  int         fingerprint;
  TaggedRef * map[1];
  static TaggedRef  * new_map[];
  static OZ_LocList * cache[];

public:
  NO_DEFAULT_CONSTRUCTORS(OZ_Location)
  static void initCache(void);
  /*
   * Construction of locations
   */
  static void initLocation(void) {
  }
  static OZ_Location * alloc(int n) {
    int sz = sizeof(OZ_Location)+sizeof(TaggedRef*)*(n-1);
    return (OZ_Location *) malloc(sz);
  }
  static void set(int n, int i) {
    new_map[n]=&(XREGS[i]);
  }
  static int getNewIndex(int n) {
    return (new_map[n]-XREGS);
  }
  static OZ_Location * getLocation(int n);

  TaggedRef ** getMapping(void) {
    return map;
  }
  int getIndex(int n) {
    return (map[n]-XREGS);
  }
  int getInIndex(int n) {
    return getIndex(n);
  }
  int getOutIndex(Builtin * bi, int n) {
    return getIndex(bi->getInArity() + n);
  }
  TaggedRef getValue(int n) {
    return *map[n];
  }
  TaggedRef getInValue(int n) {
    return getValue(n);
  }
  TaggedRef getOutValue(Builtin * bi, int n) {
    return getValue(bi->getInArity()+n);
  }
  TaggedRef getInArgs(Builtin *);
  TaggedRef getArgs(Builtin *);
};

extern OZ_Location * OZ_ID_LOC;

#ifdef FASTREGACCESS

#ifdef FASTERREGACCESS

#define XRegToInt(N) (((TaggedRef*) (N)) - XREGS)
#define XRegToPtr(N) ((TaggedRef *) (N))

#else

#define XRegToInt(N) ((N) / sizeof(TaggedRef))
#define XRegToPtr(N) ((TaggedRef *) (((intlong) XREGS) + (N)))

#endif

#define YRegToInt(N) ((N) / sizeof(TaggedRef))
#define YRegToPtr(Y,N) ((TaggedRef *) (((intlong) (Y)) + (N)))

#define GRegToInt(N) ((N) / sizeof(TaggedRef))
#define GRegToPtr(G,N) ((TaggedRef *) (((intlong) (G)) + (N)))

#else

inline
int  XRegToInt(XReg N) { return N; }
inline
int  YRegToInt(YReg N) { return N; }
inline
int  GRegToInt(GReg N) { return N; }
inline
TaggedRef * XRegToPtr(XReg N) {
  return &(XREGS[N]);
}
inline
TaggedRef * YRegToPtr(RefsArray Y, YReg N) {
  return Y+N;
}
inline
TaggedRef * GRegToPtr(RefsArray G, GReg N) {
  return G+N;
}

#endif


#endif
