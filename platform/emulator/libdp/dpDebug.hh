/*
 *  Authors:
 *    Per Brand (perbrand@sics.se)
 * 
 *  Contributors:
 *    Michael Mehl (mehl@dfki.de)
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

#ifndef __DPDEBUG_HH
#define __DPDEBUG_HH

#include "base.hh"
#include "dpBase.hh"

#ifdef INTERFACE  
#pragma interface
#endif

enum DEBUGType {
  MSG_RECEIVED,	// 0			// protocol -layer
  MSG_SENT,	
  MSG_PREP,
  TABLE,					// borrow/owner table events without print
  TABLE2,					// borrow/owner table events without print
  GC,		// 5			
  CREDIT,	
  LOOKUP,
  GLOBALIZING,
  LOCALIZING,   
  PD_VAR,	// 10				variable protocol
  CELL,						// cell protocol
  LOCK,                                 	// lock protocol
  SITE_OP,     
  THREAD_D,

  MARSHAL,	 // 15			// marshaler 
  MARSHAL_CT,    
  UNMARSHAL,
  UNMARSHAL_CT,
  MARSHAL_BE,					// marshal begin/end
  REF_COUNTER,	// 20			

  TCP,					// communication layer events
  WEIRD,        
  TCP_INTERFACE,
  TCPCACHE,
  TCPQUEUE,     // 25
  SITE,		
  REMOTE,       
  MESSAGE,
  OS,
  BUFFER,       // 30
  READ,         
  WRITE,        
  CONTENTS,				
  HASH,					// site/gname hash table
  HASH2,        // 35

  USER,					// misc
  SPECIAL,      
  ERROR_DET,    
  WRT_QUEUE,
  ACK_QUEUE,    // 40
  CELL_MGR,
  PROBES,
  NET_HANDLER,
  LOCK_MGR,    
  CHAIN,        // 45
  PORT,
  TCP_HERROR,
  TCP_CONNECTIONH,
  LAST

};

extern TaggedRef BI_startTmp;

#include <stdarg.h> 

extern char *debugTypeStr[];

class DebugVector{
public:
  Bool ar[LAST];
  DebugVector(){
    int i;
    for(i=0;i<LAST;i++) {ar[i]=FALSE;}}
  Bool on(int t){return ar[t];}
  void set(int t){ar[t]=TRUE;}
  void unset(int t){ar[t]=FALSE;}
};

extern DebugVector *DV;
extern void resize_hash();

Bool isPerdioInitialized();

inline
void _PD(int i,char *format,...)
{
  if (isPerdioInitialized() && DV->on(i)) {
    printf("%s: ",debugTypeStr[i]);
    va_list ap;
    va_start(ap,format);

    vprintf(format,ap);
    printf("\n");
  }
}

void networkTimer(int);

#ifdef DEBUG_PERDIO 

#define PD(Args) _PD Args

#else

#define PD(Args)

#endif 

#endif




