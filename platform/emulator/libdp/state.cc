/*
 *  Authors:
 *    Per Brand (perbrand@sics.se)
 *    Erik Klintskog (erik@sics.se)
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
 *     http://mozart.ps.uni-sb.de
 * 
 *  See the file "LICENSE" or
 *     http://mozart.ps.uni-sb.de/LICENSE.html
 *  for information on usage and redistribution 
 *  of this file, and for a DISCLAIMER OF ALL 
 *  WARRANTIES.
 *
 */

#if defined(INTERFACE)
#pragma implementation "state.hh"
#endif

#include "base.hh"
#include "value.hh"
#include "perdio.hh"
#include "state.hh"
#include "chain.hh"
#include "controlvar.hh"
#include "protocolState.hh"
#include "table.hh"

//

/**********************************************************************/
/*  Exported Utility                       */
/**********************************************************************/

Chain* getChainFromTertiary(Tertiary *t){
  Assert(t->isManager());
  if(t->getType()==Co_Cell){
    return ((CellManager *)t)->getChain();}
  Assert(t->getType()==Co_Lock);
  return ((LockManager *)t)->getChain();}

int getStateFromLockOrCell(Tertiary*t){
  if(t->getType()==Co_Cell){
    if(t->isManager()){
      return ((CellManager*)t)->getSec()->getState();}
    Assert(t->isFrame());
    return ((CellFrame*)t)->getSec()->getState();}
  Assert(t->getType()==Co_Lock);
  if(t->isManager()){
    return ((LockManager*)t)->getSec()->getState();}
  Assert(t->isFrame());
  return ((LockFrame*)t)->getSec()->getState();}      

CellSec *getCellSecFromTert(Tertiary *c){
  if(c->isManager()){
    return ((CellManager*)c)->getCellSec();}
  Assert(!c->isProxy());
  return ((CellFrame*)c)->getCellSec();}

LockSec *getLockSecFromTert(Tertiary *c){
  if(c->isManager()){
    return ((LockManager*)c)->getLockSec();}
  Assert(!c->isProxy());
  return ((LockFrame*)c)->getLockSec();}

PendThread* getPendThreadStartFromCellLock(Tertiary* t){
  if(t->getType()==Co_Cell){
    getCellSecFromTert(t)->getPending();}
  Assert(t->getType()==Co_Lock);
  return getLockSecFromTert(t)->getPending();}

/**********************************************************************/
/*  Utility                       */
/**********************************************************************/

static inline void sendPrepOwner(int index){
  OwnerEntry *oe=OT->getOwner(index);
  oe->getOneCreditOwner();}

/**********************************************************************/
/*  Globalizing                       */
/**********************************************************************/

void globalizeCell(CellLocal* cl, int myIndex){
  PD((CELL,"globalize cell index:%d",myIndex));
  TaggedRef val1=cl->getValue();
  CellManager* cm=(CellManager*) cl;
  CellSec* sec=new CellSec(val1);
  Chain* ch=new Chain(myDSite);
  cm->init(myIndex,ch,sec);
  initManagerForFailure(cm);
}

void globalizeLock(LockLocal* ll, int myIndex){
  PD((LOCK,"globalize lock index:%d",myIndex));
  Assert(sizeof(LockLocal)==sizeof(LockManager));
  Thread* th=ll->getLocker();
  PendThread* pt=ll->getPending();
  LockManager* lm=(LockManager*) ll;
  LockSec* sec=new LockSec(th,pt);
  Chain* ch=new Chain(myDSite);
  lm->init(myIndex,ch,sec);
  initManagerForFailure(lm);
}

void convertCellProxyToFrame(Tertiary *t){
  Assert(t->isProxy());
  CellFrame *cf=(CellFrame*) t;
  cf->convertFromProxy();
}

void convertLockProxyToFrame(Tertiary *t){
  Assert(t->isProxy());
  LockFrame *lf=(LockFrame*) t;
  lf->convertFromProxy();
}

/**********************************************************************/
/*   basic cell routine */
/**********************************************************************/

TaggedRef CellSec::unpendCell(PendThread* pt,TaggedRef val){
  val = oz_safeDeref(val);  
  if(pt==NULL) return val;
  switch(pt->exKind){
  case ACCESS:{
    ControlVarUnify(pt->controlvar,pt->old,val);
    return val;}
  case DEEPAT:{
    TaggedRef tr = tagged2SRecord(val)->getFeature(pt->nw);
    if(tr) {
      ControlVarUnify(pt->controlvar,tr,pt->old);} 
    else{
      ControlVarRaise(pt->controlvar,
	  OZ_makeException(E_ERROR,E_OBJECT,"@",2,val,pt->nw));}
    return val;}
  case ASSIGN:{
    if (tagged2SRecord(val)->replaceFeature(pt->old,pt->nw)) {
      ControlVarResume(pt->controlvar);
      return val;}
    ControlVarRaise(pt->controlvar,
	 OZ_makeException(E_ERROR,E_OBJECT,"<-",3,val,pt->old,pt->nw));
    return val;}
  case AT:{
    TaggedRef tr = tagged2SRecord(val)->getFeature(pt->old);
    if(tr) {
      ControlVarUnify(pt->controlvar,tr,pt->nw);
      return val;}
    ControlVarRaise(pt->controlvar,
		    OZ_makeException(E_ERROR,E_OBJECT,"@",2,val,pt->nw));}
  case REMOTEACCESS:{
   cellSendReadAns(((DSite*)(pt->old)),((DSite*)(pt->nw)),
		   (int)(pt->controlvar),val);
   return val;}
  case EXCHANGE:{
    Assert(pt->old!=0);
    Assert(pt->nw!=0);
    ControlVarUnify(pt->controlvar,val,pt->old);
    return pt->nw;}

  case DUMMY:
    return val;

 default:{
   Assert(0);}}

 return 0;
}


OZ_Return CellSec::exchangeVal(TaggedRef old, TaggedRef nw, ExKind exKind){
  contents = oz_safeDeref(contents);
  switch (exKind){
  case ASSIGN:{
    if (!tagged2SRecord(contents)->replaceFeature(old,nw)) {
      return OZ_raise(OZ_makeException(E_ERROR,E_OBJECT,"<-",3,contents,old,nw));}
    return PROCEED;}

  case AT:{
    TaggedRef tr = tagged2SRecord(contents)->getFeature(old);
    if(tr) {return oz_unify(tr,nw);}
    return OZ_raise(OZ_makeException(E_ERROR,E_OBJECT,"@",2, contents, old));}

  case EXCHANGE:{
    Assert(old!=0 && nw!=0);
    TaggedRef tr=contents;
    contents = nw;
    return oz_unify(tr,old);}

  default: 
    Assert(0);}
  return PROCEED; // stupid compiler
}

void CellSec::dummyExchange(CellManager* c){
  Assert(state==Cell_Lock_Invalid);
  PD((CELL,"CELL: exchange on invalid"));
  state=Cell_Lock_Requested;
  pendThreadAddDummyToEnd(&pending);
  int index=c->getIndex();
  Assert(c->isManager());
  Assert(!((CellManager*)c)->getChain()->hasFlag(TOKEN_LOST));
  DSite *toS=((CellManager*)c)->getChain()->setCurrent(myDSite,c);
  sendPrepOwner(index);
  cellLockSendForward(toS,myDSite,index);}

OZ_Return CellSec::exchange(Tertiary* c,TaggedRef old,TaggedRef nw,ExKind exKind){
  switch(state){
  case Cell_Lock_Valid:{
    PD((CELL,"CELL: exchange on valid"));
    return exchangeVal(old,nw,exKind);
  }
  case Cell_Lock_Requested|Cell_Lock_Next:
  case Cell_Lock_Requested:{
    PD((CELL,"CELL: exchange on requested"));
    pendThreadAddToEnd(&pending,old,nw,exKind);
    if(errorIgnore(c)) return SuspendOnControlVarReturnValue;
    if(entityCondMeToBlocked(c)) deferEntityProblem(c);
    return SuspendOnControlVarReturnValue;}
  case Cell_Lock_Invalid:{
    PD((CELL,"CELL: exchange on invalid"));
    state=Cell_Lock_Requested;
    pendThreadAddToEnd(&pending,old,nw,exKind);
    int index=c->getIndex();
    if(c->isFrame()){
      BorrowEntry* be=BT->getBorrow(index);
      be->getOneMsgCredit();
      cellLockSendGet(be);}
    else{
      Assert(c->isManager());
      if(!((CellManager*)c)->getChain()->hasFlag(TOKEN_LOST)){
	DSite *toS=((CellManager*)c)->getChain()->setCurrent(myDSite,c);
	sendPrepOwner(index);
	cellLockSendForward(toS,myDSite,index);}}
    if(errorIgnore(c)) return SuspendOnControlVarReturnValue;
    if(entityCondMeToBlocked(c)) deferEntityProblem(c);
    return SuspendOnControlVarReturnValue;}
  default: Assert(0);
  }
  Assert(0);
  return PROCEED;
}


OZ_Return CellSec::access(Tertiary* c,TaggedRef val,TaggedRef fea){
  switch(state){
  case Cell_Lock_Valid:{
    PD((CELL,"CELL: access on valid"));
    Assert(fea == 0);
    return oz_unify(val,contents);}
  case Cell_Lock_Requested|Cell_Lock_Next:
  case Cell_Lock_Requested:{
    PD((CELL,"CELL: access on requested"));
    pendThreadAddToEnd(&pending,val,fea,fea ? DEEPAT : ACCESS);
    if(!errorIgnore(c)){
      if(entityCondMeToBlocked(c)) deferEntityProblem(c);}
    return  SuspendOnControlVarReturnValue;}
  case Cell_Lock_Invalid:{
    PD((CELL,"CELL: access on invalid"));
    break;}
  default: Assert(0);}

  int index=c->getIndex();
  if(!c->isManager()) {
    Assert(c->isFrame());
    PD((CELL,"Sending to mgr read"));
    BorrowEntry *be=BT->getBorrow(index);
    be->getOneMsgCredit();
    cellSendRead(be,myDSite);}
  else{ // ERIK-LOOK PER-LOOK
    Assert(((CellManager*)c)->getChain()->getCurrent() != myDSite);
    sendPrepOwner(index);
    cellSendRemoteRead(((CellManager*)c)->getChain()->getCurrent(),
		       myDSite,index,myDSite);
}
  pendThreadAddToEnd(&pending,val,fea,fea ? DEEPAT : ACCESS);
  if(!errorIgnore(c)){
    if(entityCondMeToBlocked(c)) deferEntityProblem(c);}
  return SuspendOnControlVarReturnValue;
}


OZ_Return cellDoExchangeInternal(Tertiary *c,TaggedRef old,TaggedRef nw,ExKind e){
  PD((SPECIAL,"exchange old:%d new:%s type:%d",toC(old),toC(nw),e));
  maybeConvertCellProxyToFrame(c);
  PD((CELL,"CELL: exchange on %s-%d",
      (c->isManager()?myDSite:BT->getOriginSite(c->getIndex()))->stringrep(),
      (c->isManager()?c->getIndex():BT->getOriginIndex(c->getIndex()))));
  return getCellSecFromTert(c)->exchange(c,old,nw,e);
}

static
OZ_Return cellDoAccessImpl(Tertiary *c,TaggedRef val,TaggedRef fea){
  if(c->isProxy()){
    convertCellProxyToFrame(c);}
  return getCellSecFromTert(c)->access(c,val,fea);}


/**********************************************************************/
/*   interface */
/**********************************************************************/
  
OZ_Return cellDoExchangeImpl(Tertiary *c,TaggedRef old,TaggedRef nw){
   return cellDoExchangeInternal(c,old,nw,EXCHANGE);}

OZ_Return cellAssignExchangeImpl(Tertiary *c,TaggedRef fea,TaggedRef val){
   return cellDoExchangeInternal(c,fea,val,ASSIGN);}

OZ_Return cellAtExchangeImpl(Tertiary *c,TaggedRef old,TaggedRef nw){
  return cellDoExchangeInternal(c,old,nw,AT);}

OZ_Return cellAtAccessImpl(Tertiary *c, TaggedRef fea, TaggedRef val){
  return cellDoAccessImpl(c,val,fea);}

OZ_Return cellDoAccessImpl(Tertiary *c, TaggedRef val){
  if(oz_onToplevel())
    return cellDoExchangeImpl(c,val,val);
  else
    return cellDoAccessImpl(c,val,0);}

/**********************************************************************/
/*   Lock - basic routines                             */
/**********************************************************************/

void LockProxy::lock(Thread *t){
  PD((LOCK,"convertToFrame %s-%d",
      BT->getOriginSite(getIndex())->stringrep(),
      BT->getOriginIndex(getIndex())));
  convertLockProxyToFrame(this);
  ((LockFrame*)this)->lock(t);}

/**********************************************************************/
/*   Lock - interface                             */
/**********************************************************************/

void lockLockProxyImpl(Tertiary *t, Thread *thr){
  Assert(t->isProxy());
  ((LockProxy *)t)->lock(thr);
}

void lockLockManagerOutlineImpl(LockManagerEmul *lmu, Thread *thr){
  getLockSecFromTert(lmu)->lockComplex(thr,lmu);}

void unlockLockManagerOutlineImpl(LockManagerEmul *lmu, Thread *thr){
  getLockSecFromTert(lmu)->unlockComplex(lmu);}

void lockLockFrameOutlineImpl(LockFrameEmul *lfu, Thread *thr){
  getLockSecFromTert(lfu)->lockComplex(thr,lfu);}

void unlockLockFrameOutlineImpl(LockFrameEmul *lfu, Thread *thr){
  getLockSecFromTert(lfu)->unlockComplex(lfu);}

/**********************************************************************/
/*   Lock - interface                             */
/**********************************************************************/

void secLockToNext(LockSec* sec,Tertiary* t,DSite* toS){
  int index=t->getIndex();
  if(t->isFrame()){
    BorrowEntry *be=BT->getBorrow(index);
    be->getOneMsgCredit();
    NetAddress *na=be->getNetAddress();
    lockSendToken(na->site,na->index,toS);
    return;}
  Assert(t->isManager());
  OwnerEntry *oe=OT->getOwner(index);
  oe->getOneCreditOwner();
  lockSendToken(myDSite,index,toS);}

void secLockGet(LockSec* sec,Tertiary* t,Thread* th){
  int index=t->getIndex();
  sec->makeRequested();
  if(t->isFrame()){
    BorrowEntry *be=BT->getBorrow(index);
    be->getOneMsgCredit();
    cellLockSendGet(be);
    return;}
  Assert(t->isManager());
  OwnerEntry *oe=OT->getOwner(index);
  Chain* ch=((LockManager*) t)->getChain();
  DSite* current=ch->setCurrent(myDSite,t);
  oe->getOneCreditOwner();
  cellLockSendForward(current,myDSite,index);
  return;}

void LockSec::lockComplex(Thread *th,Tertiary* t){
  PD((LOCK,"lockComplex in state:%d",state));
  Assert(th==oz_currentThread());
  Assert(t->getBoardInternal()==oz_rootBoard());
  switch(state){
  case Cell_Lock_Valid|Cell_Lock_Next:{
    Assert(getLocker()!=th);   
    Assert(getLocker()!=NULL);
    pendThreadAddMoveToEnd(getPendBase());}
  case Cell_Lock_Valid:{
    Assert(getLocker()!=th);  
    Assert(getLocker()!=NULL);
    pendThreadAddToEnd(getPendBase());
    if(errorIgnore(t)) return; 
    break;}
  case Cell_Lock_Next|Cell_Lock_Requested:
  case Cell_Lock_Requested:{
    pendThreadAddToEnd(getPendBase());
    if(errorIgnore(t)) return;
    break;}
  case Cell_Lock_Invalid:{
    pendThreadAddToEnd(getPendBase());
    secLockGet(this,t,th);
    if(errorIgnore(t)) return;
    break;}
  default: Assert(0);}
  if(entityCondMeToBlocked(t)) deferEntityProblem(t);
}


void LockSec::unlockPending(Thread *t){
  PendThread **pt=&pending;
  while((*pt)->thread!=t) {
    pt=&((*pt)->next);}
  *pt=(*pt)->next;}

void LockSec::unlockComplex(Tertiary* tert){
  PD((LOCK,"unlock complex in state:%d",getState()));
  Assert(getState() & Cell_Lock_Valid);
  if(getState() & Cell_Lock_Next){
    Assert(getState()==(Cell_Lock_Next | Cell_Lock_Valid));
    if(pending==NULL){
      secLockToNext(this,tert,next);
      state=Cell_Lock_Invalid;
      return;}
    Thread *th=pending->thread;
    if(th==NULL && pending->exKind==MOVEEX){
      pendThreadRemoveFirst(getPendBase());
      secLockToNext(this,tert,next);
      state=Cell_Lock_Invalid;
      if(pending==NULL) return;
      secLockGet(this,tert,NULL);
      return;}
    if(th==NULL){
      Assert(tert->isManager());
      pendThreadRemoveFirst(getPendBase());
      unlockComplex(tert);
      return;}

    locker=pendThreadResumeFirst(getPendBase());
    return;}
  if(pending!=NULL){
    locker=pendThreadResumeFirst(getPendBase());
    return;}
  return;
}

/**********************************************************************/
/*   gc                             */
/**********************************************************************/

void gcDistCellRecurseImpl(Tertiary *t)
{
  gcEntityInfoImpl(t);
  switch (t->getTertType()) {
  case Te_Proxy:
    gcProxyRecurse(t);
    break;
  case Te_Frame: {
    CellFrame *cf=(CellFrame*)t;
    CellSec *cs=cf->getCellSec();
    cf->setCellSec((CellSec*)gcRealloc(cs,sizeof(CellSec)));
    cf->gcCellFrame();
    break; }
  case Te_Manager:{
    CellManager *cm=(CellManager*)t;
    CellFrame *cf=(CellFrame*)t;
    CellSec *cs=cf->getCellSec();
    cf->setCellSec((CellSec*)gcRealloc(cs,sizeof(CellSec)));
    cm->gcCellManager();
    break;}
  default: {
    Assert(0); }
  }
}

void gcDistLockRecurseImpl(Tertiary *t) 
{
  gcEntityInfoImpl(t);
  switch(t->getTertType()){
  case Te_Manager:{
    LockManager* lm=(LockManager*)t;
    LockFrame* lf=(LockFrame*)t;
    LockSec* ls= lf->getLockSec();
    lf->setLockSec((LockSec*)gcRealloc(ls,sizeof(LockSec)));
    lm->gcLockManager();
    break;}

  case Te_Frame:{
    LockFrame *lf=(LockFrame*)t;
    LockSec *ls=lf->getLockSec();
    lf->setLockSec((LockSec*)gcRealloc(ls,sizeof(LockSec)));
    lf->gcLockFrame();
    break;}

  case Te_Proxy:{
    gcProxyRecurse(t);
    break;}

  default:{
    Assert(0);}
  }
}

ConstTerm* auxGcDistCellImpl(Tertiary *t)
{
  CellFrame *cf=(CellFrame *)t;
  if (cf->isAccessBit()) {
    // has only been reached via gcBorrowRoot so far
    void* forward=cf->getForward();
    ((CellFrame*)forward)->resetAccessBit();
    cf->gcMark((ConstTerm *) forward);
    return (ConstTerm*) forward;
  } else {
    return (NULL);
  }
}

ConstTerm* auxGcDistLockImpl(Tertiary *t)
{
  LockFrame *lf=(LockFrame *)t;
  if(lf->isAccessBit()){
    // may be optimized by not resetting at all - PER    
    lf->resetAccessBit();
    void* forward=lf->getForward();
    ((LockFrame*)forward)->resetAccessBit();
    lf->gcMark((ConstTerm *) forward);
    return (ConstTerm*) forward;
  } else {
    return (NULL);
  }
}

void CellSec::gcCellSec(){
  gcPendThread(&pending);
  switch(stateWithoutAccessBit()){
  case Cell_Lock_Next|Cell_Lock_Requested:{
    next->makeGCMarkSite();}
  case Cell_Lock_Requested:{
    return;}
  case Cell_Lock_Next:{
    next->makeGCMarkSite();}
  case Cell_Lock_Invalid:{
    return;}
  case Cell_Lock_Valid:{
    OZ_collectHeapTerm(contents,contents);
    return;}
  default: Assert(0);}}

void CellFrame::gcCellFrame(){
  Tertiary *t=(Tertiary*)this;
  gcProxyRecurse(t);
  PD((GC,"relocate cellFrame:%d",t->getIndex()));
  getCellSec()->gcCellSec();}

void CellManager::gcCellManager(){
  getChain()->gcChainSites(); 
  int i=getIndex();
  PD((GC,"relocate cellManager:%d",i));
  OwnerEntry* oe=OT->getOwner(i);
  oe->gcPO(this);
  CellFrame *cf=(CellFrame*)this;
  getCellSec()->gcCellSec();}

void LockSec::gcLockSec(){
  if(state & Cell_Lock_Next){
    getNext()->makeGCMarkSite();}
  PD((GC,"relocate Lock in state %d",state));
  if(state & Cell_Lock_Valid)
    locker=locker->gcThread();
  if(pending!=NULL)
    gcPendThread(&pending);
  return;}

void LockFrame::gcLockFrame(){
  Tertiary *t=(Tertiary*)this;
  gcProxyRecurse(t);
  PD((GC,"relocate lockFrame:%d",t->getIndex()));
  getLockSec()->gcLockSec();}

void LockManager::gcLockManager(){
  getChain()->gcChainSites(); 
  int i=getIndex();
  PD((GC,"relocate lockManager:%d",i));
  OwnerEntry* oe=OT->getOwner(i);
  oe->gcPO(this);
  getLockSec()->gcLockSec();}

/**********************************************************************/
/*   failure                             */
/**********************************************************************/

// all these are proxies detecting manager is down
void cellLock_Perm(int state,Tertiary* t){
  switch(state){
  case Cell_Lock_Invalid:{
    if(addEntityCond(t,PERM_SOME|PERM_ME)) break;
    return;}
  case Cell_Lock_Requested|Cell_Lock_Next:
  case Cell_Lock_Requested:{
    if(addEntityCond(t,PERM_SOME|PERM_ME|PERM_BLOCKED)) break;
    return;}
  case Cell_Lock_Valid|Cell_Lock_Next: 
    if(addEntityCond(t,PERM_SOME)) break;
    return;
  case Cell_Lock_Valid:
    if(addEntityCond(t,PERM_SOME|PERM_ALL)) break;
    return;
  default: 
    Assert(0);}
  entityProblem(t);
}


void cellLock_Temp(int state,Tertiary* t){
  switch(state){
  case Cell_Lock_Invalid:{
    if(addEntityCond(t,TEMP_SOME|TEMP_ME)) break;
    return;} 
  case Cell_Lock_Requested|Cell_Lock_Next:
  case Cell_Lock_Requested:{
    if(addEntityCond(t,TEMP_SOME|TEMP_ME|TEMP_BLOCKED)) break;    
    return;}
  case Cell_Lock_Valid|Cell_Lock_Next:
    if(addEntityCond(t,TEMP_SOME)) break;    
    return;
  case Cell_Lock_Valid:
    if(addEntityCond(t,TEMP_SOME|TEMP_ALL)) break;    
    return;
  default: 
    Assert(0);}
  entityProblem(t);
}

void cellLock_OK(int state,Tertiary* t){
  switch(state){
  case Cell_Lock_Invalid:{ 
    subEntityCond(t,TEMP_SOME|TEMP_ME);
    return;} 
  case Cell_Lock_Requested|Cell_Lock_Next:
  case Cell_Lock_Requested:{
    subEntityCond(t,TEMP_SOME|TEMP_ME);
    return;} 
  case Cell_Lock_Valid|Cell_Lock_Next:
    subEntityCond(t,TEMP_SOME);
    return;
 case Cell_Lock_Valid:{
    subEntityCond(t,TEMP_SOME|TEMP_ALL);
    return;}
  default: {
    Assert(0);}}
}






