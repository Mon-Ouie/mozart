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

#ifndef __NETWORK_HH
#define __NETWORK_HH

#ifdef INTERFACE
#pragma interface
#endif

RemoteSite* createRemoteSite(DSite*, int readCtr);
void zeroRefsToRemote(RemoteSite *);
int sendTo_RemoteSite(RemoteSite*, MsgBuffer*, MessageType, DSite*, int);
void sendAck_RemoteSite(RemoteSite*);
int discardUnsentMessage_RemoteSite(RemoteSite*,int);
int getQueueStatus_RemoteSite(RemoteSite*);  // return size in bytes
SiteStatus siteStatus_RemoteSite(RemoteSite*);
void monitorQueue_RemoteSite(RemoteSite*,int size);
void demonitorQueue_RemoteSite(RemoteSite*);
void *getMonitorQueue_RemoteSite(RemoteSite*);
// frequency is in seconds (but remote sites do not use it);
ProbeReturn installProbe_RemoteSite(RemoteSite*,ProbeType,int frequency);
ProbeReturn deinstallProbe_RemoteSite(RemoteSite*,ProbeType);
ProbeReturn probeStatus_RemoteSite(RemoteSite*,ProbeType &pt,int &frequncey,void* &storePtr);
GiveUpReturn giveUp_RemoteSite(RemoteSite*);
void discoveryPerm_RemoteSite(RemoteSite*);
void siteAlive_RemoteSite(RemoteSite*);
void dumpRemoteMsgBuffer(MsgBuffer*);

//
// Run this when starting;
void initNetwork();

//
Bool openClosedConnection(int);
int openclose(int);

//
// kost@ : i've put the missing declarations here;
int getNORM_RemoteSite(RemoteSite*);
int getNOSM_RemoteSite(RemoteSite*);

#endif // __NETWORK_HH
