/*
 *  Authors:
 *    Michael Mehl (mehl@dfki.de)
 * 
 *  Contributors:
 *    Per Brand (perbrand@sics.se)
 *    Ralf Scheidhauer (Ralf.Scheidhauer@ps.uni-sb.de)
 *    Erik Klintskog (erik@sics.se)
 * 
 *  Copyright:
 *    Michael Mehl (1997,1998)
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
#pragma implementation "var_obj.hh"
#endif

#include "var_obj.hh"
#include "dpMarshaler.hh"
#include "dpBase.hh"
#include "gname.hh"
#include "unify.hh"
#include "fail.hh"


//
void ObjectVar::marshal(MsgBuffer *bs)
{
  PD((MARSHAL,"var objectproxy"));
  GName *classgn = isObjectClassAvail()
    ? globalizeConst(getClass(), bs) : getGNameClass();
  marshalVarObject(getObject(), bs, classgn);
}

/* --- ObjectProxis --- */

// mm2: deep as future!
OZ_Return ObjectVar::bindV(TaggedRef *lPtr, TaggedRef r)
{
  am.addSuspendVarList(lPtr);
  return SUSPEND;
}

// mm2: deep as future!
OZ_Return ObjectVar::unifyV(TaggedRef *lPtr, TaggedRef *rPtr)
{
  return oz_var_bind(tagged2CVar(*rPtr),rPtr,makeTaggedRef(lPtr));
}

// extern
TaggedRef newObjectProxy(Object *o, GName *gnobj,
			 GName *gnclass, TaggedRef clas)
{
  ObjectVar *pvar;
  if (gnclass) {
    pvar = new ObjectVar(oz_currentBoard(),o,gnclass);
  } else {
    pvar = new ObjectVar(oz_currentBoard(),o,
			 tagged2ObjectClass(oz_deref(clas)));
  }
  TaggedRef val = makeTaggedRef(newTaggedCVar(pvar));
  addGName(gnobj, val);
  return val;
}


static
void sendRequest(MessageType mt,BorrowEntry *be)
{
  be->getOneMsgCredit();
  NetAddress* na=be->getNetAddress();
  MsgBuffer *bs=msgBufferManager->getMsgBuffer(na->site);
  switch (mt) {
  case M_GET_OBJECT:
    marshal_M_GET_OBJECT(bs,na->index,myDSite);
    break;
  case M_GET_OBJECTANDCLASS:
    marshal_M_GET_OBJECTANDCLASS(bs,na->index,myDSite);
    break;
  default:
    Assert(0);
  }
  SendTo(na->site,bs,mt,na->site,na->index);
}

OZ_Return ObjectVar::addSuspV(TaggedRef * v, Suspendable * susp)
{
  if(!errorIgnore()){
    if(failurePreemption()) return BI_REPLACEBICALL;}

  addSuspSVar(susp);
  if (!requested) {
    requested = 1;
    MessageType mt = M_GET_OBJECT; 
    if (isObjectClassNotAvail() &&
	oz_findGName(getGNameClass())==0) {
      mt = M_GET_OBJECTANDCLASS;
    }
    BorrowEntry *be=BT->getBorrow(getObject()->getIndex());      
    sendRequest(mt,be);
  }
  return SUSPEND;
}


void ObjectVar::gCollectRecurseV(void)
{
  BT->getBorrow(getObject()->getIndex())->gcPO();
  obj = (Object *) getObject()->gCollectConstTerm();
  if (isObjectClassAvail()) {
    u.aclass = (ObjectClass *) u.aclass->gCollectConstTerm();}
  setInfo(gcEntityInfoInternal(getInfo()));
}

void ObjectVar::disposeV()
{
  disposeS();
  if (isObjectClassNotAvail()) {
    deleteGName(u.gnameClass);
  }
  freeListDispose(this,sizeof(ObjectVar));
}


OZ_Term ObjectVar::statusV()
{
  SRecord *t = SRecord::newSRecord(AtomDet, 1);
  t->setArg(0, AtomObject);
  return makeTaggedSRecord(t);
}

VarStatus ObjectVar::checkStatusV()
{
  return EVAR_STATUS_DET;
}


void ObjectVar::sendObject(DSite* sd, int si, ObjectFields& of,
			   BorrowEntry *be)
{
  Object *o = getObject();
  Assert(o);
  GName *gnobj = o->getGName1();
  Assert(gnobj);
  gnobj->setValue(makeTaggedConst(o));
            
  fillInObject(&of,o);
  ObjectClass *cl;
  if (isObjectClassAvail()) {
    cl=getClass();
  } else {
    cl=tagged2ObjectClass(oz_deref(oz_findGName(getGNameClass())));
  }
  o->setClass(cl);
  Assert(be->isVar());
  oz_bindLocalVar(this,be->getPtr(),makeTaggedConst(o));
  be->changeToRef();
  int index=o->getIndex();
  maybeHandOver(info,makeTaggedConst(o));
  o->localize();
  (void) BT->maybeFreeBorrowEntry(index);
}

void ObjectVar::sendObjectAndClass(ObjectFields& of, BorrowEntry *be)
{
  Object *o = getObject();
  Assert(o);
  GName *gnobj = o->getGName1();
  Assert(gnobj);
  gnobj->setValue(makeTaggedConst(o));
      
  fillInObjectAndClass(&of,o);
  EntityInfo *savedInfo = info; // bind disposes this!
  Assert(be->isVar());
  oz_bindLocalVar(this,be->getPtr(),makeTaggedConst(o));
  be->changeToRef();
  int index=o->getIndex();
  maybeHandOver(savedInfo,makeTaggedConst(o));
  o->localize(); 
  (void) BT->maybeFreeBorrowEntry(index);
}

// failure stuff

Bool ObjectVar::failurePreemption(){
  Bool hit=FALSE;
  Assert(info!=NULL);
  info->dealWithWatchers(getTaggedRef(),info->getEntityCond());
  EntityCond oldC=info->getSummaryWatchCond();  
  if(varFailurePreemption(getTaggedRef(),info,hit,AtomObjectFetch)){
    EntityCond newC=info->getSummaryWatchCond();
    varAdjustPOForFailure(getObject()->getIndex(),oldC,newC);}
  return hit;
}

void ObjectVar::addEntityCond(EntityCond ec){
  if(info==NULL) info=new EntityInfo();
  if(!info->addEntityCond(ec)) return;
  wakeAll();
  info->dealWithWatchers(getTaggedRef(),ec);
}

void ObjectVar::subEntityCond(EntityCond ec){
  Assert(info!=NULL);
  info->subEntityCond(ec);
}
  
void ObjectVar::probeFault(int pr){
  if(pr==PROBE_PERM){
    addEntityCond(PERM_FAIL);
    return;}
  if(pr==PROBE_TEMP){
    addEntityCond(TEMP_FAIL);    
    return;}
  Assert(pr==PROBE_OK);
  subEntityCond(TEMP_FAIL);
}
  
Bool ObjectVar::errorIgnore(){
  if(info==NULL) return TRUE;
  if(info->getEntityCond()==ENTITY_NORMAL) return TRUE;
  return FALSE;}

void ObjectVar::wakeAll(){ // mm3
  oz_checkSuspensionList(this,pc_all);
}

void ObjectVar::newWatcher(Bool b){
  if(b){
    wakeAll();
    return;}
  info->dealWithWatchers(getTaggedRef(),info->getEntityCond());
}

TaggedRef ObjectVar::getTaggedRef(){
  return borrowTable->getBorrow(getObject()->getIndex())->getRef();}



