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
 *     http://www.mozart-oz.org
 * 
 *  See the file "LICENSE" or
 *     http://www.mozart-oz.org/LICENSE.html
 *  for information on usage and redistribution 
 *  of this file, and for a DISCLAIMER OF ALL 
 *  WARRANTIES.
 *
 */

// Initialization of network layer instances

#include "base.hh"
#include "dpBase.hh"

#include "network.hh"
#include "comObj.hh"
#include "transObj.hh"
#include "tcpTransObj.hh" // DEVEL? AN
#include "byteBuffer.hh"
#include "connection.hh"
#include "timers.hh"

ComController *comController;
TCPTransController *tcptransController;
ByteBufferManager *byteBufferManager;
Timers *timers;

Bool networkNotInitiated = TRUE;
Bool ipIsbehindFW = FALSE;

/************************************************************/
/* SECTION 12b:  Exported to Perdio                           */
/************************************************************/

int openclose(int Type) {
//    int state = 0;
//    if(tcpCache->openCon) state = 1;
//    if(Type){
//      if(state) (void) tcpCache->closeConnections();
//      else tcpCache->openConnections();}
//    return state;
    return 0;
}

int startNiceClose(){
  return comController->closeDownCount();
}

int niceCloseProgress(){
  return comController->closeDownCount();
}

void setIPAddress(int adr) {
  if(networkNotInitiated) setIPAddress__(adr);
}

void setIPPort(int port){
  if(networkNotInitiated) {
    setIPPort__(port);
  }
}

void setFirewallStatus(Bool fw){
  if(networkNotInitiated)ipIsbehindFW = fw;}

Bool getFireWallStatus(){return ipIsbehindFW;}

ComObj *createComObj(DSite *site,int recCtr) {
  ComObj *comObj = comController->newComObj(site,recCtr);
  return comObj;
}

void comController_gcComObjs() {
  comController->gcComObjs();
}

/* *****************************************************************
   *****************************************************************
     STARTUP
   *****************************************************************
 * *************************************************************** */

void initNetwork() {
  PD((TCP_INTERFACE,"Init Network"));

  comController = new ComController();
  tcptransController = new TCPTransController();
  byteBufferManager = new ByteBufferManager();
  timers = new Timers();

  if(!initAccept())
    return;

  networkNotInitiated = FALSE;
}
  
/* ************************************************************************ */
/*  SECTION 43: DistPane-Info                                               */
/* ************************************************************************ */

int getNOSM_ComObj(ComObj* comObj) {
  return comObj->getNOSM();
}

int getNORM_ComObj(ComObj* comObj) {
  return comObj->getNORM();
}

int getComControllerInfo(int &size){
  size = sizeof(ComObj);
  return comController->getCTR();
}

int getTransControllerInfo(int &size) {
  // Temporary solution, should work with any type of transObj and include
  // buffers.
  size = sizeof(TCPTransObj)+2*30000;
  return tcptransController->getCTR();
}

int getMsgContainerManagerInfo(int &size) {
  size = sizeof(MsgContainer);
  return msgContainerManager->getCTR();
}


