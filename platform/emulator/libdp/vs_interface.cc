/*
 *  Authors:
 *    Konstantin Popov
 * 
 *  Contributors:
 *
 *  Copyright:
 *    Konstantin Popov 1997-1998
 * 
 *  Last change:
 *    $Date$
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

#include "base.hh"
#include "dpBase.hh"
#include "msgType.hh"
#include "comm.hh"

//
// Zeroth: define dummy "VS not configured" interface method
// implementations:

//
VirtualSite* createVirtualSiteStub(DSite *)
{
  error("'createVirtualSite' called without 'VIRTUALSITES'?");
  return ((VirtualSite *) 0);
}
//
void marshalVirtualInfoStub(VirtualInfo *, MsgBuffer *)
{
  error("'marshalVirtualInfo' called without 'VIRTUALSITES'?");
}
//
VirtualInfo* unmarshalVirtualInfoStub(MsgBuffer *)
{
  error("'unmarshalVirtualInfo' called without 'VIRTUALSITES'?");
  return ((VirtualInfo *) 0);
}
//
void unmarshalUselessVirtualInfoStub(MsgBuffer *)
{
  error("'unmarshalUselessVirtualInfo' called without 'VIRTUALSITES'?");
}
//
void zeroRefsToVirtualStub(VirtualSite *)
{
  error("'zeroRefsToVirtual' called without 'VIRTUALSITES'?");
}
//
int sendTo_VirtualSiteStub(VirtualSite*, MsgBuffer*, MessageType, DSite*, int)
{
  error("'sendTo_VirtualSite' called without 'VIRTUALSITES'?");
  return (-1);
}
//
int discardUnsentMessage_VirtualSiteStub(VirtualSite*, int)
{
  error("'discardUnsentMessage_VirtualSite' called without 'VIRTUALSITES'?");
  return (-1);
}
//
int getQueueStatus_VirtualSiteStub(VirtualSite*, int &)
{
  error("'getQueueStatus_VirtualSite' called without 'VIRTUALSITES'?");
  return (-1);
}
//
SiteStatus siteStatus_VirtualSiteStub(VirtualSite *)
{
  error("'siteStatus_VirtualSite' called without 'VIRTUALSITES'?");
  return ((SiteStatus) -1);
}
//
MonitorReturn monitorQueue_VirtualSiteStub(VirtualSite*, int, int, void*)
{
  error("'monitorQueue_VirtualSite' called without 'VIRTUALSITES'?");
  return ((MonitorReturn) -1);
}
//
MonitorReturn demonitorQueue_VirtualSiteStub(VirtualSite *)
{
  error("'demonitorQueue_VirtualSite' called without 'VIRTUALSITES'?");
  return ((MonitorReturn) -1);
}
//
ProbeReturn installProbe_VirtualSiteStub(VirtualSite*, ProbeType, int)
{
  error("'installProbe_VirtualSite' called without 'VIRTUALSITES'?");
  return ((ProbeReturn) -1);
}
//
ProbeReturn deinstallProbe_VirtualSiteStub(VirtualSite*, ProbeType pt)
{
  error("'installProbe_VirtualSite' called without 'VIRTUALSITES'?");
  return ((ProbeReturn) -1);
}
//
ProbeReturn probeStatus_VirtualSiteStub(VirtualSite*, ProbeType&, int&, void*&)
{
  error("'probeStatus_VirtualSite' called without 'VIRTUALSITES'?");
  return ((ProbeReturn) -1);
}
//
GiveUpReturn giveUp_VirtualSiteStub(VirtualSite *)
{
  error("'giveUp_VirtualSite' called without 'VIRTUALSITES'?");
  return ((GiveUpReturn) -1);
}
//
void discoveryPerm_VirtualSiteStub(VirtualSite *)
{
  error("'discoveryPerm_VirtualSite' called without 'VIRTUALSITES'?");
}
//
void dumpVirtualInfoStub(VirtualInfo *)
{
  error("'dumpVirtualInfo' called without 'VIRTUALSITES'?");
}
//
MsgBuffer* getVirtualMsgBufferStub(DSite *)
{
  error("'getVirtualMsgBuffer' called without 'VIRTUALSITES'?");
  return ((MsgBuffer *) 0);
}
//
void dumpVirtualMsgBufferStub(MsgBuffer *)
{
  error("'dumpVirtualMsgBuffer' called without 'VIRTUALSITES'?");
}
//
void siteAlive_VirtualSiteStub(VirtualSite *vs)
{
  error("'siteAlive_VirtualSite' called without 'VIRTUALSITES'?");
}
// This one may be called;
void virtualSitesExitStub() {}

//
// First: define interface methods with defaults to "dunno" stubs;
//
VirtualSite* (*createVirtualSite)(DSite* s)
  = createVirtualSiteStub;
void (*marshalVirtualInfo)(VirtualInfo *vi, MsgBuffer *mb)
  = marshalVirtualInfoStub;
VirtualInfo* (*unmarshalVirtualInfo)(MsgBuffer *mb)
  = unmarshalVirtualInfoStub;
void (*unmarshalUselessVirtualInfo)(MsgBuffer *)
  = unmarshalUselessVirtualInfoStub;
void (*zeroRefsToVirtual)(VirtualSite *vs)
  = zeroRefsToVirtualStub;
int (*sendTo_VirtualSite)(VirtualSite *vs, MsgBuffer *mb,
                          MessageType mt, DSite *storeSite, int storeIndex)
  = sendTo_VirtualSiteStub;
int (*discardUnsentMessage_VirtualSite)(VirtualSite *vs, int msgNum)
  = discardUnsentMessage_VirtualSiteStub;
int (*getQueueStatus_VirtualSite)(VirtualSite *vs, int &noMsgs)
  = getQueueStatus_VirtualSiteStub;
SiteStatus (*siteStatus_VirtualSite)(VirtualSite* vs)
  = siteStatus_VirtualSiteStub;
MonitorReturn
(*monitorQueue_VirtualSite)(VirtualSite *vs, 
			    int size, int no_msgs, void *storePtr)
  = monitorQueue_VirtualSiteStub;
MonitorReturn (*demonitorQueue_VirtualSite)(VirtualSite* vs)
  = demonitorQueue_VirtualSiteStub;
ProbeReturn
(*installProbe_VirtualSite)(VirtualSite *vs, ProbeType pt, int frequency)
  = installProbe_VirtualSiteStub;
ProbeReturn (*deinstallProbe_VirtualSite)(VirtualSite *vs, ProbeType pt)
  = deinstallProbe_VirtualSiteStub;
ProbeReturn
(*probeStatus_VirtualSite)(VirtualSite *vs,
			   ProbeType &pt, int &frequncey, void* &storePtr)
  = probeStatus_VirtualSiteStub;
GiveUpReturn (*giveUp_VirtualSite)(VirtualSite* vs)
  = giveUp_VirtualSiteStub;
void (*discoveryPerm_VirtualSite)(VirtualSite *vs)
  = discoveryPerm_VirtualSiteStub;
void (*dumpVirtualInfo)(VirtualInfo* vi)
  = dumpVirtualInfoStub;
MsgBuffer* (*getVirtualMsgBuffer)(DSite* site)
  = getVirtualMsgBufferStub;
void (*dumpVirtualMsgBuffer)(MsgBuffer* m)
  = dumpVirtualMsgBufferStub;
void (*siteAlive_VirtualSite)(VirtualSite *vs)
  = siteAlive_VirtualSiteStub;
void (*virtualSitesExit)()
  = virtualSitesExitStub;
