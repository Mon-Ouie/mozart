/*
 *  Authors:
 *    Michael Mehl (mehl@dfki.de)
 *    Denys Duchier (duchier@ps.uni-sb.de)
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

#ifdef HAVE_CONFIG_H
#include "conf.h"
#endif

#include "wsock.hh"

#include "codearea.hh"
#include "indexing.hh"

#include "perdio.hh"
#include "perdio_debug.hh"
#include "genvar.hh"
#include "gc.hh"
#include "dictionary.hh"
#include "genhashtbl.hh"
#include "urlc.hh"
#include "marshaler.hh"
#include "comm.hh"
#include "msgbuffer.hh"
#include "builtins.hh"
#include "os.hh"

#ifndef WINDOWS
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#endif
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <strings.h>
#include <netdb.h>

// ATTENTION
#define tcpHeaderSize   7


#include "componentBuffer.cc"


/* ********************************************************************** */
/*              BUILTINS                                                  */
/* ********************************************************************** */

// class ByteSource [Denys Duchier]
// something to marshal from

class ByteSource {
public:
  virtual OZ_Return maybeSkipHeader();
  virtual OZ_Return getBytes(BYTE*,int&,int&);
  virtual OZ_Return getTerm(OZ_Term, const char *compname);
  virtual OZ_Return makeByteStream(ByteStream*&);
  virtual char*     emptyMsg();
};

class ByteSourceFD : public ByteSource {
private:
  int fd;
  Bool del;
public:
  ByteSourceFD(int i,Bool d=FALSE):fd(i),del(d){}
  virtual ~ByteSourceFD(){ if (del) close(fd); }
  char* emptyMsg();
  OZ_Return maybeSkipHeader();
  OZ_Return getBytes(BYTE*,int&,int&);
};

class ByteSourceDatum : public ByteSource {
private:
  OZ_Datum dat;
  Bool     del;
  int      idx;
public:
  ByteSourceDatum():del(FALSE),idx(0){}
  ByteSourceDatum(OZ_Datum d,Bool b):dat(d),del(b),idx(0){}
  virtual ~ByteSourceDatum() {
    if (del) free(dat.data);
  }
  char* emptyMsg();
  OZ_Return maybeSkipHeader();
  OZ_Return getBytes(BYTE*,int&,int&);
};


// class ByteSink [Denys Duchier]
// something to marshal into

class ByteSink {
public:
  virtual OZ_Return putTerm(OZ_Term,char*,Bool text);
  virtual OZ_Return putBytes(BYTE*,int);
  virtual OZ_Return allocateBytes(int);
  virtual OZ_Return maybeSaveHeader(ByteStream*);
};

OZ_Return ByteSink::maybeSaveHeader(ByteStream*){
   Assert(0);
   return PROCEED;}

class ByteSinkFD : public ByteSink {
private:
  int fd;
  Bool del;
public:
  ByteSinkFD(int f,Bool b=FALSE):fd(f),del(b){}
  ByteSinkFD():fd(0),del(FALSE){}
  virtual ~ByteSinkFD() { if (del) close(fd); }
  OZ_Return allocateBytes(int);
  OZ_Return putBytes(BYTE*,int);
};

class ByteSinkFile : public ByteSink {
private:
  char* filename;
  int fd;
public:
  ByteSinkFile(char*s):filename(s),fd(-1){};
  ByteSinkFile():filename(0),fd(-1){};
  virtual ~ByteSinkFile() { if (fd>=0) close(fd); }
  OZ_Return allocateBytes(int);
  OZ_Return putBytes(BYTE*,int);
};

class ByteSinkDatum : public ByteSink {
private:
  int      idx;
public:
  OZ_Datum dat;
  ByteSinkDatum():idx(0){ dat.size=0; dat.data=0; }
  OZ_Return allocateBytes(int);
  OZ_Return putBytes(BYTE*,int);
  OZ_Return maybeSaveHeader(ByteStream*);
};


// ===================================================================
// class ByteSink
// ===================================================================

OZ_Return
ByteSink::putBytes(BYTE*pos,int n)
{
  Assert(0);
  return FAILED;
}

OZ_Return
ByteSink::allocateBytes(int n)
{
  Assert(0);
  return FAILED;
}

OZ_Term makeGenericExc(char *msg, OZ_Term arg)
{
  return OZ_makeException(E_ERROR,OZ_atom("dp"),"generic",2,oz_atom(msg),arg);
}

OZ_Return raiseGeneric(char *msg, OZ_Term arg)
{
  return OZ_raise(makeGenericExc(msg,arg));
}



OZ_Return
ByteSink::putTerm(OZ_Term in, char *filename, Bool textmode)
{
  ByteStream* bs=bufferManager->getByteStream();
  if (textmode)
    bs->setTextmode();
  marshal_M_FILE(bs,PERDIOVERSION,in);

  CheckNogoods(in,bs,"Resources found during save",bufferManager->dumpByteStream(bs));

  bs->beginWrite();
  bs->incPosAfterWrite(tcpHeaderSize);

  int total=bs->calcTotLen();
  allocateBytes(total);
  while (total) {
    Assert(total>0);
    int len=bs->getWriteLen();
    BYTE* pos=bs->getWritePos();
    total -= len;
    OZ_Return result = putBytes(pos,len);
    if (result!=PROCEED) {
      bufferManager->dumpByteStream(bs);
      return result;
    }
    bs->sentFirst();
  }
  bs->writeCheck();
  TaggedRef res = bs->getResources();
  bufferManager->freeByteStream(bs);

  //  return oz_unify(resources,bs->resources);
  if (!oz_isNil(res)) {
    return raiseGeneric("Resources found during save",
                        oz_mklist(OZ_pairA("Resources",res),
                                  OZ_pairA("Filename",oz_atom(filename))));
  }

  return PROCEED;
}

// ===================================================================
// class ByteSinkFD
// ===================================================================

OZ_Return
ByteSinkFD::allocateBytes(int n) { return PROCEED; }

OZ_Return
ByteSinkFD::putBytes(BYTE*pos,int len)
{
 loop:
  if (oswrite(fd,pos,len)<0) {
    if (errno != EINTR)
      return raiseGeneric("Write failed during save",
                          oz_cons(OZ_pairA("Error",oz_atom(OZ_unixError(errno))),
                                oz_nil()));
    goto loop;
  }
  return PROCEED;
}

// ===================================================================
// class ByteSinkFile
// ===================================================================

OZ_Return
ByteSinkFile::allocateBytes(int n)
{
  fd = strcmp(filename,"-")==0 ? STDOUT_FILENO
                               : open(filename,O_WRONLY|O_CREAT|O_TRUNC,0666);
  if (fd < 0)
    return raiseGeneric("Open failed during save",
                        oz_mklist(OZ_pairA("File",oz_atom(filename)),
                                  OZ_pairA("Error",oz_atom(OZ_unixError(errno)))));
  return PROCEED;
}

OZ_Return
ByteSinkFile::putBytes(BYTE*pos,int len)
{
 loop:
  if (oswrite(fd,pos,len)<0) {
    if (errno != EINTR)
      return raiseGeneric("Write failed during save",
                          oz_mklist(OZ_pairA("File",oz_atom(filename)),
                                    OZ_pairA("Error",oz_atom(OZ_unixError(errno)))));
    goto loop;
  }
  return PROCEED;
}

// ===================================================================
// class ByteSinkDatum
// ===================================================================

OZ_Return
ByteSinkDatum::allocateBytes(int n)
{
  dat.size = n;
  dat.data = (char*) malloc(n);
  if (dat.data==0)
    return raiseGeneric("Malloc failed during save",
                        oz_cons(OZ_pairA("Error",oz_atom(OZ_unixError(errno))),
                                oz_nil()));
  return PROCEED;
}

OZ_Return
ByteSinkDatum::putBytes(BYTE*pos,int len)
{
  memcpy(&(dat.data[idx]),pos,len);
  idx += len;
  return PROCEED;
}

OZ_Return
ByteSinkDatum::maybeSaveHeader(ByteStream*)
{
        // saveHeader(bs)
  return PROCEED;
}

OZ_Return
saveDatum(OZ_Term in,OZ_Datum& dat)
{
  ByteSinkDatum sink;
  OZ_Return result = sink.putTerm(in,"filename unknown",NO);
  if (result==PROCEED) {
    dat=sink.dat;
  } else {
    if (sink.dat.data!=0) free(sink.dat.data);
  }
  return result;
}

static
OZ_Return saveIt(OZ_Term val, char *filename, Bool textmode)
{
  ByteSinkFile sink(filename);
  OZ_Return ret = sink.putTerm(val,filename,textmode);
  if (ret!=PROCEED)
    unlink(filename);
  return ret;
}

OZ_BI_define(BIsave,2,0)
{
  OZ_declareIN(0,in);
  OZ_declareVirtualStringIN(1,filename);
  return saveIt(in,filename,NO);
} OZ_BI_end


#ifdef PICKLE2TEXTHACK
OZ_Return loadFD(int fd, OZ_Term out, const char *compname);
Bool pickle2text()
{
  OZ_Term res =   oz_newVariable();
  OZ_Return aux = loadFD(STDIN_FILENO,res,"-");
  if (aux==RAISE) {
    fprintf(stderr,"Exception: %s\n",OZ_toC(am.getExceptionValue(),10,100));
    return NO;
  }
  aux = saveIt(res,"-",OK);
  if (aux==RAISE) {
    fprintf(stderr,"Exception: %s\n",OZ_toC(am.getExceptionValue(),10,100));
    return NO;
  }
  return OK;
}

#endif


OZ_BI_define(BIexport,1,0)
{
  OZ_declareIN(0,in);
  ByteStream* bs=bufferManager->getByteStream();
  marshal_M_EXPORT(bs,in);

  CheckNogoods(in,bs,"Resources found during export",bufferManager->dumpByteStream(bs));

  bufferManager->dumpByteStream(bs);

  return PROCEED;
} OZ_BI_end


// ===================================================================
// class ByteSource
// ===================================================================

OZ_Return
ByteSource::maybeSkipHeader()
{
  Assert(0);
  return FAILED;
}

OZ_Return
ByteSource::getBytes(BYTE*pos,int&max,int&got)
{
  Assert(0);
  return FAILED;
}

char*
ByteSource::emptyMsg()
{
  Assert(0);
  return "emptyByteSource";
}

OZ_Return
ByteSource::getTerm(OZ_Term out, const char *compname)
{
  OZ_Return result = maybeSkipHeader();
  if (result!=PROCEED) return result;
  ByteStream * stream;

  result = makeByteStream(stream);
  if (result!=PROCEED) return result;
  // EK fast fix

  stream->beforeInterpret(0);
  stream->unmarshalBegin();

  char *versiongot = 0;
  OZ_Term val;

  if(stream->skipHeader() && unmarshal_SPEC(stream,versiongot,val)){
    stream->afterInterpret();
    bufferManager->dumpByteStream(stream);
    delete versiongot;
    return oz_unify(val,out);} // mm_u

  bufferManager->dumpByteStream(stream);
  if (versiongot) {
    OZ_Term vergot = oz_atom(versiongot);
    delete versiongot;
    return raiseGeneric("Version mismatch when loading of pickle",
                        oz_mklist(OZ_pairA("File",oz_atom(compname)),
                                  OZ_pairA("Expected",oz_atom(PERDIOVERSION)),
                                  OZ_pairA("Got",vergot)));
  } else {
    return raiseGeneric("Trying to load non-pickle",
                        oz_cons(OZ_pairA("File",oz_atom(compname)),oz_nil()));
  }
}


OZ_Return
ByteSource::makeByteStream(ByteStream*& stream)
{
  stream = bufferManager->getByteStream();
  stream->getSingle();
  int max,got;
  int total = 0;
  BYTE *pos = stream->initForRead(max);
  while (TRUE) {
    OZ_Return result = getBytes(pos,max,got);
    if (result!=PROCEED) return result;
    total += got;
    stream->afterRead(got);
    if (got<max) break;
    pos = stream->beginRead(max);
  }
  if (total==0)
    return raiseGeneric("Empty byte source",oz_nil());

  return PROCEED;
}

// ===================================================================
// class ByteSourceFD
// ===================================================================

char*
ByteSourceFD::emptyMsg() { return "emptyFile"; }

OZ_Return
ByteSourceFD::maybeSkipHeader() {

  return PROCEED;
}

OZ_Return
ByteSourceFD::getBytes(BYTE*pos,int&max,int&got)
{
loop:
  got = osread(fd,pos,max);
  if (got < 0) {
    if (errno==EINTR) goto loop;
    return raiseGeneric("Read error during load",
                        oz_cons(OZ_pairA("Error",oz_atom(OZ_unixError(errno))),
                                oz_nil()));
  }
  return PROCEED;
}

// ===================================================================
// class ByteSourceDatum
// ===================================================================

char*
ByteSourceDatum::emptyMsg() { return "emptyDatum"; }

OZ_Return
ByteSourceDatum::maybeSkipHeader() { return PROCEED; }

OZ_Return
ByteSourceDatum::getBytes(BYTE*pos,int&max,int&got)
{
  if (idx >= dat.size) {
    got = 0;
    return PROCEED;
  }
  got = dat.size - idx;
  if (got >= max) {
    got = max;
  }
  memcpy(pos,&(dat.data[idx]),got);
  idx += got;
  return PROCEED;
}

OZ_Return loadDatum(OZ_Datum dat,OZ_Term out, const char *compname)
{
  ByteSourceDatum src(dat,TRUE);
  return src.getTerm(out,compname);
}

OZ_Return loadFD(int fd, OZ_Term out, const char *compname)
{
  ByteSourceFD src(fd,TRUE);
  return src.getTerm(out,compname);
}

OZ_Return loadFile(char *filename,OZ_Term out)
{
  int fd = strcmp(filename,"-")==0 ? STDIN_FILENO : open(filename,O_RDONLY);
  if (fd < 0) {
    return raiseGeneric("Open failed during load",
                        oz_mklist(OZ_pairA("File",oz_atom(filename)),
                                  OZ_pairA("Error",oz_atom(OZ_unixError(errno)))));
  }
  return loadFD(fd,out,filename);
}


char *newTempFile()
{
  char tn[L_tmpnam] = ""; // I like POSIX!
  ostmpnam(tn);
  return ozstrdup(tn);
}

enum URLAction { URL_LOCALIZE , URL_OPEN , URL_LOAD };

class PipeInfo {
public:
  int fd;
  int pid;
  const char *file;
  char *url;
  TaggedRef controlvar, out;
  URLAction action;

  PipeInfo(int f, int p, const char *tmpf, const char *u, TaggedRef o, TaggedRef var,
           URLAction act):
    fd(f), pid(p), file(tmpf), out(o), action(act)
  {
    controlvar = var;;
    url = ozstrdup(u);
    OZ_protect(&controlvar);
    OZ_protect(&out);
  }
  ~PipeInfo() {
    OZ_unprotect(&controlvar);
    OZ_unprotect(&out);
    delete url;
  }
};

#define ACTION_STRING(act)      \
((act==URL_LOCALIZE)?"localize":\
 (act==URL_OPEN    )?"open":    \
 (act==URL_LOAD    )?"load":    \
 "<unknown action>")


static
void doRaise(TaggedRef controlvar, char *msg, const char *url,URLAction act)
{
  ControlVarRaise(controlvar,
                  makeGenericExc("Error in URL handler",
                                 oz_mklist(OZ_pairA("Message",oz_atom(msg)),
                                           OZ_pairA("Action",oz_atom(ACTION_STRING(act))),
                                           OZ_pairA("URL",oz_atom(url)))));
}

static
int pipeHandler(int, void *arg)
{
  PipeInfo *pi = (PipeInfo *) arg;
  char retloc = 0;
  int n = osread(pi->fd,&retloc,sizeof(retloc));
  osclose(pi->fd);

  TaggedRef controlvar = pi->controlvar;

#ifndef WINDOWS
  int u = waitpid(pi->pid,NULL,0);
  if (u!=pi->pid) {
    doRaise(controlvar,OZ_unixError(errno),pi->url,pi->action);
    // mm2: cleanup missing?
    return NO;
  }
#endif

  if (retloc!=URLC_OK) {
    doRaise(controlvar,urlcStrerror(retloc),pi->url,pi->action);
  } else {

    switch (pi->action) {
    case URL_LOCALIZE:
      // this is only called when the file is remote, therefore
      // a local copy will be made into the file
      ControlVarUnify(controlvar,pi->out,
                      OZ_mkTupleC("new",1,oz_atom(pi->file)));
      break;
    case URL_OPEN:
      {
        int fd = osopen(pi->file,O_RDONLY,0);
        if (fd < 0) {
          doRaise(controlvar,OZ_unixError(errno),pi->url,pi->action);
        } else {
          unlink(pi->file);
          ControlVarUnify(controlvar,pi->out,OZ_int(fd));
        }
        break;
      }
    case URL_LOAD:
      {
        int fd = osopen(pi->file, O_RDONLY,0);
        if (fd < 0) {
          doRaise(controlvar,OZ_unixError(errno),pi->url,pi->action);
        } else {
          OZ_Term other = oz_newVariable();
          OZ_Return aux = loadFD(fd,other,pi->file);
          if (aux==RAISE) {
            ControlVarRaise(controlvar,am.getExceptionValue());
          } else {
            Assert(aux==PROCEED); // mm_u
            unlink(pi->file);
            ControlVarUnify(controlvar,pi->out,other);
          }
        }
        break;
      }
    }
  }

  delete pi->file;
  delete pi;
  return OK;
}

#ifdef WINDOWS

class URLInfo {
public:
  char *tmpfile;
  const char *url;
  int fd;
  URLInfo(const char *file, const char *u, int f):
    tmpfile(ozstrdup(file)), url(ozstrdup(u)), fd(f) {}
  ~URLInfo() {
    delete tmpfile;
    delete url;
  }
};

unsigned __stdcall fetchThread(void *p)
{
  URLInfo *ui = (URLInfo *) p;
  int ret = localizeUrl(ui->url,ui->tmpfile);
  // message("fetchthread(%s,%s)=%d,fd=%d\n",ui->url,ui->tmpfile,ret,ui->fd);
  char buf[2];
  buf[0] = ret;
  buf[1] = '\n';
  oswrite(ui->fd,buf,2);
  osclose(ui->fd);
  delete ui;
  _endthreadex(1);
  return 1;
}

#endif



static
OZ_Return getURL(const char *url, TaggedRef out, URLAction act)
{
  char *tmpfile = newTempFile();
  //  warning("getURL: %s,%s\n",url,tmpfile);

#ifdef WINDOWS

  HANDLE rh,wh;
  CreatePipe(&rh,&wh,0,0);
  int wfd = _hdopen((int)wh,O_WRONLY|O_BINARY);
  int rfd = _hdopen((int)rh,O_RDONLY|O_BINARY);

  URLInfo *ui = new URLInfo(tmpfile,url,wfd);

  unsigned tid;
  HANDLE thrd = (HANDLE) _beginthreadex(NULL,0,&fetchThread,ui,0,&tid);
  if (thrd==NULL)
    return raiseGeneric("getURL: start thread failed",
                        oz_cons(OZ_pairA("URL",oz_atom(url)),oz_nil()));
  int pid = 0;

#else

  int fds[2];
  if (pipe(fds)<0) {
    return raiseGeneric("getURL: system call 'pipe' failed",
                        oz_cons(OZ_pairA("URL",oz_atom(url)),oz_nil()));
  }

  pid_t pid = fork();
  switch(pid) {
  case 0: /* child */
    {
      osclose(fds[0]);
      char ret = (char) localizeUrl(url,tmpfile);
      oswrite(fds[1],&ret,sizeof(ret));
      exit(0);
    }
  case -1:
    return raiseGeneric("getURL: system call 'fork' failed",
                        oz_cons(OZ_pairA("URL",oz_atom(url)),oz_nil()));
  default:
    break;
  }

  osclose(fds[1]);

  int rfd = fds[0];
#endif

  ControlVarNew(var,am.currentBoard());
  PipeInfo *pi = new PipeInfo(rfd,pid,tmpfile,url,out,var,act);
  OZ_registerReadHandler(rfd,pipeHandler,pi);
  SuspendOnControlVar;
}


// URL_get is a primitive that performs an action on a url.
// the action maybe URL_LOCALIZE, URL_OPEN, or URL_LOAD.
// In the case or URL_LOCALIZE, the return value is either:
//      old(PATH)
//              if PATH is the pathname of the original
//              local file
//      new(PATH)
//              if PATH is the pathname of a new local copy
//              of the remote file

#include <ctype.h>
#include <unistd.h>

OZ_Return URL_get(const char*url,OZ_Term& out,URLAction act)
{
#ifdef WINDOWS
  // check for WINDOWS style absolute pathname
  if (isalpha(url[0]) && url[1]==':' && (url[2]=='/' || url[2]=='\\')) {
    goto url_local;
  }
#endif
  if (strncmp(url,"file:",5)==0) { url+=5; goto url_local; }
  {
    const char*s=url;
    while (isalnum(*s)) s++;
    if (*s==':') goto url_remote;
  }
url_local:
  switch (act) {
  case URL_LOCALIZE:
    {
      if (access(url,F_OK)<0) goto kaboom;
      out = OZ_mkTupleC("old",1,oz_atom(url));
      return PROCEED;
    }
  case URL_OPEN:
    {
      int fd = osopen(url,O_RDONLY,0);
      if (fd<0) goto kaboom;
      out = OZ_int(fd);
      return PROCEED;
    }
  case URL_LOAD:
    {
      int fd = osopen(url,O_RDONLY,0);
      if (fd<0) goto kaboom;
      OZ_Term   val    = oz_newVariable();
      OZ_Return status = loadFD(fd,val,url);
      if (status==PROCEED) out=val;
      return status;
    }
  default:
    Assert(0);
    return FAILED;
  }
url_remote:
  out = oz_newVariable();
  return getURL(url,out,act);
kaboom:
  return oz_raise(E_SYSTEM,oz_atom("url"),ACTION_STRING(act),2,
                  oz_atom(OZ_unixError(errno)),
                  oz_atom(url));
}

OZ_BI_define(BIurl_localize,1,1)
{
  OZ_declareVirtualStringIN(0,url);
  return URL_get(url,OZ_out(0),URL_LOCALIZE);
} OZ_BI_end

OZ_BI_define(BIurl_open,1,1)
{
  OZ_declareVirtualStringIN(0,url);
  return URL_get(url,OZ_out(0),URL_OPEN);
} OZ_BI_end

OZ_BI_define(BIurl_load,1,1)
{
  OZ_declareVirtualStringIN(0,url);
  return URL_get(url,OZ_out(0),URL_LOAD);
} OZ_BI_end

OZ_C_proc_begin(BIload,2)
{
  OZ_Term loader = registry_get(AtomLoad);
  if (loader==0)
    loader = BI_url_load;
  am.prepareCall(loader,OZ_getCArg(0),OZ_getCArg(1));
  return BI_REPLACEBICALL;
}
OZ_C_proc_end


OZ_Return OZ_valueToDatum(OZ_Term t, OZ_Datum* d)
{
  return saveDatum(t,*d);
}


OZ_Return OZ_datumToValue(OZ_Datum d,OZ_Term t)
{
  return loadDatum(d,t,"filename unknown");
}


/*************************************************************/
/* Port interface to Gate                                    */
/*************************************************************/
extern int sendPort(OZ_Term port, OZ_Term val);

OZ_BI_define(BIGetPID,0,1)
{
  // pid = pid(host:String port:Int time:Int#Int)

  struct utsname auname;
  if(uname(&auname)<0) { return oz_raise(E_ERROR,E_SYSTEM,"getPidUname",0); }
  struct hostent *hostaddr;
  hostaddr=gethostbyname(auname.nodename);
  struct in_addr tmp;
  memcpy(&tmp,hostaddr->h_addr_list[0],sizeof(in_addr));

  OZ_Term host = oz_pairA("host",oz_string(inet_ntoa(tmp)));
  OZ_Term port = oz_pairA("port",oz_int(mySite->getPort()));
  OZ_Term time =
    oz_pairA("time",
             OZ_pair2(oz_unsignedLong((unsigned long) mySite->getTimeStamp()->start),
                      oz_int(mySite->getTimeStamp()->pid)));
  // NOTE: converting time_t to an unsigned long, maybe a [long] double!

  OZ_Term l = oz_cons(host,oz_cons(port,oz_cons(time,oz_nil())));
  OZ_RETURN(OZ_recordInit(OZ_atom("PID"),l));
} OZ_BI_end

OZ_BI_define(BIReceivedPID,1,0)
{
  oz_declareIN(0,stream);
  return oz_unify(GateStream,stream);
} OZ_BI_end


OZ_BI_define(BITicket2Port,4,1)
{
  oz_declareVirtualStringIN(0,host);
  oz_declareIntIN(1,port);
  oz_declareNonvarIN(2,timeV);
  oz_declareIntIN(3,pid);

  time_t time;
  if (oz_isSmallInt(timeV)) {
    int i = oz_IntToC(timeV);
    if (i <= 0) goto tbomb;
    time = (time_t) i;
  } else if (oz_isBigInt(timeV)) {
    unsigned long i = tagged2BigInt(timeV)->getUnsignedLong();
    if (i==0 && i == OzMaxUnsignedLong) goto tbomb;
    time = (time_t) i;
  } else {
  tbomb:
    return oz_raise(E_ERROR,E_SYSTEM,"PID.send",2,
                    OZ_atom("badTime"),OZ_in(2));
  }

  struct hostent *hostaddr = gethostbyname(host);
  if (!hostaddr) {
    return oz_raise(E_ERROR,E_SYSTEM,"PID.send",2,
                    OZ_atom("gethostbyname"),OZ_in(0));
  }
  struct in_addr tmp;
  memcpy(&tmp,hostaddr->h_addr_list[0],sizeof(in_addr));
  ip_address addr;
  addr = ntohl(tmp.s_addr);
  TimeStamp ts(time,pid);
  Site *site = findSite(addr,port,ts);

  if (!site) {
    return oz_raise(E_ERROR,E_SYSTEM,"Ticket2Port",4,
                    OZ_atom("findSite"),OZ_in(0),OZ_in(1),
                    OZ_in(2));}

  OZ_RETURN(getGatePort(site));
} OZ_BI_end
