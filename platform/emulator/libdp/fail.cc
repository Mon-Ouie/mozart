/*
 *  Authors:
 *    Per Brand (perbrand@sics.se)
 *    Erik Klintskog (erik@sics.se)
 *
 *  Contributors:
 *    Per Brand, 1998
 *    Erik Klintskog, 1998
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
#pragma implementation "fail.hh"
#endif

#include "base.hh"
#include "thr_int.hh"
#include "controlvar.hh"
#include "builtins.hh"
#include "var_ext.hh"
#include "tagged.hh"

#include "dsite.hh"
#include "fail.hh"
#include "perdio.hh"
#include "table.hh"
#include "chain.hh"
#include "state.hh"
#include "protocolFail.hh"
#include "protocolState.hh"
#include "port.hh"
#include "var.hh"
#include "var_obj.hh"

Twin *usedTwins;
Watcher* globalWatcher;

/**********************************************************************/
/*   forward                                   */
/**********************************************************************/

void adjustProxyForFailure(Tertiary*,EntityCond,EntityCond);

/**********************************************************************/
/*   deferoperations                                          */
/**********************************************************************/

TaggedRef BI_defer;

DeferElement *DeferdEvents;

OZ_BI_define(BIdefer,0,0)
{
  DeferElement *ptr = DeferdEvents, *old;
  DeferdEvents = NULL;
  while(ptr){
    switch(ptr->type){
    case DEFER_PROXY_PROBLEM:
      proxyProbeFault(ptr->tert, ptr->prob);
      break;
    case DEFER_MANAGER_PROBLEM:
      managerProbeFault(ptr->tert,ptr->site, ptr->prob);
      break;
    case DEFER_ENTITY_PROBLEM:{
      entityProblem(ptr->tert);
      break;}
    default: Assert(0);
    }
    old = ptr;
    ptr = ptr->next;
    genFreeListManager->putOne_5((FreeListEntry*)old);
  }
  return PROCEED;
} OZ_BI_end

void addDeferElement(DeferElement* e){
  if(DeferdEvents==NULL){
    Thread *tt = oz_newThreadToplevel(DEFAULT_PRIORITY);
    tt->pushCall(BI_defer);}
  e->next = DeferdEvents;
  DeferdEvents = e;
}

DeferElement* newDeferElement(){
  return (DeferElement*) genFreeListManager->getOne_5();}

void gcDeferEvents(){
  DeferElement* ptr = DeferdEvents;
  while(ptr!=NULL) {
    ptr->tert=(Tertiary*)ptr->tert->gcConstTerm();
    if(ptr->type==DEFER_MANAGER_PROBLEM)
      ptr->site->makeGCMarkSite();
    ptr=ptr->next;}
}

/**********************************************************************/
/*   support                                   */
/**********************************************************************/

#define PROBE_UNUSED PROBE_OK

void deferEntityProblem(Tertiary* t){
  DeferElement *e = newDeferElement();
  e->init(DEFER_ENTITY_PROBLEM,t);
  addDeferElement(e);
}

void deferManagerProbeFault(Tertiary* t,DSite* s, int pr){
  DeferElement *e = newDeferElement();
  e->init(s,DEFER_MANAGER_PROBLEM,pr,t);
  addDeferElement(e);
}

void deferProxyProbeFault(Tertiary* t, int pr){
  DeferElement *e = newDeferElement();
  e->init(DEFER_PROXY_PROBLEM,pr,t);
  addDeferElement(e);
}
void managerInstallProbe(Tertiary* t,ProbeType pt){
  installProbeNoRet(BT->getOriginSite(t->getIndex()),pt);}

void watcherRemoved(Watcher* w, Tertiary* t){
  EntityCond oldC=getSummaryWatchCond(t);
  Watcher** base=getWatcherBase(t);
  Assert(base!=NULL);
  while((*base)!=w){
    base=&((*base)->next);
    Assert((*base)!=NULL);}
  *base= w->next;
  EntityCond newC=getSummaryWatchCond(t);
  if(t->getTertType()==Te_Manager){}
  else
    adjustProxyForFailure(t,oldC,newC);
}

EntityCond EntityInfo::getSummaryWatchCond(){
  EntityCond ec=ENTITY_NORMAL;
  Watcher *w=watchers;
  while(w!=NULL){
    ec |= w->watchcond;
    w=w->next;}
  return ec;
}

TaggedRef mkOp1(char* label,TaggedRef first){
  return OZ_mkTupleC(label,1,first);
}

TaggedRef mkOp2(char* label,TaggedRef first,TaggedRef second){
  return OZ_mkTupleC(label,2,first,second);
}

TaggedRef mkOp3(char* label,TaggedRef first,TaggedRef second,TaggedRef third){
  return OZ_mkTupleC(label,3,first,second,third);
}

/**********************************************************************/
/*   insert and remove watchers */
/**********************************************************************/

static Bool checkForExistentInjector(EntityInfo *ei, Thread* th,
                                     EntityCond wc,unsigned int kind){
  if(!(kind & WATCHER_INJECTOR)) return FALSE;
  if(ei==NULL) return FALSE;
  Watcher *w=ei->watchers;
  if(kind & WATCHER_SITE_BASED){
    while(w!=NULL){
      if(w->isInjector() && (w->isSiteBased())) return TRUE;
      w=w->next;}
    return FALSE;}
  while(w!=NULL){
    if(w->isInjector() && (w->thread==th)) return TRUE;
    w=w->next;}
  return FALSE;
}


void insertWatcherLocal(Tertiary *t,Watcher *w){
  EntityInfo* info=t->getInfo();
  if(info==NULL){
    info=new EntityInfo(w);
    t->setInfo(info);
    return;}
  w->next=info->watchers;
  info->watchers=w;
}

Watcher*  basicInsertWatcher(Watcher* w,Watcher* old){
  if(w->isSiteBased()){
    if(old==NULL) {
      w->next=NULL;
      return w;}
    while(old->next!=NULL){
      old=old->next;}
    old->next=w;
    w->next=NULL;
    return old;}
  w->next=old;
  return w;}

void insertWatcher(EntityInfo* ei,Watcher* w,EntityCond &oldEC, EntityCond &newEC){
  oldEC=ei->getSummaryWatchCond();
  ei->watchers=basicInsertWatcher(w,ei->watchers);
  newEC=ei->getSummaryWatchCond();}

void insertWatcher(Tertiary *t,Watcher *w, EntityCond &oldEC, EntityCond &newEC){
  EntityInfo* info=t->getInfo();
  if(info==NULL){
    info=new EntityInfo(w);
    t->setInfo(info);
    oldEC=ENTITY_NORMAL;
    newEC=w->watchcond;
    return;}
  insertWatcher(info,w,oldEC,newEC);
}

/**********************************************************************/
/*   entityProblem                            */
/**********************************************************************/

PendThread* threadTrigger(Tertiary* t,Watcher* w){
  PendThread *pd;
  switch(t->getType()){
  case Co_Port:
    pd = getPendThreadStartFromPort(t);
    break;
  case Co_Cell:
  case Co_Lock:
    if(t->getTertType()==Te_Proxy) return NULL;
    pd=getPendThreadStartFromCellLock(t);
    break;
  default:
    Assert(0);
    return NULL;
  }
  while(TRUE){
    if(pd==NULL) return NULL;
    if(pd->thread==NULL){
      pd=pd->next;
      continue;}
    if(w->isSiteBased() && pd->thread!=NULL) return pd;
    if(!w->isSiteBased() && pd->thread==w->thread) return pd;
    pd=pd->next;}
  return NULL;
}

void dealWithContinue(Tertiary* t,PendThread* pd){
  switch(t->getType()){
  case Co_Cell:{
    switch(pd->exKind){
    case EXCHANGE:{
      pd->thread->pushCall(BI_exchangeCell,makeTaggedTert(t),
                                pd->nw, pd->old);
      return;}
    case ASSIGN:{
      pd->thread->pushCall(BI_assign,makeTaggedTert(t),pd->old,pd->nw);
      return;}
    case AT:{
      pd->thread->pushCall(BI_atRedo,makeTaggedTert(t),pd->old,pd->nw);
      return;}
    default: Assert(0);}}
  case Co_Lock:{
    // mm2: no builtin lockLock available
    // pd->thread->pushCall(BI_lockLock,makeTaggedTert(t));
    Assert(0);
    return;}
  case Co_Port:
    pd->thread->pushCall(BI_send, makeTaggedTert(t), pd->old);
  default:
    Assert(0);
  }}

TaggedRef detOp(Tertiary* t,PendThread* pd){
  switch(t->getType()){
  case Co_Cell:
    switch(pd->exKind){
    case ASSIGN:
      return mkOp2("objectAssign",pd->old,pd->nw);
    case AT:
      return mkOp2("objectAccess",pd->old,pd->nw);
    case EXCHANGE:
      return mkOp2("cellExchange",pd->old,pd->nw);
    case O_EXCHANGE:
      TaggedRef a,b;
      ooExchGetFeaOld(pd->old,a,b);
      return mkOp3("objectExchange",a,b,pd->nw);
    case DEEPAT:
      if(pd->nw!=0)
        return mkOp2("objectAccess",pd->nw,pd->old);
      else
        return mkOp1("cellAccess",pd->old);
    case ACCESS:
      if(pd->nw!=0)
        return mkOp2("objectAccess",pd->nw,pd->old);
      else
        return mkOp1("objectAccess",pd->old);
    default:
      Assert(0);}
    return makeTaggedNULL();
  case Co_Lock:
    return AtomLock;
  case Co_Port:
    return mkOp1("send",pd->old);
  default: Assert(0);}
  return makeTaggedNULL();
}


Bool entityProblemPerWatcher(Tertiary*t, Watcher* w,Bool &hit){
  EntityCond ec=getEntityCond(t) & w->watchcond;
  if(w->isInjector()){
    PendThread* pd=threadTrigger(t,w);
    if(pd==NULL) return FALSE;
    hit=TRUE;
    if(ec==ENTITY_NORMAL) return FALSE;
    if(w->isRetry() && !w->isFired()) dealWithContinue(t,pd);
    w->invokeInjector(t,ec,pd->controlvar,pd->thread,detOp(t,pd));
    pd->thread=NULL;
    if(w->isPersistent()) return FALSE;
    watcherRemoved(w,t);
    return TRUE;}
  if(ec==ENTITY_NORMAL) return FALSE;
  w->invokeWatcher(makeTaggedTert(t),ec);
  return TRUE;
}

void entityProblem(Tertiary *t) {
  PD((ERROR_DET,"entityProblem invoked"));
  Watcher** aux;
  EntityCond oldC=getSummaryWatchCond(t);
  Bool hit=FALSE;
  if(errorIgnore(t)) {
    if(globalWatcher==NULL) return;
    goto global;}
  else{
    aux=getWatcherBase(t);
    if(aux==NULL){
      if(globalWatcher==NULL) return;
      goto global;}}

  while((*aux)!=NULL){
    if(entityProblemPerWatcher(t,(*aux),hit)){
      *aux=(*aux)->next;
      continue;}
    if(hit) break;
    aux= &((*aux)->next);}

 global:

  if((!hit) && (globalWatcher!=NULL) &&
     (globalWatcher->watchcond & getEntityCond(t)))
    entityProblemPerWatcher(t,globalWatcher,hit);

  EntityCond newC=getSummaryWatchCond(t);
  if(t->getTertType()!=Te_Manager)
    adjustProxyForFailure(t,oldC,newC);
}

/**********************************************************************/
/*   SECTION::  watcher                */
/**********************************************************************/

void Watcher::invokeWatcher(TaggedRef t,EntityCond ec){
if(!isFired()){
  Assert(!isInjector());
  TaggedRef lis;
  if(isWatcherEligible(t)){
    lis=listifyWatcherCond(ec,tagged2Tert(t));}
  else{
    lis=listifyWatcherCond(ec);}
  Thread *tt = oz_newThreadToplevel(DEFAULT_PRIORITY);
  tt->pushCall(proc,t,lis);}
}

void Watcher::varInvokeInjector(TaggedRef t,EntityCond ec,TaggedRef Op){
  Assert(isInjector());
  Assert(oz_currentThread()!=NULL);
  am.prepareCall(proc, t,listifyWatcherCond(ec),Op);
}

void Watcher::invokeInjector(Tertiary* t,EntityCond ec,
                             TaggedRef controlvar,Thread *th,TaggedRef Op){
  Assert(isInjector());
  Assert(!isFired());
  Assert(th!=NULL);
  TaggedRef listified=listifyWatcherCond(ec,t);
  th->pushCall(proc,makeTaggedTert(t),listified,Op);
  ControlVarResume(controlvar);
}

/**********************************************************************/
/*   SECTION::  probeFault -- first indication of error                */
/**********************************************************************/

void cellLockManagerProbeFault(Tertiary *t, DSite* s, int pr){
    Chain *ch=getChainFromTertiary(t);
  if(pr==PROBE_OK){
    if(!ch->hasFlag(INTERESTED_IN_OK)) return;
    ch->managerSeesSiteOK(t,s);
    return;}
  if(pr==PROBE_TEMP){
    ChainElem *ce=ch->getFirstNonGhost();
    if((ce->getSite()==s) || ((ce->getNext()!=NULL) &&
                              (ce->getNext()->getSite()==s)))
      ch->managerSeesSiteTemp(t,s);
    return;}
  ch->managerSeesSitePerm(t,s);}

void cellLockProxyProbeFault(Tertiary *t, int pr){
  int state;
  if(t->isProxy()){
    state=Cell_Lock_Invalid;}
  else{
    state=getStateFromLockOrCell(t);}
  if(pr==PROBE_PERM){
    cellLock_Perm(state,t);
    return;}
  if(pr == PROBE_OK){
    cellLock_OK(state,t);
    return;}
  Assert(pr==PROBE_TEMP);
  cellLock_Temp(state,t);
  return;}

void portProxyProbeFault(Tertiary *t, int pr){
  PortProxy *pp = (PortProxy*) t;
  switch(pr){
  case PROBE_PERM:
    port_Perm(pp);
    break;
  case PROBE_OK:
    port_Ok(pp);
    break;
  case PROBE_TEMP:
    port_Temp(pp);
    break;
  default:
    Assert(0);
  }
}

void proxyProbeFault(Tertiary *t, int pr) {
  PD((ERROR_DET,"proxy probe invoked %d",pr));
  switch(t->getType()){
  case Co_Cell:
  case Co_Lock:
    cellLockProxyProbeFault(t,pr);
    return;
  case Co_Port:
    portProxyProbeFault(t,pr);
    return;
  case Co_Object:
  case Co_Resource:
    return;
  default: Assert(0);
    return;}
}

void managerProbeFault(Tertiary *t, DSite* s,int pr) {
  switch(t->getType()){
  case Co_Cell:
  case Co_Lock:
    if(getChainFromTertiary(t)->siteExists(s))
      cellLockManagerProbeFault(t, s, pr);
    return;
  case Co_Port:
    /* The portManager is not affected by
       other sites. */
    return;
  case Co_Object:
    return;
  case Co_Resource:
    return;
  default:
    printf("WARNING %d\n",t->getType());
    Assert(0);}
}

void DSite::probeFault(ProbeReturn pr) {
  PD((PROBES,"PROBEfAULT  site:%s",stringrep()));
  int limit=OT->getSize();
  for(int ctr = 0; ctr<limit;ctr++){
    OwnerEntry *oe = OT->getOwner(ctr);
    if(oe->isTertiary()){
      Tertiary *tr=oe->getTertiary();
      PD((PROBES,"Informing Manager"));
      Assert(tr->isManager());
      managerProbeFault(tr,this,pr);}
    else{
      if(oe->isVar()){
        GET_VAR(oe,Manager)->probeFault(this,pr);}}}

  limit=BT->getSize();
  for(int ctr1 = 0; ctr1<limit;ctr1++){
    BorrowEntry *be = BT->getBorrow(ctr1);
    if(be->isTertiary()){
      if(be->getSite()==this){
        proxyProbeFault(be->getTertiary(),pr);}}
    else{
      if(be->isVar()){
        if(be->getSite()==this){
          if(typeOfBorrowVar(be)==VAR_PROXY){
            GET_VAR(be,Proxy)->probeFault(pr);}
          else{
            Assert(typeOfBorrowVar(be)==VAR_OBJECT);
            GET_VAR(be,Object)->probeFault(pr);}}}}}
}

/**********************************************************************/
/*   SECTION::              chain and error                            */
/**********************************************************************/

void Chain::establish_PERM_SOME(Tertiary* t){
  if(hasFlag(TOKEN_PERM_SOME)) return;
  setFlag(TOKEN_PERM_SOME);
  int OTI=t->getIndex();
  triggerInforms(&inform,OT->getOwner(OTI),OTI,PERM_SOME);
  addEntityCond(t,PERM_SOME);
  entityProblem(t);}

void Chain::establish_TOKEN_LOST(Tertiary* t){
  setFlagAndCheck(TOKEN_LOST);
  int OTI=t->getIndex();
  triggerInforms(&inform,OT->getOwner(OTI),OTI,PERM_SOME|PERM_ALL|PERM_FAIL);
  addEntityCond(t,PERM_SOME|PERM_FAIL|PERM_ALL);}

void Chain::shortcutCrashLock(LockManager* lm){
  establish_PERM_SOME(lm);
  ChainElem** base=getFirstNonGhostBase();
  ChainElem *ce;
  LockSec* sec=lm->getLockSec();
  if((*base)->next==NULL){
    LockSec *sec=lm->getLockSec();
    ChainElem *ce=*base;
    ce->reinit(myDSite);
    Assert(sec->state==Cell_Lock_Invalid);
    sec->state=Cell_Lock_Valid;
    return;}
  removeNextChainElem(base);
  ce=getFirstNonGhost();
  int OTI=lm->getIndex();
  if(ce->site==myDSite){
    lockReceiveTokenManager(OT->getOwner(OTI),OTI);
    return;}
  lockSendToken(myDSite,OTI,ce->site);}

void Chain::shortcutCrashCell(CellManager* cm,TaggedRef val){
  establish_PERM_SOME(cm);
  ChainElem** base=getFirstNonGhostBase();
  ChainElem *ce;
  CellSec* sec=cm->getCellSec();
  if((*base)->next==NULL){
    CellSec *sec=cm->getCellSec();
    ChainElem *ce=*base;
    ce->reinit(myDSite);
    Assert(sec->state=Cell_Lock_Invalid);
    sec->state=Cell_Lock_Valid;
    sec->contents=val;
    return;}
  removeNextChainElem(base);
  ce=getFirstNonGhost();
  int index=cm->getIndex();
  if(ce->site==myDSite){
    cellReceiveContentsManager(OT->getOwner(index),val,index);
    return;}
  OT->getOwner(index)->getOneCreditOwner();
  cellSendContents(val,ce->site,myDSite,index);}

void Chain::handleTokenLost(Tertiary* t,OwnerEntry *oe,int OTI){
  establish_TOKEN_LOST(t);
  ChainElem *ce=first->next;
  ChainElem *back;
  Assert(first->site->siteStatus()==SITE_PERM);
  releaseChainElem(first);
  while(ce){
    if(!ce->flagIsSet(CHAIN_GHOST)){
      if(ce->site!=myDSite){
        sendTellError(oe,ce->site,OTI,PERM_FAIL,TRUE);}}
    back=ce;
    ce=ce->next;
    releaseChainElem(back);}
  first=NULL;
  last=NULL;
  entityProblem(t);
}

void Chain::managerSeesSitePerm(Tertiary *t,DSite* s){
  PD((ERROR_DET,"managerSeesSitePerm site:%s nr:%d",
      s->stringrep(),t->getIndex()));
  PD((CHAIN,"%d",printChain(this)));
  removeGhost(s); // remove ghost if any
  if(!siteExists(s)) return;
  ChainElem **base=getFirstNonGhostBase();
  ChainElem *after,*dead,*before;
  if((*base)->site==s){
    PD((ERROR_DET,"managerSeesSitePerm - perm is first site"));
    dead=*base;
    after=dead->next;
    before=NULL;}
  else{
    PD((ERROR_DET,"managerSeesSitePerm - perm is not first site"));
    while((*base)->next->site!=s){base=&((*base)->next);}
    before=*base;
    dead=before->next;
    after=dead->next;}
  if(before==NULL){
    dead->setFlag(CHAIN_PAST);}
  else{
    if(before->site->siteStatus()==SITE_PERM){
      if(dead->flagIsSet(CHAIN_BEFORE)){
        before->setFlagAndCheck(CHAIN_BEFORE);}
      removeNextChainElem(&(before->next));
      managerSeesSitePerm(t,before->site);
      return;}}
  if(after==NULL){
    PD((ERROR_DET,"managerSeesSitePerm - perm is last site"));
    dead->setFlag(CHAIN_BEFORE);}
  else{
    PD((ERROR_DET,"managerSeesSitePerm - perm is not last site"));
    if(after->site->siteStatus()==SITE_PERM){
      removeNextChainElem(&(dead->next));
      managerSeesSitePerm(t,s);
      return;}}
  if(dead->flagIsSet(CHAIN_CANT_PUT)) return;
  if(!dead->flagIsSet(CHAIN_PAST)){
    maybeChainSendQuestion(before,t,s);
    return;}
  if(!dead->flagIsSet(CHAIN_BEFORE)){
    maybeChainSendQuestion(after,t,s);
    return;}
  PD((ERROR_DET,"managerSeesSitePerm - token lost (lock can recover"));
  if(before!=NULL) {
    removeBefore(dead->site);}
  if(t->getType()==Co_Lock){
    PD((ERROR_DET,"LockToken lost, now recreated"));
    shortcutCrashLock((LockManager*) t);
    return;}
  PD((ERROR_DET,"Token lost"));
  int OTI=t->getIndex();
  handleTokenLost(t,OT->getOwner(OTI),OTI);
  Assert(inform==NULL);
  return;
}

Bool InformElem::maybeTrigger(OwnerEntry* oe, int index, EntityCond ec){
  EntityCond xec= ec & watchcond;
  xec &= ~foundcond;
  if(xec == ENTITY_NORMAL) return FALSE;
  foundcond |= xec;
  sendTellError(oe,site,index,xec,TRUE);
  if(somePermCondition(xec)) return TRUE;
  return FALSE;
}

void InformElem::maybeTriggerOK(OwnerEntry* oe, int index, EntityCond ec){
  EntityCond xec= ec & foundcond;
  if(xec == ENTITY_NORMAL) return;
  foundcond &= ~xec;
  sendTellError(oe,site,index,xec,FALSE);
}

void triggerInforms(InformElem **base,OwnerEntry* oe,int index,EntityCond ec){
  while((*base)!=NULL){
    if((*base)->maybeTrigger(oe,index,ec))
      (*base) = ((*base)->next);
    else
      base= &((*base)->next);}
}

void triggerInformsOK(InformElem **base,OwnerEntry* oe,int index,EntityCond ec){
  InformElem *ie=(*base);
  while(ie!=NULL){
    ie->maybeTriggerOK(oe,index,ec);
    ie=ie->next;}
}

  // approximative PER-LOOK
void Chain::managerSeesSiteTemp(Tertiary *t,DSite* s){
  EntityCond ec;
  int index=t->getIndex();
  OwnerEntry *oe=OT->getOwner(index);
  PD((ERROR_DET,"managerSeesSiteTemp site:%s nr:%d",
      s->stringrep(),index));

// deal with TEMP_SOME|TEMP_FAIL watchers
  triggerInforms(&inform,oe,index,(TEMP_SOME|TEMP_FAIL));

  ChainElem *ce=findAfter(s); // deal with TEMP_FAIL injectors
  while(ce!=NULL){
    if(ce->getSite()->siteStatus()!=SITE_OK) break;
    sendTellError(oe,ce->getSite(),index,ec,TRUE);
    ce=ce->next;}

  setFlag(INTERESTED_IN_OK);
}

void Chain::managerSeesSiteOK(Tertiary *t,DSite* s){
  Assert(siteExists(s));
  Assert(hasFlag(INTERESTED_IN_OK));

  int index=t->getIndex();
  OwnerEntry *oe=OT->getOwner(index);
  PD((ERROR_DET,"managerSeesSiteOK site:%s nr:%d",
      s->stringrep(),index));

// deal with TEMP_SOME|TEMP_FAIL watchers
  if(!(tempConnectionInChain())){
    triggerInformsOK(&inform,oe,index,(TEMP_SOME|TEMP_FAIL));}

  ChainElem *ce=findAfter(s); // deal with TEMP_FAIL injectors
  while(ce!=NULL){
    if(ce->getSite()->siteStatus()==SITE_TEMP) break;
    sendTellError(oe,ce->getSite(),index,TEMP_FAIL,FALSE);
    ce=ce->next;}

  if(!tempConnectionInChain()){
    resetFlagAndCheck(INTERESTED_IN_OK);}
}

/**********************************************************************/
/**********************************************************************/

inline void proxyInform(Tertiary* t,EntityCond ec){
  sendAskError(BT->getBorrow(t->getIndex()),ec);
}

inline void proxyDeInform(Tertiary* t,EntityCond ec){
    sendUnAskError(BT->getBorrow(t->getIndex()),ec);
}

EntityCond askPart(Tertiary* t, EntityCond ec){
  Assert(!(t->isLocal()));
  Assert(!(t->isManager()));
  switch(t->getType()){
  case Co_Lock:
  case Co_Cell:
    return ec & (PERM_SOME|TEMP_SOME|PERM_FAIL|TEMP_FAIL);
  case Co_Port:
    break;
  default:
    Assert(0);}
  return ENTITY_NORMAL;
}

EntityCond varAskPart(EntityCond ec){
  return ec & (TEMP_SOME|PERM_SOME);}

void adjustProxyForFailure(Tertiary*t, EntityCond oldEC, EntityCond newEC){
  if(askPart(t,newEC)!=askPart(t,oldEC)){
    if(askPart(t,oldEC)!=ENTITY_NORMAL)
      proxyDeInform(t,askPart(t,oldEC));
    proxyInform(t,askPart(t,newEC));}
}

void varAdjustPOForFailure(int index,EntityCond oldC, EntityCond newC){
  if(varAskPart(oldC)==varAskPart(newC)) return;
  if(varAskPart(oldC)!=ENTITY_NORMAL){
    sendUnAskError(BT->getBorrow(index),oldC);}
  if(varAskPart(newC)!=ENTITY_NORMAL){
    sendAskError(BT->getBorrow(index),newC);}
}

/**********************************************************************/
/*   SECTION::       installation/deinstallation utility  VARS        */
/**********************************************************************/

Bool installWatcher(TaggedRef* tPtr,EntityCond wc,TaggedRef proc,
                    Thread* th, unsigned int kind) {

  VarKind vk=classifyVar(tPtr);
  Assert(vk!=VAR_KINDED);
  if((vk==VAR_FREE) || (vk==VAR_FUTURE)){
    Assert(perdioInitialized);
    (void) globalizeFreeVariable(tPtr);
    Assert(classifyVar(tPtr)==VAR_MANAGER);
    vk=VAR_MANAGER;}

  EntityInfo *ei=varMakeOrGetEntityInfo(tPtr);
  if(checkForExistentInjector(ei,th,wc,kind))
    return FALSE;
  Watcher* w=new Watcher(proc,th,wc,kind);
  EntityCond oldC,newC;
  insertWatcher(ei,w,oldC,newC);

  switch(vk){
  case VAR_MANAGER:{
    if(ei->getEntityCond()!=ENTITY_NORMAL){
      oz_getManagerVar(*tPtr)->newWatcher(w->isInjector());}
    break;}
  case VAR_PROXY:{
    ProxyVar *pv=oz_getProxyVar(*tPtr);
    if(ei->getEntityCond()!=ENTITY_NORMAL){
      pv->newWatcher(w->isInjector());}
    varAdjustPOForFailure(pv->getIndex(),oldC,newC);
    break;}
  case VAR_OBJECT:{
    ObjectVar *ov=oz_getObjectVar(*tPtr);
    if(ei->getEntityCond()!=ENTITY_NORMAL){
      ov->newWatcher(w->isInjector());}
    varAdjustPOForFailure(ov->getObject()->getIndex(),oldC,newC);
    break;}
  default:
    Assert(0);}
  return TRUE;
}

Bool deinstallWatcher(TaggedRef* tPtr,EntityCond wc,TaggedRef proc,
                      Thread* th, unsigned int kind){
  VarKind vk=classifyVar(tPtr);
  if(vk==VAR_KINDED) return FALSE;
  if(vk==VAR_FREE) return FALSE;
  if(vk==VAR_FUTURE) return FALSE;

  EntityInfo* ei=varGetEntityInfo(tPtr);
  if(ei==NULL) return FALSE;

  EntityCond oldC,newC;
  Watcher** base= &ei->watchers;
  if(base==NULL) return FALSE;
  oldC=ei->getSummaryWatchCond();

  Bool found = FALSE;
  while(*base!=NULL){
    if((*base)->matches(proc,th,wc,kind)){
      *base = (*base)->next;
      found = TRUE;
      break;}
    else{
      base= &((*base)->next);}}

  if(!found) return FALSE;

  PD((NET_HANDLER,"Watcher deinstalled"));
  newC=ei->getSummaryWatchCond();

  switch(vk){
  case VAR_MANAGER:{
    break;}
  case VAR_PROXY:{
    ProxyVar *pv=oz_getProxyVar(*tPtr);
    varAdjustPOForFailure(pv->getIndex(),oldC,newC);
    break;}
  case VAR_OBJECT:{
    ObjectVar *ov=oz_getObjectVar(*tPtr);
    varAdjustPOForFailure(ov->getObject()->getIndex(),oldC,newC);
    break;}
  default:
    Assert(0);}
  return TRUE;
}

/**********************************************************************/
/*   SECTION::       installation/deinstallation utility             */
/**********************************************************************/


Bool installGlobalWatcher(EntityCond wc,TaggedRef proc,int kind){
  if(globalWatcher!=NULL) {return FALSE;}
  globalWatcher=new Watcher(proc,NULL,wc,kind);
  return TRUE;}

Bool deInstallGlobalWatcher(EntityCond wc,TaggedRef proc,int kind){
  if(globalWatcher==NULL){return FALSE;}
  if((wc != ANY_COND) && wc!=globalWatcher->watchcond) {return FALSE;}
  globalWatcher=NULL;
  return TRUE;}

EntityInfo *tertiaryMakeOrGetEntityInfo(Tertiary* t){
  if(t->getInfo()==NULL){
    t->setInfo(new EntityInfo);}
  return t->getInfo();}

void transferWatchers(Object* o){
  EntityInfo *cei=o->getInfo();
  if(cei==NULL) return;
  Assert(cei->getEntityCond()==ENTITY_NORMAL);
  EntityInfo *lei=new EntityInfo();
  Watcher *ow=cei->watchers;
  Assert(ow!=NULL);
  Assert(stateIsCell(o->getState()));
  CellManager* cm=(CellManager*) getCell(o->getState());
  LockManager* lm=(LockManager*) o->getLock();
  if(lm==NULL){
    cm->setInfo(cei);
    return;}

  cm->setInfo(cei);
  lm->setInfo(lei);
  Watcher* nw=new Watcher(ow->proc,ow->thread,ow->kind,ow->watchcond);
  nw->lockTwin(ow->cellTwin());
  ow=ow->next;
  Watcher* aux;
  while(ow!=NULL){
    aux=new Watcher(ow->proc,ow->thread,ow->kind,ow->watchcond);
    aux->lockTwin(ow->cellTwin());
    ow=ow->next;
    nw->next=aux;
    nw=aux;}
}

Bool installWatcher(Tertiary* t,EntityCond wc,TaggedRef proc,
                    Thread* th, unsigned int kind) {
  Watcher *w=new Watcher(proc,th,wc,kind);

  PD((NET_HANDLER,"Watcher installed on tertiary %x",t));
  EntityInfo *ei=tertiaryMakeOrGetEntityInfo(t);
  if(checkForExistentInjector(ei,th,wc,kind)){
    return FALSE;}

  EntityCond oldC,newC;
  if(t->isLocal()){
    insertWatcherLocal(t,w);
    return TRUE;}
  insertWatcher(t,w,oldC,newC);
  if(t->isManager()){}
  else{
    adjustProxyForFailure(t,oldC,newC);}
  if(w->isTriggered(getEntityCond(t))) deferEntityProblem(t);
  return TRUE;
}

Bool deinstallWatcher(Tertiary* t,EntityCond wc,TaggedRef proc,
                      Thread* th, unsigned int kind){
  if((t->getType()==Co_Object) & (t->getTertType()!=Te_Local)){
    Object* o= (Object*) t;
    if(!stateIsCell(o->getState())){
      return FALSE;}
    Bool ret=deinstallWatcher(o->getLock(),wc,proc,th,kind);
    if(!ret) return FALSE;
    PD((NET_HANDLER,"Watcher on object deinstalled"));
    ret=deinstallWatcher(getCell(o->getState()),wc,proc,th,kind);
    Assert(ret==TRUE);
    return TRUE;}

  EntityCond oldEC=getSummaryWatchCond(t);
  Bool found = FALSE;
  Watcher **base=getWatcherBase(t);
  if(base!=NULL)
    while(*base!=NULL){
      if((*base)->matches(proc,th,wc,kind)){
        *base = (*base)->next;
        found = TRUE;
        break;}
      else{
        base= &((*base)->next);}}

  if(!found) return FALSE;

  PD((NET_HANDLER,"Watcher deinstalled"));
  EntityCond newEC=getSummaryWatchCond(t);
  if(t->isLocal()) return TRUE;
  if(!t->isManager())
    adjustProxyForFailure(t,oldEC,newEC);
  return TRUE;
}

/**********************************************************************/
/*   SECTION::              user interface                            */
/**********************************************************************/

TaggedRef mkRemoteProblem(TaggedRef arg){
  return OZ_mkTupleC("remoteProblem",1,arg);}


TaggedRef mkLWC(TaggedRef label,TaggedRef field,TaggedRef contents){
   return OZ_recordInit(label,
       oz_cons(oz_pair2(field,contents),oz_nil()));
}

TaggedRef listifyWatcherCond(EntityCond ec,Bool owner,Bool state){
  TaggedRef list = oz_nil();
  TaggedRef aux;
  if(ec & PERM_FAIL){
    if(owner){
      aux=mkLWC(AtomPermFail,AtomInfo,AtomOwner);}
    else{
      if(state)
        aux=mkLWC(AtomPermFail,AtomInfo,AtomState);
      else
        aux=AtomPermFail;}
    list = oz_cons(aux, list);
    ec = ec & ~(PERM_FAIL|TEMP_FAIL);}
  if(ec & TEMP_FAIL){
    if(owner){
      aux=mkLWC(AtomTempFail,AtomInfo,AtomOwner);}
    else{
      if(state)
        aux=mkLWC(AtomTempFail,AtomInfo,AtomOwner);
      else
        aux=AtomTempFail;}
    list = oz_cons(aux, list);
    ec = ec & ~TEMP_FAIL;}
  if(ec & PERM_ALL){
    list = oz_cons(mkRemoteProblem(AtomPermAll), list);
    ec = ec & ~(PERM_ALL|TEMP_ALL|PERM_SOME|TEMP_SOME);}
  if(ec & TEMP_ALL){
    list = oz_cons(mkRemoteProblem(AtomTempAll), list);
    ec = ec & ~(TEMP_ALL|TEMP_SOME);}
  if(ec & PERM_SOME){
    list = oz_cons(mkRemoteProblem(AtomPermSome), list);
    ec = ec & ~(PERM_SOME|TEMP_SOME);}
  if(ec & TEMP_SOME){
    list = oz_cons(mkRemoteProblem(AtomTempSome), list);
    ec = ec & ~TEMP_SOME;}
  Assert(ec==0);
  return list;
}

TaggedRef listifyWatcherCond(EntityCond ec,Tertiary *t){
  switch(t->getType()){
  case Co_Lock:
  case Co_Cell:{
    if(t->getTertType()==Te_Manager){
      return listifyWatcherCond(ec,FALSE,TRUE);}
    DSite*s =BT->getBorrow(t->getIndex())->getNetAddress()->site;
    if(s->siteStatus()==SITE_PERM)
      return listifyWatcherCond(ec,TRUE,FALSE);
    else
      return listifyWatcherCond(ec,FALSE,TRUE);}
  default:
    return listifyWatcherCond(ec,FALSE,FALSE);}
  Assert(0);
  return 0;
}

TaggedRef listifyWatcherCond(EntityCond ec){
  return listifyWatcherCond(ec,FALSE,FALSE);}

/**********************************************************************/
/*   new                          */
/**********************************************************************/

Bool distHandlerInstallImpl(unsigned short kind,unsigned short ec,
                                 Thread* th,TaggedRef entity,
                                 TaggedRef proc){
  if(entity==0){
    return installGlobalWatcher(ec,proc,kind);}
  if(!oz_isVariable(oz_deref(entity))){
    entity=oz_deref(entity);
    if(!isWatcherEligible(entity)) return FALSE;
    Tertiary* tert = tagged2Tert(entity);
    return installWatcher(tert,ec,proc,th,kind);}

  DEREF(entity,vs_ptr,vs_tag);
  return installWatcher(vs_ptr,ec,proc,th,kind);
}

Bool distHandlerDeInstallImpl(unsigned short kind,unsigned short ec,
                                 Thread* th,TaggedRef entity,
                                 TaggedRef proc){
  if(entity==0){
    return deInstallGlobalWatcher(ec,proc,kind);}
  if(!oz_isVariable(oz_deref(entity))){
    entity=oz_deref(entity);
    if(!isWatcherEligible(entity)) return FALSE;
    Tertiary* tert = tagged2Tert(entity);
    return deinstallWatcher(tert,ec,proc,th,kind);}

  DEREF(entity,vs_ptr,vs_tag);
  return deinstallWatcher(vs_ptr,ec,proc,th,kind);
}

OZ_Return DistHandlerInstall(SRecord *condStruct, TaggedRef proc, Bool& suc){

  EntityCond ec;
  short kind;
  Thread *th;
  OZ_Return ret;
  TaggedRef entity;

  ret=distHandlerInstallHelp(condStruct,ec,th,entity,kind);
  if(ec==ANY_COND) return IncorrectFaultSpecification;
  if(ret!=PROCEED) return ret;
  if((entity==0) && (ec & (PERM_SOME|PERM_ALL|TEMP_SOME|TEMP_ALL))){
    return IncorrectFaultSpecification;
  }
  if(!oz_isAbstraction(proc))
    return IncorrectFaultSpecification;
  if(kind & WATCHER_INJECTOR){
    if(tagged2Abstraction(proc)->getArity()!=3)
      return IncorrectFaultSpecification;}
  else{
    if(tagged2Abstraction(proc)->getArity()!=2)
      return IncorrectFaultSpecification;}

  suc=distHandlerInstallImpl(kind,ec,th,entity,proc);
  return PROCEED;
}

OZ_Return DistHandlerDeInstall(SRecord *condStruct, TaggedRef proc, Bool &suc){

  EntityCond ec;
  short kind;
  Thread *th;
  TaggedRef entity;
  OZ_Return ret;

  ret=distHandlerInstallHelp(condStruct,ec,th,entity,kind);
  if(ret!=PROCEED) return ret;
  if((entity==0) && (ec & (PERM_SOME|PERM_ALL|TEMP_SOME|TEMP_ALL))){
    return IncorrectFaultSpecification;
  }
  suc=distHandlerDeInstallImpl(kind,ec,th,entity,proc);
  return PROCEED;
}

/**********************************************************************/
/*   gc                           */
/**********************************************************************/

EntityInfo* gcEntityInfoInternal(EntityInfo *info){
  if(info==NULL) return NULL;
  EntityInfo *newInfo = (EntityInfo *) OZ_hrealloc(info,sizeof(EntityInfo));
  newInfo->gcWatchers();
  return newInfo;}

void gcEntityInfoImpl(Tertiary *t) {
  t->setInfo(gcEntityInfoInternal(t->getInfo()));}

void gcTwins(){
  Twin** base=&usedTwins;
  Twin* aux;
  while(*base!=NULL){
    if((*base)->hasGCMark()){
      (*base)->resetGCMark();
      base= &((*base)->next);}
    else{
      aux=*base;
      base= &((*base)->next);
      aux->free();}}
}

void EntityInfo::gcWatchers(){
  Watcher **base=&watchers;
  Watcher *w=*base;
  Thread* nth=NULL;
  while(w!=NULL){
    if(w->twin!=NULL){
      if((!(w->isPersistent())) && w->isFired()){
        *base= w->next;
        w=*base;
        continue;}}
    if(w->isInjector()){
      nth= w->thread->gcThread();
      if(((nth==NULL) && !(w->isSiteBased()))){
        *base= w->next;
        w=*base;
        continue;}}
    if(w->twin!=NULL){
      w->twin->setGCMark();}

    Watcher* newW=(Watcher*) OZ_hrealloc(w,sizeof(Watcher));
    *base=newW;
    newW->thread=nth;
    OZ_collectHeapTerm(newW->proc,newW->proc);
    base= &(newW->next);
    w=*base;}
}

void gcGlobalWatcher(){
  if(globalWatcher==NULL) return;
  globalWatcher = (Watcher*) OZ_hrealloc(globalWatcher,sizeof(Watcher));
  OZ_collectHeapTerm(globalWatcher->proc,globalWatcher->proc);}

// called from gc
void maybeUnask(Tertiary* t){
  adjustProxyForFailure(t,getSummaryWatchCond(t),ENTITY_NORMAL);}

void EntityInfo::dealWithWatchers(TaggedRef tr,EntityCond ec){
  Watcher **base=getWatcherBase();
  if(base!=NULL) {
    while((*base)!=NULL){
      if((ec & (*base)->watchcond) && !(*base)->isInjector()){
        (*base)->invokeWatcher(tr,ec);
        *base= (*base)->next;}
      else
        base= &((*base)->next);}}
}

/**********************************************************************/
/*   SeifHandler                                                      */
/**********************************************************************/

DSite* gBTI(int i){
  return BT->getBorrow(i)->getNetAddress()->site;}

OZ_BI_define(BIfailureDefault,3,0){
  oz_declareIN(0,entity);
  oz_declareIN(1,what);
  oz_declareIN(2,op);
  TaggedRef list,rec;
  list=oz_cons(oz_pair2(AtomEntity,entity),oz_nil());
  list=oz_cons(oz_pair2(AtomConditions,what),list);
  list=oz_cons(oz_pair2(AtomOp,op),list);
  rec=OZ_recordInit(AtomDp,list);
  rec=mkOp1("system",rec);
  return OZ_raise(rec);
}OZ_BI_end


/**********************************************************************/
/*   Handover */
/**********************************************************************/

enum CompWatchers{
  COMPARE_DIFFERENT,
  COMPARE_EQUAL};

int compareWatchers(Watcher* one,Watcher* two){
  Assert(one->isInjector());
  Assert(two->isInjector());
  if(one->isSiteBased()){
    if(!two->isSiteBased()){ return COMPARE_DIFFERENT;}
    return COMPARE_EQUAL;}
  if(two->isSiteBased()) {return COMPARE_DIFFERENT;}
  return COMPARE_EQUAL;}

Watcher* removeWatcher(Watcher* w,Watcher* l){
  if(l==w) return l->next;
  Watcher** base= &(l->next);
  while(TRUE){
    Assert((*base)!=NULL);
    if((*base)==w){
      *base=w->next;
      return l;}
    base= &((*base)->next);}
  return l;
}

// nW has priority;
Watcher* mergeWatchers(Watcher* oW,Watcher *nW){
  Watcher *w,*toBe,*aux;
  if(nW==NULL) return oW;
  int comp;
  w=oW;
  toBe=nW;
  while(w!=NULL){
    comp=COMPARE_DIFFERENT;
    if(w->isInjector()){
      aux=toBe;
      while(aux!=NULL){
        if(aux->isInjector()){
          comp=compareWatchers(w,aux);
          if(comp!=COMPARE_DIFFERENT) break;}
        aux=aux->next;}}
    if(comp==COMPARE_DIFFERENT){
      aux=w;
      w=w->next;
      aux->next=NULL;
      toBe=basicInsertWatcher(aux,toBe);}
    else{
      w=w->next;}}
  return toBe;
}

void maybeHandOver(EntityInfo* oldE, TaggedRef t){
  if(oldE==NULL) return;
  if(oldE->watchers==NULL) return;
  EntityInfo *newE;
  DEREF(t,vs_ptr,vs_tag);
  if(isVariableTag(vs_tag)){
    newE=varMakeOrGetEntityInfo(vs_ptr);
    newE->watchers=mergeWatchers(oldE->watchers,newE->watchers);
    return;}
  if(!oz_isConst(t)) return;
  ConstTerm* c=tagged2Const(t);
  Bool flag=NO;
  switch(c->getType()){
  case Co_Cell:
  case Co_Lock: break;
  case Co_Object:flag=OK;
  case Co_Port:break;
  default: return;}
  Tertiary* tert=(Tertiary*)c;
  newE=tertiaryMakeOrGetEntityInfo(tert);
  newE->watchers=mergeWatchers(oldE->watchers,newE->watchers);
  if(flag) transferWatchers((Object*)c);
}

void dealWithDeferredWatchers(){
  DeferWatcher* w=deferWatchers;
  Bool ret;
  while(w!=NULL){
    ret=distHandlerInstallImpl(w->kind,w->watchcond,
                               w->thread,w->entity,w->proc);
    Assert(ret==TRUE);
    w=w->next;}
  deferWatchers=NULL;
}

/* NEW */

Bool entityProblemPerWatcher1(Tertiary*t, Watcher* w,Bool &hit,EntityCond &e,
                              TaggedRef &p){
  EntityCond ec=getEntityCond(t) & w->watchcond;
  if(!w->isInjector()) return FALSE;
  if(w->isFired()) return FALSE;
  if(w->isSiteBased() || (w->thread==oz_currentThread())){
    hit=TRUE;
    p=w->proc;
    e=w->watchcond & ec;
    Assert(!w->isRetry());
    if(w->isPersistent()) return FALSE;
    watcherRemoved(w,t);
    return TRUE;}
  return FALSE;
}

Bool tertiaryFail(Tertiary *t,EntityCond &ec,TaggedRef &proc){
  EntityInfo* info=t->getInfo();
  Assert(info!=NULL);
  ec=info->getEntityCond();
  if(ec == ENTITY_NORMAL) return FALSE;

  EntityCond oldC=getSummaryWatchCond(t);
  Watcher* w;
  Bool hit=FALSE;
  Watcher** aux=getWatcherBase(t);
  if(aux==NULL){
    if(globalWatcher==NULL) return FALSE;
    goto global;}

  while((*aux)!=NULL){
    if(entityProblemPerWatcher1(t,(*aux),hit,ec,proc)){
      *aux=(*aux)->next;
      continue;}
    if(hit) break;
    aux= &((*aux)->next);}

 global:

  if((!hit) && (globalWatcher!=NULL) &&
     (globalWatcher->watchcond & getEntityCond(t)))
    entityProblemPerWatcher1(t,globalWatcher,hit,ec,proc);
  if((!hit) || (ec==0)) return FALSE;

  EntityCond newC=getSummaryWatchCond(t);
  if(t->getTertType()!=Te_Manager)
    adjustProxyForFailure(t,oldC,newC);
  return TRUE;
}

OZ_Return tertiaryFailHandle(Tertiary* c,TaggedRef proc,EntityCond ec,
                              TaggedRef op){
  Assert(oz_currentThread()!=NULL);
  am.prepareCall(proc,makeTaggedTert(c),
                 listifyWatcherCond(ec,c),op);
  return BI_REPLACEBICALL;}
