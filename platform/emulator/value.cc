/*
 *  Authors:
 *    Michael Mehl (mehl@dfki.de)
 *    Ralf Scheidhauer (Ralf.Scheidhauer@ps.uni-sb.de)
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

#if defined(INTERFACE)
#pragma implementation "value.hh"
#endif

#include "runtime.hh"
#include "dictionary.hh"

/*===================================================================
 * global names and atoms
 *=================================================================== */

TaggedRef  AtomNil, AtomCons, AtomPair, AtomVoid,
  AtomSucceeded, AtomAlt, AtomMerged, AtomFailed,
  AtomEntailed, AtomSuspended, AtomBlocked,
  AtomEmpty, AtomUpper, AtomLower, AtomDigit,
  AtomCharSpace, AtomPunct, AtomOther,
  NameTrue, NameFalse, AtomBool, AtomSup, AtomCompl,
  AtomMin, AtomMax, AtomMid,
  AtomNaive, AtomSize, AtomNbSusps,
  AtomDebugCallC, AtomDebugCallF, AtomDebugCondC, AtomDebugCondF,
  AtomDebugLockC, AtomDebugLockF,
  AtomException, AtomUnify,

  NameOoFreeFlag,NameOoAttr,NameOoFreeFeatR,NameOoUnFreeFeat,
  NameOoFastMeth,NameOoDefaults,NameOoRequiredArg,NameOoDefaultVar,
  NameOoPrintName,NameOoLocking,NameOoFallback,NameOoId,
  AtomNew, AtomSend, AtomApply, AtomApplyList,

  NameUnit,
  NameNonExportable,
  AtomKinded, AtomDet, AtomRecord, AtomLow, AtomFSet,
  // Atoms for System.get and System.set
  AtomActive, AtomAtoms, AtomBuiltins, AtomCache, AtomCommitted,
  AtomCloned, AtomCode, AtomCopy, AtomCreated, AtomDebug, AtomDepth,
  AtomFeed, AtomForeign, AtomFree, AtomFreelist, AtomGC, AtomHigh,
  AtomHints, AtomIdle, AtomInt, AtomInvoked, AtomLimits, AtomLoad,
  AtomLocation, AtomMedium, AtomNames, AtomOn, AtomPropagate,
  AtomPropagators, AtomRun, AtomRunnable, AtomShowSuspension,
  AtomStopOnToplevelFailure, AtomSystem, AtomThread,
  AtomTotal, AtomDetailed, AtomBrowser, AtomApplet,
  AtomThreshold, AtomTolerance, AtomUser, AtomVariables, AtomWidth, AtomHeap,
  AtomDebugIP, AtomDebugPerdio,
  // Atoms for NetError Handlers
  AtomTempBlocked, AtomPermBlocked,
  AtomTempMe, AtomPermMe,
  AtomTempAllOthers, AtomPermAllOthers,
  AtomTempSomeOther, AtomPermSomeOther, AtomEntityNormal,
  AtomPerm, AtomTemp,AtomTempHome,AtomTempForeign,
  AtomPermHome,AtomPermForeign,
  AtomContinue, AtomRetry,
  AtomYes,AtomNo,AtomPerSite,AtomPerThread,AtomAll,AtomAny,
  AtomHandler,AtomWatcher,

  RecordFailure,
  E_ERROR, E_KERNEL, E_OBJECT, E_TK, E_OS, E_SYSTEM,
  BI_Unify,BI_portWait,BI_Show,BI_send,BI_probe,BI_Delay,BI_startTmp,
  BI_load, BI_fail, BI_url_load,
  BI_exchangeCell,BI_assign,BI_atRedo,BI_lockLock,
  BI_controlVarHandler;



TaggedRef getUniqueName(const char *s)
{
  CHECK_STRPTR(s);
  Literal *ret = addToNameTab(s);
  ret->setFlag(Lit_isUniqueName);
  return makeTaggedLiteral(ret);
}

// Some often used constants
void initLiterals()
{
  AtomNil   = makeTaggedAtom("nil");
  AtomCons  = makeTaggedAtom("|");
  AtomPair  = makeTaggedAtom("#");
  AtomVoid  = makeTaggedAtom("_");

  AtomBool  = makeTaggedAtom("bool");
  AtomSup   = makeTaggedAtom("sup");
  AtomCompl = makeTaggedAtom("compl");

  AtomEmpty     = makeTaggedAtom("");
  AtomUpper     = makeTaggedAtom("upper");
  AtomLower     = makeTaggedAtom("lower");
  AtomDigit     = makeTaggedAtom("digit");
  AtomCharSpace = makeTaggedAtom("space");
  AtomPunct     = makeTaggedAtom("punct");
  AtomOther     = makeTaggedAtom("other");

  AtomSucceeded    = makeTaggedAtom("succeeded");
  AtomAlt          = makeTaggedAtom("alternatives");
  AtomEntailed     = makeTaggedAtom("entailed");
  AtomSuspended    = makeTaggedAtom("suspended");
  AtomBlocked      = makeTaggedAtom("blocked");
  AtomMerged       = makeTaggedAtom("merged");
  AtomFailed       = makeTaggedAtom("failed");

  AtomDebugCallC   = makeTaggedAtom("call/c");
  AtomDebugCallF   = makeTaggedAtom("call/f");
  AtomDebugCondC   = makeTaggedAtom("conditional/c");
  AtomDebugCondF   = makeTaggedAtom("conditional/f");
  AtomDebugLockC   = makeTaggedAtom("lock/c");
  AtomDebugLockF   = makeTaggedAtom("lock/f");

  AtomUnify        = makeTaggedAtom("unify");
  AtomException    = makeTaggedAtom("exception");

  NameUnit          = getUniqueName("unit");
  NameNonExportable = getUniqueName("nonExportable");

  NameTrue         = getUniqueName(NAMETRUE);
  NameFalse        = getUniqueName(NAMEFALSE);

  NameOoAttr        = getUniqueName("ooAttr");
  NameOoFreeFeatR   = getUniqueName("ooFreeFeatR");
  NameOoFreeFlag    = getUniqueName("ooFreeFlag");
  NameOoDefaultVar  = getUniqueName("ooDefaultVar");
  NameOoRequiredArg = getUniqueName("ooRequiredArg");
  NameOoFastMeth    = getUniqueName("ooFastMeth");
  NameOoUnFreeFeat  = getUniqueName("ooUnFreeFeat");
  NameOoDefaults    = getUniqueName("ooDefaults");
  NameOoPrintName   = getUniqueName("ooPrintName");
  NameOoLocking     = getUniqueName("ooLocking");
  NameOoFallback    = getUniqueName("ooFallback");
  NameOoId          = getUniqueName("ooId");

  AtomNew           = makeTaggedAtom("new");
  AtomSend          = makeTaggedAtom("send");
  AtomApply         = makeTaggedAtom("apply");
  AtomApplyList     = makeTaggedAtom("applyList");

  AtomMin     = makeTaggedAtom("min");
  AtomMax     = makeTaggedAtom("max");
  AtomMid     = makeTaggedAtom("mid");
  AtomNaive   = makeTaggedAtom("naive");
  AtomSize    = makeTaggedAtom("size");
  AtomNbSusps = makeTaggedAtom("nbSusps");

  AtomLow          = makeTaggedAtom("low");

  // For system set and get
  AtomActive                = makeTaggedAtom("active");
  AtomAtoms                 = makeTaggedAtom("atoms");
  AtomBuiltins              = makeTaggedAtom("builtins");
  AtomCache                 = makeTaggedAtom("cache");
  AtomCommitted             = makeTaggedAtom("committed");
  AtomCloned                = makeTaggedAtom("cloned");
  AtomCode                  = makeTaggedAtom("code");
  AtomCopy                  = makeTaggedAtom("copy");
  AtomCreated               = makeTaggedAtom("created");
  AtomDebug                 = makeTaggedAtom("debug");
  AtomDepth                 = makeTaggedAtom("depth");
  // AtomFailed
  AtomFeed                  = makeTaggedAtom("feed");
  AtomForeign               = makeTaggedAtom("foreign");
  AtomFree                  = makeTaggedAtom("free");
  AtomFreelist              = makeTaggedAtom("freelist");
  AtomGC                    = makeTaggedAtom("gc");
  AtomHigh                  = makeTaggedAtom("high");
  AtomHints                 = makeTaggedAtom("hints");
  AtomIdle                  = makeTaggedAtom("idle");
  AtomInt                   = makeTaggedAtom("int");
  AtomInvoked               = makeTaggedAtom("invoked");
  AtomLimits                = makeTaggedAtom("limits");
  AtomLoad                  = makeTaggedAtom("load");
  AtomLocation              = makeTaggedAtom("location");
  // AtomMax
  AtomMedium                = makeTaggedAtom("medium");
  // AtomMin
  AtomNames                 = makeTaggedAtom("names");
  AtomOn                    = makeTaggedAtom("on");
  AtomPropagate             = makeTaggedAtom("propagate");
  AtomPropagators           = makeTaggedAtom("propagators");
  AtomRun                   = makeTaggedAtom("run");
  AtomRunnable              = makeTaggedAtom("runnable");
  AtomShowSuspension        = makeTaggedAtom("showSuspension");
  // AtomSize
  AtomStopOnToplevelFailure = makeTaggedAtom("stopOnToplevelFailure");
  // AtomSucceeded
  AtomSystem                = makeTaggedAtom("system");
  AtomThread                = makeTaggedAtom("thread");
  AtomThreshold             = makeTaggedAtom("threshold");
  AtomTolerance             = makeTaggedAtom("tolerance");
  AtomTotal                 = makeTaggedAtom("total");
  AtomUser                  = makeTaggedAtom("user");
  AtomVariables             = makeTaggedAtom("variables");
  AtomWidth                 = makeTaggedAtom("width");
  AtomHeap                  = makeTaggedAtom("heap");
  AtomDetailed              = makeTaggedAtom("detailed");
  AtomBrowser               = makeTaggedAtom("browser");
  AtomApplet                = makeTaggedAtom("applet");

  // AtomFree                  = makeTaggedAtom("free");
  AtomKinded                = makeTaggedAtom("kinded");
  AtomDet                   = makeTaggedAtom("det");
  AtomRecord                = makeTaggedAtom("record");
  AtomFSet                  = makeTaggedAtom("fset");
  // AtomInt                   = makeTaggedAtom("int");

  AtomDebugIP               = makeTaggedAtom("debugIP");
  AtomDebugPerdio           = makeTaggedAtom("debugPerdio");

  // Atom Handlers
  AtomTempBlocked             = makeTaggedAtom("tempBlocked");
  AtomPermBlocked             = makeTaggedAtom("permBlocked");
  AtomTempMe                  = makeTaggedAtom("tempMe");
  AtomPermMe                  = makeTaggedAtom("permMe");
  AtomTempAllOthers           = makeTaggedAtom("tempAllOthers");
  AtomPermAllOthers           = makeTaggedAtom("permAllOthers");
  AtomTempSomeOther           = makeTaggedAtom("tempSomeOther");
  AtomPermSomeOther           = makeTaggedAtom("permSomeOther");
  AtomEntityNormal            = makeTaggedAtom("entityNormal");
  AtomTemp                    = makeTaggedAtom("temp");
  AtomTempHome                = makeTaggedAtom("tempHome");
  AtomTempForeign             = makeTaggedAtom("tempForeign");
  AtomPerm                    = makeTaggedAtom("perm");
  AtomPermHome                = makeTaggedAtom("permHome");
  AtomPermForeign             = makeTaggedAtom("permForeign");
  AtomContinue                = makeTaggedAtom("continue");
  AtomRetry                   = makeTaggedAtom("retry");
  AtomYes                     = makeTaggedAtom("yes");
  AtomNo                      = makeTaggedAtom("no");
  AtomPerSite                 = makeTaggedAtom("perSite");
  AtomPerThread               = makeTaggedAtom("perThread");
  AtomHandler                 = makeTaggedAtom("handler");
  AtomWatcher                 = makeTaggedAtom("watcher");
  AtomAny                     = makeTaggedAtom("any");
  AtomAll                     = makeTaggedAtom("all");

  RecordFailure = OZ_record(OZ_atom("failure"),
                            OZ_cons(OZ_atom("debug"),OZ_nil()));
  OZ_putSubtree(RecordFailure,OZ_atom("debug"),NameUnit);
  OZ_protect(&RecordFailure);

  E_ERROR = makeTaggedAtom("error");
  E_KERNEL= makeTaggedAtom("kernel");
  E_OBJECT= makeTaggedAtom("object");
  E_TK    = makeTaggedAtom("tk");
  E_OS    = makeTaggedAtom("os");
  E_SYSTEM= makeTaggedAtom("system");

}

/*===================================================================
 * Literal
 *=================================================================== */


int Name::NameCurrentNumber = 0x200000;

const char *Literal::getPrintName()
{
  if (isAtom())
    return ((Atom*)this)->getPrintName();

  if (getFlags()&Lit_isNamedName)
    return ((NamedName*)this)->printName;

  return "";
}


Atom *Atom::newAtom(const char *str)
{
  Atom *ret = (Atom*) malloc(sizeof(Atom));
  ret->init();
  ret->printName = str;
  ret->setOthers(strlen(str));
  return ret;
}

Name *Name::newName(Board *home)
{
  COUNT(numNewName);

  Name *ret = (Name*) heapMalloc(sizeof(Name));
  ret->init();
  ret->homeOrGName = ToInt32(home);
  ret->setOthers(NameCurrentNumber += 1 << sizeOfCopyCount);
  ret->setFlag(Lit_isName);
  return ret;
}


NamedName *NamedName::newNamedName(const char *pn)
{
  COUNT(numNewNamedName);

  NamedName *ret = (NamedName*) malloc(sizeof(NamedName));
  ret->init();
  Assert(am.onToplevel());
  ret->homeOrGName = ToInt32(am.currentBoard());
  ret->setOthers(NameCurrentNumber += 1 << sizeOfCopyCount);
  ret->setFlag(Lit_isName|Lit_isNamedName);
  ret->printName = pn;
  return ret;
}


NamedName *NamedName::generateCopy()
{
  COUNT(numNewNamedName);

  NamedName *ret = (NamedName*) malloc(sizeof(NamedName));
  ret->init();
  Assert(am.onToplevel() && isCopyableName());
  ret->homeOrGName = ToInt32(am.currentBoard());
  int seqNumber = getOthers();
  seqNumber++;
  Assert(seqNumber);
  setOthers(seqNumber);
  ret->setOthers(seqNumber);
  ret->setFlag(getFlags() & ~Lit_isCopyableName);
  ret->printName = printName;
  return ret;
}


void Name::import(GName *name)
{
  Assert(oz_isRootBoard(GETBOARD(this)));
  homeOrGName = ToInt32(name);
  setFlag(Lit_hasGName);
}

/*===================================================================
 * ConstTerm
 *=================================================================== */

const char *ObjectClass::getPrintName()
{
  TaggedRef aux = classGetFeature(NameOoPrintName);
  return aux ? tagged2Literal(aux)->getPrintName() : "???";
}

const char *ConstTerm::getPrintName()
{
  switch (getType()) {
  case Co_Abstraction:
    return ((Abstraction *) this)->getPrintName();
  case Co_Object:
    return ((Object *) this)->getPrintName();
  case Co_Class:
    return ((ObjectClass *) this)->getPrintName();
  case Co_Builtin:
    return ((Builtin *)this)->getPrintName();
  default:
    return "UnknownConst";
  }
}

int ConstTerm::getArity()
{
  switch (getType()) {
  case Co_Abstraction: return ((Abstraction *) this)->getArity();
  case Co_Object:      return 1;
  case Co_Builtin:     return ((Builtin *)this)->getArity();
  default:             return -1;
  }
}

void Tertiary::setBoard(Board *b)
{
  if (getTertType() == Te_Local) {
    setPointer(b);
  } else {
    Assert(b==NULL || oz_isRootBoard(b));
  }
}



/*===================================================================
 * Object
 *=================================================================== */

/*
 * append two *det* lists
 *  NO ERROR CHECK!
 */
TaggedRef appendI(TaggedRef x,TaggedRef y)
{
  TaggedRef ret;
  TaggedRef *out=&ret;

  x=oz_deref(x);
  while (oz_isCons(x)) {
    LTuple *lt=new LTuple(head(x),makeTaggedNULL());
    *out=makeTaggedLTuple(lt);
    out=lt->getRefTail();
    x=oz_deref(tail(x));
  }
  *out=y;
  return ret;
}

Bool member(TaggedRef elem,TaggedRef list)
{
  elem = oz_deref(elem);
  list = oz_deref(list);
  while (oz_isCons(list)) {
    if (elem==oz_deref(head(list)))
      return OK;
    list = oz_deref(tail(list));
  }
  return NO;
}

/*
 * destructive reverse of a list
 */
TaggedRef reverseC(TaggedRef l)
{
  TaggedRef out=nil();
  l=oz_deref(l);
  while (oz_isCons(l)) {
    LTuple *lt=tagged2LTuple(l);
    TaggedRef next=oz_deref(lt->getTail());
    lt->setTail(out);
    out = l;
    l = next;
  }
  Assert(oz_isNil(l));
  return out;
}

// make a copy of list and return its length
TaggedRef duplist(TaggedRef list, int &len)
{
  len = 0;
  TaggedRef ret = nil();
  TaggedRef *aux = &ret;

  while(oz_isCons(list)) {
    len++;
    *aux = cons(head(list),*aux);
    aux = tagged2LTuple(*aux)->getRefTail();
    list = tail(list);
  }
  return ret;
}

TaggedRef Object::getArityList()
{
  TaggedRef ret = nil();

  SRecord *feat=getFreeRecord();
  if (feat) ret = feat->getArityList();

  SRecord *rec=getClass()->getUnfreeRecord();
  if (rec) ret=appendI(ret,rec->getArityList());
  return ret;
}

TaggedRef ObjectClass::getArityList()
{
  return features->getArityList();
}

int Object::getWidth()
{
  int ret = 0;
  SRecord *feat=getFreeRecord();
  if (feat) ret = feat->getWidth ();

  SRecord *rec=getClass()->getUnfreeRecord();
  if (rec) ret += rec->getWidth ();
  return ret;
}


int ObjectClass::getWidth()
{
  return features->getWidth();
}




Abstraction *ObjectClass::getMethod(TaggedRef label, SRecordArity arity, RefsArray X,
                               Bool &defaultsUsed)
{
  TaggedRef method;
  if (getfastMethods()->getArg(label,method)!=PROCEED)
    return NULL;

  DEREF(method,_1,_2);
  if (oz_isVariable(method)) return NULL;
  Assert(oz_isAbstraction(method));

  Abstraction *abstr = (Abstraction*) tagged2Const(method);
  defaultsUsed = NO;
  if (sameSRecordArity(abstr->getMethodArity(),arity))
    return abstr;
  defaultsUsed = OK;
  return lookupDefault(label,arity,X) ? abstr : (Abstraction*) NULL;
}

Bool ObjectClass::lookupDefault(TaggedRef label, SRecordArity arity, RefsArray X)
{
  TaggedRef def;
  if (getDefMethods()->getArg(label,def)!=PROCEED)
    return NO;

  def = oz_deref(def);
  Assert(oz_isSRecord(def));
  SRecord *rec = tagged2SRecord(def);

  if (rec->isTuple()) {
    if (!sraIsTuple(arity)) {
      return NO;
    }
    int widthDefault  = rec->getWidth();
    int widthProvided = getTupleWidth(arity);
    if (widthDefault < widthProvided ||
        literalEq(oz_deref(rec->getArg(widthProvided)),NameOoRequiredArg))
      return NO;

    for (int i=widthProvided; i<widthDefault; i++) {
      if (literalEq(oz_deref(rec->getArg(i)),NameOoDefaultVar)) {
        X[i] = oz_newVariable();
      } else {
        X[i] = rec->getArg(i);
      }
    }
    return OK;
  }

  TaggedRef auxX[100];
  if (::getWidth(arity)>=100)
    return NO;

  TaggedRef arityList = sraGetArityList(arity);

  def = rec->getArityList();

  int argno;
  int argnoProvided = 0;
  for (argno = 0; oz_isCons(def); def = tail(def), argno++) {
    TaggedRef feat  = head(def);
    TaggedRef value = oz_deref(rec->getArg(argno));

    if (!oz_isNil(arityList) && featureEq(head(arityList),feat)) {
      arityList = tail(arityList);
      auxX[argno] = X[argnoProvided];
      argnoProvided++;
    } else if (literalEq(value,NameOoDefaultVar)) {
      auxX[argno] = oz_newVariable();
    } else if (literalEq(value,NameOoRequiredArg)) {
      return NO;
    } else {
      auxX[argno] = rec->getArg(argno);
    }
  }

  /* overspecified? */
  if (!oz_isNil(arityList))
    return NO;

  while(argno>0) {
    argno--;
    X[argno] = auxX[argno];
  }

  return OK;
}

TaggedRef ObjectClass::getFallbackNew() {
  TaggedRef fbs = oz_deref(classGetFeature(NameOoFallback));

  if (!oz_isSRecord(fbs))
    return 0;

  SRecord * sr = tagged2SRecord(fbs);

  TaggedRef fbn = oz_deref(sr->getFeature(AtomNew));

  if (!oz_isAbstraction(fbn) || tagged2Const(fbn)->getArity() != 3)
    return 0;

  return fbn;
}

TaggedRef ObjectClass::getFallbackSend() {
  TaggedRef fbs = oz_deref(classGetFeature(NameOoFallback));

  if (!oz_isSRecord(fbs))
    return 0;

  SRecord * sr = tagged2SRecord(fbs);

  TaggedRef fbss = oz_deref(sr->getFeature(AtomSend));

  if (!oz_isAbstraction(fbss) || tagged2Const(fbss)->getArity() != 3)
    return 0;

  return fbss;
}

TaggedRef ObjectClass::getFallbackApply() {
  TaggedRef fbs = oz_deref(classGetFeature(NameOoFallback));

  if (!oz_isSRecord(fbs))
    return 0;

  SRecord * sr = tagged2SRecord(fbs);

  TaggedRef fba = oz_deref(sr->getFeature(AtomApply));

  if (!oz_isAbstraction(fba) || tagged2Const(fba)->getArity() != 2)
    return 0;

  return fba;
}


/*===================================================================
 * Bigint memory management
 *=================================================================== */

static void *bigint_alloc(size_t size)
{
  return freeListMalloc(size);
}

static void bigint_dealloc(void *ptr, size_t size)
{
  freeListDispose(ptr,size);
}

static void *bigint_realloc(void *ptr, size_t old_size, size_t new_size)
{
  void *ret = bigint_alloc(new_size);
  memcpy(ret,ptr,old_size);
  bigint_dealloc(ptr,old_size);
  return ret;
}

void bigIntInit()
{
  mp_set_memory_functions(bigint_alloc,bigint_realloc,bigint_dealloc);
}

/*===================================================================
 * SRecord
 *=================================================================== */


void SRecord::initArgs()
{
  for (int i = getWidth(); i--; )
    args[i] = oz_newVariable();
}

/************************************************************************/
/*                      Useful Stuff: Lists                             */
/************************************************************************/

/*
 * Precondition: lista and listb are increasing lists of features.
 * Test, whether the two lists are equal.
 * Completely deref'd
 *
 * mm2: possible bug: two different names may have the same Id
 *      (modulo counter).
 */

static
Bool listequal(TaggedRef lista, TaggedRef listb)
{
  while (oz_isCons(lista)) {
    if (!oz_isCons(listb)) return NO;
    if ( !featureEq(head(lista),head(listb)) ) return NO;

    lista = tail(lista);
    listb = tail(listb);
  }
  Assert(oz_isNil(lista));
  return oz_isNil(listb);
}

/*
 * Precondition: list is an increasing list of atoms.
 * If a is contained in list, return a list which is structally
 * equivalent to list. Otherwise, return the list which is obtained
 * by inserting a into list.
 * Everything is deref'd
 */

static
TaggedRef insert(TaggedRef a, TaggedRef list) {

  Assert(oz_isFeature(a));

  TaggedRef out;
  TaggedRef *ptr=&out;

  while (oz_isCons(list)) {
    TaggedRef oldhead = head(list);
    CHECK_DEREF(oldhead);

    switch (featureCmp(a,oldhead)) {
    case 0:
      *ptr = list;
      return out;
    case -1:
      *ptr = cons(a,list);
      return out;
    case 1:
      {
        LTuple *lt = new LTuple(oldhead,makeTaggedNULL());
        *ptr = makeTaggedLTuple(lt);
        ptr = lt->getRefTail();
        list=tail(list);
      }
      break;
    default:
      error("insert");
      return 0;
    }
  }
  Assert(oz_isNil(list));
  *ptr=cons(a,nil());

  return out;
}

/*
 * Precondition: old is an increasing list of features, ins is a list of
 * features. Return the list obtained by succesively inserting all the
 * elements of ins into old.
 * Old is deref'd
 */

static
TaggedRef insertlist(TaggedRef ins, TaggedRef old)
{
  CHECK_DEREF(old);
  DEREF(ins,_1,_2);
  CHECK_NONVAR(ins);

  while (oz_isCons(ins)) {
    old = insert(oz_deref(head(ins)),old);
    CHECK_DEREF(old);
    ins = oz_deref(tail(ins));
    CHECK_NONVAR(ins);
  }

  Assert(oz_isNil(ins));

  return old;
}

inline
void swap(TaggedRef** a, TaggedRef** b)
{
  register TaggedRef aux = **a;
  **a = **b;
  **b = aux;
}

static
void quicksort(TaggedRef** first, TaggedRef** last)
{
  register TaggedRef** i;
  register TaggedRef** j;

  if (first >= last)
    return;

  /* use middle element as pivot --> much better for sorted inputs */
  TaggedRef **middle = first + (last-first)/2;
  swap(first,middle);

  for (i = first, j = last; ; j--) {
    while (i != j && featureCmp(**i, **j) <= 0)
      j--;
    if (i == j)
      break;
    swap(i, j);
    do
      i++;
    while (i != j && featureCmp(**i, **j) <= 0);
    if (i == j)
      break;
    swap(i, j);
  } // for
  quicksort(first, i-1);
  quicksort(i+1, last);
}

Bool isSorted(TaggedRef list)
{
  list = oz_deref(list);
  if (oz_isNil(list)) return OK;

  while(1) {
    TaggedRef cdr = oz_deref(tail(list));
    if (oz_isNil(cdr)) return OK;
    if (featureCmp(head(list),head(cdr))!=-1) return NO;
    list = cdr;
  }
  return OK;

}


// mm2: optimize for already sorted list! (see isSorted)
// sort list using quicksort and duplicants
TaggedRef sortlist(TaggedRef list,int len)
{
  TaggedRef** r = new TaggedRef*[len];

  // put pointers to elems of list in array r
  TaggedRef tmp = list;
  int i = 0;
  while (oz_isCons(tmp)) {
    r[i++] = tagged2LTuple(tmp)->getRef();
    tmp = tail(tmp);
  }

  // sort array r using quicksort
  quicksort(r, r + (len - 1));

  // remove possible duplicate entries
  TaggedRef pElem = list;
  TaggedRef cElem = tagged2LTuple(list)->getTail();
  int p = 0, c = 1;
  while (oz_isCons(cElem)) {
    LTuple* cElemPtr = tagged2LTuple(cElem);
    if (featureEq(*r[p], *r[c])) {
      tagged2LTuple(pElem)->setTail(cElemPtr->getTail());
    } else {
      pElem = cElem;
      p = c;
    }
    c += 1;
    cElem = cElemPtr->getTail();
  } // while

  delete r;
  return list;
}

// mm2: cycle test
TaggedRef packsort(TaggedRef list)
{
  list=oz_deref(list);
  if (oz_isNil(list)) {
    return nil();
  }
  int len=0;

  TaggedRef tmp = list;

  while (oz_isCons(tmp)) {
    len++;
    LTuple *lt=tagged2LTuple(tmp);
    lt->setHead(oz_deref(lt->getHead()));
    tmp=oz_deref(lt->getTail());
    lt->setTail(tmp);
  }

 if (!oz_isNil(tmp)) return 0;

 return sortlist(list,len);
}

/************************************************************************/
/*                      Useful Stuff: Numbers                           */
/************************************************************************/

/*
 *      Return the truncate of the logarithm of i by base 2.
 */

inline
unsigned int intlog(unsigned int i)
{
  if ( i ) {
    unsigned int result = 0;
    while (i>>=1)
      result++;
    return result;
  } else {
    return 0;
  }
}

//************************************************************************
//                        Class Arity
//************************************************************************

/*
 *      Precondition: entrylist is a list of different atoms.
 *      Construct a Arity holding these atoms, assigning them all
 *      different successive indices.
 */

Arity *Arity::newArity(TaggedRef entrylist , Bool itf)
{
  int w = fastlength(entrylist);

  if (itf) {
    Arity *ar = (Arity *) (void *) new char[sizeof(Arity)];
    ar->next = NULL;
    ar->list = entrylist;
    ar->hashmask = 0;
    ar->width = w;
    return ar;
  }

  int size  = nextPowerOf2((int)(w*1.5));
  Arity *ar = (Arity *) (void *) new char[sizeof(Arity)+
                                         sizeof(KeyAndIndex)*size];

  DebugCheckT(ar->numberOfCollisions = 0);
  ar->next = NULL;
  ar->list = entrylist;
  ar->width = w;
  ar->hashmask = size-1;
  int j=0;
  for (int i=0 ; i<size ; ar->table[i++].key = 0);
  while (oz_isCons(entrylist)) {
    const TaggedRef entry = head(entrylist);
    const int hsh         = featureHash(entry);
    int i                 = ar->hashfold(hsh);
    const int step        = ar->scndhash(hsh);
    while (ar->table[i].key) {
      DebugCheckT(ar->numberOfCollisions++);
      i = ar->hashfold(i+step);
    }
    ar->table[i].key   = entry;
    ar->table[i].index = j++;
    entrylist = tail(entrylist);
  }
  return ar;
}


int Arity::lookupInternal(TaggedRef entry)
{
  Assert(!isTuple());
  const int hsh  = featureHash(entry);

  int i          = hashfold(hsh);
  const int step = scndhash(hsh);
  while (1) {
    const TaggedRef key = table[i].key;
    if (!key) return -1;
    if (featureEq(key,entry)) {
      return table[i].index;
    }
    i = hashfold(i+step);
  }
}

/************************************************************************/
/*                          Class ArityTable                            */
/************************************************************************/

/*
 *      Initialize the aritytable.
 */

ArityTable aritytable(ARITYTABLESIZE);

/*
 *      Construct an ArityTable of given size. The size should be a power of 2
 *      in order to make the hashing work.
 */

ArityTable::ArityTable ( int n )
{
  size = nextPowerOf2(n);
  table = ::new Arity*[size];
  for ( int i = 0 ; i < size; table[i++] = NULL ) ;
  hashmask = size-1;
}

/*
 * Compute the hashvalue of a list into aritytable.
 * For now, we just take the average of the hashvalues of the first three
 * entries in the list. The hashvalues of the entries are computed
 * analogously to the class Arity.
 * TODO: find a better hash heuristics!
 *
 * return NO, if no tuple
 *        OK, if tuple
 */

inline
Bool ArityTable::hashvalue( TaggedRef list, int &ret )
{
  int i = 0;
  int len = 0;
  while(oz_isCons(list)){
    TaggedRef it=head(list);
    if (len>=0 && oz_isSmallInt(it) && smallIntValue(it)==len+1) {
      len++;
    } else {
      len = -1;
    }
    i += featureHash(it);
    list = tail(list);
  }
  Assert(oz_isNil(list));
  ret = hashfold(i);
  return len < 0 ? NO : OK;
}

/*
 * If list is already registered in aritytable, then return the associated
 * Arity. Otherwise, create a Hashtable, insert the new pair of
 * arity and Arity into aritytable, and return the new Arity.
 */
Arity *ArityTable::find( TaggedRef list)
{
  int hsh;
  int isTuple = hashvalue(list,hsh);

  Arity *ret;
  if ( table[hsh] == NULL ) {
    ret = Arity::newArity(list,isTuple);
    table[hsh] = ret;
  } else {
    Arity* c = table[hsh];
    while ( c->next != NULL ) {
      if ( listequal(c->list,list) ) return c;
      c = c->next;
    }
    if ( listequal(c->list,list) ) return c;
    ret = Arity::newArity(list,isTuple);
    c->next = ret;
  }
  return ret;
}


void ArityTable::printStat()
{
  int ec=0,ne=0,na=0,ac=0;
  for (int i = 0 ; i < size ; i ++) {
    Arity *ar = table[i];
    while (ar) {
      na++;
      ne += ar->getWidth();
      ec += ar->getCollisions();
      ar = ar->next;
      if (ar) ac++;
    }
  }
  printf("Aritytable statistics\n");
  printf("Arities:          %d\n", na);
  printf("Arity collisions: %d\n", ac);
  printf("Entries:          %d\n", ne);
  printf("Entry collisions: %d\n", ec);
}

/************************************************************************/
/*                      Class Record                          */
/************************************************************************/


/*
 * Precondition: lista and listb are strictly increasing lists of features.
 * Return the merge of the two, without duplicates.
 * everything is deref'd
 */

static
TaggedRef merge(TaggedRef lista, TaggedRef listb)
{
  TaggedRef ret;
  TaggedRef *out = &ret;

 loop:
  if (oz_isNil(lista)) {
    *out = listb;
    return ret;
  }

  if (oz_isNil(listb)) {
    *out = lista;
    return ret;
  }

  Assert(oz_isCons(lista) && oz_isCons(listb));

  TaggedRef a = head(lista);
  TaggedRef b = head(listb);
  TaggedRef newHead;

  switch (featureCmp(a,b)) {

  case 0:
    newHead = a;
    lista = tail(lista);
    listb = tail(listb);
    break;
  case -1:
    newHead = a;
    lista = tail(lista);
    break;
  case 1:
  default:
    newHead = b;
    listb = tail(listb);
    break;
  }

  LTuple *lt = new LTuple(newHead,makeTaggedNULL());
  *out = makeTaggedLTuple(lt);
  out = lt->getRefTail();
  goto loop;
}

TaggedRef oz_adjoin(SRecord *lrec, SRecord* hrecord)
{
  TaggedRef list1 = lrec->getArityList();
  TaggedRef list2 = hrecord->getArityList();

  // adjoin arities
  TaggedRef newArityList = merge(list1,list2);
  Arity *newArity = aritytable.find(newArityList);

  SRecord *newrec = SRecord::newSRecord(hrecord->getLabel(),newArity);

  // optimize case that right record completely overwrites left side.
  if (hrecord->isTuple()) {
    if (newArity->isTuple() && hrecord->getWidth() == newArity->getWidth()) {
      return SRecord::newSRecord(hrecord)->normalize();
    }
  } else if (newArity == hrecord->getRecordArity()) {
    return makeTaggedSRecord(SRecord::newSRecord(hrecord));
  }

  // copy left record to new record
  TaggedRef ar = list1;
  CHECK_DEREF(ar);
  while (oz_isCons(ar)) {
    TaggedRef a = head(ar);
    CHECK_DEREF(a);
    newrec->setFeature(a,lrec->getFeature(a));
    ar = tail(ar);
    CHECK_DEREF(ar);
  }

  TaggedRef har = list2;
  CHECK_DEREF(har);
  while (oz_isCons(har)) {
    TaggedRef a = head(har);
    CHECK_DEREF(a);
    newrec->setFeature(a,hrecord->getFeature(a));
    har = tail(har);
    CHECK_DEREF(har);
  }
  return newrec->normalize();
}

/*
 *      Construct a SRecord from SRecord old, and adjoin
 *      the pair (feature.value). This is the functionality of
 *      adjoinAt(old,feature,value) where old is a proper SRecord
 *      and feature is not contained in old.
 */

TaggedRef oz_adjoinAt(SRecord *rec, TaggedRef feature, TaggedRef value)
{
  if (rec->getIndex(feature) != -1) {
    SRecord *newrec = SRecord::newSRecord(rec);
    newrec->setFeature(feature,value);
    return newrec->normalize();
  } else {
    TaggedRef oldArityList = rec->getArityList();
    TaggedRef newArityList = insert(feature,oldArityList);
    Arity *arity = aritytable.find(newArityList);
    SRecord *newrec = SRecord::newSRecord(rec->getLabel(),arity);

    CHECK_DEREF(oldArityList);
    while (oz_isCons(oldArityList)) {
      TaggedRef a = head(oldArityList);
      CHECK_DEREF(a);
      newrec->setFeature(a,rec->getFeature(a));
      oldArityList = tail(oldArityList);
      CHECK_DEREF(oldArityList);
    }
    Assert(oz_isNil(oldArityList));
    newrec->setFeature(feature,value);
    return newrec->normalize();
  }
}

/*
 * This is the functionality of adjoinlist(old,proplist). We assume
 * that arityList is the list of the keys in proplist. arityList
 * is computed by the builtin in order to ease error handling.
 */
TaggedRef oz_adjoinList(SRecord *lrec,TaggedRef arityList,TaggedRef proplist)
{
  TaggedRef newArityList = insertlist(arityList,lrec->getArityList());
  Arity *newArity = aritytable.find(newArityList);

  SRecord *newrec = SRecord::newSRecord(lrec->getLabel(),newArity);
  Assert(fastlength(newArityList) == newrec->getWidth());

  TaggedRef ar = lrec->getArityList();
  CHECK_DEREF(ar);
  while (oz_isCons(ar)) {
    TaggedRef a = head(ar);
    CHECK_DEREF(a);
    newrec->setFeature(a,lrec->getFeature(a));
    ar = tail(ar);
    CHECK_DEREF(ar);
  }

  newrec->setFeatures(proplist);
  return newrec->normalize();
}


void SRecord::setFeatures(TaggedRef proplist)
{
  DEREF(proplist,_1,_2);
  CHECK_NONVAR(proplist);
  while (oz_isCons(proplist)) {
    TaggedRef pair = head(proplist);
    DEREF(pair,_3,_4);
    CHECK_NONVAR(pair);
    proplist = oz_deref(tail(proplist));
    CHECK_NONVAR(proplist);

    TaggedRef fea = oz_left(pair);
    DEREF(fea,_5,_6);
    CHECK_NONVAR(fea);

#ifdef DEBUG_CHECK
    if (!setFeature(fea, oz_right(pair))) {
      error("SRecord::setFeatures: improper feature: %s",
            toC(oz_left(pair)));
    }
#else
    setFeature(fea, oz_right(pair));
#endif

  }

  Assert(oz_isNil(proplist));
}

        /*------------------------------*/
        /*      Other Services.         */
        /*______________________________*/


Bool SRecord::setFeature(TaggedRef feature,TaggedRef value)
{
  CHECK_FEATURE(feature);

  int i = getIndex(feature);
  if ( i == -1 ) {
    return NO;
  }
  setArg(i,value);
  return OK;
}

TaggedRef SRecord::replaceFeature(TaggedRef feature,TaggedRef value)
{
  CHECK_FEATURE(feature);

  int i = getIndex(feature);
  if ( i == -1 ) {
    return makeTaggedNULL();
  }

  TaggedRef oldVal = args[i];
  if (!oz_isRef(oldVal) && oz_isVariable(oldVal)) {
    return oz_adjoinAt(this,feature,value);
  }
  setArg(i,value);
  return makeTaggedSRecord(this);
}

TaggedRef makeTupleArityList(int i)
{
  Assert(i>=0);
  TaggedRef out = nil();
  while (i>0) {
    out=cons(newSmallInt(i),out);
    i--;
  }
  return out;
}

/*
 * make LTuple to SRecord
 */
SRecord *makeRecord(TaggedRef t)
{
  if (oz_isSRecord(t)) return tagged2SRecord(t);
  Assert(oz_isLTuple(t));
  LTuple *lt=tagged2LTuple(t);
  SRecord *ret = SRecord::newSRecord(AtomCons,
                                     aritytable.find(makeTupleArityList(2)));
  ret->setArg(0,lt->getHead());
  ret->setArg(1,lt->getTail());
  return ret;
}


/*===================================================================
 * Space
 *=================================================================== */

SolveActor * Space::getSolveActor() {
  return ((SolveActor *) solve->getActor());
}

Bool Space::isFailed() {
  if (!solve) return OK;
  if (solve == (Board *) 1) return NO;
  return solve->isFailed();
}

Bool Space::isMerged() {
  if (solve == (Board *) 1) return OK;
  return NO;
}

/*===================================================================
 * Misc
 *=================================================================== */

DbgInfo *allDbgInfos = NULL;

PrTabEntry *PrTabEntry::allPrTabEntries = NULL;

void PrTabEntry::printPrTabEntries()
{
  PrTabEntry *aux = allPrTabEntries;
  unsigned int heapTotal = 0, callsTotal = 0, samplesTotal = 0;
  while(aux) {
    heapTotal    += aux->heapUsed;
    callsTotal   += aux->numCalled;
    samplesTotal += aux->samples;
    if (aux->numClosures || aux->numCalled || aux->heapUsed || aux->samples) {
      char *name = ozstrdup(toC(aux->printname)); // cannot have 2 toC in one line
      printf("%20.20s Created: %5u Called: %6u Heap: %5u B, Samples: %5u",
             name,aux->numClosures,aux->numCalled,aux->heapUsed,
             aux->samples);
      OZ_Term pos=aux->getPos();
      printf(" %s(%d,%d)\n",
             OZ_atomToC(OZ_getArg(pos,0)),
             OZ_intToC(OZ_getArg(pos,1)),
             OZ_intToC(OZ_getArg(pos,2)));
      delete name;
    }
    aux = aux->next;
  }

  printf("\n=============================================================\n\n");
  printf("    Total calls: %d, total heap: %d KB, samples: %d\n\n",
         callsTotal,heapTotal/KB,samplesTotal);
}


TaggedRef PrTabEntry::getProfileStats()
{
  TaggedRef ret      = nil();
  TaggedRef ps       = oz_atom("profileStats");
  TaggedRef samples  = oz_atom("samples");
  TaggedRef heap     = oz_atom("heap");
  TaggedRef calls    = oz_atom("calls");
  TaggedRef closures = oz_atom("closures");
  TaggedRef name     = oz_atom("name");
  TaggedRef line     = oz_atom("line");
  TaggedRef file     = oz_atom("file");

  TaggedRef list = cons(file,
                        cons(line,
                             cons(name,
                                  cons(samples,
                                       cons(heap,
                                            cons(calls,
                                                 cons(closures,nil())))))));
  Arity *arity = aritytable.find(sortlist(list,fastlength(list)));

  {
    PrTabEntry *aux = allPrTabEntries;
    while(aux) {
      if (aux->numClosures || aux->numCalled || aux->heapUsed || aux->samples) {
        SRecord *rec = SRecord::newSRecord(ps,arity);
        rec->setFeature(samples,oz_unsignedInt(aux->samples));
        rec->setFeature(calls,oz_unsignedInt(aux->numCalled));
        rec->setFeature(heap,oz_unsignedInt(aux->heapUsed));
        rec->setFeature(closures,oz_unsignedInt(aux->numClosures));
        OZ_Term pos = aux->getPos(); // mm2
        rec->setFeature(line,OZ_getArg(pos,1));
        rec->setFeature(name,aux->printname);
        rec->setFeature(file,OZ_getArg(pos,0));
        ret = cons(makeTaggedSRecord(rec),ret);
      }
      aux = aux->next;
    }
  }

  {
    OZ_CFunHeader *aux = OZ_CFunHeader::getFirst();
    TaggedRef noname = oz_atom("nofile");
    while(aux) {
      if (aux->getSamples() || aux->getCalls()) {
        SRecord *rec = SRecord::newSRecord(ps,arity);
        rec->setFeature(samples,oz_unsignedInt(aux->getSamples()));
        rec->setFeature(calls,oz_unsignedInt(aux->getCalls()));
        rec->setFeature(heap,oz_unsignedInt(aux->getHeap()));
        rec->setFeature(closures,oz_int(0));
        rec->setFeature(line,oz_int(1));
        rec->setFeature(name,oz_atom(builtinTab.getName((void *)(aux->getHeaderFunc()))));
        rec->setFeature(file,noname);
        ret = cons(makeTaggedSRecord(rec),ret);
      }
      aux = aux->getNext();
    }
  }

  return ret;
}


void PrTabEntry::profileReset()
{
  PrTabEntry *aux = allPrTabEntries;
  while(aux) {
    aux->numClosures = 0;
    aux->numCalled   = 0;
    aux->heapUsed    = 0;
    aux->samples     = 0;
    aux = aux->next;
  }
}

#include "opcodes.hh"
#include "codearea.hh"

void PrTabEntry::patchFileAndLine()
{
  Reg reg;
  int next;
  TaggedRef newpos, comment, predName;
  CodeArea::getDefinitionArgs(PC-sizeOf(DEFINITION),reg,next,newpos,predName);
  pos = newpos;
}

int featureEqOutline(TaggedRef a, TaggedRef b)
{
  Assert(a != b); // already check in featureEq

  return bigIntEq(a,b);
}

inline
Bool oz_isForeignPointer(TaggedRef term)
{
  term = oz_deref(term);
  return oz_isConst(term)
    && tagged2Const(term)->getType() == Co_Foreign_Pointer;
}

void* OZ_getForeignPointer(TaggedRef t)
{
  if (! oz_isForeignPointer(t)) {
    OZ_warning("Foreign pointer expected in OZ_getForeignPointer.\n Got 0x%x. Result unspecified.\n",t);
    return NULL;
  }
  return ((ForeignPointer*)tagged2Const(oz_deref(t)))->getPointer();
}

int OZ_isForeignPointer(TaggedRef t)
{
  return oz_isForeignPointer(oz_deref(t));
}

OZ_Term OZ_makeForeignPointer(void*p)
{
  ForeignPointer * fp = new ForeignPointer(p);
  return makeTaggedConst(fp);
}

ForeignPointer*
openForeignPointer(TaggedRef t)
{
  return (ForeignPointer*)tagged2Const(oz_deref(t));
}

TaggedRef oz_long(long i)
{
  return (new BigInt(i))->shrink();
}

TaggedRef oz_unsignedLong(unsigned long i)
{
  return (new BigInt(i))->shrink();
}

Board *oz_rootBoardOutline() { return oz_rootBoard(); }
