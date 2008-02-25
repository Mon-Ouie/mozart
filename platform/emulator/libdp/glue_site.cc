/*
 *  Authors:
 *    Erik Klintskog (erik@sics.se)
 * 
 *  Contributors:
 *    Raphael Collet (raph@info.ucl.ac.be)
 * 
 *  Copyright:
 *    Erik Klintskog, 2002
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

#include "base.hh"
#include "pickle.hh"
#include "cac.hh"
#include "var_readonly.hh"
#include "glue_site.hh"
#include "glue_base.hh"
#include "glue_buffer.hh"
#include "glue_interface.hh"
#include "pstContainer.hh"
#include <netinet/in.h>

GlueSite* thisGSite = NULL;
GlueSite* gSiteList = NULL; 

const int RTT_INIT       =  5000;     // initial timeout
const int RTT_UPPERBOUND = 30000;     // higher bound for rtt monitor

// UTILS 


void putStr(DssWriteBuffer *buf, char *str, int len)
{
  for (int i = 0; i < len; i++) 
    buf->putByte(*(str+i));
}
void getStr(DssReadBuffer *buf, char *str, int len){
  for (int i = 0; i < len; i++)
    *(str+i) = buf->getByte(); 
}
void cleanStr(DssReadBuffer *buf, int len){
  for (int i = 0; i < len; i++)
    buf->getByte(); 
}

TaggedRef fs2atom(const DSiteState &state) {
  switch (state) {
  case DSite_OK:         return AtomOk;
  case DSite_TMP:        return AtomTempFail;
  case DSite_LOCAL_PRM:  return AtomLocalFail;
  case DSite_GLOBAL_PRM: return AtomPermFail;
  }
  Assert(0);
}



/****************************** GlueSite ******************************/

GlueSite::GlueSite(DSite* site, int ip, int port, int id) :
  dsite(site),
  ozsite(makeTaggedNULL()),
  faultstream(makeTaggedNULL()),
  next(gSiteList),
  gcmarked(false),
  disposed(false),
  a_ipAddress(ip),
  a_portNum(port),
  a_idNum(id),
  rtt_avg(0),
  rtt_mdev(0),
  rtt_timeout(RTT_INIT)
{
  gSiteList = this;
}

GlueSite::~GlueSite() {
  // close the fault stream with 'nil' (if present)
  if (faultstream) oz_bindReadOnly(tagged2Ref(faultstream), oz_nil());
}

// create the OzSite lazily
OZ_Term GlueSite::getOzSite() {
  if (!ozsite) ozsite = OZ_extension(new OzSite(this));
  return ozsite;
}

OZ_Term GlueSite::getFaultState() {
  return fs2atom(dsite->m_getFaultState());
}

OZ_Term GlueSite::getFaultStream() {
  if (!faultstream) faultstream = oz_newReadOnly(oz_rootBoard());
  return oz_cons(getFaultState(), faultstream);
}

OZ_Term GlueSite::m_getInfo() {
  static char ip[16];
  //a_ipAddress is in network byte order!
  unsigned int ip_addr=ntohl(a_ipAddress);
  sprintf(ip,"%d.%d.%d.%d",
	  (ip_addr/(256*256*256))%256,
	  (ip_addr/(256*256))%256,
	  (ip_addr/256)%256,
	  ip_addr%256);
  return OZ_recordInit(oz_atom("site"),
		       oz_cons(oz_pairAA("ip", ip),
		       oz_cons(oz_pairAI("port", a_portNum),
		       oz_cons(oz_pairAI("id", a_idNum),
		       oz_nil()))));
}

void
GlueSite::m_setConnection(DssChannel* vc) {
  // give the connection to the DSite, and monitor the connection
  dsite->m_connectionEstablished(vc);
}

void
GlueSite::m_gcRoots() {
  oz_gCollectTerm(faultstream, faultstream);
}

void
GlueSite::m_gcFinal() {
  if (gcmarked) {
    oz_gCollectTerm(ozsite, ozsite);
    gcmarked = false;
  } else {
    ozsite = makeTaggedNULL();
  }
}

void    
GlueSite::marshalCsSite( DssWriteBuffer* const buf){
  putInt(buf, a_ipAddress); 
  putInt(buf, a_portNum); 
  putInt(buf, a_idNum);
}

void    
GlueSite::updateCsSite( DssReadBuffer* const buf){
  // here we can check that the address hasn't changed since we
  // last heard of the site
  (void) getInt(buf); 
  (void) getInt(buf);
  (void) getInt(buf);
}

void    
GlueSite::disposeCsSite(){
  disposed = true;          // will be deleted in gcGlueSiteFinal()
}

void    
GlueSite::working() {
  rtt_avg = 0;                              // reset rtt parameters
  rtt_timeout = RTT_INIT;
  dsite->m_monitorRTT(rtt_timeout);
}

void
GlueSite::reportRTT(int rtt) {
  switch (dsite->m_getFaultState()) {
  case DSite_TMP:
    dsite->m_stateChange(DSite_OK);
    // fall through
  case DSite_OK:
    if (rtt > RTT_UPPERBOUND) rtt = RTT_UPPERBOUND;
    if (rtt_avg) {
      int err = rtt - rtt_avg;
      rtt_avg += err / 2;
      rtt_mdev += (abs(err) - rtt_mdev) / 4;
    } else {
      rtt_avg = rtt;
      rtt_mdev = rtt;
    }
    rtt_timeout = rtt_avg + rtt_mdev;
    dsite->m_monitorRTT(rtt_timeout);
    // fall through
  default:
    break;     // (stop monitoring when permfailed)
  }
}

void
GlueSite::reportTimeout(int timeout) {
  switch (dsite->m_getFaultState()) {
  case DSite_OK:  dsite->m_stateChange(DSite_TMP);
  case DSite_TMP: dsite->m_monitorRTT(rtt_timeout);
  default: break;     // (stop monitoring when permfailed)
  }
}

void
GlueSite::reportFaultState(DSiteState state) {
  if (faultstream) {
    OZ_Term head = fs2atom(state);
    OZ_Term tail = oz_newReadOnly(oz_rootBoard());
    oz_bindReadOnly(tagged2Ref(faultstream), oz_cons(head, tail));
    faultstream = tail;
  }
}

DssChannel *    
GlueSite::establishConnection(){
  OZ_Term command = OZ_recordInit(oz_atom("connect"),
				  oz_cons(oz_pair2(oz_int(1), getOzSite()),
					  oz_nil()));
  doPortSend(tagged2Port(g_connectPort), command, NULL);  
  // return NULL to indicate that the operation is assynchronous
  return NULL; 
}

void
GlueSite::receivedMsg(MsgContainer* msg) {
  PstInContainer* pst = static_cast<PstInContainer*>(msg->popPstIn());
  if (pst) {
    OZ_Term command =
      OZ_recordInit(oz_atom("deliver"),
		    oz_cons(oz_pair2(oz_atom("msg"), pst->a_term),
		    oz_cons(oz_pair2(oz_atom("src"), getOzSite()),
			    oz_nil())));
    doPortSend(tagged2Port(g_connectPort), command, NULL); 
  }
}



GlueSite* getGlueSites() {
  return gSiteList;
}

void gcGlueSiteRoots() {
  GlueSite* site = gSiteList;
  while (site) {
    site->m_gcRoots();
    site = site->getNext();
  }
}

void gcGlueSiteFinal() {
  GlueSite** siteptr = &gSiteList;
  GlueSite* site;
  while (site = *siteptr) {
    if (site->isDisposed()) {
      Assert(site != thisGSite);
      *siteptr = site->getNext();
      delete site;
    } else {
      site->m_gcFinal();
      siteptr = site->getNextPtr();
    }
  }
}



/****************************** OzSite ******************************/

int OzSite::getIdV(void) {
  return OZ_E_SITE;
}

OZ_Term OzSite::typeV(void) {
  return OZ_atom("site");
}

OZ_Term OzSite::printV(int depth) {
  if (depth == 0) {
    return OZ_atom("<site>");
  } else {
    static char str[16];
    //a_ipAddress is in network byte order!
    unsigned int ip_addr=ntohl(a_gSite->getIpNum());
    sprintf(str, "%d.%d.%d.%d",
	    (ip_addr/(256*256*256))%256,
	    (ip_addr/(256*256))%256,
	    (ip_addr/256)%256,
	    ip_addr%256);
    return OZ_mkTupleC("#", 7, oz_atom("<site "), oz_atom(str), oz_atom(":"),
		       oz_int(a_gSite->getPortNum()), oz_atom(":"),
		       oz_int(a_gSite->getIdNum()), oz_atom(">"));
  }
}

OZ_Term OzSite::printLongV(int depth, int offset) {
  return printV(depth); 
}

OZ_Extension *OzSite::gCollectV(void) {
  return new OzSite(*this);
}

void OzSite::gCollectRecurseV(void) {
  a_gSite->m_gcMark();
}

OZ_Extension *OzSite::sCloneV(void) {
  Assert(0); 
  return new OzSite(*this);
}

void OzSite::sCloneRecurseV(void) {}

OZ_Return OzSite::getFeatureV(OZ_Term feat, OZ_Term& value) {
  Assert(feat == oz_deref(feat));
  if (feat == AtomInfo) {
    value = a_gSite->m_getInfo();
    return PROCEED;
  }
  return OZ_FAILED;
}



// The idea goes as follows. The Oz-object calls the DSS object that will 
// create a serialized representation. The oz-object does thus not serialize
// a thing. It is completly done by the DSS. 
void OzSite::marshalSuspV(OZ_Term te, ByteBuffer *bs, GenTraverser *gt) {
  GlueWriteBuffer *gwb = static_cast<GlueWriteBuffer *>(bs);
  (a_gSite->getDSite())->m_marshalDSite(gwb);
}


static
OZ_Term unmarshalOzSite(MarshalerBuffer *bs, Builder*)
{
  Assert(0); 
  return (UnmarshalEXT_Error);
}

static
OZ_Term suspUnmarshalOzSite(ByteBuffer *mb, Builder*,
			    GTAbstractEntity* &bae)
{
  GlueReadBuffer *grb = static_cast<GlueReadBuffer *>(mb); 
  DSite *ds = glue_com_connection->a_msgnLayer->m_UnmarshalDSite(grb); 
  GlueSite *gsa = static_cast< GlueSite *>(ds->m_getCsSiteRep());
  return gsa->getOzSite();
}

static
OZ_Term unmarshalOzSiteCont(ByteBuffer *mb, Builder*,
			    GTAbstractEntity* bae)
{
  Assert(0);
  return (UnmarshalEXT_Error);
}

void OzSite_init()
{
  static Bool done = NO;
  if (done == NO) {
    done = OK;
    oz_registerExtension(OZ_E_SITE, unmarshalOzSite,
			 suspUnmarshalOzSite, unmarshalOzSiteCont);
  }
}

