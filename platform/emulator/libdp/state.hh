/*
 *  Authors:
 *    Per Brand (perbrand@sics.se)
 *    Erik Klintskog (erik@sics.se)
 *
 *  Contributors:
 *    Konstantin Popov <kost@sics.se>
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

#ifndef __STATEHH
#define __STATEHH

#ifdef INTERFACE
#pragma interface
#endif

#include "base.hh"
#include "value.hh"


/**********************************************************************/
/*  SECTION: class CellSec, CellProxy, CellManager                    */
/**********************************************************************/

#define DummyThread ((Thread*)0x1)
#define MoveThread  ((Thread*)NULL)

inline Bool isRealThread(Thread* t){
  if((t==MoveThread) || (t==DummyThread)) return FALSE;
  return TRUE;}

class CellSec:public CellSecEmul{
friend class CellFrame;
friend class CellManager;
friend class Chain;
public:
  USEHEAPMEMORY;
  NO_DEFAULT_CONSTRUCTORS2(CellSec);
  CellSec(TaggedRef val){ // on globalize
    Assert(sizeof(CellSecEmul) == sizeof(CellSec));
    state=Cell_Lock_Valid;
    pending=NULL;
    next=NULL;
    contents=val;
    pendBinding=NULL;}

  CellSec(){ // on Proxy becoming Frame
    Assert(sizeof(CellSecEmul) == sizeof(CellSec));
    state=Cell_Lock_Invalid;
    pending=NULL;
    pendBinding=NULL;
    next=NULL;}

  unsigned int stateWithoutAccessBit(){return state & (~Cell_Lock_Access_Bit);}

  void addPendBinding(Thread*,TaggedRef);
  DSite* getNext(){return next;}
  PendThread** getPendBase(){return &pending;}

  void gcCellSec();
  OZ_Return exchange(Tertiary*,TaggedRef,TaggedRef,Thread*,ExKind);
  OZ_Return access(Tertiary*,TaggedRef,TaggedRef);
  OZ_Return exchangeVal(TaggedRef,TaggedRef,Thread*,TaggedRef,ExKind);

  Bool secReceiveRemoteRead(DSite*,DSite*,int);
  void secReceiveReadAns(TaggedRef);
  Bool secReceiveContents(TaggedRef,DSite* &,TaggedRef &);
  Bool secForward(DSite*,TaggedRef&);
  // failure
  Bool cellRecovery(TaggedRef);
};

class CellProxy : public Tertiary {
friend void ConstTerm::gcConstRecurse(void);
private:
  int holder; // mm2: on alpha sizeof(int) != sizeof(void *)
  void *dummy; // mm2
public:
  NO_DEFAULT_CONSTRUCTORS(CellProxy);

  CellProxy(int manager):Tertiary(manager,Co_Cell,Te_Proxy){  // on import
    holder = 0;}
};

//
// "Real" cell manager - handles cells' access structures;
class CellManager : public  CellManagerEmul {
  friend void ConstTerm::gcConstRecurse(void);
public:
  CellSec* getCellSec(){return (CellSec*) getSec();}
  NO_DEFAULT_CONSTRUCTORS2(CellManager);
  CellManager() {
    Assert(sizeof(CellManagerEmul) == sizeof(CellManager));
    Assert(0);}

  Chain *getChain() {return chain;}
  void setChain(Chain *ch) { chain = ch; }
  void initOnGlobalize(int index,Chain* ch,CellSec *secX);

  void setOwnCurrent();
  Bool isOwnCurrent();
  void init();
  DSite* getCurrent();
  void gcCellManager();

  PendThread* getPending(){return sec->pending;}
  PendThread *getPendBinding(){return sec->pendBinding;}

  // failure
  void initForFailure();
  void tokenLost();
};


class CellFrame : public CellFrameEmul {
friend void ConstTerm::gcConstRecurse(void);
public:
  CellSec* getCellSec(){return (CellSec*) getSec();}
  void setCellSec(CellSec* cs){sec=(CellSecEmul*) cs;}
  NO_DEFAULT_CONSTRUCTORS2(CellFrame);
  CellFrame(){
    Assert(0);}

  void setDumpBit(){getCellSec()->state |= Cell_Lock_Dump_Asked;}

  void resetDumpBit(){getCellSec()->state &= ~Cell_Lock_Dump_Asked;}

  Bool dumpCandidate(){
    if((getCellSec()->state & Cell_Lock_Valid)
       && (!(getCellSec()->state & Cell_Lock_Dump_Asked))){
      setDumpBit();
      return OK;}
    return NO;}

  Bool isAccessBit(){return getCellSec()->state & Cell_Lock_Access_Bit;}

  void setAccessBit(){getCellSec()->state |= Cell_Lock_Access_Bit;}

  void resetAccessBit(){getCellSec()->state &= (~Cell_Lock_Access_Bit);}

  void myStoreForward(void* f) { forward = f; }
  void* getForward()           { return forward; }

  void convertToProxy(){
    setTertType(Te_Proxy);
    sec=NULL;}

  void convertFromProxy(){
    setTertType(Te_Frame);
    sec=new CellSec();}

  void gcCellFrame();
};

/**********************************************************************/
/*  SECTION: class LockSec, LockProxy, LockManager                    */
/**********************************************************************/

class LockSec : public LockSecEmul {
public:
  NO_DEFAULT_CONSTRUCTORS2(LockSec);
  LockSec(Thread *t,PendThread *pt){ // on globalize
    Assert(sizeof(LockSecEmul) == sizeof(LockSec));
    state=Cell_Lock_Valid;
    pending=pt;
    locker=t;
    next=NULL; }

  LockSec(){ // on Proxy becoming Frame
    Assert(sizeof(LockSecEmul) == sizeof(LockSec));
    state=Cell_Lock_Invalid;
    locker=NULL;
    pending=NULL;
    next=NULL;}

  void setAccessBit(){state |= Cell_Lock_Access_Bit;}

  void resetAccessBit(){state &= ~Cell_Lock_Access_Bit;}

  Bool isPending(Thread *th);

  DSite* getNext(){return next;}

  void lockComplex(Thread* th,Tertiary*);
  void unlockComplex(Tertiary* );
  void unlockComplexB(Thread *);
  void unlockPending(Thread*);
  void gcLockSec();
  Bool secReceiveToken(Tertiary*,DSite* &);
  Bool secForward(DSite*);

  void makeRequested(){
    Assert(state==Cell_Lock_Invalid);
    state=Cell_Lock_Requested;}

  // failure
  Bool lockRecovery();
};

class LockFrame : public LockFrameEmul {
friend void ConstTerm::gcConstRecurse(void);
public:
  LockSec* getLockSec(){return (LockSec*) getSec();}
  void setLockSec(LockSec* cs){sec=(LockSecEmul*)cs;}
  NO_DEFAULT_CONSTRUCTORS2(LockFrame);
  LockFrame(){Assert(0);}

  Bool isAccessBit(){
    if(getLockSec()->state & Cell_Lock_Access_Bit) return TRUE;
    return FALSE;}

  void setAccessBit(){getLockSec()->setAccessBit();}
  void resetAccessBit(){getLockSec()->resetAccessBit();}

  void setDumpBit(){getLockSec()->state |= Cell_Lock_Dump_Asked;}
  void resetDumpBit(){getLockSec()->state &= ~Cell_Lock_Dump_Asked;}

  Bool dumpCandidate(){
    if((getLockSec()->state & Cell_Lock_Valid)
       && (!(getLockSec()->state & Cell_Lock_Dump_Asked))){
      setDumpBit();
      return OK;}
    return NO;}

  void myStoreForward(void* f) { forward=f; }
  void* getForward() { return forward; }

  void convertToProxy(){
    setTertType(Te_Proxy);
    sec=NULL;}

  void convertFromProxy(){
    setTertType(Te_Frame);
    sec=new LockSec();}

  void gcLockFrame();
};

class LockManager : public LockManagerEmul {
friend void ConstTerm::gcConstRecurse(void);
public:
  LockSec* getLockSec(){return (LockSec*) getSec();}
  NO_DEFAULT_CONSTRUCTORS2(LockManager);
  LockManager() {Assert(0);}

  void initOnGlobalize(int index,Chain* ch,LockSec *secX);
  Chain *getChain() {return chain;}
  void setChain(Chain *ch) { chain = ch; }

  void gcLockManager();
  void setOwnCurrent();
  Bool isOwnCurrent();
  void init();
  DSite* getCurrent();

  // failure
  void probeFault(DSite*, int);
  void initForFailure();
  void tokenLost();
};

class LockProxy:public OzLock{
friend void ConstTerm::gcConstRecurse(void);
private:
  int holder; // mm2: on alpha sizeof(int) != sizeof(void *)
  void *dummy; // mm2
public:
  NO_DEFAULT_CONSTRUCTORS(LockProxy);
  LockProxy(int manager):OzLock(manager,Te_Proxy){  // on import
    holder = 0;}

  void lock(Thread *);
  void unlock();
  // failure
  void probeFault(DSite*, int);
};

/**********************************************************************/
/*  SECTION: provide routines                                         */
/**********************************************************************/

// may not be 'Local';
void gcDistCell(Tertiary *t);
void gcDistLock(Tertiary *t);

void convertCellProxyToFrame(Tertiary *t);
void convertLockProxyToFrame(Tertiary *t);

inline void maybeConvertLockProxyToFrame(Tertiary *t){
  if(t->isProxy())
    {convertLockProxyToFrame(t);}}

inline void maybeConvertCellProxyToFrame(Tertiary *t){
  if(t->isProxy()){
    convertCellProxyToFrame(t);}}


Chain* getChainFromTertiary(Tertiary*);
CellSec *getCellSecFromTert(Tertiary *c);
int getStateFromLockOrCell(Tertiary*);

//
OZ_Return cellDoExchangeInternal(Tertiary *, TaggedRef, TaggedRef,
                                 Thread *, ExKind);

//
OZ_Return cellDoExchangeImpl(Tertiary *c,TaggedRef old,TaggedRef nw);
OZ_Return cellDoAccessImpl(Tertiary *c, TaggedRef val);
OZ_Return cellAtAccessImpl(Tertiary *c, TaggedRef fea, TaggedRef val);
OZ_Return cellAtExchangeImpl(Tertiary *c,TaggedRef old,TaggedRef nw);
OZ_Return cellAssignExchangeImpl(Tertiary *c,TaggedRef fea,TaggedRef val);
//
void lockLockProxyImpl(Tertiary *t, Thread *thr);
void lockLockManagerOutlineImpl(LockManagerEmul *lmu, Thread *thr);
void unlockLockManagerOutlineImpl(LockManagerEmul *lmu, Thread *thr);
void lockLockFrameOutlineImpl(LockFrameEmul *lfu, Thread *thr);
void unlockLockFrameOutlineImpl(LockFrameEmul *lfu, Thread *thr);
//
void gcDistCellRecurseImpl(Tertiary *t);
void gcDistLockRecurseImpl(Tertiary *t);
ConstTerm* auxGcDistCellImpl(Tertiary *t);
ConstTerm* auxGcDistLockImpl(Tertiary *t);

void globalizeCell(CellLocal*, int);
void globalizeLock(LockLocal*, int);

Tertiary *getOtherTertFromObj(Tertiary*, Tertiary*);

void cellLock_Perm(int state,Tertiary* t);
void cellLock_Temp(int state,Tertiary* t);
void cellLock_OK(int state,Tertiary* t);

#endif
