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

#ifndef __PROTOCOLFAILHH
#define __PROTOCOLFAILHH

void receiveAskError(OwnerEntry*,DSite*,EntityCond);
void sendAskError(Tertiary*, EntityCond);
void receiveTellError(Tertiary*, DSite*, int, EntityCond, Bool);

void receiveAskError(OwnerEntry *,DSite*,EntityCond);
void receiveUnAskError(OwnerEntry *,DSite*,EntityCond);
void sendTellError(OwnerEntry *,DSite*,int,EntityCond,Bool);

void sendUnAskError(Tertiary*,EntityCond);

void informInstallHandler(Tertiary* t,EntityCond ec);

#endif
