/*
 *  Authors:
 *    Michael Mehl (mehl@dfki.de)
 *    Tobias Mueller (tmueller@ps.uni-sb.de)
 *    Kostja Popow (popow@ps.uni-sb.de)
 *    Ralf Scheidhauer (Ralf.Scheidhauer@ps.uni-sb.de)
 *    Christian Schulte <schulte@ps.uni-sb.de>
 *
 *  Contributors:
 *    Denys Duchier (duchier@ps.uni-sb.de)
 *    Per Brand (perbrand@sics.se)
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

/****************************************************************************
 ****************************************************************************/

#include "gc.hh"
#include "board.hh"
#include "var_base.hh"
#include "fdomn.hh"
#include "dictionary.hh"
#include "os.hh"
#include "value.hh"
#include "codearea.hh"
#include "var_fd.hh"
#include "var_fs.hh"
#include "var_bool.hh"
#include "var_of.hh"
#include "var_ct.hh"
#include "var_future.hh"
#include "var_simple.hh"
#include "var_ext.hh"
#include "thr_int.hh"
#include "debug.hh"
#include "pointer-marks.hh"
#include "dpInterface.hh"
#include "gname.hh"
#include "interFault.hh"
#include "weakdict.hh"

// loeckelt (for big fsets)
#include "mozart_cpi.hh"

// hack alert: usage #pragma interface requires this
#ifdef OUTLINE
#define inline
#endif

/*
 * isCollecting: collection is running
 * isInGc:       garbage collector does garbage collection, otherwise it clones
 *
 */

Bool isCollecting = NO;
static Bool isInGc;

#ifdef CS_PROFILE
int32 * cs_copy_start = NULL;
int32 * cs_orig_start = NULL;
int     cs_copy_size  = 0;
#endif

/*
 * Forward reference
 */

static void gcCode(CodeArea *block);
static void gcCode(ProgramCounter PC);

/*
 * CHECKSPACE -- check if object is really copied from heap
 *   has as set of macros:
 *    INITCHECKSPACE - save pointer to from-space & print from-space
 *    NOTINTOSPACE   - assert not in to-space
 *    INTOSPACE      - assert in to-space
 * NOTE: this works only for chunk
 */

#ifdef DEBUG_GC

static MemChunks *fromSpace;

Bool inToSpace(void *p) {
  return (!isInGc || p==NULL || MemChunks::list->inChunkChain(p));
}

Bool notInToSpace(void *p) {
  return (!isInGc || p==NULL || !MemChunks::list->inChunkChain(p));
}

Bool inFromSpace(void *p) {
  return (!isInGc || p==NULL || fromSpace->inChunkChain(p));
}

void initCheckSpace() {
  fromSpace = MemChunks::list;
  DebugGCT(printf("FROM-SPACE:\n");
           fromSpace->print();)
}

void exitCheckSpace() {
  DebugGCT(printf("TO-SPACE:\n");
           MemChunks::list->print();)
}


#define GCDBG_INFROMSPACE(P)  Assert(inFromSpace(P))
#define GCDBG_NOTINTOSPACE(P) Assert(notInToSpace(P))
#define GCDBG_INTOSPACE(P)    Assert(inToSpace(P))
#define GCDBG_INITSPACE       initCheckSpace()
#define GCDBG_EXITSPACE       exitCheckSpace()

#else

#define GCDBG_INFROMSPACE(P)
#define GCDBG_NOTINTOSPACE(P)
#define GCDBG_INTOSPACE(P)
#define GCDBG_INITSPACE
#define GCDBG_EXITSPACE

#endif




/*
 * Allocate and copy memory blocks.
 *
 */

inline
void * gcReallocStatic(void * p, size_t sz) {
  // Use for blocks where size is known statically at compile time
  DebugCheck(sz%sizeof(int) != 0,
             OZ_error("gcReallocStatic: can only handle word sized blocks"););

  if (sz > 12) {
    return memcpy(freeListMalloc(sz), p, sz);
  } else {
    register int32 * frm = (int32 *) p;
    register int32 * to  = (int32 *) freeListMalloc(sz);

    switch(sz) {
    case 12:
      to[2]=frm[2];
    case 8:
      to[1]=frm[1];
    case 4:
      to[0]=frm[0];
      break;
    default:
      Assert(0);
    }

    return to;
  }
}


/*
 * The garbage collector uses an explicit recursion stack. The stack
 * items are references where garbage collection must continue.
 *
 * The items are tagged pointers, the tag gives which routine must
 * continue, whereas the pointer itself says with which entity.
 *
 */

enum TypeOfPtr {
  PTR_LTUPLE         =  0,  // 0000
  PTR_SRECORD        =  1,  // 0001
  PTR_BOARD          =  2,  // 0010
  PTR_CVAR           =  3,  // 0011
  PTR_CONSTTERM      =  4,  // 0100
  PTR_EXTENSION      =  5,  // 0101
  PTR_SUSPLIST       =  6,  // 0110
  PTR_UNUSED2        =  7,  // 0111
  PTR_LOCAL_SUSPLIST =  8,  // 1000
  // The remaining entries are reserved:
  //  the lower three bits encode, how many suspenlists are to
  //  be collected.
};


typedef TaggedRef TypedPtr;

class GcStack: public Stack {
public:
  GcStack() : Stack(1024, Stack_WithMalloc) {}
  ~GcStack() {}

  void push(void * ptr, TypeOfPtr type) {
    Stack::push((StackEntry) makeTaggedRef2p((TypeOfTerm) type, ptr));
  }

  void pushLocalSuspList(Board * bb, SuspList ** sl, int n) {
    Assert(n<8);
    Stack::ensureFree(2);
    Stack::push((StackEntry) bb, NO);
    Stack::push((StackEntry)
                makeTaggedRef2p((TypeOfTerm) (PTR_LOCAL_SUSPLIST | n),
                                (void *) sl), NO);
  }

  void recurse(void);

};

static GcStack gcStack;


/*
 * The copy trail: CpTrail
 *
 * During copying fields that are overwritten must be saved in order
 * to reestablish the space that has been copied.
 *
 */

class CpTrail: public Stack {
public:
  CpTrail() : Stack(1024, Stack_WithMalloc) {}
  ~CpTrail() {}

  void save(int * p) {
    // Save content and address
    ensureFree(2);
    push((StackEntry) *p, NO);
    push((StackEntry) p,  NO);
  }

  void unwind(void) {
    while (!isEmpty()) {
      int * p = (int *) pop();
      int   v = (int)   pop();
      *p = v;
    }
  }
};

static CpTrail cpTrail;


/****************************************************************************
 * GCMARK
 ****************************************************************************/

/*
 * set: only used in conjunction with the function setHeapCell ???
 */

/*
 * Check if object in from-space (elem) is already collected.
 *   Then return the forward pointer to to-space.
 */
#define CHECKCOLLECTED(elem,Type)  \
if (GCISMARKED(elem)) {return (Type) GCUNMARK(elem);}


/*
 * Write a marked forward pointer (pointing into the to-space)
 * into a structure in the from-space.
 *
 *  If mode is IN_GC, store value in cell ptr only;
 *  else save cell at ptr and also store in this cell.
 *
 */
inline
void storeFwdMode(Bool isInGc, int32* fromPtr, void *newValue,
                  Bool domark=OK) {
  if (!isInGc)
    cpTrail.save(fromPtr);

  GCDBG_INFROMSPACE(fromPtr);

  *fromPtr = domark ? GCMARK(newValue) : ToInt32(newValue);
}

inline
void storeFwd(int32* fromPtr, void *newValue, Bool domark=OK) {
  storeFwdMode(isInGc, fromPtr, newValue, domark);
}

#define storeFwdField(d,t) \
  storeFwd((int32*) d->gcGetMarkField(), t, NO); d->gcMark(t);

//*****************************************************************************
//               Functions to gc external references into heap
//*****************************************************************************

class ExtRefNode;
static ExtRefNode *extRefs = NULL;

class ExtRefNode {
public:
  USEHEAPMEMORY;

  TaggedRef *elem;
  ExtRefNode *next;

  ExtRefNode(TaggedRef *el, ExtRefNode *n): elem(el), next(n){ Assert(elem); }

  void remove()                  { elem = NULL; }
  ExtRefNode *add(TaggedRef *el) { return new ExtRefNode(el,this); }

  ExtRefNode *gc()
  {
    ExtRefNode *aux = this;
    ExtRefNode *ret = NULL;
    while(aux) {
      if (aux->elem) {
        ret = new ExtRefNode(aux->elem,ret);
        OZ_collectHeapTerm(*ret->elem,*ret->elem);
      }
      aux = aux->next;
    }
    return ret;
  }


  ExtRefNode *protect(TaggedRef *el)
  {
    Assert(oz_isRef(*el) || !oz_isVariable(*el));
    Assert(!find(el));
    return add(el);
  }


  Bool unprotect(TaggedRef *el)
  {
    Assert(el);
    ExtRefNode *aux = extRefs;
    while(aux) {
      if (aux->elem==el) {
        aux->remove();
        return OK;
      }
      aux = aux->next;
    }
    return NO;
  }


  ExtRefNode *find(TaggedRef *el)
  {
    Assert(el);
    ExtRefNode *aux = extRefs;
    while(aux) {
      if (aux->elem==el)
        break;
      aux = aux->next;
    }
    return aux;
  }
};


inline
Bool needsCollection(Literal *l)
{
  if (l->isAtom()) return NO;
  Name *nm = (Name*) l;
  return nm->isOnHeap();
}


Bool needsNoCollection(TaggedRef t)
{
  Assert(t != makeTaggedNULL());

  TypeOfTerm tag = tagTypeOf(t);
  return isSmallIntTag(tag) ||
         isLiteralTag(tag) && !needsCollection(tagged2Literal(t));
}


Bool oz_protect(TaggedRef *ref)
{
  extRefs = extRefs->protect(ref);
  return OK;
}

/* protect a ref, that will never change its initial value
 *  --> no need to remember it, if it's a small int or atom
 */
Bool oz_staticProtect(TaggedRef *ref)
{
  if (needsNoCollection(*ref))
    return OK;

  return oz_protect(ref);
}

Bool oz_unprotect(TaggedRef *ref)
{
  ExtRefNode *aux = extRefs->find(ref);

  if (aux == NULL)
    return NO;

  aux->remove();
  return OK;
}

/*
 * Garbage collection needs to be aware of certain objects, e.g.,
 * since these objects store references into the heap. The gc-routine
 * of `GCMeManager' is called after all collection has been done, such
 * that the individual gc routines of the objects can avoid copying
 * references that are only established by themselves (in other words,
 * memory leaks can be avoided).
 */

#ifdef NEW_NAMER
GCMeManager * GCMeManager::_head;
#endif

/*
 * The variable copying stack: VarFix
 *
 * When during garbage collection or during copying a variable V is
 * encountered that has not been collected so far and V is not direct,
 + that is, V has been reached by a reference chain, V cannot be copied
 * directly.
 *
 * So, push the location of the reference on VarFix and replace its content
 * by a reference to the old variable, as to shorten the ref-chain.
 *
 * Later V might be reached directly, that fixes V's location. After
 * collection has finished, VarFix tracks this new location to
 * and fixes the occurence on VarFix.
 *
 */

class VarFix: public Stack {
public:
  VarFix() : Stack(1024, Stack_WithMalloc) {}
  ~VarFix() {}

  void defer(TaggedRef * var, TaggedRef * ref) {
    Assert(var);
    Stack::push((StackEntry) ref);
    *ref = makeTaggedRef(var);
  }

  void fix(void);

};

static VarFix varFix;


/****************************************************************************
 * Collect all types of terms
 ****************************************************************************/

// This procedure derefences cluster chains and collects only the object at
// the end of such a chain.


#define RAGCTag (1<<31)

inline
Bool refsArrayIsMarked(RefsArray r) {
  return (r[-1]&RAGCTag);
}

inline
void refsArrayMark(RefsArray r, void *ptr) {
  storeFwd((int32*)&r[-1],ToPointer(ToInt32(ptr)|RAGCTag),NO);
}

inline
RefsArray refsArrayUnmark(RefsArray r) {
  return (RefsArray) ToPointer(r[-1]&(~(RAGCTag)|mallocBase));
}


// Structure of type 'RefsArray' (see ./tagged.h)
// r[0]..r[n-1] data
// r[-1] gc tag set --> has already been copied

inline
RefsArray gcRefsArray(RefsArray r) {
  if (r == NULL)
    return r;

  GCDBG_NOTINTOSPACE(r);

  if (refsArrayIsMarked(r)) {
    return refsArrayUnmark(r);
  }

  Assert(!isFreedRefsArray(r));

  int sz = getRefsArraySize(r);

  RefsArray aux = allocateRefsArray(sz,NO);

  refsArrayMark(r,aux);

  OZ_collectHeapBlock(r, aux, sz);

  return aux;
}

inline
Abstraction *gcAbstraction(Abstraction *a) {
  return a ? ((Abstraction *) a->gcConstTerm()) : a;
}

/*
 * gcIsAlive(bb):
 *   bb is marked collected, not failed and all parents are alive
 *
 */

#ifdef DEBUG_CHECK

inline
int NEEDSCOPYING(Board * bb) {
 Assert(isInGc ? !bb->hasMarkOne() : 1);
 return !bb->hasMarkOne();
}

#else

#define NEEDSCOPYING(bb) (!(bb)->hasMarkOne())

#endif


inline
Bool Board::gcIsMarked(void) {
  return hasMarkTwo();
}

inline
Bool Board::gcIsAlive() {
  Board * s = this;

  do {
    int t = s->getTag();

    if (t & (BoTag_MarkOne | BoTag_MarkOne))
      return OK;

    if (t == BoTag_Root) {
      return OK;
    }

    if (t == BoTag_Failed) {
      return NO;
    }

    s = s->getParentInternal();
  } while (1);

}


inline
void Board::gcMark(Board * fwd) {
  Assert(!gcIsMarked());
  if (!isInGc)
    cpTrail.save((int32 *) &parentAndFlags);
  parentAndFlags.set((void *) fwd, BoTag_MarkTwo);
}

inline
Board * Board::gcGetFwd(void) {
  Assert(gcIsMarked());
  return getParentInternal();
}

inline
Board * Board::gcBoard() {
  GCDBG_INFROMSPACE(this);

  // Do not clone a space above or collect a space above root ;-(
  Assert(this && !hasMarkOne());

  Board * bb = derefBoard();

  Assert(bb);

  if (bb->gcIsMarked())
    return bb->gcGetFwd();

  Assert(bb->gcIsAlive());

  Board *ret = (Board *) oz_hrealloc(bb, sizeof(Board));

  gcStack.push(ret,PTR_BOARD);

  bb->gcMark(ret);

  return ret;
}

void dogcGName(GName *gn) {
  if (gn && isInGc)
    gcGName(gn);
}

/*
 * Literals:
 *   3 cases: atom, optimized name, dynamic name
 *   only dynamic names need to be copied
 */

inline
Name *Name::gcName() {
  CHECKCOLLECTED(homeOrGName, Name *);
  GName * gn = NULL;

  if (hasGName()) {
    gn = getGName1();
  }

  if (isInGc && isOnHeap() || !isInGc && !getBoardInternal()->hasMarkOne()) {

    Name *aux = (Name*) gcReallocStatic(this,sizeof(Name));

    storeFwd(&homeOrGName, aux);

    if (gn) {
      dogcGName(gn);
    } else {
      aux->homeOrGName =
        ToInt32(((Board*)ToPointer(aux->homeOrGName))->gcBoard());
    }

    return aux;

  } else {
    dogcGName(gn);
    return this;
  }
}

inline
Bool Name::gcIsMarked() { return GCISMARKED(homeOrGName); }

inline
Literal *Literal::gc() {
  if (isAtom()) return this;
  return ((Name*) this)->gcName();
}

inline
Object *Object::gcObjectInline() {
  return this ? ((Object *) gcConstTerm()) : this;
}

Object *Object::gcObject() {
  return gcObjectInline();
}


/*
 *  We reverse the order of the list, but this should be no problem.
 *
 * kost@ : ... in any case, this is complaint with the
 * 'The Definition of Kernel Oz';
 *
 */

inline
void gcSuspList(SuspList ** sl) {
  gcStack.push(sl, PTR_SUSPLIST);
}

inline
void gcLocalSuspList(Board * bb, SuspList ** sl, int n) {
  gcStack.pushLocalSuspList(bb, sl, n);
}

inline
void OzVariable::gcMark(Bool isInGc, TaggedRef * fwd) {
  Assert(!gcIsMarked());
  if (!isInGc)
    cpTrail.save((int32 *) &suspList);
  suspList = (SuspList *) MarkPointer(fwd,1);
}

inline
void OZ_FiniteDomainImpl::gc(void) {
  copyExtension();
}

inline
void OzFDVariable::gc(void) {
  ((OZ_FiniteDomainImpl *) &finiteDomain)->gc();

  gcLocalSuspList(getBoardInternal(), &(fdSuspList[0]), fd_prop_any);
}

inline
void OzFSVariable::gc(void) {

#ifdef BIGFSET
  _fset.copyExtension();
#endif

  gcLocalSuspList(getBoardInternal(), &(fsSuspList[0]), fs_prop_any);
}

inline
void OzCtVariable::gc(void) {
  // gc suspension lists
  int noOfSuspLists = getNoOfSuspLists();

  // copy
  SuspList ** new_susp_lists = (SuspList **)
    heapMalloc(sizeof(SuspList *) * noOfSuspLists);
  for (int i = noOfSuspLists; i--; )
    new_susp_lists[i] = _susp_lists[0];
  _susp_lists = new_susp_lists;
  // collect
  gcLocalSuspList(getBoardInternal(), _susp_lists, noOfSuspLists);

}

inline
void OzCtVariable::gcRecurse(void)
{
  // constraint (must go in `gcRecurse' since it may contain recursion
  _constraint = _constraint->copy();
}

const int varSizes[] = {
  sizeof(ExtVar),         // OZ_VAR_EXT
  sizeof(SimpleVar),      // OZ_VAR_SIMPLE
  sizeof(Future),         // OZ_VAR_FUTURE
  sizeof(OzBoolVariable), // OZ_VAR_BOOL
  sizeof(OzFDVariable),   // OZ_VAR_FD
  sizeof(OzOFVariable),   // OZ_VAR_OF
  sizeof(OzFSVariable),   // OZ_VAR_FS
  sizeof(OzCtVariable),   // OZ_VAR_CT
};


inline
OzVariable * OzVariable::gcVarInline(void) {
  GCDBG_INFROMSPACE(this);

  Assert(!gcIsMarked());
  Assert(!isTrailed());

  TypeOfVariable t = getType();

  OzVariable * to;

  Board * bb = getBoardInternal()->gcBoard();

  if (t != OZ_VAR_EXT) {

    to = (OzVariable *) oz_hrealloc(this,varSizes[t]);

    to->setHome(bb);
    // Only after board is collected!
    gcSuspList(&(to->suspList));

    switch (t){
    case OZ_VAR_FD:
      ((OzFDVariable *) to)->gc();
      return to;
    case OZ_VAR_FS:
      ((OzFSVariable *) to)->gc();
      return to;
    case OZ_VAR_BOOL:
      return to;
    case OZ_VAR_CT:
      ((OzCtVariable*) to)->gc();
      break;
    default:
      break;
    }

  } else {
    to = ((ExtVar *) this)->gcV();
    to->setHome(bb);
    gcSuspList(&(to->suspList));
  }

  gcStack.push(to, PTR_CVAR);

  return to;

}

OzVariable * OzVariable::gcVar(void) {
  return gcVarInline();
}

inline
DynamicTable * DynamicTable::gc(void) {
  Assert(isPwrTwo(size));

  // Copy the table:
  DynamicTable * to = (DynamicTable *) heapMalloc((size-1)*sizeof(HashElement)
                                                  + sizeof(DynamicTable));
  to->numelem = numelem;
  to->size    = size;

  HashElement * ft = table;
  HashElement * tt = to->table;

  for (dt_index i=size; i--; )
    if (ft[i].ident) {
      if (ft[i].value) {
        OZ_collectHeapBlock(&(ft[i].ident), &(tt[i].ident), 2);
      } else {
        OZ_collectHeapTerm(ft[i].ident, tt[i].ident);
        tt[i].value = makeTaggedNULL();
      }
    } else {
      tt[i].ident = makeTaggedNULL();
      tt[i].value = makeTaggedNULL();
    }

  return to;
}

inline
void OzOFVariable::gcRecurse(void) {
  OZ_collectHeapTerm(label,label);
  // Update the pointer in the copied block:
  dynamictable=dynamictable->gc();
}


void OzVariable::gcVarRecurse(void) {

  switch (getType()) {
  case OZ_VAR_SIMPLE:  ((SimpleVar *)this)->gcRecurse(); break;
  case OZ_VAR_FUTURE:  ((Future *)this)->gcRecurse(); break;
  case OZ_VAR_BOOL:    Assert(0); break;
  case OZ_VAR_FD:      Assert(0); break;
  case OZ_VAR_OF:      ((OzOFVariable*)this)->gcRecurse(); break;
  case OZ_VAR_FS:      Assert(0); break;
  case OZ_VAR_CT:      ((OzCtVariable*)this)->gcRecurse(); break;
  case OZ_VAR_EXT:     ((ExtVar *)this)->gcRecurseV(); break;
  default:
    Assert(0);
  }

}


/*
 * Float
 * WARNING: the value field of floats has no bit left for a gc mark
 *   --> copy every float !! so that X=Y=1.0 --> X=1.0, Y=1.0
 */

inline
Float *Float::gc() {
  Assert(isInGc);

  return newFloat(value);
}


inline
FSetValue * FSetValue::gc(void) {
  Assert(isInGc);

#ifdef BIGFSET
  FSetValue *retval = (FSetValue *) oz_hrealloc(this, sizeof(FSetValue));
  retval->_IN.copyExtension();
  return retval;
#else
  return (FSetValue *) oz_hrealloc(this, sizeof(FSetValue));
#endif
}


BigInt * BigInt::gc() {
  Assert(isInGc);
  BigInt *ret = new BigInt();
  mpz_set(&ret->value,&value);
  return ret;
}

inline
void Script::gc() {

  Assert(sizeof(Equation) == 2 * sizeof(TaggedRef));

  if (size > 0) {

    Equation * to = (Equation *) heapMalloc(size * sizeof(Equation));

#ifdef DEBUG_CHECK
    for (int i = size; i--; ){
      //  This is the very useful consistency check.
      //  'Equations' with non-variable at the left side are figured out;
      TaggedRef auxTerm = eqs[i].left;
      TaggedRef *auxTermPtr;
      if (!isInGc && oz_isRef(auxTerm)) {
        do {
          if (GCISMARKED(auxTerm)) {
            auxTerm = ToInt32(GCUNMARK(auxTerm));
            continue;
          }
          if (oz_isRef (auxTerm)) {
            auxTermPtr = tagged2Ref (auxTerm);
            auxTerm = *auxTermPtr;
            continue;
          }
          if (oz_isVariable (auxTerm))
            break;   // ok;
          OZ_error ("non-variable is found at left side in Script");
        } while (1);
      }
    }
#endif

    OZ_collectHeapBlock((TaggedRef *) eqs, (TaggedRef *) to,
                        size * 2);
    eqs = to;

  }

}


/*
 *  Thread items methods;
 *
 */

/* collect LTuple, SRecord */

inline
LTuple * LTuple::gc() {
  // Does basically nothing, the real stuff is in gcRecurse

  GCDBG_INFROMSPACE(this);

  if (GCISMARKED(args[0]))
    return (LTuple *) GCUNMARK(args[0]);

  LTuple * to = (LTuple *) heapMalloc(sizeof(LTuple));

  // Save the content
  to->args[0] = args[0];

  // Do not store foreward! gcRecurse takes care of this!
  args[0] = GCMARK(to->args);

  gcStack.push(this, PTR_LTUPLE);

  return to;
}

inline
SRecord *SRecord::gcSRecord() {
  Assert(this);

  CHECKCOLLECTED(label, SRecord *);

  int len = (getWidth()-1)*sizeof(TaggedRef)+sizeof(SRecord);

  SRecord *ret = (SRecord*) heapMalloc(len);

  ret->label       = label;
  ret->recordArity = recordArity;

  storeFwd((int32*)&label, ret);

  gcStack.push(this, PTR_SRECORD);

  return ret;
}


OZ_Propagator * OZ_Propagator::gc(void) {
  OZ_Propagator * to = (OZ_Propagator *) oz_hrealloc(this, sizeOf());

  return to;
}

ForeignPointer * ForeignPointer::gc(void) {
  ForeignPointer * ret =
    (ForeignPointer*) gcReallocStatic(this,sizeof(ForeignPointer));
  ret->ptr = ptr;

  storeFwdField(this, ret);
  return ret;
}

// ===================================================================
// Extension

TaggedRef gcExtension(TaggedRef term)
{
  OZ_Extension *ex = oz_tagged2Extension(term);

  Assert(ex);

  // hack alert: write forward into vtable!
  if ((*(int32*)ex)&1) {
    return oz_makeTaggedExtension((OZ_Extension *)ToPointer((*(int32*)ex)&~1));
  }

  Board *bb=(Board*)(ex->__getSpaceInternal());

  if (bb) {
    Assert(bb->gcIsAlive());
    if (!NEEDSCOPYING(bb))
      return term;
  }

  OZ_Extension *ret = ex->gcV();

  if (bb)
    ret->__setSpaceInternal(bb);

  gcStack.push(ret,PTR_EXTENSION);

  int32 *fromPtr = (int32*)ex;

  if (!isInGc)
    cpTrail.save(fromPtr);

  *fromPtr = ToInt32(ret)|1;

  return oz_makeTaggedExtension(ret);
}

void gcExtensionRecurse(OZ_Extension *ex)
{
  Board *bb=(Board*) (ex->__getSpaceInternal());

  if (bb)
    ex->__setSpaceInternal(bb->gcBoard());

  ex->gcRecurseV();
}

// ===================================================================
// Weak Dictionaries

class WeakStack : public Stack {
public:
  WeakStack() : Stack(64,Stack_WithMalloc) {}
  ~WeakStack() {}
  void push(OZ_Term fut,OZ_Term val) {
    Stack::push((StackEntry)fut);
    Stack::push((StackEntry)val);
  }
  void pop(OZ_Term& fut,OZ_Term& val) {
    val = (OZ_Term) Stack::pop();
    fut = (OZ_Term) Stack::pop();
  }
  void recurse(void);
};

static WeakStack weakStack;

void WeakStack::recurse(void)
{
  OZ_Term fut,val;
  while (!isEmpty()) {
    pop(fut,val);
    DEREF(fut,ptr,_);
    oz_bindFuture(ptr,val);
  }
}

// isNowMarked(t) returns true iff
//      t is a marked name, extension, const, var
// the logic is adapted from gcTagged(TaggedRef&,TaggedRef&)
// and simplified according to a suggestion by Christian.
inline int isNowMarked(OZ_Term t)
{
 redo:
  switch (tagTypeOf(t)) {
  case REF:
  case REFTAG2:
  case REFTAG3:
  case REFTAG4:
    {
      TaggedRef * ptr;
      do {
        ptr = tagged2Ref(t);
        t   = *ptr;
      } while (oz_isRef(t));
      goto redo;
    }
  case GCTAG     : goto RETURN_YES;
  case SMALLINT  : goto RETURN_NO;
  case FSETVALUE : goto RETURN_NO;
  case LITERAL   : DO_LITERAL:
    {
      Literal * lit = tagged2Literal(t);
      if (lit->isAtom()) return 1;
      else return ((Name*)lit)->gcIsMarked();
    }
  case EXT       : DO_EXT: return ((*(int32*)oz_tagged2Extension(t))&1);
  case LTUPLE    : goto RETURN_NO;
  case SRECORD   : goto RETURN_NO;
  case OZFLOAT   : goto RETURN_NO;
  case OZCONST   : DO_OZCONST: return (tagged2Const(t)->gcIsMarked());
  case UNUSED_VAR: goto IMPOSSIBLE;
  case UVAR      : goto IMPOSSIBLE;
  case CVAR      : DO_CVAR: return tagged2CVar(t)->gcIsMarked();
  default        : Assert(0);
  }
  return 0;
 RETURN_YES: return 1;
 IMPOSSIBLE: Assert(0);
 RETURN_NO : return 0;
}

void WeakDictionary::gcRecurseV(void) {
  if (stream) OZ_collectHeapTerm(stream, stream);
}

void WeakDictionary::weakGC()
{
  int numelem = table->numelem;
  // go through the table and finalize each entry whose value is not
  // marked.  also clear these entries.
  OZ_Term newstream = 0;
  OZ_Term list = 0;
  int count = 0;
  for (dt_index i=table->size; i--; ) {
    TaggedRef t = table->getValue(i);
    if (t!=0 && !isNowMarked(t)) {
      numelem--;
      if (stream) {
        if (!list) newstream=list=oz_newFuture(oz_rootBoard());
        OZ_Term k = table->getKey(i);
        // collect key and value
        OZ_collectHeapTerm(t,t);
        OZ_collectHeapTerm(k,k);
        list = oz_cons(oz_pair2(k,t),list);
        count++;
      }
      table->clearValue(i);
    }
  }
  // then update the stream
  if (stream && list) {
    weakStack.push(stream,list);
    stream=newstream;
  }
  // finally collect the table
  DynamicTable * frm = table;
  table = DynamicTable::newDynamicTable(numelem);
  for (dt_index i=frm->size;i--;) {
    OZ_Term v = frm->getValue(i);
    if (v!=0) {
      OZ_Term k = frm->getKey(i);
      OZ_collectHeapTerm(k,k);
      OZ_collectHeapTerm(v,v);
      put(k,v);
    }
  }
}

// ===================================================================
// Finalization

extern OZ_Term guardian_list;
extern OZ_Term finalize_list;
extern OZ_Term finalize_handler;

void gc_finalize()
{
  // go through the old guardian list
  OZ_Term old_guardian_list = guardian_list;
  guardian_list = finalize_list = oz_nil();
  if (old_guardian_list==0) return;
  while (!oz_isNil(old_guardian_list)) {
    OZ_Term pair = oz_head(old_guardian_list);
    old_guardian_list = oz_tail(old_guardian_list);
    OZ_Term obj = oz_head(pair);
    switch (tagTypeOf(obj)) {
    case EXT    :
      // same check as Michael's hack in gcExtension
      if ((*(int32*)oz_tagged2Extension(obj))&1)
        // reachable through live data
        guardian_list = oz_cons(pair,guardian_list);
      else
        // unreachable
        finalize_list = oz_cons(pair,finalize_list);
      break;
    case OZCONST:
      if (tagged2Const(obj)->gcIsMarked())
        // reachable through live data
        guardian_list = oz_cons(pair,guardian_list);
      else
        // unreachable
        finalize_list = oz_cons(pair,finalize_list);
      break;
    default     :
      Assert(0);
    }
  }
  // gc both these list normally.
  // since these lists have been freshly consed in the new half space
  // this simply means to go through both and gc the pairs
  // in the head of each cons
  for (OZ_Term l=guardian_list;!oz_isNil(l);l=oz_tail(l)) {
    LTuple *t = tagged2LTuple(l);
    OZ_collectHeapTerm(*t->getRefHead(),*t->getRefHead());
  }
  for (OZ_Term l1=finalize_list;!oz_isNil(l1);l1=oz_tail(l1)) {
    LTuple *t = tagged2LTuple(l1);
    OZ_collectHeapTerm(*t->getRefHead(),*t->getRefHead());
  }
  // if the finalize_list is not empty, we must create a new
  // thread (at top level) to effect the finalization phase
  if (!oz_isNil(finalize_list)) {
    Thread* thr = oz_newThreadInject(oz_rootBoard());
    thr->pushCall(finalize_handler,finalize_list);
    finalize_list = oz_nil();
  }
}


inline
void gcTagged(TaggedRef & frm, TaggedRef & to) {
#ifdef DEBUG_GC
  Assert(!isInGc || !fromSpace->inChunkChain(&to));
#endif

  TaggedRef aux = frm;

  switch (tagTypeOf(aux)) {

  case REF:
    Assert(aux);
  case REFTAG2:
  case REFTAG3:
  case REFTAG4:
    {
      TaggedRef * aux_ptr;

      do {
        aux_ptr = tagged2Ref(aux);
        aux     = *aux_ptr;
      } while (oz_isRef(aux));

      switch (tagTypeOf(aux)) {
        // The following cases never occur, but to allow for better code
      case REF: case REFTAG2: case REFTAG3: case REFTAG4: {}
        // All the following jumps are resolved to jumps in the switch-table!
      case GCTAG:     goto DO_GCTAG;
      case SMALLINT:  goto DO_SMALLINT;
      case FSETVALUE: goto DO_FSETVALUE;
      case LITERAL:   goto DO_LITERAL;
      case EXT:       goto DO_EXT;
      case LTUPLE:    goto DO_LTUPLE;
      case SRECORD:   goto DO_SRECORD;
      case OZFLOAT:   goto DO_OZFLOAT;
      case OZCONST:   goto DO_OZCONST;

      case UNUSED_VAR: // FUT

      case UVAR: // non-direct var: delay collection
        {
          Board * bb = tagged2VarHome(aux);

          if (!NEEDSCOPYING(bb)) {
            to = makeTaggedRef(aux_ptr);
            return;
          }

          Assert(isInGc || !bb->hasMarkOne());

          bb = bb->gcBoard();

          Assert(bb);
          varFix.defer(aux_ptr, &to);
          return;
        }

      case CVAR:
        {
          OzVariable * cv = tagged2CVar(aux);

          if (cv->gcIsMarked()) {
            Assert(tagTypeOf(*(cv->gcGetFwd())) == CVAR);
            to = makeTaggedRef(cv->gcGetFwd());
          } else if (NEEDSCOPYING(cv->getBoardInternal())) {
            OzVariable *new_cv=cv->gcVarInline();

            Assert(new_cv);

            TaggedRef * var_ptr = newTaggedCVar(new_cv);
            to = makeTaggedRef(var_ptr);
            cv->gcMark(isInGc, var_ptr);
          } else {
            to = makeTaggedRef(aux_ptr);
          }
          return;
        }

      }

      Assert(NO);
    }

  case GCTAG: DO_GCTAG:
    to = makeTaggedRef((TaggedRef*) GCUNMARK(aux));
    // This can lead to not shortened ref chains together with
    // the CONS forwarding: if a CONS cell is collected, then every
    // reference to the first element becomes a ref. May try this:
    // if (!isVar(*to)) to=deref(to); (no, cycles... CS)
    return;

  case SMALLINT: DO_SMALLINT:
    to = aux;
    return;

  case FSETVALUE: DO_FSETVALUE:
    if (isInGc) {
      to = makeTaggedFSetValue(((FSetValue *) tagged2FSetValue(aux))->gc());
    } else {
      to = aux;
    }
    return;

  case LITERAL: DO_LITERAL:
      to = makeTaggedLiteral(tagged2Literal(aux)->gc());
      return;

  case EXT: DO_EXT:
    to = gcExtension(aux);
    return;

  case LTUPLE: DO_LTUPLE:
    to = makeTaggedLTuple(tagged2LTuple(aux)->gc());
    return;

  case SRECORD: DO_SRECORD:
    to = makeTaggedSRecord(tagged2SRecord(aux)->gcSRecord());
    return;

  case OZFLOAT: DO_OZFLOAT:
    if (isInGc) {
      to = makeTaggedFloat(tagged2Float(aux)->gc());
    } else {
      to = aux;
    }
    return;

  case OZCONST: DO_OZCONST:
    to = makeTaggedConst(tagged2Const(aux)->gcConstTerm());
    return;

  case UNUSED_VAR:  // FUT
    Assert(0); OZ_error("impossible");
    return;
  case UVAR: // direct var
    {
      Board * bb = tagged2VarHome(aux);

      Assert(bb);

      if (NEEDSCOPYING(bb)) {
        Assert(isInGc || !bb->hasMarkOne());
        bb = bb->gcBoard();
        Assert(bb);
        to = makeTaggedUVar(bb);
      } else {
        frm = makeTaggedRef(&to);
        to  = aux;
      }
      storeFwdMode(isInGc, (int32 *)&frm, &to);
    }
    return;

  case CVAR: // direct cvar
    {
      OzVariable * cv = tagged2CVar(aux);

      if (cv->gcIsMarked()) {
        Assert(tagTypeOf(*(cv->gcGetFwd())) == CVAR);
        to = makeTaggedRef(cv->gcGetFwd());
      } else if (NEEDSCOPYING(cv->getBoardInternal())) {
        to = makeTaggedCVar(cv->gcVarInline());
        cv->gcMark(isInGc, &to);
      } else {
        // We cannot copy the variable, but we have already copied
        // their taggedref, so we change the original variable to a ref
        // of the copy.
        // After pushing on the update stack the
        // the original variable is replaced by a reference!
        Assert(!isInGc);
        frm = makeTaggedRef(&to);
        to  = aux;
        storeFwdMode(NO, (int32*) &frm, &to);
      }

    }
    return;

  }
}

// extern
void OZ_collect(OZ_Term *to) {
  OZ_collectHeapTerm(*to,*to);
}

void OZ_collectHeapBlock(TaggedRef * frm, TaggedRef * to, int sz) {
  for (int i=sz; i--; )
    gcTagged(frm[i], to[i]);
}

//*****************************************************************************
//                               AM::gc
//*****************************************************************************


// This method is responsible for the heap garbage collection of the
// abstract machine, ie that all entry points into heap are properly
// treated and references to variables are properly updated
void AM::gc(int msgLevel) {

  (*gcFrameToProxy)();

  isCollecting = OK;
  isInGc       = OK;

  ozstat.initGcMsg(msgLevel);

  MemChunks * oldChain = MemChunks::list;

#ifndef NEW_NAMER
  oz_varCleanup();  /* drop bound variables */
#endif

  GCDBG_INITSPACE;

  initMemoryManagement();

  for (int j=NumberOfXRegisters; j--; )
    xRegs[j] = taggedVoidValue;

  Assert(trail.getUsed() == 1);
  Assert(cachedSelf==0);
  Assert(ozstat.currAbstr==NULL);
  Assert(_inEqEq==FALSE);
  Assert(_rootBoard);

  _rootBoard = _rootBoard->gcBoard();   // must go first!
  setCurrent(_currentBoard->gcBoard());

  aritytable.gc ();
  threadsPool.gc();

  // mm2: Assert(isEmptySuspendVarList());
  emptySuspendVarList();

  if (defaultExceptionHdl)
    OZ_collectHeapTerm(defaultExceptionHdl,defaultExceptionHdl);
  OZ_collectHeapTerm(debugStreamTail,debugStreamTail);

  CodeArea::gcCodeAreaStart();
  PrTabEntry::gcPrTabEntries();
  extRefs = extRefs->gc();

  OZ_collectHeapTerm(finalize_handler,finalize_handler);
  gcStack.recurse();
  gc_finalize();
  gcWeakDictionaries();
  gcDeferWatchers();
  (*gcPerdioRoots)();
  gcStack.recurse();

  (*gcBorrowTableUnusedFrames)();
  gcStack.recurse();

#ifdef NEW_NAMER
  GCMeManager::gc();
  gcStack.recurse();
#endif
  weakStack.recurse();          // must come after namer gc
// -----------------------------------------------------------------------
// ** second phase: the reference update stack has to checked now
  varFix.fix();
  Assert(gcStack.isEmpty());


  GT.gcGNameTable();
  //   MERGECON gcPerdioFinal();
  gcSiteTable();
  (*gcPerdioFinal)();
  Assert(gcStack.isEmpty());

  GCDBG_EXITSPACE;

  CodeArea::gcCollectCodeBlocks();
  AbstractionEntry::freeUnusedEntries();

  oldChain->deleteChunkChain();

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
//                garbage collection is finished here

  cachedStack = NULL;

  ozstat.printGcMsg(msgLevel);

  isCollecting = NO;
} // AM::gc


/*
 * After collection has finished, update variable references
 *
 */
void VarFix::fix(void) {

  if (isEmpty())
    return;

  do {
    TaggedRef * to = (TaggedRef *) pop();

    Assert(oz_isRef(*to));

    TaggedRef * aux_ptr = tagged2Ref(*to);
    TaggedRef   aux     = *aux_ptr;

    TaggedRef * to_ptr  =
      (tagTypeOf(aux) == UVAR) ?
      newTaggedUVar(tagged2VarHome(aux)->derefBoard()->gcGetFwd()) :
      (TaggedRef *) GCUNMARK(aux);

    Assert(tagTypeOf(aux) == UVAR || tagTypeOf(aux) == GCTAG);

    *to = makeTaggedRef(to_ptr);
    storeFwd((int32 *) aux_ptr, to_ptr);

  } while (!isEmpty());

}


/*
 *   AM::copyTree () routine (for search capabilities of the machine)
 *
 */

#ifdef CS_PROFILE
static Bool across_redid = NO;
#endif

/*
 * Before copying all spaces but the space to be copied get marked.
 *
 * Important: even committed boards must be marked, since the globality
 * test does not do a dereference!
 *
 */

void Board::setGlobalMarks(void) {
  Assert(!isRoot());

  Board * b = this;

  do {
    b = b->getParentInternal();
    Assert(!b->hasMarkOne());
    b->setMarkOne();
  } while (!b->isRoot());

}

/*
 * Purge marks after copying
 */

void Board::unsetGlobalMarks(void) {
  Assert(!isRoot());

  Board * b = this;

  do {
    b = b->getParentInternal();
    Assert(b->hasMarkOne());
    b->unsetMarkOne();
  } while (!b->isRoot());

}

Board * Board::clone(void) {

#ifdef CS_PROFILE
  across_redid  = NO;
  across_chunks = NO;
#endif

  isCollecting = OK;
  isInGc       = NO;

  unsigned int starttime = 0;

  if (ozconf.timeDetailed)
    starttime = osUserTime();

#ifdef CS_PROFILE
redo:
  if (across_redid)
    OZ_error("Redoing cloning across chunk boundaries. Giving up!\n");

  if (across_chunks)
    across_redid = OK;

  across_chunks = NO;

  cs_orig_start = (int32 *) heapTop;
#endif

  Assert(!isCommitted());

  setGlobalMarks();

  Board * copy = gcBoard();

  Assert(copy);

  gcStack.recurse();

  varFix.fix();

#ifdef NEW_NAMER
  if (am.isPropagatorLocation()) {
    GCMeManager::copyTree();
  }
#endif

  cpTrail.unwind();

  unsetGlobalMarks();

#ifdef CS_PROFILE
  if (across_chunks) {
    goto redo;
  }

  cs_copy_size = cs_orig_start - ((int32 *) heapTop);
  cs_orig_start = (int32 *) heapTop;
  cs_copy_start = (int32*) malloc(4 * cs_copy_size + 256);

  {
    int n = cs_copy_size;

    while (n) {
      *(cs_copy_start + n) = *(cs_orig_start + n);
      n--;
    }

  }
#endif

  if (ozconf.timeDetailed)
    ozstat.timeForCopy.incf(osUserTime()-starttime);

  isCollecting = NO;

  return copy;
}

//*****************************************************************************
//                                GC-METHODS
//*****************************************************************************


/*
 * class Arity is not allocated on heap!
 * but must collect the list && the values in keytable
 */
inline
void Arity::gc()
{
  Arity *aux = this;
  while(aux) {
    if (!aux->isTuple()) {
      for (int i = aux->getSize(); i--; ) {
        if (aux->table[i].key) {
          OZ_collectHeapTerm(aux->table[i].key,aux->table[i].key);
        }
      }
    }
    OZ_collectHeapTerm(aux->list,aux->list);
    aux = aux->next;
  }
}

void ArityTable::gc() {
  for (int i = size; i--; ) {
    if (table[i] != NULL)
      (table[i])->gc();
  }
}

void PrTabEntry::gcPrTabEntries()
{
  PrTabEntry *aux = allPrTabEntries;
  while(aux) {
    OZ_collectHeapTerm(aux->info,aux->info);
    OZ_collectHeapTerm(aux->file,aux->file);
    OZ_collectHeapTerm(aux->printname,aux->printname);
    aux = aux->next;
  }
}

void AbstractionEntry::freeUnusedEntries()
{
  AbstractionEntry *aux = allEntries;
  allEntries = NULL;
  while (aux) {
    AbstractionEntry *aux1 = aux->next;
    if (aux->collected ||
        aux->abstr==NULL) { // RS: HACK alert: compiler might have reference to
                            // abstraction entries: how to detect them??
      aux->collected = NO;
      aux->next  = allEntries;
      allEntries = aux;
    } else {
      delete aux;
    }
    aux = aux1;
  }
}


void AbstractionEntry::gcAbstractionEntry()
{
  if (this==NULL || collected) return;

  collected = OK;
  abstr = gcAbstraction(abstr);
}

//*********************************************************************
//                           NODEs
//*********************************************************************

// failure interface for local tertiarys
inline void maybeGCForFailure(Tertiary *t){
  if(t->getInfo()!=NULL) (*gcEntityInfo)(t);}

inline
void ConstTermWithHome::gcConstTermWithHome()
{
  if (hasGName()) {
    dogcGName(getGName1());
  } else {
    setBoard(getBoardInternal()->gcBoard());
  }
}

ObjectClass * ObjectClass::gcClass() {
  return this ? ((ObjectClass *) gcConstTerm()) : this;
}

void ConstTerm::gcConstRecurse()
{
  switch(getType()) {
  case Co_Object:
    {
      Object *o = (Object *) this;

      switch(o->getTertType()) {
      case Te_Local:
        o->setBoard(o->getBoardInternal()->gcBoard());
        maybeGCForFailure(o);
        break;
      case Te_Proxy:   // PER-LOOK is this possible?
        (*gcProxyRecurse)(o);
        (*gcEntityInfo)(o);
        break;
      case Te_Manager:
        (*gcManagerRecurse)(o);
        (*gcEntityInfo)(o);
        break;
      default:         Assert(0);
      }

      o->setClass(o->getClass()->gcClass());
      if (o->getFreeRecord())
        o->setFreeRecord(o->getFreeRecord()->gcSRecord());
      RecOrCell state = o->getState();
      if (stateIsCell(state)) {
        if (o->isLocal() && getCell(state)->isLocal()) {
          TaggedRef newstate = ((CellLocal*) getCell(state))->getValue();
          o->setState(tagged2SRecord(oz_deref(newstate))->gcSRecord());
        } else if (getCell(state)) {
          o->setState((Tertiary*) getCell(state)->gcConstTerm());
        }
      } else {
        o->setState(getRecord(state)->gcSRecord());
      }
      if (o->getLock())
        o->lock = (OzLock *) o->getLock()->gcConstTerm();
      break;
    }

  case Co_Class:
    {
      ObjectClass *cl = (ObjectClass *) this;
      cl->gcConstTermWithHome();
      cl->fastMethods    = (OzDictionary*) cl->fastMethods->gcConstTerm();
      cl->defaultMethods = (OzDictionary*) cl->defaultMethods->gcConstTerm();
      cl->features       = cl->features->gcSRecord();
      if (cl->unfreeFeatures)
        cl->unfreeFeatures = cl->unfreeFeatures->gcSRecord();
      break;
    }

  case Co_Abstraction:
    {
      Abstraction *a = (Abstraction *) this;
      a->gcConstTermWithHome();
      if (isInGc)
        gcCode(a->getPred()->getCodeBlock());
      break;
    }

  case Co_Cell:
    {
      Tertiary *t=(Tertiary*)this;
      if (t->isLocal()) {
        CellLocal *cl=(CellLocal*)t;
        cl->setBoard(cl->getBoardInternal()->gcBoard());
        OZ_collectHeapTerm(cl->val,cl->val);
        maybeGCForFailure(t);
      } else {
        (*gcDistCellRecurse)(t);
      }
      break;
    }

  case Co_Port:
    {
      Port *p = (Port*) this;
      if (p->isLocal()) {
        p->setBoard(p->getBoardInternal()->gcBoard());
        PortWithStream *pws = (PortWithStream *) this;
        OZ_collectHeapTerm(pws->strm,pws->strm);
        maybeGCForFailure(p);
        break;
      } else {
        (*gcDistPortRecurse)(p);
      }
      break;
    }
  case Co_Space:
    {
      Space *s = (Space *) this;
      Assert(s->getInfo()==NULL);
      if (!s->isProxy()) {
        if (!s->isMarkedFailed() && !s->isMarkedMerged()) {
          if (s->solve->gcIsAlive()) {
            s->solve = s->solve->gcBoard();
          } else {
            s->solve = (Board *) 0;
            Assert(s->isMarkedFailed());
          }
        }
        if (s->isLocal())
          s->setBoard(s->getBoardInternal()->gcBoard());
      }
      break;
    }

  case Co_Chunk:
    {
      SChunk *c = (SChunk *) this;
      OZ_collectHeapTerm(c->value,c->value);
      c->gcConstTermWithHome();
      break;
    }

  case Co_Array:
    {
      OzArray *a = (OzArray*) this;
      a->gcConstTermWithHome();
      int aw = a->getWidth();
      if (aw > 0) {
        TaggedRef *newargs = (TaggedRef*) heapMalloc(sizeof(TaggedRef) * aw);
        OZ_collectHeapBlock(a->getArgs(), newargs, aw);
        a->args=newargs;
      }
      break;
    }

  case Co_Dictionary:
    {
      OzDictionary *dict = (OzDictionary *) this;
      dict->gcConstTermWithHome();
      dict->table = dict->table->gc();
      break;
    }

  case Co_Lock:
    {
      Tertiary *t=(Tertiary*)this;
      if (t->isLocal()) {
        LockLocal *ll = (LockLocal *) this;
        ll->setBoard(ll->getBoardInternal()->gcBoard());
        gcPendThreadEmul(&(ll->pending));
        ll->setLocker(SuspToThread(ll->getLocker()->gcSuspendable()));
        maybeGCForFailure(t);
        break;
      } else {
        (*gcDistLockRecurse)(t);
      }
      break;
    }

  default:
    Assert(0);
  }
}

#define CheckLocal(CONST) \
{                                         \
   Board *bb=(CONST)->getBoardInternal(); \
   Assert(bb->gcIsAlive());               \
   if (!NEEDSCOPYING(bb)) return this;    \
}


ConstTerm *ConstTerm::gcConstTerm() {
  Assert(this);

  if (gcIsMarked())
    return gcGetFwd();

  GName *gn = NULL;

  ConstTerm * ret;

  switch (getType()) {
  case Co_BigInt:
    if (isInGc) {
      ret = ((BigInt *) this)->gc();
      storeFwdField(this, ret);
      return ret;
    } else {
      return this;
    }
  case Co_Abstraction:
    {
      Abstraction *a = (Abstraction *) this;
      CheckLocal(a);

      Abstraction *newA = Abstraction::newAbstraction(a->getPred(),
                                                      a->getBoardInternal());
      gcStack.push(newA,PTR_CONSTTERM);
      storeFwdField(this, newA);
      gn = a->getGName1();
      if (gn) {
        newA->setGName(gn);
        dogcGName(gn);
      }
      OZ_collectHeapBlock(a->getGRef(),newA->getGRef(),
                          a->getPred()->getGSize());

      return newA;
    }

  case Co_Object:
    {
      Object *o = (Object *) this;
      CheckLocal(o);
      ret = (ConstTerm *) gcReallocStatic(this,sizeof(Object));
      gn = o->getGName1();
      break;
    }
  case Co_Class:
    {
      ObjectClass *cl = (ObjectClass *) this;
      CheckLocal(cl);
      gn = cl->getGName1();
      ret = (ConstTerm *) gcReallocStatic(this,sizeof(ObjectClass));
      break;
    }
  case Co_Cell:
    {
      switch(((Tertiary *)this)->getTertType()){
      case Te_Local:
        CheckLocal((CellLocal*)this);
      case Te_Proxy:
      case Te_Manager:
        ret = (ConstTerm *) gcReallocStatic(this,sizeof(CellManagerEmul));
        break;
      case Te_Frame:{
        ret = (ConstTerm *) gcReallocStatic(this,sizeof(CellFrameEmul));
        break;
      }
      default:{
        Assert(0);
        break;}}
      break;
    }

  case Co_Port:
    {
      if(((Tertiary *)this)->getTertType()==Te_Proxy) {
        ret = (ConstTerm *) gcReallocStatic(this, SIZEOFPORTPROXY);
        break;}
      if(((Tertiary *)this)->getTertType()==Te_Local) {
        CheckLocal((PortLocal *) this);}
      ret = (ConstTerm *) gcReallocStatic(this,sizeof(PortLocal));
      break;
    }
  case Co_Space:
    {
      Space *sp = (Space *) this;
      CheckLocal(sp);

      ret = (ConstTerm *) gcReallocStatic(this,sizeof(Space));
      break;
    }

  case Co_Chunk:
    {
      SChunk *sc = (SChunk *) this;
      CheckLocal(sc);
      gn = sc->getGName1();
      ret = (ConstTerm *) gcReallocStatic(this,sizeof(SChunk));
      break;
    }

  case Co_Array:
    CheckLocal((OzArray *) this);
    ret = (ConstTerm *) gcReallocStatic(this,sizeof(OzArray));
    break;

  case Co_Dictionary:
    CheckLocal((OzDictionary *) this);
    ret = (ConstTerm *) gcReallocStatic(this,sizeof(OzDictionary));
    break;

  case Co_Lock:
    {
      switch(((Tertiary *)this)->getTertType()) {
      case Te_Local:
        CheckLocal((LockLocal*)this);
      case Te_Proxy:
      case Te_Manager:
        ret = (ConstTerm *) gcReallocStatic(this,sizeof(LockLocal));
        break;
      case Te_Frame:{
        ret = (ConstTerm *) gcReallocStatic(this,sizeof(LockLocal));
        break;
      }
      default:{
        Assert(0);
        break;}}
      break;
    }

  /* builtins are allocate dusing malloc */
  case Co_Builtin:
    return this;

  case Co_Foreign_Pointer:
    return ((ForeignPointer*)this)->gc();

  case Co_Resource:{
    ret = (*gcDistResource)(this);
    storeFwdField(this, ret);
    return ret;}

  default:
    Assert(0);
    return 0;
  }

  gcStack.push(ret,PTR_CONSTTERM);
  storeFwdField(this, ret);
  dogcGName(gn);
  return ret;
}

/* the purpose of this procedure is to provide an additional entry
   into gc so to be able to distinguish between owned perdio-objects that
   are locally accssible to those that are not - currently this is needed
   only for frames (cells and locks).
   The distinction is that in this procedure the BORROW ENTRY is not marked
   but in gcConstTerm it is marked.
   Note- all other Tertiarys are marked in gcConstRecurse
*/

inline
OzDebug *OzDebug::gcOzDebug() {
  OzDebug *ret = (OzDebug*) gcReallocStatic(this,sizeof(OzDebug));

  ret->Y   = gcRefsArray(ret->Y);
  ret->CAP = gcAbstraction(ret->CAP);

  if (ret->data)
    OZ_collectHeapTerm(ret->data,ret->data);

  if (ret->arity > 0) {
    ret->arguments = (TaggedRef *)
      heapMalloc(ret->arity * sizeof(TaggedRef));

    OZ_collectHeapBlock(arguments, ret->arguments, arity);
  }

  return ret;
}

inline
TaskStack * TaskStack::gc(void) {

  TaskStack *newstack = new TaskStack(suggestNewSize());
  TaskStack *oldstack = this;

  Frame *oldtop = oldstack->getTop();
  int offset    = oldstack->getUsed();
  Frame *newtop = newstack->array + offset;

  while (1) {
    GetFrame(oldtop,PC,Y,CAP);

    if (isInGc)
      gcCode(PC);

    if (PC == C_EMPTY_STACK) {
      *(--newtop) = PC;
      *(--newtop) = Y;
      *(--newtop) = CAP;
      Assert(newstack->array == newtop);
      newstack->setTop(newstack->array+offset);
      return newstack;
    } else if (PC == C_CATCH_Ptr) {
    } else if (PC == C_XCONT_Ptr) {
      // mm2: opt: only the top task can/should be xcont!!
      ProgramCounter pc   = (ProgramCounter) *(oldtop-1);
      if (isInGc)
        gcCode(pc);
      (void) CodeArea::livenessX(pc,Y,getRefsArraySize(Y));
      Y = gcRefsArray(Y); // X
    } else if (PC == C_LOCK_Ptr) {
      Y = (RefsArray) ((OzLock *) Y)->gcConstTerm();
    } else if (PC == C_SET_SELF_Ptr) {
      Y = (RefsArray) (Y ? (((Object*)Y)->gcConstTerm()) : 0);
    } else if (PC == C_SET_ABSTR_Ptr) {
      ;
    } else if (PC == C_DEBUG_CONT_Ptr) {
      Y = (RefsArray) ((OzDebug *) Y)->gcOzDebug();
    } else if (PC == C_CALL_CONT_Ptr) {
      /* tt might be a variable, so use this ugly construction */
      *(newtop-2) = Y; /* UGLYYYYYYYYYYYY !!!!!!!! */
      TaggedRef *tt = (TaggedRef*) (newtop-2);
      OZ_collectHeapTerm(*tt,*tt);
      Y = (RefsArray) ToPointer(*tt);
      CAP = (Abstraction *)gcRefsArray((RefsArray)CAP);
    } else if (PC == C_CFUNC_CONT_Ptr) {
      CAP = (Abstraction *)gcRefsArray((RefsArray)CAP);
    } else { // usual continuation
      Y = gcRefsArray(Y);
      CAP = gcAbstraction(CAP);
    }

    *(--newtop) = PC;
    *(--newtop) = Y;
    *(--newtop) = CAP;
  } // while not task stack is empty
}


/*
 * notification board == home board of thread
 * Although this may be discarded/failed, the solve actor must be announced.
 * Therefore this procedures searches for another living board.
 */
inline
Board* Board::gcGetNotificationBoard() {
  Assert(this);

  Board *bb = derefBoard();

  Board *nb = bb;

  while (1) {

    if (bb->gcIsMarked() || bb->isRoot())
      return nb;

    Assert(!bb->isCommitted());

    if (bb->isFailed()) {
      /*
       * notification board must be changed
       */
      bb = bb->getParent();
      nb = bb;   // probably not dead;
      continue;
    }

    bb = bb->getParent();

  }
}

inline
void Thread::gcRecurse(Thread * fr) {
  taskStack = fr->taskStack->gc();
  abstr     = fr->abstr;
  id        = fr->id;
}

inline
void Propagator::gcRecurse(Propagator * fr) {
  _p = fr->_p->gc();

  _p->updateHeapRefs(isInGc);

}

inline
Suspendable * Suspendable::gcSuspendableInline(void) {
  Assert(this);

  if (isGcMarked())
    return gcGetFwd();

  if (isDead())
    return (Suspendable *) NULL;

  Suspendable * to;

  if (getBoardInternal()->gcIsAlive()) {
    Assert(isInGc || !isRunnable());

    if (isThread()) {
      to = (Suspendable *) freeListMalloc(sizeof(Thread));

      ((Thread *) to)->gcRecurse(SuspToThread(this));

    } else {
      to = (Suspendable *) freeListMalloc(sizeof(Propagator));

      ((Propagator *) to)->gcRecurse(SuspToPropagator(this));

    }

    to->setBoardInternal(getBoardInternal()->gcBoard());

  } else if (isThread()) {

    to = (Suspendable *) gcReallocStatic(this, sizeof(Thread));

    Board * nb = getBoardInternal()->gcGetNotificationBoard()->gcBoard();

    ((Thread *) to)->setTaskStack(new TaskStack(ozconf.stackMinSize));

    nb->incSuspCount();

    ((Thread *) to)->pushCall(BI_skip,0,0);

    to->setBoardInternal(nb);
  } else {
    return NULL;
  }

  to->flags = flags;
  storeFwdField(this, to);

  return to;
}

inline
Propagator * Propagator::gcLocalInline(Board * bb) {
  Assert(this);

  if (isGcMarked())
    return SuspToPropagator(gcGetFwd());

  Assert(isPropagator());

  if (isDead())
    return (Propagator *) NULL;

  Propagator * to;

  Assert(getBoardInternal()->gcIsAlive());
  Assert(isInGc || !isRunnable());

  Assert(getBoardInternal()->derefBoard()->gcIsMarked() &&
         getBoardInternal()->derefBoard()->gcGetFwd() == bb);

  to = (Propagator *) freeListMalloc(sizeof(Propagator));

  to->gcRecurse(SuspToPropagator(this));

  to->setBoardInternal(bb);

  to->flags = flags;

  storeFwdField(this, to);

  Assert(to->isPropagator());

  return to;
}

Suspendable * Suspendable::gcSuspendable(void) {
  return (this == NULL) ? NULL : gcSuspendableInline();
}

/*
 * This routine MUST maintain the order, since it is also used
 * for ordered susplists and suspendable queues
 *
 */

inline
SuspList * SuspList::gcRecurse(SuspList ** last) {
  SuspList * sl = this;
  SuspList * pl = SuspList::_gc_sentinel;

  while (sl) {
    Suspendable * to = sl->getSuspendable()->gcSuspendableInline();

    if (to) {
      SuspList * nl = new SuspList(to);
      pl->setNext(nl);
      pl = nl;
    }

    sl = sl->getNext();

  };

  pl->setNext(NULL);

  if (last)
    *last = pl;

  return SuspList::_gc_sentinel->getNext();
}

inline
SuspList * SuspList::gcLocalRecurse(Board * bb) {
  SuspList * sl = this;

  SuspList * ret;
  SuspList ** p = &ret;

  for (SuspList * sl = this; sl; sl=sl->getNext()) {
    Suspendable * to =
      SuspToPropagator(sl->getSuspendable())->gcLocalInline(bb);

    if (to) {
      SuspList * n = new SuspList(to);
      *p = n;
      p  = &(n->_next);
    }

  }

  *p = NULL;

  return ret;
}

void SuspQueue::gc(void) {
  if (isEmpty())
    return;

  SuspList * head = last->getNext();

  if (!isInGc)
    cpTrail.save((int32 *) last->getNextRef());

  last->setNext(NULL);

  head = head->gcRecurse(&last);

  last->setNext(head);

}

void ThreadsPool::gc(void) {
  Assert(isInGc);
  _q[ HI_PRIORITY].gc();
  _q[MID_PRIORITY].gc();
  _q[LOW_PRIORITY].gc();
}



/****************************************************************************
 * Board collection
 ****************************************************************************/

inline
DistBag * DistBag::gc(void) {
  DistBag *  copy = (DistBag *) 0;
  DistBag ** cur  = &copy;
  DistBag *  old  = this;

  while (old) {
    DistBag * one = new DistBag(old->dist->gc());
    *cur = one;
    cur  = &(one->next);
    old  = old->next;
  }

  *cur = 0;

  return copy;
}


inline
void Board::gcRecurse() {
  Assert(!isCommitted() && !isFailed());

  // Do not recurse over root board (be it the global one or
  // the root board for cloning!)
  if (!isRoot() && !getParentInternal()->hasMarkOne())
    parentAndFlags.set(getParentInternal()->gcBoard(),0);

  lpq.gc();

  script.Script::gc();

  OZ_collectHeapTerm(rootVar,rootVar);

  OZ_collectHeapTerm(status,status);

  gcSuspList(&suspList);
  setDistBag(getDistBag()->gc());
  gcSuspList((SuspList **) &nonMonoSuspList);

#ifdef CS_PROFILE
  if((copy_size>0) && copy_start && isInGc) {
    free(copy_start);
  }
  orig_start = (int32 *) NULL;
  copy_start = (int32 *) NULL;
  copy_size  = 0;
#endif

}


//*****************************************************************************
//                           collectGarbage
//*****************************************************************************

inline
void SRecord::gcRecurse() {
  SRecord * to = (SRecord *) GCUNMARK(label);

  OZ_collectHeapTerm(to->label,to->label);

  OZ_collectHeapBlock(getRef(), to->getRef(), getWidth());

}


inline
void LTuple::gcRecurse() {
  LTuple * frm = this;
  LTuple * to  = (LTuple *) GCUNMARK(frm->args[0]);

  // Restore original!
  frm->args[0] = to->args[0];

  TaggedRef aux = oz_deref(to->args[0]);

  // Case : L=L|_
  if (!oz_isLTuple(aux) || tagged2LTuple(aux) != this) {
    OZ_collectHeapTerm(frm->args[0], to->args[0]);

    storeFwd((int32 *)frm->args, to->args);
  }

  while (1) {
    // Store forward, order is important, since collection might already
    // have done a storeFwd, which means that this one will be overwritten
    TaggedRef t = oz_deref(frm->args[1]);

    if (!oz_isCons(t)) {
      OZ_collectHeapTerm(frm->args[1], to->args[1]);
      return;
    }

    frm = tagged2LTuple(t);

    if (GCISMARKED(frm->args[0])) {
      to->args[1] = makeTaggedLTuple((LTuple *) GCUNMARK(frm->args[0]));
      return;
    }

    LTuple * next = (LTuple *) freeListMalloc(sizeof(LTuple));

    to->args[1] = makeTaggedLTuple(next);
    to = next;

    OZ_collectHeapTerm(frm->args[0], to->args[0]);

    storeFwd((int32 *)frm->args, to->args);

  }

  Assert(0);
}


void GcStack::recurse(void) {

  while (!isEmpty()) {
    TaggedRef tp  = (TaggedRef) pop();
    void * ptr    = tagValueOf(tp);
    TypeOfPtr how = (TypeOfPtr) tagTypeOf(tp);

    switch(how) {
    case PTR_LTUPLE:
      ((LTuple *) ptr)->gcRecurse();
      break;
    case PTR_SRECORD:
      ((SRecord *) ptr)->gcRecurse();
      break;
    case PTR_BOARD:
      ((Board *) ptr)->gcRecurse();
      break;
    case PTR_CVAR:
      ((OzVariable *) ptr)->gcVarRecurse();
      break;
    case PTR_CONSTTERM:
      ((ConstTerm *) ptr)->gcConstRecurse();
      break;
    case PTR_EXTENSION:
      gcExtensionRecurse((OZ_Extension *)ptr);
      break;
    case PTR_SUSPLIST:
      *((SuspList **) ptr) = (*(SuspList **) ptr)->gcRecurse(NULL);
      break;
    case PTR_UNUSED2:
      Assert(0);
    default:
      {
        Assert(how & PTR_LOCAL_SUSPLIST);

        SuspList ** sl = (SuspList **) ptr;
        Board    *  bb = (Board *) pop();

        for (int i = how - PTR_LOCAL_SUSPLIST; i--; )
          sl[i] = sl[i]->gcLocalRecurse(bb);

      }
    }
  }
}




//*****************************************************************************
//             AM methods to launch gc under certain conditions
//*****************************************************************************


// signal handler
void checkGC() {
  Assert(!am.isCritical());
  if (getUsedMemory() > unsigned(ozconf.heapThreshold) && ozconf.gcFlag) {
    am.setSFlag(StartGC);
  }
}

void AM::doGC() {
  Assert(oz_onToplevel());

  /* do gc */
  gc(ozconf.gcVerbosity);

  /* calc limits for next gc */
  int used   = getUsedMemory();
  int wanted = ((ozconf.heapFree == 100)
                ? ozconf.heapMaxSize
                : max(((long) used) * (100 / (100 - ozconf.heapFree)),
                      ozconf.heapMinSize));

  /* Try to align as much as possible to end of blocksize */
  int block_size = HEAPBLOCKSIZE / KB;
  int block_dist = wanted % block_size;

  if (block_dist > 0)
    block_dist = block_size - block_dist;

  wanted += min(block_dist,
                (((long) wanted) * ozconf.heapTolerance / 100));

  if (wanted > ozconf.heapMaxSize) {
    if (ozconf.runningUnderEmacs) {
      OZ_warning("\n*** Heap Max Size exceeded: Increasing from %d to %d.\n",
                 ozconf.heapMaxSize,wanted);
      prefixError();
      fflush(stdout);
    }
    ozconf.heapMaxSize = wanted;
  }

  ozconf.heapThreshold = wanted;

  unsetSFlag(StartGC);
}

// special purpose to gc borrowtable entry which is a variable
TaggedRef gcTagged1(TaggedRef in) {
  TaggedRef x=oz_deref(in);
  Assert(GCISMARKED(x));
  return makeTaggedRef((TaggedRef*)GCUNMARK(x));
}



//*****************************************************************************
//       GC Code Area
//*****************************************************************************

static int codeGCgeneration = CODE_GC_CYLES;

void CodeArea::gcCodeBlock()
{
  if (referenced == NO) {
    referenced = OK;
    gclist->collectGClist();
  }
}

void gcCode(CodeArea *block) {
  Assert(isInGc);
  if (codeGCgeneration!=0)
    return;

  block->gcCodeBlock();
}


void gcCode(ProgramCounter PC) {
  gcCode(CodeArea::findBlock(PC));
}

void CodeGCList::collectGClist()
{
  CodeGCList *aux = this;
  while(aux) {
    for (int i=aux->nextFree; i--; ) {
      switch(aux->block[i].tag) {
      case C_TAGGED:
        OZ_collectHeapTerm(*(TaggedRef*)aux->block[i].pc,
                           *(TaggedRef*)aux->block[i].pc);
        break;
      case C_ABSTRENTRY:
        ((AbstractionEntry*)*(aux->block[i].pc))->gcAbstractionEntry();
        break;
      case C_INLINECACHE:
        ((InlineCache*)aux->block[i].pc)->invalidate();
        break;
      case C_FREE:
        break;
      default:
        Assert(0);
      }
    }
    aux = aux->next;
  }
}

void CodeArea::gcCodeAreaStart()
{
// #define CODEGCOFF
#ifndef CODEGCOFF
  if (ozconf.codeGCcycles == 0) {
    codeGCgeneration = 1;
  } else if (++codeGCgeneration >= ozconf.codeGCcycles) {
    // switch code GC on
    codeGCgeneration = 0;
    return;
  }
#endif

  CodeArea *code = allBlocks;

  while (code) {
    code->gcCodeBlock();
    code = code->nextBlock;
  }
}

void CodeArea::gcCollectCodeBlocks()
{
  CodeArea *code = allBlocks;
  allBlocks = NULL;
  while (code) {
    if (code->referenced == NO) {
      //message("collected(%x): %d\n",code,code->size*sizeof(ByteCode));
      //displayCode(code->getStart(),5);
      CodeArea *aux = code;
      code = code->nextBlock;
      delete aux;
    } else {
      code->referenced = NO;
      CodeArea *aux    = code;
      code             = code->nextBlock;
      aux->nextBlock   = allBlocks;
      allBlocks        = aux;
    }
  }
}
