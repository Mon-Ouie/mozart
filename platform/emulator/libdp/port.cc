/*
 *  Authors:
 *    Per Brand (perbrand@sics.se)
 *    Erik Klintskog (erik@sics.se)
 * 
 *  Contributors:
 *    optional, Contributor's name (Contributor's email address)
 * 
 *  Copyright:
 *    Erik Klintskog, 1998
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
#pragma implementation "port.hh"
#endif

#include "base.hh"
#include "builtins.hh"
#include "value.hh"
#include "dpBase.hh"
#include "perdio.hh"
#include "port.hh"
#include "table.hh"
#include "controlvar.hh"
//#include "dpMarshaler.hh"
#include "dpInterface.hh"
#include "flowControl.hh"
#include "ozconfig.hh"
#include "msgContainer.hh"

/**********************************************************************/
/*   SECTION Port protocol                                       */
/**********************************************************************/

OZ_Return portSendInternal(Tertiary *p, TaggedRef msg){
  Assert(p->getTertType()==Te_Proxy);
  BorrowEntry* b = BT->getBorrow(p->getIndex());
  NetAddress *na = b->getNetAddress();
  DSite* site     = na->site;
  int index      = na->index;
  
// AN! What should be done about this?
//  OZ_Term nogoods = bs->getNoGoods();
//    if (!oz_eq(oz_nil(),nogoods)) {
//    /*
//      int portIndex;
//      OZ_Term t;
//      unmarshal_M_PORT_SEND(bs,portIndex,t);
//      dumpRemoteMsgBuffer(bs);
//      */
//      return raiseGeneric("portSend:resources",
//  			"Resources found during send to port",
//  			oz_mklist(OZ_pairA("Resources",nogoods),
//  				  OZ_pairA("Port",makeTaggedTert(p))));
//    }

  MsgContainer *msgC = msgContainerManager->newMsgContainer(site);
  msgC->put_M_PORT_SEND(index,msg);
  msgC->setImplicitMessageCredit(b->getOneMsgCredit());

  PD((PORT,"sendingTo %s %d",site->stringrep(),index));
  send(msgC,3);

  return PROCEED;
}

//

OZ_Return portSendImpl(Tertiary *p, TaggedRef msg) 
{
  Assert(p->getTertType()==Te_Proxy);
  if(getEntityCond(p)!= ENTITY_NORMAL){
    //    printf("PortProblem %d at: %s",getEntityCond(p),myDSite->stringrep());
    //printf("from: %s\n",
    //	   BT->getBorrow(p->getIndex())->getNetAddress()->site->stringrep());
    
    pendThreadAddToEnd(&(((PortProxy*)p)->pending),
		       msg,msg,NOEX);
    deferEntityProblem(p);
    return SuspendOnControlVarReturnValue;
    
  }
  if(((PortProxy*)p)->pending!= NULL || !((PortProxy*)p)->canSend()){
    pendThreadAddToEnd(&(((PortProxy*)p)->pending),
		       msg,msg,NOEX);
    flowControler->addElement(makeTaggedConst(p));
    return SuspendOnControlVarReturnValue;
  }
  Assert(((PortProxy*)p)->pending == NULL);
  return portSendInternal(p,msg);
}


void gcDistPortRecurseImpl(Tertiary *p) 
{
  Assert(!p->isLocal());
  //
  gcEntityInfoImpl(p);
  if (p->isProxy()) {
    gcProxyRecurseImpl(p);
    gCollectPendThread(&(((PortProxy*)p)->pending));
  } else {
    gcManagerRecurseImpl(p);
    PortWithStream *pws = (PortWithStream *) p;
    
    oz_gCollectTerm(pws->strm,pws->strm);
  }
}

Bool  PortProxy::canSend(){
   BorrowEntry* b = BT->getBorrow(this->getIndex());
   NetAddress *na = b->getNetAddress();
   DSite* site     = na->site;
   /*
     if(!(site->getQueueStatus() < 
     ozconf.perdioFlowBufferSize))
	printf("ps c:%d max: %d\n",
	site->getQueueStatus(dummy), 
	ozconf.perdioFlowBufferSize);
   */
   return(site->getQueueStatus() < 
	  ozconf.perdioFlowBufferSize);}


void  PortProxy::wakeUp(){
  OZ_Return ret;
  PendThread *old;
  while(pending!=NULL){
    if(getEntityCond(this)!= ENTITY_NORMAL){
      entityProblem(this);
      return;}
    if(!this->canSend()){
	flowControler->addElement(makeTaggedConst(this));
	return;}
    
    if (pending->thread != NULL){
      ret = portSendInternal(this, pending->nw);
      if(ret!=PROCEED)
	ControlVarRaise(pending->controlvar, ret);
      else
	ControlVarResume(pending->controlvar);
    }
    old = pending;
    pending = pending->next;
    old->dispose();
  }
}
    

/**************************************************************/
/*      Failure                                               */
/**************************************************************/


void port_Temp(PortProxy* pp){
  EntityCond ec = TEMP_FAIL;
  if(!addEntityCond(pp,ec)) return;
  entityProblem(pp);     
}
void port_Ok(PortProxy* pp){
  EntityCond ec = TEMP_FAIL;
  subEntityCond(pp,ec);
  pp->wakeUp();
}
void port_Perm(PortProxy* pp){
  //printf("SettingPerm to port %s\n",BT->getBorrow(pp->getIndex())->getNetAddress()->site->stringrep());
  EntityCond ec = PERM_FAIL;
  if(!addEntityCond(pp,ec)) return;
  entityProblem(pp);     
}
PendThread* getPendThreadStartFromPort(Tertiary* t){
    return ((PortProxy*)t)->pending;}











