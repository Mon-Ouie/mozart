/*
 *  Authors:
 *    Per Brand (perbrand@sics.se)
 *    Michael Mehl (mehl@dfki.de)
 *    Ralf Scheidhauer (Ralf.Scheidhauer@ps.uni-sb.de)
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
 *     $MOZARTURL$
 * 
 *  See the file "LICENSE" or
 *     $LICENSEURL$
 *  for information on usage and redistribution 
 *  of this file, and for a DISCLAIMER OF ALL 
 *  WARRANTIES.
 *
 */

#ifndef __PERDIOHH
#define __PERDIOHH

#ifdef INTERFACE  
#pragma interface
#endif

#include "value.hh"
#include "genhashtbl.hh"
#include "perdio_debug.hh"

#define PERDIOMINOR      "12"
#define PERDIOMAJOR      OZVERSION
#define PERDIOVERSION    PERDIOMAJOR "#" PERDIOMINOR

/* ************************************************************************ */
/*                         ORGANIZATION                                 

									    */
/* ************************************************************************ */


/* ************************************************************************ */
/*  SECTION ::  ENUMs common to protocol/marshaler                          */
/* ************************************************************************ */

enum MessageType {
  M_PORT_SEND,	
  M_REMOTE_SEND,        // OTI STRING DIF (implicit 1 credit)
  M_ASK_FOR_CREDIT,     // OTI SITE (implicit 1 credit)
  M_OWNER_CREDIT,	// OTI CREDIT
  M_OWNER_SEC_CREDIT,	// NA CREDIT

  M_BORROW_CREDIT,      // NA  CREDIT
  M_REGISTER,           // OTI SITE (implicit 1 credit)
  M_REDIRECT,           // NA  DIF
  M_ACKNOWLEDGE,        // NA (implicit 1 credit)
  M_SURRENDER,          // OTI SITE DIF (implicit 1 credit)

  M_REQUEST_FUTURE,     // OTI* SITE
  M_CELL_LOCK_GET,      // OTI* SITE
  M_CELL_LOCK_FORWARD,  // NA* INTEGER SITE
  M_CELL_LOCK_DUMP,     // OTI* SITE
  M_CELL_CONTENTS,      // NA* DIF

  M_CELL_READ,          // OTI* DIF 
  M_CELL_REMOTEREAD,    // NA* DIF
  M_CELL_READANS,
  M_CELL_CANTPUT,
  M_LOCK_TOKEN,          // NA* 

  M_LOCK_CANTPUT,
  M_CHAIN_ACK,
  M_CHAIN_QUESTION,
  M_CHAIN_ANSWER,
  M_ASK_ERROR,

  M_TELL_ERROR,
  M_GET_OBJECT,         // OTI* SITE
  M_GET_OBJECTANDCLASS, // OTI* SITE
  M_SEND_OBJECT,        //
  M_SEND_OBJECTANDCLASS,//

  M_REGISTER_VS,
  M_FILE,
  M_EXPORT,
  M_INIT_VS,
  M_UNASK_ERROR,

  M_SEND_GATE,
  M_LAST
};

extern char *mess_names[];

// the DIFs
// the protocol layer needs to know about some of these 

typedef enum {
  DIF_PROMISE,
  DIF_SMALLINT,         
  DIF_BIGINT,           
  DIF_FLOAT, 		
  DIF_ATOM,		
  DIF_NAME,		
  DIF_UNIQUENAME,	
  DIF_RECORD,		
  DIF_TUPLE,
  DIF_LIST,
  DIF_REF, 
  DIF_REF_DEBUG, 
  DIF_OWNER, 
  DIF_OWNER_SEC,
  DIF_PORT,		
  DIF_CELL,             
  DIF_LOCK,             
  DIF_VAR,
  DIF_BUILTIN,
  DIF_DICT,
  DIF_OBJECT,
  DIF_THREAD,		
  DIF_SPACE,		
  DIF_CHUNK,            // SITE INDEX NAME value
  DIF_PROC,		// SITE INDEX NAME ARITY globals code
  DIF_CLASS,            // SITE INDEX NAME obj class
  DIF_ARRAY,
  DIF_FSETVALUE,	// finite set constant
  DIF_ABSTRENTRY,	// AbstractionEntry (code instantiation)
  DIF_PRIMARY,
  DIF_SECONDARY,
  DIF_REMOTE,
  DIF_VIRTUAL,
  DIF_PERM,
  DIF_PASSIVE,
  DIF_COPYABLENAME,
  DIF_LAST
} MarshalTag;


// the names of the difs for statistics 


enum {
  MISC_STRING,
  MISC_GNAME,
  MISC_SITE,

  MISC_LAST
};

extern char* misc_names[];
extern char* dif_names[];

/* ************************************************************************ */
/*  SECTION ::  general common                                              */
/* ************************************************************************ */

typedef long Credit;  /* TODO: full credit,long credit? */
class MsgBuffer;
class PerdioVar;
class FatInt;
class SendRecvCounter;

/* ************************************************************************ */
/*  SECTION ::  provided to the engine                                     */
/* ************************************************************************ */

OZ_Return remoteSend(Tertiary *p, char *biName, TaggedRef msg);
OZ_Return portSend(Tertiary *p, TaggedRef msg);
OZ_Return cellDoExchange(Tertiary*,TaggedRef,TaggedRef);
OZ_Return cellDoAccess(Tertiary*,TaggedRef);
OZ_Return cellAtAccess(Tertiary*,TaggedRef,TaggedRef);
OZ_Return cellAtExchange(Tertiary*,TaggedRef,TaggedRef);
OZ_Return cellAssignExchange(Tertiary*,TaggedRef,TaggedRef);
void lockInstallHandler(Tertiary*,TaggedRef,TaggedRef,Thread*);
TaggedRef cellGetContentsFast(Tertiary *c);
int perdioInit();

/* ************************************************************************ */
/*  SECTION ::  provided to gc                                              */
/* ************************************************************************ */

void gcPerdioFinal();
void gcPerdioRoots();
void gcBorrowTableUnusedFrames();
void gcFrameToProxy();
void gcGName(GName*);

extern Bool checkMySite();

/* ************************************************************************ */
/*  SECTION ::  provided to marshaler                                       */
/* ************************************************************************ */

OZ_Term unmarshalTertiary(MsgBuffer*,MarshalTag); 
OZ_Term unmarshalOwner(MsgBuffer*,MarshalTag);
Site *unmarshalGNameSite(MsgBuffer*);
OZ_Term unmarshalVar(MsgBuffer*);
Site* unmarshalSite(MsgBuffer*);
void unmarshalUnsentSite(MsgBuffer*);
Credit unmarshalCreditOutline(MsgBuffer*);

Bool marshalTertiary(Tertiary *,MarshalTag, MsgBuffer*);
void marshalVar(PerdioVar *,MsgBuffer*);
void marshalSite(Site *,MsgBuffer*);
void marshalCreditOutline(Credit c,MsgBuffer *bs);

void addGName(GName*,TaggedRef);
TaggedRef findGNameOutline(GName*);
void deleteGName(GName*);

// isn't this a variety of globalization - ATTENTION
PerdioVar *var2PerdioVar(TaggedRef *, Bool);  

void SiteUnifyCannotFail(TaggedRef,TaggedRef); // ATTENTION 
void pushUnify(Thread *,TaggedRef,TaggedRef); // ATTENTION - for compponents 

extern SendRecvCounter mess_counter[];
extern Site* creditSite;

/* ************************************************************************ */
/*  SECTION ::  counter common to protocol/marshaler                        */
/* ************************************************************************ */

class SendRecvCounter {
private:
  long c[2];
public:
  SendRecvCounter() { c[0]=0; c[1]=0; }
  void send() { c[0]++; }
  long getSend() { return c[0]; }
  void recv() { c[1]++; }
  long getRecv() { return c[1]; }
};

/* ************************************************************************ */
/*  SECTION ::  gname common to protocol/marshaler                          */
/* ************************************************************************ */

const int fatIntDigits = 2;
extern FatInt *idCounter;
const unsigned int maxDigit = 0xffffffff;  

class FatInt {
public:
  unsigned int number[fatIntDigits];

  FatInt() { for(int i=0; i<fatIntDigits; i++) number[i]=0; }
  void inc()
  {
    int i=0;
    while(number[i]==maxDigit) {
      number[i]=0;
      i++;
    }
    Assert(i<fatIntDigits);
    number[i]++;
  }

  Bool same(FatInt &other)
  {
    for (int i=0; i<fatIntDigits; i++) {
      if(number[i]!=other.number[i])
	return NO;
    }
    return OK;
  }
};

enum GNameType {
  GNT_NAME,
  GNT_PROC,
  GNT_CODE,
  GNT_CHUNK,
  GNT_OBJECT,
  GNT_CLASS,
  GNT_PROMISE
};

class GName {
  TaggedRef value;
  char gcMark;

public:
  char gnameType;
  Site* site;
  FatInt id;
  TaggedRef url;

  TaggedRef getURL() { return url; }
  void markURL(TaggedRef u) { 
    if (u && !literalEq(u,NameUnit))
      url = u; 
  }

  TaggedRef getValue()       { return value; }
  void setValue(TaggedRef v) { value = v; }

  Bool same(GName *other) {
    return (site==other->site && id.same(other->id));
  }

  GName() { gcMark = 0; url=0; value = 0; }
  // GName(GName &) // this implicit constructor is used!
  GName(Site *s, GNameType gt, TaggedRef val) 
  {
    gcMark = 0;
    url = 0;
    site=s;
    idCounter->inc();
    id = *idCounter;
    gnameType = (char) gt;
    value = val;
  }
  
  GNameType getGNameType() { return (GNameType) gnameType; }

  void setGCMark()   { gcMark = 1; }
  Bool getGCMark()   { return gcMark; }
  void resetGCMark() { gcMark = 0;}

  void gcGName(){
    if (getGNameType()!=GNT_CODE && !getGCMark()) {
      setGCMark();
      gcMarkSite();
      OZ_collectHeapTerm(value,value);}}

  void gcMarkSite();
};

/* ************************************************************************ */
/*  SECTION ::  common to protocol/marshaler                          */
/* ************************************************************************ */

class MarshalInfo;
extern TaggedRef currentURL;

/* ************************************************************************ */
/*  SECTION ::  provided to gc                                              */
/* ************************************************************************ */

void gcPendThread(PendThread**);
void gcGName(GName *);

/* ************************************************************************ */
/*  SECTION ::  provided by components/gate                                 */
/* ************************************************************************ */

int loadURL(TaggedRef url, OZ_Term out, Thread *th);
int loadURL(const char *,OZ_Term,Thread *th);
void sendGate(OZ_Term t);
void initComponents();
OZ_Term getGatePort(Site*);
/* ************************************************************************ */
/*  SECTION ::  provided to engine                                      */
/* ************************************************************************ */
extern int tempTimeCtr;
#define TIME_CTR_THRESHOLD 100

/* ************************************************************************ */
/*  SECTION ::  misc                                                        */
/* ************************************************************************ */



void SendTo(Site *toS,MsgBuffer *bs,MessageType mt,Site *sS,int sI);

#define NOT_IMPLEMENTED   {warning("not implemented - perdio");Assert(0);}

/* RS: have to GC the byte stream again !!!!!!!!!*/
#define CheckNogoods(val,bs,msg,Cleanup)		\
  { OZ_Term nogoods = bs->getNoGoods();			\
    if (!oz_isNil(nogoods)) {				\
       Cleanup;						\
       return oz_raise(E_ERROR,OZ_atom("dp"),msg,3,	\
	  	       oz_atom("nogoods"),		\
		       val,				\
		       nogoods);			\
    }							\
  }


/* ************************************************************************ */
/*  SECTION ::  provided to builtins                                        */
/* ************************************************************************ */

TaggedRef listifyWatcherCond(EntityCond);
Thread *getDefaultThread();
OZ_Return bindPerdioVar(PerdioVar *pv,TaggedRef *lPtr,TaggedRef v);

/* __PERDIOHH */
#endif 





