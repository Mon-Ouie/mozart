/*
 *  Authors:
 *    Kostja Popov (kost@sics.se)
 *    Per Brand (perbrand@sics.se)
 * 
 *  Contributors:
 *    Ralf Scheidhauer (Ralf.Scheidhauer@ps.uni-sb.de)
 * 
 *  Copyright:
 *    Organization or Person (Year(s))
 * 
 *  Last change:
 *    $Date$
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

#ifndef __GENTRAVERSER_H
#define __GENTRAVERSER_H

#if defined(INTERFACE)
#pragma interface
#endif

#include <setjmp.h>
#include "base.hh"

#include "stack.hh"
#include "hashtbl.hh"
#include "indexing.hh"
#include "tagged.hh"
#include "value.hh"
#include "codearea.hh"
#include "am.hh"
#include "dictionary.hh"
#include "gname.hh"

//#define USE_ROBUST_UNMARSHALER

//
#define GT_STACKSIZE	4096

#ifdef DEBUG_CHECK
// let's count nodes processed...
#define CrazyDebug(Code) Code
// #define CrazyDebug(Code)
#else
#define CrazyDebug(Code)
#endif

//
// The class NodeProcessor allows to build a stack-based traverser on top
// of it. An object of this class keeps track of nodes in the (may be
// rational) tree that are still to be processed. Traversing begins with
// 'start(root, proc, opaque)' where 'root' is a root node, 'proc' is a
// procedure that processes nodes in the tree, and 'opaque' is an opaque
// for the node processor chunk of data that is needed for processing
// nodes in the tree. 'proc' performs whatever it wants on tree nodes;
// for non-leaf nodes (e.g. records) it can apply
// 'nodeProcessor->add(node)' for subtrees in whatever order. This causes
// the node processor to visit subtrees: it will call 'proc(node)' for
// nodes in the reverse order that you added them.
//
// Traversing can be also suspended and resumed, and the queue of nodes
// being processed can be emptied.
//
// Note that the GC could be built using this class (though, slightly
// extended for perfromance reasons) as well!

//	   
class Opaque {};
class NodeProcessor;
typedef void (*ProcessNodeProc)(OZ_Term, Opaque*, NodeProcessor*);

//
// 'NodeProcessorStack' keeps OZ_Term"s to be traversed.
// Basically only allocation routines are used from 'Stack';
class NodeProcessorStack : protected Stack {
public:
  NodeProcessorStack() : Stack(GT_STACKSIZE, Stack_WithMalloc) {}
  ~NodeProcessorStack() {}

  //
  void ensureFree(int n) {
    if (stackEnd <= tos+n)
      resize(n);
  }

  //
  // we don't use 'push' from Stack because we care about
  // space ourselves;
  void put(OZ_Term term) {
    Assert(tagTypeOf(term) != GCTAG);
    checkConsistency();
    *tos++ = ToPointer(term);
  }
  OZ_Term get() {
    checkConsistency();
    Assert(isEmpty() == NO);
    return (ToInt32(*(--tos)));
  }
  OZ_Term lookup() {
    checkConsistency();
    return (ToInt32(topElem()));
  }

  //
  // For special 'GenTraverser' machinery: handling code areas, etc.
  void putInt(int32 i) {
    checkConsistency();
    *tos++ = ToPointer(i);
  }
  int32 getInt() {
    checkConsistency();
    Assert(isEmpty() == NO);
    return (ToInt32(*(--tos)));
  }
  int32 lookupInt() {
    checkConsistency();
    return (ToInt32(topElem()));
  }
  void putPtr(void* ptr) {
    checkConsistency();
    *tos++ = ptr;
  }
  void* getPtr() {
    checkConsistency();
    Assert(isEmpty() == NO);
    return (*(--tos));
  }
  void* lookupPtr() {
    checkConsistency();
    return (*(tos-1));
  }

  //
  // And yet we update stack entries:
  StackEntry* putPtrSERef(void *ptr) {
    checkConsistency();
    *tos = ptr;
    return (tos++);
  }
  void updateSEPtr(StackEntry* se, void *ptr) { *se = ptr; }
  //
  void dropEntry() { tos--; }
  void dropEntries(int n) { tos -= n; }
};

//
class NodeProcessor : public NodeProcessorStack {
protected:
  Bool keepRunning;
  ProcessNodeProc proc;
  Opaque* opaque;

  //
protected:
  // actual processor;
  void doit() {
    while (keepRunning && !isEmpty()) {
      OZ_Term term = get();
      (*proc)(term, opaque, this);
    }
  }

  //
public:
  NodeProcessor() {
    keepRunning = OK;		// done originally;
    DebugCode(opaque = (Opaque *) -1);
  }
  ~NodeProcessor() {}

  //
  // If 'suspend()' is called (by 'ProcessNodeProc') then 'start(...)'
  // will return; it can be later resumed by using 'resume()';
  void suspend() { keepRunning = NO; }
  void add(OZ_Term t) {
    ensureFree(1);
    put(t);
  } // adds a new entry to the process stack;

  //
  void resume() { doit(); } // see 'suspend()';
  void clear() { mkEmpty(); } // deletes all entries in process queue;

  //
  Opaque* getOpaque() { return (opaque); }
  //
  // Define the first node & start the action. Returns 'TRUE' when
  // we are done (i.e. the stack is empty);
  Bool start(OZ_Term t, ProcessNodeProc p, Opaque* o) {
    clear();
    put(t);
    proc = p;
    opaque = o;
    //
    keepRunning = OK;
    doit();
    // if 'keepRunning' is true when the stack is empty:
    return (keepRunning);
  }

};

//
// 'IndexTable' is basically former 'RefTrail' from 'marshaler.hh'
// (done by Ralf?). Since the later one should go away, i've copied it
// here (this allows for a fair perfomance comparision);
//
// However, it takes now 'OZ_Term's as keys!
class IndexTable : public HashTableFastReset {
public:
  IndexTable() : HashTableFastReset(2000) {}

  //
protected:
  int remember(OZ_Term l) {
    Assert(!oz_isRef(l));
    Assert(find(l) == -1);
    int index = getSize();	// meets our needs...
    htAdd((intlong) l, ToPointer(index));
    return (index);
  }
  int find(OZ_Term l) {
    Assert(!oz_isRef(l));
    void *ret = htFind((intlong) l);
    return ((ret == htEmpty) ? -1 : (int) ToInt32(ret));
  }

  //
public:
  // In fact, we could also have two tables: one for terms, and one -
  // for pointers:
  int remember(void* p) {
    Assert(oz_isRef((OZ_Term) p));
    Assert(find(p) == -1);
    int index = getSize();
    htAdd((intlong) p, ToPointer(index));
    return (index);
  }
  int find(void *p) {
    Assert(oz_isRef((OZ_Term) p));
    void *ret = htFind((intlong) p);
    return ((ret == htEmpty) ? -1 : (int) ToInt32(ret));
  }

  //
protected:
  //
  void unwind() {
    mkEmpty();
    Assert(getSize() == 0);
  }
};

//
// An object of the 'GenTraverser' class traverses the node graph. The
// user of this class is supposed to create a subclass that specifies
// what to do with each type of nodes. Traversing starts with
// 'traverse(OZ_Term t, Opaque *o)', where 't' is a root node.  The
// traverser then calls 'processXXX(root)', depending on the type of
// root node XXX.  The processXXX virtual methods are divided into two
// categories, those that return void (these are always leaves) and
// boolean ones which return TRUE to indicate a leaf, and FALSE
// otherwise.  For instance, if 'processRecord()' returns FALSE, then
// the traverser proceeds with traversing the subtrees, i.e. will call
// processXXX for each argument. The traverser works in depth-first
// manner (it is based on 'NodeProcessor').
//
// For handling of cycles and co-references, the method 'remember()'
// can be used inside 'processXXX()': it returns an integer uniquely
// identifying the remembered node. Thereafter if the node is reached
// again the traverser does not call 'processXXX(OZ_Term)' but rather
// 'processRepetition(int)'. The last method also returns a Bool,
// indicating if traversal should continue or not. For example, let's
// assume that 'remember(f(X Y))' returns '1'. Then later
// 'processRepetition(1)' is called upon reaching a repetition
// (pointer equality). Usually it would return 'TRUE' and f(X Y) would
// not be traversed again. [Possibly you might want to traverse the
// same thing twice, in which case you return FALSE].
// 
// Note that the idea is that the idea is that you can easily create
// subclasses for marshaling, export control, etc.
// 
// suspend/resume are inherited. 
//

//
#define MAKETRAVERSERTASK(task)  makeGCTaggedInt((int32) task)
//
inline
int32 getTraverserTaskArg(OZ_Term taggedTraverserTask)
{
  Assert(tagTypeOf(taggedTraverserTask) == GCTAG);
  return (getGCTaggedInt(taggedTraverserTask));
}
// no argument is needed currenty;
const OZ_Term taggedBATask = MAKETRAVERSERTASK(0);

//
// Marshaling/unmarshaling of record arity also requires some special
// machinery... The problem is that the surrounding context must know
// sometimes (aka when unmarshaling hash tables from code area) how
// many subtrees are there, and what to do with them;
enum RecordArityType {
  RECORDARITY,
  TUPLEWIDTH
};

//
inline
RecordArityType unmarshalRecordArityType(MsgBuffer *bs) {
  return ((RecordArityType) unmarshalNumber(bs));
}
//
inline
RecordArityType unmarshalRecordArityTypeRobust(MsgBuffer *bs, int *overload) {
  return ((RecordArityType) unmarshalNumberRobust(bs, overload));
}
inline
void marshalRecordArityType(RecordArityType type, MsgBuffer *bs) {
  marshalNumber(type, bs);
}
inline
SRecordArity makeRealRecordArity(OZ_Term arityList) {
  Assert(isSorted(arityList));
  Arity *ari = aritytable.find(arityList);
  Assert(!ari->isTuple());
  return (mkRecordArity(ari));
}


//
// A user can declare a binary area which will be processed with
// 'MarshalerBinaryAreaProcessor' supplied (see also the comments for
// GenTraverser::marshalBinary()');
typedef Bool (*MarshalerBinaryAreaProcessor)(GenTraverser *m, void *arg);

//
// An object of the class can be used for more than one traversing;
class GenTraverser : private NodeProcessor, public IndexTable {
private:
  CrazyDebug(int debugNODES;);
  void doit();			// actual processor;

private:
  CrazyDebug(void incDebugNODES() { debugNODES++; });
  CrazyDebug(void decDebugNODES() { debugNODES--; });

  //
  // 'reset()' returns the traverser to the original state;
  void reset() {
    CrazyDebug(debugNODES = 0;);
    suspend();
    clear();
    unwind();
  }

  //
public:
  GenTraverser() {
    DebugCode(opaque = (Opaque *) -1);
  }
  virtual ~GenTraverser() {}

  void rememberNode(OZ_Term node, MsgBuffer *bs) {
    int ind = remember(node);
    marshalTermDef(ind, bs);
  }
  void rememberNode(void *p, MsgBuffer *bs) {
    int ind = remember(p);
    marshalTermDef(ind, bs);
  }

  //
  // For efficiency reasons 'GenTraverser' has its own 'doit' - not
  // the one from 'NodeProcessor'. Because of that, 'resume()' is 
  // overloaded as well (but with the same meaning);
  void traverse(OZ_Term t, Opaque* o) {
    reset();
    DebugCode(proc = (ProcessNodeProc) -1;); // not used;
    DebugCode(keepRunning = NO;);            // not used;
    Assert(opaque == (Opaque *) -1); // otherwise that's recursive;
    Assert(o != (Opaque *) -1);	     // not allowed (limitation);
    opaque = o;
    //
    ensureFree(1);
    put(t);
    //
    doit();
    // CrazyDebug(fprintf(stdout, " --- %d nodes.\n", debugNODES););
    // CrazyDebug(fflush(stdout););
    DebugCode(opaque = (Opaque *) -1);
  }

  //
  // Sometimes it is desirable to marshal a number of values with
  // detecting equal nodes within all of them... This is useful when
  // e.g. marshaling a (PERDIO) message, 'cause we know the message is
  // unmarshaled atomically, so the term table keep containing values
  // across "(new)unmarshalTerm()'. 
  //
  // In this case one should start with 'prepareTraversing()' and
  // specify values to be marshaled with 'traverseOne()', and finish
  // with 'finishTraversing()':
  void prepareTraversing(Opaque *o) {
    reset();
    DebugCode(proc = (ProcessNodeProc) -1;); // not used;
    DebugCode(keepRunning = NO;);            // not used;
    Assert(opaque == (Opaque *) -1); // otherwise that's recursive;
    Assert(o != (Opaque *) -1);	     // not allowed (limitation);
    opaque = o;
  }
  void traverseOne(OZ_Term t) {
    ensureFree(1);
    put(t);
    //
    doit();
    // CrazyDebug(fprintf(stdout, " --- %d nodes.\n", debugNODES););
    // CrazyDebug(fflush(stdout););
  }
  void finishTraversing() {
    DebugCode(opaque = (Opaque *) -1);
  }

  //
  Opaque* getOpaque() {
    Assert(opaque != (Opaque *) -1);
    return (opaque);
  }

  //	
protected:
  //
  // Note that co-references are discovered not among all nodes, but
  // only among: literals, cvar"s, ltuples, srecords, and all oz
  // const"s;
  //
  // OZ_Term"s are dereferenced;
  virtual void processSmallInt(OZ_Term siTerm) = 0;
  virtual void processFloat(OZ_Term floatTerm) = 0;
  virtual void processLiteral(OZ_Term litTerm) = 0;
  virtual void processExtension(OZ_Term extensionTerm) = 0;
  // OzConst"s;
  virtual void processBigInt(OZ_Term biTerm, ConstTerm *biConst) = 0;
  virtual void processBuiltin(OZ_Term biTerm, ConstTerm *biConst) = 0;
  virtual void processObject(OZ_Term objTerm, ConstTerm *objConst) = 0;
  // 'Tertiary' OzConst"s;
  virtual void processLock(OZ_Term lockTerm, Tertiary *lockTert) = 0;
  virtual void processCell(OZ_Term cellTerm, Tertiary *cellTert) = 0;
  virtual void processPort(OZ_Term portTerm, Tertiary *portTert) = 0;
  virtual void processResource(OZ_Term resTerm, Tertiary *tert) = 0;
  // anything else:
  virtual void processNoGood(OZ_Term resTerm, Bool trail) = 0;
  //
  virtual void processUVar(OZ_Term *uvarTerm) = 0;
  // If 'processCVar' return non-zero, then this means we have to
  // process that value instead of the variable;
  virtual OZ_Term processCVar(OZ_Term *cvarTerm) = 0;

  //
  // These methods return TRUE if the node to be considered a leaf;
  // (Note that we might want to go through a repetition, don't we?)
  virtual Bool processRepetition(int repNumber) = 0;

  //
  virtual Bool processLTuple(OZ_Term ltupleTerm) = 0;
  virtual Bool processSRecord(OZ_Term srecordTerm) = 0;
  virtual Bool processFSETValue(OZ_Term fsetvalueTerm) = 0;
  // composite OzConst"s;
  virtual Bool processDictionary(OZ_Term dictTerm, ConstTerm *dictConst) = 0;
  virtual Bool processChunk(OZ_Term chunkTerm, ConstTerm *chunkConst) = 0;
  virtual Bool processClass(OZ_Term classTerm, ConstTerm *classConst) = 0;
  //
  // 'processAbstraction' also issues 'marshalBinary';
  virtual Bool processAbstraction(OZ_Term absTerm, ConstTerm *absConst) = 0;

public:
  // 
  // The 'marshalBinary' method is the only artifact due to the
  // iterative nature of marshaling. Consider marshaling of a code
  // area: it contains Oz values in it. Recursive marshaler just
  // marshals those values "in place", where they occur. Iterative
  // marshaler can either (a) have the traverser knowing where Oz
  // values are, so the job is done similar to Oz records, (b) declare
  // them using 'GenTraverser::marshalOzValue()' (see below). The first
  // approach requires more knowledge from the traverser and in
  // general two scanning phases, yet the traverser's stack can be as
  // large as there are Oz values in the code area. The second
  // approach fixes first two problems but still suffers from the last
  // one. That problem is solved by making binary areas split into
  // pieces; the first one is declared using 'marshalBinary'.
  // Marshaling a binary area is finished when 'proc' returns
  // TRUE. 'proc' must take care of descriptor (e.g. deallocate it);
  void marshalBinary(MarshalerBinaryAreaProcessor proc,
		     void *binaryDescriptor) {
    Assert(binaryDescriptor);	// '0' is used by the traverser itself;
    ensureFree(3);
    putPtr((void *) proc);
    putPtr(binaryDescriptor);
    putInt(taggedBATask);
  }
  //
  // Hopefully, this abstraction is sufficient for marshaling
  // arbitrary stuff.

  //
  // 'MarshalerBinaryAreaProcessor' declare Oz values to be marshaled
  // via 'marshalOzValue()'. Unmarshaler must declare them to be
  // unmarshaled in the same order (but in the stream they appear in
  // reverse order);
  void marshalOzValue(OZ_Term term) {
    ensureFree(1);
    put(term);
  }

  //
  // When a binary area is small (exactly speaking, when we can first
  // marshal the area completely, and after that Oz values contained
  // in it), it's legal to use 'GenTraverser' as 'NodeProcessor', thus
  // to call 'marshalOzValue()' on its own;

protected:
  //
  //
  // There is also a possible optimization for tail-recursive (well,
  // sort of) processing of lists: if a traverser sees that a cons
  // cell is followed by another cell, its 'car' and 'cdr' (that is a
  // list) are treated together. If the traverser discovers a
  // repetition on either car or cdr side, it will fall back to
  // "normal" processing of subtrees;
  //
  // kost@ : TODO: i'm not yet sure that this is worth the result...
  //         Ralf, do you have an idea about that?
  //  virtual Bool processSmallIntList(OZ_Term siTerm, OZ_Term ltuple) = 0;
  //  virtual Bool processLiteralList(OZ_Term litTerm, OZ_Term ltuple) = 0;
};


// 
// The class builder builds a node graph. It does the reverse of the
// traverser. Builder assembles a value mostly top-down, except for
// special cases like an arity list of a record is constructed before
// the record itself is. Builder takes primitive values and
// instructions where and how compound values are placed.  The user of
// the class is supposed to parse the input stream on his own, and
// call corresponding 'build' methods. For (all) primitive values the
// 'buildValue()' method is called. There are methods that correspond
// to 'DIF_XXX' of compound structures, which eventually supply a
// value (in general - not immediately, but in this case the s-pointer
// must be preserved until the structure is really built up);
//
// Internally, Builder contains a stack of tasks, which describe what
// to do with values appearing from the stream. These tasks are
// necessary for dealing with compound structures. In general, a task
// consists out of a type tag (which, on certain platforms, can be
// used for threaded code) and two arguments. For the simplest task,
// "spointer", one of the arguments is an 's-pointer' aka in WAM (and
// another is ignored). The three-field task frame format is motivated
// by 'recordArg' task: it places a record subtree given the record
// and its feature name;

//
// OK, the following task types emerged (keep the 'builderTaskNames'
// array consistent!):
enum BuilderTaskType {
  //
  // Mostly, the only thing to be done with a term is to place it at a
  // given location (including the topmost task):
  BT_spointer = 0,		// (keep this value!!!)
  // A variation of this is to do also a next task:
  BT_spointer_iterate,
  // When constructing subtree in a bottom-up fashion, the node is to
  // be remebered (it proceeds iteratively further). Note that this
  // task is never popped: it follows some '_iterate' task:
  BT_buildValue,

  //
  // 'buildTuple' puts 'makeTuple' (and a special frame with the
  // arity, and maybe - for 'makeTupleMemo' - with storing index).
  // 'makeTuple' creates the tuple and puts 'n' 'tuple' tasks, each of
  // whose contains a corresponding s-pointer;
  BT_makeTuple,
  BT_makeTupleMemo,

  //
  // Records are processed like this: first, 'takeRecordLabel' and
  // 'takeRecordArity' subsequently accumulate a label and arity list
  // respectively. 'takeRecordArity' issues 'makeRecord_intermediate'
  // what constructs the record and issues 'recordArg's. The thing is
  // called "intermediate" since it's applied when a first subtree
  // arrives; thus, it does the construction job and immediately
  // proceeds to the actual topmost task (which is 'recordArg'). Note
  // that "intermediate" tasks could work also for other data
  // structures, provided they (a) preserve "value" argument of
  // 'buildValue', (b) don't go iterate on their own, and (c) simulate
  // another 'buildValue' with 'value' saved (thus, go to the beginning 
  // of 'buildValueOutline');
  BT_takeRecordLabel,
  BT_takeRecordLabelMemo,
  BT_takeRecordArity,
  BT_takeRecordArityMemo,

  BT_makeRecord_intermediate,
  BT_makeRecordMemo_intermediate,
  //
  BT_recordArg,
  BT_recordArg_iterate,

  //
  // Dictionaries are yet more complicated. 'DIF_DICT/n' creates an
  // empty dictionary and 'n' 'BT_dictKey' tasks. 'dictKey' refers the
  // dictionary and issues a 'dictVal' task. 'dictVal' refers both the
  // dictionary and the key and puts a value into the dictionary;
  BT_dictKey,
  BT_dictVal,

  //
  BT_fsetvalue,
  BT_fsetvalueMemo,
  BT_fsetvalueFinalMemo,
  BT_fsetvalueFinal,
  // 
  BT_chunk,
  BT_chunkMemo,
  //
  BT_classFeatures,
  //
  BT_procFile,
  BT_procFileMemo,
  BT_proc,
  BT_procMemo,
  BT_closureElem,
  BT_closureElem_iterate,

  //
  // Dealing with binary areas (e.g. code areas);
  BT_binary,
  // an Oz value to appear in the stream after a binary area. The task
  // contains a "processor" procedure and its (opaque) argument (see
  // also the comments for 'Builder::getOzValue()');
  BT_binary_getValue,
  // ... the same but process it first when it's built up (using an
  // intermediate task, as one would expect):
  BT_binary_getCompleteValue,
  BT_binary_getValue_intermediate,

  //
  BT_NOTASK
};

//
static const int bsFrameSize = 3;
typedef StackEntry BTFrame;
// ... and stack entries are actually 'void*' (see stack.hh);
//

//
#ifdef DEBUG_CHECK
#define BUILDER_RINGBUFFER_FRAME        (bsFrameSize+1)
#define BUILDER_RINGBUFFER_ENTRIES	64
#define BUILDER_RINGBUFFER_BUFSIZE	64*(bsFrameSize+1)
// A ring buffer for the builder: it keeps track of recently processed
// tasks.

//
// 
static char* builderTaskNames[] = {
  "spointer",
  "spointer_iterate",
  "buildValue",
  "makeTuple",
  "makeTupleMemo",
  "takeRecordLabel",
  "takeRecordLabelMemo",
  "takeRecordArity",
  "takeRecordArityMemo",
  "makeRecord_intermediate",
  "makeRecordMemo_intermediate",
  "recordArg",
  "recordArg_iterate",
  "dictKey",
  "dictVal",
  "fsetvalue",
  "fsetvalueMemo",
  "fsetvalueFinalMemo",
  "fsetvalueFinal",
  "chunk",
  "chunkMemo",
  "classFeatures",
  "procFile",
  "procFileMemo",
  "proc",
  "procMemo",
  "closureElem",
  "closureElem_iterate",
  "binary",
  "binary_getValue",
  "binary_getCompleteValue",
  "binary_getValue_intermediate"
};

//
class BuilderRingBuffer {
private:
  int cnt;
  int current;
  StackEntry buf[BUILDER_RINGBUFFER_BUFSIZE];

  //
public:
  void init() {
    cnt = current = 0;
    for (int i = 0; i < BUILDER_RINGBUFFER_BUFSIZE; i++)
      buf[i] = 0;
  }
  BuilderRingBuffer() {
    Assert(BUILDER_RINGBUFFER_BUFSIZE % BUILDER_RINGBUFFER_FRAME == 0);
    init();
  }

  //
  void save(StackEntry *se,
	    BuilderTaskType type, StackEntry e1, StackEntry e2) {
    cnt++;
    current %= BUILDER_RINGBUFFER_BUFSIZE;
    buf[current++] = se;
    buf[current++] = ToPointer(type);
    buf[current++] = e1;
    buf[current++] = e2;
  }

  //
  BuilderTaskType getLastType() {
    return ((BuilderTaskType) int(buf[current-4]));
  }

  //
  void print(int n) {
    int index = current;	// to be allocated;
    n = max(n, 0);
    n = min(n, cnt);
    n = min(n, BUILDER_RINGBUFFER_ENTRIES);
    fprintf(stdout, "Builder's ring buffer (%d tasks ever recorded):\n",
	    cnt);

    //
    while (n--) {
      index -= BUILDER_RINGBUFFER_FRAME;
      if (index < 0) 
	index = BUILDER_RINGBUFFER_BUFSIZE - BUILDER_RINGBUFFER_FRAME;
      //
      int maybetask = (int32) buf[index+1];
      if (maybetask >= 0 && maybetask < BT_NOTASK) {
	char *name = builderTaskNames[maybetask];
	fprintf(stdout, " frame(%p) e0=%p (%s?), e1=%p, e2=%p\n",
		buf[index], buf[index+1], name, buf[index+2], buf[index+3]);
      } else {
	fprintf(stdout, " frame(%p) e0=%p, e1=%p, e2=%p\n",
		buf[index], buf[index+1], buf[index+2], buf[index+3]);
      }
    }

    //
    fflush(stdout);
  }
};
#endif

//
#define ReplaceBTTask(frame,type)			\
{							\
  *(frame-1) = ToPointer(type);				\
}
#define ReplaceBTTask1stArg(frame,type,uintArg)		\
{							\
  *(frame-1) = ToPointer(type);				\
  *(frame-2) = ToPointer(uintArg);			\
}
#define ReplaceBTTask1stPtrOnly(frame,ptr)		\
{							\
  *(frame-2) = ptr;					\
}
#define ReplaceBTTask1Ptr(frame,type,ptr)		\
{							\
  *(frame-1) = ToPointer(type);				\
  *(frame-2) = ptr;					\
}
#define ReplaceBTTaskPtrArg(frame,type,ptrArg,uintArg)	\
{							\
  *(frame-1) = ToPointer(type);				\
  *(frame-2) = ptrArg;					\
  *(frame-3) = ToPointer(uintArg);			\
}
#define ReplaceBTTask2ndArg(frame,type,uintArg)		\
{							\
  *(frame-1) = ToPointer(type);				\
  *(frame-3) = ToPointer(uintArg);			\
}
//
#define ReplaceBTFrame1stArg(frame,arg)			\
{							\
  *(frame-1) = ToPointer(arg);				\
}
#define ReplaceBTFrame2ndPtr(frame,ptr)			\
{							\
  *(frame-2) = ptr;					\
}
#define ReplaceBTFrame2ndArg(frame,arg)			\
{							\
  *(frame-2) = ToPointer(arg);				\
}
#define ReplaceBTFrame3rdArg(frame,arg)			\
{							\
  *(frame-3) = ToPointer(arg);				\
}

//
// Separated 'GetBTFrame'/'getType'/'getArg'/'discardBTFrame'/...
// Proceed with care...
#define GetBTFrame(frame)				\
  BTFrame *frame = getTop();

#define GetBTTaskType(frame, type)			\
  BuilderTaskType type =			       	\
    (BuilderTaskType) ToInt32(*(frame-1));		\
  DebugCode(ringbuf.save(frame, type,			\
            *(frame-2), *(frame-3)););
#define GetBTTaskTypeNoDecl(frame, type)		\
  type = (BuilderTaskType) ToInt32(*(frame-1));		\
  DebugCode(ringbuf.save(frame, type,			\
            *(frame-2), *(frame-3)););

#define GetBTTaskArg1(frame, ATYPE, arg)		\
  ATYPE arg = (ATYPE) ToInt32(*(frame-2));
#define GetBTTaskPtr1(frame, PTYPE, ptr)		\
  PTYPE ptr = (PTYPE) *(frame-2);
#define GetBTTaskArg1NoDecl(frame, ATYPE, arg)		\
  arg = (ATYPE) ToInt32(*(frame-2));
#define GetBTTaskPtr1NoDecl(frame, PTYPE, ptr)		\
  ptr = (PTYPE) *(frame-2);
#define GetBTTaskArg2(frame, ATYPE, arg)		\
  ATYPE arg = (ATYPE) ToInt32(*(frame-3));
#define GetBTTaskPtr2(frame, PTYPE, ptr)		\
  PTYPE ptr = (PTYPE) *(frame-3);

#define DiscardBTFrame(frame)				\
  frame = frame - bsFrameSize;
#define DiscardBT2Frames(frame)				\
  frame = frame - bsFrameSize - bsFrameSize;

// Special: lookup the type of the next frame...
#define GetBTNextTaskType(frame, type)			\
  BuilderTaskType type = (BuilderTaskType)		\
    ToInt32(*(frame - bsFrameSize - 1));		\
  DebugCode(ringbuf.save(frame-bsFrameSize, type,	\
            *(frame - bsFrameSize - 2),			\
            *(frame - bsFrameSize - 3)););
#define GetBTNextTaskArg1(frame, ATYPE, arg)		\
  ATYPE arg = (ATYPE) ToInt32(*(frame - bsFrameSize - 2));
#define GetBTNextTaskPtr1(frame, PTYPE, ptr)		\
  PTYPE ptr = (PTYPE) *(frame - bsFrameSize - 2);

//
#define GetBTFrameArg1(frame, ATYPE, arg)		\
  ATYPE arg = (ATYPE) ToInt32(*(frame-1));
#define GetBTFramePtr1(frame, PTYPE, ptr)		\
  PTYPE ptr = (PTYPE) *(frame-1);
#define GetBTFrameArg2(frame, ATYPE, arg)		\
  ATYPE arg = (ATYPE) ToInt32(*(frame-2));
#define GetBTFramePtr2(frame, PTYPE, ptr)		\
  PTYPE ptr = (PTYPE) *(frame-2);
#define GetBTFrameArg3(frame, ATYPE, arg)		\
  ATYPE arg = (ATYPE) ToInt32(*(frame-3));
#define GetBTFramePtr3(frame, PTYPE, ptr)		\
  PTYPE ptr = (PTYPE) *(frame-3);

#define GetNextBTFrameArg1(frame, ATYPE, arg)		\
  ATYPE arg = (ATYPE) ToInt32(*(frame - bsFrameSize - 1));

#define EnsureBTSpace(frame,n)				\
  frame = ensureFree(frame, n * bsFrameSize);
#define EnsureBTSpace1Frame(frame)			\
  frame = ensureFree(frame, bsFrameSize);
#define SetBTFrame(frame)				\
  setTop(frame);

#define PutBTTask(frame,type)				\
{							\
  DebugCode(*(frame) = ToPointer(0xffffffff););		\
  DebugCode(*(frame+1) = ToPointer(0xffffffff););	\
  *(frame+2) = ToPointer(type);				\
  frame = frame + bsFrameSize;				\
}
#define PutBTTaskPtr(frame,type,ptr)			\
{							\
  DebugCode(*(frame) = ToPointer(0xffffffff););		\
  *(frame+1) = ptr;					\
  *(frame+2) = ToPointer(type);				\
  frame = frame + bsFrameSize;				\
}
#define PutBTTaskArg(frame,type,arg)			\
{							\
  DebugCode(*(frame) = ToPointer(0xffffffff););		\
  *(frame+1) = ToPointer(arg);				\
  *(frame+2) = ToPointer(type);				\
  frame = frame + bsFrameSize;				\
}
#define PutBTTask2Ptrs(frame,type,ptr1,ptr2)		\
{							\
  *(frame) = ptr2;					\
  *(frame+1) = ptr1;					\
  *(frame+2) = ToPointer(type);				\
  frame = frame + bsFrameSize;				\
}
#define PutBTTaskPtrArg(frame,type,ptr,arg)		\
{							\
  *(frame) = ToPointer(arg);				\
  *(frame+1) = ptr;					\
  *(frame+2) = ToPointer(type);				\
  frame = frame + bsFrameSize;				\
}

//
#define PutBTEmptyFrame(frame)				\
{							\
  DebugCode(*(frame) = ToPointer(0xffffffff););		\
  DebugCode(*(frame+1) = ToPointer(0xffffffff););	\
  DebugCode(*(frame+2) = ToPointer(0xffffffff););	\
  frame = frame + bsFrameSize;				\
}
#define PutBTFramePtr(frame,ptr)			\
{							\
  DebugCode(*(frame) = ToPointer(0xffffffff););		\
  DebugCode(*(frame+1) = ToPointer(0xffffffff););	\
  *(frame+2) = ptr;					\
  frame = frame + bsFrameSize;				\
}
#define PutBTFrameArg(frame,arg)			\
{							\
  DebugCode(*(frame) = ToPointer(0xffffffff););		\
  DebugCode(*(frame+1) = ToPointer(0xffffffff););	\
  *(frame+2) = ToPointer(arg);				\
  frame = frame + bsFrameSize;				\
}
#define PutBTFrame2Ptrs(frame,ptr1,ptr2)		\
{							\
  DebugCode(*(frame) = ToPointer(0xffffffff););		\
  *(frame+1) = ptr2;					\
  *(frame+2) = ptr1;					\
  frame = frame + bsFrameSize;				\
}
#define PutBTFramePtrArg(frame,ptr,arg)			\
{							\
  DebugCode(*(frame) = ToPointer(0xffffffff););		\
  *(frame+1) = ToPointer(arg);				\
  *(frame+2) = ptr;					\
  frame = frame + bsFrameSize;				\
}
#define PutBTFrame2PtrsArg(frame,ptr1,ptr2,arg)		\
{							\
  *(frame) = ToPointer(arg);				\
  *(frame+1) = ptr2;					\
  *(frame+2) = ptr1;					\
  frame = frame + bsFrameSize;				\
}
#define PutBTFrame2Args(frame,arg1,arg2)		\
{							\
  DebugCode(*(frame) = ToPointer(0xffffffff););		\
  *(frame+1) = ToPointer(arg2);				\
  *(frame+2) = ToPointer(arg1);				\
  frame = frame + bsFrameSize;				\
}
#define PutBTFrame3Args(frame,arg1,arg2,arg3)		\
{							\
  *(frame) = ToPointer(arg3);				\
  *(frame+1) = ToPointer(arg2);				\
  *(frame+2) = ToPointer(arg1);				\
  frame = frame + bsFrameSize;				\
}

//
class BuilderStack : protected Stack {
public:
  BuilderStack() : Stack(GT_STACKSIZE*bsFrameSize, Stack_WithMalloc) {}
  ~BuilderStack() {}

  //
  StackEntry *getTop()            { Assert(tos >= array); return (tos); }
  StackEntry *getBottom()         { return (array); }
  void setTop(StackEntry *newTos) { 
    tos = newTos;
    checkConsistency();
  }
  void clear() { tos = array; }
  //
  StackEntry *ensureFree(StackEntry *frame, int n)
  {
    if (stackEnd <= frame + n) {
      setTop(frame);
      resize(n);
      frame = tos;
    }
    return (frame);
  }

  //
  void putTask(BuilderTaskType type, void* ptr1, uint32 arg2) {
    StackEntry *newTop = Stack::ensureFree(bsFrameSize);
    *(newTop) = ToPointer(arg2);
    *(newTop+1) = ptr1;
    *(newTop+2) = ToPointer(type);
    setTop(newTop + bsFrameSize);
  }
  void putTask(BuilderTaskType type, uint32 arg1, uint32 arg2) {
    StackEntry *newTop = Stack::ensureFree(bsFrameSize);
    *(newTop) = ToPointer(arg2);
    *(newTop+1) = ToPointer(arg1);
    *(newTop+2) = ToPointer(type);
    setTop(newTop + bsFrameSize);
  }
  void putTask(BuilderTaskType type, void* ptr) {
    StackEntry *newTop = Stack::ensureFree(bsFrameSize);
    DebugCode(*(newTop) = ToPointer(0xffffffff););
    *(newTop+1) = ptr;
    *(newTop+2) = ToPointer(type);
    setTop(newTop + bsFrameSize);
  }
  void putTask(BuilderTaskType type, void* ptr1, void* ptr2) {
    StackEntry *newTop = Stack::ensureFree(bsFrameSize);
    *(newTop) = ptr2;
    *(newTop+1) = ptr1;
    *(newTop+2) = ToPointer(type);
    setTop(newTop + bsFrameSize);
  }
  void putTask(BuilderTaskType type, uint32 arg) {
    StackEntry *newTop = Stack::ensureFree(bsFrameSize);
    DebugCode(*(newTop) = ToPointer(0xffffffff););
    *(newTop+1) = ToPointer(arg);
    *(newTop+2) = ToPointer(type);
    setTop(newTop + bsFrameSize);
  }
  void putTask2ndArg(BuilderTaskType type, uint32 arg) {
    StackEntry *newTop = Stack::ensureFree(bsFrameSize);
    *(newTop) = ToPointer(arg);
    DebugCode(*(newTop+1) = ToPointer(0xffffffff););
    *(newTop+2) = ToPointer(type);
    setTop(newTop + bsFrameSize);
  }
  void putTask(BuilderTaskType type) {
    StackEntry *newTop = Stack::ensureFree(bsFrameSize);
    DebugCode(*(newTop) = ToPointer(0xffffffff););
    DebugCode(*(newTop+1) = ToPointer(0xffffffff););
    *(newTop+2) = ToPointer(type);
    setTop(newTop + bsFrameSize);
  }

  //
#ifdef DEBUG_CHECK
  void print(int n) {
    StackEntry *se = getTop();
    n = max(n, 0);
    fprintf(stdout, "Builder's stack:\n");

    //
    while (n-- && se > array) {
      StackEntry sse = se;
      StackEntry e0 = *(--se);
      StackEntry e1 = *(--se);
      StackEntry e2 = *(--se);
      int maybetask = (int32) e0;
      if (maybetask >= 0 && maybetask < BT_NOTASK) {
	char *name = builderTaskNames[maybetask];
	fprintf(stdout, " frame(%p) e0=%p (%s?), e1=%p, e2=%p\n",
		sse, e0, name, e1, e2);
      } else {
	fprintf(stdout, " frame(%p) e0=%p, e1=%p, e2=%p\n",
		sse, e0, e1, e2);
      }
    }    

    //
    fflush(stdout);
  }
#endif  
};

//
// That's also a piece of history (former 'RefTable');
class TermTable {
  OZ_Term *array;
  int size;
  int last_index; // used for robust marshaler
  int last_set_index;
public:
  TermTable() {
    size     = 100;
    array    = new OZ_Term[size];
    last_index = -1; // used for robust marshaler
  }

  void resize(int newsize) {
    int oldsize = size;
    OZ_Term  *oldarray = array;
    while(size <= newsize) {
      size = (size*3)/2;
    }
    array = new OZ_Term[size];
    for (int i=0; i<oldsize; i++) {
      array[i] = oldarray[i];
    }
    delete oldarray;
  }

  //
  OZ_Term get(int i) {
    Assert(i < size);
    return (array[i]);
  }
  void set(OZ_Term val, int pos) {
    Assert(pos >= 0);
    if (pos>=size) 
      resize(pos);
    array[pos] = val;
  }

  // For robust unmarshaling. To check that RefTags occurs in order.
  Bool checkNewIndex(int index) {
    int result = (index - last_index) == 1;
    last_index = index;
    return result;
  }
  // For robust unmarshaling. To check that ref refers to known value.
  Bool checkIndexFound(int ref) {
    return ref <= last_index;
  }
  void resetIndexChecker() {
    last_index = -1;
  }
#ifdef DEBUG_CHECK
  void resetTT() {
    for (int i = 0; i < size; i++) 
      array[i] = (OZ_Term) 0;
  }
#endif  

};



//
// Dealing with binary areas requires a temporary stack used within
// 'buildValueOutline'. The problem is that some 'build' methods, for
// instance - 'buildProc()', do not create a value and match it
// against a "purpose" task from the builder's stack; instead, a value
// is created later and matched against its task using (usally)
// "_iterate" tasks. However, instead of a "purpose" task a "binary"
// one can occur (when the user has declared an area): these "binary"
// tasks are saved until 'buildValueOutline" finishes;
//
// Unfortunately, the depth of this stack depends on a data structure
// being built: observe that a procedure can be a last closure element
// of another procedure, recursively, causing crossing of multiple
// 'BT_binary' entries per 'buildValueOutline()' call;
//
typedef void* BAStackEntry;
//
class BuilderAuxStack : private Stack {
public:
  BuilderAuxStack() : Stack(16, Stack_WithMalloc) {}
  ~BuilderAuxStack() {}

  //
  Bool isEmpty() { return (tos == array); }

  //
  void save(void* value) {
    checkConsistency();
    ensureFree(1);
    *tos = value;
    tos++;
  }
  void* pop() {
    Assert(tos != array);
    tos--;
    return (*tos);
  }
};

//
typedef int BuilderOpaqueBA;
typedef void (*OzValueProcessor)(void *arg, OZ_Term value);

//
class Builder : private BuilderStack, public TermTable {
private:
  CrazyDebug(int debugNODES;);
  OZ_Term result;		// used as a "container";
  OZ_Term blackhole;		// ... for discarding stuff;
  BuilderAuxStack binaryTasks;	// (see the comments for 'BuilderAuxStack';)
  DebugCode(BuilderRingBuffer ringbuf;);

private:
  CrazyDebug(void incDebugNODES() { debugNODES++; });
  CrazyDebug(void decDebugNODES() { debugNODES--; });

  //
  void buildValueOutline(OZ_Term value, BTFrame *frame,
			 BuilderTaskType type);
  void buildValueOutlineRobust(OZ_Term value, BTFrame *frame,
			       BuilderTaskType type);

  //
public:
  Builder() : result((OZ_Term) 0), blackhole((OZ_Term) 0) {}
  ~Builder() {}

  //
  // begin building:
  void build() {
    // kost@ : don't run 'resetTT()' in general, because things like
    // 'unmarshalFullObjectAndClass()' require the table to be passed
    // between builder's steps. It is OK to enable it when checking 
    // plain pickles.
    // DebugCode(resetTT(););
    CrazyDebug(debugNODES = 0;);
    DebugCode(ringbuf.init(););
    putTask(BT_spointer, &result);
  }
  // returns '0' if inconsistent:
  OZ_Term finish() {
    resetIndexChecker();
    if (isEmpty()) {
      // CrazyDebug(fprintf(stdout, " --- %d nodes.\n", debugNODES););
      // CrazyDebug(fflush(stdout););
      return (result);
    } else {
      // may be empty binary task(s)?
      while (1) {
	GetBTFrame(frame);
	GetBTTaskType(frame, type);
	GetBTTaskPtr1(frame, void*, bp);
	//
	if (type == BT_binary && bp == (void *) 0) {
	  DiscardBTFrame(frame);
	  SetBTFrame(frame);
	} else {
	  break;
	}
      }

      //     
      if (isEmpty()) {
	// CrazyDebug(fprintf(stdout, " --- %d nodes.\n", debugNODES););
	// CrazyDebug(fflush(stdout););
	return (result);
      } else {
	clear();		// do it eagerly - presumably seldom;
	return ((OZ_Term) 0);
      }
    }
  }
  void stop() { clear(); }

  //
  void buildTuple(int arity) {
    putTask(BT_makeTuple, arity);
  }
  void buildTupleRemember(int arity, int n) {
    putTask(BT_makeTupleMemo, arity, n);
  }

  //
  // Tasks's frame will be extended for label and arity;
  void buildRecord() {
    GetBTFrame(frame);
    EnsureBTSpace(frame, 2);
    PutBTEmptyFrame(frame);
    PutBTTask(frame, BT_takeRecordLabel);
    SetBTFrame(frame);
  }
  void buildRecordRemember(int n) {
    GetBTFrame(frame);
    EnsureBTSpace(frame, 2);
    PutBTFrameArg(frame, n);
    PutBTTask(frame, BT_takeRecordLabelMemo);

    // Setting the slot to a 'GC' TaggedRef is exploited later by
    // '_intermediate' tasks to reassign a proper value passed over
    // that '_intermediate' task;
    set(makeGCTaggedInt(n), n);
    SetBTFrame(frame);
  }

  //
  void buildFSETValue() {
    putTask(BT_fsetvalue);
  }
  void buildFSETValueRemember(int n) {
    putTask(BT_fsetvalueMemo, n);
  }

  //
  // New chunks/classes/procedures are built with 'buildXXX()' while
  // those that are already imported are placed with 'knownXXX()'.
  // Note that one cann't do 'buildValue()' instead of 'knownXXX()'
  // because the later one discards also the unused terms!
  void buildChunk(GName *gname) {
    Assert(gname);
    putTask(BT_chunk, gname);
  }
  void buildChunkRemember(GName *gname, int n) {
    Assert(gname);
    putTask(BT_chunkMemo, gname, n);
  }
  // knownChunk se general_builder_methods.hh

  //
  void buildClass(GName *gname, int flags) {
    Assert(gname);

    //      
    ObjectClass *cl = new ObjectClass(makeTaggedNULL(), 
				      makeTaggedNULL(),
				      makeTaggedNULL(), 
				      makeTaggedNULL(), NO, NO,
				      am.currentBoard());
    cl->setGName(gname);
    OZ_Term classTerm = makeTaggedConst(cl);
    addGName(gname, classTerm);

    //
    putTask(BT_classFeatures, cl, flags);
  }

  //
  void buildClassRemember(GName *gname, int flags, int n) {
    Assert(gname);

    //      
    ObjectClass *cl = new ObjectClass(makeTaggedNULL(), 
				      makeTaggedNULL(),
				      makeTaggedNULL(), 
				      makeTaggedNULL(), NO, NO,
				      am.currentBoard());
    cl->setGName(gname);
    OZ_Term classTerm = makeTaggedConst(cl);
    addGName(gname, classTerm);
    //
    set(classTerm, n);

    putTask(BT_classFeatures, cl, flags);
  }

  //
  // Procedures are "more" interesting... they are done in two phases,
  // of which the second one deals with the code area and can contain
  // a number of sub-steps. First, the name of the procedure arrives,
  // which will match the 'proc' task. This task creates the procedure
  // but its 'pc' field, and yet pushes two tasks (in this order) - a
  // 'procSecondary' that will update procedure's 'pc', and a 'binary'
  // task, that provides for filling the code area. The later one
  // matches the 'DIF_CODEAREA' from the stream.
  void buildProc(GName *gname, int arity,
		 int gsize, int maxX, int line, int column,
		 ProgramCounter pc) {
    Assert(gname);
    GetBTFrame(frame);
    EnsureBTSpace(frame, 4);
    PutBTFrame2Args(frame, arity, gsize);
    PutBTFrame3Args(frame, maxX, line, column);
    PutBTFrame2Ptrs(frame, gname, pc);
    PutBTTask(frame, BT_procFile);
    SetBTFrame(frame);
  }
  void buildProcRemember(GName *gname, int arity,
			 int gsize, int maxX, int line, int column,
			 ProgramCounter pc, int memoIndex) {
    Assert(gname);
    GetBTFrame(frame);
    EnsureBTSpace(frame, 4);
    PutBTFrame2Args(frame, arity, gsize);
    PutBTFrame3Args(frame, maxX, line, column);
    PutBTFrame2PtrsArg(frame, gname, pc, memoIndex);
    PutBTTask(frame, BT_procFileMemo);
    SetBTFrame(frame);
  }

  //
  // An Oz value can contain a binary area that can contain references
  // to (other) Oz values and that can be split into pieces. The
  // unmarshaler can declare such an area using 'buildBinary':
  void buildBinary(void *binaryAreaDesc) {
    // Zero arguments are not allowed since they are used internally;
    Assert(binaryAreaDesc);
    putTask(BT_binary, binaryAreaDesc);
  }

  //
  // When a fragment of marshaled binary area begins in the stream the
  // unmarshaler is supposed to know its type (e.g. using a dedicated
  // 'DIF' header, aka 'DIF_CODEAREA' for code areas) while the
  // abstract argument for its processing is supplied by the builder
  // using 'fillBinary()':
  void* fillBinary(BuilderOpaqueBA &opaque) {
    CrazyDebug(incDebugNODES(););
    GetBTFrame(frame);
    void *bp;
    // Even if there are some empty binary tasks, these were binary
    // ones:
    DebugCode(GetBTTaskType(frame, type););
    Assert(type == BT_binary);

    //
    while (1) {
      GetBTTaskPtr1NoDecl(frame, void*, bp);
      if (bp) {
	break;		// found;
      } else {
	DiscardBTFrame(frame);
	SetBTFrame(frame);
      }
    }
    // 'bp' can be zero meaning we are discarding the definition;
    opaque = (BuilderOpaqueBA) (ToInt32(frame) - ToInt32(getBottom()));
    return (bp);
  }

  //
  // Binary areas can contain references to Oz values that will appear
  // later in the stream. The builder can be instructed to process
  // them using 'getOzValue()' method.
  //
  // Observe that 'proc' will be applied when a value in the stream is
  // reached (but not completely built up - because of top-down
  // building process);
  //
  // 'proc' must take of 'arg' by itself. For the builder it is really
  // an opaque value;
  //
  void getOzValue(OzValueProcessor proc, void *arg) {
    putTask(BT_binary_getValue, (void *) proc, arg);
  }

  //
  // ... however, it certain cases the value must be processed first
  // when it's built up. Thus we have also 'getCompleteOzValue()':
  void getCompleteOzValue(OzValueProcessor proc, void *arg) {
    GetBTFrame(frame);
    EnsureBTSpace(frame, 2);
    PutBTFrame2Ptrs(frame, proc, arg);
    PutBTTask(frame, BT_binary_getCompleteValue);
    SetBTFrame(frame);
  }

  //
  // Sometimes a value from a stream just is to be stored just at a
  // pointer:
  void getOzValueLoc(OZ_Term *ptr) {
    putTask(BT_spointer, ptr);
  }

  //
  // In "discard" mode, a value that will appear (in the stream) is
  // not needed (and we don't need any special processing for it).  In
  // the end, if the value is not needed but nevertheless must be
  // processed, everyone is free to define its own processor;
  void discardOzValueCA() {
    putTask(BT_spointer, &blackhole);
  }

  // 
  // User can interrupt filling the area with 'suspendFillBinary()'.
  // This is needed when a term representation(s) will appear next in
  // the stream (of terms previously declared with 'getOzValue()'):
  void suspendFillBinary(BuilderOpaqueBA opaque) {
    // That's a NOP: the task remains in place;
    DebugCode(BTFrame* frame = (BTFrame *) (ToInt32(getBottom()) + opaque););
    DebugCode(GetBTTaskType(frame, type););
    Assert(type == BT_binary);
  }

  //
  // This guy either disposes the (top-level) 'fillArea' task or
  // invalidates it, so that 'buildValueOutline()' will get rid of it;
  void finishFillBinary(BuilderOpaqueBA opaque) {
    // trust user...
    BTFrame* frame = (BTFrame *) (ToInt32(getBottom()) + opaque);
    GetBTFrame(realBTFrame);
    //
    DebugCode(GetBTTaskType(frame, type););
    Assert(type == BT_binary);

    //
    if (realBTFrame == frame) {
      // able to get rid of it now;
      DiscardBTFrame(frame);
      SetBTFrame(frame);
    } else {
      ReplaceBTTask1stPtrOnly(frame, 0);
    }
  }

#include "robust_builder_methods.hh"
#include "fast_builder_methods.hh"


  //
#ifdef DEBUG_CHECK
  void prRB(int n = BUILDER_RINGBUFFER_ENTRIES) { ringbuf.print(n); }
  void prST(int n = 100000) { BuilderStack::print(n); }
  void pr() { 
    prST();
    prRB();
  }
#endif  
};

#endif
