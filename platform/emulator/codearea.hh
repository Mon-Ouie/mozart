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

#ifndef __CODE_AREAH
#define __CODE_AREAH

#include <sys/time.h>
#include "base.hh"
#include "hashtbl.hh"
#include "opcodes.hh"
#include "value.hh"

/**********************************************************************
 *                                                                    *
 *    class AbstractionTable: represented as hash table               *
 *                                                                    *
 **********************************************************************/


class AbstractionEntry {
private:
  Abstraction *abstr;
  ProgramCounter pc;
  RefsArray g;
  int arity;

  /* all entries are linked for GC */
  AbstractionEntry *next;
  static AbstractionEntry* allEntries;

public:
  IHashTable *indexTable;
  Bool dupOnload;  // true iff has to be created newly when loading (for components)

  AbstractionEntry(Bool fc) {
    abstr      = 0;
    pc         = NOCODE;
    next       = allEntries;
    dupOnload  = fc;
    allEntries = this;
    indexTable = 0;
  }
  Abstraction *getAbstr() { return abstr; };
  RefsArray getGRegs()    { return g; };
  ProgramCounter getPC()  { return pc; };
  int getArity()          { return arity; };
  void setPred(Abstraction *abs);

  static void gcAbstractionEntries();
};


class AbstractionTable: public HashTable {
public:
  AbstractionTable(int s) : HashTable(HT_INTKEY,s) {};

  static AbstractionEntry *add(int id);
  static AbstractionEntry *add(Abstraction *abstr);
  void gcAbstractionTable()
  {
    AbstractionEntry::gcAbstractionEntries();
  }

};




/*****************************************************************************/

#define getWord(PC) (*(PC))


#ifdef THREADED
  typedef uint32 AdressOpcode;
#else
  typedef Opcode AdressOpcode;
#endif


class CodeArea {
  friend class AM;
  friend class TMapping;
  friend int engine(Bool init);
  friend class Statistics;
  static HashTable atomTab;
  static HashTable nameTab;
  friend Literal *addToAtomTab(const char *str);
  friend Literal *addToNameTab(const char *str);
  friend inline void printAtomTab();
  friend inline void printNameTab();

  static Bool getNextDebugInfoArgs(ProgramCounter from,
                                   TaggedRef &file, TaggedRef &line,
                                   TaggedRef &column, TaggedRef &comment);

protected:
  ByteCode *codeBlock;    /* a block of abstract machine code */
  int size;               /* size of this block */
  CodeArea *nextBlock;
  ProgramCounter wPtr;    /* write pointer for the code block */
  ProgramCounter curInstr;/* start of current instruction */
  time_t timeStamp;       /* feed time */
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

  ByteCode *getStart() { return codeBlock; }
  static AbstractionTable abstractionTab;
  static int totalSize; /* total size of code allocated in bytes */

  /* read from file and return start in "pc" */
  CodeArea(CompStream *fd, int size, ProgramCounter &pc);
  CodeArea(int sz);

  static void printDef(ProgramCounter PC);
  static TaggedRef dbgGetDef(ProgramCounter PC, ProgramCounter definitionPC,
                             int frameId, RefsArray Y, RefsArray G);
  static TaggedRef getFrameVariables(ProgramCounter, RefsArray, RefsArray);
  static void getDefinitionArgs(ProgramCounter PC, Reg &reg,
                                int &next, TaggedRef &file,
                                TaggedRef &line, TaggedRef &column,
                                TaggedRef &predName);

  /* with one argument it means that we need the code till the "query"  */
  static void display (ProgramCounter from, int size = 1, FILE* = stderr,
                       ProgramCounter to=NOCODE);
  static int livenessX(ProgramCounter from, TaggedRef *X=0,int n=0);

  static ProgramCounter definitionStart(ProgramCounter from);
  static ProgramCounter definitionEnd(ProgramCounter from);

#ifdef OLD_COMPILER
  /* load statements from "codeFile" until "ENDOFFILE", acknowledge if ok*/
  static Bool load(CompStream *fd, ProgramCounter &newPC);
#endif

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

  static char **opToString;
  static Opcode stringToOp(const char *s);

  static void gc();

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

#ifdef OLD_COMPILER
// functions
  static int scanChar(CompStream *fd);
  static char *scanString (CompStream *fd);
  static int scanUInt(CompStream *fd);
  static Bool scanBool(CompStream *fd);
  void scanVariablename (CompStream *fd);
  void scanLiteral(CompStream *fd);
  void scanFeature(CompStream *fd);
  TaggedRef parseFeature(CompStream *fd);
  TaggedRef parseLiteral(CompStream *fd);
  TaggedRef parseLiteral(CompStream *fd, int what);
  TaggedRef parseNumber(CompStream *fd);
  void scanRegister(CompStream *fd, int &regAdd);
  void scanRegisterIndex (CompStream *fd);
  void scanArity(CompStream *fd);
  void scanNumber(CompStream *fd);
  void scanPosint(CompStream *fd);
  void scanCache(CompStream *fd);
  void scanPredicateRef(CompStream *fd);
  void scanGenCallInfo(CompStream *fd);
  void scanApplMethInfo(CompStream *fd);
  void scanLabel(CompStream *fd, int offset);
  SRecordArity parseRecordArity(CompStream *fd);
  void scanRecordArity(CompStream *fd);
  TaggedRef parseRecordArity (CompStream *fd, int length);
  void scanBuiltinname(CompStream *fd);
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
    Assert(isLiteral(literal));
    return writeWord(literal,ptr);
  }

  static ProgramCounter writeTagged(TaggedRef t, ProgramCounter ptr)
  {
    ProgramCounter ret = writeWord(t,ptr);
    gcStaticProtect((TaggedRef *)ptr);
    return ret;
  }

  static ProgramCounter writeInt(TaggedRef i, ProgramCounter ptr)
  {
    Assert(isNumber(i));
    return writeWord(i,ptr);
  }

  static ProgramCounter writeLabel(int label, int offset, ProgramCounter ptr,
                                   Bool checkLabel)
  {
    //  label==0 means fail in createCond
    //  in this case do not add start
    return writeWord(checkLabel && label==0 ? NOCODE : ToPointer(label-offset), ptr);
  }

  static ProgramCounter writeBuiltin(Builtin *bi, ProgramCounter ptr)
  {
    return writeWord(bi,ptr);
  }

  static ProgramCounter writeOpcode(Opcode oc, ProgramCounter ptr)
  {
    return writeWord(opcodeToAdress(oc),ptr);
  }

  static ProgramCounter writeRegIndex(int index, ProgramCounter ptr)
  {
#ifdef FASTREGACCESS
    index *= sizeof(TaggedRef);
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


  static ProgramCounter writePredicateRef(int i, ProgramCounter ptr)
  {
    AbstractionEntry *entry = AbstractionTable::add(i);
    return writeWord(entry, ptr);
  }

  static ProgramCounter writeAddress(void *p, ProgramCounter ptr)
  {
    return writeWord(p, ptr);
  }

  void writeCache() { wPtr = writeCache(wPtr); }
  static ProgramCounter writeCache(ProgramCounter PC);

  void writeInt(int i)                   { CheckWPtr; wPtr=writeInt(i,wPtr); }
  void writeTagged(TaggedRef t)          { CheckWPtr; wPtr=writeTagged(t,wPtr); }
  void writeBuiltin(Builtin *bi)         { CheckWPtr; wPtr=writeBuiltin(bi,wPtr); }
  void writeOpcode(Opcode oc)            { CheckWPtr; curInstr=wPtr; wPtr=writeOpcode(oc,wPtr); }
  void writeSRecordArity(SRecordArity ar){ CheckWPtr; wPtr=writeSRecordArity(ar,wPtr); }
  void writeAddress(void *ptr)           { CheckWPtr; wPtr=writeWord(ptr,wPtr); }
  void writeReg(int i)                   { CheckWPtr; wPtr=writeRegIndex(i,wPtr); }
  void writeLabel(int lbl) { CheckWPtr; wPtr=writeLabel(lbl,curInstr-codeBlock,wPtr,OK); }
  int computeLabel(int lbl) { return lbl-(curInstr-codeBlock); }
  void writeDebugInfo(TaggedRef file, int line) {
    CheckWPtr; allDbgInfos = new DbgInfo(wPtr,file,line,allDbgInfos);
  }
  ProgramCounter getWritePtr(void)       { return wPtr; }
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

#define getRegArg(PC)    ((Reg) getWord(PC))
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

  Abstraction *lookup(ObjectClass *c, TaggedRef meth, SRecordArity arity,RefsArray X)
  {
    if (ToInt32(c) != key) {
      Bool defaultsUsed = NO;
      Abstraction *ret = c->getMethod(meth,arity,X,defaultsUsed);
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

class GenCallInfoClass {
public:
  int regIndex;
  Bool isMethAppl, isTailCall;
  TaggedRef mn;
  SRecordArity arity;

  GenCallInfoClass(int ri, Bool ism, TaggedRef name, Bool ist, SRecordArity ar)
  {
    regIndex   = ri;
    isMethAppl = ism;
    isTailCall = ist;
    arity = ar;
    mn = name;
    OZ_protect(&mn);
  }

  ~GenCallInfoClass() { OZ_unprotect(&mn); }
  void dispose()      { delete this; }
};

class ApplMethInfoClass {
public:
  TaggedRef methName;
  SRecordArity arity;
  InlineCache methCache;

  ApplMethInfoClass(TaggedRef mn, SRecordArity i)
  {
    arity = i;
    methName = mn;
    gcStaticProtect(&methName);
    protectInlineCache(&methCache);
  }
};



#ifdef FASTREGACCESS
inline Reg regToInt(Reg N) { return (N / sizeof(TaggedRef)); }
inline Reg intToReg(Reg N) { return N * sizeof(TaggedRef); }
#else
inline Reg regToInt(Reg N) { return N; }
inline Reg intToReg(Reg N) { return N; }
#endif


#endif
