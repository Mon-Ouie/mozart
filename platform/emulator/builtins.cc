/*
 *  Authors:
 *    Michael Mehl (mehl@dfki.de)
 *    Kostja Popow (popow@ps.uni-sb.de)
 *    Ralf Scheidhauer (Ralf.Scheidhauer@ps.uni-sb.de)
 *    Christian Schulte (schulte@dfki.de)
 *
 *  Contributors:
 *    Peter van Roy (pvr@info.ucl.ac.be)
 *    Denys Duchier (duchier@ps.uni-sb.de)
 *    Leif Kornstaedt (kornstae@ps.uni-sb.de)
 *
 *  Copyright:
 *    Michael Mehl, 1997
 *    Kostja Popow, 1997
 *    Ralf Scheidhauer, 1997
 *    Christian Schulte, 1997
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

#if defined(INTERFACE) && !defined(PEANUTS)
#pragma implementation "builtins.hh"
#endif

#include "wsock.hh"
#include "builtins.hh"
#include "perdio.hh"
#include "os.hh"
#include "codearea.hh"
#include "debug.hh"
#include "copycode.hh"

#include "wsock.hh"

#include "iso-ctype.hh"
#include <string.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>

#if defined(LINUX) || defined(SOLARIS) || defined(SUNOS_SPARC) || defined(IRIX) || defined(OSF1_ALPHA) || defined(FREEBSD) || defined(NETBSD)
#   define DLOPEN 1
#if defined(FREEBSD)
#define RTLD_NOW 1
#endif
#endif

#ifdef DLOPEN
#ifdef SUNOS_SPARC
#define RTLD_NOW 1
extern "C" void * dlopen(char *, int);
extern "C" char * dlerror(void);
extern "C" void * dlsym(void *, char *);
extern "C" int dlclose(void *);
#else
#include <dlfcn.h>
#endif
#endif

#ifdef IRIX
#include <bstring.h>
#include <sys/time.h>
#endif

#ifdef HPUX_700
#include <dl.h>
#endif

#include "runtime.hh"

#include "genvar.hh"
#include "ofgenvar.hh"
#include "fdbuilti.hh"
#include "fdhook.hh"
#include "solve.hh"
#include "oz_cpi.hh"
#include "dictionary.hh"

/********************************************************************
 * Macros
 ******************************************************************** */

#define NONVAR(X,term)                          \
TaggedRef term = X;                             \
{ DEREF(term,_myTermPtr,_myTag);                \
  if (isAnyVar(_myTag)) return SUSPEND;         \
}

// mm2
// Suspend on UVAR and SVAR:
#define NONSUVAR(X,term,tag)                    \
TaggedRef term = X;                             \
TypeOfTerm tag;                                 \
{ DEREF(term,_myTermPtr,myTag);                 \
  tag = myTag;                                  \
  if (isNotCVar(tag)) return SUSPEND;           \
}


#define DECLAREBI_USEINLINEREL1(Name,InlineName)        \
OZ_C_proc_begin(Name,1)                                 \
{                                                       \
  oz_declareArg(0,arg1);                                \
  OZ_Return state = InlineName(arg1);                   \
  if (state == SUSPEND) {                               \
    oz_suspendOn(arg1);                                 \
  } else {                                              \
    return state;                                       \
  }                                                     \
}                                                       \
OZ_C_proc_end


#define DECLAREBI_USEINLINEREL2(Name,InlineName)        \
OZ_C_proc_begin(Name,2)                                 \
{                                                       \
  oz_declareArg(0,arg0);                                \
  oz_declareArg(1,arg1);                                \
  OZ_Return state = InlineName(arg0,arg1);              \
  if (state == SUSPEND) {                               \
    oz_suspendOn2(arg0,arg1);                           \
  } else {                                              \
    return state;                                       \
  }                                                     \
}                                                       \
OZ_C_proc_end


#define DECLAREBI_USEINLINEREL3(Name,InlineName)        \
OZ_C_proc_begin(Name,3)                                 \
{                                                       \
  oz_declareArg(0,arg0);                                \
  oz_declareArg(1,arg1);                                \
  oz_declareArg(2,arg2);                                \
  OZ_Return state = InlineName(arg0,arg1,arg2);         \
  if (state == SUSPEND) {                               \
    oz_suspendOn3(arg0,arg1,arg2);                      \
  } else {                                              \
    return state;                                       \
  }                                                     \
}                                                       \
OZ_C_proc_end


#define DECLAREBI_USEINLINEFUN1(Name,InlineName)        \
OZ_C_proc_begin(Name,2)                                 \
{                                                       \
  OZ_Term help;                                         \
  oz_declareArg(0,arg1);                                \
  oz_declareArg(1,out);                                 \
  OZ_Return state = InlineName(arg1,help);              \
  switch (state) {                                      \
  case SUSPEND:                                         \
    oz_suspendOn(arg1);                                 \
  case PROCEED:                                         \
    return oz_unify(help,out);                          \
  default:                                              \
    return state;                                       \
  }                                                     \
}                                                       \
OZ_C_proc_end


#define DECLAREBI_USEINLINEFUN2(Name,InlineName)        \
OZ_C_proc_begin(Name,3)                                 \
{                                                       \
  OZ_Term help;                                         \
  oz_declareArg(0,arg0);                                \
  oz_declareArg(1,arg1);                                \
  oz_declareArg(2,out);                                 \
  OZ_Return state=InlineName(arg0,arg1,help);           \
  switch (state) {                                      \
  case SUSPEND:                                         \
    oz_suspendOn2(arg0,arg1);                           \
  case PROCEED:                                         \
    return oz_unify(help,out);                          \
  default:                                              \
    return state;                                       \
  }                                                     \
}                                                       \
OZ_C_proc_end

#define DECLAREBI_USEINLINEFUN3(Name,InlineName)        \
OZ_C_proc_begin(Name,4)                                 \
{                                                       \
  OZ_Term help;                                         \
  oz_declareArg(0,arg0);                                \
  oz_declareArg(1,arg1);                                \
  oz_declareArg(2,arg2);                                \
  oz_declareArg(3,out);                                 \
  OZ_Return state=InlineName(arg0,arg1,arg2,help);      \
  switch (state) {                                      \
  case SUSPEND:                                         \
    oz_suspendOn3(arg0,arg1,arg2);                      \
  case PROCEED:                                         \
    return oz_unify(help,out);                          \
  default:                                              \
    return state;                                       \
  }                                                     \
}                                                       \
OZ_C_proc_end

#define DECLAREBOOLFUN1(BIfun,BIifun,BIirel)            \
OZ_Return BIifun(TaggedRef val, TaggedRef &out)         \
{                                                       \
  OZ_Return state = BIirel(val);                        \
  switch(state) {                                       \
  case PROCEED: out = NameTrue;  return PROCEED;        \
  case FAILED:  out = NameFalse; return PROCEED;        \
  default: return state;                                \
  }                                                     \
}                                                       \
DECLAREBI_USEINLINEFUN1(BIfun,BIifun)

#define DECLAREBOOLFUN2(BIfun,BIifun,BIirel)                            \
OZ_Return BIifun(TaggedRef val1, TaggedRef val2, TaggedRef &out)        \
{                                                                       \
  OZ_Return state = BIirel(val1,val2);                                  \
  switch(state) {                                                       \
  case PROCEED: out = NameTrue;  return PROCEED;                        \
  case FAILED:  out = NameFalse; return PROCEED;                        \
  default: return state;                                                \
  }                                                                     \
}                                                                       \
DECLAREBI_USEINLINEFUN2(BIfun,BIifun)

#define CheckLocalBoard(Object,Where);                                  \
  if (!am.onToplevel() && !am.isCurrentBoard(GETBOARD(Object))) {       \
    return oz_raise(E_ERROR,E_KERNEL,"globalState",1,oz_atom(Where));   \
  }

/********************************************************************
 * `builtin`
 ******************************************************************** */


OZ_C_proc_begin(BIbuiltin,3)
{
  oz_declareAtomArg(0,name);
  oz_declareIntArg(1,arity);
  oz_declareArg(2,ret);

  BuiltinTabEntry *found = builtinTab.find(name);

  if (found == htEmpty) {
    return oz_raise(E_ERROR,E_SYSTEM,"builtinUndefined",1,OZ_getCArg(0));
  }

  if (arity!=-1 && arity != found->getArity()) {
    return oz_raise(E_ERROR,E_SYSTEM,"builtinArity",3,
                    OZ_getCArg(0),OZ_getCArg(1),
                    makeTaggedSmallInt(found->getArity()));
  }

  return oz_unify(ret,makeTaggedConst(found));
}
OZ_C_proc_end


/********************************************************************
 * Type tests
 ******************************************************************** */

OZ_Return isValueInline(TaggedRef val)
{
  NONVAR( val, _1);
  return PROCEED;
}
DECLAREBI_USEINLINEREL1(BIisValue,isValueInline)


OZ_C_proc_begin(BIwaitOr,2)
{
  oz_declareDerefArg(0,a);

  if (!isAnyVar(a)) return PROCEED;

  oz_declareDerefArg(1,b);

  if (!isAnyVar(b)) return PROCEED;

  Assert(isAnyVar(a) && isAnyVar(b));

  am.addSuspendVarList(aPtr);
  am.addSuspendVarList(bPtr);

  return SUSPEND;
}
OZ_C_proc_end


OZ_Return isLiteralInline(TaggedRef t)
{
  NONSUVAR( t, term, tag );
  if (isCVar(tag)) {
      switch (tagged2CVar(term)->getType()) {
      case OFSVariable:
        {
          GenOFSVariable *ofsvar=tagged2GenOFSVar(term);
          if (ofsvar->getWidth()>0) return FAILED;
          return SUSPEND;
        }
      case FDVariable:
      case BoolVariable:
          return FAILED;
      default:
          return SUSPEND;
      }
  }
  return isLiteral(tag) ? PROCEED : FAILED;
}

DECLAREBI_USEINLINEREL1(BIisLiteral,isLiteralInline)
DECLAREBOOLFUN1(BIisLiteralB,isLiteralBInline,isLiteralInline)


OZ_Return isAtomInline(TaggedRef t)
{
  NONSUVAR( t, term, tag );
  if (isCVar(tag)) {
      switch (tagged2CVar(term)->getType()) {
      case OFSVariable:
        {
          GenOFSVariable *ofsvar=tagged2GenOFSVar(term);
          TaggedRef lbl=ofsvar->getLabel();
          DEREF(lbl,_1,lblTag);
          if (isLiteral(lblTag) && !isAtom(lbl)) return FAILED;
          if (ofsvar->getWidth()>0) return FAILED;
          return SUSPEND;
        }
      case FDVariable:
      case BoolVariable:
          return FAILED;
      default:
          return SUSPEND;
      }
  }
  return isAtom(term) ? PROCEED : FAILED;
}

DECLAREBI_USEINLINEREL1(BIisAtom,isAtomInline)
DECLAREBOOLFUN1(BIisAtomB,isAtomBInline,isAtomInline)

OZ_Return isLockInline(TaggedRef t)
{
  NONSUVAR( t, term, tag );
  return isLock(term) ? PROCEED : FAILED;
}

DECLAREBI_USEINLINEREL1(BIisLock,isLockInline)
DECLAREBOOLFUN1(BIisLockB,isLockBInline,isLockInline)




OZ_Return isFreeRelInline(TaggedRef term) {
  DEREF(term, _1, tag);
  switch (tag) {
  case UVAR: case SVAR: return PROCEED;
  default:              return FAILED;
  }
}

DECLAREBI_USEINLINEREL1(BIisFreeRel,isFreeRelInline)
DECLAREBOOLFUN1(BIisFree,isFreeInline,isFreeRelInline)

OZ_Return isKindedRelInline(TaggedRef term) {
  DEREF(term, _1, tag);
  return isCVar(tag) ? PROCEED : FAILED;
}

DECLAREBI_USEINLINEREL1(BIisKindedRel,isKindedRelInline)
DECLAREBOOLFUN1(BIisKinded,isKindedInline,isKindedRelInline)

OZ_Return isDetRelInline(TaggedRef term) {
  DEREF(term, _1, tag);
  return isAnyVar(tag) ? FAILED : PROCEED;
}

DECLAREBI_USEINLINEREL1(BIisDetRel,isDetRelInline)
DECLAREBOOLFUN1(BIisDet,isDetInline,isDetRelInline)



OZ_Return isNameInline(TaggedRef t)
{
  NONSUVAR( t, term, tag );
  if (isCVar(tag)) {
      switch (tagged2CVar(term)->getType()) {
      case OFSVariable:
        {
          GenOFSVariable *ofsvar=tagged2GenOFSVar(term);
          TaggedRef lbl=ofsvar->getLabel();
          Assert(lbl!=makeTaggedNULL());
          DEREF(lbl,_1,lblTag);
          if (isAtom(lbl)) return FAILED;
          if (ofsvar->getWidth()>0) return FAILED;
          return SUSPEND;
        }
      case FDVariable:
      case BoolVariable:
          return FAILED;
      default:
          return SUSPEND;
      }
  }
  if (!isLiteral(tag)) return FAILED;
  return isAtom(term) ? FAILED: PROCEED;
}

DECLAREBI_USEINLINEREL1(BIisName,isNameInline)
DECLAREBOOLFUN1(BIisNameB,isNameBInline,isNameInline)


OZ_Return isTupleInline(TaggedRef t)
{
  NONSUVAR( t, term, tag );
  if (isCVar(tag)) {
      switch (tagged2CVar(term)->getType()) {
      case FDVariable:
      case BoolVariable:
          return FAILED;
      default:
          return SUSPEND;
      }
  }
  return isTuple(term) ? PROCEED : FAILED;
}

DECLAREBI_USEINLINEREL1(BIisTuple,isTupleInline)
DECLAREBOOLFUN1(BIisTupleB,isTupleBInline,isTupleInline)


OZ_Return isRecordInline(TaggedRef t)
{
  NONSUVAR( t, term, tag );
  if (isCVar(tag)) {
      switch (tagged2CVar(term)->getType()) {
      case FDVariable:
      case BoolVariable:
          return FAILED;
      default:
          return SUSPEND;
      }
  }
  return isRecord(term) ? PROCEED : FAILED;
}

DECLAREBI_USEINLINEREL1(BIisRecord,isRecordInline)
DECLAREBOOLFUN1(BIisRecordB,isRecordBInline,isRecordInline)


OZ_Return isProcedureInline(TaggedRef t)
{
  NONVAR( t, term);
  return isProcedure(term) ? PROCEED : FAILED;
}

DECLAREBI_USEINLINEREL1(BIisProcedure,isProcedureInline)
DECLAREBOOLFUN1(BIisProcedureB,isProcedureBInline,isProcedureInline)

OZ_Return isChunkInline(TaggedRef t)
{
  NONSUVAR( t, term, tag );
  if (isCVar(tag)) {
      switch (tagged2CVar(term)->getType()) {
      case OFSVariable:
      case FDVariable:
      case BoolVariable:
          return FAILED;
      default:
          return SUSPEND;
      }
  }
  return isChunk(term) ? PROCEED : FAILED;
}
DECLAREBI_USEINLINEREL1(BIisChunk,isChunkInline)
DECLAREBOOLFUN1(BIisChunkB,isChunkBInline,isChunkInline)

OZ_Return procedureArityInline(TaggedRef procedure, TaggedRef &out)
{
  NONVAR( procedure, pterm );

  if (isProcedure(pterm)) {
    int arity;
    ConstTerm *rec = tagged2Const(pterm);

    switch (rec->getType()) {
    case Co_Abstraction:
      arity = ((Abstraction *) rec)->getArity();
      break;
    case Co_Builtin:
      arity = ((BuiltinTabEntry *) rec)->getArity();
      break;
    default:
      goto typeError;
    }
    out = newSmallInt(arity);
    return PROCEED;
  }
  goto typeError;

typeError:
  out = nil();
  oz_typeError(0,"Procedure");
}

DECLAREBI_USEINLINEFUN1(BIprocedureArity,procedureArityInline)

OZ_C_proc_begin(BIprocedureEnvironment,2)
{
  oz_declareNonvarArg(0,p);
  oz_declareArg(1,out);

  if (!isProcedure(p)) {
    oz_typeError(0,"Procedure");
  }

  OZ_Term t;

  if (isBuiltin(p)) {
    t = OZ_atom("environment");
  } else {
    Abstraction *a=tagged2Abstraction(p);

    RefsArray &g=a->getGRegs();
    if (g) {
      int len=a->getGSize();
      t = OZ_tuple(OZ_atom("environment"),len);
      for (int i=0; i<len; i++) OZ_putArg(t,i,g[i]);
    } else {
      t = OZ_atom("environment");
    }
  }
  return oz_unify(out,t);
}
OZ_C_proc_end


OZ_C_proc_begin(BIgetProcInfo,2)
{
  oz_declareNonvarArg(0,p); p = deref(p);
  oz_declareArg(1,out);

  if (!isAbstraction(p)) {
    oz_typeError(0,"Abstraction");
  }

  return oz_unify(out,tagged2Abstraction(p)->getPred()->getInfo());
}
OZ_C_proc_end

OZ_C_proc_begin(BIsetProcInfo,2)
{
  oz_declareNonvarArg(0,p); p = deref(p);
  oz_declareArg(1,t);

  if (!isAbstraction(p)) {
    oz_typeError(0,"Abstraction");
  }

  tagged2Abstraction(p)->getPred()->setInfo(t);
  return PROCEED;
}
OZ_C_proc_end


OZ_C_proc_begin(BIgetProcNames,2)
{
  oz_declareNonvarArg(0,p); p = deref(p);
  oz_declareArg(1,out);

  if (!isAbstraction(p)) {
    oz_typeError(0,"Abstraction");
  }

  return oz_unify(out,tagged2Abstraction(p)->getPred()->getNames());
}
OZ_C_proc_end

OZ_C_proc_begin(BIsetProcNames,2)
{
  oz_declareNonvarArg(0,p); p = deref(p);
  oz_declareArg(1,t);

  if (!isAbstraction(p)) {
    oz_typeError(0,"Abstraction");
  }

  tagged2Abstraction(p)->getPred()->setNames(t);
  return PROCEED;
}
OZ_C_proc_end


OZ_C_proc_begin(BIgetProcPos,3)
{
  oz_declareNonvarArg(0,p); p = deref(p);
  oz_declareArg(1,file);
  oz_declareArg(2,line);

  if (!isAbstraction(p)) {
    oz_typeError(0,"Abstraction");
  }

  PrTabEntry *pte = tagged2Abstraction(p)->getPred();

  return oz_unify(file,pte->getFileName()) && oz_unifyInt(line,pte->getLine());
}
OZ_C_proc_end


OZ_Return isCellInline(TaggedRef cell)
{
  NONVAR( cell, term);
  return isCell(term) ? PROCEED : FAILED;
}
DECLAREBI_USEINLINEREL1(BIisCell,isCellInline)
DECLAREBOOLFUN1(BIisCellB,isCellBInline,isCellInline)

OZ_Return isPortInline(TaggedRef port)
{
  NONVAR( port, term );
  return isPort(term) ? PROCEED : FAILED;
}
DECLAREBI_USEINLINEREL1(BIisPort,isPortInline)
DECLAREBOOLFUN1(BIisPortB,isPortBInline,isPortInline)

/*********************************************************************
 * OFS Records
 *********************************************************************/

/*
 * Constrain term to a record, with given label (wait until
 * determined), with an initial size sufficient for at least tNumFeats
 * features.  If term is already a record, do nothing.
 */

OZ_C_proc_begin(BIsystemTellSize,3)
{
  TaggedRef label = OZ_getCArg(0);
  TaggedRef tNumFeats = OZ_getCArg(1);
  TaggedRef t = OZ_getCArg(2);

  // Wait for label
  DEREF(label,labelPtr,labelTag);
  DEREF(t, tPtr, tag);

  /* most probable case first */
  if (isLiteral(labelTag) && isNotCVar(tag)) {
    DEREF(tNumFeats, nPtr, ntag);
    if (!isSmallInt(tNumFeats)) oz_typeError(1,"Int");
    dt_index numFeats=smallIntValue(tNumFeats);
    dt_index size=ceilPwrTwo((numFeats<=FILLLIMIT) ? numFeats
                             : (int)ceil((double)numFeats/FILLFACTOR));
    GenOFSVariable *newofsvar=new GenOFSVariable(label,size);
    Bool ok=am.unify(makeTaggedRef(newTaggedCVar(newofsvar)),
                     makeTaggedRef(tPtr));
    Assert(ok);
    return PROCEED;
  }

  switch (labelTag) {
  case LTUPLE:
  case SRECORD:
    oz_typeError(0,"Literal");
  case LITERAL:
    break;
  case UVAR:
  case SVAR:
    oz_suspendOn (makeTaggedRef(labelPtr));
  case CVAR:
    switch (tagged2CVar(label)->getType()) {
    case OFSVariable:
      {
        GenOFSVariable *ofsvar=tagged2GenOFSVar(label);
        if (ofsvar->getWidth()>0) return FAILED;
       oz_suspendOn (makeTaggedRef(labelPtr));
      }
    case FDVariable:
    case BoolVariable:
      oz_typeError(0,"Literal");
    default:
      oz_suspendOn (makeTaggedRef(labelPtr));
    }
  default:
    oz_typeError(0,"Literal");
  }

  Assert(labelTag == LITERAL);

  // Create record:
  switch (tag) {
  case LTUPLE:
    return literalEq(label, AtomCons) ? PROCEED : FAILED;
  case LITERAL:
    return literalEq(label, t) ? PROCEED : FAILED;
  case SRECORD:
    return literalEq(label, tagged2SRecord(t)->getLabel()) ? PROCEED : FAILED;
  case CVAR:
    if (tagged2CVar(t)->getType()==OFSVariable) {
       OZ_Return ret=oz_unify(tagged2GenOFSVar(t)->getLabel(),label);
       tagged2GenOFSVar(t)->propagateOFS();
       return ret;
    }
    // else fall through to creation case
  case UVAR:
  case SVAR:
    {
      // Calculate initial size of hash table:
      DEREF(tNumFeats, nPtr, ntag);
      if (!isSmallInt(tNumFeats)) oz_typeError(1,"Int");
      dt_index numFeats=smallIntValue(tNumFeats);
      dt_index size=ceilPwrTwo((numFeats<=FILLLIMIT) ? numFeats
                                                     : (int)ceil((double)numFeats/FILLFACTOR));
      // Create newofsvar with unbound variable as label & given initial size:
      GenOFSVariable *newofsvar=new GenOFSVariable(label,size);
      // Unify newofsvar and term:
      Bool ok=am.unify(makeTaggedRef(newTaggedCVar(newofsvar)),
                       makeTaggedRef(tPtr));
      Assert(ok);
      return PROCEED;
    }
  default:
    return FAILED;
  }
}
OZ_C_proc_end


// Constrain term to a record, with given label (wait until determined).
OZ_C_proc_begin(BIrecordTell,2)
{
  TaggedRef label = OZ_getCArg(0);
  TaggedRef t = OZ_getCArg(1);

  // Wait for label
  DEREF(t, tPtr, tag);
  DEREF(label,labelPtr,labelTag);

  /* most probable case first */
  if (isLiteral(labelTag) && isNotCVar(tag)) {
    GenOFSVariable *newofsvar=new GenOFSVariable(label);
    Bool ok=am.unify(makeTaggedRef(newTaggedCVar(newofsvar)),
                     makeTaggedRef(tPtr));
    Assert(ok);
    return PROCEED;
  }

  switch (labelTag) {
  case LTUPLE:
  case SRECORD:
    oz_typeError(0,"Literal");
  case LITERAL:
    break;
  case UVAR:
  case SVAR:
    oz_suspendOn (makeTaggedRef(labelPtr));
  case CVAR:
    switch (tagged2CVar(label)->getType()) {
    case OFSVariable:
      {
        GenOFSVariable *ofsvar=tagged2GenOFSVar(label);
        if (ofsvar->getWidth()>0) return FAILED;
        oz_suspendOn (makeTaggedRef(labelPtr));
      }
    case FDVariable:
    case BoolVariable:
      oz_typeError(0,"Literal");
    default:
      oz_suspendOn (makeTaggedRef(labelPtr));
    }
  default:
    oz_typeError(0,"Literal");
  }

  Assert(labelTag == LITERAL);
  // Create record:
  switch (tag) {
  case LTUPLE:
    return literalEq(label, AtomCons) ? PROCEED : FAILED;
  case LITERAL:
    return literalEq(label, t) ? PROCEED : FAILED;
  case SRECORD:
    return literalEq(label, tagged2SRecord(t)->getLabel()) ? PROCEED : FAILED;
  case CVAR:
    if (tagged2CVar(t)->getType()==OFSVariable) {
       OZ_Return ret=oz_unify(tagged2GenOFSVar(t)->getLabel(),label);
       tagged2GenOFSVar(t)->propagateOFS();
       return ret;
    }
    oz_typeError(0,"Record");
    // else fall through to creation case
  case UVAR:
  case SVAR:
    {
      // Create newofsvar with unbound variable as label & given initial size:
      GenOFSVariable *newofsvar=new GenOFSVariable(label);
      // Unify newofsvar and term:
      Bool ok=am.unify(makeTaggedRef(newTaggedCVar(newofsvar)),
                       makeTaggedRef(tPtr));
      Assert(ok);
      return PROCEED;
    }
  default:
    return FAILED;
  }
}
OZ_C_proc_end


// Suspend until can determine whether term is a record or not.
// This routine extends isRecordInline to accept undetermined records.
OZ_Return isRecordCInline(TaggedRef t)
{
  DEREF(t, tPtr, tag);
  switch (tag) {
  case LTUPLE:
  case LITERAL:
  case SRECORD:
    break;
  case CVAR:
    switch (tagged2CVar(t)->getType()) {
    case OFSVariable:
        break;
    case FDVariable:
    case BoolVariable:
        return FAILED;
    default:
        return SUSPEND;
    }
    break;
  case UVAR:
  case SVAR:
    return SUSPEND;
  default:
    return FAILED;
  }
  return PROCEED;
}

DECLAREBI_USEINLINEREL1(BIisRecordC,isRecordCInline)
DECLAREBOOLFUN1(BIisRecordCB,isRecordCBInline,isRecordCInline)



//
OZ_C_proc_begin(BIisRecordCVarB,2)
{
  TaggedRef t = OZ_getCArg(0);
  DEREF(t, tPtr, tag);
  switch (tag) {
  case LTUPLE:
  case LITERAL:
  case SRECORD:
    break;
  case CVAR:
    if (tagged2CVar(t)->getType()!=OFSVariable)
      return (oz_unify (OZ_getCArg (1), NameFalse));
    break;
  case UVAR:
  case SVAR:
    return (oz_unify (OZ_getCArg (1), NameFalse));
  default:
    return (oz_unify (OZ_getCArg (1), NameFalse));
  }
  return (oz_unify (OZ_getCArg (1), NameTrue));
}
OZ_C_proc_end


/*
 * {RecordC.widthC X W} -- builtin that constrains number of features
 * of X to be equal to finite domain variable W.  Will constrain X to
 * a record and W to a finite domain.  This built-in installs a
 * WidthPropagator.
 */
OZ_C_proc_begin(BIwidthC, 2)
{
    OZ_EXPECTED_TYPE("record,finite domain");

    TaggedRef rawrec=OZ_getCArg(0);
    TaggedRef rawwid=OZ_getCArg(1);
    TaggedRef rec=OZ_getCArg(0);
    TaggedRef wid=OZ_getCArg(1);
    DEREF(rec, recPtr, recTag);
    DEREF(wid, widPtr, widTag);

    // Wait until first argument is a constrained record (OFS, SRECORD, LTUPLE, LITERAL):
    switch (recTag) {
    case UVAR:
    case SVAR:
      oz_suspendOn(rawrec);
    case CVAR:
      switch (tagged2CVar(rec)->getType()) {
      case OFSVariable:
          break;
      case FDVariable:
      case BoolVariable:
          oz_typeError(0,"Record");
      default:
          oz_suspendOn(rawrec);
      }
      break;
    case SRECORD:
    case LITERAL:
    case LTUPLE:
      break;
    default:
      oz_typeError(0,"Record");
    }

    // Ensure that second argument wid is a FD or integer:
    switch (widTag) {
    case UVAR:
    case SVAR:
    {
        // Create new fdvar:
        GenFDVariable *fdvar=new GenFDVariable(); // Variable with maximal domain
        // Unify fdvar and wid:
        Bool ok=am.unify(makeTaggedRef(newTaggedCVar(fdvar)),rawwid);
        Assert(ok);
        break;
    }
    case CVAR:
        if (tagged2CVar(wid)->getType()!=FDVariable) return FAILED;
        break;
    case BIGINT:
    case SMALLINT:
        break;
    default:
        return FAILED;
    }

    // This completes the propagation abilities of widthC.  However, entailment is
    // still hard, so this rule will not be added now--we'll wait until people need it.
    //   // If propagator exists already on the variable, just unify the widths
    //   // Implements rule: width(x,w1)/\width(x,w2) -> width(x,w1)/\(w1=w2)
    //   if (recTag==CVAR) {
    //       TaggedRef otherwid=am.getWidthSuspension((void*)BIpropWidth,rec);
    //       if (otherwid!=makeTaggedNULL()) {
    //           return (am.unify(otherwid,rawwid) ? PROCEED : FAILED);
    //       }
    //   }

    OZ_Expect pe;
    OZ_EXPECT(pe, 0, expectRecordVar);
    OZ_EXPECT(pe, 1, expectIntVar);

    return pe.impose(new WidthPropagator(rawrec, rawwid)); // oz_args[0], oz_args[1]));
}
OZ_C_proc_end

OZ_CFunHeader WidthPropagator::spawner = BIwidthC;

// Used by BIwidthC built-in
// {PropWidth X W} where X is OFS and W is FD width.
// Assume: rec is OFS or SRECORD or LITERAL.
// Assume: wid is FD or SMALLINT or BIGINT.
// This is the simplest most straightforward possible
// implementation and it can be optimized in many ways.
OZ_Return WidthPropagator::propagate(void)
{
    int recwidth;
    OZ_Return result = SLEEP;

    TaggedRef rec=rawrec;
    TaggedRef wid=rawwid;
    DEREF(rec, recptr, recTag);
    DEREF(wid, widptr, widTag);

    switch (recTag) {
    case SRECORD:
    record:
    case LITERAL:
    case LTUPLE:
    {
        // Impose width constraint
        recwidth=(recTag==SRECORD) ? tagged2SRecord(rec)->getWidth() :
                 ((recTag==LTUPLE) ? 2 : 0);
        if (isGenFDVar(wid)) {
            // GenFDVariable *fdwid=tagged2GenFDVar(wid);
            // res=fdwid->setSingleton(recwidth);
          Bool res=am.unify(makeTaggedSmallInt(recwidth),rawwid);
          if (!res) { result = FAILED; break; }
        } else if (isSmallInt(widTag)) {
            int intwid=smallIntValue(wid);
            if (recwidth!=intwid) { result = FAILED; break; }
        } else if (isBigInt(widTag)) {
            // BIGINT case: fail
            result = FAILED; break;
        } else {
            error("unexpected wrong type for width in determined widthC");
        }
        result = PROCEED;
        break;
    }
    case CVAR:
    {
        Assert(tagged2CVar(rec)->getType() == OFSVariable);
        // 1. Impose width constraint
        GenOFSVariable *recvar=tagged2GenOFSVar(rec);
        recwidth=recvar->getWidth(); // current actual width of record
        if (isGenFDVar(wid)) {
            // Build fd with domain recwidth..fd_sup:
            OZ_FiniteDomain slice;
            slice.initRange(recwidth,fd_sup);
            OZ_FiniteDomain &dom = tagged2GenFDVar(wid)->getDom();
            if (dom.getSize() > (dom & slice).getSize()) {
                GenFDVariable *fdcon=new GenFDVariable(slice);
                Bool res=am.unify(makeTaggedRef(newTaggedCVar(fdcon)),rawwid);
                // No loc/glob handling: res=(fdwid>=recwidth);
                if (!res) { result = FAILED; break; }
            }
        } else if (isSmallInt(widTag)) {
            int intwid=smallIntValue(wid);
            if (recwidth>intwid) { result = FAILED; break; }
        } else if (isBigInt(widTag)) {
            // BIGINT case: fail
            result = FAILED; break;
        } else {
            error("unexpected wrong type for width in undetermined widthC");
        }
        // 2. Convert representation if necessary
        // 2a. Find size and value (latter is valid only if goodsize==TRUE):
        int goodsize,value;
        DEREF(wid,_3,newwidTag);
        if (isGenFDVar(wid)) {
            GenFDVariable *newfdwid=tagged2GenFDVar(wid);
            goodsize=(newfdwid->getDom().getSize())==1;
            value=newfdwid->getDom().getMinElem();
        } else if (isSmallInt(newwidTag)) {
            goodsize=TRUE;
            value=smallIntValue(wid);
        } else {
            goodsize=FALSE;
            value = 0; // make gcc quiet
        }
        // 2b. If size==1 and all features and label present,
        //     then convert to SRECORD or LITERAL:
        if (goodsize && value==recwidth) {
            TaggedRef lbl=tagged2GenOFSVar(rec)->getLabel();
            DEREF(lbl,_4,lblTag);
            if (isLiteral(lblTag)) {
                result = PROCEED;
                if (recwidth==0) {
                    // Convert to LITERAL:
                  Bool res=am.unify(rawrec,lbl);
                  if (!res) error("unexpected failure of Literal conversion");
                } else {
                    // Convert to SRECORD or LTUPLE:
                    // (Two efficiency problems: 1. Creates record & then unifies,
                    // instead of creating & only binding.  2. Rec->normalize()
                    // wastes the space of the original record.)
                    TaggedRef alist=tagged2GenOFSVar(rec)->getTable()->getArityList();
                    Arity *arity=aritytable.find(alist);
                    SRecord *newrec = SRecord::newSRecord(lbl,arity);
                    newrec->initArgs();
                    Bool res=am.unify(rawrec,newrec->normalize());
                    Assert(res);
                }
            }
        }
        break;
    }
    default:
      // mm2: type error ?
      warning("unexpected bad first argument to widthC");
      result=FAILED;
    }

    return (result);
}



// {RecordC.monitorArity X K L} -- builtin that tracks features added to OFS X
// in the list L.  Goes away if K is determined (if K is determined on first call,
// L is list of current features).  monitorArity imposes that X is a record (like
// RecordC) and hence fails if X is not a record.
OZ_C_proc_begin(BImonitorArity, 3)
{
    OZ_EXPECTED_TYPE("any(record),any,any(list)");

    OZ_Term rec = OZ_getCArg(0);
    OZ_Term kill = OZ_getCArg(1);
    OZ_Term arity = OZ_getCArg(2);

    OZ_Term tmpkill=OZ_getCArg(1);
    DEREF(tmpkill,_1,killTag);
    Bool isKilled = !isAnyVar(killTag);

    OZ_Term tmprec=OZ_getCArg(0);
    DEREF(tmprec,_2,recTag);
    switch (recTag) {
    case LTUPLE:
        return am.unify(arity,makeTupleArityList(2)) ? PROCEED : FAILED;
    case LITERAL:
        // *** arity is nil
        return (am.unify(arity,AtomNil)? PROCEED : FAILED);
    case SRECORD:
    record:
        // *** arity is known set of features of the SRecord
        return am.unify(arity,tagged2SRecord(tmprec)->getArityList()) ? PROCEED : FAILED;
    case UVAR:
    case SVAR:
        oz_suspendOn(rec);
    case CVAR:
        switch (tagged2CVar(tmprec)->getType()) {
        case OFSVariable:
            break;
        case FDVariable:
        case BoolVariable:
            oz_typeError(0,"Record");
        default:
            oz_suspendOn(rec);
        }
        // *** arity is calculated from the OFS; see below
        break;
    default:
        oz_typeError(0,"Record");
    }
    tmprec=OZ_getCArg(0);
    DEREF(tmprec,_3,_4);

    // At this point, rec is OFS and tmprec is dereferenced and undetermined

    if (isKilled) {
        TaggedRef featlist;
        featlist=tagged2GenOFSVar(tmprec)->getArityList();

        return (am.unify(arity,featlist)? PROCEED : FAILED);
    } else {
        TaggedRef featlist;
        TaggedRef feattail;
        Board *home=am.currentBoard();
        featlist=tagged2GenOFSVar(tmprec)->getOpenArityList(&feattail,home);

        if (!am.unify(featlist,arity)) return FAILED;

        OZ_Expect pe;
        OZ_EXPECT(pe, 0, expectRecordVar);
        OZ_EXPECT(pe, 1, expectVar);

        TaggedRef uvar=makeTaggedRef(newTaggedUVar(home));
        return pe.impose(
            new MonitorArityPropagator(rec,kill,feattail,uvar,uvar),
            OZ_getMediumPrio(),
            OFS_flag);
    }

    return PROCEED;
}
OZ_C_proc_end // BImonitorArity

OZ_CFunHeader MonitorArityPropagator::spawner = BImonitorArity;

// The propagator for the built-in RecordC.monitorArity
// {PropFeat X K L FH FT} -- propagate features from X to the list L, and go
// away when K is determined.  L is closed when K is determined.  X is used to
// check in addFeatOFSSuspList that the suspension is waiting for the right
// variable.  FH and FT are a difference list that holds the features that
// have been added.
OZ_Return MonitorArityPropagator::propagate(void)
{
    // Check if killed:
    TaggedRef kill=K;
    TaggedRef tmpkill=kill;
    DEREF(tmpkill,_2,killTag);
    Bool isKilled = !isAnyVar(killTag);

    TaggedRef tmptail=FT;
    DEREF(tmptail,_3,_4);

    // Get featlist (a difference list stored in the arguments):
    TaggedRef fhead = FH;
    TaggedRef ftail = FT;

    if (tmptail!=AtomNil) {
        // The record is not determined, so reinitialize the featlist:
        // The home board of uvar must be taken from outside propFeat!
        // Get the home board for any new variables:
        Board* home=tagged2VarHome(tmptail);
        TaggedRef uvar=makeTaggedRef(newTaggedUVar(home));
        FH=uvar;
        FT=uvar;
    } else {
        // Precaution for the GC?
        FH=makeTaggedNULL();
        FT=makeTaggedNULL();
    }

    // Add the features to L (the tail of the output list)
    TaggedRef arity=L;
    if (!am.unify(fhead,arity)) return FAILED; // No further updating of the suspension
    L=ftail; // 'ftail' is the new L in the suspension

    if (tmptail!=AtomNil) {
        // The record is not determined, so the suspension is revived:
        if (!isKilled) return (SLEEP);
        else return (am.unify(ftail,AtomNil)? PROCEED : FAILED);
    }
    return PROCEED;
}





// Create new thread on suspension:
OZ_Return uparrowInlineNonBlocking(TaggedRef, TaggedRef, TaggedRef&);
DECLAREBI_USEINLINEFUN2(BIuparrowNonBlocking,uparrowInlineNonBlocking)

// Block current thread on suspension:
OZ_Return uparrowInlineBlocking(TaggedRef, TaggedRef, TaggedRef&);
DECLAREBI_USEINLINEFUN2(BIuparrowBlocking,uparrowInlineBlocking)


/*
 * NOTE: similar functions are dot, genericSet, uparrow
 */
// X^Y=Z: add feature Y to open feature structure X (Tell operation).
OZ_Return genericUparrowInline(TaggedRef term, TaggedRef fea, TaggedRef &out, Bool blocking)
{
    TaggedRef termOrig=term;
    TaggedRef feaOrig=fea;
    DEREF(term, termPtr, termTag);
    DEREF(fea,  feaPtr,  feaTag);
    int suspFlag=FALSE;

    // optimize the most common case: adding or reading a feature
    if (isCVar(termTag)) {
      if (tagged2CVar(term)->getType()!=OFSVariable) goto typeError1;
      if (isFeature(feaTag)) {
        GenOFSVariable *ofsvar=tagged2GenOFSVar(term);

        TaggedRef t=ofsvar->getFeatureValue(fea);
        if (t!=makeTaggedNULL()) {
          // Feature exists
          out=t;
          return PROCEED;
        }

        if (am.isCurrentBoard(GETBOARD(ofsvar))) {
          TaggedRef uvar=oz_newVariable();
          Bool ok=ofsvar->addFeatureValue(fea,uvar);
          Assert(ok);
          ofsvar->propagateOFS();
          out=uvar;
          return PROCEED;
        }
      }
    }

    // Wait until Y is a feature:
    if (isAnyVar(feaTag)) {
      if (isCVar(feaTag)) {
        if (tagged2CVar(fea)->getType()==OFSVariable) {
          GenOFSVariable *ofsvar=tagged2GenOFSVar(fea);
          if (ofsvar->getWidth()>0) goto typeError2;
        }
      }
      if (!isAnyVar(term) && !isRecord(term)) goto typeError2;

      if (blocking) {
        return SUSPEND;
      } else {
        if (isNotCVar(term)) {
          // Create newofsvar with unbound variable as label:
          GenOFSVariable *newofsvar=new GenOFSVariable();
          // Unify newofsvar and term:
          Bool ok=am.unify(makeTaggedRef(newTaggedCVar(newofsvar)),
                           makeTaggedRef(termPtr));
          Assert(ok);
          term=makeTaggedRef(termPtr);
          DEREF(term, termPtr2, tag2);
          termPtr=termPtr2;
          termTag=tag2;
        }
        // Create thread containing relational blocking version of uparrow:
        RefsArray x=allocateRefsArray(3, NO);
        out=oz_newVariable();
        x[0]=termOrig;
        x[1]=feaOrig;
        x[2]=out;
        OZ_Thread thr=OZ_makeSuspendedThread(BIuparrowBlocking,x,3);
        OZ_addThread(feaOrig,thr);
        return PROCEED;
      }
    }
    if (!isFeature(feaTag)) goto typeError2;

    // Add feature and return:
    Assert(term!=makeTaggedNULL());
    switch (termTag) {
    case CVAR:
      {
        Assert(tagged2CVar(term)->getType() == OFSVariable);
        GenOFSVariable *ofsvar=tagged2GenOFSVar(term);
        TaggedRef t=ofsvar->getFeatureValue(fea);
        if (t!=makeTaggedNULL()) {
            // Feature exists
            out=t;
        } else {
            // Feature does not yet exist
            // Add feature by (1) creating new ofsvar with one feature,
            // (2) unifying the new ofsvar with the old.
          if (am.isCurrentBoard(GETBOARD(ofsvar))) {
                // Optimization:
                // If current board is same as ofsvar board then can add feature directly
                TaggedRef uvar=oz_newVariable();
                Bool ok=ofsvar->addFeatureValue(fea,uvar);
                Assert(ok);
                ofsvar->propagateOFS();
                out=uvar;
            } else {
                // Create newofsvar:
                GenOFSVariable *newofsvar=new GenOFSVariable();
                // Add feature to newofsvar:
                TaggedRef uvar=oz_newVariable();
                Bool ok1=newofsvar->addFeatureValue(fea,uvar);
                Assert(ok1);
                out=uvar;
                // Unify newofsvar and term (which is also an ofsvar):
                Bool ok2=am.unify(makeTaggedRef(newTaggedCVar(newofsvar)),
                                  makeTaggedRef(termPtr));
                Assert(ok2);
            }
        }
        return PROCEED;
      }

    case UVAR:
    case SVAR:
      {
        // Create newofsvar:
        GenOFSVariable *newofsvar=new GenOFSVariable();
        // Add feature to newofsvar:
        TaggedRef uvar=oz_newVariable();
        Bool ok1=newofsvar->addFeatureValue(fea,uvar);
        Assert(ok1);
        out=uvar;
        // Unify newofsvar (CVAR) and term (SVAR or UVAR):
        Bool ok2=am.unify(makeTaggedRef(newTaggedCVar(newofsvar)),
                          makeTaggedRef(termPtr));
        Assert(ok2);
        return PROCEED;
      }

    case SRECORD:
    record:
      {
        // Get the SRecord corresponding to term:
        SRecord* termSRec=makeRecord(term);

        TaggedRef t=termSRec->getFeature(fea);
        if (t!=makeTaggedNULL()) {
            out=t;
            return PROCEED;
        }
        return FAILED;
      }

    case LTUPLE:
      {
        if (!isSmallInt(fea)) return FAILED;
        int i2 = smallIntValue(fea);
        switch (i2) {
        case 1:
          out=tagged2LTuple(term)->getHead();
          return PROCEED;
        case 2:
          out=tagged2LTuple(term)->getTail();
          return PROCEED;
        }
        return FAILED;
      }

    case LITERAL:
        return FAILED;

    default:
        goto typeError1;
    }
typeError1:
    oz_typeError(0,"Record");
typeError2:
    oz_typeError(1,"Feature");
}


OZ_Return uparrowInlineNonBlocking(TaggedRef term, TaggedRef fea,
                                   TaggedRef &out)
{
    return genericUparrowInline(term, fea, out, FALSE);
}

OZ_Return uparrowInlineBlocking(TaggedRef term, TaggedRef fea, TaggedRef &out)
{
    return genericUparrowInline(term, fea, out, TRUE);
}

// ---------------------------------------------------------------------
// Spaces
// ---------------------------------------------------------------------

#define declareSpace()                                  \
  OZ_Term tagged_space = OZ_getCArg(0);                 \
  DEREF(tagged_space, space_ptr, space_tag);            \
  if (isAnyVar(space_tag))                              \
    oz_suspendOn(makeTaggedRef(space_ptr));             \
  if (!isSpace(tagged_space))                           \
    oz_typeError(0, "Space");                           \
  Space *space = (Space *) tagged2Const(tagged_space);


OZ_C_proc_begin(BInewSpace, 2) {
  OZ_Term proc = OZ_getCArg(0);

  DEREF(proc, proc_ptr, proc_tag);
  if (isAnyVar(proc_tag))
    oz_suspendOn(makeTaggedRef(proc_ptr));

  if (!isProcedure(proc))
    oz_typeError(0, "Procedure");

  Board* CBB = am.currentBoard();

  ozstat.incSolveCreated();
  // creation of solve actor and solve board
  SolveActor *sa = new SolveActor(CBB);

  // thread creation for {proc root}
  sa->inject(DEFAULT_PRIORITY, proc);

  // create space
  OZ_Return s = oz_unify(OZ_getCArg(1),
                         makeTaggedConst(new Space(CBB,sa->getSolveBoard())));

  if (s == PROCEED)
    return BI_PREEMPT;

  return s;

} OZ_C_proc_end


OZ_C_proc_begin(BIisSpace, 2) {
  OZ_Term tagged_space = OZ_getCArg(0);

  DEREF(tagged_space, space_ptr, space_tag);

  if (isAnyVar(space_tag))
    oz_suspendOn(makeTaggedRef(space_ptr));

  return oz_unify(OZ_getCArg(1), isSpace(tagged_space) ? NameTrue : NameFalse);
} OZ_C_proc_end


OZ_C_proc_begin(BIaskSpace, 2) {
  declareSpace();
  oz_declareArg(1,out);

  if (space->isProxy()) {
    return remoteSend(space,"Space.ask",out);
  }

  if (space->isFailed())
    return oz_unify(out, AtomFailed);

  if (space->isMerged())
    return oz_unify(out, AtomMerged);

  TaggedRef answer = space->getSolveActor()->getResult();

  DEREF(answer, answer_ptr, answer_tag);

  if (isAnyVar(answer_tag))
    oz_suspendOn(makeTaggedRef(answer_ptr));

  return oz_unify(out,
                  (isSTuple(answer) &&
                   literalEq(tagged2SRecord(answer)->getLabel(),
                             AtomSucceeded))
                    ? AtomSucceeded : answer);
} OZ_C_proc_end


OZ_C_proc_begin(BIaskVerboseSpace, 2) {
  declareSpace();
  oz_declareArg(1,out);

  if (space->isProxy()) {
    return remoteSend(space,"Space.askVerbose",out);
  }

  if (space->isFailed())
    return oz_unify(out, AtomFailed);

  if (space->isMerged())
    return oz_unify(out, AtomMerged);

  if (space->getSolveActor()->isBlocked()) {
    SRecord *stuple = SRecord::newSRecord(AtomBlocked, 1);
    stuple->setArg(0, am.currentUVarPrototype());

    if (oz_unify(out, makeTaggedSRecord(stuple)) == FAILED)
      return FAILED;

    OZ_args[1] = stuple->getArg(0);
  }

  TaggedRef answer = space->getSolveActor()->getResult();

  DEREF(answer, answer_ptr, answer_tag);

  if (isAnyVar(answer_tag))
    oz_suspendOn(makeTaggedRef(answer_ptr));

  return oz_unify(OZ_args[1], answer);
} OZ_C_proc_end


OZ_C_proc_begin(BImergeSpace, 2) {
  declareSpace();
  oz_declareArg(1,out);

  if (space->isProxy()) {
    return remoteSend(space,"Space.merge",out);
  }

  if (space->isMerged())
    return oz_raise(E_ERROR,E_KERNEL,"spaceMerged",1,tagged_space);

  if (space->isFailed())
    return FAILED;

  Board *CBB = am.currentBoard();
  Board *SBB = space->getSolveBoard()->derefBoard();
  Board *SBP = SBB->getParent()->derefBoard();

  /* There can be two different situations during merging:
   *  1) SBB is subordinated to CBB:          CBB  <-+
   *                                           |     |
   *                                          SBB   -+
   *
   *
   *  2) SBB is a sibling of CBB:            parent
   *                                          /   \
   *                                        CBB   SBB
   *                                         ^     |
   *                                         +-----+
   */

  Assert(CBB == CBB->derefBoard());

  Bool isSibling = (!am.isRootBoard(CBB) &&
                    CBB->getParent()->derefBoard() == SBP &&
                    CBB != SBB);

  if (!isSibling && CBB != SBP)
    return oz_raise(E_ERROR,E_KERNEL,"spaceSuper",1,tagged_space);

  Assert(!am.isBelow(CBB,SBB));

  TaggedRef result = space->getSolveActor()->getResult();

  if (result == makeTaggedNULL())
    return FAILED;

  if (OZ_isVariable(result)) {

    if (isSibling) {
      switch (am.installPath(SBP)) {
      case INST_FAILED: case INST_REJECTED: return FAILED;
      case INST_OK: break;
      }

      if (OZ_unify(result, AtomMerged) == FAILED)
        return FAILED;

      switch (am.installPath(CBB)) {
      case INST_FAILED: case INST_REJECTED: return FAILED;
      case INST_OK: break;
      }

    } else {
      if (OZ_unify(result, AtomMerged) == FAILED)
        return FAILED;
    }
  }


  TaggedRef root = space->getSolveActor()->merge(CBB, isSibling);
  space->merge();

  if (root == makeTaggedNULL())
    return FAILED;

  return OZ_unify(root, out);
} OZ_C_proc_end


OZ_C_proc_begin(BIcloneSpace, 2) {
  declareSpace();

  oz_declareArg(1,out);

  if (space->isProxy()) {
    return remoteSend(space,"Space.clone",out);
  }

  if (space->isMerged())
    return oz_raise(E_ERROR,E_KERNEL,"spaceMerged",1,tagged_space);

  Board* CBB = am.currentBoard();

  if (space->isFailed())
    return oz_unify(out,
                    makeTaggedConst(new Space(CBB, (Board *) 0)));

  TaggedRef result = space->getSolveActor()->getResult();

  DEREF(result, result_ptr, result_tag);
  if (isAnyVar(result_tag))
    oz_suspendOn(makeTaggedRef(result_ptr));

  return oz_unify(out,
                  makeTaggedConst(new Space(CBB,
                                            space->getSolveActor()->clone(CBB))));

} OZ_C_proc_end


OZ_C_proc_begin(BIcommitSpace, 2) {
  declareSpace();
  oz_declareArg(1,choice);

  if (space->isProxy()) {
    return remoteSend(space,"Space.commit",choice);
  }

  if (space->isMerged())
    return oz_raise(E_ERROR,E_KERNEL,"spaceMerged",1,tagged_space);

  if (space->isFailed())
    return PROCEED;

  TaggedRef result = space->getSolveActor()->getResult();

  DEREF(result, result_ptr, result_tag);
  if (isAnyVar(result_tag))
    oz_suspendOn(makeTaggedRef(result_ptr));

  DEREF(choice, choice_ptr, choice_tag);

  if (isAnyVar(choice_tag))
    oz_suspendOn(makeTaggedRef(choice_ptr));

  TaggedRef left, right;

  if (isSmallInt(choice_tag)) {
    left  = choice;
    right = choice;
  } else if (isSTuple(choice) &&
             literalEq(AtomPair,
                       tagged2SRecord(choice)->getLabel()) &&
             tagged2SRecord(choice)->getWidth() == 2) {
    left  = tagged2SRecord(choice)->getArg(0);
    DEREF(left, left_ptr, left_tag);

    if (isAnyVar(left_tag))
      oz_suspendOn(makeTaggedRef(left_ptr));

    right = tagged2SRecord(choice)->getArg(1);

    DEREF(right, right_ptr, right_tag);

    if (isAnyVar(right_tag))
      oz_suspendOn(makeTaggedRef(right_ptr));
  } else {
    oz_typeError(1, "Integer or pair of integers");
  }

  if (!am.isCurrentBoard(space->getSolveBoard()->getParent()))
    return oz_raise(E_ERROR,E_KERNEL,"spaceParent",1,tagged_space);

  int l = smallIntValue(left) - 1;
  int r = smallIntValue(right) - 1;

  SolveActor *sa = space->getSolveActor();

  WaitActor *wa = sa->select(l,r);

  if (!wa)
    return oz_raise(E_ERROR,E_KERNEL,"spaceNoChoice",1,tagged_space);

  int n = wa->getChildCount();

  if (n>1) {
    sa->patchChoiceResult(n);

    return PROCEED;
  }

  if (n==1)
    sa->removeChoice();

  Thread *tt = wa->getThread();

  sa->unsetGround();
  sa->clearResult(GETBOARD(space));

  am.suspThreadToRunnable(tt);
  am.scheduleThread(tt);

  return BI_PREEMPT;
} OZ_C_proc_end


OZ_C_proc_begin(BIinjectSpace, 2)
{
  declareSpace();
  oz_declareArg(1,proc);

  if (space->isProxy()) {
    return remoteSend(space,"Space.inject",proc);
  }

  if (space->isMerged())
    return oz_raise(E_ERROR,E_KERNEL,"spaceMerged",1,tagged_space);

  // Check whether space is failed!
  if (space->isFailed())
    return PROCEED;

  if (!am.isCurrentBoard(space->getSolveBoard()->getParent()))
    return oz_raise(E_ERROR,E_KERNEL,"spaceParent", 1, tagged_space);

  DEREF(proc, proc_ptr, proc_tag);

  if (isAnyVar(proc_tag))
    oz_suspendOn(makeTaggedRef(proc_ptr));

  if (!isProcedure(proc))
    oz_typeError(1, "Procedure");

  Board      *sb = space->getSolveBoard();
  SolveActor *sa = space->getSolveActor();

  // clear status
  sa->unsetGround();
  sa->clearResult(GETBOARD(space));

  // inject
  sa->inject(DEFAULT_PRIORITY, proc);

  return BI_PREEMPT;
} OZ_C_proc_end


#ifdef CS_PROFILE
OZ_C_proc_begin(BIgetCloneDiff, 2) {
  declareSpace();

  if (space->isMerged())
    return oz_raise(E_ERROR,E_KERNEL,"spaceMerged",1,tagged_space);

  return OZ_unify(OZ_getCArg(1), space->getSolveActor()->getCloneDiff());
} OZ_C_proc_end

#endif


#undef declareSpace


// ---------------------------------------------------------------------
// Tuple
// ---------------------------------------------------------------------

OZ_Return tupleInline(TaggedRef label, TaggedRef argno, TaggedRef &out)
{
  DEREF(argno,_1,argnoTag);
  DEREF(label,_2,labelTag);

  if (isSmallInt(argnoTag)) {
    if (isLiteral(labelTag)) {
      int i = smallIntValue(argno);

      if (i < 0) {
        goto typeError1;
      }

      // literals
      if (i == 0) {
        out = label;
        return PROCEED;
      }

      {
        SRecord *s = SRecord::newSRecord(label,i);

        for (int j = 0; j < i; j++) {
          s->setArg(j,am.currentUVarPrototype());
        }

        out = s->normalize();
        return PROCEED;
      }
    }
    if (isAnyVar(labelTag)) {
      return SUSPEND;
    }
    goto typeError0;
  }
  if (isAnyVar(argnoTag)) {
    if (isAnyVar(labelTag) || isLiteral(labelTag)) {
      return SUSPEND;
    }
    goto typeError0;
  }
  goto typeError1;

 typeError0:
  oz_typeError(0,"Literal");
 typeError1:
  oz_typeError(1,"(non-negative small) Int");
}

DECLAREBI_USEINLINEFUN2(BItuple,tupleInline)


// ---------------------------------------------------------------------
// Tuple & Record
// ---------------------------------------------------------------------


OZ_Return labelInline(TaggedRef term, TaggedRef &out)
{
  // Wait for term to be a record with determined label:
  // Get the term's label, if it exists
  DEREF(term,_1,tag);
  switch (tag) {
  case LTUPLE:
    out=AtomCons;
    return PROCEED;
  case LITERAL:
    out=term;
    return PROCEED;
  case SRECORD:
  record:
    out=tagged2SRecord(term)->getLabel();
    return PROCEED;
  case UVAR:
  case SVAR:
    return SUSPEND;
  case CVAR:
    switch (tagged2CVar(term)->getType()) {
    case OFSVariable:
      {
        TaggedRef thelabel=tagged2GenOFSVar(term)->getLabel();
        DEREF(thelabel,_1,_2);
        if (isAnyVar(thelabel)) return SUSPEND;
        out=thelabel;
        return PROCEED;
      }
    case FDVariable:
    case BoolVariable:
        oz_typeError(0,"Record");
    default:
        return SUSPEND;
    }
  default:
    oz_typeError(0,"Record");
  }
}

DECLAREBI_USEINLINEFUN1(BIlabel,labelInline)

OZ_Return hasLabelInline(TaggedRef term, TaggedRef &out)
{
  // Wait for term to be a record with determined label:
  // Get the term's label, if it exists
  DEREF(term,_1,tag);
  switch (tag) {
  case LTUPLE:
    out=NameTrue;
    return PROCEED;
  case LITERAL:
    out=NameTrue;
    return PROCEED;
  case SRECORD:
  record:
    out=NameTrue;
    return PROCEED;
  case UVAR:
  case SVAR:
    out=NameFalse;
    return PROCEED;
  case CVAR:
    switch (tagged2CVar(term)->getType()) {
    case OFSVariable:
      {
        TaggedRef thelabel=tagged2GenOFSVar(term)->getLabel();
        DEREF(thelabel,_1,_2);
        out = isAnyVar(thelabel) ? NameFalse : NameTrue;
        return PROCEED;
      }
    case FDVariable:
    case BoolVariable:
      oz_typeError(0,"Record");
    default:
      out=NameFalse;
      return PROCEED;
    }
  default:
    oz_typeError(0,"Record");
  }
}

DECLAREBI_USEINLINEFUN1(BIhasLabel,hasLabelInline)

/*
 * NOTE: similar functions are dot, genericSet, uparrow
 */
OZ_Return genericDot(TaggedRef term, TaggedRef fea, TaggedRef *out, Bool dot)
{
  DEREF(fea, _1,feaTag);
LBLagain:
  DEREF(term, _2, termTag);

  if (isAnyVar(feaTag)) {
    switch (termTag) {
    case LTUPLE:
    case SRECORD:
    case SVAR:
    case UVAR:
      return SUSPEND;
    case CVAR:
      switch (tagged2CVar(term)->getType()) {
      case FDVariable:
      case BoolVariable:
          goto typeError0;
      default:
          return SUSPEND;
      }
      // if (tagged2CVar(term)->getType() == OFSVariable) return SUSPEND;
      // if (tagged2CVar(term)->getType() == AVAR) return SUSPEND;
      // goto typeError0;
    case LITERAL:
      goto typeError0;
    default:
      if (isChunk(term)) return SUSPEND;
      goto typeError0;
    }
  }

  if (!isFeature(feaTag)) goto typeError1;

  switch (termTag) {
  case LTUPLE:
    {
      if (!isSmallInt(fea)) {
        if (dot) goto raise; else return FAILED;
      }
      int i2 = smallIntValue(fea);

      if (i2 == 1) {
        if (out) *out = tagged2LTuple(term)->getHead();
        return PROCEED;
      }
      if (i2 == 2) {
        if (out) *out = tagged2LTuple(term)->getTail();
        return PROCEED;
      }

      if (dot) goto raise; else return FAILED;
    }

  case SRECORD:
    {
      TaggedRef t = tagged2SRecord(term)->getFeature(fea);
      if (t == makeTaggedNULL()) {
        if (dot) goto raise; else return FAILED;
      }
      if (out) *out = t;
      return PROCEED;
    }

  case UVAR:
  case SVAR:
    if (!isFeature(feaTag)) {
      oz_typeError(1,"Feature");
    }
    return SUSPEND;

  case CVAR:
    {
      int ret = tagged2CVar(term)->hasFeature(fea,out);
      if (ret == FAILED) goto typeError0;
      return ret;
    }

  case LITERAL:
    if (dot) goto raise; else return FAILED;

  default:
    if (isChunk(term)) {
      TaggedRef t;
      switch (tagged2Const(term)->getType()) {
      case Co_Chunk:
        t = tagged2SChunk(term)->getFeature(fea);
        break;
      case Co_Object:
        t = tagged2Object(term)->getFeature(fea);
        break;
      case Co_Class:
        t = tagged2ObjectClass(term)->classGetFeature(fea);
        break;
      case Co_Array:
      case Co_Dictionary:
      default:
        // no public known features
        t = makeTaggedNULL();
        break;
      }
      if (t == makeTaggedNULL()) {
        if (dot) goto raise; else return FAILED;
      }
      if (out) *out = t;
      return PROCEED;
    }

    goto typeError0;
  }
typeError0:
  oz_typeError(0,"Record or Chunk");
typeError1:
  oz_typeError(1,"Feature");
raise:
  return oz_raise(E_ERROR,E_KERNEL,".",2,term,fea);
}


OZ_Return dotInline(TaggedRef term, TaggedRef fea, TaggedRef &out)
{
  return genericDot(term,fea,&out,TRUE);
}
DECLAREBI_USEINLINEFUN2(BIdot,dotInline)


OZ_Return hasFeatureInline(TaggedRef term, TaggedRef fea)
{
  return genericDot(term,fea,0,FALSE);
}

DECLAREBOOLFUN2(BIhasFeatureB,hasFeatureBInline,hasFeatureInline)

OZ_Return subtreeInline(TaggedRef term, TaggedRef fea, TaggedRef &out)
{
  return genericDot(term,fea,&out,FALSE);
}

extern OZ_Return subtreeInline(TaggedRef term, TaggedRef fea, TaggedRef &out);


/*
 * fun {matchDefault Term Attr Defau}
 *    if X in Term.Attr = X then X else Defau fi
 * end
 */

OZ_Return matchDefaultInline(TaggedRef term, TaggedRef attr, TaggedRef defau,
                             TaggedRef &out)
{
  switch(subtreeInline(term,attr,out)) {
  case PROCEED:
    return PROCEED;
  case SUSPEND:
    return SUSPEND;
  case FAILED:
  default:
    out = defau;
    return PROCEED;
  }
}

DECLAREBI_USEINLINEFUN3(BImatchDefault,matchDefaultInline)


OZ_Return widthInline(TaggedRef term, TaggedRef &out)
{
  DEREF(term,_,tag);

  switch (tag) {
  case LTUPLE:
    out = makeTaggedSmallInt(2);
    return PROCEED;
  case SRECORD:
  record:
    out = makeTaggedSmallInt(tagged2SRecord(term)->getWidth());
    return PROCEED;
  case LITERAL:
    out = makeTaggedSmallInt(0);
    return PROCEED;
  case UVAR:
  case SVAR:
    return SUSPEND;
  case CVAR:
    switch (tagged2CVar(term)->getType()) {
    case OFSVariable:
        return SUSPEND;
    case FDVariable:
    case BoolVariable:
        break;
    default:
        return SUSPEND;
    }
    break;
  default:
    break;
  }

  oz_typeError(0,"Record");
}

DECLAREBI_USEINLINEFUN1(BIwidth,widthInline)


// ---------------------------------------------------------------------
// Unit
// ---------------------------------------------------------------------

OZ_Return isUnitInline(TaggedRef t)
{
  NONSUVAR( t, term, tag);
  if (isCVar(tag)) {
    switch (tagged2CVar(term)->getType()) {
    case OFSVariable:
      {
        GenOFSVariable *ofsvar=tagged2GenOFSVar(term);
        if (ofsvar->getWidth()>0) return FAILED;
        TaggedRef lbl=ofsvar->getLabel();
        DEREF(lbl,_1,lblTag);
        if (isLiteral(lblTag)) {
          if (isAtom(lbl)) {
            if (literalEq(term,NameUnit))
              return SUSPEND;
            else
              return FAILED;
          } else { // isName
            return FAILED;
          }
        }
        return SUSPEND;
      }
    case FDVariable:
    case BoolVariable:
      return FAILED;
    default:
      return SUSPEND;
    }
  }
  if (literalEq(term,NameUnit))
    return PROCEED;
  else
    return FAILED;
}

DECLAREBI_USEINLINEREL1(BIisUnit,isUnitInline)
DECLAREBOOLFUN1(BIisUnitB,isUnitBInline,isUnitInline)

// ---------------------------------------------------------------------
// Bool things
// ---------------------------------------------------------------------

OZ_Return isBoolInline(TaggedRef t)
{
  NONSUVAR( t, term, tag);
  if (isCVar(tag)) {
    switch (tagged2CVar(term)->getType()) {
    case OFSVariable:
      {
        GenOFSVariable *ofsvar=tagged2GenOFSVar(term);
        if (ofsvar->getWidth()>0) return FAILED;
        TaggedRef lbl=ofsvar->getLabel();
        DEREF(lbl,_1,lblTag);
        if (isLiteral(lblTag)) {
          if (isAtom(lbl)) {
            if (literalEq(term,NameTrue) ||
                literalEq(term,NameFalse))
              return SUSPEND;
            else
              return FAILED;
          } else { // isName
            return FAILED;
          }
        }
        return SUSPEND;
      }
    case FDVariable:
    case BoolVariable:
      return FAILED;
    default:
      return SUSPEND;
    }
  }
  if (literalEq(term,NameTrue) || literalEq(term,NameFalse))
    return PROCEED;
  else
    return FAILED;
}

DECLAREBI_USEINLINEREL1(BIisBool,isBoolInline)
DECLAREBOOLFUN1(BIisBoolB,isBoolBInline,isBoolInline)

OZ_Return notInline(TaggedRef A, TaggedRef &out)
{
  NONVAR(A,term);

  if (literalEq(term,NameTrue)) {
    out = NameFalse;
    return PROCEED;
  } else {
    if (literalEq(term,NameFalse)) {
      out = NameTrue;
      return PROCEED;
    }
  }

  oz_typeError(0,"Bool");
}

DECLAREBI_USEINLINEFUN1(BInot,notInline)


OZ_Return andInline(TaggedRef A, TaggedRef B, TaggedRef &out) {
  DEREF(A,_1,tagA);
  DEREF(B,_2,tagB);

  if (literalEq(A,NameTrue)) {
    if (isAnyVar(B)) {
      return SUSPEND;
    } else if (literalEq(B,NameTrue) || literalEq(B,NameFalse)) {
      out = B;
      return PROCEED;
    } else {
      oz_typeError(1,"Bool");
    }
  } else if (literalEq(A,NameFalse)) {
    if (isAnyVar(B)) {
      return SUSPEND;
    } else if (literalEq(B,NameTrue) || literalEq(B,NameFalse)) {
      out = NameFalse;
      return PROCEED;
    } else {
      oz_typeError(1,"Bool");
    }
  } else if (isAnyVar(A)) {
    return SUSPEND;
  } else {
    oz_typeError(0,"Bool");
  }
}

DECLAREBI_USEINLINEFUN2(BIand,andInline)


OZ_Return orInline(TaggedRef A, TaggedRef B, TaggedRef &out) {
  DEREF(A,_1,tagA);
  DEREF(B,_2,tagB);

  if (literalEq(A,NameTrue)) {
    if (isAnyVar(B)) {
      return SUSPEND;
    } else if (literalEq(B,NameTrue) || literalEq(B,NameFalse)) {
      out = NameTrue;
      return PROCEED;
    } else {
      oz_typeError(1,"Bool");
    }
  } else if (literalEq(A,NameFalse)) {
    if (isAnyVar(B)) {
      return SUSPEND;
    } else if (literalEq(B,NameTrue) || literalEq(B,NameFalse)) {
      out = B;
      return PROCEED;
    } else {
      oz_typeError(1,"Bool");
    }
  } else if (isAnyVar(A)) {
    return SUSPEND;
  } else {
    oz_typeError(0,"Bool");
  }
}

DECLAREBI_USEINLINEFUN2(BIor,orInline)



// ---------------------------------------------------------------------
// Atom
// ---------------------------------------------------------------------

OZ_Return atomToStringInline(TaggedRef t, TaggedRef &out)
{
  DEREF(t,_1,_2);
  if (isAnyVar(t)) return SUSPEND;

  if (!isAtom(t)) {
    oz_typeError(-1,"atom");
  }

  out = OZ_string(tagged2Literal(t)->getPrintName());
  return PROCEED;
}
DECLAREBI_USEINLINEFUN1(BIatomToString,atomToStringInline)


OZ_C_proc_begin(BIstringToAtom,2)
{
  oz_declareProperStringArg(0,str);
  oz_declareArg(1,out);

  OZ_Return ret = oz_unifyAtom(out,str);

  return ret;
}
OZ_C_proc_end

// ---------------------------------------------------------------------
// Virtual Strings
// ---------------------------------------------------------------------

inline
TaggedRef vs_suspend(SRecord *vs, int i, TaggedRef arg_rest) {
  if (i == vs->getWidth()-1) {
    return arg_rest;
  } else {
    SRecord *stuple = SRecord::newSRecord(AtomPair, vs->getWidth() - i);
    stuple->setArg(0, arg_rest);
    i++;
    for (int j=1 ; i < vs->getWidth() ; (j++, i++))
      stuple->setArg(j, vs->getArg(i));
    return makeTaggedSRecord(stuple);
  }
}

static OZ_Return vs_check(OZ_Term vs, OZ_Term *rest) {
  DEREF(vs, vs_ptr, vs_tag);

  if (isAnyVar(vs_tag)) {
    *rest = makeTaggedRef(vs_ptr);
    oz_suspendOn(*rest);
  } else if (isInt(vs_tag)) {
    return PROCEED;
  } else if (isFloat(vs_tag)) {
    return PROCEED;
  } else if (isLiteral(vs_tag) && tagged2Literal(vs)->isAtom()) {
    return PROCEED;
  } else if (isCons(vs_tag)) {
    TaggedRef cdr  = vs;
    TaggedRef prev = vs;

    while (1) {
      DEREF(cdr, cdr_ptr, cdr_tag);

      if (isNil(cdr))
        return PROCEED;

      if (isAnyVar(cdr_tag)) {
        *rest = prev;
        oz_suspendOn(makeTaggedRef(cdr_ptr));
      }

      if (!isCons(cdr_tag))
        return FAILED;

      TaggedRef car = tagged2LTuple(cdr)->getHead();
      DEREF(car, car_ptr, car_tag);

      if (isAnyVar(car_tag)) {
        *rest = cdr;
        oz_suspendOn(makeTaggedRef(car_ptr));
      } else if (!isSmallInt(car_tag) ||
                 (smallIntValue(car) < 0) ||
                 (smallIntValue(car) > 255)) {
        return FAILED;
      } else {
        prev = cdr;
        cdr  = tagged2LTuple(cdr)->getTail();
      }
    };

    return FAILED;

  } else if (isSTuple(vs) &&
             literalEq(tagged2SRecord(vs)->getLabel(),AtomPair)) {
    for (int i=0; i < tagged2SRecord(vs)->getWidth(); i++) {
      TaggedRef arg_rest;
      OZ_Return status = vs_check(tagged2SRecord(vs)->getArg(i), &arg_rest);

      if (status == SUSPEND) {
        *rest = vs_suspend(tagged2SRecord(vs), i, arg_rest);

        return SUSPEND;
      } else if (status==FAILED) {
        return FAILED;
      }
    }
    return PROCEED;
  } else {
    return FAILED;
  }
}


// mm2: cycle check needed
static OZ_Return vs_length(OZ_Term vs, OZ_Term *rest, int *len) {
  DEREF(vs, vs_ptr, vs_tag);

  if (isAnyVar(vs_tag)) {
    *rest = makeTaggedRef(vs_ptr);
    oz_suspendOn(*rest);
  } else if (isInt(vs_tag)) {
    *len = *len + strlen(toC(vs));
    return PROCEED;
  } else if (isFloat(vs_tag)) {
    *len = *len + strlen(toC(vs));
    return PROCEED;
  } else if (isLiteral(vs_tag) && tagged2Literal(vs)->isAtom()) {
    if (literalEq(vs,AtomPair) ||
        literalEq(vs,AtomNil))
      return PROCEED;
    *len = *len + ((Atom*)tagged2Literal(vs))->getSize();
    return PROCEED;
  } else if (isCons(vs_tag)) {
    TaggedRef cdr  = vs;
    TaggedRef prev = vs;

    while (1) {
      DEREF(cdr, cdr_ptr, cdr_tag);

      if (isNil(cdr))
        return PROCEED;

      if (isAnyVar(cdr_tag)) {
        *rest = prev;
        Assert((*len)>0);
        *len = *len - 1;
        oz_suspendOn(makeTaggedRef(cdr_ptr));
      }

      if (!isCons(cdr_tag))
        return FAILED;

      TaggedRef car = tagged2LTuple(cdr)->getHead();
      DEREF(car, car_ptr, car_tag);

      if (isAnyVar(car_tag)) {
        *rest = cdr;
        oz_suspendOn(makeTaggedRef(car_ptr));
      } else if (!isSmallInt(car_tag) ||
                 (smallIntValue(car) < 0) ||
                 (smallIntValue(car) > 255)) {
        return FAILED;
      } else {
        prev = cdr;
        cdr  = tagged2LTuple(cdr)->getTail();
        *len = *len + 1;
      }
    };

    return FAILED;

  } else if (isSTuple(vs) &&
             literalEq(tagged2SRecord(vs)->getLabel(),AtomPair)) {
    for (int i=0; i < tagged2SRecord(vs)->getWidth(); i++) {
      TaggedRef arg_rest;
      OZ_Return status =
        vs_length(tagged2SRecord(vs)->getArg(i), &arg_rest, len);

      if (status == SUSPEND) {
        *rest = vs_suspend(tagged2SRecord(vs), i, arg_rest);
        return SUSPEND;
      } else if (status==FAILED) {
        return FAILED;
      }
    }
    return PROCEED;
  } else {
    return FAILED;
  }
}


OZ_C_proc_begin(BIvsLength,3) {
  TaggedRef rest = makeTaggedNULL();
  int len = smallIntValue(deref(OZ_args[1]));
  OZ_Return status = vs_length(OZ_args[0], &rest, &len);
  if (status == SUSPEND) {
    OZ_args[0] = rest;
    OZ_args[1] = makeTaggedSmallInt(len);
    return SUSPEND;
  } else if (status == FAILED) {
    oz_typeError(0, "Virtual String");
  } else {
    return oz_unify(OZ_args[2], makeTaggedSmallInt(len));
  }
} OZ_C_proc_end

OZ_C_proc_begin(BIvsIs,2) {
  TaggedRef rest = makeTaggedNULL();
  OZ_Return status = vs_check(OZ_args[0], &rest);
  if (status == SUSPEND) {
    OZ_args[0] = rest;
    return SUSPEND;
  }
  return oz_unify(OZ_args[1], (status == PROCEED) ? NameTrue : NameFalse);
} OZ_C_proc_end


// ---------------------------------------------------------------------
// Chunk
// ---------------------------------------------------------------------

OZ_C_proc_begin(BInewChunk,2)
{
  oz_declareNonvarArg(0,val);
  oz_declareArg(1,out);

  if (!isRecord(val)) oz_typeError(0,"Record");

  return oz_unify(out,oz_newChunk(val));
}
OZ_C_proc_end

OZ_C_proc_begin(BIchunkArity,2)
{
  OZ_Term ch =  OZ_getCArg(0);
  OZ_Term out = OZ_getCArg(1);

  DEREF(ch, chPtr, chTag);

  switch(chTag) {
  case UVAR:
  case SVAR:
  case CVAR:
    oz_suspendOn(makeTaggedRef(chPtr));

  case OZCONST:
    if (!isChunk(ch)) oz_typeError(0,"Chunk");
    //
    switch (tagged2Const(ch)->getType()) {
    case Co_Class:
      return oz_unify(out,tagged2ObjectClass(ch)->getArityList());
    case Co_Object:
      return oz_unify(out,tagged2Object(ch)->getArityList());
    case Co_Chunk:
      return oz_unify(out,tagged2SChunk(ch)->getArityList());
    default:
      // no features
      return oz_unify(out,nil());
    }

  default:
    oz_typeError(0,"Chunk");
  }
}
OZ_C_proc_end

OZ_C_proc_begin(BIchunkWidth, 2)
{
  OZ_Term ch =  OZ_getCArg(0);
  OZ_Term out = OZ_getCArg(1);

  DEREF(ch, chPtr, chTag);

  switch(chTag) {
  case UVAR:
  case SVAR:
  case CVAR:
    oz_suspendOn(makeTaggedRef(chPtr));

  case OZCONST:
    if (!isChunk(ch)) oz_typeError(0,"Chunk");
    //
    switch (tagged2Const(ch)->getType()) {
    case Co_Class:
      return oz_unify(out, makeTaggedSmallInt(tagged2ObjectClass(ch)->getWidth()));
    case Co_Object:
      return oz_unify(out, makeTaggedSmallInt(tagged2Object(ch)->getWidth()));
    case Co_Chunk:
      return oz_unify(out, makeTaggedSmallInt(tagged2SChunk(ch)->getWidth()));
    default:
      // no features
      return oz_unify(out,makeTaggedSmallInt (0));
    }

  default:
    oz_typeError(0,"Chunk");
  }
}
OZ_C_proc_end

OZ_C_proc_begin(BIrecordWidth, 2)
{
  OZ_Term arg = OZ_getCArg(0);
  OZ_Term out = OZ_getCArg(1);

  DEREF(arg, argPtr, argTag);

  switch (argTag) {
  case CVAR:
    switch (tagged2CVar(arg)->getType ()) {
    case OFSVariable:
      {
        GenOFSVariable *ofsVar = tagged2GenOFSVar(arg);
        return (oz_unify (out, makeTaggedSmallInt(ofsVar->getWidth ())));
      }

    default:
      oz_typeError(0, "Record");
    }

  case SRECORD:
    return (oz_unify (out, makeTaggedSmallInt(tagged2SRecord(arg)->getWidth ())));

  default:
    oz_typeError(0, "Record");
  }
}
OZ_C_proc_end

/* ---------------------------------------------------------------------
 * Threads
 * --------------------------------------------------------------------- */

OZ_C_proc_begin(BIthreadThis,1)
{
  oz_declareArg(0,out);

  return oz_unify(out, makeTaggedConst(am.currentThread()));
}
OZ_C_proc_end

/*
 * change priority of a thread
 *  if my priority is lowered, then preempt me
 *  if priority of other thread become higher than mine, then preempt me
 */
OZ_C_proc_begin(BIthreadSetPriority,2)
{
  oz_declareThreadArg(0,th);
  oz_declareNonvarArg(1,atom_prio);

  int prio;

  if (!isAtom(atom_prio))
    goto type_goof;

  if (literalEq(atom_prio, AtomLow)) {
    prio = LOW_PRIORITY;
  } else if (literalEq(atom_prio, AtomMedium)) {
    prio = MID_PRIORITY;
  } else if (literalEq(atom_prio, AtomHigh)) {
    prio = HI_PRIORITY;
  } else {
  type_goof:
    oz_typeError(1,"Atom [low medium high]");
  }

  if (th->isProxy()) {
    return remoteSend(th,"Thread.setPriority",atom_prio);
  }

  if (th->isDeadThread()) return PROCEED;

  int oldPrio = th->getPriority();
  th->setPriority(prio);

  if (am.currentThread() == th) {
    if (prio <= oldPrio) {
      am.setSFlag(ThreadSwitch);
      return BI_PREEMPT;
    }
  } else {
    if (th->isRunnable()) {
      am.rescheduleThread(th);
    }
    if (prio > am.currentThread()->getPriority()) {
      return BI_PREEMPT;
    }
  }

  return PROCEED;
}
OZ_C_proc_end

OZ_Term threadGetPriority(Thread *th) {
  switch (th->getPriority()) {
  case LOW_PRIORITY: return AtomLow;
  case MID_PRIORITY: return AtomMedium;
  case HI_PRIORITY:  return AtomHigh;
  default: Assert(0); return AtomHigh;
  }
}

OZ_C_proc_begin(BIthreadGetPriority,2)
{
  oz_declareThreadArg(0,th);
  oz_declareArg(1,out);

  if (th->isProxy()) {
    return remoteSend(th,"Thread.getPriority",out);
  }

  return oz_unify(threadGetPriority(th),out);
}
OZ_C_proc_end

OZ_C_proc_begin(BIthreadID,2)
{
  oz_declareThreadArg(0,th);
  oz_declareArg(1,out);

  if (th->isProxy())
    return oz_raise(E_ERROR,E_SYSTEM,"threadID Proxy not impl",0);

  return oz_unifyInt(out, th->getID() & THREAD_ID_MASK);
}
OZ_C_proc_end

OZ_C_proc_begin(BIsetThreadID,2)
{
  oz_declareThreadArg(0,th);
  oz_declareIntArg(1,id);

  if (th->isProxy())
    return oz_raise(E_ERROR,E_SYSTEM,"setThreadID Proxy not impl",0);

  th->setID(id | (1 << THREAD_ID_SIZE));
  return PROCEED;
}
OZ_C_proc_end

OZ_C_proc_begin(BIparentThreadID,2)
{
  oz_declareThreadArg(0,th);
  oz_declareArg(1,out);

  if (th->isProxy())
    return oz_raise(E_ERROR,E_SYSTEM,"parentThreadID Proxy not impl",0);

  return oz_unifyInt(out, (th->getID() >> THREAD_ID_SIZE) & THREAD_ID_MASK);
}
OZ_C_proc_end

OZ_C_proc_begin(BIthreadIs,2)
{
  oz_declareNonvarArg(0,th);
  oz_declareArg(1,out);

  return oz_unify(out,oz_isThread(th)?NameTrue:NameFalse);
}
OZ_C_proc_end

/*
 * raise exception on thread
 */
OZ_C_proc_proto(BIraise);
OZ_C_proc_proto(BIraiseDebug);

void threadRaise(Thread *th,OZ_Term E,int debug=0) {
  Assert(am.currentThread() != th);

  if (th->isDeadThread()) return;

  RefsArray args=allocateRefsArray(1, NO);
  args[0]=E;

  th->pushCFun(debug?BIraiseDebug:BIraise, args, 1, OK);

  th->setStop(NO);
  th->zeroPStop();

  if (th->isSuspended())
    am.suspThreadToRunnable(th);

  if (!am.isScheduledSlow(th))
    am.scheduleThread(th);
}

OZ_C_proc_begin(BIthreadRaise,2)
{
  oz_declareThreadArg(0,th);
  oz_declareNonvarArg(1,E);

  if (th->isProxy()) {
    return remoteSend(th,"Thread.raise",E);
  }

  if (am.currentThread() == th) {
    return OZ_raise(E);
  }

  threadRaise(th,E);
  return PROCEED;
}
OZ_C_proc_end

/*
 * suspend a thread
 *   is done lazy: when the thread becomes running the stop flag is tested
 */
OZ_C_proc_begin(BIthreadSuspend,1)
{
  oz_declareThreadArg(0,th);

  if (th->isProxy()) {
    return remoteSend(th,"Thread.suspend",nil());
  }

  th->setStop(OK);
  if (th == am.currentThread()) {
    return BI_PREEMPT;
  }
  return PROCEED;
}
OZ_C_proc_end

void threadResume(Thread *th) {
  th->setStop(NO);

  if (th->isDeadThread()) return;

  if (th->isRunnable() && !am.isScheduledSlow(th)) {
    am.scheduleThread(th);
  }
}

OZ_C_proc_begin(BIthreadResume,1)
{
  oz_declareThreadArg(0,th);

  if (th->isProxy()) {
    return remoteSend(th,"Thread.resume",nil());
  }


  threadResume(th);

  return PROCEED;
}
OZ_C_proc_end

OZ_C_proc_begin(BIthreadIsSuspended,2)
{
  oz_declareThreadArg(0,th);
  oz_declareArg(1,out);

  if (th->isProxy()) {
    return remoteSend(th,"Thread.isSuspended",out);
  }


  return oz_unify(out, th->getStop() ? NameTrue : NameFalse);
}
OZ_C_proc_end

OZ_Term threadState(Thread *th) {
  if (th->isDeadThread()) {
    return oz_atom("terminated");
  }
  if (th->isRunnable()) {
    return oz_atom("runnable");
  }
  return oz_atom("blocked");
}

OZ_C_proc_begin(BIthreadState,2)
{
  oz_declareThreadArg(0,th);
  oz_declareArg(1,out);

  if (th->isProxy()) {
    return remoteSend(th,"Thread.state",out);
  }

  return oz_unify(out,threadState(th));
}
OZ_C_proc_end

OZ_C_proc_begin(BIthreadPreempt,1)
{
  oz_declareThreadArg(0,th);

  if (th->isProxy())
    return oz_raise(E_ERROR,E_SYSTEM,"threadPreempt Proxy not impl",0);

  if (th == am.currentThread()) {
    return BI_PREEMPT;
  }
  return PROCEED;
}
OZ_C_proc_end

OZ_C_proc_begin(BIthreadSetRaiseOnBlock,2)
{
  oz_declareThreadArg(0,thread);
  oz_declareNonvarArg(1,yesno);

  if (OZ_isTrue(yesno))
    thread->setNoBlock(OK);
  else if (OZ_isFalse(yesno))
    thread->setNoBlock(NO);
  else
    oz_typeError(1,"Bool");
  return PROCEED;
}
OZ_C_proc_end

OZ_C_proc_begin(BIthreadGetRaiseOnBlock,2)
{
  oz_declareThreadArg(0,thread);
  oz_declareArg(1,out);

  return OZ_unify(out,thread->getNoBlock()? NameTrue: NameFalse);
}
OZ_C_proc_end

// ------------------ explore a thread's taskstack ---------------------------

OZ_C_proc_begin(BIthreadTaskStack,4)
{
  oz_declareThreadArg(0,thread);
  oz_declareIntArg(1,depth);
  oz_declareNonvarArg(2,verbose);
  oz_declareArg(3,out);

  if (thread->isDeadThread() || !thread->hasStack())
    return OZ_unify(out, nil());

  Bool doverbose;
  if (OZ_isTrue(verbose))
    doverbose = OK;
  else if (OZ_isFalse(verbose))
    doverbose = NO;
  else
    oz_typeError(2,"Bool");

  TaskStack *taskstack = thread->getTaskStackRef();
  return OZ_unify(out, taskstack->getTaskStack(thread,doverbose,depth));
}
OZ_C_proc_end

OZ_C_proc_begin(BIthreadFrameVariables,3)
{
  oz_declareThreadArg(0,thread);
  oz_declareIntArg(1,frameId);
  oz_declareArg(2,out);

  if (thread->isDeadThread() || !thread->hasStack())
    return OZ_unify(out, NameUnit);

  TaskStack *taskstack = thread->getTaskStackRef();
  return OZ_unify(out, taskstack->getFrameVariables(frameId));
}
OZ_C_proc_end

OZ_C_proc_begin(BIthreadLocation,2)
{
  oz_declareThreadArg(0,thread);
  oz_declareArg(1,out);

  if (thread->isDeadThread()) {
    return OZ_unify(out, nil());
  } else {
    return OZ_unify(out, oz_getLocation(GETBOARD(thread)));
  }
}
OZ_C_proc_end

// ---------------------------------------------------------------------
// NAMES
// ---------------------------------------------------------------------

OZ_C_proc_begin(BInewName,1)
{
  OZ_Term out = OZ_getCArg(0);
  return oz_unify(out,oz_newName());
}
OZ_C_proc_end


OZ_C_proc_begin(BInewUniqueName,2)
{
  oz_declareAtomArg(0,name);
  OZ_Term out = OZ_getCArg(1);
  return oz_unify(out,getUniqueName(name));
}
OZ_C_proc_end

// ---------------------------------------------------------------------
// term type
// ---------------------------------------------------------------------

OZ_Return BItermTypeInline(TaggedRef term, TaggedRef &out)
{
  out = OZ_termType(term);
  if (oz_eq(out,oz_atom("variable"))) {
    return SUSPEND;
  }
  return PROCEED;
}

DECLAREBI_USEINLINEFUN1(BItermType,BItermTypeInline)

OZ_Return BIstatusInline(TaggedRef term, TaggedRef &out) {
  DEREF(term, _1, tag);

  switch (tag) {
  case UVAR:
  case SVAR:
    out = AtomFree; break;
  case CVAR: {
    SRecord *t = SRecord::newSRecord(AtomKinded, 1);
    switch (tagged2CVar(term)->getType()) {
    case FDVariable:
    case BoolVariable:
      t->setArg(0, AtomInt); break;
    case FSetVariable:
      t->setArg(0, AtomFSet); break;
    case OFSVariable:
      t->setArg(0, AtomRecord); break;
    default:
      t->setArg(0, AtomOther); break;
    }
    out = makeTaggedSRecord(t);
    break;
  }
  default: {
    SRecord *t = SRecord::newSRecord(AtomDet, 1);
    t->setArg(0, OZ_termType(term));
    out = makeTaggedSRecord(t);
  }
  }
  return PROCEED;
}

DECLAREBI_USEINLINEFUN1(BIstatus,BIstatusInline)


// ---------------------------------------------------------------------
// Builtins ==, \=, ==B and \=B
// ---------------------------------------------------------------------

inline
OZ_Return AM::eqeq(TaggedRef Ain,TaggedRef Bin)
{
  trail.pushMark();
  shallowHeapTop = heapTop;
  Bool ret = unify(Ain,Bin,(ByteCode*)1);
  shallowHeapTop = NULL;
  if (ret == NO) {
    reduceTrailOnFail();
    return FAILED;
  }

  if (trail.isEmptyChunk()) {
    trail.popMark();
    return PROCEED;
  }

  reduceTrailOnEqEq();

  return SUSPEND;
}

inline
OZ_Return eqeqWrapper(TaggedRef Ain, TaggedRef Bin)
{
  TaggedRef A = Ain, B = Bin;
  DEREF(A,aPtr,tagA); DEREF(B,bPtr,tagB);

  /* Really fast test for equality */
  if (tagA != tagB) {
    if (isAnyVar(A) || isAnyVar(B)) goto dontknow;
    return FAILED;
  }

  if (isSmallInt(tagA)) return smallIntEq(A,B) ? PROCEED : FAILED;
  if (isBigInt(tagA))   return bigIntEq(A,B)   ? PROCEED : FAILED;
  if (isFloat(tagA))    return floatEq(A,B)    ? PROCEED : FAILED;

  if (isLiteral(tagA))  return literalEq(A,B)  ? PROCEED : FAILED;

  if (A == B && !isAnyVar(A)) return PROCEED;

  if (isConst(tagA))    return FAILED;

 dontknow:
  return am.eqeq(Ain,Bin);
}

OZ_C_proc_begin(BIeq,2)
{
  TaggedRef A = OZ_getCArg(0);
  TaggedRef B = OZ_getCArg(1);
  return eqeqWrapper(A,B);
}
OZ_C_proc_end


OZ_C_proc_begin(BIneq,2)
{
  TaggedRef A = OZ_getCArg(0);
  TaggedRef B = OZ_getCArg(1);
  return eqeqWrapper(A,B);
}
OZ_C_proc_end



OZ_Return neqInline(TaggedRef A, TaggedRef B, TaggedRef &out);
OZ_Return eqeqInline(TaggedRef A, TaggedRef B, TaggedRef &out);


OZ_C_proc_begin(BIneqB,3)
{
  OZ_Term help;
  OZ_Return ret=neqInline(OZ_getCArg(0),OZ_getCArg(1),help);
  return ret==PROCEED ? oz_unify(help,OZ_getCArg(2)) : ret;
}
OZ_C_proc_end

OZ_C_proc_begin(BIeqB,3)
{
  OZ_Term help;
  OZ_Return ret=eqeqInline(OZ_getCArg(0),OZ_getCArg(1),help);
  return ret==PROCEED ? oz_unify(help,OZ_getCArg(2)): ret;
}
OZ_C_proc_end


OZ_Return eqeqInline(TaggedRef A, TaggedRef B, TaggedRef &out)
{
  switch(eqeqWrapper(A,B)) {
  case PROCEED:
    out = NameTrue;
    return PROCEED;
  case FAILED:
    out = NameFalse;
    return PROCEED;
  case SUSPEND:
  default:
    return SUSPEND;
  }
}


OZ_Return neqInline(TaggedRef A, TaggedRef B, TaggedRef &out)
{
  switch(eqeqWrapper(A,B)) {
  case PROCEED:
    out = NameFalse;
    return PROCEED;
  case FAILED:
    out = NameTrue;
    return PROCEED;
  case SUSPEND:
  default:
    return SUSPEND;
  }
}

OZ_C_proc_begin(BIlength,2)
{
  OZ_Term in=OZ_getCArg(0);
  OZ_Term out=OZ_getCArg(1);

  OZ_Term ret = oz_isList(in);
  if (isRef(ret)) oz_suspendOnVar(ret);
  if (ret==NameFalse) oz_typeError(0,"finite list");
  return oz_unify(out,ret);
}
OZ_C_proc_end

// ---------------------------------------------------------------------
// String
// ---------------------------------------------------------------------

OZ_C_proc_begin(BIisString,2)
{
  OZ_Term in=OZ_getCArg(0);
  OZ_Term out=OZ_getCArg(1);

  OZ_Term var;
  if (!OZ_isString(in,&var)) {
    if (var == 0) return oz_unify(out,NameFalse);
    oz_suspendOn(var);
  }
  return oz_unify(out,NameTrue);
}
OZ_C_proc_end


// ---------------------------------------------------------------------
// Char
// ---------------------------------------------------------------------

#define FirstCharArg \
 TaggedRef tc = OZ_getCArg(0);      \
 int i;                             \
 { DEREF(tc, tc_ptr, tc_tag);       \
 if (isAnyVar(tc_tag)) {            \
   am.addSuspendVarList(tc_ptr);    \
   return SUSPEND;                  \
 }                                  \
 if (!isSmallInt(tc)) {             \
   oz_typeError(1,"Char");          \
 } else {                           \
   i = smallIntValue(tc);           \
   if ((i < 0) || (i > 255)) {      \
     oz_typeError(1,"Char");        \
   }                                \
 } }

#define TestChar(TEST)                                            \
  FirstCharArg;                                                   \
  return oz_unify(OZ_getCArg(1), TEST ((unsigned char) i) ? NameTrue : NameFalse);

OZ_C_proc_begin(BIcharIs,2) {
 oz_declareNonvarArg(0,c);
 oz_declareArg(1,out);
 c = deref(c);
 if (!isSmallInt(c)) return oz_unify(out,NameFalse);
 int i = smallIntValue(c);
 return oz_unify(out,(i >=0 && i <= 255) ? NameTrue : NameFalse);
} OZ_C_proc_end

OZ_C_proc_begin(BIcharIsAlNum,2) { TestChar(iso_isalnum); } OZ_C_proc_end

OZ_C_proc_begin(BIcharIsAlpha,2) { TestChar(iso_isalpha); } OZ_C_proc_end

OZ_C_proc_begin(BIcharIsCntrl,2) { TestChar(iso_iscntrl); } OZ_C_proc_end

OZ_C_proc_begin(BIcharIsDigit,2) { TestChar(iso_isdigit); } OZ_C_proc_end

OZ_C_proc_begin(BIcharIsGraph,2) { TestChar(iso_isgraph); } OZ_C_proc_end

OZ_C_proc_begin(BIcharIsLower,2) { TestChar(iso_islower); } OZ_C_proc_end

OZ_C_proc_begin(BIcharIsPrint,2) { TestChar(iso_isprint); } OZ_C_proc_end

OZ_C_proc_begin(BIcharIsPunct,2) { TestChar(iso_ispunct); } OZ_C_proc_end

OZ_C_proc_begin(BIcharIsSpace,2) { TestChar(iso_isspace); } OZ_C_proc_end

OZ_C_proc_begin(BIcharIsUpper,2) { TestChar(iso_isupper); } OZ_C_proc_end

OZ_C_proc_begin(BIcharIsXDigit,2) {TestChar(iso_isxdigit);} OZ_C_proc_end

OZ_C_proc_begin(BIcharToLower,2) {
  FirstCharArg;
  return oz_unifyInt(OZ_getCArg(1), iso_tolower((unsigned char) i));
} OZ_C_proc_end

OZ_C_proc_begin(BIcharToUpper,2) {
  FirstCharArg;
  return oz_unifyInt(OZ_getCArg(1), iso_toupper((unsigned char) i));
} OZ_C_proc_end

OZ_C_proc_begin(BIcharToAtom,2) {
  FirstCharArg;
  if (i) {
     char s[2]; s[0]= (char) i; s[1]='\0';
     return oz_unify(OZ_getCArg(1), makeTaggedAtom(s));
  }
  return oz_unify(OZ_getCArg(1), AtomEmpty);
} OZ_C_proc_end

OZ_C_proc_begin(BIcharType,2) {
  FirstCharArg;
  TaggedRef type;
  if (iso_isupper(i))      type = AtomUpper;
  else if (iso_islower(i)) type = AtomLower;
  else if (iso_isdigit(i)) type = AtomDigit;
  else if (iso_isspace(i)) type = AtomCharSpace;
  else if (iso_ispunct(i)) type = AtomPunct;
  else                     type = AtomOther;
  return oz_unify(OZ_getCArg(1), type);
} OZ_C_proc_end


/********************************************************************
 * Records
 ******************************************************************** */


OZ_Return BIadjoinInline(TaggedRef t0, TaggedRef t1, TaggedRef &out)
{
  DEREF(t0,_0,tag0);
  DEREF(t1,_1,tag1);

  switch (tag0) {
  case LITERAL:
    switch (tag1) {
    case SRECORD:
    case LITERAL:
    case LTUPLE:
      out = t1;
      return PROCEED;
    case UVAR:
    case SVAR:
      return SUSPEND;
    case CVAR:
      switch (tagged2CVar(t1)->getType()) {
      case FDVariable:
      case BoolVariable:
          oz_typeError(1,"Record");
      default:
          return SUSPEND;
      }
    default:
      oz_typeError(1,"Record");
    }
  case LTUPLE:
  case SRECORD:
    {
      SRecord *rec= makeRecord(t0);
      switch (tag1) {
      case LITERAL:
        {
          SRecord *newrec = SRecord::newSRecord(rec);
          newrec->setLabelForAdjoinOpt(t1);
          out = newrec->normalize();
          return PROCEED;
        }
      case SRECORD:
      case LTUPLE:
        {
          out = oz_adjoin(rec,makeRecord(t1));
          return PROCEED;
        }
      case UVAR:
      case SVAR:
        return SUSPEND;
      case CVAR:
        switch (tagged2CVar(t1)->getType()) {
        case FDVariable:
        case BoolVariable:
            oz_typeError(1,"Record");
        default:
            return SUSPEND;
        }
      default:
        oz_typeError(1,"Record");
      }
    }
  case UVAR:
  case SVAR:
  case CVAR:
    if (tag0==CVAR) {
        switch (tagged2CVar(t0)->getType()) {
        case FDVariable:
        case BoolVariable:
          oz_typeError(0,"Record");
        default:
          break;
        }
    }
    switch (tag1) {
    case UVAR:
    case SVAR:
    case SRECORD:
    case LTUPLE:
    case LITERAL:
      return SUSPEND;
    case CVAR:
      switch (tagged2CVar(t1)->getType()) {
      case FDVariable:
      case BoolVariable:
          oz_typeError(1,"Record");
      default:
        return SUSPEND;
      }
    default:
      oz_typeError(1,"Record");
    }
  default:
    oz_typeError(0,"Record");
  }
}

DECLAREBI_USEINLINEFUN2(BIadjoin,BIadjoinInline)

OZ_C_proc_begin(BIadjoinAt,4)
{
  OZ_Term rec = OZ_getCArg(0);
  OZ_Term fea = OZ_getCArg(1);
  OZ_Term value = OZ_getCArg(2);
  OZ_Term out = OZ_getCArg(3);

  DEREF(rec,recPtr,tag0);
  DEREF(fea,feaPtr,tag1);

  switch (tag0) {
  case LITERAL:
    if (isFeature(fea)) {
      SRecord *newrec
        = SRecord::newSRecord(rec,aritytable.find(cons(fea,nil())));
      newrec->setArg(0,value);
      return oz_unify(out, makeTaggedSRecord(newrec));
    }
    if (isNotCVar(fea)) {
      oz_suspendOnPtr(feaPtr);
    }
    if (isCVar(fea)) {
      if (tagged2CVar(fea)->getType()!=OFSVariable ||
          tagged2GenOFSVar(fea)->getWidth()>0)
        oz_typeError(1,"Feature");
      oz_suspendOnPtr(feaPtr);;
    }
    oz_typeError(1,"Feature");

  case LTUPLE:
  case SRECORD:
    {
      if (isAnyVar(tag1)) {
        oz_suspendOnPtr(feaPtr);
      }
      if (!isFeature(tag1)) {
        oz_typeError(1,"Feature");
      }
      return oz_unify(out,oz_adjoinAt(makeRecord(rec),fea,value));
    }

  case UVAR:
  case SVAR:
  case CVAR:
    if (tag0==CVAR && tagged2CVar(rec)->getType()!=OFSVariable)
        oz_typeError(0,"Record");
    if (isFeature(fea) || isNotCVar(fea)) {
      oz_suspendOnPtr(recPtr);
    }
    if (isCVar(fea)) {
      if (tagged2CVar(fea)->getType()!=OFSVariable ||
          tagged2GenOFSVar(fea)->getWidth()>0)
        oz_typeError(1,"Feature");
      oz_suspendOnPtr(recPtr);
    }
    oz_typeError(1,"Feature");

  default:
    oz_typeError(0,"Record");
  }
}
OZ_C_proc_end

TaggedRef getArityFromPairList(TaggedRef list)
{
  TaggedRef arity;
  TaggedRef *next=&arity;
  Bool updateFlag=NO;
  DerefReturnVar(list);
  TaggedRef old = list;
loop:
  if (isLTuple(list)) {
    TaggedRef pair = head(list);
    DerefReturnVar(pair);
    if (!oz_isPair2(pair)) return 0;

    TaggedRef fea = tagged2SRecord(pair)->getArg(0);
    DerefReturnVar(fea);

    if (!isFeature(fea)) return 0;

    LTuple *lt=new LTuple();
    *next=makeTaggedLTuple(lt);
    lt->setHead(fea);
    next=lt->getRefTail();

    list = tail(list);
    DerefReturnVar(list);
    if (list==old) return 0;
    if (updateFlag) {
      old=deref(tail(old));
    }
    updateFlag=1-updateFlag;
    goto loop;
  }

  if (isNil(list)) {
    *next=nil();
    return arity;
  }

  return 0;
}

/* common subroutine for builtins adjoinList and record:
   recordFlag=OK: 'adjoinList' allows records as first arg
              N0: 'makeRecord' only allows literals */

OZ_Return adjoinPropListInline(TaggedRef t0, TaggedRef list, TaggedRef &out,
                           Bool recordFlag)
{
  TaggedRef arity=getArityFromPairList(list);
  if (!arity) {
    oz_typeError(1,"finite list(Feature#Value)");
  }
  DEREF(t0,t0Ptr,tag0);
  if (isRef(arity)) { // must suspend
    out=arity;
    switch (tag0) {
    case UVAR:
    case SVAR:
    case LITERAL:
      return SUSPEND;
    case SRECORD:
    case LTUPLE:
      if (recordFlag) {
        return SUSPEND;
      }
      goto typeError0;
    case CVAR:
      if (tagged2CVar(t0)->getType()!=OFSVariable)
          goto typeError0;
      if (recordFlag) {
        return SUSPEND;
      }
      goto typeError0;
    default:
      goto typeError0;
    }
  }

  if (isNil(arity)) { // adjoin nothing
    switch (tag0) {
    case SRECORD:
    case LTUPLE:
      if (recordFlag) {
        out = t0;
        return PROCEED;
      }
      goto typeError0;
    case LITERAL:
      out = t0;
      return PROCEED;
    case UVAR:
    case SVAR:
      out=makeTaggedRef(t0Ptr);
      return SUSPEND;
    case CVAR:
      if (tagged2CVar(t0)->getType()!=OFSVariable)
          goto typeError0;
      out=makeTaggedRef(t0Ptr);
      return SUSPEND;
    default:
      goto typeError0;
    }
  }

  switch (tag0) {
  case LITERAL:
    {
      int len1 = fastlength(arity);
      arity = sortlist(arity,len1);
      int len = fastlength(arity); // NOTE: duplicates may be removed
      if (!recordFlag && len!=len1) {  // handles case f(a:_ a:_)
        return oz_raise(E_ERROR,E_KERNEL,"recordConstruction",2,
                        t0,list
                        );
      }
      SRecord *newrec = SRecord::newSRecord(t0,aritytable.find(arity));
      newrec->setFeatures(list);
      out = newrec->normalize();
      return PROCEED;
    }
  case SRECORD:
  case LTUPLE:
    if (recordFlag) {
      out = oz_adjoinList(makeRecord(t0),arity,list);
      return PROCEED;
    }
    goto typeError0;
  case UVAR:
  case SVAR:
    out=makeTaggedRef(t0Ptr);
    return SUSPEND;
  case CVAR:
    if (tagged2CVar(t0)->getType()!=OFSVariable)
        goto typeError0;
    out=makeTaggedRef(t0Ptr);
    return SUSPEND;
  default:
    goto typeError0;
  }

 typeError0:
  if (recordFlag) {
    oz_typeError(0,"Record");
  } else {
    oz_typeError(0,"Literal");
  }
}

OZ_Return adjoinPropList(TaggedRef t0, TaggedRef list, TaggedRef &out,
                     Bool recordFlag)
{
  return adjoinPropListInline(t0,list,out,recordFlag);
}


OZ_C_proc_begin(BIadjoinList,3)
{
  OZ_Term help;

  OZ_Return state = adjoinPropListInline(OZ_getCArg(0),OZ_getCArg(1),help,OK);
  switch (state) {
  case SUSPEND:
    oz_suspendOn(help);
  case PROCEED:
    return(oz_unify(help,OZ_getCArg(2)));
  default:
    return state;
  }
}
OZ_C_proc_end


OZ_C_proc_begin(BImakeRecord,3)
{
  OZ_Term help;

  OZ_Return state = adjoinPropListInline(OZ_getCArg(0),OZ_getCArg(1),help,NO);
  switch (state) {
  case SUSPEND:
    oz_suspendOn(help);
    return PROCEED;
  case PROCEED:
    return(oz_unify(help,OZ_getCArg(2)));
  default:
    return state;
  }
}
OZ_C_proc_end


OZ_Return BIarityInline(TaggedRef term, TaggedRef &out)
{
  DEREF(term,termPtr,tag);

  out = getArityList(term);
  if (out) return PROCEED;
  if (isNotCVar(tag)) return SUSPEND;
  if (isCVar(tag)) {
    if (tagged2CVar(term)->getType()!=OFSVariable)
      oz_typeError(0,"Record");
    return SUSPEND;
  }
  oz_typeError(0,"Record");
}

DECLAREBI_USEINLINEFUN1(BIarity,BIarityInline)


/* -----------------------------------------------------------------------
   Numbers
   ----------------------------------------------------------------------- */

static OZ_Return bombArith(char *type)
{
  oz_typeError(-1,type);
}

#define suspendTest(A,B,test,type)                      \
  if (isAnyVar(A)) {                                    \
    if (isAnyVar(B) || test(B)) { return SUSPEND; }     \
    return bombArith(type);                             \
  }                                                     \
  if (isAnyVar(B)) {                                    \
    if (isNumber(A)) { return SUSPEND; }                \
  }                                                     \
  return bombArith(type);


static OZ_Return suspendOnNumbers(TaggedRef A, TaggedRef B)
{
  suspendTest(A,B,isNumber,"int or float\nuniformly for all arguments");
}

inline Bool isNumOrAtom(TaggedRef t)
{
  return isNumber(t) || isAtom(t);
}

static OZ_Return suspendOnNumbersAndAtoms(TaggedRef A, TaggedRef B)
{
  suspendTest(A,B,isNumOrAtom,"int, float or atom\nuniformly for all arguments");
}

static OZ_Return suspendOnFloats(TaggedRef A, TaggedRef B)
{
  suspendTest(A,B,isFloat,"Float");
}


static OZ_Return suspendOnInts(TaggedRef A, TaggedRef B)
{
  suspendTest(A,B,isInt,"Int");
}

#undef suspendTest





/* -----------------------------------
   Z = X op Y
   ----------------------------------- */

// Float x Float -> Float
OZ_Return BIfdivInline(TaggedRef A, TaggedRef B, TaggedRef &out)
{
  DEREF(A,_1,tagA);
  DEREF(B,_2,tagB);
  if (isFloat(tagA) && isFloat(tagB)) {
    out = oz_float(floatValue(A) / floatValue(B));
    return PROCEED;
  }
  return suspendOnFloats(A,B);
}


// Int x Int -> Int
#define BIGOP(op)                                                             \
  if (tagA == BIGINT) {                                                       \
    if (tagB == BIGINT) {                                                     \
      out = tagged2BigInt(A)->op(tagged2BigInt(B));                           \
      return PROCEED;                                                         \
    }                                                                         \
    if (tagB == SMALLINT) {                                                   \
      BigInt *b = new BigInt(smallIntValue(B));                               \
      out = tagged2BigInt(A)->op(b);                                          \
      b->dispose();                                                           \
      return PROCEED;                                                         \
    }                                                                         \
  }                                                                           \
  if (tagB == BIGINT) {                                                       \
    if (tagA == SMALLINT) {                                                   \
      BigInt *a = new BigInt(smallIntValue(A));                               \
      out = a->op(tagged2BigInt(B));                                          \
      a->dispose();                                                           \
      return PROCEED;                                                         \
    }                                                                         \
  }

// Integer x Integer -> Integer
OZ_Return BIdivInline(TaggedRef A, TaggedRef B, TaggedRef &out)
{
  DEREF(A,_1,tagA);
  DEREF(B,_2,tagB);

  if (tagB == SMALLINT && smallIntValue(B) == 0) {
    if (tagA == SMALLINT || tagA == BIGINT) {
      return oz_raise(E_ERROR,E_KERNEL,"div0",1,A);
    } else {
      return bombArith("Int");
    }
  }

  if ( (tagA == SMALLINT) && (tagB == SMALLINT)) {
    out = newSmallInt(smallIntValue(A) / smallIntValue(B));
    return PROCEED;
  }
  BIGOP(div);
  return suspendOnInts(A,B);
}

// Integer x Integer -> Integer
OZ_Return BImodInline(TaggedRef A, TaggedRef B, TaggedRef &out)
{
  DEREF(A,_1,tagA);
  DEREF(B,_2,tagB);

  if ((tagB == SMALLINT && smallIntValue(B) == 0)) {
    if (tagA == SMALLINT || tagA == BIGINT) {
      return oz_raise(E_ERROR,E_KERNEL,"mod0",1,A);
    } else {
      return bombArith("Int");
    }
  }

  if ( (tagA == SMALLINT) && (tagB == SMALLINT)) {
    out = newSmallInt(smallIntValue(A) % smallIntValue(B));
    return PROCEED;
  }

  BIGOP(mod);
  return suspendOnInts(A,B);
}


/* Division is slow on RISC (at least SPARC)
 *  --> first make a simpler test for no overflow
 */

inline
int multOverflow(int a, int b)
{
  int absa = ozabs(a);
  int absb = ozabs(b);
  const int bits = (sizeof(TaggedRef)*8-tagSize)/2 - 1;

  if (!((absa|absb)>>bits)) /* if none of the 13 MSB in neither a nor b are set */
    return NO;
  return ((b!=0) && (absa >= OzMaxInt / absb));
}


OZ_Return BImultInline(TaggedRef A, TaggedRef B, TaggedRef &out)
{
  DEREF(A,_1,tagA);
  DEREF(B,_2,tagB);

  if (tagA == SMALLINT && tagB == SMALLINT) {
    int valA = smallIntValue(A);
    int valB = smallIntValue(B);
    if ( multOverflow(valA,valB) ) {
      BigInt *a = new BigInt(valA);
      BigInt *b = new BigInt(valB);
      out = a->mul(b);
      a->dispose();
      b->dispose();
      return PROCEED;
    } else {
      out = newSmallInt(valA*valB);
      return PROCEED;
    }
  }

  if (isFloat(tagA) && isFloat(tagB)) {
    out = oz_float(floatValue(A) * floatValue(B));
    return PROCEED;
  }

  BIGOP(mul);
  return suspendOnNumbers(A,B);
}


OZ_Return BIminusInline(TaggedRef A, TaggedRef B, TaggedRef &out)
{
  DEREF(A,_1,_11);
  DEREF(B,_2,_22);

  if ( isSmallInt(A) && isSmallInt(B) ) {
    out = makeInt(smallIntValue(A) - smallIntValue(B));
    return PROCEED;
  }

  if (isFloat(A) && isFloat(B)) {
    out = oz_float(floatValue(A) - floatValue(B));
    return PROCEED;
  }


  TypeOfTerm tagA = tagTypeOf(A);
  TypeOfTerm tagB = tagTypeOf(B);
  BIGOP(sub);
  return suspendOnNumbers(A,B);
}

OZ_Return BIplusInline(TaggedRef A, TaggedRef B, TaggedRef &out)
{
  DEREF(A,_1,_11);
  DEREF(B,_2,_22);

  if ( isSmallInt(A) && isSmallInt(B) ) {
    out = makeInt(smallIntValue(A) + smallIntValue(B));
    return PROCEED;
  }

  if (isFloat(A) && isFloat(B)) {
    out = oz_float(floatValue(A) + floatValue(B));
    return PROCEED;
  }

  TypeOfTerm tagA = tagTypeOf(A);
  TypeOfTerm tagB = tagTypeOf(B);
  BIGOP(add);
  return suspendOnNumbers(A,B);
}


OZ_Return BIminusOrPlus(Bool callPlus,TaggedRef A, TaggedRef B, TaggedRef &out)
{
  return callPlus ?  BIplusInline(A,B,out) : BIminusInline(A,B,out);
}


#undef BIGOP

/* -----------------------------------
   Z = op X
   ----------------------------------- */

// unary minus: Number -> Number
OZ_Return BIuminusInline(TaggedRef A, TaggedRef &out)
{
  DEREF(A,_1,tagA);

  switch(tagA) {

  case OZFLOAT:
    out = oz_float(-floatValue(A));
    return PROCEED;

  case SMALLINT:
    out = newSmallInt(-smallIntValue(A));
    return PROCEED;

  case BIGINT:
    out = tagged2BigInt(A)->neg();
    return PROCEED;

  case UVAR:
  case SVAR:
    return SUSPEND;

  default:
    oz_typeError(0,"Int");
  }
}

OZ_Return BIabsInline(TaggedRef A, TaggedRef &out)
{
  DEREF(A,_1,tagA);

  if (isSmallInt(tagA)) {
    int i = smallIntValue(A);
    out = (i >= 0) ? A : newSmallInt(-i);
    return PROCEED;
  }

  if (isFloat(tagA)) {
    double f = floatValue(A);
    out = (f >= 0.0) ? A : oz_float(fabs(f));
    return PROCEED;
  }

  if (isBigInt(tagA)) {
    BigInt *b = tagged2BigInt(A);
    out = (b->cmp(0l) >= 0) ? A : b->neg();
    return PROCEED;
  }

  if (isAnyVar(tagA)){
    return SUSPEND;
  }

  oz_typeError(0,"Number");
}

// add1(X) --> X+1
OZ_Return BIadd1Inline(TaggedRef A, TaggedRef &out)
{
  DEREF(A,_1,tagA);

  if (isSmallInt(tagA)) {
    /* INTDEP */
    int res = (int)A + (1<<tagSize);
    if ((int)A < res) {
      out = res;
      return PROCEED;
    }
  }

  return BIplusInline(A,makeTaggedSmallInt(1),out);
}

// sub1(X) --> X-1
OZ_Return BIsub1Inline(TaggedRef A, TaggedRef &out)
{
  DEREF(A,_1,tagA);

  if (isSmallInt(tagA)) {
    /* INTDEP */
    int res = (int)A - (1<<tagSize);
    if ((int)A > res) {
      out = res;
      return PROCEED;
    }
  }

  return BIminusInline(A,makeTaggedSmallInt(1),out);
}


/* -----------------------------------
   X test Y
   ----------------------------------- */

OZ_Return bigintLess(BigInt *A, BigInt *B)
{
  return (A->cmp(B) < 0 ? PROCEED : FAILED);
}


OZ_Return bigintLe(BigInt *A, BigInt *B)
{
  return (A->cmp(B) <= 0 ? PROCEED : FAILED);
}


OZ_Return bigtest(TaggedRef A, TaggedRef B,
                  OZ_Return (*test)(BigInt*, BigInt*))
{
  if (isBigInt(A)) {
    if (isBigInt(B)) {
      return test(tagged2BigInt(A),tagged2BigInt(B));
    }
    if (isSmallInt(B)) {
      BigInt *b = new BigInt(smallIntValue(B));
      OZ_Return res = test(tagged2BigInt(A),b);
      b->dispose();
      return res;
    }
  }
  if (isBigInt(B)) {
    if (isSmallInt(A)) {
      BigInt *a = new BigInt(smallIntValue(A));
      OZ_Return res = test(a,tagged2BigInt(B));
      a->dispose();
      return res;
    }
  }
  if (isAnyVar(A) || isAnyVar(B))
    return SUSPEND;

  oz_typeError(-1,"int, float or atom\nuniformly for all arguments");
}




OZ_Return BIminInline(TaggedRef A, TaggedRef B, TaggedRef &out)
{
  DEREF(A,_1,tagA);
  DEREF(B,_2,tagB);

  if (tagA == tagB) {
    switch(tagA) {
    case SMALLINT: out = (smallIntLess(A,B) ? A : B);             return PROCEED;
    case OZFLOAT:  out = (floatValue(A) < floatValue(B)) ? A : B; return PROCEED;
    case LITERAL:
      if (isAtom(A) && isAtom(B)) {
        out = (strcmp(tagged2Literal(A)->getPrintName(),
                      tagged2Literal(B)->getPrintName()) < 0)
          ? A : B;
        return PROCEED;
      }
      oz_typeError(-1,"Comparable");

    default: break;
    }
  }

  OZ_Return ret = bigtest(A,B,bigintLess);
  switch (ret) {
  case PROCEED: out = A; return PROCEED;
  case FAILED:  out = B; return PROCEED;
  case RAISE:   return RAISE;
  default:      break;
  }

  return suspendOnNumbersAndAtoms(A,B);
}


/* code adapted from min */
OZ_Return BImaxInline(TaggedRef A, TaggedRef B, TaggedRef &out)
{
  DEREF(A,_1,tagA);
  DEREF(B,_2,tagB);

  if (tagA == tagB) {
    switch(tagA) {
    case SMALLINT:
      out = (smallIntLess(A,B) ? B : A);
      return PROCEED;
    case OZFLOAT:
      out = (floatValue(A) < floatValue(B)) ? B : A;
      return PROCEED;
    case LITERAL:
      if (isAtom(A) && isAtom(B)) {
        out = (strcmp(tagged2Literal(A)->getPrintName(),
                      tagged2Literal(B)->getPrintName()) < 0)
          ? B : A;
        return PROCEED;
      }
      oz_typeError(-1,"Comparable");

    default: break;
    }
  }

  OZ_Return ret = bigtest(A,B,bigintLess);
  switch (ret) {
  case PROCEED: out = B; return PROCEED;
  case FAILED:  out = A; return PROCEED;
  case RAISE:   return RAISE;
  default:      break;
  }

  return suspendOnNumbersAndAtoms(A,B);
}


OZ_Return BIlessInline(TaggedRef A, TaggedRef B)
{
  DEREF(A,_1,tagA);
  DEREF(B,_2,tagB);

  if (tagA == tagB) {
    if (tagA == SMALLINT) {
      return (smallIntLess(A,B) ? PROCEED : FAILED);
    }

    if (isFloat(tagA)) {
      return (floatValue(A) < floatValue(B)) ? PROCEED : FAILED;
    }

    if (tagA == LITERAL) {
      if (isAtom(A) && isAtom(B)) {
        return (strcmp(tagged2Literal(A)->getPrintName(),
                       tagged2Literal(B)->getPrintName()) < 0)
          ? PROCEED : FAILED;
      }
      oz_typeError(-1,"Comparable");
    }
  }

  OZ_Return ret = bigtest(A,B,bigintLess);
  if (ret!=SUSPEND)
    return ret;

  return suspendOnNumbersAndAtoms(A,B);
}


OZ_Return BIlessInlineFun(TaggedRef A, TaggedRef B, TaggedRef &out)
{
  OZ_Return ret = BIlessInline(A,B);
  switch (ret) {
  case PROCEED: out = NameTrue;  return PROCEED;
  case FAILED:  out = NameFalse; return PROCEED;
  default:      return ret;
  }
}

OZ_Return BIgreatInline(TaggedRef A, TaggedRef B)
{
  return BIlessInline(B,A);
}

OZ_Return BIgreatInlineFun(TaggedRef A, TaggedRef B, TaggedRef &out)
{
  return BIlessInlineFun(B,A,out);
}


OZ_Return BIleInline(TaggedRef A, TaggedRef B)
{
  DEREF(A,_1,tagA);
  DEREF(B,_2,tagB);

  if (tagA == tagB) {

    if (tagA == SMALLINT) {
      return (smallIntLE(A,B) ? PROCEED : FAILED);
    }

    if (isFloat(tagA)) {
      return (floatValue(A) <= floatValue(B)) ? PROCEED : FAILED;
    }

    if (tagA == LITERAL) {
      if (isAtom(A) && isAtom(B)) {
        return (strcmp(tagged2Literal(A)->getPrintName(),
                       tagged2Literal(B)->getPrintName()) <= 0)
          ? PROCEED : FAILED;
      }
      oz_typeError(-1,"Comparable");
    }

  }

  OZ_Return ret = bigtest(A,B,bigintLe);
  if (ret!=SUSPEND)
    return ret;

  return suspendOnNumbersAndAtoms(A,B);
}


OZ_Return BIleInlineFun(TaggedRef A, TaggedRef B, TaggedRef &out)
{
  OZ_Return ret = BIleInline(A,B);
  switch (ret) {
  case PROCEED: out = NameTrue;  return PROCEED;
  case FAILED:  out = NameFalse; return PROCEED;
  default:      return ret;
  }
}



OZ_Return BIgeInlineFun(TaggedRef A, TaggedRef B, TaggedRef &out)
{
  return BIleInlineFun(B,A,out);
}


OZ_Return BIgeInline(TaggedRef A, TaggedRef B)
{
  return BIleInline(B,A);
}

OZ_Return BILessOrLessEq(Bool callLess, TaggedRef A, TaggedRef B)
{
  return callLess ? BIlessInline(A,B) : BIleInline(A,B);
}


/* -----------------------------------
   X = conv(Y)
   ----------------------------------- */


OZ_Return BIintToFloatInline(TaggedRef A, TaggedRef &out)
{
  DEREF(A,_1,_2);
  if (isSmallInt(A)) {
    out = oz_float((double)smallIntValue(A));
    return PROCEED;
  }
  if (isBigInt(A)) {
    char *s = toC(A);
    out = OZ_CStringToFloat(s);
    return PROCEED;
  }

  if (oz_isVariable(A)) {
    return SUSPEND;
  }

  oz_typeError(0,"Int");
}

/* mm2: I don't know if this is efficient, but it's only called,
   when rounding "xx.5" */
inline
Bool ozisodd(double ff)
{
  double m = ff/2;
  return m != floor(m);
}

/* ozround -- round float to int as required by IEEE Standard */
inline
double ozround(double in) {
  double ff = floor(in);
  double diff = in-ff;
  if (diff > 0.5 || (diff == 0.5 && ozisodd(ff))) {
    ff += 1;
  }
  return ff;
}

OZ_Return BIfloatToIntInline(TaggedRef A, TaggedRef &out) {
  A=deref(A);

  if (isAnyVar(A))
    return SUSPEND;

  if (isFloat(A)) {
    double ff = ozround(floatValue(A));
    if (ff > INT_MAX || ff < INT_MIN) {
      OZ_warning("float to int: truncated to signed 32 Bit\n");
    }
    out = makeInt((int) ff);
    return PROCEED;
  }

  oz_typeError(-1,"Float");
}

OZ_C_proc_begin(BIfloatToString, 2)
{
  oz_declareNonvarArg(0,in);
  oz_declareArg(1,out);

  if (oz_isFloat(in)) {
    char *s = OZ_toC(in,100,100); // mm2
    return oz_unify(out,OZ_string(s));
  }
  oz_typeError(0,"Float");
}
OZ_C_proc_end

OZ_C_proc_begin(BIstringToFloat, 2)
{
  oz_declareProperStringArg(0,str);
  oz_declareArg(1,out);

  char *end = OZ_parseFloat(str);
  if (!end || *end != 0) {
    return oz_raise(E_ERROR,E_KERNEL,"stringNoFloat",1,OZ_getCArg(0));
  }
  OZ_Return ret = oz_unify(out,OZ_CStringToFloat(str));
  return ret;
}
OZ_C_proc_end

OZ_C_proc_begin(BIstringIsFloat, 2)
{
  oz_declareProperStringArg(0,str);
  oz_declareArg(1,out);

  if (!str) return oz_unify(out,NameFalse);

  char *end = OZ_parseFloat(str);

  if (!end || *end != 0) {
    return oz_unify(out,NameFalse);
  }

  return oz_unify(out,NameTrue);
}
OZ_C_proc_end

OZ_C_proc_begin(BIstringIsAtom, 2) {
  OZ_Term in = OZ_getCArg(0);
  OZ_Term out= OZ_getCArg(1);

  OZ_Term var;
  if (!OZ_isProperString(in,&var)) {
    if (var == 0) return oz_unify(out,NameFalse);
    oz_suspendOn(var);
  }
  return oz_unify(out,NameTrue);
}
OZ_C_proc_end

OZ_C_proc_begin(BIstringToInt, 2)
{
  oz_declareProperStringArg(0,str);
  oz_declareArg(1,out);

  if (!str) return oz_raise(E_ERROR,E_KERNEL,"stringNoInt",1,OZ_getCArg(0));


  OZ_Term res = OZ_CStringToInt(str);
  if (res == 0)
    return oz_raise(E_ERROR,E_KERNEL,"stringNoInt",1,OZ_getCArg(0));
  else
    return oz_unify(out,res);
}
OZ_C_proc_end

OZ_C_proc_begin(BIstringIsInt, 2)
{
  oz_declareProperStringArg(0,str);
  oz_declareArg(1,out);

  if (!str) return oz_unify(out,NameFalse);

  char *end = OZ_parseInt(str);

  if (!end || *end != 0) {
    return oz_unify(out,NameFalse);
  }

  return oz_unify(out,NameTrue);
}
OZ_C_proc_end

OZ_C_proc_begin(BIintToString, 2)
{
  oz_declareNonvarArg(0,in);
  oz_declareArg(1,out);

  if (oz_isInt(in)) {
    return oz_unify(out,OZ_string(OZ_toC(in,100,100))); //mm2
  }
  oz_typeError(0,"Int");
}
OZ_C_proc_end

/* -----------------------------------
   type X
   ----------------------------------- */

OZ_Return BIisFloatInline(TaggedRef num)
{
  DEREF(num,_,tag);

  if (isAnyVar(tag)) {
    return SUSPEND;
  }

  return isFloat(tag) ? PROCEED : FAILED;
}

DECLAREBI_USEINLINEREL1(BIisFloat,BIisFloatInline)
DECLAREBOOLFUN1(BIisFloatB,BIisFloatBInline,BIisFloatInline)

OZ_Return BIisIntInline(TaggedRef num)
{
  DEREF(num,_,tag);

  if (isAnyVar(tag)) {
    return SUSPEND;
  }

  return isInt(tag) ? PROCEED : FAILED;
}

DECLAREBI_USEINLINEREL1(BIisInt,BIisIntInline)
DECLAREBOOLFUN1(BIisIntB,BIisIntBInline,BIisIntInline)



OZ_Return BIisNumberInline(TaggedRef num)
{
  DEREF(num,_,tag);

  if (isAnyVar(tag)) {
    return SUSPEND;
  }

  return isNumber(tag) ? PROCEED : FAILED;
}

DECLAREBI_USEINLINEREL1(BIisNumber,BIisNumberInline)
DECLAREBOOLFUN1(BIisNumberB,BIisNumberBInline,BIisNumberInline)


/* -----------------------------------------------------------------------
   misc. floating point functions
   ----------------------------------------------------------------------- */


#define FLOATFUN(Fun,BIName,InlineName)                 \
OZ_Return InlineName(TaggedRef AA, TaggedRef &out)      \
{                                                       \
  DEREF(AA,_,tag);                                      \
                                                        \
  if (isAnyVar(tag)) {                                  \
    return SUSPEND;                                     \
  }                                                     \
                                                        \
  if (isFloat(tag)) {                                   \
    out = oz_float(Fun(floatValue(AA)));                \
    return PROCEED;                                     \
  }                                                     \
  oz_typeError(0,"Float");                              \
}                                                       \
DECLAREBI_USEINLINEFUN1(BIName,InlineName)


FLOATFUN(exp, BIexp, BIinlineExp)
FLOATFUN(log, BIlog, BIinlineLog)
FLOATFUN(sqrt,BIsqrt,BIinlineSqrt)
FLOATFUN(sin, BIsin, BIinlineSin)
FLOATFUN(asin,BIasin,BIinlineAsin)
FLOATFUN(cos, BIcos, BIinlineCos)
FLOATFUN(acos,BIacos,BIinlineAcos)
FLOATFUN(tan, BItan, BIinlineTan)
FLOATFUN(atan,BIatan,BIinlineAtan)
FLOATFUN(ceil,BIceil,BIinlineCeil)
FLOATFUN(floor,BIfloor,BIinlineFloor)
FLOATFUN(ozround, BIround, BIinlineRound)
#undef FLOATFUN


OZ_Return BIfPowInline(TaggedRef A, TaggedRef B, TaggedRef &out)
{
  DEREF(A,_1,tagA);
  DEREF(B,_2,tagB);

  if (isFloat(tagA) && isFloat(tagB)) {
    out = oz_float(pow(floatValue(A),floatValue(B)));
    return PROCEED;
  }
  return suspendOnFloats(A,B);
}

OZ_Return BIatan2Inline(TaggedRef A, TaggedRef B, TaggedRef &out)
{
  DEREF(A,_1,tagA);
  DEREF(B,_2,tagB);

  if (isFloat(tagA) && isFloat(tagB)) {
    out = oz_float(atan2(floatValue(A),floatValue(B)));
    return PROCEED;
  }
  return suspendOnFloats(A,B);
}


/* -----------------------------------
   make non inline versions
   ----------------------------------- */

DECLAREBI_USEINLINEREL2(BIless,BIlessInline)
DECLAREBI_USEINLINEREL2(BIle,BIleInline)
DECLAREBI_USEINLINEREL2(BIgreat,BIgreatInline)
DECLAREBI_USEINLINEREL2(BIge,BIgeInline)

DECLAREBI_USEINLINEFUN2(BIplus,BIplusInline)
DECLAREBI_USEINLINEFUN2(BIminus,BIminusInline)

DECLAREBI_USEINLINEFUN2(BImult,BImultInline)
DECLAREBI_USEINLINEFUN2(BIdiv,BIdivInline)
DECLAREBI_USEINLINEFUN2(BIfdiv,BIfdivInline)
DECLAREBI_USEINLINEFUN2(BImod,BImodInline)

DECLAREBI_USEINLINEFUN2(BIfPow,BIfPowInline)
DECLAREBI_USEINLINEFUN2(BIatan2,BIatan2Inline)

DECLAREBI_USEINLINEFUN2(BImax,BImaxInline)
DECLAREBI_USEINLINEFUN2(BImin,BIminInline)

DECLAREBI_USEINLINEFUN2(BIlessFun,BIlessInlineFun)
DECLAREBI_USEINLINEFUN2(BIleFun,BIleInlineFun)
DECLAREBI_USEINLINEFUN2(BIgreatFun,BIgreatInlineFun)
DECLAREBI_USEINLINEFUN2(BIgeFun,BIgeInlineFun)

DECLAREBI_USEINLINEFUN1(BIintToFloat,BIintToFloatInline)
DECLAREBI_USEINLINEFUN1(BIfloatToInt,BIfloatToIntInline)
DECLAREBI_USEINLINEFUN1(BIuminus,BIuminusInline)
DECLAREBI_USEINLINEFUN1(BIabs,BIabsInline)
DECLAREBI_USEINLINEFUN1(BIadd1,BIadd1Inline)
DECLAREBI_USEINLINEFUN1(BIsub1,BIsub1Inline)

// ---------------------------------------------------------------------
// Ports
// ---------------------------------------------------------------------

OZ_C_proc_begin(BInewPort,2)
{
  oz_declareArg(0,val);
  oz_declareArg(1,out);

  OZ_Term ret = oz_newPort(val);
  return oz_unify(out,ret);
}
OZ_C_proc_end

OZ_Return sendPort(OZ_Term prt, OZ_Term val)
{
  Assert(isPort(prt));

  Port *port  = tagged2Port(prt);
  TertType tt = port->getTertType();

  CheckLocalBoard(port,"port");

  if(tt==Te_Proxy) {
    portSend(port,val);
    return PROCEED;
  }
  LTuple *lt = new LTuple(val,am.currentUVarPrototype());

  OZ_Term old = ((PortWithStream*)port)->exchangeStream(lt->getTail());
  DEREF(old,oldPtr,_1);

  if(isAnyVar(old) && !isCVar(old)) {
    am.bindToNonvar(oldPtr,old,makeTaggedLTuple(lt),0);
  } else {
    old = makeTaggedRef(oldPtr);
    if (oz_unify(makeTaggedLTuple(lt),old)!=PROCEED) {
      /* respect port semantics:
       *
       * fun {NewPort S}
       *   C={NewCell S}
       * in
       *   proc {$ Val}
       *     {Exchange Cell Old T}
       *     thread Old=H|T end         %% !!!
       *   end
       * end
       */
      OZ_unifyInThread(makeTaggedLTuple(lt),old);
    }
  }
  return PROCEED;
}


OZ_C_proc_begin(BIsendPort,2)
{
  oz_declareNonvarArg(0,prt);
  oz_declareArg(1,val);

  if (!isPort(prt)) {
    oz_typeError(0,"Port");
  }

  return sendPort(prt,val);
}
OZ_C_proc_end

// ---------------------------------------------------------------------
// Locks
// ---------------------------------------------------------------------

OZ_C_proc_begin(BInewLock,1)
{
  oz_declareArg(0,out);

  OZ_Term ret = makeTaggedConst(new LockLocal(am.currentBoard()));

  return oz_unify(out,ret);
}
OZ_C_proc_end


OZ_C_proc_begin(BIlockLock,1)
{
  oz_declareNonvarArg(0,lock);

  if (!isLock(lock)) {
    oz_typeError(0,"Lock");
  }

  Tertiary *t=tagged2Tert(lock);
  if(t->getTertType()==Te_Local){
    LockLocal *ll=(LockLocal*)t;
    if (!am.onToplevel()) {
      if (!am.isCurrentBoard(GETBOARD(ll))) {
        return oz_raise(E_ERROR,E_KERNEL,"globalState",1,oz_atom("lock"));
      }}
    ll->lock(am.currentThread());
    return PROCEED;}
  if(!am.onToplevel()){
    return oz_raise(E_ERROR,E_KERNEL,"globalState",1,oz_atom("lock"));}

  switch(t->getTertType()){
  case Te_Manager:{
    ((LockManager *)t)->lock(am.currentThread());
    return PROCEED;}
  case Te_Proxy:{
    ((LockProxy*)t)->lock(am.currentThread());
    return PROCEED;}
  case Te_Frame:{
    ((LockFrame*)t)->lock(am.currentThread());
    return PROCEED;}
  default:
    Assert(0);}

  Assert(0);
  return PROCEED;
}
OZ_C_proc_end


OZ_C_proc_begin(BIunlockLock,1)
{
  oz_declareNonvarArg(0,lock);

  if (!isLock(lock)) {
    oz_typeError(0,"Lock");
  }
  Tertiary *t=tagged2Tert(lock);
  switch(t->getTertType()){
  case Te_Local:{
    ((LockLocal*)t)->unlock();
    return PROCEED;}
  case Te_Manager:{
    ((LockManager*)t)->unlock(am.currentThread());
    return PROCEED;}
  case Te_Proxy:{
    return oz_raise(E_ERROR,E_KERNEL,"globalState",1,oz_atom("lock"));}
  case Te_Frame:{
    ((LockFrame*)t)->unlock(am.currentThread());
    return PROCEED;}
  }
  Assert(0);
  return PROCEED;
}
OZ_C_proc_end


// ---------------------------------------------------------------------
// Cell
// ---------------------------------------------------------------------

OZ_C_proc_begin(BInewCell,2)
{
  OZ_Term val = OZ_getCArg(0);
  OZ_Term out = OZ_getCArg(1);

  return oz_unify(out,oz_newCell(val));
}
OZ_C_proc_end


inline
OZ_Return checkSuspend()
{
  return am.currentThread()->getPStop() ? BI_PREEMPT : PROCEED;
}


OZ_Return BIexchangeCellInline(TaggedRef c, TaggedRef oldVal, TaggedRef newVal)
{
  NONVAR(c,rec);

  if (!isCell(rec)) {oz_typeError(0,"Cell");}

  Tertiary *tert = tagged2Tert(rec);
  if(tert->getTertType()!=Te_Local){
    cellDoExchange(tert,oldVal,newVal,am.currentThread());
    return checkSuspend();
  }

  CellLocal *cell=(CellLocal*)tert;
  CheckLocalBoard(cell,"cell");
  return oz_unify(cell->exchangeValue(newVal),oldVal);
}

/* we do not use here DECLAREBI_USEINLINEFUN2,
 * since we only want to suspend on the first argument
 */
OZ_C_proc_begin(BIexchangeCell,3)
{
  OZ_Term cell     = OZ_getCArg(0);
  OZ_Term inState  = OZ_getCArg(1);
  OZ_Term outState = OZ_getCArg(2);
  OZ_Return state=BIexchangeCellInline(cell,inState,outState);
  if (state==SUSPEND) {
    oz_suspendOn(cell);
  } else {
    return state;
  }
}
OZ_C_proc_end


OZ_Return BIaccessCellInline(TaggedRef c, TaggedRef &out)
{
  NONVAR(c,rec);

  if (!isCell(rec)) {
    oz_typeError(0,"Cell");
  }
  Tertiary *tert=tagged2Tert(rec);
  if(tert->getTertType()!=Te_Local){
    TaggedRef out = oz_newVariable(); /* ATTENTION - clumsy */
    cellDoAccess(tert,out);
    return checkSuspend();}
  CellLocal *cell = (CellLocal*)tert;
  out = cell->getValue();
  return PROCEED;
}

DECLAREBI_USEINLINEFUN1(BIaccessCell,BIaccessCellInline)

OZ_Return BIassignCellInline(TaggedRef c, TaggedRef in)
{
  NONVAR(c,rec);

  if (!isCell(rec)) {
    oz_typeError(0,"Cell");
  }

  Tertiary *tert = tagged2Tert(rec);
  if(tert->getTertType()!=Te_Local){
    TaggedRef oldIgnored = oz_newVariable();
    cellDoExchange(tert,oldIgnored,in,am.currentThread());
    return checkSuspend();
  }

  CellLocal *cell=(CellLocal*)tert;
  CheckLocalBoard(cell,"cell");
  cell->setValue(in);
  return PROCEED;
}

DECLAREBI_USEINLINEREL2(BIassignCell,BIassignCellInline)

/********************************************************************
 * Arrays
 ******************************************************************** */

OZ_C_proc_begin(BIarrayNew,4)
{
  oz_declareIntArg(0,ilow);
  oz_declareIntArg(1,ihigh);
  oz_declareArg(2,initValue);
  oz_declareArg(3,out);


  OZ_Term arr = makeTaggedConst(new OzArray(am.currentBoard(),
                                            ilow,ihigh,initValue));
  return oz_unify(arr, out);
}
OZ_C_proc_end


OZ_Return isArrayInline(TaggedRef t, TaggedRef &out)
{
  NONVAR( t, term );
  out = isArray(term) ? NameTrue : NameFalse;
  return PROCEED;
}

DECLAREBI_USEINLINEFUN1(BIisArray,isArrayInline)

OZ_Return arrayLowInline(TaggedRef t, TaggedRef &out)
{
  NONVAR( t, term );
  if (!isArray(term)) {
    oz_typeError(0,"Array");
  }
  out = makeInt(tagged2Array(term)->getLow());
  return PROCEED;
}
DECLAREBI_USEINLINEFUN1(BIarrayLow,arrayLowInline)

OZ_Return arrayHighInline(TaggedRef t, TaggedRef &out)
{
  NONVAR( t, term );
  if (!isArray(term)) {
    oz_typeError(0,"Array");
  }
  out = makeInt(tagged2Array(term)->getHigh());
  return PROCEED;
}

DECLAREBI_USEINLINEFUN1(BIarrayHigh,arrayHighInline)

OZ_Return arrayGetInline(TaggedRef t, TaggedRef i, TaggedRef &out)
{
  NONVAR( t, array );
  NONVAR( i, index );

  if (!isArray(array)) {
    oz_typeError(0,"Array");
  }

  if (!isSmallInt(index)) {
    oz_typeError(1,"smallInteger");
  }

  OzArray *ar = tagged2Array(array);
  out = ar->getArg(smallIntValue(index));
  if (out) return PROCEED;
  return oz_raise(E_ERROR,E_KERNEL,"array",2,array,index);
}
DECLAREBI_USEINLINEFUN2(BIarrayGet,arrayGetInline)

OZ_Return arrayPutInline(TaggedRef t, TaggedRef i, TaggedRef value)
{
  NONVAR( t, array );
  NONVAR( i, index );

  if (!isArray(array)) {
    oz_typeError(0,"Array");
  }

  if (!isSmallInt(index)) {
    oz_typeError(1,"smallInteger");
  }

  OzArray *ar = tagged2Array(array);
  CheckLocalBoard(ar,"array");
  if (ar->setArg(smallIntValue(index),value)) return PROCEED;

  return oz_raise(E_ERROR,E_KERNEL,"array",2,array,index);
}

DECLAREBI_USEINLINEREL3(BIarrayPut,arrayPutInline)


// ---------------------------------------------------------------------
// Perdio stuff
// ---------------------------------------------------------------------
OZ_C_proc_begin(BIcrash,0)   /* only for debugging */
{
  exit(1);

  return PROCEED;
}
OZ_C_proc_end

OZ_C_proc_begin(BIrestop,1)
{
  OZ_Term entity = OZ_getCArg(0);
  DEREF(entity,entityPtr,entityTag);

  switch(entityTag){
  case OZCONST:{
    switch(tagged2Const(entity)->getType()){
    case Co_Cell:
    case Co_Lock:{
      tagged2Tert(entity)->restop();
      break;}
    default: Assert(0);}
    break;}
  default:{
    Assert(0);}}
  oz_suspendOnNet(am.currentThread());
  return BI_PREEMPT;
}
OZ_C_proc_end

OZ_C_proc_begin(BIhandlerInstall,3)
{
  OZ_Term entity0   = OZ_getCArg(0);
  OZ_Term cond0     = OZ_getCArg(1);
  OZ_Term proc     = OZ_getCArg(2);

  NONVAR(cond0,cond);
  NONVAR(entity0,entity);

  EntityCond ec;
  if(cond==AtomPermBlocked) {ec=PERM_BLOCKED;}
  else{
    if(cond==AtomTempBlocked) {ec=TEMP_BLOCKED|PERM_BLOCKED;}
    else {
      return oz_raise(E_ERROR,E_SYSTEM,"invalid handler condition",0);}}


  Tertiary *tert = tagged2Tert(entity);
  if((tert->getType()!=Co_Cell) && (tert->getType()!=Co_Lock)){
    return oz_raise(E_ERROR,E_SYSTEM,"handlers on ? not implemented",0);}
  if(tert->installHandler(ec,proc,am.currentThread())){
    return PROCEED;}
  return oz_raise(E_ERROR,E_SYSTEM,"handler already installed",0);
}
OZ_C_proc_end

Bool translateWatcherCond(TaggedRef tr,EntityCond &ec){
  TaggedRef car;
  TaggedRef cdr=tr;
  ec=ENTITY_NORMAL;

  while(!isNil(cdr)){
    if(isAnyVar(cdr)) {
      return NO;}
    if(!isCons(cdr)){
      return NO;
      return OK;}
    car=tagged2LTuple(cdr)->getHead();
    if(isAnyVar(car)) {
      return NO;}
    cdr=tagged2LTuple(cdr)->getTail();
    if(car==AtomPermMe){
      ec|=PERM_BLOCKED;
      continue;}
    if(car==AtomTempMe) {
      ec |= (TEMP_BLOCKED|PERM_BLOCKED);
      continue;}
    if(car==AtomPermAllOthers){
      ec |= PERM_ALL;
      continue;}
    if(car==AtomTempAllOthers){
      ec |= (TEMP_ALL|PERM_ALL);
      continue;}
    if(car==AtomPermSomeOther){
      ec |= PERM_SOME;
      continue;}
    if(car==AtomTempSomeOther){
      ec |= (TEMP_SOME|PERM_SOME);
      continue;}
    return NO;}
  if(ec==ENTITY_NORMAL) return NO;
  return ec;}

OZ_C_proc_begin(BIwatcherInstall,3)
{
  OZ_Term e        = OZ_getCArg(0);
  OZ_Term c        = OZ_getCArg(1);
  OZ_Term proc     = OZ_getCArg(2);


  NONVAR(c, cond);
  /*
  if(isAnyVar(cond)) {return SUSPEND;}
  */
  EntityCond ec;

  if(!translateWatcherCond(cond,ec)){
    return oz_raise(E_ERROR,E_SYSTEM,"invalid watcher condition",0);}
  NONVAR(e, entity);

  /*
  if(isAnyVar(entity)){
  return oz_raise(E_ERROR,E_SYSTEM,"handlers on variables not implemented",0);      }
  */


  Tertiary *tert = tagged2Tert(entity);
  if((tert->getType()!=Co_Cell) && (tert->getType()!=Co_Lock)){
    return oz_raise(E_ERROR,E_SYSTEM,"watchers on ? not implemented",0);}
  tert->installWatcher(ec,proc);
  return PROCEED;
}
OZ_C_proc_end


/********************************************************************
 *   Dictionaries
 ******************************************************************** */

OZ_C_proc_begin(BIdictionaryNew,1)
{
  oz_declareArg(0,out);

  return oz_unify(makeTaggedConst(new OzDictionary(am.currentBoard())),out);
}
OZ_C_proc_end


OZ_C_proc_begin(BIdictionaryKeys,2)
{
  oz_declareDictionaryArg(0,dict);
  oz_declareArg(1,out);

  return oz_unify(dict->keys(),out);
}
OZ_C_proc_end


OZ_C_proc_begin(BIdictionaryMarkSafe,1)
{
  oz_declareDictionaryArg(0,dict);
  dict->markSafe();
  return PROCEED;
}
OZ_C_proc_end


OZ_C_proc_begin(BIdictionaryEntries,2)
{
  oz_declareDictionaryArg(0,dict);
  oz_declareArg(1,out);

  return oz_unify(dict->pairs(),out);
}
OZ_C_proc_end


OZ_C_proc_begin(BIdictionaryItems,2)
{
  oz_declareDictionaryArg(0,dict);
  oz_declareArg(1,out);

  return oz_unify(dict->items(),out);
}
OZ_C_proc_end


OZ_C_proc_begin(BIdictionaryClone,2)
{
  oz_declareDictionaryArg(0,dict);
  oz_declareArg(1,out);

  return oz_unify(dict->clone(am.currentBoard()),out);
}
OZ_C_proc_end


OZ_Return isDictionaryInline(TaggedRef t, TaggedRef &out)
{
  NONVAR( t, term);
  out = isDictionary(term) ? NameTrue : NameFalse;
  return PROCEED;
}
DECLAREBI_USEINLINEFUN1(BIisDictionary,isDictionaryInline)


OZ_Return dictionaryIsMtInline(TaggedRef d, TaggedRef &out) {
  NONVAR(d,dictaux);
  if (!isDictionary(dictaux)) { oz_typeError(0,"Dictionary"); }
  out = tagged2Dictionary(dictaux)->isEmpty() ? NameTrue : NameFalse;
  return PROCEED;
}
DECLAREBI_USEINLINEFUN1(BIdictionaryIsMt,dictionaryIsMtInline)


#define GetDictAndKey(d,k,dict,key,checkboard)                  \
  NONVAR(d,dictaux);                                            \
  NONVAR(k,key);                                                \
  if (!isDictionary(dictaux)) { oz_typeError(0,"Dictionary"); } \
  if (!isFeature(key))        { oz_typeError(1,"feature"); }    \
  OzDictionary *dict = tagged2Dictionary(dictaux);              \
  if (checkboard) { CheckLocalBoard(dict,"dict"); }


OZ_Return dictionaryMemberInline(TaggedRef d, TaggedRef k, TaggedRef &out)
{
  GetDictAndKey(d,k,dict,key,NO);
  out = dict->member(key);
  return PROCEED;
}
DECLAREBI_USEINLINEFUN2(BIdictionaryMember,dictionaryMemberInline)


OZ_Return dictionaryGetInline(TaggedRef d, TaggedRef k, TaggedRef &out)
{
  GetDictAndKey(d,k,dict,key,NO);
  if (dict->getArg(key,out) != PROCEED) {
    return oz_raise(E_SYSTEM,E_KERNEL,"dict",2,d,k);
  }
  return PROCEED;
}
DECLAREBI_USEINLINEFUN2(BIdictionaryGet,dictionaryGetInline)


OZ_Return dictionaryCondGetInline(TaggedRef d, TaggedRef k, TaggedRef deflt, TaggedRef &out)
{
  GetDictAndKey(d,k,dict,key,NO);
  if (dict->getArg(key,out) != PROCEED) {
    out = deflt;
  }
  return PROCEED;
}
DECLAREBI_USEINLINEFUN3(BIdictionaryCondGet,dictionaryCondGetInline)

OZ_Return dictionaryPutInline(TaggedRef d, TaggedRef k, TaggedRef value)
{
  GetDictAndKey(d,k,dict,key,OK);
  dict->setArg(key,value);
  return PROCEED;
}

DECLAREBI_USEINLINEREL3(BIdictionaryPut,dictionaryPutInline)


OZ_Return dictionaryCondPutInline(TaggedRef d, TaggedRef k, TaggedRef value)
{
  GetDictAndKey(d,k,dict,key,OK);
  dict->setCondArg(key,value);
  return PROCEED;
}

DECLAREBI_USEINLINEREL3(BIdictionaryCondPut,dictionaryCondPutInline)


OZ_C_proc_begin(BIdictionaryExchange,4) {
  TaggedRef d = OZ_getCArg(0);
  TaggedRef k = OZ_getCArg(1);

  GetDictAndKey(d,k,dict,key,OK);

  TaggedRef ov;

  dict->exchange(key, OZ_getCArg(3), &ov);

  if (ov == makeTaggedNULL()) {
    return oz_raise(E_SYSTEM,E_KERNEL,"dict",2,d,k);
  }
  return oz_unify(ov,OZ_getCArg(2));
} OZ_C_proc_end

OZ_C_proc_begin(BIdictionaryCondExchange,5) {
  TaggedRef d = OZ_getCArg(0);
  TaggedRef k = OZ_getCArg(1);

  GetDictAndKey(d,k,dict,key,OK);

  TaggedRef ov;

  dict->exchange(key, OZ_getCArg(3), &ov);

  return oz_unify(OZ_getCArg(2),
                  ((ov==makeTaggedNULL()) ? (OZ_getCArg(4)) : ov));
} OZ_C_proc_end


OZ_Return dictionaryRemoveInline(TaggedRef d, TaggedRef k)
{
  GetDictAndKey(d,k,dict,key,OK);
  dict->remove(key);
  return PROCEED;
}
DECLAREBI_USEINLINEREL2(BIdictionaryRemove,dictionaryRemoveInline)


OZ_C_proc_begin(BIdictionaryRemoveAll,1)
{
  oz_declareNonvarArg(0,dict);
  if (!isDictionary(dict)) {
    oz_typeError(0,"Dictionary");
  }

  tagged2Dictionary(dict)->removeAll();
  return PROCEED;
}
OZ_C_proc_end


/* -----------------------------------------------------------------
   Statistics
   ----------------------------------------------------------------- */


OZ_C_proc_begin(BIstatisticsReset, 0)
{
  ozstat.initCount();
  return PROCEED;
}
OZ_C_proc_end


OZ_C_proc_begin(BIstatisticsPrint, 0)
{
  ProfileCode(ozstat.printCount());
  return PROCEED;
}
OZ_C_proc_end

#ifdef PROFILE_INSTR
OZ_C_proc_begin(BIinstructionsPrint, 0)
{
  ozstat.printInstr();
  return PROCEED;
}
OZ_C_proc_end
#endif

#ifdef PROFILE_BI
OZ_C_proc_begin(BIbiPrint, 0)
{
  unsigned long sum = 0;
  for (HashNode *hn = builtinTab.getFirst(); hn; hn = builtinTab.getNext(hn)) {
    BuiltinTabEntry *abit = (BuiltinTabEntry *) hn->value;
    sum += abit->getCounter();
    if (abit->getCounter()!=0) {
      printf("%010lu x %s\n",abit->getCounter(),abit->getPrintName());
    }
  }
  printf("----------\n%010lu\n",sum);
  return PROCEED;
}
OZ_C_proc_end
#endif


OZ_C_proc_begin(BIstatisticsPrintProcs, 0)
{
  PrTabEntry::printPrTabEntries();
  return PROCEED;
}
OZ_C_proc_end

OZ_C_proc_begin(BIstatisticsGetProcs, 1)
{
  oz_declareArg(0,ret);
  return oz_unify(ret,PrTabEntry::getProfileStats());
}
OZ_C_proc_end

OZ_C_proc_begin(BIsetProfileMode, 1)
{
  oz_declareArg(0,onoff);
  am.setProfileMode(literalEq(deref(onoff),NameTrue));
  return PROCEED;
}
OZ_C_proc_end

/* -----------------------------------------------------------------
   dynamic link objects files
   ----------------------------------------------------------------- */


#ifdef _MSC_VER
#define F_OK 00
#endif

char *expandFileName(const char *fileName,char *path) {

  char *ret;

#ifdef WINDOWS
  if (isalpha(*fileName) && fileName[1]==':') {
#else
  if (*fileName == '/') {
#endif
    if (access(fileName,F_OK) == 0) {
      ret = new char[strlen(fileName)+1];
      strcpy(ret,fileName);
      return ret;
    }
    return NULL;
  }

  if (!path) {
    return NULL;
  }

  char *tmp = new char[strlen(path)+strlen(fileName)+2];
  char *last = tmp;
  while (1) {
    if (*path == PathSeparator || *path == 0) {

      if (last == tmp && *path == 0) { // path empty ??
        return NULL;
      }

      *last++ = '/';
      strcpy(last,fileName);
      if (access(tmp,F_OK) == 0) {
        ret = new char[strlen(tmp)+1];
        strcpy(ret,tmp);
        delete [] tmp;
        return ret;
      }
      last = tmp;
      if (*path == 0) {
        return NULL;
      }
      path++;
    } else {
      *last++ = *path++;
    }
  }

}


static void strCat(char toStr[], int &toStringSize, char *fromString)
{
  int i = toStringSize;
  while (*fromString)  {
    toStr[i++] = *fromString;
    fromString++;
  }
  toStr[i] = '\0';
  toStringSize = i;
}


/* arrayFromList(list,array,size):
 *       "list" is a oz list of atoms.
 *     "array" is an array containing those atoms of max. size "size"
 *     returns NULL on failure
 */

char **arrayFromList(OZ_Term list, char **array, int size)
{
  int i=0;

  while(OZ_isCons(list)) {

    if (i >= size-1) {
      goto bomb;
    }

    OZ_Term hh = head(deref(list));
    if (!OZ_isAtom(hh)) {
      goto bomb;
    }
    const char *fileName = OZ_atomToC(hh);

    char *f = expandFileName(fileName,ozconf.linkPath);

    if (!f) {
      goto bomb;
    }

    array[i++] = f;
    list = OZ_tail(list);
  }

  array[i] = NULL;
  return array;

 bomb:
  while (i>0) {
    i--;
    char *f = array[i];
    delete [] f;
  }
  return NULL;
}

/* linkObjectFiles(+filelist,-handle)
 *    filelist: list of atoms (== object files)
 *    handle:   for future calls of findFunction
 * we do not check for well-typedness, do it in oz !!
 */

OZ_C_proc_begin(BIlinkObjectFiles,2)
{
  OZ_Term list = OZ_getCArg(0);
  OZ_Term out = OZ_getCArg(1);

#ifdef DLOPEN
  const int commandSize = 1000;
  int commandUsed = 0;
  char command[commandSize];
  char *tempfile = tmpnam(NULL);
#ifdef SOLARIS
  strCat(command, commandUsed, "ld -G -lsocket -lnsl -lintl -o ");
#endif
#ifdef SUNOS_SPARC
  strCat(command, commandUsed, "ld -o ");
#endif
#ifdef IRIX
  strCat(command, commandUsed, "ld -shared -check_registry /dev/null -o ");
#endif
#ifdef OSF1_ALPHA
  strCat(command, commandUsed, "ld -shared -expect_unresolved \\* -o ");
#endif
#ifdef HPUX_700
  strCat(command, commandUsed, "ld -b -o ");
#endif
#ifdef LINUX
  strCat(command, commandUsed, "ld -shared -o ");
#endif
#if defined(FREEBSD) || defined(NETBSD)
  strCat(command, commandUsed, "ld -Bshareable -o ");
#endif
  strCat(command, commandUsed, tempfile);

  const int numOfiles = 100;
  char *ofiles[numOfiles];
  if (arrayFromList(list,ofiles,numOfiles) == NULL) {
    unlink(tempfile);
    goto raise;
  }

  {
    for (int i=0; ofiles[i] != NULL; i++) {
      char *f = ofiles[i];
      if (commandUsed + strlen(f) >= commandSize-1) {
        unlink(tempfile);
        delete [] f;
        goto raise;
      }
      strCat(command, commandUsed, " ");
      strCat(command, commandUsed, f);
      delete [] f;
    }
  }

  if (ozconf.showForeignLoad) {
    message("Linking files\n %s\n",command);
  }

  if (osSystem(command) < 0) {
    char buf[1000];
    sprintf(buf,"'%s' failed in linkObjectFiles",command);
    ozpwarning(buf);
    unlink(tempfile);
    goto raise;
  }

#ifdef HPUX_700
  shl_t handle;
  handle = shl_load(tempfile,
                    BIND_IMMEDIATE | BIND_NONFATAL | BIND_NOSTART | BIND_VERBOSE, 0L);

  if (handle == NULL) {
    goto raise;
  }
#else
  void *handle;

  if (!(handle = (void *)dlopen(tempfile, RTLD_NOW ))) {
    OZ_warning("dlopen failed in linkObjectFiles: %s",dlerror());
    unlink(tempfile);
    goto raise;
  }
#endif

  unlink(tempfile);
  return oz_unifyInt(out,ToInt32(handle));

#elif defined(WINDOWS)

  const numOfiles = 100;
  char *ofiles[numOfiles];
  if (arrayFromList(list,ofiles,numOfiles) == NULL ||
      ofiles[0]==NULL ||
      ofiles[1]!=NULL) {
    OZ_warning("linkObjectFiles(%s): can only accept one DLL\n",toC(list));
    goto raise;
  }

  if (ozconf.showForeignLoad) {
    message("Linking files\n %s\n",ofiles[0]);
  }

  {
    void *handle = (void *)LoadLibrary(ofiles[0]);
    if (handle==NULL) {
      OZ_warning("failed in linkObjectFiles: %d",GetLastError());
      goto raise;
    }

    FARPROC winLink(HMODULE handle, const char *name);
    FARPROC linkit = winLink(handle, "OZ_linkFF");
    if (linkit==0) {
      OZ_warning("OZ_linkFF not found, maybe not exported from DLL?");
      goto raise;
    }
    extern void **ffuns;
    if (linkit(OZVERSION,ffuns)==0) {
      OZ_warning("call of 'OZ_linkFF' failed, maybe recompilation needed?");
      goto raise;
    }
    return oz_unifyInt(out,ToInt32(handle));
  }
#endif
  return PROCEED;

raise:
  return oz_raise(E_ERROR,oz_atom("foreign"),"linkFiles",1,list);
}
OZ_C_proc_end



OZ_C_proc_begin(BIdlOpen,2)
{
  oz_declareVirtualStringArg(0,filename);
  oz_declareArg(1,out);

  OZ_Term err=NameUnit;
  OZ_Term ret=NameUnit;

  filename = expandFileName(filename,ozconf.linkPath);

  if (!filename) {
    err = oz_atom("expand filename failed");
    goto raise;
  }

  if (ozconf.showForeignLoad) {
    message("Linking file %s\n",filename);
  }

#ifdef DLOPEN
#ifdef HPUX_700
  {
    shl_t handle;
    handle = shl_load(filename,
                      BIND_IMMEDIATE | BIND_NONFATAL |
                      BIND_NOSTART | BIND_VERBOSE, 0L);

    if (handle == NULL) {
      goto raise;
    }
    ret = oz_int(ToInt32(handle));
  }
#else
  {
    void *handle=dlopen(filename, RTLD_NOW);

    if (!handle) {
      err=oz_atom(dlerror());
      goto raise;
    }
    ret = oz_int(ToInt32(handle));
  }
#endif

#elif defined(WINDOWS)
  {
    void *handle = (void *)LoadLibrary(filename);
    if (!handle) {
      err=oz_int(GetLastError());
      goto raise;
    }
    ret = oz_int(ToInt32(handle));
  }
#endif

  return oz_unify(out,ret);

raise:

  return oz_raise(E_ERROR,oz_atom("foreign"),"dlOpen",2,
                  OZ_getCArg(0),err);
}
OZ_C_proc_end

OZ_C_proc_begin(BIunlinkObjectFile,1)
{
  oz_declareAtomArg(0,fileName);

#ifdef WINDOWS
  FreeLibrary(GetModuleHandle(fileName));
#endif

  return PROCEED;
}
OZ_C_proc_end


OZ_C_proc_begin(BIdlClose,1)
{
  oz_declareIntArg(0,handle);

#ifdef DLOPEN
  if (dlclose((void *)handle)) {
    goto raise;
  }
#endif

#ifdef WINDOWS
  FreeLibrary((void *) handle);
#endif

  return PROCEED;

raise:
  return oz_raise(E_ERROR,oz_atom("foreign"),"dlClose",1,OZ_int(handle));
}
OZ_C_proc_end

#if defined(DLOPEN)
#define Link(handle,name) dlsym(handle,name)
#elif defined(HPUX_700)
void *ozdlsym(void *handle,char *name)
{
  void *symaddr;

  int symbol = shl_findsym((shl_t*)&handle, name, TYPE_PROCEDURE, &symaddr);
  if (symbol != 0) {
    return NULL;
  }

  return symaddr;
}

#define Link(handle,name) ozdlsym(handle,name)

#elif defined(WINDOWS)

FARPROC winLink(HMODULE handle, const char *name)
{
  FARPROC ret = GetProcAddress(handle,name);
  if (ret == NULL) {
    // try prepending a "_"
    char buf[1000];
    sprintf(buf,"_%s",name);
    ret = GetProcAddress(handle,buf);
  }
  return ret;
}

#define Link(handle,name) winLink(handle,name)

#endif

/* findFunction(+fname,+farity,+handle) */
OZ_C_proc_begin(BIfindFunction,3)
{
#if defined(DLOPEN) || defined(HPUX_700) || defined(WINDOWS)
  oz_declareVirtualStringArg(0,functionName);
  oz_declareIntArg(1,functionArity);
  oz_declareIntArg(2,handle);

  // get the function
  OZ_CFun func;
  if ((func = (OZ_CFun) Link((void *) handle,functionName)) == 0) {
#ifdef WINDOWS
    OZ_warning("error=%d\n",GetLastError());
#endif
    return oz_raise(E_ERROR, oz_atom("foreign"), "cannotFindFunction", 3,
                    OZ_getCArg(0),
                    OZ_getCArg(1),
                    OZ_getCArg(2));
  }

  OZ_addBuiltin(ozstrdup(functionName),functionArity,*func);
  return PROCEED;
#else
  not ported
#endif
}
OZ_C_proc_end

// {FindFile R Name ?Fullpath}
// R is tuple of symbols representing directories

#include <sys/types.h>
#include <sys/stat.h>

int find_file(OZ_Term R,char* Name,char* Fullpath)
{
#define NAMESIZE 256
  if (!OZ_isTuple(R)) return -1;
  int NameLen = strlen(Name);
  if (NameLen>=NAMESIZE) return -2;
  // Each element of the tuple must be a virtual string
  // and is interpreted as naming a directory
  int width = OZ_width(R);
  char buffer[NAMESIZE];
  for(int i=0;i<width;i++) {
    OZ_Term term = OZ_getArg(R,i);
    if (!OZ_isVirtualString(term,0)) return -3;
    char * Dir = OZ_virtualStringToC(term);
    int DirLen = strlen(Dir);
    // 1 is for possible additional / to insert
    if ((NameLen + DirLen + 1)>=NAMESIZE) return -2;
    memcpy(buffer,Dir,DirLen);
    if (buffer[DirLen-1]!='/') {
      buffer[DirLen] = '/';
      DirLen++;
    }
    memcpy(buffer+DirLen,Name,NameLen+1);
    struct stat buf;
    if (stat(buffer,&buf)==0) {
      strcpy(Fullpath,buffer);
      return 0;
    }
  }
  return 1;
}

OZ_C_proc_begin(BIFindFile,3)
{
  OZ_declareNonvarArg(0,R);
  OZ_declareVirtualStringArg(1,N);
  OZ_Term Fullpath = OZ_getCArg(2);
  char Name[NAMESIZE];
  char Path[NAMESIZE];
  strcpy(Name,N);
  if (Name[0]=='/') {           // absolute path
    struct stat buf;
    if (stat(Name,&buf)==0)
      return OZ_unify(Fullpath,OZ_atom(Name));
  }
  switch (find_file(R,Name,Path)) {
  case  1: return OZ_unify(Fullpath,OZ_false());
  case  0: return OZ_unify(Fullpath,OZ_atom(Path));
  case -1: return OZ_typeError(0,"Tuple");
  case -2: return OZ_raiseErrorC("FindFile",2,OZ_atom("bufferOverflow"),
                                 OZ_getCArg(1));
  case -3: return  OZ_raiseErrorC("FindFile",2,
                                  OZ_atom("nonVirtualStringIn"),
                                  R);
  default: Assert(0); return FAILED;
  }
}
OZ_C_proc_end

// {EnvToTuple "PATH" ?Tuple}
// looks up an environment variable that is supposed to be a list of
// pathnames separated by ':' (or ';' on Windows) and turns it into a
// tuple appropriate as an argument to FindFile.  If the environment
// variable does not exist, `false' is returned instead.

int env_to_tuple(char*var,OZ_Term* tup)
{
  char * S = getenv(var);
  if (S==NULL) return 1;
  // S is a list of pathnames separated by ':'
#ifndef NAMESIZE
#define NAMESIZE 256
#endif
  OZ_Term arg[NAMESIZE];
  int argidx = 0;
#define QPUSH(T)                                        \
  if (argidx<NAMESIZE) {                                \
    arg[argidx++] = T;                                  \
  } else {                                              \
    return -1;                                          \
  }
  char buffer[NAMESIZE];
  int index;
split:
  switch (*S) {
  case '\0': goto finish;
  case PathSeparator : QPUSH(OZ_nil()); S++; goto split;
  case ' ' :
  case '\t':
  case '\n': S++; goto split;
  default  :
    index = 0;
    buffer[index++] = *S++;
    goto splitx;
  }
splitx:
  switch (*S) {
  case '\0':
    buffer[index] = '\0';
    QPUSH(OZ_atom(buffer));
    goto finish;
  case PathSeparator :
    buffer[index] = '\0';
    QPUSH(OZ_atom(buffer));
    S++; goto split;
  default:
    buffer[index++] = *S++;
    goto splitx;
  }
finish:
  *tup = OZ_tuple(OZ_atom("env"),argidx);
  for(int i=0;i<argidx;i++)
    OZ_putArg(*tup,i,arg[i]);
  return 0;
}

OZ_C_proc_begin(BIEnvToTuple,2)
{
  OZ_declareVirtualStringArg(0,V);
  OZ_Term result = OZ_getCArg(1);
  OZ_Term tuple;
  switch (env_to_tuple(V,&tuple)) {
  case -1: return OZ_raiseErrorC("EnvToTuple",2,
                                 OZ_atom("tooManyPathnames"),
                                 OZ_getCArg(0));
  case  0: return OZ_unify(result,tuple);
  case  1: return OZ_unify(result,OZ_false());
  default: Assert(0); return FAILED;
  }
}
OZ_C_proc_end

/* ------------------------------------------------------------
 * Shutdown
 * ------------------------------------------------------------ */

OZ_C_proc_begin(BIshutdown,1)
{
  OZ_declareIntArg(0,status);
  am.exitOz(status);
  return(PROCEED); /* not reached but anyway */
}
OZ_C_proc_end

/* ------------------------------------------------------------
 * Alarm und Delay
 * ------------------------------------------------------------ */

OZ_C_proc_begin(BIalarm,2) {
  oz_declareIntArg(0,t);
  oz_declareArg(1,out);

  if (!am.onToplevel()) {
    return oz_raise(E_ERROR,E_KERNEL,"globalState",1,oz_atom("io"));
  }

  if (t <= 0)
    return oz_unify(NameUnit,out);

  am.insertUser(t,cons(NameUnit,out));
  return PROCEED;
}
OZ_C_proc_end


OZ_C_proc_begin(BIdelay,1) {
  oz_declareIntArg(0,t);

  if (!am.onToplevel()) {
    return oz_raise(E_ERROR,E_KERNEL,"globalState",1,oz_atom("io"));
  }

  if (t <= 0)
    return PROCEED;

  TaggedRef var = oz_newVariable();

  am.insertUser(t,cons(NameUnit,var));
  DEREF(var, var_ptr, var_tag);

  if (isAnyVar(var_tag)) {
    am.addSuspendVarList(var_ptr);
    OZ_args[0] = makeTaggedSmallInt(-1);
    return SUSPEND;
  }
  return PROCEED;
}
OZ_C_proc_end


/* ------------------------------------------------------------
 * Garbage Collection
 * ------------------------------------------------------------ */

OZ_C_proc_begin(BIgarbageCollection,0)
{
  am.setSFlag(StartGC);

  return BI_PREEMPT;
}
OZ_C_proc_end

/* ------------------------------------------------------------
 * System specials
 * ------------------------------------------------------------ */

OZ_C_proc_begin(BIsystemEq,3) {
  oz_declareArg(0,a);
  oz_declareArg(1,b);
  oz_declareArg(2,out);
  return oz_unify(out, oz_eq(a,b) ? NameTrue : NameFalse);
}
OZ_C_proc_end

/*
 * unify: used by compiler if '\sw -optimize'
 */
OZ_C_proc_begin(BIunify,2)
{
  oz_declareArg(0,a);
  oz_declareArg(1,b);

  return oz_unify(a,b);
}
OZ_C_proc_end

OZ_C_proc_begin(BIfail,0)
{
  return FAILED;
}
OZ_C_proc_end

OZ_C_proc_begin(BInop,0)
{
  return PROCEED;
}
OZ_C_proc_end

// ------------------------------------------------------------------------
// --- Apply
// ------------------------------------------------------------------------

OZ_C_proc_begin(BIapply,2)
{
  oz_declareNonvarArg(0,proc);
  oz_declareArg(1,args);

  OZ_Term var;
  if (!OZ_isList(args,&var)) {
    if (var == 0) oz_typeError(1,"finite List");
    oz_suspendOn(var);
  }

  int len = OZ_length(args);
  RefsArray argsArray = allocateY(len);
  for (int i=0; i < len; i++) {
    argsArray[i] = OZ_head(args);
    args=OZ_tail(args);
  }
  Assert(OZ_isNil(args));

  if (!isProcedure(proc) && !isObject(proc)) {
    oz_typeError(0,"Procedure or Object");
  }

  am.currentThread()->pushCall(proc,argsArray,len);
  deallocateY(argsArray);
  return BI_REPLACEBICALL;
}
OZ_C_proc_end

// ------------------------------------------------------------------------
// --- special Cell access
// ------------------------------------------------------------------------

OZ_C_proc_begin(BIdeepFeed,2)
{
  oz_declareNonvarArg(0,c);
  oz_declareArg(1,val);

  if (!isCell(c)) {
    oz_typeError(0,"Cell");
  }

  CellLocal *cell = (CellLocal*)tagged2Tert(c);

  Board *savedNode = am.currentBoard();
  Board *home1 = GETBOARD(cell);

  switch (am.installPath(home1)) {
  case INST_FAILED:
  case INST_REJECTED:
    error("deep: install");
  case INST_OK:
    break;
  }

  TaggedRef newVar = oz_newVariable();
  TaggedRef old = cell->exchangeValue(newVar);
  OZ_Return ret = oz_unify(old,cons(val,newVar));

  switch (am.installPath(savedNode)) {
  case INST_FAILED:
  case INST_REJECTED:
    error("deep: install back");
  case INST_OK:
    break;
  }

  return ret;
}
OZ_C_proc_end


/* ---------------------------------------------------------------------
 * Browser: special builtins: getsBound, intToAtom
 * --------------------------------------------------------------------- */

OZ_C_proc_begin(_getsBound_dummy, 0)
{
  return PROCEED;
}
OZ_C_proc_end


OZ_C_proc_begin(BIgetsBound, 1)
{
  oz_declareDerefArg(0,v);

  if (isAnyVar(vTag)){
    Thread *thr =
      (Thread *) OZ_makeSuspendedThread (_getsBound_dummy, NULL, 0);
    addSuspAnyVar(vPtr, thr);
  }

  return PROCEED;
}
OZ_C_proc_end


OZ_C_proc_begin(_getsBound_dummyB, 2)
{
  oz_declareArg(1,out);
  return oz_unify (out, NameTrue);
}
OZ_C_proc_end


OZ_C_proc_begin(BIgetsBoundB, 2)
{
  oz_declareDerefArg(0,v);

  if (isAnyVar(vTag)){
    Thread *thr =
      (Thread *) OZ_makeSuspendedThread (_getsBound_dummyB, OZ_args, OZ_arity);
    addSuspAnyVar(vPtr, thr);
  }

  return PROCEED;               // no result yet;
}
OZ_C_proc_end

/* ---------------------------------------------------------------------
 * ???
 * --------------------------------------------------------------------- */

OZ_C_proc_begin(BIconstraints,2)
{
  oz_declareDerefArg(0,in);
  oz_declareArg(1,out);

  int len = 0;
  if (isCVar(inTag)) {
    len=tagged2CVar(in)->getSuspListLength();
  } else if (isSVar(inTag)) {
    len = tagged2SVar(in)->getSuspList()->length();
  }
  return oz_unifyInt(out,len);
}
OZ_C_proc_end

// ---------------------------------------------------------------------------
// abstraction table
// ---------------------------------------------------------------------------

OZ_C_proc_begin(BIsetAbstractionTabDefaultEntry,1)
{
  oz_declareDerefArg(0,in);
  if (!isAbstraction(in)) {
    oz_typeError(0,"Abstraction");
  }

  warning("setAbstractionTabDefaultEntry no longer needed");
  return PROCEED;
}
OZ_C_proc_end

/* ---------------------------------------------------------------------
 * System
 * --------------------------------------------------------------------- */

/* print and show are inline,
   because it prevents the compiler from generating different code
   */
OZ_Return printInline(TaggedRef term)
{
  oz_printStream(term,cout,ozconf.printDepth,ozconf.printWidth);
  return PROCEED;
}

DECLAREBI_USEINLINEREL1(BIprint,printInline)

OZ_C_proc_begin(BIprintInfo,1)
{
  oz_declareArg(0,t);
  OZ_printVirtualString(t);
  fflush(stdout);
  return PROCEED;
}
OZ_C_proc_end

OZ_C_proc_begin(BIprintError,1)
{
  oz_declareArg(0,t);
  prefixError(); // print popup code for opi
  char * s = OZ_virtualStringToC(t);
  fprintf(stderr, "%s", s);
  fflush(stderr);
  return PROCEED;
}
OZ_C_proc_end

OZ_C_proc_begin(BItermToVS,4)
{
  oz_declareArg(0,t);
  oz_declareIntArg(1,depth);
  oz_declareIntArg(2,width);
  oz_declareArg(3,out);
  return oz_unify(out, OZ_string(OZ_toC(t,depth,width)));
}
OZ_C_proc_end

OZ_C_proc_begin(BIgetTermSize,4) {
  oz_declareArg(0,t);
  oz_declareIntArg(1,depth);
  oz_declareIntArg(2,width);
  oz_declareArg(3,out);

  return oz_unify(out, oz_int(OZ_termGetSize(t, depth, width)));
}
OZ_C_proc_end

OZ_Return showInline(TaggedRef term)
{
  printInline(term);
  printf("\n");
  return (PROCEED);
}

DECLAREBI_USEINLINEREL1(BIshow,showInline)

// ---------------------------------------------------------------------------
// ???
// ---------------------------------------------------------------------------

TaggedRef Abstraction::DBGgetGlobals() {
  int n = getGSize();
  OZ_Term t = OZ_tuple(oz_atom("globals"),n);
  for (int i = 0; i < n; i++) {
    OZ_putArg(t,i,gRegs[i]);
  }
  return t;
}

OZ_C_proc_begin(BIgetPrintName,2)
{
  oz_declareDerefArg(0,t);
  oz_declareArg(1,out);

  switch (tTag) {
  case OZCONST:
    if (isConst(t)) {
      ConstTerm *rec = tagged2Const(t);
      switch (rec->getType()) {
      case Co_Builtin:     return oz_unify(out, ((BuiltinTabEntry *) rec)->getName());
      case Co_Abstraction: return oz_unify(out, ((Abstraction *) rec)->getName());
      case Co_Object:      return oz_unifyAtom(out, tagged2Object(t)->getPrintName());
      case Co_Class:       return oz_unifyAtom(out, tagged2ObjectClass(t)->getPrintName());

      case Co_Cell:
      case Co_Dictionary:
      case Co_Array:
      default:             return oz_unifyAtom(out,"_");
      }
    }
    break;

  case UVAR:
  case SVAR:
  case CVAR:    return oz_unifyAtom(out, VariableNamer::getName(OZ_getCArg(0)));
  case LITERAL: {
    Literal *l = tagged2Literal(t);
    const char *s = l->getPrintName();
    if (s && *s) {
      return oz_unifyAtom(out, s);
    } else {
      static char buf[100];
      sprintf(buf,"::%d",l->hash());
      return oz_unifyAtom(out,buf);
    }
  }

  default:      break;
  }

  return oz_unifyAtom(out, "");
}
OZ_C_proc_end

//----------------------------------------------------------------------
//  System set and get
//----------------------------------------------------------------------

#define GetRecord \
  SRecord *r = tagged2SRecord(deref(OZ_getCArg(0)));

#define SetTaggedArg(a, t) \
  r->setFeature(a, t);

#define SetIntArg(a, n) \
  r->setFeature(a, makeInt(n));

#define SetBoolArg(a, n) \
  r->setFeature(a, (n) ? NameTrue : NameFalse);

OZ_C_proc_begin(BISystemGetThreads,1) {
  GetRecord;
  SetIntArg(AtomCreated,  ozstat.createdThreads.total);
  SetIntArg(AtomRunnable, am.getRunnableNumber());
  SetIntArg(AtomMin,      ozconf.stackMinSize / TASKFRAMESIZE);
  SetIntArg(AtomMax,      ozconf.stackMaxSize / TASKFRAMESIZE);
  return PROCEED;
}
OZ_C_proc_end

OZ_C_proc_begin(BISystemGetPriorities,1) {
  GetRecord;
  SetIntArg(AtomHigh,   ozconf.hiMidRatio);
  SetIntArg(AtomMedium, ozconf.midLowRatio);
  return PROCEED;
}
OZ_C_proc_end

OZ_C_proc_begin(BISystemGetTime,1) {
  GetRecord;
  unsigned int timeNow = osUserTime();

  unsigned int copy = 0;
  unsigned int gc   = 0;
  unsigned int load = 0;
  unsigned int prop = 0;
  unsigned int run  = 0;

  if (ozconf.timeDetailed) {
    copy = ozstat.timeForCopy.total;
    gc   = ozstat.timeForGC.total;
    load = ozstat.timeForLoading.total;
    prop = ozstat.timeForPropagation.total;
    run  = timeNow-(copy + gc + load + prop);
  }

  SetIntArg(AtomCopy,      copy);
  SetIntArg(AtomGC,        gc);
  SetIntArg(AtomLoad,      load);
  SetIntArg(AtomPropagate, prop);
  SetIntArg(AtomRun,       run);

  SetIntArg(AtomSystem,    osSystemTime());
  SetIntArg(AtomTotal,     osTotalTime());
  SetIntArg(AtomUser,      timeNow);
  SetBoolArg(AtomDetailed, ozconf.timeDetailed);

  return PROCEED;
}
OZ_C_proc_end

OZ_C_proc_begin(BISystemGetGC,1) {
  GetRecord;

  SetIntArg(AtomMin,       ozconf.heapMinSize*KB);
  SetIntArg(AtomMax,       ozconf.heapMaxSize*KB);
  SetIntArg(AtomFree,      ozconf.heapFree);
  SetIntArg(AtomTolerance, ozconf.heapTolerance);
  SetBoolArg(AtomOn,       ozconf.gcFlag);
  SetIntArg(AtomThreshold, ozconf.heapThreshold*KB);
  SetIntArg(AtomSize,      getUsedMemory()*KB);
  SetIntArg(AtomActive,    ozstat.gcLastActive*KB);

  return PROCEED;
}
OZ_C_proc_end

OZ_C_proc_begin(BISystemGetPrint,1) {
  GetRecord;

  SetIntArg(AtomDepth, ozconf.printDepth);
  SetIntArg(AtomWidth, ozconf.printWidth);

  return PROCEED;
}
OZ_C_proc_end

OZ_C_proc_begin(BISystemGetFD,1) {
  GetRecord;

  SetIntArg(AtomVariables,   ozstat.fdvarsCreated.total);
  SetIntArg(AtomPropagators, ozstat.propagatorsCreated.total);
  SetIntArg(AtomInvoked,     ozstat.propagatorsInvoked.total);
  SetIntArg(AtomThreshold,   32 * fd_bv_max_high);

  return PROCEED;
}
OZ_C_proc_end

OZ_C_proc_begin(BISystemGetSpaces,1) {
  GetRecord;

  SetIntArg(AtomCommitted, ozstat.solveAlt.total);
  SetIntArg(AtomCloned,    ozstat.solveCloned.total);
  SetIntArg(AtomCreated,   ozstat.solveCreated.total);
  SetIntArg(AtomFailed,    ozstat.solveFailed.total);
  SetIntArg(AtomSucceeded, ozstat.solveSolved.total);

  return PROCEED;
}
OZ_C_proc_end

OZ_C_proc_begin(BISystemGetErrors,1) {
  GetRecord;

  SetBoolArg(AtomLocation, ozconf.errorLocation);
  SetBoolArg(AtomDebug,    ozconf.errorDebug);
  SetBoolArg(AtomHints,    ozconf.errorHints);
  SetIntArg(AtomThread,    ozconf.errorThreadDepth);
  SetIntArg(AtomDepth,     ozconf.errorPrintDepth);
  SetIntArg(AtomWidth,     ozconf.errorPrintWidth);

  return PROCEED;
}
OZ_C_proc_end

OZ_C_proc_begin(BISystemGetMessages,1) {
  GetRecord;

  SetBoolArg(AtomGC,      ozconf.gcVerbosity);
  SetBoolArg(AtomIdle,    ozconf.showIdleMessage);
  SetBoolArg(AtomFeed,    ozconf.showFastLoad);
  SetBoolArg(AtomForeign, ozconf.showForeignLoad);
  SetBoolArg(AtomLoad,    ozconf.showLoad);
  SetBoolArg(AtomCache,   ozconf.showCacheLoad);

  return PROCEED;
}
OZ_C_proc_end

OZ_C_proc_begin(BISystemGetMemory,1) {
  GetRecord;

  SetIntArg(AtomAtoms,    ozstat.getAtomMemory());
  SetIntArg(AtomNames,    ozstat.getNameMemory());
  SetIntArg(AtomBuiltins, builtinTab.memRequired());
  SetIntArg(AtomFreelist, getMemoryInFreeList());
  SetIntArg(AtomCode,     CodeArea::totalSize);
  SetIntArg(AtomHeap,     ozstat.heapUsed.total+getUsedMemory());

  return PROCEED;
}
OZ_C_proc_end


OZ_C_proc_begin(BIheapUsed,1)
{
  oz_declareArg(0,out);
  return OZ_unifyInt(out,getUsedMemory());
}
OZ_C_proc_end


OZ_C_proc_begin(BISystemGetLimits,1) {
  GetRecord;

  SetTaggedArg(AtomInt, oz_pair2(makeInt(OzMinInt),
                                 makeInt(OzMaxInt)));

  return PROCEED;
}
OZ_C_proc_end

OZ_C_proc_begin(BISystemGetArgv,1) {
  TaggedRef out = nil();
  for(int i=ozconf.argC-1; i>=0; i--) {
    out = cons(oz_atom(ozconf.argV[i]),out);
  }
  return oz_unify(OZ_getCArg(0),out);
}
OZ_C_proc_end

OZ_C_proc_begin(BISystemGetStandalone,1) {
  return oz_unify(OZ_getCArg(0),ozconf.runningUnderEmacs? NameFalse: NameTrue);
}
OZ_C_proc_end

OZ_C_proc_begin(BISystemGetHome,1) {
  return oz_unifyAtom(OZ_getCArg(0),ozconf.ozHome);
}
OZ_C_proc_end

OZ_C_proc_begin(BISystemGetPlatform,1) {
  return oz_unify(OZ_getCArg(0),
                  oz_pair2(oz_atom(ozconf.osname),oz_atom(ozconf.cpu)));
}
OZ_C_proc_end

#undef GetRecord
#undef SetTaggedArg
#undef SetIntArg
#undef SetBoolArg


#define LookRecord(t)                           \
  OZ_Term t = OZ_getCArg(0);                    \
  DEREF(t, tPtr, tTag);                         \
  if (isAnyVar(tTag))                           \
    oz_suspendOnPtr(tPtr);                      \
  if (isLiteral(tTag))                          \
    return PROCEED;                             \
  if (!isSRecord(t))                            \
    oz_typeError(0, "Record");

#define DoPercentFeature(var,term,atom)                         \
  int     var = -1;                                             \
  { OZ_Term out = tagged2SRecord(term)->getFeature(atom);       \
    if (out) {                                                  \
      DEREF(out, outPtr, outTag);                               \
      if (isAnyVar(outTag)) oz_suspendOnPtr(outPtr);            \
      if (!isSmallInt(outTag) ||                                \
          (var=smallIntValue(out)) < 1 || var > 100)            \
         oz_typeError(0, "Int");                                \
    }                                                           \
  }

#define DoNatFeature(var,term,atom)                                     \
  int     var = -1;                                                     \
  { OZ_Term out = tagged2SRecord(term)->getFeature(atom);               \
    if (out) {                                                          \
      DEREF(out, outPtr, outTag);                                       \
      if (isAnyVar(outTag)) oz_suspendOnPtr(outPtr);                    \
      if (isSmallInt(outTag) && smallIntValue(out)>=0) {                \
        var = smallIntValue(out);                                       \
      } else if (isBigInt(outTag) && tagged2BigInt(out)->getInt()>=0) { \
        var = tagged2BigInt(out)->getInt();                             \
      } else {                                                          \
        oz_typeError(0, "Int");                                         \
      }                                                                 \
    }                                                                   \
  }

#define DoBoolFeature(var,term,atom)                            \
  int     var = -1;                                             \
  { OZ_Term out = tagged2SRecord(term)->getFeature(atom);       \
    if (out) {                                                  \
      DEREF(out, outPtr, outTag);                               \
      if (isAnyVar(outTag)) oz_suspendOnPtr(outPtr);            \
      if (isLiteral(outTag))                                    \
        if (literalEq(out,NameTrue)) {                          \
          var = 1;                                              \
        } else if (literalEq(out,NameFalse)) {                  \
          var = 0;                                              \
        } else {                                                \
          oz_typeError(0, "Bool");                              \
        }                                                       \
    }                                                           \
  }

#define SetIfPos(left,right,scale) if (right >= 0) left = right / scale;

OZ_C_proc_begin(BISystemSetTime,1) {
  LookRecord(t);

  DoBoolFeature(detailed, t, AtomDetailed);

  SetIfPos(ozconf.timeDetailed, detailed, 1);

  return PROCEED;
}
OZ_C_proc_end

OZ_C_proc_begin(BISystemSetThreads,1) {
  LookRecord(t);
  DoNatFeature(minsize, t, AtomMin);
  DoNatFeature(maxsize, t, AtomMax);

  SetIfPos(ozconf.stackMaxSize, maxsize, TASKFRAMESIZE);
  SetIfPos(ozconf.stackMinSize, minsize, TASKFRAMESIZE);

  if (ozconf.stackMinSize > ozconf.stackMaxSize)
    ozconf.stackMinSize = ozconf.stackMaxSize;

  return PROCEED;
} OZ_C_proc_end


OZ_C_proc_begin(BISystemSetPriorities,1) {
  LookRecord(t);

  DoPercentFeature(high,   t, AtomHigh);
  DoPercentFeature(medium, t, AtomMedium);

  SetIfPos(ozconf.hiMidRatio,  high,   1);
  SetIfPos(ozconf.midLowRatio, medium, 1);

  return PROCEED;
}
OZ_C_proc_end

OZ_C_proc_begin(BISystemSetGC,1) {
  LookRecord(t);

  DoNatFeature(max_size,      t, AtomMax);
  DoNatFeature(min_size,      t, AtomMin);
  DoPercentFeature(free,      t, AtomFree);
  DoPercentFeature(tolerance, t, AtomTolerance);
  DoBoolFeature(active,       t, AtomOn);

  SetIfPos(ozconf.heapMaxSize,    max_size,  KB);
  SetIfPos(ozconf.heapMinSize,    min_size,  KB);

  SetIfPos(ozconf.heapFree,       free,      1);
  SetIfPos(ozconf.heapTolerance,  tolerance, 1);
  SetIfPos(ozconf.gcFlag,         active,    1);

  if (ozconf.heapMinSize > ozconf.heapMaxSize)
    ozconf.heapMinSize = ozconf.heapMaxSize;

  if (ozconf.heapMinSize > ozconf.heapThreshold)
    ozconf.heapThreshold = ozconf.heapMinSize;

  if (ozconf.heapThreshold > ozconf.heapMaxSize) {
    am.setSFlag(StartGC);
    return BI_PREEMPT;
  }

  return PROCEED;
}
OZ_C_proc_end

OZ_C_proc_begin(BISystemSetPrint,1) {
  LookRecord(t);

  DoNatFeature(width, t, AtomWidth);
  DoNatFeature(depth, t, AtomDepth);

  SetIfPos(ozconf.printWidth, width, 1);
  SetIfPos(ozconf.printDepth, depth, 1);

  return PROCEED;
}
OZ_C_proc_end

OZ_C_proc_begin(BISystemSetFD,1) {
  LookRecord(t);

  DoNatFeature(threshold, t, AtomThreshold);

  reInitFDs(threshold);

  return PROCEED;
}
OZ_C_proc_end

OZ_C_proc_begin(BISystemSetErrors,1) {
  LookRecord(t);

  DoBoolFeature(location, t, AtomLocation);
  DoBoolFeature(hints,    t, AtomHints);
  DoBoolFeature(debug,    t, AtomDebug);
  DoNatFeature(thread,    t, AtomThread);
  DoNatFeature(width,     t, AtomWidth);
  DoNatFeature(depth,     t, AtomDepth);

  SetIfPos(ozconf.errorThreadDepth, thread,   1);
  SetIfPos(ozconf.errorLocation,    location, 1);
  SetIfPos(ozconf.errorDebug,       debug,    1);
  SetIfPos(ozconf.errorHints,       hints,    1);
  SetIfPos(ozconf.errorPrintWidth,  width,    1);
  SetIfPos(ozconf.errorPrintDepth,  depth,    1);

  return PROCEED;
}
OZ_C_proc_end

OZ_C_proc_begin(BISystemSetMessages,1) {
  LookRecord(t);

  DoBoolFeature(gc,      t, AtomGC);
  DoBoolFeature(idle,    t, AtomIdle);
  DoBoolFeature(feed,    t, AtomFeed);
  DoBoolFeature(foreign, t, AtomForeign);
  DoBoolFeature(load,    t, AtomLoad);
  DoBoolFeature(cache,   t, AtomCache);

  SetIfPos(ozconf.gcVerbosity,     gc,      1);
  SetIfPos(ozconf.showIdleMessage, idle,    1);
  SetIfPos(ozconf.showFastLoad,    feed,    1);
  SetIfPos(ozconf.showForeignLoad, foreign, 1);
  SetIfPos(ozconf.showLoad,        load,    1);
  SetIfPos(ozconf.showCacheLoad,   cache,   1);

  return PROCEED;
}
OZ_C_proc_end


OZ_C_proc_begin(BISystemSetInternal,1) {
  LookRecord(t);

  DoBoolFeature(debugmode,  t, AtomDebug);
  DoBoolFeature(suspension, t, AtomShowSuspension);
  DoBoolFeature(stop,       t, AtomStopOnToplevelFailure);
  DoNatFeature(debugIP,     t, AtomDebugIP);
  DoNatFeature(debugPerdio, t, AtomDebugPerdio);

  if (!debugmode) {
    am.unsetSFlag(DebugMode);
  } else if (debugmode) {
    am.setSFlag(DebugMode);
  }

  SetIfPos(ozconf.showSuspension,        suspension, 1);
  SetIfPos(ozconf.stopOnToplevelFailure, stop,       1);
  SetIfPos(ozconf.debugIP,               debugIP,    1);
  SetIfPos(ozconf.debugPerdio,           debugPerdio,1);

  return PROCEED;
}
OZ_C_proc_end


#undef LookRecord
#undef DoPercentFeature
#undef DoNatFeature
#undef DoBoolFeature
#undef SetIfPos


// ---------------------------------------------------------------------------

OZ_C_proc_begin(BIonToplevel,1)
{
  oz_declareArg(0,out);

  return oz_unify(out,OZ_onToplevel() ? NameTrue : NameFalse);
}
OZ_C_proc_end

OZ_C_proc_begin(BIaddr,2)
{
  oz_declareArg(0,val);
  oz_declareArg(1,out);

  DEREF(val,valPtr,valTag);
  if (valPtr) {
    return oz_unifyInt(out,ToInt32(valPtr));
  }
  return oz_unifyInt(out,ToInt32(tagValueOf2(valTag,val)));
}
OZ_C_proc_end

// ---------------------------------------------------------------------------

#ifdef UNUSED
OZ_C_proc_begin(BIindex2Tagged,2)
{
  OZ_Term in  = OZ_getCArg(0);
  OZ_Term out = OZ_getCArg(1);

  if (!OZ_isSmallInt(in)) {
    OZ_warning("Invalid index for builtin `index2Tagged'");
    return oz_unify(out, nil());
  }
  if (OZ_intToC(in) > am.toplevelVarsCount) {
    OZ_warning("Index too big for builtin `index2Tagged'");
    return oz_unify(out, nil());
  }
  return oz_unify(out, am.toplevelVars[OZ_intToC(in)]);
}
OZ_C_proc_end
#endif


#ifdef UNUSED
extern OZ_Term make_time(const struct tm*);  // defined in unix.cc

OZ_C_proc_begin(BItime2localTime,2)
{
  OZ_Term in  = OZ_getCArg(0);
  OZ_Term out = OZ_getCArg(1);

  if (!OZ_isInt(in)) {
    OZ_warning("Invalid first argument for builtin `time2localTime'");
    return oz_unify(out, nil());
  }
  else {
    time_t time = time_t(OZ_intToC(in));
    return oz_unify(out, make_time(localtime(&time)));
  }
}
OZ_C_proc_end
#endif

// ---------------------------------------------------------------------------


OZ_C_proc_begin(BIshowBuiltins,0)
{
  builtinTab.print();
  return(PROCEED);
}
OZ_C_proc_end

// ---------------------------------------------------------------------------

OZ_C_proc_begin(BIforeignFDProps, 1)
{
#ifdef FOREIGNFDPROPS
  return oz_unify(OZ_args[0], NameTrue);
#else
  return oz_unify(OZ_args[0], NameFalse);
#endif
}
OZ_C_proc_end

// ---------------------------------------------------------------------------

OZ_C_proc_proto(ozparser_parseFile)
OZ_C_proc_proto(ozparser_parseVirtualString)
OZ_C_proc_proto(ozparser_fileExists)


// ---------------------------------------------------------------------
// OO Stuff
// ---------------------------------------------------------------------

/*
 *      Construct a new SRecord to be a copy of old.
 *      This is the functionality of adjoin(old,newlabel).
 */
OZ_C_proc_begin(BIcopyRecord,2)
{
  oz_declareNonvarArg(0,rec);
  oz_declareArg(1,out);

  switch (recTag) {
  case SRECORD:
    {
      SRecord *rec0 = tagged2SRecord(rec);
      SRecord *rec1 = SRecord::newSRecord(rec0);
      return oz_unify(out,makeTaggedSRecord(rec1));
    }
  case LITERAL:
    return oz_unify(out,rec);

  default:
    oz_typeError(0,"Determined Record");
  }
}
OZ_C_proc_end


inline
SRecord *getStateInline(RecOrCell state, Bool isAssign, OZ_Term fea, OZ_Term &val)
{
  if (!stateIsCell(state)) {
    return getRecord(state);}

  TaggedRef old;
  Tertiary *t=getCell(state);          // shortcut
  if(t->getTertType()!=Te_Proxy){
    CellSec* sec;
    if(t->getTertType()==Te_Frame){
      sec=((CellFrame*)t)->getSec();}
    else{
      sec=((CellManager*)t)->getSec();}
    if(sec->getState()==Cell_Lock_Valid){
      old=sec->getContents();
      old=deref(old);
      if (!isAnyVar(old))
        return tagged2SRecord(old);}}

  old = oz_newVariable();
  if (am.onToplevel())
    cellDoExchange(t,old,old,am.currentThread());
  else
    cellDoAccess(t,old);
  if (!isAnyVar(deref(old)))
    return tagged2SRecord(deref(old));

  // OZ_suspendOnInternal(old);

  RefsArray x = allocateRefsArray(3,NO);
  x[0] = old;
  x[1] = fea;
  if (!isAssign)
    val = oz_newVariable();
  x[2] = val;
  am.currentThread()->pushCFun(isAssign?BIassignWithState:BIatWithState,x,3,NO);

  return NULL;
}

SRecord *getState(RecOrCell state, Bool isAssign, OZ_Term fea, OZ_Term &val)
{
  return getStateInline(state,isAssign,fea,val);
}


inline
OZ_Return doAt(SRecord *rec, TaggedRef fea, TaggedRef &out)
{
  Assert(rec!=NULL);

  DEREF(fea, _1, feaTag);
  if (!isFeature(fea)) {
    if (isAnyVar(fea)) {
      return SUSPEND;
    }
    goto bomb;
  }
  {
    TaggedRef t = rec->getFeature(fea);
    if (t) {
      out = t;
      return PROCEED;
    }
  }
bomb:
  oz_typeError(0,"(valid) Feature");
}

OZ_Return atInline(TaggedRef fea, TaggedRef &out)
{
  RecOrCell state = am.getSelf()->getState();
  SRecord *rec = getStateInline(state,NO,fea,out);
  if (rec==NULL) {
    return BI_REPLACEBICALL;
  }
  return doAt(rec,fea,out);
}
DECLAREBI_USEINLINEFUN1(BIat,atInline)

OZ_C_proc_begin(BIatWithState,3)
{
  oz_declareNonvarArg(0,state);
  OZ_Term fea = OZ_getCArg(1);
  OZ_Term out;

  OZ_Return ret = doAt(tagged2SRecord(deref(state)),fea,out);
  return (ret==PROCEED) ? oz_unify(OZ_getCArg(2),out) : ret;
}
OZ_C_proc_end


inline
OZ_Return doAssign(SRecord *r, TaggedRef fea, TaggedRef value)
{
  Assert(r!=NULL);

  DEREF(fea, _2, feaTag);
  if (!isFeature(fea)) {
    if (isAnyVar(fea)) {
      return SUSPEND;
    }
    oz_typeError(0,"Feature");
  }

  if (r->replaceFeature(fea,value) == makeTaggedNULL()) {
    return oz_raise(E_ERROR,E_OBJECT,"<-",3,makeTaggedSRecord(r),fea,value);
  }

  return PROCEED;
}

OZ_Return assignInline(TaggedRef fea, TaggedRef value)
{
  Object *self = am.getSelf();
  CheckLocalBoard(self,"object");

  RecOrCell state = self->getState();
  SRecord *rec = getStateInline(state,OK,fea,value);
  if (rec==NULL)
    return BI_REPLACEBICALL;
  return doAssign(rec,fea,value);
}

DECLAREBI_USEINLINEREL2(BIassign,assignInline)

OZ_C_proc_begin(BIassignWithState,3)
{
  oz_declareNonvarArg(0,state);
  OZ_Term fea = OZ_getCArg(1);
  OZ_Term val = OZ_getCArg(2);

  return doAssign(tagged2SRecord(deref(state)),fea,val);
}
OZ_C_proc_end

OZ_Return ooExchInline(TaggedRef fea, TaggedRef newAttr, TaggedRef &oldAttr)
{
  DEREF(fea, _1, feaTag);

  RecOrCell state = am.getSelf()->getState();

  if (stateIsCell(state)) {
    return oz_raise(E_ERROR,E_SYSTEM,"ooExchOnDistObject",
                    1,makeTaggedConst(am.getSelf()));
  }

  SRecord *rec = getRecord(state);
  Assert(rec!=NULL);
  if (!isFeature(feaTag)) {
    if (isAnyVar(feaTag)) {
      return SUSPEND;
    }
    goto bomb;
  }
  {
    TaggedRef aux = rec->getFeature(fea);
    if (aux) {
      oldAttr = aux;
      aux = rec->replaceFeature(fea,newAttr);
      Assert(aux!=makeTaggedNULL());
      return PROCEED;
    }
  }

bomb:
  oz_typeError(1,"(valid) Feature");
}
DECLAREBI_USEINLINEFUN2(BIooExch,ooExchInline)


int sizeOf(SRecord *sr)
{
  return sr ? sr->sizeOf() : 0;
}

Object *newObject(SRecord *feat, SRecord *st, ObjectClass *cla, Board *b)
{
  COUNT1(sizeObjects,sizeof(Object)+sizeOf(feat)+sizeOf(st));
  COUNT1(sizeRecords,-sizeOf(feat)-sizeOf(st));
  OzLock *lck=NULL;
  if (cla->supportsLocking()) {
    lck = new LockLocal(am.currentBoard());
    COUNT1(sizeObjects,sizeof(LockLocal));
  }
  return new Object(b,st,cla,feat,lck);
}


OZ_C_proc_begin(BImakeClass,6)
{
  OZ_Term fastmeth   = OZ_getCArg(0); { DEREF(fastmeth,_1,_2); }
  OZ_Term features   = OZ_getCArg(1); { DEREF(features,_1,_2); }
  OZ_Term ufeatures  = OZ_getCArg(2); { DEREF(ufeatures,_1,_2); }
  OZ_Term defmethods = OZ_getCArg(3); { DEREF(defmethods,_1,_2); }
  OZ_Term locking    = OZ_getCArg(4); { DEREF(locking,_1,_2); }
  OZ_Term out        = OZ_getCArg(5);

  if (!isDictionary(fastmeth))   { oz_typeError(0,"dictionary"); }
  if (!isRecord(features))       { oz_typeError(1,"record"); }
  if (!isRecord(ufeatures))      { oz_typeError(2,"record"); }
  if (!isDictionary(defmethods)) { oz_typeError(3,"dictionary"); }

  SRecord *uf = isSRecord(ufeatures) ? tagged2SRecord(ufeatures) : (SRecord*)NULL;

  ObjectClass *cl = new ObjectClass(tagged2SRecord(features),
                                    tagged2Dictionary(fastmeth),
                                    uf,
                                    tagged2Dictionary(defmethods),
                                    literalEq(locking,NameTrue),
                                    am.currentBoard());

  return oz_unify(out,makeTaggedConst(cl));
}
OZ_C_proc_end


OZ_C_proc_begin(BIcomma,2)
{
  oz_declareNonvarArg(0,cl);
  cl = deref(cl);

  if (!isClass(cl)) {
    oz_typeError(0,"Class");
  }

  ObjectClass * oc = tagged2ObjectClass(cl);

  TaggedRef fb = oc->getFallbackApply();

  Assert(fb);

  am.currentThread()->pushCall(fb,OZ_args,2);
  am.emptySuspendVarList();
  return BI_REPLACEBICALL;
}
OZ_C_proc_end

OZ_C_proc_begin(BIsend,3)
{
  oz_declareNonvarArg(1,cl);
  cl = deref(cl);

  if (!isClass(cl)) {
    oz_typeError(0,"Class");
  }

  ObjectClass * oc = tagged2ObjectClass(cl);

  TaggedRef fb = oc->getFallbackSend();

  Assert(fb);

  am.currentThread()->pushCall(fb,OZ_args,3);

  am.emptySuspendVarList();
  return BI_REPLACEBICALL;
}
OZ_C_proc_end

OZ_Return BIisObjectInline(TaggedRef t)
{
  DEREF(t,_1,_2);
  if (isAnyVar(t)) return SUSPEND;
  return isObject(t) ?  PROCEED : FAILED;
}

DECLAREBI_USEINLINEREL1(BIisObject,BIisObjectInline)
DECLAREBOOLFUN1(BIisObjectB,BIisObjectBInline,BIisObjectInline)


OZ_Return getClassInline(TaggedRef t, TaggedRef &out)
{
  DEREF(t,_,tag);
  if (isAnyVar(tag)) return SUSPEND;
  if (!isObject(t)) {
    oz_typeError(0,"Object");
  }
  out = makeTaggedConst(tagged2Object(t)->getClass());
  return PROCEED;
}

DECLAREBI_USEINLINEFUN1(BIgetClass,getClassInline)




inline
TaggedRef cloneObjectRecord(TaggedRef record, Bool cloneAll)
{
  if (isLiteral(record))
    return record;

  Assert(isSRecord(record));

  SRecord *in  = tagged2SRecord(record);
  SRecord *rec = SRecord::newSRecord(in);

  for(int i=0; i < in->getWidth(); i++) {
    OZ_Term arg = in->getArg(i);
    if (cloneAll || literalEq(NameOoFreeFlag,deref(arg))) {
      arg = oz_newVariable();
    }
    rec->setArg(i,arg);
  }

  return makeTaggedSRecord(rec);
}

static TaggedRef dummyRecord;

inline
OZ_Term makeObject(OZ_Term initState, OZ_Term ffeatures, ObjectClass *clas)
{
  Assert(isRecord(initState) && isRecord(ffeatures));

  /* state is _allways_ a record, this makes life somewhat easier */
  if (!isSRecord(initState)) {
    if (dummyRecord==makeTaggedNULL()) {
      SRecord *rec = SRecord::newSRecord(OZ_atom("noattributes"),
                                         aritytable.find(cons(OZ_newName(),nil())));
      rec->setArg(0,OZ_atom("novalue"));
      dummyRecord = makeTaggedSRecord(rec);
    }
    initState = dummyRecord;
  }

  Object *out =
    newObject(isSRecord(ffeatures) ? tagged2SRecord(ffeatures) : (SRecord*) NULL,
              tagged2SRecord(initState),
              clas,
              am.currentBoard());

  return makeTaggedConst(out);
}


OZ_Return newObjectInline(TaggedRef cla, TaggedRef &out)
{
  { DEREF(cla,_1,_2); }
  if (isAnyVar(cla)) return SUSPEND;
  if (!isClass(cla)) {
    oz_typeError(0,"Class");
  }

  ObjectClass *realclass = tagged2ObjectClass(cla);
  TaggedRef attr = realclass->classGetFeature(NameOoAttr);
  { DEREF(attr,_1,_2); }
  if (isAnyVar(attr)) return SUSPEND;

  TaggedRef attrclone = cloneObjectRecord(attr,NO);

  TaggedRef freefeat = realclass->classGetFeature(NameOoFreeFeatR);
  { DEREF(freefeat,_1,_2); }
  Assert(!isAnyVar(freefeat));
  TaggedRef freefeatclone = cloneObjectRecord(freefeat,OK);

  out = makeObject(attrclone, freefeatclone, realclass);

  return PROCEED;
}

DECLAREBI_USEINLINEFUN1(BInewObject,newObjectInline)


OZ_C_proc_begin(BINew,3)
{
  oz_declareNonvarArg(0,cl);
  cl = deref(cl);

  if (!isClass(cl)) {
    oz_typeError(0,"Class");
  }

  ObjectClass * oc = tagged2ObjectClass(cl);

  TaggedRef fb = oc->getFallbackNew();

  Assert(fb);

  am.currentThread()->pushCall(fb,OZ_args,3);
  am.emptySuspendVarList();
  return BI_REPLACEBICALL;
}
OZ_C_proc_end


OZ_C_proc_begin(BIsetSelf,1) {
  oz_declareNonvarArg(0,obj);
  obj = deref(obj);
  if (!isObject(obj)) {
    oz_typeError(0,"Object");
  }
  am.changeSelf(tagged2Object(obj));
  return PROCEED;
}
OZ_C_proc_end


OZ_Return ooGetLockInline(TaggedRef val)
{
  OzLock *lock = am.getSelf()->getLock();
  if (lock==NULL)
    return oz_raise(E_ERROR,E_OBJECT,"locking",1,
                    makeTaggedConst(am.getSelf()));

  return am.fastUnify(val,makeTaggedConst(lock),0) ? PROCEED : FAILED;
}
DECLAREBI_USEINLINEREL1(BIooGetLock,ooGetLockInline)


/********************************************************************
 * Exceptions
 ******************************************************************** */

OZ_C_proc_begin(BIsetDefaultExceptionHandler,1)
{
  oz_declareNonvarArg(0,hdl);
  if (!oz_isProcedure(hdl) || tagged2Const(hdl)->getArity()!=1) {
    oz_typeError(0,"Procedure/1");
  }

  am.setDefaultExceptionHdl(hdl);
  return PROCEED;
}
OZ_C_proc_end

OZ_C_proc_begin(BIgetDefaultExceptionHandler,1)
{
  OZ_declareArg(0,ret);
  OZ_Term hdl = am.getDefaultExceptionHdl();

  if (hdl==makeTaggedNULL()) {
    return oz_raise(E_ERROR,E_SYSTEM,"fallbackNotInstalled",1,
                    oz_atom("setDefaultExceptionHandler"));
  }

  return OZ_unify(ret,hdl);
}
OZ_C_proc_end

/*
 * the builtin exception handler
 */
OZ_C_proc_begin(BIbiExceptionHandler,1)
{
  OZ_Term arg=OZ_getCArg(0);

  errorHeader();
  message("UNCAUGHT EXCEPTION: %s\n",toC(arg));
  errorTrailer();

  return am.onToplevel() ? PROCEED : FAILED;
}
OZ_C_proc_end

OZ_C_proc_begin(BIraise,1)
{
  oz_declareArg(0,exc);

  return OZ_raise(exc);
}
OZ_C_proc_end

OZ_C_proc_begin(BIraiseError,1)
{
  oz_declareArg(0,exc);

  return OZ_raiseError(exc);
}
OZ_C_proc_end

OZ_C_proc_begin(BIraiseDebug,1)
{
  oz_declareArg(0,exc);

  return OZ_raiseDebug(exc);
}
OZ_C_proc_end


/********************************************************************
 * builtins for the new compiler
 * (OPI and environment handling)
 ******************************************************************** */

OZ_C_proc_begin(BIsetOPICompiler,1)
{
  oz_declareNonvarArg(0,obj);
  if (!isObject(obj)) {
    oz_typeError(0,"Object");
  } else if (am.getOpiCompiler()!=makeTaggedNULL()) {
    return oz_raise(E_ERROR,E_SYSTEM,"opiCompilerAlreadySet",1,
                    oz_atom("setOPICompiler"));
  } else {
    am.setOpiCompiler(obj);
    return PROCEED;
  }
}
OZ_C_proc_end

OZ_C_proc_begin(BIgetOPICompiler,1)
{
  OZ_declareArg(0,ret);
  OZ_Term obj = am.getOpiCompiler();

  if (obj==makeTaggedNULL())
    return OZ_unify(ret,NameFalse);
  else
    return OZ_unify(ret,obj);
}
OZ_C_proc_end

OZ_C_proc_begin(BIisBuiltin,2)
{
  oz_declareNonvarArg(0,val);
  oz_declareArg(1,res);

  if (isConst(val) && tagged2Const(val)->getType() == Co_Builtin)
    return oz_unify(res,NameTrue);
  else
    return oz_unify(res,NameFalse);
}
OZ_C_proc_end

OZ_C_proc_begin(BIgetBuiltinName,2)
{
  oz_declareNonvarArg(0,val);
  oz_declareArg(1,res);

  if (!isConst(val))
    oz_typeError(0,"builtin");
  ConstTerm *cnst = tagged2Const(val);
  if (cnst->getType() != Co_Builtin)
    oz_typeError(0,"builtin");
  return oz_unify(res,((BuiltinTabEntry *) cnst)->getName());
}
OZ_C_proc_end

OZ_C_proc_begin(BInameVariable,2)
{
  oz_declareArg(0,var);
  oz_declareAtomArg(1,name);
  VariableNamer::addName(var,name);
  return PROCEED;
}
OZ_C_proc_end

OZ_C_proc_begin(BInewNamedName,2)
{
  static int uniqueID = 1;
  oz_declareAtomArg(0,printName);
  char *s = new char[strlen(printName) + 12];
  sprintf(s,"%s %d",printName,uniqueID);
  uniqueID++;
  oz_declareArg(1,res);
  OZ_Return ret = oz_unify(makeTaggedLiteral(addToNameTab(s)),res);
  delete s;
  return ret;
}
OZ_C_proc_end

OZ_C_proc_begin(BIisUniqueName,2)
{
  oz_declareNonvarArg(0,val);
  oz_declareArg(1,res);

  if (isLiteral(val) && tagged2Literal(val)->isUniqueName())
    return oz_unify(res,NameTrue);
  else
    return oz_unify(res,NameFalse);
}
OZ_C_proc_end

OZ_C_proc_begin(BIgenerateAbstractionTableID,2)
{
  oz_declareNonvarArg(0,forComponent);
  oz_declareArg(1,res);
  AbstractionEntry *entry = new AbstractionEntry(OZ_isTrue(forComponent));
  return oz_unifyInt(res,ToInt32(entry));
}
OZ_C_proc_end

OZ_C_proc_begin(BIconcatenateAtomAndInt,3)
{
  // {ConcatenateAtomAndInts S I ?Res} computes:
  //    Res = {String.toAtom {Append {Atom.toString S} {Int.toString I}}}
  oz_declareAtomArg(0,s);
  oz_declareIntArg(1,i);
  oz_declareArg(2,res);
  char *news = new char[strlen(s) + 12];
  sprintf(news,"%s%d",s,i);
  OZ_Term newa = oz_atom(news);
  delete[] news;
  return oz_unify(res,newa);
}
OZ_C_proc_end

#define BITS_PER_INT (sizeof(int) * 8)

#define oz_declareHeapChunkArg(ARG,VAR1,VAR2)   \
int VAR1,*VAR2;                                 \
{                                               \
  oz_declareNonvarArg(ARG,_VAR);                \
  if (!OZ_isHeapChunk(_VAR)) {                  \
    oz_typeError(ARG,"HeapChunk");              \
  } else {                                      \
    VAR1 = OZ_getHeapChunkSize(_VAR);           \
    VAR2 = (int *) OZ_getHeapChunkData(_VAR);   \
  }                                             \
}

OZ_C_proc_begin(BIregSet_new,3)
{
  oz_declareIntArg(0,lower);
  oz_declareIntArg(1,upper);
  oz_declareArg(2,res);
  int count;
  if (lower > upper)
    count = 2;
  else
    count = (upper - lower) / BITS_PER_INT + 3;
  OZ_Term heapChunk = OZ_makeHeapChunk(count * sizeof(int));
  int *data = (int *) OZ_getHeapChunkData(heapChunk);
  data[0] = lower;
  data[1] = upper;
  for (int i = 2; i < count; i++) data[i] = 0;
  return oz_unify(res,heapChunk);
}
OZ_C_proc_end

OZ_C_proc_begin(BIregSet_copy,2)
{
  oz_declareHeapChunkArg(0,size,data);
  oz_declareArg(1,res);
  OZ_Term heapChunk = OZ_makeHeapChunk(size);
  int *newData = (int *) OZ_getHeapChunkData(heapChunk);
  int count = size / sizeof(int);
  for (int i = 0; i < count; i++) newData[i] = data[i];
  return oz_unify(res,heapChunk);
}
OZ_C_proc_end

OZ_C_proc_begin(BIregSet_adjoin,2)
{
  oz_declareHeapChunkArg(0,size,data);
  oz_declareIntArg(1,element);
  if (data[0] <= element && element <= data[1]) {
    int relative = element - data[0];
    data[relative / BITS_PER_INT + 2] |= 1 << (relative % BITS_PER_INT);
    return PROCEED;
  } else
    return OZ_raiseErrorC("regSet",4,oz_atom("elementOutOfBounds"),
                          oz_int(element),oz_int(data[0]),oz_int(data[1]));
}
OZ_C_proc_end

OZ_C_proc_begin(BIregSet_remove,2)
{
  oz_declareHeapChunkArg(0,size,data);
  oz_declareIntArg(1,element);
  if (data[0] <= element && element <= data[1]) {
    int relative = element - data[0];
    data[relative / BITS_PER_INT + 2] &= ~(1 << (relative % BITS_PER_INT));
    return PROCEED;
  } else
    return OZ_raiseErrorC("regSet",4,oz_atom("elementOutOfBounds"),
                          oz_int(element),oz_int(data[0]),oz_int(data[1]));
}
OZ_C_proc_end

OZ_C_proc_begin(BIregSet_member,3)
{
  oz_declareIntArg(0,element);
  oz_declareHeapChunkArg(1,size,data);
  oz_declareArg(2,res);
  if (data[0] <= element && element <= data[1]) {
    int relative = element - data[0];
    if (data[relative / BITS_PER_INT + 2] & 1 << (relative % BITS_PER_INT))
      return oz_unify(res,NameTrue);
    else
      return oz_unify(res,NameFalse);
  } else
    return oz_unify(res,NameFalse);
}
OZ_C_proc_end

OZ_C_proc_begin(BIregSet_union,2)
{
  oz_declareHeapChunkArg(0,size1,data1);
  oz_declareHeapChunkArg(1,size2,data2);
  if (data1[0] == data2[0] && data1[1] == data2[1]) {
    int count = size1 / sizeof(int);
    for (int i = 2; i < count; i++) data1[i] |= data2[i];
    return PROCEED;
  } else
    return OZ_raiseErrorC("regSet",5,oz_atom("boundsDoNotMatch"),
                          oz_int(data1[0]),oz_int(data1[1]),
                          oz_int(data2[0]),oz_int(data2[1]));
}
OZ_C_proc_end

OZ_C_proc_begin(BIregSet_intersect,2)
{
  oz_declareHeapChunkArg(0,size1,data1);
  oz_declareHeapChunkArg(1,size2,data2);
  if (data1[0] == data2[0] && data1[1] == data2[1]) {
    int count = size1 / sizeof(int);
    for (int i = 2; i < count; i++) data1[i] &= data2[i];
    return PROCEED;
  } else
    return OZ_raiseErrorC("regSet",5,oz_atom("boundsDoNotMatch"),
                          oz_int(data1[0]),oz_int(data1[1]),
                          oz_int(data2[0]),oz_int(data2[1]));
}
OZ_C_proc_end

OZ_C_proc_begin(BIregSet_subtract,2)
{
  oz_declareHeapChunkArg(0,size1,data1);
  oz_declareHeapChunkArg(1,size2,data2);
  if (data1[0] == data2[0] && data1[1] == data2[1]) {
    int count = size1 / sizeof(int);
    for (int i = 2; i < count; i++) data1[i] &= ~data2[i];
    return PROCEED;
  } else
    return OZ_raiseErrorC("regSet",5,oz_atom("boundsDoNotMatch"),
                          oz_int(data1[0]),oz_int(data1[1]),
                          oz_int(data2[0]),oz_int(data2[1]));
}
OZ_C_proc_end

OZ_C_proc_begin(BIregSet_toList,2)
{
  oz_declareHeapChunkArg(0,size,data);
  oz_declareArg(1,res);

  OZ_Term list = AtomNil;
  int i, j, word;
  int offset = ((data[1] - data[0]) / BITS_PER_INT) * BITS_PER_INT + data[0];
  for (i = size / sizeof(int) - 1; i >= 2; i--) {
    word = data[i];
    for (j = BITS_PER_INT - 1; j >= 0; j--)
      if (word & (1 << j))
        list = OZ_cons(OZ_int(offset + j),list);
    offset -= BITS_PER_INT;
  }
  return oz_unify(res,list);
}
OZ_C_proc_end

OZ_C_proc_begin(BIregSet_complementToList,2)
{
  oz_declareHeapChunkArg(0,size,data);
  oz_declareArg(1,res);

  OZ_Term list = AtomNil;
  int i, j, word;
  int offset = ((data[1] - data[0]) / BITS_PER_INT) * BITS_PER_INT + data[0];
  for (i = size / sizeof(int) - 1; i >= 2; i--) {
    word = data[i];
    for (j = BITS_PER_INT - 1; j >= 0; j--)
      if (!(word & (1 << j)))
        list = OZ_cons(OZ_int(offset + j),list);
    offset -= BITS_PER_INT;
  }
  return oz_unify(res,list);
}
OZ_C_proc_end

/********************************************************************
 * Finalization
 ******************************************************************** */
#ifdef FINALIZATION

// Copyright © by Denys Duchier, Nov 1997, Universitšt des Saarlandes
//
// {Finalize.register OBJECT HANDLER}
//
//      When OBJECT becomes inaccessible {HANDLER OBJECT} is
//      eventually executed.  Both data must be at the top-level.
//      In practice, (OBJECT|HANDLER) is simply entered into the
//      `GUARDIAN' list.  Only OZCONST values can be registered.
//      Finalization makes no sense for replicable data.
//
// !!! For the moment I am ignoring distributed stuff
//
// {Finalize.setHandler HANDLER}
//
//      HANDLER is a procedure of 1 argument which is a list
//      of conses (OBJECT|HANDLER).  Its mandate is to execute
//      each {HANDLER OBJECT} safely (i.e. catching all exceptions)
//
// WHAT HAPPENS DURING GC?
//
// The GUARDIAN list is a list of conses (OBJECT|HANDLER).  During
// GC, after all other data has been collected (i.e. towards the end)
// this list is specially gc'ed as follows:
//
// *    each cons where OBJECT has already been copied (i.e. is
//      otherwise reachable through live data) is entered into the
//      guardian list in the new half-space.
//
// *    each cons where OBJECT has not already been copied (i.e.
//      is not otherwise reachable through live data) is entered
//      into the FINALIZE list.
//
// Both the new GUARDIAN and the FINALIZE lists are then processed
// to normally gc the element conses (i.e. OBJECT|HANDLER).
//
// If the FINALIZE list is not empty, a top-level thread is created
// to evaluate {FINALIZE_HANDLER FINALIZE_LIST} where FINALIZE_HANDLER
// is the handler specified by Finalize.setHandler and FINALIZE_LIST
// is the FINALIZE list.

OZ_Term guardian_list   = 0;
OZ_Term finalize_list   = 0;
OZ_Term finalize_handler= 0;

// returns 0 if non determined
//         1 if determined and (literal or top-level)
//         2 otherwise
static int finalizable(OZ_Term& x)
{
  DEREF(x,xPtr,xTag);

  switch (xTag) {
  case UVAR:
  case SVAR:
  case CVAR:
    return 0;
    //  case SMALLINT:
    //  case FSETVALUE:
    //  case LITERAL:
    //  case BIGINT:
    //  case OZFLOAT:
    //    return 1;
  case OZCONST:
    {
      ConstTerm* xp = tagged2Const(x);
      Board*b;
      switch (xp->getType()) {
#ifdef FOREIGN_POINTER
      case Co_Foreign_Pointer:
        return 1;
#else
      case Co_UNUSED1:
#endif
      case Co_UNUSED2:
        return 2;
        // Tertiary Consts
      case Co_Thread:
        b = ((Thread*)xp)->getBoardInternal(); break;
      case Co_Abstraction:
        b = ((Abstraction*)xp)->getBoardInternal(); break;
      case Co_Builtin:
        return 1;
      case Co_Cell:
        b = ((Tertiary*)xp)->getBoardInternal(); break;
      case Co_Space:
        b = ((Space*)xp)->getBoardInternal(); break;
      case Co_Object:
        b = ((Object*)xp)->getBoardInternal(); break;
      case Co_Port:
        b = ((Port*)xp)->getBoardInternal(); break;
      case Co_Chunk:
        b = ((SChunk*)xp)->getBoardInternal(); break;
      case Co_HeapChunk:
        return 1;
      case Co_Array:
        b = ((OzArray*)xp)->getBoardInternal(); break;
      case Co_Dictionary:
        b = ((OzDictionary*)xp)->getBoardInternal(); break;
      case Dummy:
        return 2;
      case Co_Lock:
        b = ((Tertiary*)xp)->getBoardInternal(); break;
      case Co_Class:
        b = ((ObjectClass*)xp)->getBoardInternal(); break;
      }
      return am.isRootBoard(b)?1:2;
    }
  default:
    return 2;
  }
}

// Builtin {Finalize.register OBJECT HANDLER}

OZ_C_proc_begin(BIfinalize_register,2)
{
  OZ_Term obj = OZ_getCArg(0);
  OZ_Term hdl = OZ_getCArg(1);
  switch (finalizable(obj)) {
  case 0: oz_suspendOn(obj);
  case 1: break;
  case 2:
    return oz_raise(E_ERROR,E_KERNEL,"nonGlobal",3,
                    oz_atom("Finalize.register"),
                    oz_atom("object"),obj);
  }
  switch (finalizable(hdl)) {
  case 0: oz_suspendOn(hdl);
  case 1: break;
  case 2:
    return oz_raise(E_ERROR,E_KERNEL,"nonGlobal",3,
                    oz_atom("Finalize.register"),
                    oz_atom("handler"),hdl);
  }
  if (guardian_list==0) guardian_list=oz_nil();
  guardian_list = oz_cons(oz_cons(obj,hdl),guardian_list);
  return PROCEED;
}
OZ_C_proc_end

OZ_C_proc_begin(BIfinalize_setHandler,1)
{
  OZ_Term hdl = OZ_getCArg(0);
  switch (finalizable(hdl)) {
  case 0: oz_suspendOn(hdl);
  case 1: break;
  case 2: return oz_raise(E_ERROR,E_KERNEL,"nonGlobal",2,
                          oz_atom("Finalize.setHandler"),
                          hdl);
  }
  if (!(isProcedure(hdl)||isObject(hdl)))
    oz_typeError(0,"Procedure|Object");
  finalize_handler = hdl;
  return PROCEED;
}
OZ_C_proc_end

#endif

/********************************************************************
 * Service Registry
 ******************************************************************** */

// Copyright © by Denys Duchier, Jan 1998, Universitšt des Saarlandes
//
// The service registry is for plugins that provide (improved)
// services.  For example, search path and cache mechanisms for
// loading etc...  In particular, the emulator itself can take advantage
// of services implemented in Oz.
//
// see value.hh for:
//      service_get(OZ_Term)
//      service_get(char*)
//      service_put(OZ_Term,OZ_Term)
//      service_put(char*s,OZ_Term)

OZ_Term service_registry;

OZ_C_proc_begin(BIservice_registry,1)
{
  return oz_unify(service_registry,OZ_getCArg(0));
}
OZ_C_proc_end

/********************************************************************
 * Copy Code
 ******************************************************************** */


/* only for testing: */

OZ_C_proc_begin(BIcopyCode,2)
{
  oz_declareNonvarArg(0,proc);
  oz_declareNonvarArg(1,dict);

  if (!isAbstraction(proc)) { oz_typeError(0,"Abstraction"); }
  if (!isDictionary(dict))  { oz_typeError(1,"Dictionary"); }

  (void) copyCode(tagged2Abstraction(proc)->getPC(), tagged2Dictionary(dict),OK);

  return PROCEED;
}
OZ_C_proc_end


/********************************************************************
 * Table of builtins
 ******************************************************************** */

OZ_C_proc_proto(BIdebugPrint);
OZ_C_proc_proto(BIdebugPrintLong);


BIspec allSpec[] = {
  {"/",   3, BIfdiv,     (IFOR) BIfdivInline},
  {"*",   3, BImult,     (IFOR) BImultInline},
  {"div", 3, BIdiv,      (IFOR) BIdivInline},
  {"mod", 3, BImod,      (IFOR) BImodInline},
  {"-",   3, BIminus,    (IFOR) BIminusInline},
  {"+",   3, BIplus,     (IFOR) BIplusInline},
  {"Max", 3, BImax,      (IFOR) BImaxInline},
  {"Min", 3, BImin,      (IFOR) BIminInline},

  {"<",    3, BIlessFun,   (IFOR) BIlessInlineFun},
  {"=<",   3, BIleFun,     (IFOR) BIleInlineFun},
  {">",    3, BIgreatFun,  (IFOR) BIgreatInlineFun},
  {">=",   3, BIgeFun,     (IFOR) BIgeInlineFun},

  {"=<Rel",   2, BIle,     (IFOR) BIleInline},
  {"<Rel",    2, BIless,   (IFOR) BIlessInline},
  {">=Rel",   2, BIge,     (IFOR) BIgeInline},
  {">Rel",    2, BIgreat,  (IFOR) BIgreatInline},

  {"~",  2, BIuminus,   (IFOR) BIuminusInline},
  {"+1", 2, BIadd1,     (IFOR) BIadd1Inline},
  {"-1", 2, BIsub1,     (IFOR) BIsub1Inline},

  {"Exp",   2, BIexp,   (IFOR) BIinlineExp},
  {"Log",   2, BIlog,   (IFOR) BIinlineLog},
  {"Sqrt",  2, BIsqrt,  (IFOR) BIinlineSqrt},
  {"Sin",   2, BIsin,   (IFOR) BIinlineSin},
  {"Asin",  2, BIasin,  (IFOR) BIinlineAsin},
  {"Cos",   2, BIcos,   (IFOR) BIinlineCos},
  {"Acos",  2, BIacos,  (IFOR) BIinlineAcos},
  {"Tan",   2, BItan,   (IFOR) BIinlineTan},
  {"Atan",  2, BIatan,  (IFOR) BIinlineAtan},
  {"Ceil",  2, BIceil,  (IFOR) BIinlineCeil},
  {"Floor", 2, BIfloor, (IFOR) BIinlineFloor},
  {"Abs",   2, BIabs,   (IFOR) BIabsInline},
  {"Round", 2, BIround, (IFOR) BIinlineRound},
  {"Atan2", 3, BIatan2, (IFOR) BIatan2Inline},

  {"fPow",  3, BIfPow, (IFOR) BIfPowInline},

  /* conversion: float <-> int <-> virtualStrings */
  {"IntToFloat", 2, BIintToFloat,  (IFOR) BIintToFloatInline},
  {"FloatToInt", 2, BIfloatToInt,  (IFOR) BIfloatToIntInline},

  {"IntToString",    2, BIintToString,          0},
  {"FloatToString",  2, BIfloatToString,        0},
  {"StringToInt",    2, BIstringToInt,          0},
  {"StringToFloat",  2, BIstringToFloat,        0},
  {"String.isInt",   2, BIstringIsInt,          0},
  {"String.isFloat", 2, BIstringIsFloat,        0},
  {"String.isAtom",  2, BIstringIsAtom,         0},

  {"IsArray",    2, BIisArray,   (IFOR) isArrayInline},
  {"NewArray",   4, BIarrayNew,  0},
  {"Array.high", 2, BIarrayHigh, (IFOR) arrayHighInline},
  {"Array.low",  2, BIarrayLow,  (IFOR) arrayLowInline},
  {"Get",        3, BIarrayGet,  (IFOR) arrayGetInline},
  {"Put",        3, BIarrayPut,  (IFOR) arrayPutInline},

  {"NewDictionary",        1, BIdictionaryNew,  0},
  {"IsDictionary",         2, BIisDictionary,
                              (IFOR) isDictionaryInline},
  {"Dictionary.isEmpty",   2, BIdictionaryIsMt,
                              (IFOR) dictionaryIsMtInline},
  {"Dictionary.get",       3, BIdictionaryGet,
                              (IFOR) dictionaryGetInline},
  {"Dictionary.condGet",   4, BIdictionaryCondGet,
                              (IFOR) dictionaryCondGetInline},
  {"Dictionary.put",       3, BIdictionaryPut,
                              (IFOR) dictionaryPutInline},
  {"Dictionary.condPut",   3, BIdictionaryCondPut,
                              (IFOR) dictionaryCondPutInline},
  {"Dictionary.exchange",    4, BIdictionaryExchange, 0},
  {"Dictionary.condExchange", 5, BIdictionaryCondExchange,   0},
  {"Dictionary.remove",    2, BIdictionaryRemove,
                              (IFOR) dictionaryRemoveInline},
  {"Dictionary.removeAll", 1, BIdictionaryRemoveAll, 0},
  {"Dictionary.member",    3, BIdictionaryMember,
                              (IFOR) dictionaryMemberInline},
  {"Dictionary.keys",      2, BIdictionaryKeys,     0},
  {"Dictionary.entries",   2, BIdictionaryEntries,  0},
  {"Dictionary.items",     2, BIdictionaryItems,    0},
  {"Dictionary.clone",     2, BIdictionaryClone,    0},
  {"Dictionary.markSafe",  1, BIdictionaryMarkSafe, 0},

  {"NewLock",         1,BInewLock,       0},
  {"Lock",            1,BIlockLock,      0},
  {"Unlock",          1,BIunlockLock,    0},

  {"NewPort",         2,BInewPort,       0},
  {"Send",            2,BIsendPort,      0},

  {"NewCell",         2,BInewCell,       0},
  {"Exchange",        3,BIexchangeCell, (IFOR) BIexchangeCellInline},
  {"Access",          2,BIaccessCell,   (IFOR) BIaccessCellInline},
  {"Assign",          2,BIassignCell,   (IFOR) BIassignCellInline},

 {"perdioRestop",   1, BIrestop,          0},
 {"crash",          0, BIcrash,           0},
 {"InstallHandler", 3, BIhandlerInstall,  0},
 {"InstallWatcher", 3, BIwatcherInstall,  0},

  {"IsChar",        2, BIcharIs,        0},
  {"Char.isAlNum",  2, BIcharIsAlNum,   0},
  {"Char.isAlpha",  2, BIcharIsAlpha,   0},
  {"Char.isCntrl",  2, BIcharIsCntrl,   0},
  {"Char.isDigit",  2, BIcharIsDigit,   0},
  {"Char.isGraph",  2, BIcharIsGraph,   0},
  {"Char.isLower",  2, BIcharIsLower,   0},
  {"Char.isPrint",  2, BIcharIsPrint,   0},
  {"Char.isPunct",  2, BIcharIsPunct,   0},
  {"Char.isSpace",  2, BIcharIsSpace,   0},
  {"Char.isUpper",  2, BIcharIsUpper,   0},
  {"Char.isXDigit", 2, BIcharIsXDigit,  0},
  {"Char.toLower",  2, BIcharToLower,   0},
  {"Char.toUpper",  2, BIcharToUpper,   0},
  {"Char.toAtom",   2, BIcharToAtom,    0},
  {"Char.type",     2, BIcharType,      0},

  {"Adjoin",          3, BIadjoin,           (IFOR) BIadjoinInline},
  {"AdjoinList",      3, BIadjoinList,       0},
  {"record",          3, BImakeRecord,       0},
  {"Arity",           2, BIarity,            (IFOR) BIarityInline},
  {"AdjoinAt",        4, BIadjoinAt,         0},

  {"IsNumber",        2, BIisNumberB,    (IFOR) BIisNumberBInline},
  {"IsInt"   ,        2, BIisIntB,       (IFOR) BIisIntBInline},
  {"IsFloat" ,        2, BIisFloatB,     (IFOR) BIisFloatBInline},
  {"IsRecord",        2, BIisRecordB,    (IFOR) isRecordBInline},
  {"IsTuple",         2, BIisTupleB,     (IFOR) isTupleBInline},
  {"IsLiteral",       2, BIisLiteralB,   (IFOR) isLiteralBInline},
  {"IsLock",          2, BIisLockB,      (IFOR) isLockBInline},
  {"IsCell",          2, BIisCellB,      (IFOR) isCellBInline},
  {"IsPort",          2, BIisPortB,      (IFOR) isPortBInline},
  {"IsProcedure",     2, BIisProcedureB, (IFOR) isProcedureBInline},
  {"IsName",          2, BIisNameB,      (IFOR) isNameBInline},
  {"IsAtom",          2, BIisAtomB,      (IFOR) isAtomBInline},
  {"IsBool",          2, BIisBoolB,      (IFOR) isBoolBInline},
  {"IsUnit",          2, BIisUnitB,      (IFOR) isUnitBInline},
  {"IsChunk",         2, BIisChunkB,     (IFOR) isChunkBInline},
  {"IsRecordC",       2, BIisRecordCB,   (IFOR) isRecordCBInline},
  {"IsObject",        2, BIisObjectB,    (IFOR) BIisObjectBInline},
  {"IsString",        2, BIisString,     0},
  {"IsVirtualString", 2, BIvsIs,         0},
  {"IsFree",          2, BIisFree,       (IFOR) isFreeInline},
  {"IsKinded",        2, BIisKinded,     (IFOR) isKindedInline},
  {"IsDet",           2, BIisDet,        (IFOR) isDetInline},

  {"isNumberRel",    1, BIisNumber,    (IFOR) BIisNumberInline},
  {"isIntRel"   ,    1, BIisInt,       (IFOR) BIisIntInline},
  {"isFloatRel" ,    1, BIisFloat,     (IFOR) BIisFloatInline},
  {"isRecordRel",    1, BIisRecord,    (IFOR) isRecordInline},
  {"isTupleRel",     1, BIisTuple,     (IFOR) isTupleInline},
  {"isLiteralRel",   1, BIisLiteral,   (IFOR) isLiteralInline},
  {"isCellRel",      1, BIisCell,      (IFOR) isCellInline},
  {"isPortRel",      1, BIisPort,      (IFOR) isPortInline},
  {"isProcedureRel", 1, BIisProcedure, (IFOR) isProcedureInline},
  {"isNameRel",      1, BIisName,      (IFOR) isNameInline},
  {"isAtomRel",      1, BIisAtom,      (IFOR) isAtomInline},
  {"isLockRel",      1, BIisLock,      (IFOR) isLockInline},
  {"isBoolRel",      1, BIisBool,      (IFOR) isBoolInline},
  {"isUnitRel",      1, BIisUnit,      (IFOR) isUnitInline},
  {"isChunkRel",     1, BIisChunk,     (IFOR) isChunkInline},
  {"isRecordCRel",   1, BIisRecordC,   (IFOR) isRecordCInline},
  {"isObjectRel",    1, BIisObject,    (IFOR) BIisObjectInline},
  {"IsFreeRel",      1, BIisFreeRel,   (IFOR) isFreeRelInline},
  {"IsKindedRel",    1, BIisKindedRel, (IFOR) isKindedRelInline},
  {"IsDetRel",       1, BIisDetRel,    (IFOR) isDetRelInline},

  {"Wait",   1, BIisValue,            (IFOR) isValueInline},
  {"WaitOr", 2, BIwaitOr,             (IFOR) 0},

  {"virtualStringLength",  3, BIvsLength, 0},
  {"Length",  2, BIlength, 0},

  {"Not",     2,BInot,             (IFOR) notInline},
  {"And",     3,BIand,             (IFOR) andInline},
  {"Or",      3,BIor,              (IFOR) orInline},

  {"Type.ofValue", 2, BItermType,        (IFOR) BItermTypeInline},
  {"Value.status", 2, BIstatus,          (IFOR) BIstatusInline},

  {"procedureEnvironment",2,BIprocedureEnvironment,0},
  {"getProcInfo",         2,BIgetProcInfo,         0},
  {"setProcInfo",         2,BIsetProcInfo,         0},
  {"getProcNames",        2,BIgetProcNames,        0},
  {"setProcNames",        2,BIsetProcNames,        0},
  {"getProcPos",          3,BIgetProcPos,          0},
  {"MakeTuple",           3,BItuple,               (IFOR) tupleInline},
  {"Label",               2,BIlabel,               (IFOR) labelInline},
  {"hasLabel",            2,BIhasLabel,            (IFOR) hasLabelInline},
  {"ProcedureArity",      2,BIprocedureArity,      (IFOR)procedureArityInline},

  {"TellRecord",  2, BIrecordTell,      0},
  {"WidthC",      2, BIwidthC,          0},

  {"monitorArity",   3, BImonitorArity,   0},
  {"tellRecordSize", 3, BIsystemTellSize, 0},
  {"recordCIsVarB",  2, BIisRecordCVarB,  0},

  {".",            3,BIdot,              (IFOR) dotInline},
  {"^",            3,BIuparrowBlocking,  (IFOR) uparrowInlineBlocking},

  {"HasFeature", 3, BIhasFeatureB,  (IFOR) hasFeatureBInline},
  {"CondSelect", 4, BImatchDefault, (IFOR) matchDefaultInline},
  {"Width",      2, BIwidth,        (IFOR) widthInline},

  {"AtomToString",    2, BIatomToString,        (IFOR) atomToStringInline},
  {"StringToAtom",    2, BIstringToAtom,        0},

  {"NewChunk",        2,BInewChunk,     0},
  {"chunkArity",      2,BIchunkArity,   0},
  {"chunkWidth",      2,BIchunkWidth,   0},
  {"recordWidth",     2,BIrecordWidth,  0},

  {"NewName",         1,BInewName,       0},
  {"NewUniqueName",   2,BInewUniqueName, 0},

  {"==",      3,BIeqB,    (IFOR) eqeqInline},
  {"\\=",     3,BIneqB,   (IFOR) neqInline},
  {"==Rel",   2,BIeq,     0},
  {"\\=Rel",  2,BIneq,    0},

  {"linkObjectFiles",2, BIlinkObjectFiles,      0},
  {"unlinkObjectFile",1,BIunlinkObjectFile,     0},

  {"dlOpen",2,          BIdlOpen,               0},
  {"dlClose",1,         BIdlClose,              0},

  {"FindFile",3,        BIFindFile,             0},
  {"EnvToTuple",2,      BIEnvToTuple,           0},

  {"findFunction",   3, BIfindFunction,         0},
  {"shutdown",       1, BIshutdown,             0},

  {"Alarm",          2, BIalarm,                0},
  {"Delay",          1, BIdelay,                0},

  {"System.gcDo",    0, BIgarbageCollection,    0},

  {"System.apply",   2, BIapply,                0},

  {"System.eq",      3, BIsystemEq,             0},

  {"=",              2, BIunify,                0},
  {"fail",           0, BIfail,                 0},
  {"nop",            0, BInop,                  0},

  {"deepFeed",       2, BIdeepFeed,             0},

  {"getsBound",      1, BIgetsBound,            0},
  {"getsBoundB",     2, BIgetsBoundB,           0},


  {"setAbstractionTabDefaultEntry", 1, BIsetAbstractionTabDefaultEntry, 0},

  {"showBuiltins",0,BIshowBuiltins},
  {"Print", 1, BIprint,  (IFOR) printInline},
  {"Show",  1, BIshow,   (IFOR) showInline},

  {"System.nbSusps", 2, BIconstraints,          0},
  {"SystemGetThreads",    1, BISystemGetThreads},
  {"SystemGetPriorities", 1, BISystemGetPriorities},
  {"SystemGetTime",       1, BISystemGetTime},
  {"SystemGetGC",         1, BISystemGetGC},
  {"SystemGetPrint",      1, BISystemGetPrint},
  {"SystemGetFD",         1, BISystemGetFD},
  {"SystemGetSpaces",     1, BISystemGetSpaces},
  {"SystemGetErrors",     1, BISystemGetErrors},
  {"SystemGetMessages",   1, BISystemGetMessages},
  {"SystemGetMemory",     1, BISystemGetMemory},
  {"SystemGetLimits",     1, BISystemGetLimits},
  {"SystemGetArgv",       1, BISystemGetArgv},
  {"SystemGetStandalone", 1, BISystemGetStandalone},
  {"SystemGetHome",       1, BISystemGetHome},
  {"SystemGetPlatform",   1, BISystemGetPlatform},

  {"SystemSetTime",       1, BISystemSetTime},
  {"SystemSetThreads",    1, BISystemSetThreads},
  {"SystemSetPriorities", 1, BISystemSetPriorities},
  {"SystemSetPrint",      1, BISystemSetPrint},
  {"SystemSetFD",         1, BISystemSetFD},
  {"SystemSetGC",         1, BISystemSetGC},
  {"SystemSetErrors",     1, BISystemSetErrors},
  {"SystemSetMessages",   1, BISystemSetMessages},
  {"SystemSetInternal",   1, BISystemSetInternal},

  {"onToplevel",1,BIonToplevel},
  {"addr",2,BIaddr},

  // source level debugger
  {"Debug.mode",            1, BIdebugmode},
  {"Debug.getStream",       1, BIgetDebugStream},
  {"Debug.setStepFlag",     2, BIsetStepFlag},
  {"Debug.setTraceFlag",    2, BIsetTraceFlag},
  {"Debug.checkStopped",    2, BIcheckStopped},

  // Debug module
  {"Debug.dumpThreads", 0, BIdumpThreads},
  {"Debug.listThreads", 1, BIlistThreads},
  {"Debug.breakpointAt",4, BIbreakpointAt},
  {"Debug.breakpoint",  0, BIbreakpoint},
  {"Debug.displayCode", 2, BIdisplayCode},
  {"Debug.procedureCode", 2, BIprocedureCode},
  {"Debug.livenessX", 2, BIlivenessX},

#ifdef UNUSED
  {"index2Tagged",  2,BIindex2Tagged},
  {"time2localTime",2,BItime2localTime},
#endif

  // Builtins for the Thread module

  {"Thread.is",             2, BIthreadIs},
  {"Thread.id",             2, BIthreadID},
  {"Thread.setId",          2, BIsetThreadID},
  {"Thread.parentId",       2, BIparentThreadID},
  {"Thread.this",           1, BIthreadThis},
  {"Thread.suspend",        1, BIthreadSuspend},
  {"Thread.unleash",        2, BIthreadUnleash},
  {"Thread.resume",         1, BIthreadResume},
  {"Thread.injectException",2, BIthreadRaise},
  {"Thread.preempt",        1, BIthreadPreempt},
  {"Thread.setPriority",    2, BIthreadSetPriority},
  {"Thread.getPriority",    2, BIthreadGetPriority},
  {"Thread.isSuspended",    2, BIthreadIsSuspended},
  {"Thread.state",          2, BIthreadState},
  {"Thread.setRaiseOnBlock",2, BIthreadSetRaiseOnBlock},
  {"Thread.getRaiseOnBlock",2, BIthreadGetRaiseOnBlock},
  {"Thread.taskStack",      4, BIthreadTaskStack},
  {"Thread.frameVariables", 3, BIthreadFrameVariables},
  {"Thread.location",       2, BIthreadLocation},

#ifdef DEBUG_PRINT
  // see print.cc
  {"Debug.print",2,BIdebugPrint},
  {"Debug.printLong",2,BIdebugPrintLong},
#endif

  {"statisticsReset",     0, BIstatisticsReset},
  {"statisticsPrint",     0, BIstatisticsPrint},
  {"statisticsPrintProcs",0, BIstatisticsPrintProcs},
  {"statisticsGetProcs",  1, BIstatisticsGetProcs},
  {"setProfileMode",      1, BIsetProfileMode},
#ifdef PROFILE_INSTR
  {"instructionsPrint",   0, BIinstructionsPrint},
#endif

#ifdef PROFILE_BI
  {"biPrint",   0, BIbiPrint},
#endif

#ifdef DEBUG_TRACE
  {"halt",0,BIhalt},
#endif

  {"System.printName",2,BIgetPrintName},
  {"System.printInfo",1,BIprintInfo},
  {"System.printError",1,BIprintError},
  {"System.valueToVirtualString",4,BItermToVS},
  {"getTermSize",4,BIgetTermSize},
  {"heapUsed",1,BIheapUsed},

  {"foreignFDProps", 1, BIforeignFDProps},

  {"@",               2,BIat,                  (IFOR) atInline},
  {"<-",              2,BIassign,              (IFOR) assignInline},
  {"copyRecord",      2,BIcopyRecord,          0},
  {"makeClass",       6,BImakeClass,           0},
  {",",               2,BIcomma,               0},
  {"send",            3,BIsend,                0},
  {"getClass",        2,BIgetClass,            (IFOR) getClassInline},
  {"ooGetLock",       1,BIooGetLock,           (IFOR) ooGetLockInline},
  {"newObject",       2,BInewObject,           (IFOR) newObjectInline},
  {"New",             3,BINew,                0},
  {"setSelf",         1,BIsetSelf,            0},
  {"ooExch",          3,BIooExch,             (IFOR) ooExchInline},

  {"Space.new",           2, BInewSpace,        0},
  {"IsSpace",             2, BIisSpace,         0},
  {"Space.ask",           2, BIaskSpace,        0},
  {"Space.askVerbose",    2, BIaskVerboseSpace, 0},
  {"Space.merge",         2, BImergeSpace,      0},
  {"Space.clone",         2, BIcloneSpace,      0},
  {"Space.commit",        2, BIcommitSpace,     0},
  {"Space.inject",        2, BIinjectSpace,     0},

  {"biExceptionHandler",         1, BIbiExceptionHandler,         0},
  {"setDefaultExceptionHandler", 1, BIsetDefaultExceptionHandler, 0},
  {"getDefaultExceptionHandler", 1, BIgetDefaultExceptionHandler, 0},

  {"raise",      1, BIraise,      0},
  {"raiseError", 1, BIraiseError, 0},
  {"raiseDebug", 1, BIraiseDebug, 0},

  // builtins for the new compiler
  // (OPI and environment handling):
  {"setOPICompiler",             1, BIsetOPICompiler,             0},
  {"getOPICompiler",             1, BIgetOPICompiler,             0},
  {"isBuiltin",                  2, BIisBuiltin,                  0},
  {"getBuiltinName",             2, BIgetBuiltinName,             0},
  {"nameVariable",               2, BInameVariable,               0},
  {"newNamedName",               2, BInewNamedName,               0},
  {"isUniqueName",               2, BIisUniqueName,               0},
  {"generateAbstractionTableID", 2, BIgenerateAbstractionTableID, 0},
  {"concatenateAtomAndInt",      3, BIconcatenateAtomAndInt,      0},
  {"RegSet.new",                 3, BIregSet_new,                 0},
  {"RegSet.copy",                2, BIregSet_copy,                0},
  {"RegSet.adjoin",              2, BIregSet_adjoin,              0},
  {"RegSet.remove",              2, BIregSet_remove,              0},
  {"RegSet.member",              3, BIregSet_member,              0},
  {"RegSet.union",               2, BIregSet_union,               0},
  {"RegSet.intersect",           2, BIregSet_intersect,           0},
  {"RegSet.subtract",            2, BIregSet_subtract,            0},
  {"RegSet.toList",              2, BIregSet_toList,              0},
  {"RegSet.complementToList",    2, BIregSet_complementToList,    0},

  // Oz parser
  {"ozparser_parseFile",         3, ozparser_parseFile,           0},
  {"ozparser_parseVirtualString",3, ozparser_parseVirtualString,  0},
  {"ozparser_fileExists",        2, ozparser_fileExists,          0},

  {"copyCode",                   2, BIcopyCode,                   0},

#ifdef FINALIZATION
  // Finalization
  {"Finalize.register",          2, BIfinalize_register,          0},
  {"Finalize.setHandler",        1, BIfinalize_setHandler,        0},
#endif

#ifdef CS_PROFILE
  {"GetCloneDiff", 2, BIgetCloneDiff, 0},
#endif

  {"ServiceRegistry",1,BIservice_registry,0},

  {0,0,0,0}
};


extern void BIinitFD(void);
extern void BIinitFSet(void);
extern void BIinitMeta(void);
extern void BIinitAVar(void);
extern void BIinitPerdioVar(void);
extern void BIinitUnix();
extern void BIinitAssembler();
extern void BIinitTclTk();
extern void BIinitPerdio();
extern void BIinitLazy();

TaggedRef dummyState;

BuiltinTabEntry *BIinit()
{
  BuiltinTabEntry *bi = BIadd("builtin",3,BIbuiltin);

  if (!bi)
    return bi;

  BIaddSpec(allSpec);

  BIinitAssembler();

  BIinitFD();
  BIinitFSet();
  BIinitMeta();

  BIinitAVar();
  BIinitPerdioVar();
  BIinitUnix();
  BIinitTclTk();

  BIinitPerdio();

  BIinitLazy();

  dummyRecord = makeTaggedNULL();
  OZ_protect(&dummyRecord);

  service_registry = makeTaggedConst(new OzDictionary(ozx_rootBoard()));
  OZ_protect(&service_registry);

  return bi;
}
