/*
 *  Authors:
 *    Michael Mehl (mehl@dfki.de)
 *
 *  Contributors:
 *    Ralf Scheidhauer (Ralf.Scheidhauer@ps.uni-sb.de)
 *    Tobias Mueller (tmueller@ps.uni-sb.de)
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
 *     http://mozart.ps.uni-sb.de
 *
 *  See the file "LICENSE" or
 *     http://mozart.ps.uni-sb.de/LICENSE.html
 *  for information on usage and redistribution
 *  of this file, and for a DISCLAIMER OF ALL
 *  WARRANTIES.
 *
 */

#ifndef __BASEH
#define __BASEH

#ifdef INTERFACE
#pragma interface
#endif

#ifdef HAVE_CONFIG_H
#include "conf.h"
#endif

#include "machine.hh"
#include "resources.hh"
#include "config.h"
#include "ozostream.hh"

#include <string.h>

// more includes at end!

#if !defined(__GNUC__) && !defined(NULL)
# define NULL 0
#endif

const unsigned int KB = 1024;
const unsigned int MB = KB*KB;

const int WordSize = sizeof(void*);

// see print.cc
#ifdef DEBUG_PRINT
#define OZPRINT                                                              \
  void printStream(ostream &stream=cout, int depth = 10);    \
  void printLongStream(ostream &stream=cout, int depth = 10, int offset = 0) \
    { printStream(stream,depth); stream << endl; }                   \
  void print(void)                                                           \
    { printStream(cerr); cerr << endl; cerr.flush(); }               \
  void printLong(void)                                                       \
    { printLongStream(cerr); cerr.flush(); }

#define OZPRINTLONG                                                           \
  void printStream(ostream &stream=cout, int depth = 10);     \
  void printLongStream(ostream &stream=cout, int depth = 10, int offset = 0); \
  void print(void)                                                            \
    { printStream(cerr); cerr << endl; cerr.flush(); }                \
  void printLong(void)                                                        \
    { printLongStream(cerr); cerr.flush(); }
#else
#define OZPRINT
#define OZPRINTLONG
#endif

inline int min(int a, int b) {return a < b ? a : b;}
inline int max(int a, int b) {return a > b ? a : b;}

inline int ozabs(int a) {return a > 0 ? a : -a;}
inline float ozabs(float a) {return a > 0 ? a : -a;}

#define Swap(A,B,Type) { Type help=A; A=B; B=help; }

typedef int Bool;
const Bool NO = 0;
const Bool OK = 1;

/* AIX and OSF/1 define these */
#ifdef TRUE
#undef TRUE
#endif
#ifdef FALSE
#undef FALSE
#endif
const Bool TRUE  = 1;
const Bool FALSE = 0;


/*
 * special return values for builtins
 */
#define BI_PREEMPT       1024
#define BI_REPLACEBICALL 1025
#define BI_TYPE_ERROR    1026


typedef unsigned char BYTE;

typedef int32 ByteCode;

typedef ByteCode *ProgramCounter;

#define NOCODE ((ProgramCounter) -1l)

typedef int32 PosInt;
typedef PosInt Reg;

typedef unsigned int32 TaggedRef;

enum PropCaller {pc_propagator = 0, pc_std_unif = 1, pc_cv_unif = 2};

// duplicated from oz.h !!! Not nice, but ...
typedef unsigned int OZ_Term;
typedef unsigned int OZ_Return;
extern "C" {
  typedef int (* OZ_IOHandler)(int,void *);
}

typedef OZ_Return (*InlineRel1)(TaggedRef In1);
typedef OZ_Return (*InlineRel2)(TaggedRef In1, TaggedRef In2);
typedef OZ_Return (*InlineRel3)(TaggedRef In1, TaggedRef In2, TaggedRef In3);
typedef OZ_Return (*InlineFun1)(TaggedRef In1, TaggedRef &Out);
typedef OZ_Return (*InlineFun2)(TaggedRef In1, TaggedRef In2,
                                    TaggedRef &Out);
typedef OZ_Return (*InlineFun3)(TaggedRef In1, TaggedRef In2,
                                    TaggedRef In3, TaggedRef &Out);

//  ------------------------------------------------------------------------

/* some macros to help debugging
   DebugCheck:  if 'precondition' then print file and line and execute body
   DebugCheckT: check without precondition
   Assert:      issue an error if Cond is not fulfilled
   */

#ifdef DEBUG_CHECK
#define WHERE(file)                                                           \
  fprintf(file,"%s:%d ",__FILE__,__LINE__);

#define DebugCheck(Cond,Then)                                                 \
     if (Cond) { WHERE(stderr);Then;}

#define DebugCheckT(Then) Then

#define DebugCode(C) C

#define Assert(Cond)                                                          \
  if (! (Cond)) {                                                             \
    WHERE(stderr);                                                            \
    error(" assertion '%s' failed", #Cond);                                   \
  }
#else
#define WHERE(file)
#define DebugCheck(Cond,Then)
#define DebugCheckT(Then)
#define DebugCode(C)
#define Assert(Cond)
#endif


#ifdef DEBUG_TRACE
#define DebugTrace(Command) Command
#else
#define DebugTrace(Command)
#endif


#ifdef DEBUG_FD
#define DebugFD(Cond,Then) if (Cond) {Then;}
#else
#define DebugFD(Cond,Then)
#endif

#ifdef DEBUG_GC
#define DebugGC(Cond,Then) if (Cond) {Then;}
#define DebugGCT(Then) Then
#else
#define DebugGC(Cond,Then)
#define DebugGCT(Then)
#endif

//  ------------------------------------------------------------------------

/* Avoid that the compiler generates constructors, destructors and
 * assignment operators which are not wanted in Oz */
#define NO_DEFAULT_CONSTRUCTORS2(aclass)        \
  ~aclass();                                    \
  aclass(const aclass &);                       \
  aclass &operator = (const aclass&)

#define NO_DEFAULT_CONSTRUCTORS1(aclass)        \
  NO_DEFAULT_CONSTRUCTORS2(aclass);             \
  aclass()

#define NO_DEFAULT_CONSTRUCTORS(aclass) NO_DEFAULT_CONSTRUCTORS1(aclass)

/*
   Forward declarations of classes and procedures
*/

struct Equation;

class SVariable;
class GenCVariable;
class GenFDVariable;
class GenBoolVariable;
class GenFSetVariable;
class GenCtVariable;
class DynamicTable;
class SRecord;
class Arity;
class Abstraction;
class LTuple;
class Literal;
class Float;
class SmallInt;
class BigInt;
class ConstTerm;
class Cell;
class SChunk;

class Watcher;
class Tertiary;

class Port;
class PortWithStream;
class PortProxy;
class PortLocal;
class PortManager;
class ProcProxy;

class PendThread;
class PendBinding;
class CellManager;
class CellFrame;
class CellSec;
class Chain;
class ChainElem;

class OwnerEntry;
class BorrowEntry;
class OwnerTable;
class BorrowTable;
class Site;

class Builtin;

class FiniteDomain;

class OZ_FSetValue;

class Continuation;

class SuspList;
class CondSuspList;
class Suspension;
class Propagator;
class OZ_Propagator;

class CpStack;
class Thread;
class ThreadsPool;
class ThreadQueue;
class Group;
class Toplevel;
class Actor;
class AWActor;
class WaitActor;
class AskActor;
class SolveActor;
class Board;
class RunnableThreadBody;

class Script;

class Trail;

class TaskStack;
class CallList;

class LocalPropagatorQueue;

// source level debugger
enum OzDebugDoit {DBG_STEP, DBG_NOSTEP, DBG_EXIT};
class OzDebug;

class AM;
extern AM am; // the one and only engine

// assem
class CodeArea;
class PrTabEntry;

//
class BuiltinTab;

class FastQueue;
class DLLStack;

class OzSleep;
class Alarm;

class IHashTable;

class CompStream;

class Object;

class OzDictionary;

class OzLock;

class InlineCache;
class OZ_Location;

class NetAddress;
class GName;

class IONode;

// this class can be used to fake the type system,e.g. to define a second
//  constructor without arguments
class DummyClass {};

void checkGC();

// see version.sed
void version();

// see emulate.cc
int engine(Bool);
void scheduler();

// return code of the emulator
enum ThreadReturn {
  T_PREEMPT,            // thread is preempted
  T_SUSPEND,            // thread must suspend
  T_SUSPEND_ACTOR,      // thread must suspend on deep guard
  T_RAISE,              // an exception must be handled
  T_FAILURE,            // an failure exception must be handled
  T_TERMINATE,          // the thread terminated
  T_ERROR               // a fatal error occured
};

// see am.cc
void handlerUSR1();
void handlerINT();
void handlerTERM();
void handlerMessage();
void handlerSEGV();
void handlerBUS();
void handlerPIPE();
void handlerCHLD();
void handlerFPE();
void handlerALRM();
void handlerUSR2();

void oz_checkExtSuspension(Suspension susp, Board * home);

#ifdef DEBUG_STABLE
extern SuspList * board_constraints;
void printBC(ostream &, Board *);
void printBCDebug(Board * = NULL);
#endif

// printing (see foreign.cc)
void oz_printStream(OZ_Term term, ostream &out,
                    int depth=-1, int width=-1);
void oz_print(OZ_Term term);
// see also OZ_toC();
char *toC(OZ_Term);

#ifdef DEBUG_PRINT
// debug print (see print.cc)
void ozd_printStream(OZ_Term val, ostream &stream, int depth=20);
void ozd_print(OZ_Term term);
void ozd_printLongStream(OZ_Term val, ostream &stream,
                         int depth = 20, int offset = 0);
void ozd_printLong(OZ_Term term);
void ozd_printBoards();
void ozd_printThreads();
void ozd_printAM();
#endif

char *replChar(char *s,char from,char to);
char *delChar(char *s,char c);

// see perdio.cc
int perdioInit();
int loadURL(TaggedRef url, OZ_Term out, Thread *th);
int loadURL(const char *,OZ_Term,Thread *th);

// see term.cc
void initLiterals();

// see codearea.cc
void displayCode(ProgramCounter from, int ssize);
void displayDef(ProgramCounter from, int ssize);

// see builtins.cc
Builtin *BIinit();
void threadRaise(Thread *th,OZ_Term E,int debug=0);
extern OZ_Return dotInline(TaggedRef term, TaggedRef fea, TaggedRef &out);
extern OZ_Return uparrowInlineBlocking(TaggedRef term, TaggedRef fea,
                                       TaggedRef &out);
OZ_Return BIarityInline(TaggedRef, TaggedRef &);
OZ_Return adjoinPropList(TaggedRef t0, TaggedRef list, TaggedRef &out,
                             Bool recordFlag);
OZ_Return BIminusOrPlus(Bool callPlus,TaggedRef A, TaggedRef B, TaggedRef &out);
OZ_Return BILessOrLessEq(Bool callLess, TaggedRef A, TaggedRef B);

OZ_Term oz_getLocation(Board *bb);

OZ_Return oz_bi_wrapper(Builtin *bi,OZ_Term *X);

// see ??
SuspList *oz_installPropagators(SuspList *local_list, SuspList *glob_list,
                                Board *glob_home);

SuspList * oz_checkAnySuspensionList(SuspList *suspList,Board *home,
                                     PropCaller calledBy);

// see ioHandler.cc
void oz_io_select(int fd, int mode, OZ_IOHandler fun, void *val);
void oz_io_acceptSelect(int fd, OZ_IOHandler fun, void *val);
int oz_io_select(int fd, int mode,TaggedRef l,TaggedRef r);
void oz_io_acceptSelect(int fd,TaggedRef l,TaggedRef r);
void oz_io_deSelect(int fd,int mode);
void oz_io_deSelect(int fd);
void oz_io_handle();
void oz_io_check();
void oz_io_awakeVar(TaggedRef var);

// see gc.cc
Bool oz_staticProtect(TaggedRef *);
Bool oz_protect(TaggedRef *);
Bool oz_unprotect(TaggedRef *);

void OZ_collectHeapTerm(TaggedRef &, TaggedRef &);
void OZ_collectLocalHeapBlock(TaggedRef *, TaggedRef *, int);
void OZ_updateLocalHeapTerm(TaggedRef &);
void * OZ_hrealloc(void *, size_t);


/* Ultrix does not have 'strdup' */
inline char *ozstrdup(const char *s)
{
  char *ret = new char[strlen(s)+1];
  strcpy(ret,s);
  return ret;
}

template <class T>
class EnlargeableArray {
private:
  int _size;
  T * _array;
public:
  EnlargeableArray(int s) : _size(s), _array((T *) malloc(s * sizeof(T))) {}
  ~EnlargeableArray() { free(_array); }

  inline
  T &operator [](int i) {
    Assert(0 <= i && i < _size);
    return _array[i];
  }

  inline
  void request(int s, int m = 100) { // margin of 100
    if (s >= _size) {
      _size = s + m;
      _array = (T *) realloc(_array, _size * sizeof(T));
    }
  }

  inline
  operator T*() { return _array; } // conversion operator
};

//
// "free list" data manager: try to get a piece of memory from a
// free list, and fall back with the 'fdmMalloc' function;
typedef void* (*MallocFun)(size_t size);
//
template <class T>
class FreeListDataManager {
private:
  MallocFun mallocFun;
  T* freeList;

  //
public:
  FreeListDataManager(MallocFun fun)
    : mallocFun(fun), freeList((T *) NULL)
  {
    Assert(sizeof(T) >= sizeof(T*));
  }
  ~FreeListDataManager() {}

  //
  T *allocate() {
    if (freeList) {
      T *b = freeList;
      freeList = *((T **) freeList);
      return (b);
    } else {
      return ((T *) mallocFun(sizeof(T)));
    }
  }
  void dispose(T *b) {
    *((T **) b) = freeList;
    freeList = b;
  }
};

#ifdef __cplusplus

extern "C" {
  void error( const char *format ...);
  void warning( const char *format ...);
  void message( const char *format ...);
  Bool isDeadSTDOUT();
  void statusMessage( const char *format ...);
  void prefixError();
  void prefixWarning();
  void prefixStatus();
  void ozperror( const char *msg);
  void ozpwarning( const char *msg);
}

#endif

void errorHeader();
void errorTrailer();

#ifdef __GNUC__
#define NEW_TEMP_ARRAY(Type, Var, Size) Type Var[Size]

#define DELETE_TEMP_ARRAY(Var)
#else
#define NEW_TEMP_ARRAY(Type, Var, Size) Type * Var = new Type[Size]
#define DELETE_TEMP_ARRAY(Var) delete [] Var
#endif

#endif
