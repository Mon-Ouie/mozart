/*
 *  Authors:
 *    Michael Mehl (mehl@dfki.de)
 *    Christian Schulte (schulte@dfki.de)
 * 
 *  Copyright:
 *    Michael Mehl, 1997
 *    Christian Schulte, 1997
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

#include "wsock.hh"

#include "runtime.hh"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#if defined(WINDOWS) && !defined(GNUWIN32)
#include <direct.h>
#else
#include <dirent.h>
#include <netdb.h>
#endif

#if defined(LINUX) || defined(HPUX_700)
extern int h_errno;
#endif

#include <time.h>
#include <sys/stat.h>

#if !defined(OS2_I486) && !defined(WINDOWS)
#include <sys/param.h>
#include <sys/socket.h>
#if !defined(LINUX) && !defined(IRIX6)
#include <sys/uio.h>
#endif
#include <sys/un.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/resource.h>
#include <netinet/in.h>
#endif

#include <signal.h>

#if !defined(WINDOWS) || defined(GNUWIN32)
#include <sys/wait.h>
#include <sys/utsname.h>
#endif

#if defined(GNUWIN32) || defined(LINUX)
#define CHARCAST (char *)
#else
#define CHARCAST
#endif


#ifdef IRIX
#include <bstring.h>
#endif

#if !defined(_MSC_VER) && !defined(GNUWIN32)
extern "C" char *inet_ntoa(struct in_addr in);
#endif

#define max_vs_length 4096*4
#define vs_buff(VAR) char VAR[max_vs_length + 256];

//
// Argument handling
//


#define OZ_C_ioproc_begin(Name,Arity)					\
OZ_C_proc_begin(Name,Arity)						\
  if (!OZ_onToplevel()) {						\
    return oz_raise(E_ERROR,E_KERNEL,"globalState",1,OZ_atom("io"));	\
  }

#define OZ_C_ioproc_end }

// FUCK!!! THIS MUST INSERT A STATEMENT INTO THE BODY
#define OZ_BI_iodefine(Name,InArity,OutArity)				\
OZ_BI_define(Name,InArity,OutArity) {					\
  if (!OZ_onToplevel()) {						\
    return oz_raise(E_ERROR,E_KERNEL,"globalState",1,OZ_atom("io"));	\
  }
#define OZ_BI_ioend } OZ_BI_end

#define OZ_declareVsArg(ARG,VAR)					\
 vs_buff(VAR); OZ_nonvarArg(ARG);					\
 { int len; OZ_Return status; OZ_Term rest, susp;			\
   status = buffer_vs(OZ_getCArg(ARG), VAR, &len, &rest, &susp);	\
   if (status == SUSPEND) {						\
     if (OZ_isVariable(susp)) {						\
       OZ_suspendOn(susp);						\
     } else {								\
       return oz_raise(E_SYSTEM,E_SYSTEM,"limitInternal",1,		\
			OZ_string("virtual string too long"));		\
     }									\
   } else if (status != PROCEED) {					\
     return status;							\
   }									\
   *(VAR+len) = '\0';							\
 }

#define OZ_declareVsIN(ARG,VAR)						\
 vs_buff(VAR); OZ_nonvarIN(ARG);					\
 { int len; OZ_Return status; OZ_Term rest, susp;			\
   status = buffer_vs(OZ_in(ARG), VAR, &len, &rest, &susp);		\
   if (status == SUSPEND) {						\
     if (OZ_isVariable(susp)) {						\
       OZ_suspendOn(susp);						\
     } else {								\
       return oz_raise(E_SYSTEM,E_SYSTEM,"limitInternal",1,		\
			OZ_string("virtual string too long"));		\
     }									\
   } else if (status != PROCEED) {					\
     return status;							\
   }									\
   *(VAR+len) = '\0';							\
 }

#define DeclareAtomListArg(ARG,VAR)					\
OZ_Term VAR = OZ_getCArg(ARG);						\
{ OZ_Term arg = VAR;							\
  while (OZ_isCons(arg)) {						\
    TaggedRef a = OZ_head(arg);						\
    if (OZ_isVariable(a)) OZ_suspendOn(a);				\
    if (!OZ_isAtom(a))    return OZ_typeError(ARG,"list(Atom)");	\
    arg = OZ_tail(arg);							\
  }									\
  if (OZ_isVariable(arg)) OZ_suspendOn(arg);				\
  if (!OZ_isNil(arg))     return OZ_typeError(ARG,"list(Atom)");	\
}

#define DeclareAtomListIN(ARG,VAR)					\
OZ_Term VAR = OZ_in(ARG);						\
{ OZ_Term arg = VAR;							\
  while (OZ_isCons(arg)) {						\
    TaggedRef a = OZ_head(arg);						\
    if (OZ_isVariable(a)) OZ_suspendOn(a);				\
    if (!OZ_isAtom(a))    return OZ_typeError(ARG,"list(Atom)");	\
    arg = OZ_tail(arg);							\
  }									\
  if (OZ_isVariable(arg)) OZ_suspendOn(arg);				\
  if (!OZ_isNil(arg))     return OZ_typeError(ARG,"list(Atom)");	\
}

#define DeclareNonvarArg(ARG,VAR) \
  OZ_Term VAR = OZ_getCArg(ARG);    \
  OZ_nonvarArg(ARG);

#define DeclareNonvarIN(ARG,VAR) \
  OZ_Term VAR = OZ_in(ARG);    \
  OZ_nonvarIN(ARG);

#define IsPair(s) (s[0]=='#' && s[1]=='\0')



// checking

inline 
int unixIsCons(OZ_Term list, OZ_Term *hd, OZ_Term *tl) { 
  if (!OZ_isCons(list)) {
    return 0;
  }
  *hd = OZ_head(list);
  *tl = OZ_tail(list);
  return 1;
}


// -------------------------------------------------
// errors
// -------------------------------------------------


#define WRAPCALL(CALL, RET) \
int RET;                                     \
while ((RET = CALL) < 0) {                   \
  if (ossockerrno() != EINTR) { RETURN_UNIX_ERROR; } \
}


// -------------------------------------------------
// specification of returning
// -------------------------------------------------

int raiseUnixError(int n, char * e, char * g) {
  return oz_raise(E_SYSTEM,E_OS, g, 2, OZ_int(n), OZ_string(e)); 
}

// return upon unix-error
#define RETURN_UNIX_ERROR \
{ return raiseUnixError(ossockerrno(), OZ_unixError(ossockerrno()), "os"); }


#if defined(ULTRIX_MIPS) || defined(OS2_I486)

#define RETURN_NET_ERROR \
{ return raiseUnixError(0, "Host lookup failure.", "host"); }

#else

static char* h_strerror(const int err) {
  switch (err) {
  case HOST_NOT_FOUND:
    return "No such host is known.";
  case TRY_AGAIN:
    return "Retry later again.";
  case NO_RECOVERY:
    return "Unexpected non-recoverable server failure.";
#if defined(SOLARIS) || defined(LINUX)
  case NO_ADDRESS:
#endif
#if defined(SUNOS_SPARC)
  case NO_DATA:
#endif
    return "No internet address.";
  default:
    return "Hostname lookup failure.";
  }
}

#define RETURN_NET_ERROR \
{ return raiseUnixError(h_errno, h_strerror(h_errno), "host"); }


#endif


// return suspension upon
#define RETURN_SUSPEND(OUT,LEN,VAR,REST)       \
{ OZ_Term susp_tuple = OZ_tupleC("suspend",3); \
  OZ_putArg(susp_tuple,0,LEN);                 \
  OZ_putArg(susp_tuple,1,VAR);                 \
  OZ_putArg(susp_tuple,2,REST);                \
  return OZ_unify(OUT,susp_tuple);             \
}

#define NEW_RETURN_SUSPEND(LEN,VAR,REST)       \
{ OZ_Term susp_tuple = OZ_tupleC("suspend",3); \
  OZ_putArg(susp_tuple,0,LEN);                 \
  OZ_putArg(susp_tuple,1,VAR);                 \
  OZ_putArg(susp_tuple,2,REST);                \
  OZ_RETURN(susp_tuple);             	       \
}



// -------------------------------------------------
// check file descriptors
// -------------------------------------------------

#define CHECK_READ(FD) \
{ int sel = osTestSelect(FD,SEL_READ);                           \
  if (sel < 0)  { RETURN_UNIX_ERROR; }	                         \
  if (sel == 0) {                                                \
    TaggedRef t = oz_newVariable(); \
    (void) OZ_readSelect(FD, NameUnit, t);                       \
    DEREF(t, t_ptr, t_tag);                                      \
    if (isAnyVar(t_tag)) {                                       \
      am.addSuspendVarList(t_ptr);                               \
      return SUSPEND;                                            \
    }                                                            \
  }                                                              \
}
                             
#define CHECK_WRITE(FD) \
{ int sel = osTestSelect(FD,SEL_WRITE);                          \
  if (sel < 0)  { RETURN_UNIX_ERROR; }	                         \
  if (sel == 0) {                                                \
    TaggedRef t = oz_newVariable(); \
    (void) OZ_writeSelect(FD, NameUnit, t);                      \
    DEREF(t, t_ptr, t_tag);                                      \
    if (isAnyVar(t_tag)) {                                       \
      am.addSuspendVarList(t_ptr);                               \
      return SUSPEND;                                            \
    }                                                            \
  }                                                              \
}

static OZ_Term openbuff2list(int len, const char *s, const OZ_Term tl) {
  // gives back a list of length len which elments are taken from a C-string
  // the tail of the list is given by list
  OZ_Term prev, hd;

  if (len == 0)
    return tl;

  hd = OZ_tupleC("|", 2);
  OZ_putArg(hd, 0, OZ_int((unsigned char) *s++));
  prev = hd;
  
  while (--len) {
    OZ_Term next = OZ_tupleC("|", 2); 

    OZ_putArg(next, 0, OZ_int((unsigned char) *s++));
    OZ_putArg(prev, 1, next);  
    prev = next;
  }

  OZ_putArg(prev, 1, tl);
  return hd;
}


//
// Handling of virtual strings
//

inline OZ_Term buff2list(int len, const char *s) 
{
  return openbuff2list(len, s, nil()); 
}



OZ_Return atom2buff(OZ_Term atom, char **write_buff, int *len, 
		  OZ_Term *rest, OZ_Term *susp)
{
  char c;
  
  if (!OZ_isAtom(atom)) {
    return OZ_typeError(-1,"VirtualString");
  }

  const char *string = OZ_atomToC(atom);

  if (IsPair(string))
    return PROCEED;
  
  while ((c = *string) &&
	 *len < max_vs_length) {
    **write_buff = c;
    (*write_buff)++;
    (*len)++;
    string++;
  }

  if (*len == max_vs_length && c) {
    *susp = OZ_string(string);
    *rest = *susp;
    return SUSPEND;
  }

  return PROCEED;
}
    

OZ_Return int2buff(OZ_Term ozint, char **write_buff, int *len,
		 OZ_Term *rest, OZ_Term *susp)
{
  char *string = OZ_toC(ozint,0,0);
  if (*string == '~') *string='-';
  char c;
  
  char *help = string;
  while ((c = *help) &&
	 *len < max_vs_length) {
    **write_buff = c;
    (*write_buff)++;
    (*len)++;
    help++;
  }

  if (*len == max_vs_length && c) {
    *susp = OZ_string(help);
    *rest = *susp;
    return SUSPEND;
  }

  return PROCEED;
}

OZ_Return float2buff(OZ_Term ozfloat, char **write_buff, int *len,
		   OZ_Term *rest, OZ_Term *susp)
{
  char *string = OZ_toC(ozfloat,0,0);
  for (char *p=string; *p; p++) {
    if (*p == '~') *p='-';
  }
  char c;
  
  char *help = string;
  while ((c = *help) &&
	 *len < max_vs_length) {
    **write_buff = c;
    (*write_buff)++;
    (*len)++;
    help++;
  }

  if (*len == max_vs_length && c) {
    *susp = OZ_string(help);
    *rest = *susp;
    return SUSPEND;
  }

  return PROCEED;
}


OZ_Return list2buff(OZ_Term list, char **write_buff, int *len,
		  OZ_Term *rest, OZ_Term *susp)
{
  OZ_Term hd, tl;

  while (unixIsCons(list, &hd, &tl)) {
    if ((*len == max_vs_length) || OZ_isVariable(hd)) {
      *susp = hd;
      *rest = list;
      return SUSPEND;
    }

    int c;

    if (OZ_isInt(hd)) {
      c = OZ_intToC(hd);
      if ((c >= 0) && (c < 256)) {
	**write_buff = (unsigned char) c;
	(*write_buff)++;
	(*len)++;
	list = tl;
	continue;
      }
    }
    return OZ_typeError(-1,"VirtualString");
  }

  if (OZ_isVariable(list)) {
    *susp = list;
    *rest = list;
    return SUSPEND;
  }

  if (OZ_isNil(list))
    return PROCEED;


  return OZ_typeError(-1,"VirtualString");
}


static OZ_Return vs2buff(OZ_Term vs, char **write_buff, int *len,
                       OZ_Term *rest, OZ_Term *susp)
{
  if (OZ_isAtom(vs)) {
    return OZ_isNil(vs) ? PROCEED : atom2buff(vs, write_buff, len, rest, susp);
  }

  const char *label = NULL;
  if (OZ_isTuple(vs) && (label = OZ_atomToC(OZ_label(vs)))) {
    int width = OZ_width(vs);
    if (IsPair(label) && width > 0) {
      OZ_Term arg_susp, arg_rest;

      for (int i=0; i<width; i++) {

        OZ_Return status = vs2buff(OZ_getArg(vs,i), write_buff, len,
                                 &arg_rest, &arg_susp);
        if (status == SUSPEND) {
          *susp = arg_susp;

          if (i==width-1) {
            *rest = arg_rest;
          } else {
            *rest = OZ_tupleC("#", (width - i));
            
            OZ_putArg(*rest, 0, arg_rest);
            i++;
            for (int j=1 ; i < width ; (j++, i++)) {
              OZ_putArg(*rest, j, OZ_getArg(vs, i));
            }
          }
          return SUSPEND;
        } else if (status != PROCEED) {
          return status;
        }
            
      }

      return PROCEED;
      
    } else if (label[0] == '|' && label[1] == '\0' && width == 2) {
      return list2buff(vs, write_buff, len, rest, susp);
    }
    return OZ_typeError(-1,"VirtualString");
  }

  if (OZ_isInt(vs)) {
    return int2buff(vs, write_buff, len, rest, susp);
  }

  if (OZ_isFloat(vs)) {
    return float2buff(vs, write_buff, len, rest, susp);
  }

  if (OZ_isVariable(vs)) {
    *rest = vs;
    *susp = vs;
    return SUSPEND;
  }

  return OZ_typeError(-1,"VirtualString");
}


inline OZ_Return buffer_vs(OZ_Term vs, char *write_buff, int *len,
                         OZ_Term *rest, OZ_Term *susp)
{
  *len = 0;
  return vs2buff(vs, &write_buff, len, rest, susp);
}


// -------------------------------------------------
// unix IO
// -------------------------------------------------

#ifndef _MSC_VER
OZ_BI_iodefine(unix_fileDesc,1,1)
{
  OZ_declareAtomIN( 0, OzFileDesc);
  
  int desc;
  if (!strcmp(OzFileDesc,"STDIN_FILENO")) {
    desc=osdup(STDIN_FILENO);
  } else if (!strcmp(OzFileDesc,"STDOUT_FILENO")) {
    desc=osdup(STDOUT_FILENO);    
  } else if (!strcmp(OzFileDesc,"STDERR_FILENO")) {
    desc=osdup(STDERR_FILENO);
  } else {
    return OZ_typeError(0,"enum(STDIN_FILENO STDOUT_FILENO STDERR_FILENO)");
  }

  OZ_RETURN_INT(desc);
} OZ_BI_ioend


static OZ_Term readEntries(DIR *dp) {
  static struct dirent *dirp;
  OZ_Term dirEntry;
  if ((dirp = readdir(dp)) != NULL) {
    dirEntry = OZ_string(dirp->d_name);
    return cons(dirEntry, readEntries(dp));
  }
  else 
    return nil();
}

OZ_BI_iodefine(unix_getDir,1,1)
{
  DIR *dp;
  OZ_Term dirValue;
  OZ_declareVsIN(0, path);

  if ((dp = opendir(path)) == NULL)
    RETURN_UNIX_ERROR;

  dirValue = readEntries(dp);

  if (closedir(dp) < 0)
    RETURN_UNIX_ERROR;

  OZ_RETURN(dirValue);
} OZ_BI_ioend
#endif


OZ_BI_iodefine(unix_stat,1,1)
{
  struct stat buf;
  char *fileType;
  OZ_declareVsIN(0, filename);

  if (stat(filename, &buf) < 0)
    RETURN_UNIX_ERROR;

#ifndef _MSC_VER
  if      (S_ISREG(buf.st_mode))  fileType = "reg";
  else if (S_ISDIR(buf.st_mode))  fileType = "dir";
  else if (S_ISCHR(buf.st_mode))  fileType = "chr";
  else if (S_ISBLK(buf.st_mode))  fileType = "blk";
  else if (S_ISFIFO(buf.st_mode)) fileType = "fifo";
  else 
#endif
    fileType = "unknown";

  OZ_Term pairlist=
    cons(OZ_pairAA("type",fileType),
            cons(OZ_pairAI("size",buf.st_size),
		 cons(OZ_pairAI("mtime",buf.st_mtime),
                    nil())));
  OZ_RETURN(OZ_recordInit(OZ_atom("stat"),pairlist));
} OZ_BI_ioend

#if !defined(WINDOWS) || defined(GNUWIN32)
OZ_BI_iodefine(unix_uName,0,1)
{
  struct utsname buf;
  if (uname(&buf) < 0)
    RETURN_UNIX_ERROR;

  OZ_Term t2=OZ_pairAS("machine",buf.machine);
  OZ_Term t3=OZ_pairAS("nodename",buf.nodename);
  OZ_Term t4=OZ_pairAS("release",buf.release);
  OZ_Term t5=OZ_pairAS("sysname",buf.sysname);
  OZ_Term t6=OZ_pairAS("version",buf.version);

  OZ_Term pairlist = cons(t2,cons(t3,cons(t4,cons(t5,cons(t6,nil())))));

#if defined(SUNOS_SPARC) || defined(LINUX)

#ifdef SUNOS_SPARC
  char dname[65];
  if (getdomainname(dname, 65)) {
    RETURN_UNIX_ERROR;
  }
#else
  char * dname;
  dname = buf.domainname;
#endif

  pairlist = cons(OZ_pairAS("domainname",dname),pairlist);

#endif

  OZ_RETURN(OZ_recordInit(OZ_atom("utsname"),pairlist));

} OZ_BI_ioend
#endif


OZ_BI_iodefine(unix_getCWD,0,1)
{
  const int SIZE=256;
  char buf[SIZE];
  if (getcwd(buf,SIZE)) OZ_RETURN_ATOM(buf);
  if (errno != ERANGE) RETURN_UNIX_ERROR;

  int size=SIZE+SIZE;
  char *bigBuf;
  while (OK) {
    bigBuf=(char *) malloc(size);
    if (getcwd(bigBuf,size)) {
      OZ_Term res = oz_atom(bigBuf); // bug fix, was still using buf
      free(bigBuf);
      OZ_RETURN(res);
    }
    if (errno != ERANGE) RETURN_UNIX_ERROR;
    free(bigBuf);
    size+=SIZE;
  }
} OZ_BI_ioend

#if defined(WINDOWS) && !defined(GNUWIN32)
#define O_NOCTTY   0
#define O_NONBLOCK 0
#define O_SYNC     0
#endif

#ifndef O_SYNC
#define O_SYNC     0
#endif


OZ_BI_iodefine(unix_open,3,1)
{
  OZ_declareVsIN(0, filename);
  DeclareAtomListIN(1, OzFlags);
  DeclareAtomListIN(2, OzMode);

  // Compute flags from their textual representation

  int flags = 0;
  OZ_Term hd, tl;
  
  while (unixIsCons(OzFlags, &hd, &tl)) {

    if (OZ_isVariable(hd)) return SUSPEND;

    if (OZ_unifyAtom(hd,"O_RDONLY") == PROCEED) {
      flags |= O_RDONLY;
    } else if (OZ_unifyAtom(hd,"O_WRONLY"  ) == PROCEED) {
      flags |= O_WRONLY;
    } else if (OZ_unifyAtom(hd,"O_RDWR"    ) == PROCEED) {
      flags |= O_RDWR;
    } else if (OZ_unifyAtom(hd,"O_APPEND"  ) == PROCEED) {
      flags |= O_APPEND;
    } else if (OZ_unifyAtom(hd,"O_CREAT"   ) == PROCEED) {
      flags |= O_CREAT;
    } else if (OZ_unifyAtom(hd,"O_EXCL"    ) == PROCEED) {
      flags |= O_EXCL;
    } else if (OZ_unifyAtom(hd,"O_TRUNC"   ) == PROCEED) {
      flags |= O_TRUNC;
    } else if (OZ_unifyAtom(hd,"O_NOCTTY"  ) == PROCEED) {
      flags |= O_NOCTTY;
    } else if (OZ_unifyAtom(hd,"O_NONBLOCK") == PROCEED) {
      flags |= O_NONBLOCK;
    } else if (OZ_unifyAtom(hd,"O_SYNC"    ) == PROCEED) {
      flags |= O_SYNC;
    } else {
      return OZ_typeError(1,"enum openFlags");
    }

    OzFlags = tl;
  }

  if (OZ_isVariable(OzFlags)) {
    return SUSPEND;
  } else if (!OZ_isNil(OzFlags)) {
    return OZ_typeError(1,"enum openFlags");
  }

  // Compute modes from their textual representation

  int mode = 0;
  while (unixIsCons(OzMode, &hd, &tl)) {

    if (OZ_isVariable(hd))
      return SUSPEND;
#ifdef OS2_I486
    return OZ_typeError(2,"enum openMode");
#else
#ifndef _MSC_VER
    if (OZ_unifyAtom(hd,"S_IRUSR") == PROCEED) { mode |= S_IRUSR; }
    else if (OZ_unifyAtom(hd,"S_IWUSR") == PROCEED) { mode |= S_IWUSR; }
    else if (OZ_unifyAtom(hd,"S_IXUSR") == PROCEED) { mode |= S_IXUSR; }
    else if (OZ_unifyAtom(hd,"S_IRGRP") == PROCEED) { mode |= S_IRGRP; }
    else if (OZ_unifyAtom(hd,"S_IWGRP") == PROCEED) { mode |= S_IWGRP; }
    else if (OZ_unifyAtom(hd,"S_IXGRP") == PROCEED) { mode |= S_IXGRP; }
    else if (OZ_unifyAtom(hd,"S_IROTH") == PROCEED) { mode |= S_IROTH; }
    else if (OZ_unifyAtom(hd,"S_IWOTH") == PROCEED) { mode |= S_IWOTH; }
    else if (OZ_unifyAtom(hd,"S_IXOTH") == PROCEED) { mode |= S_IXOTH; }
    else
#endif
      return OZ_typeError(2,"enum openMode");
#endif
    OzMode = tl;
  }

  if (OZ_isVariable(OzMode)) {
    return SUSPEND;
  } else if (!OZ_isNil(OzMode)) {
    return OZ_typeError(2,"enum openMode");
  }

  WRAPCALL(osopen(filename, flags, mode),desc);

  OZ_RETURN_INT(desc);
} OZ_BI_ioend



OZ_BI_iodefine(unix_close,1,0)
{
  OZ_declareIntIN(0,fd);

  WRAPCALL(osclose(fd),ret);

  return PROCEED;
} OZ_BI_ioend


OZ_BI_iodefine(unix_read,5,0)
{ 
  OZ_declareIntIN(0,fd);
  OZ_declareIntIN(1,maxx);
  OZ_declareIN(2, outHead);
  OZ_declareIN(3, outTail);
  OZ_declareIN(4, outN);

  CHECK_READ(fd);

  char *buf = (char *) malloc(maxx+1);

  WRAPCALL(osread(fd, buf, maxx), ret);

  OZ_Term hd = openbuff2list(ret, buf, outTail);

  free(buf);
  
  return ((OZ_unify(outHead, hd) == PROCEED)&&
          (OZ_unifyInt(outN,ret) == PROCEED)) ? PROCEED : FAILED;
} OZ_BI_ioend



OZ_BI_iodefine(unix_write, 2,1)
{
  OZ_declareIntIN(0, fd);
  DeclareNonvarIN(1, vs);

  CHECK_WRITE(fd);

  { 
    int len;
    OZ_Return status;
    OZ_Term rest, susp;
    vs_buff(write_buff);

    status = buffer_vs(vs, write_buff, &len, &rest, &susp);

    if (status != PROCEED && status != SUSPEND)
      return status;
  
    WRAPCALL(oswrite(fd, write_buff, len), ret);
    
    if (status == PROCEED) {
      if (len == ret) {
	OZ_RETURN_INT(len);
      } else {
	Assert(len > ret);
	NEW_RETURN_SUSPEND(OZ_int(ret), nil(), rest);
      }
    } else {
      Assert(status == SUSPEND);
      if (len == ret) {
	NEW_RETURN_SUSPEND(OZ_int(ret), susp, rest);
      } else {
	Assert(len > ret);
	NEW_RETURN_SUSPEND(OZ_int(ret), susp, 
		       OZ_pair2(buff2list(len - ret, write_buff + ret), rest));
      }
    }
  }
} OZ_BI_ioend


OZ_BI_iodefine(unix_lSeek,3,1) {
  OZ_declareIntIN(0, fd);
  OZ_declareIntIN(1, offset);
  OZ_declareAtomIN(2, OzWhence);

  int whence;
  
  if (!strcmp(OzWhence,"SEEK_SET")) {
    whence=SEEK_SET;
  } else if (!strcmp(OzWhence,"SEEK_CUR")) {
    whence=SEEK_CUR;    
  } else if (!strcmp(OzWhence,"SEEK_END")) {
    whence=SEEK_END;
  } else {
    return OZ_typeError(2,"enum(SEEK_CUR SEEK_END)");
  }
    
  WRAPCALL(lseek(fd, offset, whence),ret);

  OZ_RETURN_INT(ret);
} OZ_BI_ioend


OZ_BI_iodefine(unix_readSelect, 1,0) {
  OZ_declareIntIN(0,fd);

  WRAPCALL(osTestSelect(fd,SEL_READ),sel);

  if (sel == 0) {
    TaggedRef t = oz_newVariable();

    (void) OZ_readSelect(fd, NameUnit, t);
    DEREF(t, t_ptr, t_tag);
    
    if (isAnyVar(t_tag)) {
      am.addSuspendVarList(t_ptr);
      return SUSPEND;
    }
  }

  return PROCEED;
} OZ_BI_ioend


OZ_BI_iodefine(unix_writeSelect,1,0) {
  OZ_declareIntIN(0,fd);

  WRAPCALL(osTestSelect(fd,SEL_WRITE),sel);

  if (sel == 0) {
    TaggedRef t = oz_newVariable();
    
    (void) OZ_writeSelect(fd, NameUnit, t);
    DEREF(t, t_ptr, t_tag);
    
    if (isAnyVar(t_tag)) {
      am.addSuspendVarList(t_ptr);
      return SUSPEND;
    }
  }

  return PROCEED;
} OZ_BI_ioend


OZ_BI_iodefine(unix_acceptSelect,1,0) {
  OZ_declareIntIN(0,fd);

  WRAPCALL(osTestSelect(fd,SEL_READ),sel);

  if (sel == 0) {

    TaggedRef t = oz_newVariable();
    
    (void) OZ_acceptSelect(fd, NameUnit, t);
    DEREF(t, t_ptr, t_tag);
    
    if (isAnyVar(t_tag)) {
      am.addSuspendVarList(t_ptr);
      return SUSPEND;
    }
  }

  return PROCEED;
} OZ_BI_ioend




OZ_BI_define(unix_deSelect,1,0) {
  OZ_declareIntIN(0,fd);
  OZ_deSelect(fd);
  return PROCEED;
} OZ_BI_end




// -------------------------------------------------
// sockets
// -------------------------------------------------
OZ_BI_iodefine(unix_socket,3,1)
{
  OZ_declareAtomIN(0, OzDomain);
  OZ_declareAtomIN(1, OzType);
  OZ_declareVsIN(2, OzProtocol);

  int domain, type, protocol;

  // compute domain
#ifndef WINDOWS
  if (!strcmp(OzDomain,"PF_UNIX")) {
    domain = PF_UNIX;
  } else 
#endif
  if (!strcmp(OzDomain,"PF_INET")) {
    domain = PF_INET;
  } else {
    return OZ_typeError(0,"enum(PF_UNIX PF_INET)");
  }

  // compute type
  if (!strcmp(OzType,"SOCK_STREAM")) {
    type = SOCK_STREAM;
  } else if (!strcmp(OzType,"SOCK_DGRAM")) {
    type = SOCK_DGRAM;
  } else {
    return OZ_typeError(1,"enum(SOCK_STREAM SOCK_DGRAM)");
  }

  // compute protocol   
  if (*OzProtocol != '\0') {
    struct protoent *proto;

    proto = getprotobyname(OzProtocol);
    
    if (!proto) {
      return OZ_typeError(0,"enum protocol");
    }

    protocol = proto->p_proto;
  } else {
    protocol = 0;
  }

  WRAPCALL(ossocket(domain, type, protocol), sock);

  OZ_RETURN_INT(sock);
} OZ_BI_ioend

OZ_BI_iodefine(unix_bindInet,2,0)
{
  OZ_declareIntIN(0,sock);
  OZ_declareIntIN(1,port);

  struct sockaddr_in addr;

  memset((char *)&addr, 0, sizeof (addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons ((unsigned short) port);

  WRAPCALL(bind(sock,(struct sockaddr *)&addr,sizeof(struct
                                                     sockaddr_in)),ret);
  return PROCEED;
} OZ_BI_ioend


OZ_BI_define(unix_getSockName,1,1)
{
  OZ_declareIntIN(0,s);

  struct sockaddr_in addr;
  int length = sizeof(addr);

  WRAPCALL(getsockname(s, (struct sockaddr *) &addr, &length), ret);

  OZ_RETURN_INT(ntohs(addr.sin_port));
} OZ_BI_end


OZ_BI_iodefine(unix_listen,2,0)
{
  OZ_declareIntIN(0, s);
  OZ_declareIntIN(1, n);

  WRAPCALL(listen(s,n), ret);

  return PROCEED;
} OZ_BI_ioend


OZ_BI_define(unix_connectInet,3,0)
{
  OZ_declareIntIN(0, s);
  OZ_declareVsIN(1, host);
  OZ_declareIntIN(2, port);

  struct hostent *hostaddr;

  if ((hostaddr = gethostbyname(host)) == NULL) {
    RETURN_NET_ERROR;
  }

  struct sockaddr_in addr;
  memset((char *)&addr, 0, sizeof (addr));
  addr.sin_family = AF_INET;
  memcpy(&addr.sin_addr,hostaddr->h_addr_list[0],sizeof(addr.sin_addr));
  addr.sin_port = htons ((unsigned short) port);

  int ret = osconnect(s,(struct sockaddr *) &addr,sizeof(addr));
  if (ret<0) {
    Assert(errno != EINTR);
    RETURN_UNIX_ERROR;
  }

  return PROCEED;
} OZ_BI_end

OZ_BI_iodefine(unix_acceptInet,1,3)
{
  OZ_declareIntIN(0, sock);
  // OZ_out(0) == host
  // OZ_out(1) == port
  // OZ_out(2) == fd

  struct sockaddr_in from;
  int fromlen = sizeof from;

  WRAPCALL(osaccept(sock,(struct sockaddr *)&from, &fromlen),fd);

  struct hostent *gethost = gethostbyaddr((char *) &from.sin_addr,
                                          fromlen, AF_INET);
  if (gethost) {
    OZ_out(0) = OZ_int(ntohs(from.sin_port));
    OZ_out(1) = OZ_string((char*)gethost->h_name);
    OZ_out(2) = OZ_int(fd);
    return PROCEED;
  } else {
    OZ_out(0) = OZ_int(ntohs(from.sin_port));
    OZ_out(1) = OZ_string(inet_ntoa(from.sin_addr));
    OZ_out(2) = OZ_int(fd);
    return PROCEED;
  }
} OZ_BI_ioend


static OZ_Return get_send_recv_flags(OZ_Term OzFlags, int * flags)
{
  OZ_Term hd, tl;

  *flags = 0;
  
  while (unixIsCons(OzFlags, &hd, &tl)) {

    if (OZ_isVariable(hd))
      return SUSPEND;

    if (OZ_unifyAtom(hd,"MSG_OOB") == PROCEED) {
      *flags |= MSG_OOB;
    } else if (OZ_unifyAtom(hd,"MSG_PEEK") == PROCEED) {
      *flags |= MSG_PEEK;
    } else {
      return OZ_typeError(-1,"enum(MSG_OOB MSG_PEEK)");
    }

    OzFlags = tl;
  }

  if (OZ_isVariable(OzFlags))
    return SUSPEND;

  if (!OZ_isNil(OzFlags)) {
    return OZ_typeError(-1,"enum(MSG_OOB MSG_PEEK)");
  }

  return PROCEED;
}


OZ_BI_iodefine(unix_send, 3,1)
{
  OZ_declareIntIN(0, sock);
  DeclareNonvarIN(1, vs);
  DeclareAtomListIN(2, OzFlags);


  int flags;
  OZ_Return flagBool;
  
  if (!((flagBool = get_send_recv_flags(OzFlags,&flags)) == PROCEED))
      return flagBool;

  CHECK_WRITE(sock);

  {
    int len;
    OZ_Return status;
    OZ_Term rest, susp, from_buff, rest_all;
    vs_buff(write_buff);
    
    status = buffer_vs(vs, write_buff, &len, &rest, &susp);
    
    if (status != PROCEED && status != SUSPEND)
      return status;
    
    WRAPCALL(send(sock, write_buff, len, flags), ret);
    
    if (len==ret && status != SUSPEND) {
      OZ_RETURN_INT(len);
    }
    
    if (status != SUSPEND) {
      susp = nil();
      rest = susp;
    }
    
    if (len > ret) {
      from_buff = buff2list(len - ret, write_buff + ret);
      
      rest_all = OZ_pair2(from_buff,rest);
      
      NEW_RETURN_SUSPEND(OZ_int(ret),susp,rest_all);
    } else {
      NEW_RETURN_SUSPEND(OZ_int(ret),susp,rest);
    }
    
  }
} OZ_BI_ioend

OZ_BI_iodefine(unix_sendToInet, 5,1)
{
  OZ_declareIntIN(0, sock);
  DeclareNonvarIN(1, vs);
  DeclareAtomListIN(2, OzFlags);
  OZ_declareVsIN(3, host);
  OZ_declareIntIN(4, port);

  int flags;
  OZ_Return flagBool;
  
  if (!((flagBool = get_send_recv_flags(OzFlags,&flags)) == PROCEED))
      return flagBool;

  CHECK_WRITE(sock);

  {
    struct hostent *hostaddr;
    
    if ((hostaddr = gethostbyname(host)) == NULL) {
      RETURN_NET_ERROR;
    }
    
    struct sockaddr_in addr;
    memset((char *)&addr, 0, sizeof (addr));
    addr.sin_family = AF_INET;
    memcpy(&addr.sin_addr, hostaddr->h_addr_list[0],
	   sizeof(addr.sin_addr));
    addr.sin_port = htons ((unsigned short) port);
    
    
    int len;
    OZ_Return status;
    OZ_Term rest, susp;
    vs_buff(write_buff);
    
    status = buffer_vs(vs, write_buff, &len, &rest, &susp);
    
    if (status != PROCEED && status != SUSPEND)
      return status; 
    
    WRAPCALL(sendto(sock, write_buff, len, flags,
		    (struct sockaddr *) &addr, sizeof(addr)), ret);
    
    if (len==ret && status != SUSPEND) {
      OZ_RETURN_INT(len);
    }
    
    if (status != SUSPEND) {
      susp = nil();
      rest = susp;
    }
    
    if (len > ret) {
      OZ_Term rest_all = OZ_pair2(buff2list(len - ret, write_buff + ret),rest);
      
      NEW_RETURN_SUSPEND(OZ_int(ret),susp,rest_all);
    } else {
      NEW_RETURN_SUSPEND(OZ_int(ret),susp,rest);
    }
  }

} OZ_BI_ioend


OZ_BI_iodefine(unix_shutDown, 2,0)
{
  OZ_declareIntIN(0,sock);
  OZ_declareIntIN(1,how);

  WRAPCALL(shutdown(sock, how), ret);

  return PROCEED;
} OZ_BI_ioend
  

  
OZ_BI_iodefine(unix_receiveFromInet,5,3)
{ 
  OZ_declareIntIN(0,sock);
  OZ_declareIntIN(1,maxx);
  DeclareAtomListIN(2, OzFlags);
  OZ_declareIN(3, hd);
  OZ_declareIN(4, tl);
  // OZ_out(0) == host
  // OZ_out(1) == port
  // OZ_out(2) == n

  int flags;
  OZ_Return flagBool;
  
  if (!((flagBool = get_send_recv_flags(OzFlags,&flags)) == PROCEED))
      return flagBool;

  CHECK_READ(sock);

  char *buf = (char *) malloc(maxx+1);

  struct sockaddr_in from;
  int fromlen = sizeof from;
  
  WRAPCALL(recvfrom(sock, buf, maxx, flags,
                    (struct sockaddr*)&from, &fromlen),ret);

  struct hostent *gethost = gethostbyaddr((char *) &from.sin_addr,
                                            fromlen, AF_INET);
    
  OZ_Term localhead = openbuff2list(ret, buf, tl);

  free(buf);

  if (OZ_unify(localhead, hd) != PROCEED) return FAILED;
  OZ_out(0) = OZ_string(CHARCAST (gethost ?
				  gethost->h_name :
				  inet_ntoa(from.sin_addr)));
  OZ_out(1) = OZ_int(ntohs(from.sin_port));
  OZ_out(2) = OZ_int(ret);
  return PROCEED;
} OZ_BI_ioend


const int maxArgv = 100;
static char* argv[maxArgv];

OZ_BI_define(unix_pipe,2,2)
{
  OZ_declareVsIN(0, s);
  OZ_declareIN(1, args);
  // OZ_out(0) == rpid
  // OZ_out(1) == rwsock

  OZ_Term hd, tl, argl;
  int argno = 0;

  argl=args;
  
  while (unixIsCons(argl, &hd, &tl)) {
    OZ_Term var = (OZ_Term) 0;
    if (!OZ_isVirtualString(hd,&var)) {
      if (var) {
	OZ_suspendOnInternal(var);
	return SUSPEND;
      } else {
	return OZ_typeError(1, "list(VirtualString)");
      }
    }
    argno++;
    argl = tl;
  }

  if (OZ_isVariable(argl)) {
    OZ_suspendOnInternal(argl);
    return SUSPEND;
  }

  if (!OZ_isNil(argl))
    return OZ_typeError(1,"list(VirtualString)");

  argl=args;
  
  if (argno+2 >= maxArgv) {
    return oz_raise(E_SYSTEM,E_SYSTEM,"limitInternal",1,
		    OZ_string("too many arguments for pipe"));
  }
  argv[0] = s;
  argv[argno+1] = 0;

  argno = 1;
  
  while (unixIsCons(argl, &hd, &tl)) {
    int len;
    OZ_Return status;
    OZ_Term rest, susp;

    char *vsarg = (char *) malloc(max_vs_length + 256);
    
    status = buffer_vs(hd, vsarg, &len, &rest, &susp);
    
    if (status == SUSPEND) {
      free(vsarg);
      Assert(!OZ_isVariable(susp));
      return oz_raise(E_SYSTEM,E_SYSTEM,"limitInternal",1,
		   OZ_string("virtual string too long"));
    }
    Assert(status == PROCEED);
    *(vsarg+len) = '\0';

    argv[argno++] = vsarg;
  
    argl = tl;
  }

#ifdef WINDOWS
  int k;
  char buf[10000];
  buf[0] = '\0';
  for (k=0 ; k<argno; k++) {
    strcat(buf,argv[k]);
  }

  SECURITY_ATTRIBUTES sa;
  sa.nLength = sizeof(sa);
  sa.lpSecurityDescriptor = NULL;
  sa.bInheritHandle = TRUE;

  STARTUPINFO si;
  memset(&si,0,sizeof(si));
  si.cb = sizeof(si);
  si.dwFlags = STARTF_FORCEOFFFEEDBACK;

  PROCESS_INFORMATION pinf;
  
  /* win32 does not support process groups,
   * so we set OZPPID such that subprocess can check whether
   * its father still lives
   */
  char auxbuf[100];
  int ppid = GetCurrentProcessId();
  sprintf(auxbuf,"%d",ppid);
  SetEnvironmentVariableA("OZPPID",strdup(auxbuf));


  HANDLE saveout = GetStdHandle(STD_OUTPUT_HANDLE);
  HANDLE savein  = GetStdHandle(STD_INPUT_HANDLE);
  HANDLE rh1,wh1,rh2,wh2;
  if (!CreatePipe(&rh1,&wh1,&sa,0)  ||
      !CreatePipe(&rh2,&wh2,&sa,0)  ||
      !SetStdHandle((DWORD)STD_OUTPUT_HANDLE,wh1) ||
      !SetStdHandle((DWORD)STD_INPUT_HANDLE,rh2) ||
      !CreateProcess(NULL,buf,&sa,NULL,TRUE,0,
		     NULL,NULL,&si,&pinf)) {
    return raiseUnixError(0, "Cannot create pipe process.", 
			  "windows");
  }

  int pid = (int) pinf.hProcess;
  CloseHandle(wh1);
  CloseHandle(rh2);
  SetStdHandle((DWORD)STD_OUTPUT_HANDLE,saveout);
  SetStdHandle((DWORD)STD_INPUT_HANDLE,savein);
    
  int rsock = _hdopen((int)rh1,O_RDONLY|O_BINARY);
  int wsock = _hdopen((int)wh2,O_WRONLY|O_BINARY);
  if (rsock<0 || wsock<0) {
    return raiseUnixError(0, 
			  "Cannot connect to created pipe process.", 
			  "windows");
  }

#else  /* !WINDOWS */

  int sv[2];
  WRAPCALL(socketpair(PF_UNIX,SOCK_STREAM,0,sv),ret);


  int pid =  fork();
  switch (pid) {
  case 0: // child
    {

#ifdef DEBUG_FORK_GROUP
      /*
       * create a new process group for child
       *   this allows to press Control-C when debugging the emulator
       */
      if (setsid() < 0) {
        RETURN_UNIX_ERROR;
      }
#endif

      int i;
      for (i = 0; i < FD_SETSIZE; i++) {
        if (i != sv[1]) {
          close(i);
        }
      }
      osdup(sv[1]);
      osdup(sv[1]);
      osdup(sv[1]);
      if (execvp(s,argv)  < 0) {
        RETURN_UNIX_ERROR;
      }
      printf("execvp failed\n");
      exit(-1);
    }
  case -1:
    RETURN_UNIX_ERROR;
  default: // parent
    break;
  }
  close(sv[1]);
  
  int rsock = sv[0];
  /* we cann use the same descriptor for both reading and writing: */
  int wsock = rsock;
#endif

  int i;
  for (i=1 ; i<argno ; i++)
    free(argv[i]);
    
  addChildProc(pid);

  TaggedRef rw = OZ_pair2(OZ_int(rsock),OZ_int(wsock));
  OZ_out(0) = OZ_int(pid);
  OZ_out(1) = rw;
  return PROCEED;
} OZ_BI_end

static OZ_Term mkAliasList(char **alias)
{
  OZ_Term ret = nil();
  while (*alias != 0) {
    ret = cons(OZ_string(*alias), ret);
    alias++;
  }
  return ret;
}

static OZ_Term mkAddressList(char **lstptr)
{
  OZ_Term ret = nil();
  while (*lstptr != NULL) {
    ret = cons(OZ_string(inet_ntoa(**((struct in_addr **) lstptr))),
                  ret);
    lstptr++;
  }
  return ret;
}

OZ_BI_iodefine(unix_getHostByName, 1,1)
{
  OZ_declareVsIN(0, name);

  struct hostent *hostaddr;

  if ((hostaddr = gethostbyname(name)) == NULL) {
    RETURN_NET_ERROR;
  }

  OZ_Term t1=OZ_pairAS("name",CHARCAST hostaddr->h_name);
  OZ_Term t2=OZ_pairA("aliases",mkAliasList(hostaddr->h_aliases));
  OZ_Term t3=OZ_pairA("addrList",mkAddressList(hostaddr->h_addr_list));
  OZ_Term pairlist= cons(t1,cons(t2,cons(t3,nil())));

  OZ_RETURN(OZ_recordInit(OZ_atom("hostent"),pairlist));
} OZ_BI_ioend


// Misc stuff

OZ_BI_iodefine(unix_unlink, 1,0) {
  OZ_declareVsIN(0,path);

  WRAPCALL(unlink(path),ret);
  return PROCEED;
} OZ_BI_ioend
  

OZ_BI_iodefine(unix_system,1,1)
{
  OZ_declareVsIN(0, vs);

  int ret = osSystem(vs);

  OZ_RETURN_INT(ret);
} OZ_BI_ioend

#if !defined(WINDOWS) || defined(GNUWIN32)
OZ_BI_iodefine(unix_wait,0,2)
{
  // OZ_out(0) == rpid
  // OZ_out(1) == rstat

  int status;
  int pid = waitpid(-1, &status, WNOHANG | WUNTRACED);

  OZ_out(0) = OZ_int(pid);
  OZ_out(1) = OZ_int(status);
  return PROCEED;
} OZ_BI_ioend


OZ_BI_iodefine(unix_getServByName, 2,1)
{
  OZ_declareVsIN(0, name);
  OZ_declareVsIN(1, proto);

  struct servent *serv;
  serv = getservbyname(name, proto);

  if (!serv) OZ_RETURN(OZ_false());

  OZ_RETURN_INT(ntohs(serv->s_port));
} OZ_BI_ioend
#endif

OZ_BI_iodefine(unix_tmpnam,0,1) {
  char *filename; 

  if (!(filename = tmpnam(NULL))) {
    return raiseUnixError(0, "OS.tmpnam failed.", "os");
  }
  filename = ozstrdup(filename);

  OZ_RETURN_STRING(filename);
} OZ_BI_ioend
  

OZ_BI_iodefine(unix_getEnv,1,1)
{
  OZ_declareVsIN(0, envVar);

  char *envValue;

  envValue = getenv(envVar);
  if (envValue == 0) OZ_RETURN(OZ_false());

  OZ_RETURN_STRING(envValue);
} OZ_BI_ioend


/* putenv is NOT POSIX !!! */
OZ_BI_iodefine(unix_putEnv,2,0)
{
  OZ_declareVsIN(0, envVar);
  OZ_declareVsIN(1, envValue);

  char *buf = new char[strlen(envVar)+strlen(envValue)+2];
  sprintf(buf,"%s=%s",envVar,envValue);
  int ret = putenv(buf);
  if (ret != 0) {
    delete buf;
    return raiseUnixError(0, "OS.putEnv failed.", "os");
  }

  return PROCEED;
} OZ_BI_ioend


OZ_Term make_time(const struct tm* tim)
{
  OZ_Term t1=OZ_pairAI("hour",tim->tm_hour);
  OZ_Term t2=OZ_pairAI("isDst",tim->tm_isdst);
  OZ_Term t3=OZ_pairAI("mDay",tim->tm_mday);
  OZ_Term t4=OZ_pairAI("min",tim->tm_min);
  OZ_Term t5=OZ_pairAI("mon",tim->tm_mon);
  OZ_Term t6=OZ_pairAI("sec",tim->tm_sec);
  OZ_Term t7=OZ_pairAI("wDay",tim->tm_wday);    
  OZ_Term t8=OZ_pairAI("yDay",tim->tm_yday);
  OZ_Term t9=OZ_pairAI("year",tim->tm_year);

  OZ_Term l1=cons(t6,cons(t7,cons(t8,cons(t9,nil()))));
  OZ_Term l2=cons(t1,cons(t2,cons(t3,cons(t4,cons(t5,l1)))));
  return OZ_recordInit(OZ_atom("time"),l2);
}

OZ_BI_iodefine(unix_time, 0,1)
{
  OZ_RETURN_INT(time(0));
} OZ_BI_ioend

OZ_BI_iodefine(unix_gmTime,0,1)
{
  time_t timebuf;

  time(&timebuf);
  OZ_RETURN(make_time(gmtime(&timebuf)));
  
} OZ_BI_ioend

OZ_BI_iodefine(unix_localTime, 0,1)
{
  time_t timebuf;

  time(&timebuf);
  OZ_RETURN(make_time(localtime(&timebuf)));
  
} OZ_BI_ioend

OZ_BI_define(unix_rand, 0,1)
{
  OZ_RETURN_INT(rand());
} OZ_BI_end

OZ_BI_define(unix_srand, 1,0)
{
  OZ_declareIntIN(0, seed);

  if (seed) {
    srand((unsigned int) seed);
  } else {
    srand((unsigned int) time(NULL));
  }
  
  return PROCEED;
} OZ_BI_end

#ifndef RAND_MAX
#ifdef SUNOS_SPARC
#define RAND_MAX ((1<<31)-1)
#else
#ifdef AIX3_RS6000
#   define RAND_MAX     32767
#else
... fill in RAND_MAX ...
#endif
#endif
#endif

OZ_BI_define(unix_randLimits, 0,2)
{
  // OZ_out(0) == minn
  // OZ_out(1) == maxx

  OZ_out(0) = OZ_int(0);
  OZ_out(1) = OZ_int(RAND_MAX);
  return PROCEED;
} OZ_BI_end

#ifdef WALSER
OZ_BI_define(unix_random, 0,1)
{
#if defined(SOLARIS) || defined(SUNOS_SPARC) || defined(LINUX)
  OZ_RETURN_INT(random());
#else
  return oz_raise(E_SYSTEM,E_SYSTEM,"limitExternal",1,OZ_atom("OS.random"));
  // OZ_RETURN_INT(rand())
#endif
} OZ_BI_end


OZ_BI_define(unix_srandom, 1,0)
{
  OZ_declareIntIN(0, seed);

  if (!seed) { seed = time(NULL); }

#if defined(SOLARIS) || defined(SUNOS_SPARC) || defined(LINUX)
  srandom((unsigned int) seed);
#else
  return oz_raise(E_SYSTEM,E_SYSTEM,"limitExternal",1,OZ_atom("OS.srandom"));
  // srand((unsigned int) seed);
#endif
  
  return PROCEED;
} OZ_BI_end
#endif


#ifdef WINDOWS

#define NotAvail(Name,Arity,Fun)			\
OZ_BI_iodefine(Fun,Arity)				\
{							\
  return oz_raise(E_SYSTEM,E_SYSTEM,"limitExternal",1,	\
		   OZ_atom(Name));			\
} OZ_BI_ioend


#ifndef GNUWIN32
NotAvail("OS.getServByName",   3, unix_getServByName);
NotAvail("OS.wait",            2, unix_wait);
NotAvail("OS.uName",           1, unix_uName);
#endif

#ifdef _MSC_VER
NotAvail("OS.getDir",          2, unix_getDir);
NotAvail("OS.fileDesc",        2, unix_fileDesc);
#endif

#endif

#include <pwd.h>
#include <sys/types.h>
OZ_BI_define(unix_getpwnam,1,1)
{
  OZ_declareVirtualStringIN(0,user);
  struct passwd *p = getpwnam(user);
  if (p==0) {
    return raiseUnixError(errno,OZ_unixError(errno),"getpwnam");
  } else {
    // return only POSIX fields
    OZ_Term N1 = oz_pairAA("name"  ,p->pw_name  );
    OZ_Term N2 = oz_pairAI("uid"   ,p->pw_uid   );
    OZ_Term N3 = oz_pairAI("gid"   ,p->pw_gid   );
    OZ_Term N4 = oz_pairAA("dir"   ,p->pw_dir   );
    OZ_Term N5 = oz_pairAA("shell" ,p->pw_shell );
    OZ_Term R =
      OZ_recordInit(
        oz_atom("passwd"),
        oz_cons(N1,oz_cons(N2,oz_cons(N3,oz_cons(N4,oz_cons(N5,oz_nil()))))));
    OZ_RETURN(R);
  }
} OZ_BI_end

void BIinitUnix()
{
#ifdef WINDOWS
  WSADATA wsa_data;
  WORD req_version = MAKEWORD(1,1);

  int ret = WSAStartup(req_version, &wsa_data);
  if (ret != 0 && ret != WSASYSNOTREADY)
    OZ_warning("Initialization of socket interface failed\n");

  //  fprintf(stderr, "szDescription = \"%s\"", wsa_data.szDescription);
  //  fprintf(stderr, "szSystemStatus = \"%s\"", wsa_data.szSystemStatus);
#endif
}
