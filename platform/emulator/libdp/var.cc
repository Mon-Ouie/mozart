/*
 *  Authors:
 *    Michael Mehl (mehl@dfki.de)
 *    Per Brand (perbrand@sics.se)
 *
 *  Contributors:
 *    Ralf Scheidhauer (Ralf.Scheidhauer@ps.uni-sb.de)
 *    Erik Klintskog (erik@sics.se)
 *
 *  Copyright:
 *    Michael Mehl (1997,1998)
 *    Per Brand, 1998
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

#if defined(INTERFACE)
#pragma implementation "var.hh"
#endif

#include "var.hh"
#include "var_ext.hh"
#include "var_obj.hh"
#include "dpMarshaler.hh"
#include "newmarshaler.hh"
#include "unify.hh"
#include "var_simple.hh"
#include "var_future.hh"
#include "chain.hh"
#include "flowControl.hh"

#define USE_ALT_VAR_PROTOCOL ozconf.perdioUseAltVarProtocol

Bool globalRedirectFlag=AUT_REG;

/* --- Common unification --- */

#define UNIFY_ERRORMSG \
   "Unification of distributed variable with term containing resources"


// compare NAs
#define GET_ADDR(var,SD,OTI)                                            \
DSite* SD;int OTI;                                                      \
if (var->getIdV()==OZ_EVAR_PROXY) {                                     \
  NetAddress *na=BT->getBorrow(var->getIndex())->getNetAddress();       \
  SD=na->site;                                                          \
  OTI=na->index;                                                        \
} else {                                                                \
  SD=myDSite;                                                           \
  OTI=var->getIndex();                                                  \
}

// mm2: simplify: first check OTI only if same compare NA
static
int compareNetAddress(ProxyManagerVar *lVar,ProxyManagerVar *rVar)
{
  GET_ADDR(lVar,lSD,lOTI);
  GET_ADDR(rVar,rSD,rOTI);
  int ret = lSD->compareSites(rSD);
  if (ret != 0) return ret;
  return lOTI<rOTI ? -1 : 1;
}

TaggedRef ProxyVar::getTaggedRef(){
  return borrowTable->getBorrow(getIndex())->getRef();}

TaggedRef ManagerVar::getTaggedRef(){
  return ownerTable->getOwner(getIndex())->getRef();}

inline
OZ_Return ProxyManagerVar::unifyV(TaggedRef *lPtr, TaggedRef *rPtr)
{
  TaggedRef rVal = *rPtr;

  if (!oz_isExtVar(rVal)) {
    // switch order
    if (isSimpleVar(rVal))  {
      return oz_var_bind(tagged2CVar(rVal),rPtr,makeTaggedRef(lPtr));
    } else {
      return bindV(lPtr,makeTaggedRef(rPtr));
    }
  }

  ExtVar *rVar = oz_getExtVar(rVal);
  int rTag=rVar->getIdV();
  if (rTag!=OZ_EVAR_PROXY && rTag!=OZ_EVAR_MANAGER) {
    return bindV(lPtr,makeTaggedRef(rPtr));
  }

  // Note: for distr. variables: isLocal == onToplevel
  if (oz_isLocalVar(this)) {
    int ret = compareNetAddress(this, (ProxyManagerVar*)rVar);
    Assert(ret!=0);
    if (ret>0) {
      return rVar->bindV(rPtr,makeTaggedRef(lPtr));
    }
  }

  return bindV(lPtr,makeTaggedRef(rPtr));
}

//
// kost@ : certain versions of gcc (e.g. 2.7.2.3/linux) have problems
// with an 'inline' version of this function: its usage in
// 'BorrowEntry::copyBorrow(BorrowEntry* from,int i)' cannot be
// resolved as in inline one, while a compiled one does not exist
// either...
void ProxyManagerVar::gcSetIndex(int i)
{
  index =  i;
}


/* --- ProxyVar --- */

OZ_Return ProxyVar::addSuspV(TaggedRef *, Suspendable * susp)
{
  if(!errorIgnore()){
    if(failurePreemption(AtomWait)) return BI_REPLACEBICALL;}
  BorrowEntry *be=BT->getBorrow(getIndex());
  addSuspSVar(susp);
  return SUSPEND;
}

void ProxyVar::gCollectRecurseV(void)
{
  PD((GC,"ProxyVar b:%d",getIndex()));
  Assert(getIndex()!=BAD_BORROW_INDEX);
  BT->getBorrow(getIndex())->gcPO();
  if (binding)
    oz_gCollectTerm(binding,binding);
  setInfo(gcEntityInfoInternal(getInfo()));
}

static
void sendSurrender(BorrowEntry *be,OZ_Term val){
  be->getOneMsgCredit();
  NetAddress *na = be->getNetAddress();
  MsgBuffer *bs=msgBufferManager->getMsgBuffer(na->site);
  marshal_M_SURRENDER(bs,na->index,myDSite,val);
  SendTo(na->site,bs,M_SURRENDER,na->site,na->index);
}

Bool dealWithInjectors(TaggedRef t,EntityInfo *info,EntityCond ec,Thread* th,Bool &hit,TaggedRef term){
  Watcher* w;
  Watcher** base= info->getWatcherBase();
  while(TRUE){
    if((*base)==NULL) return FALSE;
    if((*base)->isSiteBased()) break;
    if(th==(*base)->thread) break;
    base= &((*base)->next);}
  if(!((((*base)->watchcond)) & ec)) return FALSE;
  (*base)->varInvokeInjector(t,(*base)->watchcond & ec,term);
  hit=TRUE;
  if((*base)->isPersistent()) return FALSE;
  *base=(*base)->next;
  return TRUE;
}

Bool varFailurePreemption(TaggedRef t,EntityInfo* info,Bool &hit,TaggedRef term){
  EntityCond ec=info->getEntityCond();
  if(ec==ENTITY_NORMAL) return FALSE;
  Bool ret=dealWithInjectors(t,info,ec,oz_currentThread(),hit,term);
  if(hit) return ret;
  if(globalWatcher==NULL) return FALSE;
  ec=globalWatcher->watchcond & ec;
  if(!ec) return FALSE;
  globalWatcher->varInvokeInjector(t,ec,term);
  hit=TRUE;
  return FALSE;
}

Bool ProxyVar::failurePreemption(TaggedRef term){
  Assert(info!=NULL);
  info->dealWithWatchers(getTaggedRef(),info->getEntityCond());
  Bool hit=FALSE;
  EntityCond oldC=info->getSummaryWatchCond();
  if(varFailurePreemption(getTaggedRef(),info,hit,term)){
    EntityCond newC=info->getSummaryWatchCond();
    varAdjustPOForFailure(getIndex(),oldC,newC);}
  return hit;
}

Bool ManagerVar::failurePreemption(TaggedRef term){
  Assert(info!=NULL);
  info->dealWithWatchers(getTaggedRef(),info->getEntityCond());
  Bool hit=FALSE;
  EntityCond oldC=info->getSummaryWatchCond();
  if(varFailurePreemption(getTaggedRef(),info,hit,term)){}
  return hit;
}

#define ExportControl(Val) \
{ if(ozconf.perdioMinimal) { \
     OZ_Return ret=export(Val); \
     if(ret!=PROCEED) return ret;}}

OZ_Return ProxyVar::bindV(TaggedRef *lPtr, TaggedRef r){
  PD((PD_VAR,"ProxyVar::doBind by thread: %x",oz_currentThread()));
  PD((PD_VAR,"bind proxy b:%d v:%s",getIndex(),toC(r)));
  Bool isLocal = oz_isLocalVar(this);
  if (isLocal) {
    if(!errorIgnore()){
      if(isFuture()){
        if(failurePreemption(AtomWait)) return BI_REPLACEBICALL;}
      else{
        if(failurePreemption(mkOp1("bind",r))) return BI_REPLACEBICALL;}}
    if (!binding) {
      if(isFuture()){
        am.addSuspendVarList(lPtr);
        return SUSPEND;}
      BorrowEntry *be=BT->getBorrow(getIndex());
      ExportControl(r);
      sendSurrender(be,r);
      PD((THREAD_D,"stop thread proxy bind %x",oz_currentThread()));
      binding=r;
    }
    am.addSuspendVarList(lPtr);
    return SUSPEND;
  } else {
    // in guard: bind and trail
    if(!errorIgnore()){
      if(failurePreemption(mkOp1("bind",r))) {
        return BI_REPLACEBICALL;}}
    oz_bindGlobalVar(this,lPtr,r);
    return PROCEED;
  }
}

void ProxyVar::redoStatus(TaggedRef val,TaggedRef status){
  SiteUnify(status,oz_status(val));
}

void ProxyVar::redirect(TaggedRef *vPtr,TaggedRef val, BorrowEntry *be)
{
  int BTI=getIndex();
  if(status!=0){
    redoStatus(val,status);}
  PD((TABLE,"REDIRECT - borrow entry hit b:%d",BTI));
  if (binding) {
    DebugCode(binding=0);
    PD((PD_VAR,"REDIRECT while pending"));
  }
  EntityInfo* ei=info;
  oz_bindLocalVar(this,vPtr,val);
  be->changeToRef();
  maybeHandOver(ei,val);
  (void) BT->maybeFreeBorrowEntry(BTI);
}

void ProxyVar::acknowledge(TaggedRef *vPtr, BorrowEntry *be)
{
  int BTI=getIndex();
  if(status!=0){
    redoStatus(binding,status);}
  PD((PD_VAR,"acknowledge"));

  EntityInfo* ei=info;
  oz_bindLocalVar(this,vPtr,binding);

  be->changeToRef();
  maybeHandOver(ei,binding);
  (void) BT->maybeFreeBorrowEntry(BTI);
}

/* --- ManagerVar --- */

OZ_Return ManagerVar::addSuspV(TaggedRef *vPtr, Suspendable * susp)
{
  if(!errorIgnore()){
    if(failurePreemption(AtomWait)) return BI_REPLACEBICALL;}

  /*
  if (origVar->getType()==OZ_VAR_FUTURE) {
    if (((Future *)origVar)->kick(vPtr))
      return PROCEED;
  }
  */
  addSuspSVar(susp);
  return SUSPEND;
}

void ManagerVar::gCollectRecurseV(void)
{
  origVar=origVar->gCollectVar();
  OT->getOwner(getIndex())->gcPO();
  PD((GC,"ManagerVar o:%d",getIndex()));
  ProxyList **last=&proxies;
  for (ProxyList *pl = proxies; pl; pl = pl->next) {
    pl->sd->makeGCMarkSite();
    ProxyList *newPL = new ProxyList(pl->sd,0);
    *last = newPL;
    last = &newPL->next;}
  *last = 0;
  setInfo(gcEntityInfoInternal(getInfo()));
}

static void sendAcknowledge(DSite* sd,int OTI){
  PD((PD_VAR,"sendAck %s",sd->stringrep()));
  MsgBuffer *bs=msgBufferManager->getMsgBuffer(sd);
  OT->getOwner(OTI)->getOneCreditOwner();
  marshal_M_ACKNOWLEDGE(bs,myDSite,OTI);
  SendTo(sd,bs,M_ACKNOWLEDGE,myDSite,OTI);
}

// extern
void sendRedirect(DSite* sd,int OTI,TaggedRef val)
{
  PD((PD_VAR,"sendRedirect %s",sd->stringrep()));
  MsgBuffer *bs=msgBufferManager->getMsgBuffer(sd);
  OT->getOwner(OTI)->getOneCreditOwner();
  marshal_M_REDIRECT(bs,myDSite,OTI,val);
  SendTo(sd,bs,M_REDIRECT,myDSite,OTI);
}

inline Bool queueTrigger(DSite* s){
  int msgs;
  if(s->getQueueStatus(msgs)>0) return TRUE;
  return FALSE;}

// ERIK-LOOK use antoher ozconf.

static inline Bool canSend(DSite* s){
  int msgs;
  if(s->getQueueStatus(msgs)>ozconf.perdioFlowBufferSize) return FALSE;
  return TRUE;}

Bool varCanSend(DSite* s){
  return canSend(s);}

void ManagerVar::sendRedirectToProxies(OZ_Term val, DSite* ackSite)
{
  PD((PD_VAR,"sendRedirectToProxies"));
  ProxyList *pl = proxies;
  while (pl) {
    DSite* sd = pl->sd;
    Assert(sd!=myDSite);
    if (sd==ackSite) {
      sendAcknowledge(sd,getIndex());}
    else {
      if(!canSend(sd)){
        flowControler->addElement(val,sd,getIndex());}
      else{
        if(!(USE_ALT_VAR_PROTOCOL) && (pl->kind==EXP_REG || queueTrigger(sd))){
          //NOTE globalRedirect is only important if we use the alt var
          //NOTE protocol.
          globalRedirectFlag=EXP_REG;
          sendRedirect(sd,getIndex(),val);
          globalRedirectFlag=AUT_REG;}
        else{
          Assert(pl->kind==AUT_REG);
          sendRedirect(sd,getIndex(),val);}}}
    pl=pl->dispose();
  }
  proxies = 0;
}


OZ_Return ManagerVar::bindVInternal(TaggedRef *lPtr, TaggedRef r,DSite *s)
{
  int OTI=getIndex();
  PD((PD_VAR,"ManagerVar::doBind by thread: %x",oz_currentThread()));
  PD((PD_VAR,"bind manager o:%d v:%s",OTI,toC(*lPtr)));
  Bool isLocal = oz_isLocalVar(this);
  if (isLocal) {
    if(isFuture()){
      am.addSuspendVarList(lPtr);
      return SUSPEND;
    }
    ExportControl(r);
    EntityInfo *ei=info;
    sendRedirectToProxies(r, s);
    oz_bindLocalVar(this,lPtr,r);
    OT->getOwner(OTI)->changeToRef();
    maybeHandOver(ei,r);
    return PROCEED;
  } else {
    oz_bindGlobalVar(this,lPtr,r);
    return PROCEED;
  }
}

OZ_Return ManagerVar::bindV(TaggedRef *lPtr, TaggedRef r){
  if(!errorIgnore()){
    if(failurePreemption(mkOp1("bind",r))) return BI_REPLACEBICALL;}
  return bindVInternal(lPtr,r,myDSite);}

void varGetStatus(DSite* site,int OTI, TaggedRef tr){
  MsgBuffer *bs=msgBufferManager->getMsgBuffer(site);
  OT->getOwner(OTI)->getOneCreditOwner();
  marshal_M_SENDSTATUS(bs,myDSite,OTI,tr);
  SendTo(site,bs,M_SENDSTATUS,myDSite,OTI);
}

void ProxyVar::receiveStatus(TaggedRef tr){
  Assert(status!=0);
  SiteUnify(status,tr);
  status=0;
}


OZ_Return ManagerVar::forceBindV(TaggedRef *lPtr, TaggedRef r)
{
  int OTI=getIndex();
  PD((PD_VAR,"ManagerVar::doBind by thread: %x",oz_currentThread()));
  PD((PD_VAR,"bind manager o:%d v:%s",OTI,toC(*lPtr)));
  Bool isLocal = oz_isLocalVar(this);
  if (isLocal) {
    // send redirect done first to check if r is exportable
    ExportControl(r);
    sendRedirectToProxies(r, myDSite);
    EntityInfo *ei=info;
    oz_bindLocalVar(this,lPtr,r);
    OT->getOwner(OTI)->changeToRef();
    maybeHandOver(ei,r);
    return PROCEED;
  } else {
    oz_bindGlobalVar(this,lPtr,r);
    return PROCEED;
  }
}

void ManagerVar::surrender(TaggedRef *vPtr, TaggedRef val)
{
  OZ_Return ret = bindV(vPtr,val);
  Assert(ret==PROCEED);
}

void requested(TaggedRef*);

/* --- Marshal --- */

void ManagerVar::marshal(MsgBuffer *bs)
{
  int i=getIndex();
  PD((MARSHAL,"var manager o:%d",i));
  if((USE_ALT_VAR_PROTOCOL) && globalRedirectFlag==AUT_REG){
    if(isFuture()){
      marshalOwnHead(DIF_FUTURE_AUTO,i,bs);}
    else{
      marshalOwnHead(DIF_VAR_AUTO,i,bs);}
    registerSite(bs->getSite());
    return;}
  if(isFuture()){
    marshalOwnHead(DIF_FUTURE,i,bs);}
  else{
    marshalOwnHead(DIF_VAR,i,bs);}
}

//
void ProxyVar::marshal(MsgBuffer *bs)
{
  DSite *sd=bs->getSite();
  int i=getIndex();
  PD((MARSHAL,"var proxy o:%d",i));
  if (sd && borrowTable->getOriginSite(i) == sd) {
    marshalToOwner(i, bs);
  } else {
    if (isFuture()) {
      marshalBorrowHead(DIF_FUTURE, i, bs);
    } else {
      marshalBorrowHead(DIF_VAR, i, bs);
    }
  }
}


ManagerVar* globalizeFreeVariable(TaggedRef *tPtr){
  OwnerEntry *oe;
  int i = ownerTable->newOwner(oe);
  PD((GLOBALIZING,"globalize var index:%d",i));
  oe->mkVar(makeTaggedRef(tPtr));
  OzVariable *cv = oz_getVar(tPtr);
  ManagerVar *mv = new ManagerVar(cv,i);
  mv->setSuspList(cv->unlinkSuspList());
  *tPtr=makeTaggedCVar(mv);
  return mv;
}

// Returning 'NO' means we are going to proceed with 'marshal bomb';
Bool marshalVariableImpl(TaggedRef *tPtr, MsgBuffer *bs)
{
  const TaggedRef var = *tPtr;
  if (oz_isManagerVar(var)) {
    if (!bs->globalize()) return TRUE;
    oz_getManagerVar(var)->marshal(bs);
  } else if (oz_isProxyVar(var)) {
    if (!bs->globalize()) return TRUE;
    oz_getProxyVar(var)->marshal(bs);
  } else if (oz_isObjectVar(var)) {
    Assert(bs->globalize());
    oz_getObjectVar(var)->marshal(bs);
  } else if (oz_isFree(var) || isFuture(var)) {
    if (!bs->globalize()) return TRUE;
    Assert(perdioInitialized);
    globalizeFreeVariable(tPtr)->marshal(bs);
  } else {
    return FALSE;
  }
  return TRUE;
}

Bool triggerVariableImpl(TaggedRef *tPtr){
  const TaggedRef var = *tPtr;
  if(isFuture(var)){
    return ((Future*) oz_getVar(tPtr))->kick(tPtr);}
  return FALSE;
}

/* --- Unmarshal --- */

static void sendRegister(BorrowEntry *be) {
  PD((PD_VAR,"sendRegister"));
  Assert(creditSiteOut == NULL);
  be->getOneMsgCredit();
  NetAddress *na = be->getNetAddress();
  MsgBuffer *bs=msgBufferManager->getMsgBuffer(na->site);
  marshal_M_REGISTER(bs,na->index,myDSite);
  SendTo(na->site,bs,M_REGISTER,na->site,na->index);
}

static void sendDeRegister(BorrowEntry *be) {
  PD((PD_VAR,"sendDeRegister"));
  Assert(creditSiteOut == NULL);
  be->getOneMsgCredit();
  NetAddress *na = be->getNetAddress();
  MsgBuffer *bs=msgBufferManager->getMsgBuffer(na->site);
  marshal_M_DEREGISTER(bs,na->index,myDSite);
  SendTo(na->site,bs,M_DEREGISTER,na->site,na->index);
}

void ProxyVar::nowGarbage(BorrowEntry* be){
  PD((PD_VAR,"nowGarbage"));
  sendDeRegister(be);}

// extern
OZ_Term unmarshalVarImpl(MsgBuffer* bs, Bool isFuture, Bool isAuto){
  OB_Entry *ob;
  int bi;
  OZ_Term val1 = unmarshalBorrow(bs,ob,bi);

  if (val1) {
    PD((UNMARSHAL,"var/chunk hit: b:%d",bi));
    return val1;}

  PD((UNMARSHAL,"var miss: b:%d",bi));
  ProxyVar *pvar = new ProxyVar(oz_currentBoard(),bi,isFuture);

  TaggedRef val = makeTaggedRef(newTaggedCVar(pvar));
  ob->changeToVar(val); // PLEASE DONT CHANGE THIS
  if(!isAuto) {
    sendRegister((BorrowEntry *)ob);}
  else{
    pvar->makeAuto();}
  /*
    switch(((BorrowEntry*)ob)->getSite()->siteStatus()){
    case SITE_OK:{
    break;}
    case SITE_PERM:{
    deferProxyProbeFault(tert,PROBE_PERM);
    break;}
    case SITE_TEMP:{
    deferProxyProbeFault(tert,PROBE_TEMP);
    break;}
    default:
    Assert(0);
    }
  */
  return val;
}

/* --- IsVar test --- */

static
void sendGetStatus(BorrowEntry *be){
  be->getOneMsgCredit();
  NetAddress *na = be->getNetAddress();
  MsgBuffer *bs=msgBufferManager->getMsgBuffer(na->site);
  marshal_M_GETSTATUS(bs,myDSite,na->index);
  SendTo(na->site,bs,M_GETSTATUS,na->site,na->index);
}

OZ_Term ProxyVar::statusV()
{
  if(status ==0){
    BorrowEntry *be=BT->getBorrow(getIndex());
    sendGetStatus(be);
    status= oz_newVariable();}
  return status;
}

VarStatus ProxyVar::checkStatusV()
{
  return EVAR_STATUS_UNKNOWN;
}

/* --- IsVar test --- */

inline
void ManagerVar::localize(TaggedRef *vPtr)
{
  Assert(getInfo()==NULL);
  origVar->setSuspList(unlinkSuspList());
  *vPtr=makeTaggedCVar(origVar);
  origVar=0;
  disposeV();
}

OZ_Term ManagerVar::statusV() {
  return origVar->getType()==OZ_VAR_FUTURE ? AtomFuture : AtomFree;
}

VarStatus ManagerVar::checkStatusV(){
  return origVar->getType()==OZ_VAR_FUTURE ?  EVAR_STATUS_FUTURE :
    EVAR_STATUS_FREE;
}

void oz_dpvar_localize(TaggedRef *vPtr) {
  Assert(classifyVar(vPtr)==VAR_MANAGER);
  oz_getManagerVar(*vPtr)->localize(vPtr);
}


// FAILURE structure fundamentals

// mm3 tPtr comes from DEREF(_,_,tPtr)

VarKind classifyVarLim(TaggedRef tr){
  if(oz_isProxyVar(tr)){
    return VAR_PROXY;}
  if(oz_isManagerVar(tr)){
    return VAR_MANAGER;}
  if(oz_isObjectVar(tr)){
    return VAR_OBJECT;}
  Assert(0);
  return VAR_FREE;
}

VarKind classifyVar(TaggedRef* tPtr){
  TaggedRef tr= *tPtr;
  if(oz_isProxyVar(tr)){
    return VAR_PROXY;}
  if(oz_isManagerVar(tr)){
    return VAR_MANAGER;}
  if(oz_isObjectVar(tr)){
    return VAR_OBJECT;}
  if(oz_isFree(tr)){
    return VAR_FREE;}
  if(isFuture(tr)){
    return VAR_FUTURE;}
  return VAR_KINDED;
}


EntityInfo* varGetEntityInfo(TaggedRef* tPtr){
  switch(classifyVar(tPtr)){
  case VAR_MANAGER:
    return oz_getManagerVar(*tPtr)->getInfo();
  case VAR_PROXY:
    return oz_getProxyVar(*tPtr)->getInfo();
  case VAR_OBJECT:
    return oz_getObjectVar(*tPtr)->getInfo();
  default:
    Assert(0);}
  return NULL;}

EntityInfo* varMakeEntityInfo(TaggedRef* tPtr){
  EntityInfo* ei= new EntityInfo();
  switch(classifyVar(tPtr)){
  case VAR_MANAGER:
    oz_getManagerVar(*tPtr)->setInfo(ei);
    return ei;
  case VAR_PROXY:
    oz_getProxyVar(*tPtr)->setInfo(ei);
    return ei;
  case VAR_OBJECT:
    oz_getObjectVar(*tPtr)->setInfo(ei);
    return ei;
  default:
    Assert(0);}
  return NULL;
}

EntityInfo* varMakeOrGetEntityInfo(TaggedRef* tPtr){
  EntityInfo* ei=varGetEntityInfo(tPtr);
  if(ei!=NULL) return ei;
  return varMakeEntityInfo(tPtr);
}

// FAILURE stuff

void ProxyVar::addEntityCond(EntityCond ec){
  if(info==NULL) info= new EntityInfo();
  if(!info->addEntityCond(ec)) return;
  wakeAll();
  info->dealWithWatchers(getTaggedRef(),ec);
}

void ProxyVar::newWatcher(Bool b){
  if(b){
    wakeAll();
    return;}
  info->dealWithWatchers(getTaggedRef(),info->getEntityCond());
}

void ProxyVar::subEntityCond(EntityCond ec){
  Assert(info!=NULL);
  info->subEntityCond(ec);
}

void ManagerVar::newWatcher(Bool b){
  if(b) {
    wakeAll();
    return;}
  info->dealWithWatchers(getTaggedRef(),info->getEntityCond());
}

void ManagerVar::addEntityCond(EntityCond ec){
  if(info==NULL) info= new EntityInfo();
  if(!info->addEntityCond(ec)) return;
  int i=getIndex();
  OwnerEntry* oe=OT->getOwner(i);
  triggerInforms(&inform,oe,i,ec);
  wakeAll();
  info->dealWithWatchers(getTaggedRef(),ec);
}

void ManagerVar::subEntityCond(EntityCond ec){
  Assert(info!=NULL);
  info->subEntityCond(ec);
  int i=getIndex();
  OwnerEntry* oe=OT->getOwner(i);
  triggerInforms(&inform,oe,i,ec);
}

Bool ManagerVar::siteInProxyList(DSite* s){
  ProxyList* pl=proxies;
  while(pl!=NULL){
    if(pl->sd==s) return TRUE;
    pl=pl->next;}
  return FALSE;
}

void ManagerVar::probeFault(DSite *s,int pr){
  if(!siteInProxyList(s)) return;
  if(pr==PROBE_PERM){
    deregisterSite(s);
    addEntityCond(PERM_SOME);
    return;}
  if(pr==PROBE_TEMP){
    addEntityCond(TEMP_SOME);
    return;}
  Assert(pr==PROBE_OK);
  subEntityCond(TEMP_SOME);
}

void ProxyVar::probeFault(int pr){
  if(pr==PROBE_PERM){
    addEntityCond(PERM_FAIL|PERM_SOME);
    return;}
  if(pr==PROBE_TEMP){
    addEntityCond(TEMP_FAIL|TEMP_SOME);
    return;}
  Assert(pr==PROBE_OK);
  subEntityCond(TEMP_FAIL|TEMP_SOME);
}

EntityCond varGetEntityCond(TaggedRef* tr){
  return varGetEntityInfo(tr)->getEntityCond();
}

VarKind typeOfBorrowVar(BorrowEntry* b){
  Assert(b->isVar());
  ExtVar *ev=oz_getExtVar((oz_deref(b->getRef())));
  switch(ev->getIdV()){
  case OZ_EVAR_PROXY:
    return VAR_PROXY;
  case OZ_EVAR_OBJECT:
    return VAR_OBJECT;
  default:
    Assert(0);}
  return VAR_PROXY; // stupid compiler
}

Bool errorIgnoreVar(BorrowEntry* b){
  EntityInfo* ie;
  switch(typeOfBorrowVar(b)){
  case VAR_PROXY:
    ie = GET_VAR(b,Proxy)->getInfo();
    if(ie==NULL) return TRUE;
    return FALSE;
  case VAR_OBJECT:
    ie= GET_VAR(b,Object)->getInfo();
    if(ie==NULL) return TRUE;
    return FALSE;
  default:
    Assert(0);}
  return FALSE; // stupid compiler
}

void maybeUnaskVar(BorrowEntry* b){
  EntityInfo* ie;
  switch(typeOfBorrowVar(b)){
  case VAR_PROXY:
    ie= GET_VAR(b,Proxy)->getInfo();
    if(ie==NULL) return;
    varAdjustPOForFailure(GET_VAR(b,Proxy)->getIndex(),
                          ie->getEntityCond(),ENTITY_NORMAL);
    return;
  case VAR_OBJECT:{
    ie= GET_VAR(b,Object)->getInfo();
    if(ie==NULL) return;
    int i=GET_VAR(b,Object)->getObject()->getIndex();
    varAdjustPOForFailure(i,ie->getEntityCond(),ENTITY_NORMAL);
    return;}
  default:
    Assert(0);}
}

void ManagerVar::newInform(DSite* s,EntityCond ec){
  InformElem* ie=new InformElem(s,ec);
  ie->next=inform;
  inform=ie;
}

void ProxyVar::wakeAll(){
  oz_checkSuspensionList(this,pc_all);
}

void ManagerVar::wakeAll(){
  oz_checkSuspensionList(this,pc_all);
}

void recDeregister(TaggedRef tr,DSite* s){
  OZ_Term vars=digOutVars(tr);
  while(!oz_isNil(vars)){
    OZ_Term t=oz_head(vars);
    DEREF(t,tPtr,_2);
    if(classifyVar(tPtr)==VAR_MANAGER){
      oz_getManagerVar(*tPtr)->deAutoSite(s);
    }
    vars=oz_tail(vars);}
}

static ProxyList** findBefore(DSite* s,ProxyList** base ){
  while((*base)!=NULL){
    if((*base)->sd==s) return base;
    base= &((*base)->next);}
  return NULL;}

void ManagerVar::deAutoSite(DSite* s){
  ProxyList **aux= findBefore(s, &proxies);
  if (aux!=NULL && *aux!=NULL){
    ProxyList *pl=*aux;
    pl->kind=EXP_REG;}
}

void ManagerVar::deregisterSite(DSite* s){
  ProxyList **pl=findBefore(s, &proxies);
  Assert(pl!=NULL);
  *pl= (*pl)->next;
}
