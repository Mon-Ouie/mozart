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
 *     $MOZARTURL$
 * 
 *  See the file "LICENSE" or
 *     $LICENSEURL$
 *  for information on usage and redistribution 
 *  of this file, and for a DISCLAIMER OF ALL 
 *  WARRANTIES.
 *
 */

// abstract interface to the ip network


#include "wsock.hh"
#ifndef WINDOWS
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#endif
#include <sys/utsname.h>
#include <errno.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "os.hh"
#include "perdio.hh"
#include "marshaler.hh"
#include "comm.hh"
#include "msgbuffer.hh"
#include "perdio_debug.hh"





/* ************************************************************************ */
/*  SECTION ::  Enums & Defines                                            */
/* ************************************************************************ */

#define NETWORK_ERROR(Args) {error Args;}
#define IMPLEMENT(Args) {error Args;}


/* free list sizes */

#define SITE_CUTOFF    100
#define MESSAGE_CUTOFF 10
#define READ_CONNECTION_CUTOFF  30
#define WRITE_CONNECTION_CUTOFF  30
#define NO_MSG_NUM -1

#define TCP_MSG_SIGN_BITS 3
#define INT_IN_BYTES_LEN 7
#define CONNECTION REMOTE
#define REMOTESITE SITE

#define BYTEBUFFER_SIZE 2048
#define BYTEBUFFER_CUTOFF 100
#define NetMsgBuffer_CUTOFF 50

#define OZReadPortNumber  9000
#define OZStartUpTries    490
#ifdef WINDOWS
/* connect under windows takes very long */
#define OZConnectTries    10
#else
#define OZConnectTries    200
#endif
#define OZWritePortNumber 9500



static Bool net_timer_set;
int tempTimeCtr=0;

#include <netdb.h>



static const int netIntSize=4;
static const int msgNrSize =4;
static const int ansNrSize =4;
static const int tcpSimpleHeaderSize = 1+netIntSize;
static const int tcpHeaderSize=1+netIntSize+msgNrSize + ansNrSize;
static const int tcpConfirmSize=tcpHeaderSize+netIntSize;

enum tcpMessageType {
  TCP_CLOSE_REQUEST_FROM_WRITER = 0,
  TCP_CLOSE_REQUEST_FROM_READER,
  TCP_CLOSE_ACK_FROM_READER,
  TCP_MSG_ACK_FROM_READER,
  TCP_RESEND_MESSAGES,
  TCP_NO_CONNECTION, // from READER
  TCP_CONNECTION,    // from READER
  TCP_PACKET,        // from WRITER
  TCP_MYSITE,        // from WRITER
  TCP_MYSITE_ACK, 
  TCP_PING_REQUEST,
  TCP_NONE 

};

enum ConnectionFlags{
  CLOSING=1,  // has sent CLOSE request
  OPENING=2,  // has made connection 
  WANTS_TO_OPEN=4,      // NOT USED! 
  WANTS_TO_PROBE=8,     // NOT USED!
  WANTS_TO_CLOSE=4,     // is blocked on trying to send CLOSE
  CURRENT=8,            // incompleteRead or Write
  WRITE_QUEUE=16,       // has something in write queue
  CAN_CLOSE = 32,       // Zero refs to site, but still something to send
  ACK_MSG_INCOMMING=64, // incomplete on back-channel
  WRITE_CON = 128,      // connection type is WRITE
  MY_DWN= 256,          // Closed by me 
  TMP_PROBE = 512,      // Probe for Perm 
  PRM_PROBE = 1024,     // Probe for Tmp
  OK_PROBE =  2048,     // Probe for Ok
  TMP_DWN   = 4096      // Closed due to Tmp problem   
  
};

enum ByteStreamType{
  BS_None,
  BS_Marshal,
  BS_Write,
  BS_Read,
  BS_Unmarshal};


#define MAXTCPCACHE 25

enum ipReturn{
  IP_OK              =   0,
  IP_BLOCK           =  -1,
  IP_NO_MORE_TRIES   =  -2,
  IP_TIMER           =  -3,
  IP_CLOSE           =  -4,
  IP_TIMER_EXCEPTION =  -5,
  IP_NET_CRASH       =  -6,
  IP_TEMP_BLOCK      =  -7,
  IP_PERM_BLOCK      =  -8,
  IP_GARBAGE         =  -9,
  IP_EOF             = -10
};

#define LOST -1

enum closeInitiator{
  MY_INITIATIVE,
  HIS_INITIATIVE,
  TMP_INITIATIVE
};

#ifdef XXGNUWIN32
#define ntohl __ntohl
#define ntohs __ntohs
#endif

#define WKUPMYC 0
#define WKUPTMP 1
#define WKUPPRB 3

#define WKUPMYC_TIME 2000
#define WKUPTMP_TIME 20000
#define WKUPPRB_TIME 180000

/* ************************************************************************ */
/*  SECTION ::  Forward declarations                                        */
/* ************************************************************************ */

class Connection;
class ReadConnection;
class WriteConnection;
class BufferManager;
class Message;
class IOQueue;
class WriteConnectionManager;
class ReadConnectionManager;
class NetMsgBufferManager;
class RemoteSiteManager;
class ByteBufferManager;
class MessageManager;
class TcpCache;
class TcpOpenMsgBuffer;
class NetMsgBuffer;


ipReturn tcpSend(int,Message *,Bool);
int intifyUnique(BYTE *);
TcpOpenMsgBuffer *tcpOpenMsgBuffer;
Bool tcpAckReader(ReadConnection*, int);
static ipReturn tcpOpen(RemoteSite *,WriteConnection *) ;
int tcpConnectionHandler(int,void *);
void wakeUpTmp(int,int);

static int tcpWriteHandler(int,void*);
static int tcpCloseHandler(int,void*);
int tcpPreReadHandler(int,void*);
static int tcpReadHandler(int,void*);
inline ipReturn writeI(int,BYTE*);
inline ipReturn readI(int,BYTE *);

int findProbeCons(Connection*);

/* ************************************************************************ */
/*  SECTION ::  Global Variables                                            */
/* ************************************************************************ */

WriteConnectionManager *writeConnectionManager;
ReadConnectionManager *readConnectionManager;
NetMsgBufferManager *netMsgBufferManager;
RemoteSiteManager *remoteSiteManager;
ByteBufferManager *byteBufferManager;
MessageManager *messageManager;
TcpCache *tcpCache;




class Message{
  friend class IOQueue;
  friend class MessageManager;
  friend class RemoteSite;
  friend class WriteConnection;
protected:
  Message *next;
  tcpMessageType type; // ?? 
  NetMsgBuffer *bs;
  int remainder;
  int msgNum;
  Site *site;// not needed ??
  MessageType msgType;
  int storeIndx;
  
public:
  Message(){}
  void init(NetMsgBuffer *b, int n, 
	    Site *s, MessageType msg, int stI){
    next=NULL;
    bs=b;
    remainder=0;
    type=TCP_NONE;
    msgNum = n;
    site = s;
    msgType = msg;
    storeIndx = stI;}
  NetMsgBuffer* getMsgBuffer(){
    return bs;};
  void setType(tcpMessageType t){
    PD((MESSAGE,"set type %d",t));
    type=t;}
  tcpMessageType getType(){
    return type;}
  int getRemainder(){
    return remainder;} 
  void setRemainder(int i){
    PD((MESSAGE,"set remainder %d",i));
    remainder=i;}
  Message* getNext(){
    return next;}
  int getMsgNum(){
    return msgNum;}
  void setMsgNum(int n){
    msgNum = n;}
  void resend();
  
};


class IOQueue {
private:
  Message *first;
  Message *last;
  void checkQueue();
public:
  IOQueue():first(NULL),last(NULL){}
  Message* getFirst();
  Message* removeFirst();
  Bool     queueEmpty(){
    Assert(((first==NULL) && (last==NULL))|| 
	   ((first!=NULL) && (last!=NULL)));
    return first == NULL;}
  Message* find(int msg){
    Assert(((first==NULL) && (last==NULL))||((first!=NULL) && (last!=NULL)));
    Message *b=first,*d=NULL;
    while(b!=NULL && b->msgNum != msg)
      { d = b; b = b->next;}
    if(b == NULL) return NULL;
    if(b == last)
      last = d;
    if(b == first)
      first = b->next;
    else
      d->next = b->next;
    return b;}
  
  void enqueue(Message *m);
  void dequeue(Message *m);
  void addfirst(Message *m);
};      

/* ************************************************************************ */
/*  SECTION ::  Network MsgBuffer and friends                                       */
/* ************************************************************************ */

class ByteBuffer{
  friend class ByteBufferManager;
  friend class NetMsgBuffer;
  friend class NetMsgBufferManager;
  
protected:
  BYTE buf[BYTEBUFFER_SIZE];
  ByteBuffer *next;
  
public: 
  ByteBuffer(){}
  
  BYTE *head(){return buf;}
  BYTE *tail(){return buf+BYTEBUFFER_SIZE-1;}
  void init() {next=NULL;}
};

class ByteBufferManager: public FreeListManager {
public:
  ByteBufferManager():FreeListManager(BYTEBUFFER_CUTOFF){wc = 0;}
  int wc;

  ByteBuffer*newByteBuffer(){
    FreeListEntry *f=getOne();
    ByteBuffer *bb;
    if(f==NULL) {bb=new ByteBuffer();}
    else{GenCast(f,FreeListEntry*,bb,ByteBuffer*);}
    bb->init();
    PD((BUFFER,"New ByteBuffer %d %d nr:%d",bb, bb->next, ++wc));
    return bb;}
  
  void deleteByteBuffer(ByteBuffer* bb){
    FreeListEntry *f;
     PD((BUFFER,"DeAllocating ByteBuffer nr:%d",--wc));
    GenCast(bb,ByteBuffer*,f,FreeListEntry*);
    if(putOne(f)) return;
    delete bb;
    return;}
}; 

  



class NetMsgBuffer:public MsgBuffer{
  friend class NetMsgBufferManager;
protected:
  ByteBuffer *first; 
  ByteBuffer *start;
  ByteBuffer *stop;
  ByteBuffer *last;
  BYTE *pos; 
  BYTE *curpos;
  BYTE *endpos; 
  int totlen;  /* include header */
  int type;
  Site *site;
  
  RemoteSite *remotesite;
  int availableSpace();
  
  ByteBuffer *beforeLast(){
    Assert(first!=last);
    ByteBuffer *bb=first;
    while(bb->next!=last){bb=bb->next;}
    return bb;}

  Bool within(BYTE*,ByteBuffer*);
  
public:
  NetMsgBuffer() {}
  
  //ATTENTION Hack Remove
  void startfixerik1(){
    NetMsgBuffer();}

  void resend();
  void reset();
  void unmarshalReset();

  void init(){
    MsgBuffer::init();
    type=BS_None;first=NULL;start=NULL;last=NULL;
    site=NULL;remotesite=NULL;stop=NULL;}

  void init(Site *s) {
    init();
    site=s;
  }

  void setSite(Site *s){ site = s;}
  Site* getSite(){ return site;}

  Bool isPersistentBuffer() { return NO; }

  RemoteSite* getRemoteSite(){
    return remotesite;}

  char *siteStringrep();
  int getTotLen();
  
  void removeFirst(){
    Assert(first!=last);
    ByteBuffer *bb=first;
    first=bb->next;
    byteBufferManager->deleteByteBuffer(bb);
  }

  void   constructMsg(RemoteSite*,tcpMessageType);

  void removeSingle(){
    Assert(first==last);    
    Assert(first!=NULL);
    byteBufferManager->deleteByteBuffer(first);
    last=first=NULL;}
      
  ByteBuffer* getAnother();
  void getSingle();

  void marshalBegin(){
    PD((MARSHAL_BE,"marshal begin"));
    Assert(type==BS_None);
    Assert(first==NULL);
    Assert(last==NULL);
    first=getAnother();
    start=first;
    last=first;
    stop=first;
    totlen= 0;
    type=BS_Marshal;
    posMB=first->head()+tcpHeaderSize;
    endMB=first->tail();
    pos=NULL;}

  void putNext(BYTE b){
    Assert(type==BS_Marshal);
    Assert(posMB==endMB+1);
    PD((BUFFER,"bytebuffer alloc Marshal: %d",no_bufs()));
    ByteBuffer *bb=getAnother();
    last->next=bb;
    last=bb;
    stop=bb;
    totlen += BYTEBUFFER_SIZE;
    posMB=bb->head();
    endMB=bb->tail();
    *posMB++=b;
    return;}

  void marshalEnd(){
    Assert(type==BS_Marshal);
    endpos=posMB;
    pos=first->head();
    if(endpos==NULL) {totlen +=BYTEBUFFER_SIZE;}
    else {totlen +=endpos-last->head();}
    type=BS_None;}
  
  void beginWrite(RemoteSite*); /* putting end packet header */
  void PiggyBack(Message*);
    
  void sentFirst();

  int getWriteLen();
  
  BYTE* getWritePos();

  void incPosAfterWrite(int i);

  int calcTotLen();

  void writeCheck();
  
  /* read  endpos=first free slotpos (if NULL then last is full)
           pos= first BYTE of unhandled message 

     after read endpos updated */
    
  BYTE* initForRead(int&);
  BYTE* beginRead(int&);
  void afterRead(int);

  /* interface pos=first BYTE curpos=last BYTE (may be in different buffers)
     between curpos and endpos BYTES in next message */

  void beforeInterpret(int);
  void afterInterpret();
  BYTE getInRead();
  int interLen(); /* used for debugging only */

  /* unmarshall pos=first unread BYTE curpos last unread BYTE */

  void unmarshalBegin(){
    type=BS_Unmarshal;
    Assert(within(pos,first));
    if(first==last) {
      Assert(within(curpos,first));
      Assert(curpos>=pos);}
    else {Assert(within(curpos,last));}
    posMB=pos;
    if(within(curpos,first)) {
      endMB=curpos;}
    else{
      endMB=first->tail();}
    pos=NULL;}

  BYTE getNext(){
    Assert(type==BS_Unmarshal);
    Assert(posMB!=NULL);
    if(posMB==curpos){
      posMB=NULL;
      Assert(first==last);
      return *curpos;}
    if(posMB==first->tail()){
      BYTE ch;
      Assert(first!=last);
      ch=*posMB;
      removeFirst(); 
      posMB=first->head();
      if(within(curpos,first)) {
	endMB=curpos;}
      else{
	endMB=first->tail();}
      return ch;}
    Assert(0);
    return 0;}

  void unmarshalEnd(){
    Assert(type==BS_Unmarshal);    
    type=BS_None;
    Assert(posMB==NULL);}

  int no_bufs();

  void dumpByteBuffers(){
    if(type == BS_Write){
      Assert(start != NULL);
      while(start!=stop) {
	ByteBuffer *bb=start;
	start=bb->next;
	byteBufferManager->deleteByteBuffer(bb);
      }
      byteBufferManager->deleteByteBuffer(start);
      start=stop=NULL;}
    else{
      Assert(first != NULL);
      while(first!=last) {
	ByteBuffer *bb=first;
	first=bb->next;
	byteBufferManager->deleteByteBuffer(bb);
      }
      byteBufferManager->deleteByteBuffer(first);
      first=last=NULL;}}
  
};


void Message::resend(){
  next = NULL;
  bs->resend();}
char* NetMsgBuffer::siteStringrep(){
  return site->stringrep();}
int NetMsgBuffer::getTotLen() {return totlen;}

BYTE* NetMsgBuffer::getWritePos(){
  Assert(type==BS_Write);return pos;}

int NetMsgBuffer::availableSpace(){
  Assert(last!=NULL);
  if(endpos==NULL) return 0;
  Assert(within(endpos,last));
  return last->tail()-endpos+1;}

Bool NetMsgBuffer::within(BYTE *p,ByteBuffer *bb){
  if(p<bb->head()) return FALSE;
  if(p>bb->tail()) return FALSE;  
  return TRUE;}

int NetMsgBuffer::no_bufs(){
  int i=0;
  ByteBuffer *bb=first;
  while(bb!=NULL){
    i++;
    bb=bb->next;}
  return i;}

void NetMsgBuffer::writeCheck(){
  Assert(type==BS_Write);
  Assert(first==NULL);
  Assert(last==NULL); 
  return;}

void NetMsgBuffer::sentFirst(){
  Assert(type==BS_Write);
  PD((BUFFER,"bytebuffer sent: %d",no_bufs()));
  if(first==last){
    first=last=NULL;
    return;}
  first=first->next;
  pos=first->head();}


int NetMsgBuffer::calcTotLen(){
  Assert(type==BS_Write);
  if(first==last){
    if(endpos!=NULL){return endpos-pos;}
    return first->tail()-pos+1;}
  else{
    int i=first->tail()-pos+1;
    ByteBuffer *bb=first->next;
    while(bb->next!=NULL){
      i+=BYTEBUFFER_SIZE;
      bb=bb->next;}
    Assert(bb==last);
    if(endpos==NULL){return i+BYTEBUFFER_SIZE;}
    return i+endpos-last->head();}}


void NetMsgBuffer::incPosAfterWrite(int i){
    Assert(type==BS_Write);
    Assert(within(pos,first));
    Assert(pos+i<=first->tail());
    pos +=i;}


void NetMsgBuffer::getSingle(){
  Assert(first==NULL);
  Assert(last==NULL);
  ByteBuffer *bb=getAnother();
  
  if(type == BS_Write){
    // EK 
    // Must this be so?
    start = bb;stop = bb;}
  first=bb;
  last=bb;}

int NetMsgBuffer::getWriteLen(){
  Assert(type==BS_Write);
  Assert(first!=NULL);
  if(first==last){
    if(endpos!=NULL) {return endpos-pos;}
    return first->tail()-pos+1;}
  return first->tail()-pos+1;}


class NetMsgBufferManager:public FreeListManager{
public:
  NetMsgBufferManager():FreeListManager(NetMsgBuffer_CUTOFF){}
  
  int wc; // for debug purposes
  
  NetMsgBuffer*newNetMsgBuffer(Site *s){
    FreeListEntry *f=getOne();
    NetMsgBuffer *bb;
    if(f==NULL) {bb=new NetMsgBuffer();}
    else{GenCast(f,FreeListEntry*,bb,NetMsgBuffer*);}
    bb->startfixerik1();
    bb->init(s);
    PD((BUFFER,"New netMsgBuffer b:%r nr:%d",bb,++wc));
    return bb;}
  
  void deleteNetMsgBuffer(NetMsgBuffer* b){
    Assert(b->start == b->stop);
    Assert(b->start == NULL);
    PD((BUFFER,"Deallocating netMsgBuffer b:%d nr:%d",b,--wc));
    FreeListEntry *f;
    GenCast(b,NetMsgBuffer*,f,FreeListEntry*);
    if(putOne(f)) return;
    delete b;
    return;}
  
  void dumpNetMsgBuffer(NetMsgBuffer* nb) {
    if(nb->start!=NULL && nb->first==NULL)
      nb->reset();
    if(nb->first!=NULL)
      nb->dumpByteBuffers();
    deleteNetMsgBuffer(nb);}

  NetMsgBuffer *getNetMsgBuffer(Site *s) {
    return newNetMsgBuffer(s);}
 
  void freeNetMsgBuffer(NetMsgBuffer* bs){
    if(bs->first!=NULL){
      Assert(bs->last==bs->first);
      bs->removeSingle();}
    deleteNetMsgBuffer(bs);}
};

ByteBuffer *NetMsgBuffer::getAnother(){
  return(byteBufferManager->newByteBuffer());}

/* *********************************************************************/
/*   SECTION :: RemoteSite                                          */
/* *********************************************************************/


class RemoteSite{
  friend class RemoteSiteManager;
  
  int recMsgCtr; 
  int tmpMsgNum;             // optimize away
  SiteStatus status;         // TEMP,PERM,OK
  
  // This is all monitor stuff. 
  // should be encapsulated in an  object with manager
  int totalMsgSize;      // size of queued msgs
  int MsgMonitorSize;    // limits given by protocol-layer
  int treashFullCtr;
  
  WriteConnection *writeConnection; 
  ReadConnection *readConnection;
  Message *sentMsg;      // non-acknowledge msgs
  IOQueue writeQueue; //Unsent Messages,
  int tSn;
public:
  Site* site;

protected:
  void init(Site*, int);
public:
  RemoteSite(): writeConnection(NULL),readConnection(NULL){} 
  
  void setAckStartNr(int nr);
  int resendAckQueue(Message *m);
  
  int readRecMsgCtr();
  void receivedNewAck(int);

  int sendTo(NetMsgBuffer*,MessageType,Site*, int);
  void zeroReferences();
  void nonZeroReferences();
  
  void writeConnectionRemoved();
  void readConnectionRemoved();

  void addWriteQueue(Message* m,int msg){
    m->setMsgNum(msg);
    addWriteQueue(m);}
  
  void addWriteQueue(Message*);
  void addFirstWriteQueue(Message* m){
    writeQueue.addfirst(m);}
    
  Message* getWriteQueue();

  SiteStatus siteStatus(){return status;}
  void setSiteStatus(SiteStatus s){status = s;} 
  
  void siteTmpDwn(closeInitiator);
  void sitePrmDwn();

  int getTmpMsgNum(){ return tmpMsgNum++;}

  Bool receivedNewMsg(int);
  int getRecMsgCtr();
  void clearRecMsgCtr(){recMsgCtr = 0;}
  
  void storeSentMessage(Message* bs);

  time_t getTimeStamp(){return site->timestamp;}
  port_t getPort(){return site->port;}
  ip_address getAddress(){return site->address;}
  int getTmpSessionNr(){return tSn;}
  void incTmpSessionNr(){tSn = ++tSn % 199802;}

  WriteConnection *getWriteConnection(){return writeConnection;}
  ReadConnection *getReadConnection(){return readConnection;}
  
  void setWriteConnection(WriteConnection *r);
  void setReadConnection(ReadConnection *r);
  
  int discardUnsentMessage(int msgNum);

  void queueMessage(int size){
    Assert(totalMsgSize >=0);
    totalMsgSize += size;
    PD((WRT_QUEUE,"New entry s:%d tS:%d tr:%d",size,totalMsgSize,MsgMonitorSize));}
  
  void deQueueMessage(int size){
    PD((WRT_QUEUE,"Rm entry s:%d tS:%d tr:%d",size,totalMsgSize,MsgMonitorSize));
    totalMsgSize -= size;
    if(totalMsgSize < 0) totalMsgSize = 0;
    Assert( totalMsgSize >= 0);
    if (totalMsgSize <= MsgMonitorSize){
      treashFullCtr = (1+treashFullCtr) % 100000;}}

  void monitorQueueMsgs(int SizeMess){
    Assert(MsgMonitorSize == -1);
    MsgMonitorSize=SizeMess;}
  void deMonitorQueue(){MsgMonitorSize = -1;}
  int  getQueueSize(){return totalMsgSize;}
  int  getTreashCtr(){return treashFullCtr;}
  Site* getSite(){return site;}
  
  ProbeReturn installProbe(ProbeType);
  ProbeReturn deInstallProbe(ProbeType);

  void sendAck();
};


/***********************************************************************/
/* SECTION :: Read and Write - Connection                                */
/***********************************************************************/

class Connection {
public:
  int fd;               // LOST means no connection
  int flags;
  RemoteSite *remoteSite;
  Message* current;
  Connection *next, *prev;
  
  void setFlag(int f){flags |= f;}
  void clearFlag(int f){flags &= ~f;}
  Bool testFlag(int f){return f & flags;}
  
  Bool isWriteCon(){return testFlag(WRITE_CON);}

  void setIncomplete(){
    Assert(!testFlag(CURRENT));
    PD((TCP_INTERFACE,"setIncomplete site:%s",remoteSite->site->stringrep()));   
    setFlag(CURRENT);}
  Bool isIncomplete(){
    return testFlag(CURRENT);}
  void clearIncomplete(){
    Assert(testFlag(CURRENT));
    PD((TCP_INTERFACE,"clearIncomplete site:%s",remoteSite->site->stringrep()));    
    clearFlag(CURRENT);}
  
  void setSite(RemoteSite *s){remoteSite=s;}
  
  void setClosing();
  Bool isClosing(){
    return testFlag(CLOSING);}
  void clearClosing(){
    PD((TCP_INTERFACE,"clearClosing site:%s",remoteSite->site->stringrep()));
    Assert(testFlag(CLOSING));
    clearFlag(CLOSING);}
  
  Bool isRemovable(){return flags==0;}

  RemoteSite *getRemoteSite(){return remoteSite;}
  
  void connectionLost(){remoteSite->site->discoveryPerm();}
  void connectionBlocked(){remoteSite->siteTmpDwn(TMP_INITIATIVE);}
  
  void clearFD(){fd=LOST;}
  void setFD(int f){fd=f;}
  int getFD(){ return fd;}
  
  void addCurQueue(Message *m){
    PD((TCPQUEUE,"add CurQueue m:%x r:%x",m,this));
    setIncomplete();
    current = m;}
  Message* getCurQueue(){
    Message *m = current;
    current = NULL;
    clearIncomplete();
    return m;}
  
  void disconnect(){
    Assert(next!=NULL);
    Assert(prev!=NULL);
    next->prev=prev;
    prev->next=next;}

  Bool isOpening(){
    return testFlag(OPENING);}
  void setOpening(){
     Assert(!testFlag(OPENING));
     setFlag(OPENING);}
  void clearOpening(){
    Assert(testFlag(OPENING));
    clearFlag(OPENING);}


};

class ReadConnection:public Connection{
  friend class ReadConnectionManager;
  friend class TcpCache;

protected:
  int  maxSizeAck;
  int  recSizeAck; 
public:
   
  void fastfixerik2()
  {Connection();}
  
  void receivedNewSize(int size){
    recSizeAck +=  size;
    PD((ACK_QUEUE,"SizeReceived s: %d l: %d",recSizeAck	,maxSizeAck));
    if(recSizeAck >= maxSizeAck)  sendAck();}
  
  void sendAck(){
    if(tcpAckReader(this,remoteSite->getRecMsgCtr())){
      PD((ACK_QUEUE,"Ack, limit reached...."));
      recSizeAck = 0;}}
  
  void messageSent(){recSizeAck=0;}
  
  void informSiteRemove(){
    remoteSite->readConnectionRemoved();}
  
  Bool resend();
  
  void setMaxSizeAck(int max){
    PD((SITE,"Max size: %d",max));
    maxSizeAck = max;
    recSizeAck = 0;}
  
  Bool informSiteAck(int, int);

  void init(RemoteSite *s){
    next = prev = NULL;
    flags=0;
    current = NULL;
    maxSizeAck = 0;
    recSizeAck = 0;
    remoteSite=s;
    current=NULL;}
  
  Bool canbeClosed(){
    if(isClosing()) return FALSE;
    if(isOpening()) return FALSE;
    return TRUE;}
  
  Bool goodCloseCand(){
    return canbeClosed() && !isIncomplete();}
  
  void niceClose(){remoteSite->clearRecMsgCtr();}
  void close();
  void prmDwn();
  void closeConnection();
};

class WriteConnection:public Connection{
  friend class WriteConnectionManager;
  friend class TcpCache;
protected:
  BYTE intBuffer[INT_IN_BYTES_LEN];
  BYTE *bytePtr;
  int sentMsgCtr;
  Message *sentMsg;      // non-acknowledge msgs
  int probeCtrPrm;
  int probeCtrTmp;
public:
  void fastfixerik3(){
    Connection();}
  
  void close(closeInitiator);
  void closeConnection();

  void informSiteRemove(){
    remoteSite->writeConnectionRemoved();}
  
  ipReturn open();
  
  void tmpDwn();
  Bool shouldSendFromUser();
    
  void informSiteResendAckQueue();

  void storeSentMessage(Message* m);
  
  int getMsgCtr(){return sentMsgCtr;}
  int incMsgCtr(){return ++sentMsgCtr;}
  
  Bool checkAckQueue();

  void ackReceived(int);


  Bool goodCloseCand(){
    return (canbeClosed() && (!isWritePending()));}

  void opened(){
    Assert(isOpening());
    PD((TCP_INTERFACE,"opened site:%s",remoteSite->site->stringrep()));
    clearOpening();
    remoteSite->setSiteStatus(SITE_OK);
    remoteSite->incTmpSessionNr();
    if(isProbingOK()){
      remoteSite->site->probeFault(PROBE_OK);
      clearProbingOK();}}
  
  Bool addByteToInt(BYTE msg){
    if(!testFlag(ACK_MSG_INCOMMING))
      return FALSE;
    *bytePtr++ = msg;   
    PD((TCP,"Reading byte: %d diff: %d tot: %d",
	msg, bytePtr - intBuffer, INT_IN_BYTES_LEN));
    if((bytePtr - intBuffer) == INT_IN_BYTES_LEN){
      clearFlag(ACK_MSG_INCOMMING);
      ackReceived(intifyUnique(intBuffer));
      return FALSE;}
    return TRUE; }
  
  void prmDwn();
  
  Bool isInWriteQueue(){
    Assert(!isIncomplete());
    return testFlag(WRITE_QUEUE);}
  void setInWriteQueue(){
    PD((TCP_INTERFACE,"setInWriteQueue s:%s",remoteSite->site->stringrep()));
    setFlag(WRITE_QUEUE);}
  void clearInWriteQueue(){
    PD((TCP_INTERFACE,"Removed from writequeue  s:%s",	remoteSite->site->stringrep()));
    clearFlag(WRITE_QUEUE);}
  
  void setWantsToProbe(){
    Assert(!testFlag(WANTS_TO_PROBE));    
    PD((TCP_INTERFACE,"setWantsToPROBE site:%s",remoteSite->site->stringrep()));
    setFlag(WANTS_TO_PROBE);}
  Bool isWantsToProbe(){
    return testFlag(WANTS_TO_PROBE);}
  void clearWantsToProbe(){ 
    Assert(testFlag(WANTS_TO_PROBE));
    PD((TCP_INTERFACE,"clearWantsToProbe site:%s",remoteSite->site->stringrep()));
    clearFlag(WANTS_TO_PROBE);}
  
  Bool isProbingPrm(){
    return testFlag(TMP_PROBE | PRM_PROBE);}
  Bool isProbingTMP(){
    return testFlag(TMP_PROBE);}
  void setProbingPrm(){
    setFlag(PRM_PROBE);}
  void setProbingTmp(){
    setFlag(TMP_PROBE|PRM_PROBE);}
  void clearProbingPrm(){
    clearFlag(PRM_PROBE);}
  void clearProbingTmp(){
    clearFlag(TMP_PROBE);}
  Bool isProbingOK(){
    return testFlag(OK_PROBE);}
  void setProbingOK(){
    setFlag(OK_PROBE);}
  void clearProbingOK(){
    clearFlag(OK_PROBE);}

  void setWantsToClose();
  void clearWantsToClose();
  Bool isWantsToClose(){
    return testFlag(WANTS_TO_CLOSE);}
  
  void incommingAck(){
    bytePtr = intBuffer;
    setFlag(ACK_MSG_INCOMMING);}
  Bool isWritePending(){
    return testFlag(WRITE_QUEUE|CURRENT);}

  // Zero references to Site. When no msgs are waiting
  // close the writeConnedtion.
  void setCanClose(){
    // EK assertion?
    PD((TCP_INTERFACE,"You can now close site:%s",remoteSite->site->stringrep()));
    setFlag(CAN_CLOSE);}
  Bool isCanClose(){
    return testFlag(CAN_CLOSE);}
  void clearCanClose(){
    Assert(isCanClose());
    clearFlag(CAN_CLOSE);}

  int discardUnsentMessage(int msgNum);
  Bool canbeClosed(){
    if(isClosing()) return FALSE;
    if(isOpening()) return FALSE;
    if(isWantsToProbe()) return FALSE;
    // Ek is it writing? insert that here 
    return TRUE;}
  
  Bool isTmpDwn(){
    return testFlag(TMP_DWN);}
  void setTmpDwn(){
    Assert(!isTmpDwn());
    setFlag(TMP_DWN);}
  void clearTmpDwn(){
    Assert(isTmpDwn());
    clearFlag(TMP_DWN);}
  Bool isAcked(){
    return sentMsg==NULL;}
  
  Bool isMyInitiative(){
    return testFlag(MY_DWN);}
  void setMyInitiative(){
    Assert(!isMyInitiative());
    setFlag(MY_DWN);}
  void clearMyInitiative(){
    Assert(isMyInitiative());
    clearFlag(MY_DWN);}

  void init(RemoteSite *s){
    next = prev = NULL;
    current = NULL;
    sentMsg = NULL;
    sentMsgCtr =0;
    flags=WRITE_CON;
    remoteSite=s;
    bytePtr = intBuffer + INT_IN_BYTES_LEN;
    probeCtrTmp = 0;
    probeCtrPrm = 0;}

  void installProbe(ProbeType pt){
    PD((PROBES,"Probe Installed to Tmp: %d Prm: %d  site: %s",
	probeCtrTmp, probeCtrPrm, 
	remoteSite->site->stringrep()));
    Assert(probeCtrPrm >=0 && probeCtrTmp >= 0);
    if(pt == PROBE_TYPE_PERM)
      {if(probeCtrPrm==0)
	setProbingPrm();
      probeCtrPrm++;}
    else
      {if(probeCtrTmp==0)
	setProbingTmp();
      probeCtrTmp++;}}
  
  void deInstallProbe(ProbeType pt){
    PD((PROBES,"Probe DeInstalled to  Tmp: %d Prm: %d site: %s",
	probeCtrTmp, probeCtrPrm,
	remoteSite->site->stringrep()));
    if(pt == PROBE_TYPE_PERM){
      Assert(probeCtrPrm>0);
      probeCtrPrm--;
      if(probeCtrPrm==0 && probeCtrPrm==0)
	clearProbingPrm();
      return;}
    Assert(probeCtrTmp>0);
    probeCtrTmp--;
    if(probeCtrTmp==0){
      clearProbingTmp();
      if(probeCtrPrm==0)
	clearProbingPrm();}}
  
};
  
/************************************************************/
/*  SECTION :: MySiteInfo                                      */
/************************************************************/
class MySiteInfo{
public:
  int        tcpFD;    // the file descriptor of the port
  RemoteSite       *site;
  int        maxNrAck;
  int        maxSizeAck;
}mySiteInfo;

/************************************************************/
/* SECTION :: Managers                                         */
/************************************************************/

class RemoteSiteManager: public FreeListManager{

  RemoteSite* newRemoteSite(){
    RemoteSite* s;
    FreeListEntry *f=getOne();
    if(f==NULL) {s=new RemoteSite();}
    else{GenCast(f,FreeListEntry*,s,RemoteSite*);}
    return s;}

  void deleteRemoteSite(RemoteSite *s){
    FreeListEntry *f;
    GenCast(s,RemoteSite*,f,FreeListEntry*);
    if(putOne(f)) {return;}
    delete s;
    return;}

public:
  RemoteSiteManager():FreeListManager(SITE_CUTOFF){}

  void freeRemoteSite(RemoteSite *s){ 
    deleteRemoteSite(s);}

  RemoteSite* allocRemoteSite(Site *s, int r){ 
    RemoteSite *rs=newRemoteSite();
    PD((SITE,"allocated a:%x ctr:%d rs:%x",s,r,rs));
    rs->init(s, r);
    return rs;}
};

class ReadConnectionManager: public FreeListManager{
  int wc; 
  void deleteConnection(ReadConnection *r){
    FreeListEntry *f;
    GenCast(r,ReadConnection*,f,FreeListEntry*);
    if(!putOne(f)) {delete r;}
    return;}
  ReadConnection* newConnection(RemoteSite *s,int fd0){
    FreeListEntry *f=getOne();
    ReadConnection *r;
    if(f==NULL) {r=new ReadConnection();}
    else {GenCast(f,FreeListEntry*,r,ReadConnection*);}
    r->fastfixerik2();
    r->init(s);
    r->fd=fd0;
    return r;}
public:
  ReadConnectionManager():FreeListManager(READ_CONNECTION_CUTOFF){} 
  
  void freeConnection(ReadConnection *r){ 
    PD((TCP_INTERFACE,"freed r:%x nr:%d",r,--wc));
    //Assert(r->isRemovable());
    deleteConnection(r);
    return;}

  ReadConnection *allocConnection(RemoteSite *s,int f){
    ReadConnection *r=newConnection(s,f);
    PD((TCP_INTERFACE,"allocated r:%x s:%x fd:%d nr:%d",r,s,f,++wc));
    return r;}
};


class WriteConnectionManager: public FreeListManager{
  int wc; 
  
  void deleteConnection(WriteConnection *r){
    FreeListEntry *f;
    GenCast(r,WriteConnection*,f,FreeListEntry*);
    if(!putOne(f)) {delete r;}
    return;}
  WriteConnection* newConnection(RemoteSite *s,int fd0){
    FreeListEntry *f=getOne();
    WriteConnection *r;
    if(f==NULL) {r=new WriteConnection();}
    else {GenCast(f,FreeListEntry*,r,WriteConnection*);}
    r->fastfixerik3();
    r->init(s);
    r->fd=fd0;
    return r;}
public:
  WriteConnectionManager():
    FreeListManager(WRITE_CONNECTION_CUTOFF){wc = 0;} 

  void freeConnection(WriteConnection *r){ 
    PD((TCP_INTERFACE,"freed r:%x nr%d",r,--wc));
    r->clearFlag(WRITE_CON);
    //Assert(r->isRemovable());
    deleteConnection(r);
    return;}

  WriteConnection *allocConnection(RemoteSite *s,int f){
    WriteConnection *r=newConnection(s,f);
    PD((TCP_INTERFACE,"allocated r:%x s:%x fd:%d nr:%d",r,s,f,++wc));
    return r;}
 };

/************************************************************/
/* SECTION :: small routines for RemoteSites                   */
/************************************************************/

void RemoteSite::zeroReferences(){
  PD((SITE,"Zero references to site %s",site->stringrep()));
  if(writeConnection == NULL)  return;
  if(writeConnection -> goodCloseCand())
    writeConnection->closeConnection();
  else
    writeConnection -> setCanClose();}
  
void RemoteSite::nonZeroReferences(){
  PD((SITE,"Non zero references to site %s",site->stringrep()));
  if(writeConnection == NULL && writeConnection->isCanClose())
    writeConnection->clearCanClose();}
  
void RemoteSite::writeConnectionRemoved(){
    writeConnection = NULL;
    if(readConnection == NULL){
      site->dumpRemoteSite(recMsgCtr);
      remoteSiteManager->freeRemoteSite(this);}}

void RemoteSite::readConnectionRemoved(){
  readConnection = NULL;
    if(writeConnection == NULL){
      site->dumpRemoteSite(recMsgCtr);
      remoteSiteManager->freeRemoteSite(this);}}


/************************************************************/
/* SECTION :: tcpError                                         */
/************************************************************/


ipReturn tcpError()
{
switch(ossockerrno()){
  case EPIPE:{
    PD((TCP_HERROR,"Connection socket lost: EPIPE"));
    return IP_PERM_BLOCK;}
  case ECONNRESET:{
    PD((TCP_HERROR,"Connection socket lost: ECONNRESET"));
    return IP_PERM_BLOCK;}
  case EBADF:{
    PD((TCP_HERROR,"Connection socket lost: EBADF"));
    return IP_PERM_BLOCK;}
#ifndef WINDOWS
case ETIMEDOUT:{
  PD((TCP_HERROR,"Connection socket temp: ETIMEDOUT"));
  return IP_TEMP_BLOCK;}
#endif
default:{
  PD((TCP_HERROR,"Unhandled error: %d please inform erik@sics.se",
      ossockerrno()));
  return IP_PERM_BLOCK;}}
return IP_TEMP_BLOCK;
}

/************************************************************/
/* SECTION :: TcpCache                                         */
/************************************************************/

class TcpCache {
  Connection* currentHead; // CurrentConnections
  Connection* currentTail; //
  Connection* closeHead;   // Connections that are closing
  Connection* closeTail;   //
  Connection* myHead;      // Writes that this site has taken down
  Connection* myTail;      //
  Connection* tmpHead;     // Connections that are tmp down
  Connection* tmpTail;     //
  int current_size;
  int close_size;
  int open_size;
  int max_size;
  
  void addToFront(Connection *r, Connection* &head, Connection* &tail){
    if(head==NULL){
      Assert(tail==NULL);
      r->next=NULL;
      r->prev=NULL;
      head=r;
      tail=r;
      return;}
    r->next=head;
    r->prev=NULL;
    head->prev=r;
    head=r;}

  void addToTail(Connection *r, Connection* &head, Connection* &tail){
    if(tail==NULL){
      Assert(head==NULL);
      r->next=NULL;
      r->prev=NULL;
      head=r;
      tail=r;
      return;}
    r->next=NULL;
    r->prev=tail;
    tail->next=r;
    tail=r;}


  Connection* getLast(Connection* &head,Connection* &tail){
    PD((TCPCACHE,"get last"));
    Connection *w = tail;
    if (head == tail) 
      head = tail = NULL;
    else{
      tail = w->prev;
      w->prev = NULL;
      tail->next = NULL;}
    return w;}

  void unlink(Connection *r,Connection* &head, Connection* &tail){
    PD((TCPCACHE,"cache unlink r:%x",r));
    if(tail==r) {
      if(head==r){
	tail=NULL;
	head=NULL;
	return;}
      tail=r->prev;
      tail->next=NULL;
      return;} 
    if(head==r) {
      head=r->next;
      head->prev=NULL;
      return;}
    r->disconnect();}
  
  void decreaseConnections() {
    PD((TCPCACHE,"close"));
    WriteConnection* w;
    ReadConnection* r;
    Connection *c=currentTail;
    WriteConnection *cncls = NULL;
    while(c!=NULL){
      if(c->isWriteCon()){
	GenCast(c, Connection*, w,WriteConnection*);
	if(w->goodCloseCand()){
	  w->closeConnection();
	  return;}
	if(cncls==NULL && w->canbeClosed())
	  cncls = w;}
      else{
	GenCast(c, Connection*,  r, ReadConnection*);
	if(r->goodCloseCand()){
	  r->closeConnection();
	  return;}}
    c = c->next;}
    if(open_size>max_size * 2 && cncls != NULL){
      cncls->closeConnection();
      return;}}

public:
  Bool accept;
  Bool openCon;
  Bool probes;
  
  void closeConnections(){
    PD((TCPCACHE,"ClosingConnections fakingTmp"));
    Connection *c=currentHead, *cc;
    while(c!=NULL){
      PD((TCPCACHE,"Closing Connection %x %x",c,
	  c->getRemoteSite()));
      if(!c->isClosing()){
	PD((TCPCACHE,"Closing connection %x",c));
	cc = c->next;
	if(c->testFlag(WRITE_CON))
	  ((WriteConnection*)c)->closeConnection();
	else
	  ((ReadConnection*)c)->closeConnection();}
      else{
	PD((TCPCACHE,"Not Closing connection %x",c));
	c = c->next;}
      c = cc;}
    openCon = FALSE;}
  
  void openConnections(){
    PD((TCPCACHE,"OpeningConnections %x",tmpHead));
    openCon = TRUE;
    if(tmpHead!=NULL)
      // EK watchThis 5 just for dbging
      wakeUpTmp(WKUPTMP,5);}
  
  TcpCache():accept(FALSE){ 
    currentHead = NULL; 
    currentTail= NULL;
    closeHead= NULL;
    closeTail= NULL;
    myHead= NULL;
    myTail= NULL;
    tmpHead= NULL;
    tmpTail= NULL;
    current_size = 0;
    close_size = 0;
    open_size = 0;
    max_size=MAXTCPCACHE;
    openCon = TRUE;
    probes = FALSE;
    PD((TCPCACHE,"max_size:%d",max_size));}
  
  void nowAccept(){
    Assert(accept==FALSE);
    accept=TRUE;}
  
  Bool Accept(){return accept;}
  Bool CanOpen(){return openCon;}

    
  void adjust(){
    PD((TCPCACHE,"adjusting size:%d maxsize:%d",open_size,max_size));
    if(myHead!=NULL && open_size<max_size){
      openMyClosedConnection();
      return;}
    if(open_size>max_size) decreaseConnections();
    if ((open_size + close_size) > (2 * max_size))
      accept = FALSE;
    else
      accept = TRUE;} 
  
  void add(Connection *w) {
    PD((TCPCACHE,"cache add connection r:%x site:%x o:%d c:%d",w,
	w->getRemoteSite(),open_size,close_size));
    if(w->isOpening()){
      addToFront(w, currentHead, currentTail);  
      open_size++;
      adjust();
      return;}
    if(w->isClosing()){
      addToFront(w, closeHead, closeTail);
      close_size++;
      return;}
    Assert(w->isWriteCon());
    if(((WriteConnection*)w)->isMyInitiative()){
      if(myHead==NULL){
	PD((TCPCACHE,"Starting wakeups"));
	wakeUpTmp(WKUPMYC, WKUPMYC_TIME);}
      else
	PD((TCPCACHE,"Relying on old wkups"));
      addToFront(w, myHead, myTail);
      return;}
    if(((WriteConnection*)w)->isTmpDwn()){
      if(tmpHead == NULL && openCon)
	wakeUpTmp(WKUPTMP,WKUPTMP_TIME);
      addToFront(w, tmpHead, tmpTail);
      return;}
    OZ_warning("Unknown type of connection");
    Assert(0);}
  
  void remove(Connection *w){
    PD((TCPCACHE,"cache remove connection r:%x site:%x o:%d c:%d",w,
	w->getRemoteSite(),open_size, close_size));
    //EK remove thisone?
    if(w->isOpening()){
      unlink(w, currentHead, currentTail);  
      open_size--;
      adjust();
      return;}
    if(w->isClosing()){
      unlink(w, closeHead, closeTail);
      close_size--;
      return;}
    if(w->isWriteCon()){
      if(((WriteConnection*)w)->isMyInitiative()){
	unlink(w, myHead, myTail);
	return;}
      if(((WriteConnection*)w)->isTmpDwn()){
	unlink(w, tmpHead, tmpTail);
return;}}
    unlink(w, currentHead, currentTail);  
    open_size--;
    adjust();}
  
  
  void touch(Connection *r) {
    PD((TCPCACHE,"cache touch r:%x",r));
    if(currentHead!=r){
      unlink(r, currentHead, currentTail);
      addToFront(r,currentHead, currentTail);}
    adjust();}
  
  Bool openTmpBlockedConnection(){
    PD((TCPCACHE,"OpeningTmps %x %x",tmpHead, tmpTail));
    if(tmpHead!=NULL){
      WriteConnection *w = ((WriteConnection *) getLast(tmpHead, tmpTail));
      w->clearTmpDwn();
      w->open();}
    return tmpHead!=NULL;}
  
  Bool openMyClosedConnection(){
    PD((TCPCACHE,"OpeningMys %x %x",myHead, myTail));
    if(myHead!=NULL){
      WriteConnection *w = ((WriteConnection *)getLast(myHead, myTail));
      w->clearMyInitiative();
      w->open();}
    return myHead!=NULL;}

  void probeStarted(){
    // ATTENTION 
    return;
    if(!probes){
      probes = TRUE;
      wakeUpTmp(WKUPPRB, WKUPPRB_TIME);
    }}
  
  Bool sendProbeConnections(){
    PD((TCPCACHE,"SEND On PROBE CONS"));
    probes =  findProbeCons(currentHead) != 0;
    return probes;}
};



/************************************************************/
/* SECTION ::  Exported to Perdio                           */
/************************************************************/

Bool openClosedConnection(int Type){
  PD((TCPCACHE,"OpeningClosedConnections"));
  switch(Type){ 
  case WKUPMYC:
    return tcpCache->openMyClosedConnection();
  case WKUPTMP:
    return tcpCache->openTmpBlockedConnection();
  case WKUPPRB:  
    return tcpCache->sendProbeConnections();
  default:
    Assert(0);}
  return FALSE;}


int openclose(int Type){
  int state = 0;
  if(tcpCache->openCon) state = 1;
  if(Type){
    if(state) tcpCache->closeConnections();
    else tcpCache->openConnections();}
  return state;}
  

/***********************************************************/
/* SECTION :: ?????                                           */
/************************************************************/

void Connection::setClosing(){
  PD((TCP_INTERFACE,"setClosing site:%s",remoteSite->site->stringrep()));
  Assert(!testFlag(CLOSING));
  tcpCache->remove(this);
  setFlag(CLOSING);
  tcpCache->add(this);}

Bool WriteConnection::shouldSendFromUser(){
    if(isClosing()){PD((TCP_INTERFACE,"isClosing")); return FALSE;}
    if(isOpening()) {PD((TCP_INTERFACE,"isOpening")); return FALSE;}
    if(isIncomplete()) {PD((TCP_INTERFACE,"isIncompleteWrite")); return FALSE;}
    if(isInWriteQueue()) {PD((TCP_INTERFACE,"isInWriteQueue")); return FALSE;}
    if(isMyInitiative()) {PD((TCP_INTERFACE,"isMyInitiative")); return FALSE;}
    Assert(fd!=LOST);
    return TRUE;}

ipReturn WriteConnection::open(){
    PD((TCP_INTERFACE,"OpenConnection"));
    setOpening(); 
    tcpCache->add(this);
    return tcpOpen(remoteSite, this);}
/**********************************************************************/
/*   SECTION :: MessageManagers                                           */
/**********************************************************************/

class MessageManager: public FreeListManager {
private:
  int Msgs;
  Message * newMessage(){
    FreeListEntry *f=getOne();
    Message *m;
    if(f==NULL) {m=new Message();}
    else {GenCast(f,FreeListEntry*,m,Message*);}
    return m;}

  void deleteMessage(Message *m){
    FreeListEntry *f;
    GenCast(m,Message*,f,FreeListEntry*);
    if(!putOne(f)) {delete m;}
    return;}

public:
  MessageManager():FreeListManager(MESSAGE_CUTOFF){Msgs = 0;};

  Message * allocMessage(NetMsgBuffer *bs, int msgNum,
			 Site *s, MessageType b, int i){
    Message*m = newMessage();
    PD((MESSAGE,"allocate  nr:%d", ++Msgs));
    m->init(bs,msgNum,s,b,i);
    return m;}

  Message * allocMessage(NetMsgBuffer *bs,tcpMessageType t,int rem){
    Message*m = newMessage();
    PD((MESSAGE,"allocate DANGER r:%x nr:%d", ++Msgs));
    m->init(bs,0,NULL,M_LAST,0);
    m->setRemainder(rem);
    m->setType(t);
    return m;}
  
  void freeMessage(Message *m){
    PD((MESSAGE,"freed nr: %d", --Msgs));
    deleteMessage(m);}

  void freeMessageAndMsgBuffer(Message *m){
    netMsgBufferManager->dumpNetMsgBuffer(m->bs);
    PD((MESSAGE,"BM freed nr:%d", --Msgs));
    deleteMessage(m);}
};


/**********************************************************************/
/*   SECTION :: Methods                                            */
/**********************************************************************/

int findProbeCons(Connection *c){
  if(c==NULL) return 0;
  if(c->isWriteCon()){
    WriteConnection *w = (WriteConnection *)c;
    if(w->isProbingPrm()){
      int nr = 1 + findProbeCons(c->next);
      if(!w->isWritePending()){
	NetMsgBuffer* nb = netMsgBufferManager->
	  getNetMsgBuffer(NULL);
	nb->constructMsg(w->remoteSite,TCP_PING_REQUEST);
	Message* m= messageManager->
	  allocMessage(nb, NO_MSG_NUM, NULL,(MessageType) 0, 0); 
	switch(tcpSend(w->fd,m,TRUE)){
	case IP_OK:{
	  PD((TCP,"PingMsg sent"));
	  messageManager->freeMessageAndMsgBuffer(m);
	  break;}
	case IP_BLOCK:{
	  PD((TCP,"PingMsg Blocked"));
	  w->addCurQueue(m);
	  OZ_registerWriteHandler(w->fd,tcpWriteHandler,(void *)w);
	  break;}
	case IP_PERM_BLOCK:{
	  PD((TCP,"CloseMsg nSent sitecrash"));
	  messageManager->freeMessageAndMsgBuffer(m);
	  w->connectionLost();
	  break;}
	default: Assert(0);}}
      return nr;}
  }
  return findProbeCons(c->next);}


int RemoteSite::discardUnsentMessage(int msgNum){
  Message *m  = writeQueue.find(msgNum);
  if (m != NULL){
    NetMsgBuffer *bs = m->bs;
    messageManager->freeMessage(m);
    return (int) bs;}
  // EK
  // Should the message be looked for
  // among the maybe sent msgs?
  return writeConnection->discardUnsentMessage(msgNum);}


/********************************************************

      RemoteSite DBG

********************************************************/

Bool WriteConnection::checkAckQueue(){
  Message *m = sentMsg;
  if (m == NULL ) return FALSE;
  while (m != NULL){
    PD((ACK_QUEUE,"Ack queue ptr: %d nr: %d ctr: %d",
	m,m->msgNum,sentMsgCtr));
    m=m->next;}
  return TRUE;
}
  
/************************************************************/
/*  SECTION :: Stornig messages till ack received           */
/************************************************************/

void RemoteSite::storeSentMessage(Message* m) {
  PD((ACK_QUEUE,"Adding to ack queue  %d",m));
  Assert(writeConnection != NULL);
  writeConnection->storeSentMessage(m);}
  
void WriteConnection::storeSentMessage(Message* m) {
  m->next = sentMsg;
  sentMsg = m;
  PD((ACK_QUEUE,"*** QUEUE CHECKED %d***",checkAckQueue()));}

int WriteConnection::discardUnsentMessage(int msgNum){
  Message *m = sentMsg;
    if(m != NULL){
	if(m->getMsgNum() == msgNum){
	    NetMsgBuffer *bs = m->getMsgBuffer();
	    sentMsg = m->next;
	    messageManager->freeMessage(m);
	    return (int) bs;}
	Message *ptr = m->next;
	while(ptr != NULL){
	  if(ptr->getMsgNum() == msgNum){
	    NetMsgBuffer *bs = ptr->getMsgBuffer();
	    m->next = ptr->next;
	    messageManager->freeMessage(ptr);
	    return (int) bs;}
	  m->next = ptr;
	  ptr = ptr->next;}}
    return MSG_SENT;}

/* *****************************************************************
            MsgBuffer operations
   *******************************************************************/

/* read       pos= first BYTE    endpos = first free slot or NULL
   interpret  pos= first BYTE curpos= last BYTE 
                              endpos= first free slot or NULL */

BYTE* NetMsgBuffer::initForRead(int &len){
  Assert(type==BS_None);
  PD((BUFFER,"bytebuffer alloc Read: %d",no_bufs()));
  pos=first->head();
  endpos=first->head();
  len=BYTEBUFFER_SIZE;
  type=BS_Read;
  return endpos;}

BYTE* NetMsgBuffer::beginRead(int &len){
  Assert(type==BS_None);
  type=BS_Read;
  if(endpos==NULL){
    if(pos==NULL){
      pos=first->head();
      endpos=first->head();
      len=BYTEBUFFER_SIZE;
      return endpos;}
    Assert(within(pos,first));    
    PD((BUFFER,"bytebuffer alloc Read: %d",no_bufs()));
    ByteBuffer *bb=byteBufferManager->newByteBuffer();
    last->next=bb;
    last=bb;
    len=BYTEBUFFER_SIZE;
    endpos=last->head();
    return endpos;}
  if(pos==NULL){pos=endpos;}
  else{Assert(within(pos,first));}
  len=last->tail()-endpos+1;
  Assert(len<BYTEBUFFER_SIZE);
  return endpos;}

void NetMsgBuffer::afterRead(int len){
  Assert(type==BS_Read);
  Assert(len<=BYTEBUFFER_SIZE);
  Assert(availableSpace()>=len);
  Assert(within(endpos,last));  
  type=BS_None;
  if(endpos+len-1==last->tail()){endpos=NULL;}
  else{endpos=endpos+len;}}

void NetMsgBuffer::beforeInterpret(int len){
  if(pos==NULL) {return;} /* read only header for close message*/
  Assert(type==BS_None);
  Assert(within(pos,first));
  Assert(len<=0);
  if(endpos==NULL){
    curpos=last->tail()+len;}
  else{
    curpos=endpos+len-1;}}

void NetMsgBuffer::afterInterpret(){
  Assert(first==last);
  Assert(first!=NULL);
  pos=curpos+1;
  Assert(within(curpos,last));}
    
BYTE NetMsgBuffer::getInRead(){
  Assert(within(pos,first));
  if(pos==first->tail()){
    BYTE ch=*pos;
    if(first==last){
      pos=NULL;
      return ch;}
    PD((BUFFER,"bytebuffer dumped read (SPEC):%d",no_bufs())); 
    byteBufferManager->deleteByteBuffer(first);
    first=last;
    pos=first->head();
    return ch;}
  return *pos++;}

int NetMsgBuffer::interLen(){
  if(first==last){return curpos-pos+1+tcpHeaderSize;}
  int i=first->tail()-pos+1+tcpHeaderSize;
  ByteBuffer *bb=first->next;
  while(bb!=last){bb=bb->next;i+=BYTEBUFFER_SIZE;}
  return i+curpos-last->head()+1;}
  
/* *****************************************************************
 * *****************************************************************
 * *****************************************************************
 * TCP
 *      Startup
 *         createTcpPort      network side of initializing global port
 *      Writer   
 *         tcpOpen            writer wants to open connection 
 *         tcpConnectionHandler writer before connection acknowledgement
 *         tcpCloseHandler    handles reader-initiated close connections
 *         tcpOpenCheck       tries to open connections for pending msgs
 *         tcpWriteHandler    handles write-continuations (prev blocked) 
 *         tcpSend            basic send 
 *         
 *      Reader   
 *         tcpReadHandler     handles reads 
 *         tcpAcceptHandler   handles writer-inititated opens
 *         
 * *****************************************************************
 * *****************************************************************
 * *************************************************************** */

inline BYTE *int2net(BYTE *buf,int i){ 
  for (int k=0; k<4; k++) { 
    *buf++ = i & 0xFF; 
    i = i>>8;}
  return buf;}

inline BYTE *short2net(BYTE *buf,short i){ 
  for (int k=0; k<2; k++) { 
    *buf++ = i & 0xFF; 
    i = i>>8;}
  return buf;}

inline int net2int(BYTE *buf){
  unsigned int i1 = *buf++;
  unsigned int i2 = *buf++;
  unsigned int i3 = *buf++;
  unsigned int i4 = *buf++;
  return (int) (i1 + (i2<<8) + (i3<<16) + (i4<<24));}

inline short net2short(BYTE *buf){
  unsigned int i1 = *buf++;
  unsigned int i2 = *buf++;
  return (short) (i1 + (i2<<8));} 


void NetMsgBuffer::beginWrite(RemoteSite *s){
  Assert(type==BS_None);
  Assert(first!=NULL);
  remotesite =  s;
  BYTE* thispos= first->head();
  *thispos++=TCP_PACKET;
  type=BS_Write;
  int2net(thispos,totlen);}

void NetMsgBuffer::resend(){
  Assert(type == BS_Write);
  reset();
  pos = first->head();}

void NetMsgBuffer::reset(){
  Assert(start!=NULL && stop != NULL);
  first= start;
  last = stop;}

void NetMsgBuffer::unmarshalReset(){
  reset();
  pos = first->head() + tcpHeaderSize ;
  if(endpos==NULL) curpos=last->tail();
  else curpos= endpos-1;
}

void NetMsgBuffer::PiggyBack(Message* m)
{
  int msgCtr = remotesite->getWriteConnection()->incMsgCtr();
  BYTE* thispos= first->head() + 5;
  int2net(thispos, msgCtr);
  thispos += 4;
  int2net(thispos, remotesite->getRecMsgCtr());
  m->setMsgNum(msgCtr);
  remotesite->storeSentMessage(m);
  PD((ACK_QUEUE,"����� Message nr: %d Ack nr: %d inserted ��������"
      ,msgCtr, remotesite->getRecMsgCtr()));
}

void NetMsgBuffer::constructMsg(RemoteSite* rs, tcpMessageType msg){
  type = BS_Write;
  first=getAnother();
  start=first;
  last=first;
  stop=first;
  remotesite = rs;
  pos=first->head();
  *pos=msg;
  pos++;
  int2net(pos,tcpHeaderSize);
  pos+=4;
  int2net(pos,remotesite->getWriteConnection()->getMsgCtr());

  pos+=4;
  int2net(pos,remotesite->getRecMsgCtr());
  pos+=4;
  endpos=pos;
  pos=first->head();
}
  

/* *****************************************************************
   *****************************************************************
     STARTUP
   *****************************************************************
 * *************************************************************** */

inline Bool createTcpPort_RETRY(){
  if(ossockerrno()==ENOENT) return TRUE;
  if(ossockerrno()==EADDRINUSE) return FALSE;
  return FALSE;}
  

static ipReturn createTcpPort(int port,ip_address &ip,port_t &oport,int &fd) 
{
  PD((TCP,"Start createTcpPort: p:%d",port));
  int smalltries=OZConnectTries;
  int bigtries=OZStartUpTries;

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  
  struct utsname auname;

  if(uname(&auname)<0) {NETWORK_ERROR(("createTcpPort"));}
  struct hostent *hostaddr;
  hostaddr=gethostbyname(auname.nodename);
  struct in_addr tmp;
  memcpy(&tmp,hostaddr->h_addr_list[0],sizeof(in_addr));
  ip=ntohl(tmp.s_addr);

retry:
  if(bigtries<0){
    PD((WEIRD,"bind - ran out of tries"));
    return IP_TIMER;} // ATTENTION - major fault
  fd=ossocket(PF_INET,SOCK_STREAM,0);
  if (fd < 0) {
    if(errno==ENOBUFS) return IP_TIMER;
    NETWORK_ERROR(("system:socket %d\n",errno));}
  addr.sin_port = htons(port);

  if(bind(fd,(sockaddr *) &addr,sizeof(struct sockaddr_in))<0) {
    PD((WEIRD,"create TcpPort bind redo port:%d errno:%d",
	port,errno));
    osclose(fd);
    smalltries--;
    if(createTcpPort_RETRY() && smalltries>0) goto retry;
    bigtries--;
    port++;
    smalltries=OZConnectTries;
    goto retry;}
  PD((TCP,"createTcpPort: got ip:%u port:%u",ip,port));
  if (listen(fd,5)<0) {NETWORK_ERROR(("listen %d\n",errno));}

  struct sockaddr_in addr1;
#if __GLIBC__ == 2
  unsigned int length = sizeof(addr1);
#else
  int length = sizeof(addr1);
#endif
  if (getsockname(fd, (struct sockaddr *) &addr1, &length) < 0) {
    NETWORK_ERROR(("getsockname %d\n",errno));
  }
  oport=ntohs(addr1.sin_port);

  PD((TCP,"create TcpPort OK"));
  return IP_OK;
}

/* *****************************************************************
   *****************************************************************
          WRITER
   *****************************************************************
   *************************************************************** */

/* *****************************************************************
     tcpOpen
   *************************************************************** */


#define TCPOPENMSGBUFFER_SIZE 100
class TcpOpenMsgBuffer:public MsgBuffer{
  BYTE buffer[TCPOPENMSGBUFFER_SIZE];
  BYTE *pos;
  int size;
public:
  TcpOpenMsgBuffer(){
    MsgBuffer::init();
    pos = NULL;}
  void marshalBegin(){
    posMB = buffer + 5;
    endMB = buffer + TCPOPENMSGBUFFER_SIZE;
    size = 0;}
  void marshalEnd(){ size = posMB - buffer  - 5;}
  void unmarshalMarshal(){}
  void beginWrite(BYTE b){
    pos = buffer;
    *pos++=b;
    int2net(pos,size);
    size += 5;
    pos = NULL;}
  void unmarshalBegin(){
    posMB = buffer;
    endMB = buffer + TCPOPENMSGBUFFER_SIZE;}
  void unmarshalEnd(){
    pos = NULL;}
  Site *getSite(){
    Assert(0);
    return NULL;}
  Bool isPersistentBuffer() { return NO; }
  int getLen(){
    return size;}
  BYTE* getBuf(){
    return buffer;}
  char* siteStringrep(){
    Assert(0);
    return "Assert(0)";}
  BYTE readyForRead(int &len){
    BYTE b=get();
    len = net2int(posMB);
    posMB += 4;
    return b;}
  BYTE getNext(){
    Assert(0);
    return 0;}
  void putNext(BYTE b){
    Assert(0);}
};

#define USE_OLD_PORT -1

/*  *****************************************************************
          tcpSend
   ***************************************************************** */

ipReturn tcpSend(int fd,Message *m, Bool flag) 
{
  int total,len,ret;
  NetMsgBuffer *bs = m->getMsgBuffer();
  if(flag){
    total=bs->calcTotLen();
    PD((TCP,"tcpSend invoked rem:%d",total));}
  else{
    total=bs->getTotLen();
    PD((CONTENTS,"tot:%d",total));
    PD((TCP,"tcpSend invoked tot:%d",total));
    bs->PiggyBack(m);}
      
  while(total){
    Assert(total>0);
    len=bs->getWriteLen();
    BYTE* pos=bs->getWritePos();
    while(TRUE){
      ret=oswrite(fd,pos,len);

      if(ret>0) break;
      // EK check ret. The Assert will disapear in
      // non-debug code.
      Assert(ret<0);
      if(errno==EINTR) continue;
      if(!((errno==EWOULDBLOCK) || (errno==EAGAIN)))
	return tcpError();
      break;}
    PD((WRITE,"wr:%d try:%d error:%d",ret,len,errno));
    if(ret<len){
      if(ret>0){
	bs->incPosAfterWrite(ret);
	PD((TCP,"tcpSend blocked wrote %d of %d",ret,len));}
      return IP_BLOCK;}
    bs->sentFirst();
    total -=len;}
  bs->writeCheck();
  PD((TCP,"tcpSend fin"));
  return IP_OK;
}


/* ********************************************************************** 
         tcpConnectionHandler
   ********************************************************************** */

int smallMustRead(int fd,BYTE *pos,int todo,int tries){
  PD((TCP,"smallMustRead invoked"));  
  while(TRUE){
    int ret=osread(fd,pos,todo);
    if(ret==todo) return 0;
    if(ret == 0) return IP_BLOCK;
    if(ret<0){
      // EK
      // Handle these errors. They can occur.
      // Inform Connection and Site.
      if(errno!=EINTR) return IP_BLOCK;
      tries--;
      if(tries<0) {return IP_NO_MORE_TRIES;}
    pos += ret;
    todo -= ret;}}}

#define CONNECTION_HANDLER_TRIES 50


inline ipReturn writeI(int fd,BYTE *buf)
{
  int ret;
  while(TRUE){
    ret = oswrite(fd,buf,1);
    if(ret>0){
      Assert(ret==1);
      return IP_OK;}
    if (errno == EINTR){
      PD((WEIRD,"write interrupted"));}
    else {
      if((errno==EWOULDBLOCK) || (errno==EAGAIN)) {
	return IP_BLOCK;}
      NETWORK_ERROR(("writeI %d\n",errno));}}}

inline ipReturn readI(int fd,BYTE *buf)
{
  int ret;
  while(TRUE){
    ret = osread(fd,buf,1);
    if(ret>0){
      Assert(ret==1);
      return IP_OK;
    }
    
    if (ret==0) {
      PD((TCP,"Connection closed/lost %d",fd));
      return IP_EOF;
    }

    if (errno == EINTR){
      PD((WEIRD,"readI interrupted"));
    } else {
      if((errno==EWOULDBLOCK) || (errno==EAGAIN)) {
	return IP_BLOCK;
      }
      PD((WEIRD,"readI discoversCRASH errno:%d",errno));
      return IP_NET_CRASH;
    }
  }
}

/* *****************************************************************
   *****************************************************************
          READER
   *****************************************************************
   *************************************************************** */

#define EAGAIN_TRIES 100000


/************************************************************/
/* SECTION :: Div small routines                              */
/************************************************************/

void uniquefyInt(BYTE *buf, int Num){
  PD((TCP,"Intfy %d",Num));
  int ctr = 0;
  BYTE b;
  while(ctr < 33){
    b = Num & 255;
    b <<=  TCP_MSG_SIGN_BITS;
    *buf++ = b;
    PD((TCP,"Writing to buffer %d lim:33 act:%d", b, ctr));
    Num >>= (8 - TCP_MSG_SIGN_BITS);
    ctr += 8 - TCP_MSG_SIGN_BITS;}}

int intifyUnique(BYTE *buf){
  int ctr = 0;
  unsigned int i;
  int ans = 0;
  while(ctr < 33){
    i = *buf++;
    i >>= TCP_MSG_SIGN_BITS;
    ans += i << ctr;
    PD((TCP,"Reading from buffer %d ans:%d lim:33 act:%d", i,ans,ctr));
    ctr += (8 - TCP_MSG_SIGN_BITS);}
  PD((TCP,"Ack read: %d", ans));
  return ans;}

/************************************************************/
/* SECTION :: ReadStuff                                        */
/************************************************************/

Bool tcpAckReader(ReadConnection *r, int ack){
  BYTE buffer[INT_IN_BYTES_LEN + 1];
  BYTE *buf = buffer;
  int fd=r->getFD();
  Assert(fd!=LOST);
  ipReturn ret;
  PD((TCP,"tcpAckReader r:%x a:%d",r, ack));    
  *buf=(BYTE) TCP_MSG_ACK_FROM_READER;
  uniquefyInt(buf + 1, ack);
  while((buf - buffer) < INT_IN_BYTES_LEN + 1){
    PD((TCP,"Writing byte: %d diff: %d tot: %d",*buf,buf - buffer,INT_IN_BYTES_LEN ));
    ret=writeI(fd,buf++);
    if(ret!=IP_OK){
      PD((TCP,"Write fail in Acking"));
      return FALSE;}}
  return TRUE;}

inline
int tcpRead(int fd,BYTE *buf,int size,Bool &readAll)
{
  int ret;
  int no_tries=EAGAIN_TRIES;
  while(no_tries){
    ret = osread(fd,buf,size);
    if (ret > 0) {
      if(ret==size) {readAll=TRUE;}
      else {readAll=FALSE;}
      return ret;}
    if (ret==0) return -1;
    if (errno == EINTR){ 
      PD((WEIRD,"read interrupted"));
      no_tries--;
      continue;}
    if(errno==EAGAIN || errno==EWOULDBLOCK){
      PD((WEIRD,"read EAGAIN"));
      no_tries--;
      continue;}
    break;}
  return ret;
}

inline 
ipReturn interpret(NetMsgBuffer *bs,tcpMessageType type, Bool
 ValidMsg){
  switch(type){
  case TCP_PACKET:{
    if(ValidMsg){
      PD((TCP,"interpret-packet"));      
      PD((TCP,"received TCP_PACKET"));
      bs->getSite()->msgReceived(bs);}
    return IP_OK;}
  case TCP_CLOSE_REQUEST_FROM_WRITER:{
    PD((TCP,"interpret - close"));      
    return IP_CLOSE;}
  case TCP_PING_REQUEST:{
    PD((TCP,"interpret ping"));      
    return IP_OK; }
  default:{
    OZ_warning("Something is very wrong");
    Assert(0);
    return IP_GARBAGE;}
  }
}

inline tcpMessageType getHeader(NetMsgBuffer *bs,int &len, int &msg, int &ans){
  BYTE buf[4];
  tcpMessageType type;
  BYTE b=bs->getInRead();
  GenCast(b,BYTE,type,tcpMessageType);
  buf[0]=bs->getInRead();
  buf[1]=bs->getInRead();
  buf[2]=bs->getInRead();
  buf[3]=bs->getInRead();
  len=net2int(buf);
  buf[0]=bs->getInRead();
  buf[1]=bs->getInRead();
  buf[2]=bs->getInRead();
  buf[3]=bs->getInRead();
  msg=net2int(buf);
  buf[0]=bs->getInRead();
  buf[1]=bs->getInRead();
  buf[2]=bs->getInRead();
  buf[3]=bs->getInRead();
  ans=net2int(buf);
  return type;}

// EKC
// Check this one out
// Uses the 5 arg allocMessage....

inline Message* newReadCur(Message *m,NetMsgBuffer *bs,
			   ReadConnection *r,
			   tcpMessageType type,int rem){
  if(m==NULL){
    PD((WEIRD,"newReadCur r:%x",r));
    Message *m=messageManager->allocMessage(bs,type,rem);
    r->addCurQueue(m);  return m;}
  else{
    PD((WEIRD,"oldReadCur r:%x",r));
    m->setType(type);
    m->setRemainder(rem);
    r->addCurQueue(m);  return m;}


}
 
/*   ***************************************************************** */


#define PREREAD_HANDLER_TRIES 50
#define PREREAD_NO_BYTES 5


/************************************************************/
/* SECTION :: Handshake Readin Site                            */
/************************************************************/

int accHbufSize = 9 + strlen(PERDIOVERSION);
BYTE *accHbuf = new BYTE[accHbufSize];

static int acceptHandler(int fd,void *unused)
{
  PD((TCP_INTERFACE,"acceptHandler: %d",fd));
  struct sockaddr_in from;
  int fromlen = sizeof(from);
  int newFD=osaccept(fd,(struct sockaddr *) &from, &fromlen);

  if (newFD < 0) {NETWORK_ERROR(("acceptHandler:accept %d\n",errno));}

  if (!tcpCache->Accept()|| !tcpCache->CanOpen()){
    PD((TCP_INTERFACE,"Connection Refused"));
    osclose(newFD);
    return 0;}
  
  ip_address ip=from.sin_addr.s_addr;
  port_t port=from.sin_port;
  Bool accept = tcpCache->Accept();
  
  BYTE *auxbuf = accHbuf;
  
  *auxbuf = TCP_CONNECTION;
  auxbuf++;
  int2net(auxbuf,mySite->getTimeStamp());
  auxbuf += netIntSize;
  int2net(auxbuf, accHbufSize - 9);
  auxbuf += netIntSize;
  for (char *pv = PERDIOVERSION; *pv;) {
    PD((TCP,"Writing %c %d",*pv,auxbuf));
    *auxbuf++ = *pv++;}

  Assert(auxbuf-accHbuf == accHbufSize);
  // EK!!
  // fcntl(newFD,F_SETFL,O_NONBLOCK);
  fcntl(newFD,F_SETFL,O_NDELAY);
  int written = 0;
  while(TRUE){
    int ret=oswrite(newFD,accHbuf+written,accHbufSize-written);
    written += ret;
    if(written==accHbufSize) break;
    if(ret<=0 && errno!=EINTR && errno!=EWOULDBLOCK && errno!=EAGAIN ) { 
      DebugCode(OZ_warning("Error in OPening, we're closing");)
      osclose(newFD);
      return 0;}}
  
  ReadConnection *r=readConnectionManager->allocConnection(NULL,newFD);
  r->setOpening();
  tcpCache->add(r);
  PD((TCP_CONNECTIONH,"acceptHandler success r:%x",r)); 
  OZ_registerReadHandler(newFD,tcpPreReadHandler,(void *)r);
  return 0; }

int tcpPreReadHandler(int fd,void *r0){
  ReadConnection *r=(ReadConnection *)r0, *old;
  tcpOpenMsgBuffer->unmarshalBegin();
  BYTE *pos = tcpOpenMsgBuffer->getBuf(),header;
  int ackStartNr,maxNrSize,todo;
  Site *si;
  PD((TCP_CONNECTIONH,"tcpPreReadHandler invoked r:%x",r));  
  int ret=smallMustRead(fd,pos,PREREAD_NO_BYTES,PREREAD_HANDLER_TRIES);
  if(ret!=0){
    if(ret<0) 
      goto tcpPreFailure; 
    pos += ret;}
  pos +=5;
  header =  tcpOpenMsgBuffer->readyForRead(todo);
  
  if(header!=TCP_MYSITE && header!=TCP_MYSITE_ACK) goto tcpPreFailure;
  if(smallMustRead(fd,pos,todo,PREREAD_HANDLER_TRIES)!=0) goto tcpPreFailure;

  ackStartNr = unmarshalNumber(tcpOpenMsgBuffer);
  maxNrSize= unmarshalNumber(tcpOpenMsgBuffer);
  si = unmarshalSite(tcpOpenMsgBuffer);
  old = si->getRemoteSite()->getReadConnection();
  if(old!=NULL){
    PD((TCP,"ReadConnection has been disconnected, not discovered"));
    old->close();}
  
  si->getRemoteSite()->setReadConnection(r);
  
  tcpOpenMsgBuffer->unmarshalEnd();

  if(header==TCP_MYSITE_ACK) if(!r->resend()) goto tcpPreFailure;

  r->setMaxSizeAck(maxNrSize);
  r->clearOpening();
  OZ_registerReadHandler(fd,tcpReadHandler,(void *)r);  
  return 0;
 
tcpPreFailure:
  osclose(fd);
  tcpCache->remove(r);
  readConnectionManager->freeConnection(r);
  return 1;  
}

static int tcpReadHandler(int fd,void *r0)
{  
  PD((TCP,"tcpReadHandler Invoked"));
  ReadConnection *r = (ReadConnection*) r0;


  int ret,rem,len,msgNr,ansNr;
  int totLen;
  ipReturn ip;
  tcpMessageType type = TCP_NONE;
  Message *m;
  NetMsgBuffer *bs;
  BYTE *pos;
  Assert(fd==r->getFD());
  Bool readAll;
  
  if(r->isIncomplete()){
    m=r->getCurQueue();
    Assert(m!=NULL);
    bs=m->getMsgBuffer();
    rem=m->getRemainder();
    type=m->getType();
    PD((TCP,"readHandler incomplete r:%x rem:%d",r,rem));
    pos=bs->beginRead(len);
    if (rem >= 0){ 
      msgNr = r->remoteSite->readRecMsgCtr() + 1;
      // ATTENTION
      totLen = rem;
    }
  }
  else{
    m=NULL;
    //EK
    rem=0-tcpHeaderSize;
    bs=netMsgBufferManager->getNetMsgBuffer(r->remoteSite->site);
    PD((TCP,"readHandler from scratch r:%x",r));
    bs->getSingle();
    pos=bs->initForRead(len);}

start:

  Assert(osTestSelect(fd,SEL_READ));
  ret=tcpRead(fd,pos,len,readAll);
  if (ret<0) {
    
    if(r->isClosing()){
      PD((TCP,"tcpReadHandler ses Close"));
      netMsgBufferManager->dumpNetMsgBuffer(bs);
      r->niceClose();
      r->close();
      return 0;}
    
    if(m!=NULL){
      fprintf(stderr,"dumping incomplete read\n");
      Assert(r->isIncomplete());
      m = r->getCurQueue();
      messageManager->freeMessage(m);}
        
    netMsgBufferManager->dumpNetMsgBuffer(bs);
    if(tcpError() == IP_TEMP_BLOCK)
      r->close();
    else{
      warning("Connection Site Has Crashed error: %d %s ", ossockerrno(),
	      r->remoteSite->site->stringrep());
      r->connectionLost();}
    return 0;}
  
  PD((READ,"no:%d av:%d rem:%d",ret,len,rem));
  bs->afterRead(ret);
  
  while(TRUE){
    PD((READ,"WHILE no:%d av:%d rem:%d",ret,len,rem));
    if(rem<0){
      if(rem+ret>=0){
	type=getHeader(bs,len,msgNr,ansNr);
	r->remoteSite->receivedNewAck(ansNr);
	totLen = len;
	PD((READ,"Header done no:%d av:%d rem:%d tcp:%d",ret,len,rem,tcpHeaderSize));
	rem=len-ret-tcpHeaderSize-rem;}
      else{
	rem +=ret;
	goto maybe_redo;}}
    else{
      rem -=ret;}
  
    if(rem>0){goto maybe_redo;}
    
    PD((CONTENTS," Informin: %d %d %d ",msgNr,ansNr,totLen));

    /***************************************************/
    /* ATTENTION                                       */
    /* EK                                              */
    /* For concistency, When to inform the remoteSite  */
    /* About the received Message.                     */
    /***************************************************/
    
    bs->beforeInterpret(rem);
    PD((CONTENTS,"interpret rem:%d len:%d",
		 rem,bs->interLen()));
    // EK this might be done in a nicer way...
    ip=interpret(bs,type,r->informSiteAck(msgNr,totLen));
    if(ip==IP_CLOSE){
      Assert(rem==0);
      if(m!=NULL){
	Assert(r->isIncomplete());
	messageManager->freeMessage(m);}
      netMsgBufferManager->dumpNetMsgBuffer(bs);
      goto close;}
    bs->afterInterpret();    
    if(rem==0){goto fin;}
    ret=0-rem;
    rem=0-tcpHeaderSize;}

maybe_redo:  

  if(readAll && osTestSelect(fd,SEL_READ)){
    if(rem==0) {pos=bs->initForRead(len);}
    else {pos=bs->beginRead(len);}
    goto start;}
  
fin:
  if(!r->isClosing())
    tcpCache->touch(r);
  if(rem==0){
    if(m==NULL){
      // EK check this out...
      // ATTENTION
      // Here we must dump all the ByteBuffers, they should
      // have been removed dynamicly during marshaling.
      // ATTENTION
      netMsgBufferManager->dumpNetMsgBuffer(bs);
      return 0;}
    Assert(!r->isIncomplete());
    messageManager->freeMessageAndMsgBuffer(m);
    return 0;}
  newReadCur(m,bs,r,type,rem);
  return 0;

close:

  PD((TCP,"readHandler received close"));
  if(rem!=0) {NETWORK_ERROR(("readHandler gets bytes after Close"));}
  switch(type){
  case TCP_CLOSE_REQUEST_FROM_WRITER:{
    if(r->isClosing())
      return 0;
    r->setClosing();
    if(!tcpAckReader(r, r->remoteSite->getRecMsgCtr())){
      OZ_warning("During close, acks faild to be sent");
      OZ_warning("Inform erik");
      Assert(0);}
    
    BYTE msg=TCP_CLOSE_ACK_FROM_READER;
    while(TRUE){
      int ret=oswrite(fd,&msg,1);
      if(ret==1) break;
      if(ret<0){
	if((ret!=EINTR) && (ret!=EWOULDBLOCK) && (ret!=EAGAIN)){
	  r->connectionLost();
	  return 0;}}}
    return 0;}
  default:{
    Assert(0);
    return 0;}
  }
}



/************************************************************/
/* SECTION :: Handshake Writing Site                           */
/************************************************************/

static ipReturn tcpOpen(RemoteSite *remoteSite,WriteConnection *r) 
{
  port_t aport;
  
  if(!tcpCache->CanOpen()){
    r->connectionBlocked();
    return IP_TEMP_BLOCK;}
  
  aport=remoteSite->getPort();
  PD((TCP,"open s:%s",remoteSite->site->stringrep()));
  
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr=htonl(remoteSite->getAddress());
  addr.sin_port = htons(aport);
  
  int tries=OZConnectTries;
  int fd;
  int one = 1;
  
retry:
  PD((TCP,"Opening connection t:%d",tries));
  fd=ossocket(PF_INET,SOCK_STREAM,0);
  if (fd < 0) {
    if(errno==ENOBUFS){
      r->connectionBlocked();
      return IP_TEMP_BLOCK;}
    r->connectionLost();
    return IP_PERM_BLOCK;}
  
  if(setsockopt(fd,IPPROTO_TCP,TCP_NODELAY,(char*) &one,sizeof(one))<0){
    r->connectionBlocked();
    return IP_TEMP_BLOCK;}
  
  // EK!!
  fcntl(fd,F_SETFL,O_NDELAY);
  if(osconnect(fd,(struct sockaddr *) &addr,sizeof(addr))==0 
     || ossockerrno()==EINPROGRESS) {
    PD((TCP,"open success p:%d fd:%d",aport,fd));
    r->setFD(fd);
    OZ_registerReadHandler(fd,tcpConnectionHandler,(void *)r);
    PD((OS,"register READ %d - tcpConnectionHandler",fd));
    return IP_OK;}
  
  tries--;
  if(tries<=0){
    r->connectionBlocked();
    return IP_TEMP_BLOCK;}
  if(ossockerrno() == ECONNREFUSED || ossockerrno() == EADDRNOTAVAIL       ){
    r->connectionLost();
    return IP_PERM_BLOCK;}
  addr.sin_port = htons(aport);
  goto retry;}


int tcpConnectionHandler(int fd,void *r0){
  WriteConnection *r=(WriteConnection *)r0;
  int ret,bufSize = 9, tcheck,todo=bufSize, written;
  BYTE *buf = new BYTE[bufSize], *pos = buf;
  unsigned int strngLen;
  time_t timestamp;
  char msgType;
  
  PD((TCP,"tcpConnectionHandler invoked r:%x",r));  
  if(smallMustRead(fd,pos,bufSize,CONNECTION_HANDLER_TRIES))
  goto tcpConFailure;
  
  pos = buf;
  if(*pos!=TCP_CONNECTION) goto tcpConFailure;
    
  pos++;
  timestamp=net2int(pos);
  tcheck=r->remoteSite->site->checkTimeStamp(timestamp);
  if(tcheck!=0) goto tcpConPermLost;
    
  pos += netIntSize;
  strngLen = net2int(pos);

  delete buf;
  buf = new BYTE[strngLen + 1];
  pos = buf;
  
  ret=smallMustRead(fd,pos,strngLen,CONNECTION_HANDLER_TRIES);
  if(ret!=0) goto tcpConFailure;
     
  if (strlen(PERDIOVERSION)!=strngLen ||
      strncmp(PERDIOVERSION,(char*)pos,strngLen)!=0) goto tcpConPermLost;   
  
  delete buf;

  PD((TCP,"Sending My Site Message..%s",mySite->stringrep()));
  if(r->isAcked()) msgType = TCP_MYSITE;
  else msgType = TCP_MYSITE_ACK;

  tcpOpenMsgBuffer->marshalBegin();
  marshalNumber(r->getMsgCtr(), tcpOpenMsgBuffer);
  marshalNumber(mySiteInfo.maxSizeAck, tcpOpenMsgBuffer);
  mySite->marshalSite(tcpOpenMsgBuffer);
  tcpOpenMsgBuffer->marshalEnd();
  tcpOpenMsgBuffer->beginWrite(msgType);

  {
    bufSize = tcpOpenMsgBuffer->getLen();
    BYTE *otherbuf=tcpOpenMsgBuffer->getBuf();
    written = 0;
    
    while(TRUE){
      ret=oswrite(fd,otherbuf+written,bufSize-written);
      written += ret;
      if(written==bufSize) break;
      if(ret<=0 && ret!=EINTR && ossockerrno()!=EWOULDBLOCK && ossockerrno()!=EAGAIN )
	goto tcpConFailure;}
  }

  PD((OS,"unregister READ fd:%d",fd));
  // fcntl(fd,F_SETFL,O_NONBLOCK);
  // fcntl(fd,F_SETFL,O_NDELAY);
  OZ_registerReadHandler(fd,tcpCloseHandler,(void *)r);
  PD((OS,"register READ - close fd:%d",fd));
  
  if(r->isWritePending())
    OZ_registerWriteHandler(fd,tcpWriteHandler,(void *)r);
  PD((TCP,"tcpConnectionHandler ord fin"));    
  r->opened();
  return 0;
  
tcpConFailure:
  osclose(fd);
  delete buf;
  r->connectionBlocked();
  return 1;
  
tcpConPermLost:
  osclose(fd);
  delete buf;
  r->connectionLost();
  return 1;
}



/************************************************************/
/* SECTION ::  WriteHandlers                                    */
/************************************************************/

int tcpWriteHandler(int fd,void *r0){
  WriteConnection *r=(WriteConnection *)r0;
  Message *m;
  ipReturn ret;
  RemoteSite* site = r->remoteSite;
  
  PD((TCP,"tcpWriteHandler invoked r:%x",r));
  if(r->isIncomplete()){
    m=r->getCurQueue();
    Assert(m!=NULL);
    ret=tcpSend(r->getFD(),m,TRUE);
    if(ret<0) goto writeHerrorBlock;}
  
  if(r->isClosing()){
    messageManager->freeMessageAndMsgBuffer(m);
    return 1;}
  if(r->isWantsToClose()){
    r->closeConnection();
    return 0;}

  while(r->isInWriteQueue()){
    PD((TCP,"taking from write queue %x",r));
    m=site->getWriteQueue();
    Assert(m!=NULL);
    ret=tcpSend(r->getFD(),m,FALSE);
    if(ret<0) goto writeHerrorBlock;}
  
  PD((TCP,"tcpWriteHandler finished r:%x",r));  
  
  OZ_unregisterWrite(fd);
  PD((OS,"unregister WRITE fd:%d",fd));
  if(r->isCanClose()){
    r->clearCanClose();
    r->closeConnection();}
  else
    tcpCache->touch(r);
  return 0;
  
writeHerrorBlock:
  switch(ret){
  case IP_BLOCK:{
    PD((WEIRD,"incomplete write %x",r));
    r->addCurQueue(m);
    tcpCache->touch(r);
    break;}
  case IP_PERM_BLOCK:{
    PD((WEIRD,"Site lost in maybeWrite %x",r));
    r->connectionLost();
    messageManager->freeMessageAndMsgBuffer(m);
    break;}
  default:{
    OZ_warning("Tmp block during send, not handled...");
    break;}}
  return 0;}



static int tcpCloseHandler(int fd,void *r0){
  WriteConnection *r=(WriteConnection *)r0;
  BYTE msg;
  ipReturn ret;
  PD((TCP,"tcpCloseHandler invoked r:%x",r));
  ret=readI(fd,&msg);
  PD((TCP,"tcpCloseHandler read b:%d r:%d e:%d",msg, ret,errno));
close_handler_read:
  if(ret!=IP_OK){ // crashed Connection site 
    PD((ERROR_DET,"crashed Connection site %s error:%d",r->remoteSite->site->stringrep(), errno));
    if(ret==IP_EOF || tcpError()!=IP_TEMP_BLOCK)
      r->connectionLost();
    else
      r->connectionBlocked();
    return 0;}
  if(msg==TCP_MSG_ACK_FROM_READER){
    PD((TCP,"Incomming Ack"));
    r->incommingAck();
    ret=readI(fd,&msg);
    if(ret!=IP_BLOCK)
      goto close_handler_read;
    return 0;}
  if(msg==TCP_CLOSE_REQUEST_FROM_READER){
    PD((TCP,"ClsoeReq from reader"));
    if(!r->isClosing())
      r->setClosing();
    r->remoteSite->siteTmpDwn(HIS_INITIATIVE);
    return 0;}
  if(msg==TCP_CLOSE_ACK_FROM_READER){
    PD((TCP,"CloseAck form reader"));
    r->close(MY_INITIATIVE);
    return 0;}
  if(msg==TCP_RESEND_MESSAGES){
    PD((TCP,"Resend from reader"));
    r->informSiteResendAckQueue();}
  if(r->addByteToInt(msg)){
    ret=readI(fd,&msg);
    if(ret!=IP_BLOCK)
      goto close_handler_read;}
  PD((TCP,"Close Handler done..."));
  return 0;
}

/************************************************************/
/* SECTION ::  Ack-stuff                                        */
/************************************************************/

Bool ReadConnection::informSiteAck(int m, int s){
  if(!isClosing() && remoteSite->receivedNewMsg(m)){
    receivedNewSize(s);
    return TRUE;}
  return FALSE;}

void WriteConnection::informSiteResendAckQueue(){
  int resend = remoteSite->resendAckQueue(sentMsg);
  sentMsgCtr -=  resend;
  if(resend && !isInWriteQueue())
    setInWriteQueue();
  if(isInWriteQueue())
    OZ_registerWriteHandler(getFD(),tcpWriteHandler,(void *)this);
  sentMsg = NULL;}

int RemoteSite::resendAckQueue(Message  *ptr){
  Message *m = ptr;
  int nr = 0;
  while(ptr!=NULL){
    nr++;
    m = ptr;
    ptr = ptr->next;
    m->resend();
    writeQueue.addfirst(m);}
  return nr;}

Bool ReadConnection::resend(){ 
  PD((TCP_INTERFACE,"Resend messages"));
  int fd=getFD();
  Assert(fd!=LOST);
  BYTE msg = TCP_RESEND_MESSAGES;
  if(tcpAckReader(this,remoteSite->getRecMsgCtr()))
    return (writeI(fd,&msg) == IP_OK);
  return FALSE;}

void WriteConnection::ackReceived(int nr){  
    int ctr = sentMsgCtr;
    PD((ACK_QUEUE,"Ack nr:%d received, total sent: %d",nr,ctr));
    PD((ACK_QUEUE,"*** QUEUE CHECKED %d***",checkAckQueue()));
    Message *ptr = sentMsg;
    Message *old;
    if(ctr >= nr && ptr!=NULL){
      if(ctr == nr)
	sentMsg = NULL;
      else{
	while(ctr > nr){
	  PD((ACK_QUEUE,"Stepping through ackQueue cur:%d stop%d",ctr, nr));
	  old = ptr;
	  ptr = ptr->next;
	  if(ptr == NULL) break;
	  ctr = ptr -> msgNum;}
	old->next = NULL;}
      while(ptr!=NULL) {
	PD((ACK_QUEUE,"Removing from queue nr %d %d",
	    ptr->msgNum,ptr));
	    old = ptr;
	    ptr=ptr->next;
	    remoteSite->deQueueMessage(old->getMsgBuffer()->getTotLen());
	    // EK I dont know about this resend....
	    messageManager->freeMessageAndMsgBuffer(old);}}}

Bool RemoteSite::receivedNewMsg(int Nr){
  PD((ACK_QUEUE,"MsgnumReceived new:%d old:%d",Nr,recMsgCtr));
  if (Nr  == recMsgCtr + 1){
    PD((ACK_QUEUE,"Message nr %d accepted",Nr));
    recMsgCtr = Nr;}
  else if(Nr > recMsgCtr){
    PD((ACK_QUEUE,"Message nr %d too large, old %d", Nr, recMsgCtr));
    readConnection->resend();
    return FALSE;}
  return TRUE;}

void RemoteSite::receivedNewAck(int a){
  if(writeConnection!= NULL) {
    PD((ACK_QUEUE,"Ack received: %d waiting for: %d",
	a,writeConnection->getMsgCtr()));
    writeConnection->ackReceived(a);}}

int RemoteSite::getRecMsgCtr(){
  PD((TCP_INTERFACE,"MsgnumGet %d",recMsgCtr));
  if(readConnection != NULL)
    readConnection->messageSent();
  return recMsgCtr;}
int RemoteSite::readRecMsgCtr(){
  return recMsgCtr;}



/************************************************************/
/* SECTION Prm/Tmp-Down routines                            */
/************************************************************/

void RemoteSite::sitePrmDwn(){
  status = SITE_PERM;
  if(writeConnection!=NULL){
    if(writeConnection->getFD() != LOST)
      writeConnection->prmDwn();
    writeConnectionManager->freeConnection(writeConnection);
    writeConnection = NULL;}
  
  if(readConnection!=NULL){
    readConnection->prmDwn();
    readConnectionManager->freeConnection(readConnection);
    readConnection=NULL;}
  Message* m;
  while((m = writeQueue.removeFirst()) && m != NULL) {
    site->communicationProblem(m->msgType, m->site, m->storeIndx,
			       COMM_FAULT_PERM_NOT_SENT,(FaultInfo) m->bs);
    messageManager->freeMessageAndMsgBuffer(m);}
  remoteSiteManager->freeRemoteSite(this);}
  

void RemoteSite::siteTmpDwn(closeInitiator ci){
  if(status ==  SITE_OK) {
    status = SITE_TEMP;
    Message* m = writeQueue.getFirst();
    while(m != NULL) {
      site->communicationProblem(m->msgType, m->site, m->storeIndx,
 				 COMM_FAULT_TEMP_NOT_SENT,
  				 (int) m->bs);
      m = m->getNext();}}
  if(writeConnection!=NULL)
    writeConnection->close(ci);}

void ReadConnection::prmDwn(){
  PD((TCP,"ReadConnection is taken down fd: %d",fd));
  osclose(fd);
  OZ_unregisterRead(fd);
  tcpCache->remove(this);
  if(isIncomplete()) 
    messageManager->freeMessage(getCurQueue());}

void WriteConnection::prmDwn(){
    osclose(fd);
    PD((TCP,"WriteConnection is taken down fd: %d",fd));
    tcpCache->remove(this);
    if(isProbingPrm()){
      remoteSite->site->probeFault(PROBE_PERM);
      clearProbingPrm();}
    
    Message *m = sentMsg;
    sentMsg = NULL;
    while(m != NULL){
      PD((ACK_QUEUE,"Emptying ackqueu m:%x bs: %x",m, m->bs)); 
      m->bs->resend();
      remoteSite->site->communicationProblem(m->msgType, m->site, m->storeIndx,
				 COMM_FAULT_PERM_MAYBE_SENT,(FaultInfo) m->bs);
      Message *tmp = m;
      m = m->next;

      messageManager->freeMessageAndMsgBuffer(tmp);}
        
    if(!isWritePending())
      return;
    if(isInWriteQueue())
      clearInWriteQueue();
    OZ_unregisterWrite(fd);
    
    if(isIncomplete()) {
      clearIncomplete();
      Assert(current != NULL);
      remoteSite->site->communicationProblem(current->msgType, current->site,
					     current->storeIndx,COMM_FAULT_PERM_NOT_SENT,
					     (FaultInfo) current->bs);
      messageManager->freeMessage(current);
      current = NULL;}}

/************************************************************/
/* SECTION :: closing                                       */
/************************************************************/

void WriteConnection::setWantsToClose(){
  PD((TCP_INTERFACE,"setWantsToClose site:%s",remoteSite->site->stringrep()));
  Assert(!testFlag(WANTS_TO_CLOSE));
  Assert(!testFlag(CLOSING));
  setFlag(WANTS_TO_CLOSE);}

void WriteConnection::clearWantsToClose(){
  PD((TCP_INTERFACE,"nowClosing site:%s",remoteSite->site->stringrep()));
  clearFlag(WANTS_TO_CLOSE);
  Assert(!testFlag(CLOSING));}

void WriteConnection::closeConnection(){
  PD((TCP_INTERFACE,"tcpCloseWriter r:%x site: %s",this,remoteSite->site->stringrep()));
  int fd=getFD();
  Assert(fd!=LOST);
  if(isWantsToClose())
    clearWantsToClose();
  if(!isClosing()){
    if(isIncomplete() || isOpening()){
      setWantsToClose();
      return;}
    setClosing();}
  NetMsgBuffer* nb = netMsgBufferManager->
    getNetMsgBuffer(NULL);
  nb->constructMsg(remoteSite,TCP_CLOSE_REQUEST_FROM_WRITER);
  Message* m= messageManager->
    allocMessage(nb, NO_MSG_NUM, NULL,(MessageType) 0, 0);
  switch(isInWriteQueue()?IP_BLOCK:tcpSend(fd,m,TRUE)){
  case IP_OK:{
    PD((TCP,"CloseMsg sent"));
    messageManager->freeMessageAndMsgBuffer(m);
    return;}
  case IP_BLOCK:{
    PD((TCP,"CloseMsg Blocked"));
    addCurQueue(m);
    OZ_registerWriteHandler(fd,tcpWriteHandler,(void *)this);
    return;}
  case IP_PERM_BLOCK:{
    PD((TCP,"CloseMsg nSent sitecrash"));
    messageManager->freeMessageAndMsgBuffer(m);
    connectionLost();
    return;}
  default:  Assert(0);}}

void WriteConnection::close(closeInitiator type){
  PD((TCP_CONNECTIONH,"close r:%x fd:%d type(%d:MY %d:HIS %d:TMP): %d",
      this,fd, MY_INITIATIVE,HIS_INITIATIVE,TMP_INITIATIVE,type));
  if(fd >0)  
    {OZ_unregisterRead(fd);
    OZ_unregisterWrite(fd);
    osclose(fd);}
  if(isIncomplete())
    getCurQueue();  
  tcpCache->remove(this);
  if(isClosing()) clearClosing();
  if(isOpening()) 
    clearOpening();
  if(type != MY_INITIATIVE && isProbingTMP() && 
     !isProbingOK()){
    remoteSite->site->probeFault(PROBE_TEMP); 
    setProbingOK();}
  if(type!=TMP_INITIATIVE) sentMsgCtr=0;
  if(sentMsg!=NULL || isWritePending() || 
     isProbingOK() || isProbingPrm()){
    PD((TCP_CONNECTIONH,"Storing connection"));
    if(type == MY_INITIATIVE)
      setMyInitiative();
    else{
      if(type == HIS_INITIATIVE && 
	 (sentMsg!=NULL || isWritePending())){
	remoteSite->resendAckQueue(sentMsg);
	setInWriteQueue();
	sentMsg = NULL;}
      setTmpDwn();}
    clearFD();
    tcpCache->add(this); 
    return;}
  PD((TCP_CONNECTIONH,"Discarding Connection"));
  informSiteRemove();
  clearFlag(WRITE_CON);
  writeConnectionManager->freeConnection(this);} 

void ReadConnection::closeConnection(){
  BYTE msg;
  Assert(!isClosing());
  int fd=getFD();
  Assert(fd!=LOST);
  PD((TCP_CONNECTIONH,"tcpCloserReader r:%x",this));    
  msg=TCP_CLOSE_REQUEST_FROM_READER;
  if(tcpAckReader(this,remoteSite->getRecMsgCtr())
     && IP_OK ==writeI(fd,&msg))
    setClosing();
  return;}

void ReadConnection::close(){
  PD((TCP_CONNECTIONH,"close ReadConnection r:%x fd:%d",this,fd));
  OZ_unregisterRead(fd);
  osclose(fd);
  clearFD();
  tcpCache->remove(this);
  informSiteRemove();
  if(isIncomplete())
    messageManager->freeMessage(getCurQueue());
  readConnectionManager->freeConnection(this);}



/************************************************************/
/* SECTION :: ????                                          */
/************************************************************/

void RemoteSite::init(Site* s, int msgCtr){
    writeConnection=NULL;
    readConnection=NULL;
    recMsgCtr = msgCtr;
    tmpMsgNum = 1;
    totalMsgSize = 0;
    MsgMonitorSize = -1;
    treashFullCtr = -1;
    site = s;
    tSn = 0;
    status = SITE_OK;
}

void RemoteSite::setWriteConnection(WriteConnection *r){
  Assert(writeConnection==NULL);
  writeConnection=r; 
  r->setSite(this);}

void RemoteSite::setReadConnection(ReadConnection *r){
  Assert(readConnection==NULL);
  readConnection=r; 
  r->setSite(this);}

/************************************************************/
/* SECTION IOQueue methods                                  */
/************************************************************/

void IOQueue::checkQueue(){
  if(first==NULL) {Assert(last==NULL);return;}
  if(first==last) {Assert(first->next==NULL);return;}
  Message *b=first;
  while(b!=last){Assert(b!=NULL);b=b->next;}
  Assert(last->next==NULL);
  return;}

Message* IOQueue::getFirst(){return first;}
Message* IOQueue::removeFirst(){
  if(first == NULL){
    Assert(last == NULL);
    return NULL;}
  Message *t=first;
  first = first -> next;
  if(first == NULL)
    last = NULL;
  return t;
}
void IOQueue::enqueue(Message *m){ 
  Assert(((first==NULL) && (last==NULL))|| 
	 ((first!=NULL) && (last!=NULL)));
  if(last!=NULL){last->next=m;Assert(m->next==NULL);}
  else {Assert(first==NULL);first=m;}
  last=m;
  return;}
void IOQueue::addfirst(Message *m){
  Assert(((first==NULL) && (last==NULL))|| 
	 ((first!=NULL) && (last!=NULL)));
  if(first!=NULL){
    Assert(m->next==NULL);
    m->next=first;}
  else {
    Assert(last==NULL);
    last=m;}
  first=m;
  return;}

void IOQueue::dequeue(Message *m){
  Assert(((first==NULL) && (last==NULL))|| 
	 ((first!=NULL) && (last!=NULL)));
  if(first==m){
    first=m->next; 
    if(last==m) last=NULL;
    m->next=NULL;
    return;}
  Message *b=first;
  while(b->next!=m) {Assert(b!=NULL);b=b->next;}
  b->next=m->next;
  if(b->next==NULL) {Assert(m->next==NULL);last=b;}
  m->next=NULL;
    return;}

/************************************************************/
/* SECTION probes                                           */
/************************************************************/

ProbeReturn RemoteSite::installProbe(ProbeType pt){
  if(siteStatus() == SITE_PERM)
    return PROBE_PERM;
  if(writeConnection==NULL){
    PD((TCP_INTERFACE,"try open %s",site->stringrep()));
    writeConnection=writeConnectionManager->allocConnection(this,LOST);
    if(writeConnection->open() == IP_PERM_BLOCK)
      return PROBE_PERM; 
    PD((TCP,"is reopened %s",site->stringrep()));}
  writeConnection->installProbe(pt);
  if(siteStatus() == SITE_TEMP && pt==PROBE_TYPE_ALL)
    return PROBE_TEMP;
  return PROBE_INSTALLED;}

ProbeReturn RemoteSite::deInstallProbe(ProbeType pt){
  if(siteStatus() == SITE_PERM)
    return PROBE_DEINSTALLED;
  Assert(writeConnection!= NULL);
  writeConnection->deInstallProbe(pt);
  return PROBE_DEINSTALLED;}

void RemoteSite::addWriteQueue(Message* m){
  if(writeConnection)
    writeConnection->setInWriteQueue();
  else
    Assert(status == SITE_TEMP);
  writeQueue.enqueue(m);}

Message* RemoteSite::getWriteQueue(){
  Message* m = writeQueue.removeFirst();
  if(writeQueue.queueEmpty())
    writeConnection->clearInWriteQueue();
  return m;}

void RemoteSite::sendAck(){
  PD((SITE,"Ack Invoked from perdioLayer"));
  if(siteStatus() == SITE_PERM){return;}
  Assert(readConnection != NULL);
  readConnection->sendAck();}

int RemoteSite::
sendTo(NetMsgBuffer *bs, MessageType msg,
       Site *storeS, int storeInd)
{
  int msgNum,ret;
  Message *m=messageManager->allocMessage(bs,NO_MSG_NUM,
storeS,msg,storeInd);
  queueMessage(m->getMsgBuffer()->getTotLen());  
  switch (siteStatus()){
  case SITE_TEMP:{
    PD((TCP_HERROR,"Site Temp, seen before send"));
    bs->beginWrite(this);
    goto tmpdwnsend2;}
  case SITE_PERM:{ 
    PD((TCP_HERROR,"Discovered perm before send"));
    return PERM_NOT_SENT;}
  case SITE_OK:{
    PD((TCP_INTERFACE,"OK"));}}  
  bs->beginWrite(this);
  int fd;
  if(writeConnection==NULL){
    PD((TCP_INTERFACE,"try open %s",site->stringrep()));
    writeConnection=writeConnectionManager->allocConnection(this,LOST);
    fd = writeConnection->open();
    if(fd == IP_OK){ 
      PD((TCP_INTERFACE,"is opened %s",site->stringrep()));
      goto ipBlockSend;}
    if(fd==IP_TEMP_BLOCK){
      PD((TCP_INTERFACE,"is blocked opened %s",site->stringrep()));
      goto tmpdwnsend;}
    PD((TCP_HERROR,"is lost %s", site->stringrep()));
    return PERM_NOT_SENT;}
  else{
    if(!writeConnection->shouldSendFromUser())
      goto ipBlockSend;
    tcpCache->touch(writeConnection);}  
  
  fd=writeConnection->getFD();
  Assert(fd>0);
  switch(tcpSend(fd,m,FALSE)){
  case IP_OK:{
    PD((TCP_INTERFACE,"reliableSend- all sent %d"));
    return ACCEPTED;}
  case IP_BLOCK:{
    PD((TCP_INTERFACE,"reliableSend- part sent"));
    OZ_registerWriteHandler(fd,tcpWriteHandler,
			    (void *)this->writeConnection); 
    writeConnection->addCurQueue(m);
    return ACCEPTED;}
  case IP_TEMP_BLOCK:
    goto tmpdwnsend;
  case IP_PERM_BLOCK:{
    PD((TCP_INTERFACE,"reliableSend- perm failed"));
    site->discoveryPerm();
    return PERM_NOT_SENT;}
  default:{
    Assert(0); 
    return PERM_NOT_SENT;}}
tmpdwnsend:
  PD((TCP_INTERFACE,"TMPDWNSEND1"));
  siteTmpDwn(TMP_INITIATIVE);
tmpdwnsend2:
  PD((TCP_INTERFACE,"TMPDWNSEND2"));
  msgNum = getTmpMsgNum();
  addWriteQueue(m);
  return msgNum;
ipBlockSend:
  PD((TCP_INTERFACE,"sendTo IpBlock add to writeQueue %d",m));
  addWriteQueue(m);

  return ACCEPTED;
}

/**********************************************************************/
/*   SECTION :: exported to the protocol layer              */
/**********************************************************************/

RemoteSite* createRemoteSite(Site* site, int readCtr){
  return remoteSiteManager->allocRemoteSite(site, readCtr);}

void zeroRefsToRemote(RemoteSite *s){s->zeroReferences();}
void nonZeroRefsToRemote(RemoteSite *s){s->nonZeroReferences();} 

int sendTo_RemoteSite(RemoteSite* rs,MsgBuffer* bs,MessageType m,Site* s, int i){
  return rs->sendTo((NetMsgBuffer*)bs,m,s,i);}
void sendAck_RemoteSite(RemoteSite* rs){
  rs->sendAck();}

int discardUnsentMessage_RemoteSite(RemoteSite* s,int msg){
  return s->discardUnsentMessage(msg);}
SiteStatus siteStatus_RemoteSite(RemoteSite* s){
  return s->siteStatus();}

void monitorQueue_RemoteSite(RemoteSite* site,int size){
  site->monitorQueueMsgs(size);}
void demonitorQueue_RemoteSite(RemoteSite* site){
  site->deMonitorQueue();}
int getTreashCtr_RemoteSite(RemoteSite* site){
  return site->getTreashCtr();}
int getQueueStatus_RemoteSite(RemoteSite* s){
  return s->getQueueSize();}


ProbeReturn installProbe_RemoteSite(RemoteSite* site,ProbeType pt, int frequency){
  return site->installProbe(pt);}
ProbeReturn deinstallProbe_RemoteSite(RemoteSite* site,ProbeType pt){
  return site->deInstallProbe(pt);}
ProbeReturn probeStatus_RemoteSite(RemoteSite* site,ProbeType &pt,int &frequncey,void* &storePtr){
  Assert(0);return PROBE_PERM;}


GiveUpReturn giveUp_RemoteSite(RemoteSite* site){
  Assert(0);return GIVES_UP;}


void discoveryPerm_RemoteSite(RemoteSite* site){
  site->sitePrmDwn();} 

void initNetwork(){
  ip_address ip;
  port_t p;
  int tcpFD;
  PD((TCP_INTERFACE,"Init Network"));
  
  writeConnectionManager = new WriteConnectionManager();
  readConnectionManager = new ReadConnectionManager();
  netMsgBufferManager = new NetMsgBufferManager();
  remoteSiteManager = new RemoteSiteManager();
  byteBufferManager = new ByteBufferManager();
  messageManager = new MessageManager();
  tcpCache = new TcpCache();
  tcpOpenMsgBuffer= new TcpOpenMsgBuffer();
  
  ipReturn ret=createTcpPort(OZReadPortNumber,ip,p,tcpFD);
  if (ret<0){
    PD((WEIRD,"timer"));
    Assert(ret==IP_TIMER);
    return;}
  time_t timestamp=time(0);
  mySiteInfo.tcpFD=tcpFD;
  mySiteInfo.maxNrAck = 100;
  mySiteInfo.maxSizeAck = 10000;
  Assert(mySite==NULL);
  mySite=initMySite(ip,p,timestamp);
  Assert(mySite!=NULL);  
  OZ_registerAcceptHandler(tcpFD,acceptHandler,NULL);
  PD((OS,"register ACCEPT- acceptHandler fd:%d",tcpFD));
  tcpCache->nowAccept();}  // can be removed ?? 
  
MsgBuffer* getRemoteMsgBuffer(Site* s){ 
  return netMsgBufferManager->getNetMsgBuffer(s);}

void dumpRemoteMsgBuffer(MsgBuffer *m){
  netMsgBufferManager->dumpNetMsgBuffer((NetMsgBuffer*) m);}
  


/**********************************************************************/
/*   SECTION :: exported for debugging                                */
/**********************************************************************/
int timeCtr = 0;
#define MY_INITIATED_CLOSE 5
#define HIS_INITIATED_CLOSE 10
#define PROBE 20





