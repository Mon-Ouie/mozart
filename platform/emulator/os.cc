/*
 *  Authors:
 *    Michael Mehl (mehl@dfki.de)
 *    Ralf Scheidhauer (Ralf.Scheidhauer@ps.uni-sb.de)
 *
 *  Contributors:
 *    Christian Schulte <schulte@ps.uni-sb.de>
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

// Unix/Posix functions

#if defined(INTERFACE)
#pragma implementation "os.hh"
#endif

#include "wsock.hh"

#include "am.hh"
#include "os.hh"
#include "value.hh"
#include "ozconfig.hh"

#include <errno.h>
#include <string.h>
#include <limits.h>

#ifdef HAVE_DLOPEN

#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#else
extern "C" void * dlopen(char *, int);
extern "C" char * dlerror(void);
extern "C" void * dlsym(void *, char *);
extern "C" int dlclose(void *);
#endif

#if !defined(RTLD_NOW)
#define RTLD_NOW 1
#endif

#if !defined(RTLD_GLOBAL)
#define RTLD_GLOBAL 0
#endif

#endif

#ifdef IRIX
#include <bstring.h>
#include <sys/time.h>
#endif

#ifdef HPUX_700
#include <dl.h>
#endif

#if defined(FOPEN_MAX) && !defined(OPEN_MAX)
#define OPEN_MAX  FOPEN_MAX
#endif

#ifdef MAX_OPEN
#define OPEN_MAX  MAX_OPEN
#endif

#ifdef WINDOWS
#include <time.h>
#ifdef _MSC_VER
#include <io.h>
#else
#include <sys/time.h>
#endif
#include <process.h>

#else
#include <sys/times.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/utsname.h>
#endif

#include <fcntl.h>

#if !defined(ultrix) && !defined(WINDOWS)
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#ifdef AIX3_RS6000
#include <sys/select.h>
#endif

fd_set socketFDs;
static int maxSocket = 0;

#ifdef WINDOWS

inline
Bool isSocket(int fd)
{
  return (FD_ISSET(fd,&socketFDs));
}

static int runningUnderNT()
{
  static int underNT = -1;
  if (underNT==-1) {
    OSVERSIONINFO vi;
    vi.dwOSVersionInfoSize = sizeof(vi);
    BOOL b = GetVersionEx(&vi);
    Assert(b==TRUE);
    underNT = (vi.dwPlatformId==VER_PLATFORM_WIN32_NT);
  }
  return underNT;
}

#ifdef __GNUC__
typedef long long verylong;
#else
typedef long verylong;
#endif

static verylong fileTimeToMS(FILETIME *ft)
{
  verylong x1 = ((verylong)(unsigned int)ft->dwHighDateTime)<<32;
  verylong x2 = x1 + (unsigned int)ft->dwLowDateTime;
  verylong ret = x2 / 10000;
  return ret;
}


static verylong emulatorStartTime;

/* return time since start in milli seconds */
static unsigned int getTotalTime()
{
  SYSTEMTIME st;
  GetSystemTime(&st);
  FILETIME ft;
  SystemTimeToFileTime(&st,&ft);
  return fileTimeToMS(&ft)-emulatorStartTime;
}

#else

static long emulatorStartTime = 0;

#endif


// return current usertime in milliseconds
unsigned int osUserTime()
{
#ifdef WINDOWS
  FILETIME ct,et,kt,ut;
  if (GetProcessTimes(GetCurrentProcess(),&ct,&et,&kt,&ut) != FALSE) {
    // only NT supports this
    return (unsigned int) fileTimeToMS(&ut);
  } else {
    return getTotalTime();
  }
#else
  struct tms buffer;

  times(&buffer);
  return (unsigned int)(buffer.tms_utime*1000.0/(double)sysconf(_SC_CLK_TCK));
#endif
}

// return current systemtime in milliseconds
unsigned int osSystemTime()
{
#ifdef WINDOWS
  FILETIME ct,et,kt,ut;
  if (GetProcessTimes(GetCurrentProcess(),&ct,&et,&kt,&ut) != FALSE) {
    // only NT supports this
    return (unsigned int) fileTimeToMS(&kt);
  } else {
    return 0;
  }
#else
  struct tms buffer;

  times(&buffer);
  return (unsigned int)(buffer.tms_stime*1000.0/(double)sysconf(_SC_CLK_TCK));
#endif
}



unsigned int osTotalTime()
{
#if defined(WINDOWS)
  return getTotalTime();

#elif defined(SUNOS_SPARC)

  struct timeval tp;

  (void) gettimeofday(&tp, NULL);

  return (unsigned int) ((tp.tv_sec - emulatorStartTime) * 1000 +
                         tp.tv_usec / 1000);

#else

  struct tms buffer;
  int t = times(&buffer) - emulatorStartTime;
  return (unsigned int) (t*1000.0/(double)sysconf(_SC_CLK_TCK));

#endif
}

#ifdef WINDOWS
class TimerThread {
public:
  HANDLE thrd;
  int wait;
  TimerThread(int w);
};


static TimerThread *timerthread = NULL;

static DWORD __stdcall timerFun(void *p)
{
  TimerThread *ti = (TimerThread*) p;
  /* make sure that this thread is not mixed with others */
  if (SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_HIGHEST)==FALSE) {
    OZ_warning("SetThreadPriority failed: %d\n",GetLastError());
  }
  while(1) {
    Sleep(ti->wait);
    handlerALRM(0);
  }
  delete ti;
  ExitThread(1);
  return 1;
}

TimerThread::TimerThread(int w)
{
  wait = w;
  DWORD tid;
  thrd = CreateThread(NULL,10000,&timerFun,this,0,&tid);
  if (thrd==NULL) {
    ozpwarning("osSetAlarmTimer(start thread)");
  }
}

#endif


void osBlockSignals(Bool check)
{
#ifdef WINDOWS
  if (timerthread)
    SuspendThread(timerthread->thrd);
#else
  sigset_t s,sOld;
  sigfillset(&s);

  /* some signals should not be blocked */
  sigdelset(&s,SIGINT);
  sigdelset(&s,SIGHUP);
  sigdelset(&s,SIGTERM);

  sigprocmask(SIG_SETMASK,&s,&sOld);

#ifdef DEBUG_CHECK_SIGNALS
  if (check) {
    sigemptyset(&s);
    if (memcmp(&s,&sOld,sizeof(sigset_t)) != 0) {
      OZ_warning("blockSignals: there are blocked signals");
    }
  }
#endif
#endif
}

void osUnblockSignals()
{
#ifdef WINDOWS
  if (timerthread)
    ResumeThread(timerthread->thrd);
#else
  sigset_t s;
  sigemptyset(&s);
  sigprocmask(SIG_SETMASK,&s,NULL);
#endif
}


/********************************************************************
 * Signal handling: we allow to install both C and Oz functions as
 * signal handlers. so in fact we install "genericHandler" as the
 * general handler which then dispatches to the right routines.
 ********************************************************************/

typedef void OsSigFun(int sig);

typedef struct {
  int signo;
  char *name;
  Bool pending;
  OsSigFun *chandler;
  OZ_Term  ozhandler;
} SigHandler ;

void handlerIgnore(int sig)
{
}

void handlerDefault(int sig)
{
  // kost@ : virtual sites need to be cleaned up - otherwise some
  // shared memory pages will get stuck in the system;
  am.exitOz(1);
}

#ifdef WINDOWS
#define SIGHUP SIGTERM
#endif


#define SIGLAST -1

static SigHandler handlers[] = {
  {SIGINT, "SIGINT", NO,handlerDefault,0},
  {SIGTERM,"SIGTERM",NO,handlerDefault,0},
  {SIGSEGV,"SIGSEGV",NO,handlerDefault,0},
#ifndef WINDOWS
  {SIGBUS, "SIGBUS", NO,handlerDefault,0},
#endif
  {SIGUSR1,"SIGUSR1",NO,handlerDefault,0},
  {SIGFPE, "SIGFPE", NO,handlerDefault,0},

#ifdef SIGHUP
  {SIGHUP, "SIGHUP",NO,handlerDefault,0},
#endif
#ifdef SIGUSR2
  {SIGUSR2,"SIGUSR2",NO,handlerDefault,0},
#endif
#ifdef SIGKILL
  {SIGKILL,"SIGKILL",NO,handlerDefault,0},
#endif
#ifdef SIGPIPE
  {SIGPIPE,"SIGPIPE",NO,handlerDefault,0},
#endif
#ifdef SIGCHLD
  {SIGCHLD,"SIGCHLD",NO,handlerIgnore,0},
#endif

  {SIGLAST,0,NO,0,0}
};

static
SigHandler *findHandler(int sig)
{
  SigHandler *aux = handlers;
  while(aux->signo != SIGLAST) {
    if (aux->signo == sig)
      return aux;
    aux++;
  }
  return NULL;
}

static
SigHandler *findHandler(const char *sig)
{
  SigHandler *aux = handlers;
  while(aux->signo != SIGLAST) {
    if (strcmp(aux->name,sig)==0)
      return aux;
    aux++;
  }
  return NULL;
}

static
void dropHandlers()
{
  handlers->signo = SIGLAST;
}

void pushSignalHandlers()
{
  SigHandler *aux = handlers;
  while(aux->signo != SIGLAST) {
    if (aux->pending) {
      if (OZ_eq(aux->ozhandler,OZ_atom("default"))) {
        (*aux->chandler)(aux->signo);
      } else {
        // existing Oz handler dominates handler that just does the default exit
        if (aux->chandler != handlerDefault)
          (*aux->chandler)(aux->signo);
        OZ_Thread thread = OZ_newRunnableThread();
        OZ_Term args[1];
        args[0] = OZ_atom(aux->name);
        OZ_pushCall(thread,aux->ozhandler,args,1);
      }
      aux->pending = NO;
    }
    aux++;
  }
}


static
void genericHandler(int sig)
{
  osBlockSignals();
  SigHandler *aux = findHandler(sig);
  if (aux == NULL)
    goto exit;

  Assert(aux->ozhandler != 0);

  if (OZ_eq(aux->ozhandler,OZ_atom("ignore"))) // also ignore C handler
    goto exit;

  aux->pending = OK;
  am.setSFlag(SigPending);
  // kost@ : see comments in front of 'AM::suspendEngine()';
  osWatchFD(fileno(stderr), SEL_WRITE);

exit:
  osUnblockSignals();
}

/* Oz version of signal(2)
   NOTE: (mm 13.10.94)
    Linux & Solaris are not POSIX compatible:
     therefor we need casts to (OsSigFun *). look at HERE.
    */

static
OsSigFun *osSignalInternal(int signo, OsSigFun *fun)
{
#ifdef WINDOWS
  signal(signo,(void(*)(int))fun);
  return NULL;
#else
  struct sigaction act, oact;

  /* type of act.sa_handler ist not the same on all platforms,
   * therefore this quite intricateness */
  OsSigFun **f = (OsSigFun**)&act.sa_handler;
  *f = fun;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;

  /* The following piece of code is from Stevens: Advanced UNIX Programming */
  if (signo == SIGALRM || signo == SIGUSR2) {
#ifdef SA_INTERUPT /* SunOS */
    act.sa_flags |= SA_INTERUPT;
#endif
  } else {
#ifdef SA_RESTART /* Sys V */
    act.sa_flags |= SA_RESTART;
#endif
  }
  if (sigaction(signo,&act,&oact) <0) {
    /* HERE */
    return (OsSigFun *) SIG_ERR;
  }
  /* HERE */
  return (OsSigFun *) oact.sa_handler;
#endif
}


static
Bool osSignal(int sig, OsSigFun *fun)
{
  SigHandler *aux = findHandler(sig);
  if (aux == NULL)
    return NO;

  aux->chandler = (fun == SIG_IGN) ? handlerIgnore : fun;
  return OK;
}

int atomToSignal(const char *signo)
{
  SigHandler *aux = findHandler(signo);
  return (aux == NULL) ? -1 : aux->signo;
}


Bool osSignal(const char *signo, OZ_Term proc)
{
  SigHandler *aux = findHandler(signo);
  if (aux == NULL)
    return NO;

  aux->ozhandler = proc;
  return OK;
}


int oskill(int pid, int sig)
{
#ifdef WINDOWS
  if (pid == 0) { // send to every process in process group
    if (sig == SIGTERM) { // function `raise' ignores signal SIGTERM
      osExit(sig|0x80);
    } else {
      //--** send to all process in ChildProc::allchildren as well
      return raise(sig) ? -1 : 0;
    }
  }

  switch (sig) {
  case SIGTERM:
  case SIGINT:
    {
      HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, 0, pid);
      if (hProcess) {
        return TerminateProcess(hProcess,sig|0x80) != FALSE ? 0 : -1;
      } else {
        return -1;
      }
    }
  default: // dont know how to send other signals (RS)
    return -1;
  }
#else
  return kill(pid,sig);
#endif
}

/* Oz version of system(3)
 * Posix 2 requires this call not to be interuptible by a signal
 * however some OS (like Solaris 2.3) do return EINTR, so we define our
 * own one, stolen from Stevens.0
 */


int osSystem(char *cmd)
{
#ifdef WINDOWS
  if (!runningUnderNT())
    return system(cmd);

  // RS: don't know why but NT hangs when I use system(3)
  STARTUPINFO si;
  memset(&si,0,sizeof(si));
  si.cb = sizeof(si);
  si.dwFlags = STARTF_FORCEOFFFEEDBACK;

  PROCESS_INFORMATION pinf;

  if (CreateProcess(NULL,cmd,NULL,NULL,FALSE,0,NULL,NULL,&si,&pinf) == FALSE)
    return 1;
  CloseHandle(pinf.hThread);
  DWORD ret = WaitForSingleObject(pinf.hProcess,INFINITE);
  if (ret == WAIT_FAILED ||
      GetExitCodeProcess(pinf.hProcess,&ret) == FALSE)
    ret = 1;

  CloseHandle(pinf.hProcess);
  return ret;
#else
  if (cmd == NULL) {
    return 1;
  }

  pid_t pid;
  if ((pid = fork()) < 0) {
    return -1;
  }

  if (pid == 0) {
    execl("/bin/sh","sh","-c",cmd, (char*) NULL);
    _exit(127);     /* execl error */
  }

  int status;
  while(waitpid(pid,&status,0) < 0) {
    if (errno != EINTR) {
      return -1;
    }
  }

  return status;
#endif
}


#if !defined(__GNUC__) || defined(CCMALLOC)
// Linking object files compiled with gcc needs these:

/* void * operator new (size_t sz) */
extern "C" void * __builtin_new (size_t sz)
{
  void *p;
  /* malloc (0) is unpredictable; avoid it.  */
  if (sz == 0)
    sz = 1;
  p = (void *) malloc (sz);
  return p;
}

extern "C" void * __builtin_vec_new (size_t sz)
{
  return __builtin_new(sz);
}

/* void operator delete (void *ptr) */
extern "C" void __builtin_delete (void *ptr)
{
  if (ptr)
    free (ptr);
}

extern "C" void __builtin_vec_delete (void *ptr)
{
  __builtin_delete(ptr);
}

#endif  /* __GNUC__ */


static int wrappedStdin = -1;


#ifdef WINDOWS

static
int rawread(int fd, void *buf, int sz)
{
  if (fd==STDIN_FILENO)
    fd = wrappedStdin;

  if (isSocket(fd))
    return recv(fd,((char*)buf),sz,0);

  return read(fd,buf,sz);
}


static
int rawwrite(int fd, void *buf, int sz)
{
  if (isSocket(fd))
    return send(fd, (char *)buf, sz, 0);

  return write(fd,buf,sz);
}

#else

#define rawwrite(fd,buf,sz) write(fd,buf,sz)
#define rawread(fd,buf,sz) read(fd,buf,sz)

#endif

static
int nonBlockSelect(int nfds, fd_set *readfds, fd_set *writefds)
{
  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 0;
  return select(nfds,readfds,writefds,NULL,&timeout);
}


/* under windows FD_SET is not idempotent */
#define OZ_FD_SET(i,fds) if (!FD_ISSET(i,fds)) { FD_SET(i,fds); }
#define OZ_FD_CLR(i,fds) if (FD_ISSET(i,fds))  { FD_CLR(i,fds); }


/* abstract timeout values */
#define WAIT_NULL     (int*) -1

int osOpenMax();

void printfds(fd_set *fds)
{
  fprintf(stderr,"FDS: ");
  for(int i=0; i<osOpenMax(); i++) {
    if (FD_ISSET(i,fds)) {
      fprintf(stderr,"%d,",i);
    }
  }
  fprintf(stderr,"\n");
  fflush(stderr);
}


void registerSocket(int fd)
{
#ifdef WINDOWS
  if (fd==STDIN_FILENO)
    fd = wrappedStdin;
#endif
  OZ_FD_SET(fd,&socketFDs);
  maxSocket = max(fd,maxSocket);
}




// 't' is in miliseconds;
void osSetAlarmTimer(int t)
{
#ifdef DEBUG_DET
    return;
#endif

#ifdef WINDOWS

  if (timerthread==NULL) {
    timerthread = new TimerThread(t);
  }

  if (t==0) {
    SuspendThread(timerthread->thrd);
  } else {
    timerthread->wait = t;
    ResumeThread(timerthread->thrd);
  }
#else
  struct itimerval newT;

  int sec  = t/1000;
  int usec = (t*1000)%1000000;
  newT.it_interval.tv_sec  = sec;
  newT.it_interval.tv_usec = usec;
  newT.it_value.tv_sec     = sec;
  newT.it_value.tv_usec    = usec;

  if (setitimer(ITIMER_REAL,&newT,NULL) < 0) {
    ozpwarning("setitimer");
  }
#endif
}

static long openMax;


#ifndef WINDOWS


int osGetAlarmTimer()
{
#ifdef DEBUG_DET
  return 0;
#else
  struct itimerval timer;
  if (getitimer(ITIMER_REAL,&timer) < 0) {
    ozpwarning("getitimer");
    return -1;
  }

  return (timer.it_value.tv_sec * 1000) + (timer.it_value.tv_usec/1000);
#endif
}
#endif


#ifdef WINDOWS
static int splitSocks(fd_set *in, fd_set *socks, fd_set *other)
{
  FD_ZERO(socks);
  FD_ZERO(other);

  /* hack: optimized scanning "in" by using definition of adt "fd_set" */
  int ret=0;
  for (unsigned i = 0; i < in->fd_count ; i++) {
    int fd = in->fd_array[i];
    if (isSocket(fd)) {
      ret++;
      OZ_FD_SET(fd,socks);
    } else {
      OZ_FD_SET(fd,other);
    }
  }
  return ret;
}


static int addOther(fd_set *socks, fd_set *other, fd_set *out)
{
  FD_ZERO(out);
  *out = *socks;

  /* hack: optimized scanning fd_set by using definition of adt "fd_set" */
  for (unsigned i = 0; i < other->fd_count ; i++) {
    OZ_FD_SET(i,out);
  }

  return out->fd_count;
}
#endif


/*
 * Wait *timeout msecs on given fds. If 'ptimeout' is WAIT_NULL,
 * return immediately.
 * Returns number of fds ready, and writes in *timeout #msecs really
 * spent in waiting;
 */
static
int osSelect(fd_set *readfds, fd_set *writefds, unsigned int *ptimeout)
{
  struct timeval timeoutstruct, *timeoutptr;
  unsigned int currentSystemTime;

  if (ptimeout == (unsigned int*) WAIT_NULL) {
    timeoutstruct.tv_sec = 0;
    timeoutstruct.tv_usec = 0;
    timeoutptr = &timeoutstruct;
  } else {
    int timeout = *ptimeout;
    if (timeout == 0) {
      timeoutptr = NULL;        // indefinitely;
    } else {
      timeoutstruct.tv_sec = timeout/1000;
      timeoutstruct.tv_usec = (timeout*1000)%1000000;
      timeoutptr = &timeoutstruct;
    }

    //
    currentSystemTime = osTotalTime();
    // note that the alarm clock is untouched here now;
    osUnblockSignals();
  }

#ifdef WINDOWS
  fd_set rsocks, wsocks, rother, wother;
  int numSocks = splitSocks(readfds,&rsocks,&rother)
               + splitSocks(writefds,&wsocks,&wother);
  int ret = 0;
  if (numSocks != 0) {
    ret = select(openMax,&rsocks,&wsocks,NULL,timeoutptr);
  }
  if (ret < 0 )
    return ret;
  ret = addOther(&rsocks,&rother,readfds) + addOther(&wsocks,&wother,writefds);
  *readfds = rsocks; *writefds = wsocks;
#else
  int ret = select(openMax,readfds,writefds,NULL,timeoutptr);
#endif

  if (ptimeout != (unsigned int*) WAIT_NULL) {
    // kost@ : Note that effectively the time spent in wait
    // may be greater than specified;
    *ptimeout = (int) (osTotalTime() - currentSystemTime);
    //    Assert(*ptimeout >= 0); // mm2: *ptimeout is unsigned!!!
    osBlockSignals();
  }

  return ret;
}

void osInitSignals()
{
  OZ_Term def = OZ_atom("default");
  SigHandler *aux = handlers;
  while(aux->signo != SIGLAST) {
    aux->ozhandler = def;
    OZ_protect(&aux->ozhandler);
    osSignal(aux->signo,aux->chandler);
    osSignalInternal(aux->signo,genericHandler);
    aux++;
  }

  // 'SIGUSR2' notifies about presence of tasks. Right now these are
  // only virtual site messages;
#ifdef SIGUSR2
  osSignal(SIGUSR2,handlerUSR2);
#endif
#ifdef SIGUSR1
  osSignal(SIGUSR1,handlerUSR1);
#endif
#ifdef SIGPIPE
  osSignal(SIGPIPE,handlerPIPE);
#endif
#ifdef SIGCHLD
  osSignal(SIGCHLD,handlerCHLD);
#endif
#ifdef SIGSEGV
  osSignal(SIGSEGV,handlerSEGV);
#endif
#ifdef SIGSBUS
  osSignal(SIGBUS,handlerBUS);
#endif

  // do not allow to overload SIGALRM
#ifdef SIGALRM
  osSignalInternal(SIGALRM,handlerALRM);
#endif
}


/*********************************************************
 *       Sockets                                         *
 *********************************************************/

static fd_set globalFDs[2];     // mask of active read/write FDs
static fd_set copyFDs[2];       // ... its copy for 'select()';

int osOpenMax()
{
#ifdef WINDOWS
  /* socket numbers may grow very large on Windows */
  return 100000;
#else
  int ret = sysconf(_SC_OPEN_MAX);
  if (ret == -1) {
    ret = _POSIX_OPEN_MAX;
  }
  return ret;
#endif
}



char *oslocalhostname()
{
#ifdef WINDOWS
  char buf[1000];
  int aux = gethostname(buf,sizeof(buf));
  if (aux != 0) {
    // OZ_warning("cannot determine hostname\n");
    // sprintf(buf,"%s","localhost");
    return 0;
  }
  return strdup(buf);
#else
  struct utsname unp;
  int n = uname(&unp);
  if(0 > n) /* braindead Solaris, returns >0 if OK. POSIX says 0 */
    return 0;
  return strdup(unp.nodename);
#endif
}


#ifdef WINDOWS

char *ostmpnam(char *s)
{
  char prefix[128];
  DWORD ret = GetTempPath(sizeof(prefix),prefix);
  if (ret == 0 || ret >= sizeof(prefix))
    strcpy(prefix,"C:\\TEMP\\");

  static char *tn = NULL;
  static int tnlen = 128;
  if (tn==0) tn = (char *) malloc(tnlen*sizeof(char));

  int newlen = strlen(prefix) + 10;
  if (newlen>tnlen) {
    tnlen = newlen;
    tn = (char*)realloc(tn,tnlen);
  }

  static int counter = 0;
  while(1) {
    sprintf(tn,"%soztmp%d",prefix,counter);
    counter = (counter++) % 10000;
    if (access(tn,F_OK)!=0)
      break;
  }

  if (s) {
    strcpy(s,tn);
    return s;
  }

  return tn;
}

int osdup(int fd)
{
  //--** no dup yet
  if (fd == STDIN_FILENO)
    return wrappedStdin;
  else
    return fd;
}

char *osgetenv(char *var)
{
  static char buffer[2048];
  int n = GetEnvironmentVariable(var,buffer,sizeof(buffer));
  if (n == 0 || n > sizeof(buffer))
    return NULL;
  else
    return buffer;
}

#else

char *ostmpnam(char *s) { return tmpnam(s); }

int osdup(int fd) { return dup(fd); }

char *osgetenv(char *var) { return getenv(var); }

#endif


char *osinet_ntoa(char *ip)
{
  /* workaroud for a bug in gcc 2.8 under Irix 6.x */
#if defined(IRIX6) && defined(__GNUC__)
  static char buf[20];
  sprintf(buf,"%d.%d.%d.%d",
          (unsigned)ip[0],(unsigned)ip[1],(unsigned)ip[2],(unsigned)ip[3]);
  return buf;
#else
  struct in_addr tmp;
  memcpy(&tmp,ip,sizeof(in_addr));
  return inet_ntoa(tmp);
#endif
}



void osInit()
{
  DebugCheck(CLOCK_TICK < 1000, OZ_error("CLOCK_TICK must be greater than 1 ms"));

  openMax=osOpenMax();

  FD_ZERO(&globalFDs[SEL_READ]);
  FD_ZERO(&globalFDs[SEL_WRITE]);

  FD_ZERO(&socketFDs);

#ifdef SUNOS_SPARC

  struct timeval tp;

  (void) gettimeofday(&tp, NULL);

  emulatorStartTime = tp.tv_sec;

#elif defined(WINDOWS)

  /* make sure everything is opened in binary mode */
  setmode(fileno(stdin),O_BINARY);  // otherwise input blocks!!
  _fmode = O_BINARY;

  SYSTEMTIME st;
  GetSystemTime(&st);
  FILETIME ft;
  SystemTimeToFileTime(&st,&ft);
  emulatorStartTime = fileTimeToMS(&ft);

  /* init sockets */
  WSADATA wsa_data;
  WORD req_version = MAKEWORD(1,1);

  int ret = WSAStartup(req_version, &wsa_data);
  if (ret != 0 && ret != WSASYSNOTREADY) {
    fprintf(stderr,
            "*** Initialization of the Windows socket interface failed.\n"
            "*** Most likely you have to install the Windows networking software, sorry.\n");
    fflush(stderr);
    Sleep(1000);
    am.exitOz(1);
  }

  /* allow select on stdin */
  int sv[2];
  int aux = ossocketpair(PF_UNIX,SOCK_STREAM,0,sv);
  createReader(sv[0],GetStdHandle(STD_INPUT_HANDLE));
  wrappedStdin = sv[1];

  /* win32 does not support process groups,
   * so we set OZPPID such that a subprocess can check whether
   * its father still lives
   */
  char auxbuf[100];
  int ppid = GetCurrentProcessId();
  sprintf(auxbuf,"%d",ppid);
  SetEnvironmentVariable("OZPPID",strdup(auxbuf));

#else

  struct tms buffer;
  emulatorStartTime = times(&buffer);;

#endif
}

#define CheckMode(mode) Assert(mode==SEL_READ || mode==SEL_WRITE)

static
void osWatchFDInternal(int fd, int mode)
{
  CheckMode(mode);
  OZ_FD_SET(fd,&globalFDs[mode]);
  // kost@ : in the case of preventing blocking in 'select()' by
  // waiting for write permission into 'stderr' (used by task
  // checkers&handlers, see am.cc), the copy must be updated as well:
  OZ_FD_SET(fd,&copyFDs[mode]);
}


void osWatchFD(int fd, int mode)
{
  osWatchFDInternal(fd,mode);
}

void osWatchAccept(int fd)
{
  osWatchFDInternal(fd,SEL_READ);
}


void osClrWatchedFD(int fd, int mode)
{
  CheckMode(mode);
  OZ_FD_CLR(fd,&globalFDs[mode]);
}


Bool osIsWatchedFD(int fd, int mode)
{
  CheckMode(mode);
  return (FD_ISSET(fd,&globalFDs[mode]));
}


/*
 * do a select, that waits "ms".
 * if "ms" <= 0 do a blocking select
 */
void osBlockSelect(unsigned int &ms)
{
  copyFDs[SEL_READ]  = globalFDs[SEL_READ];
  copyFDs[SEL_WRITE] = globalFDs[SEL_WRITE];
  (void) osSelect(&copyFDs[SEL_READ],&copyFDs[SEL_WRITE],&ms);
}

/* osClearSocketErrors
 * remove the closed/failed descriptors from the fd_sets
 */
void osClearSocketErrors()
{
  fd_set auxFDs[2];

  /* osClrWatchedFD might change globalFDs, such that the next
   * call to FD_ISSET on globalFDs might fail */
  auxFDs[SEL_READ]  = globalFDs[SEL_READ];
  auxFDs[SEL_WRITE] = globalFDs[SEL_WRITE];

  for (int i = 0; i < openMax; i++) {
    for(int mode=SEL_READ; mode <= SEL_WRITE; mode++) {
      if (FD_ISSET(i,&auxFDs[mode]) && osTestSelect(i,mode) < 0) {
        osClrWatchedFD(i,mode);
      }
    }
  }
}

/* returns: 1 if select succeeded on fd
 *          0 did not succeed
 *         -1 on error
 */
int osTestSelect(int fd, int mode)
{
  CheckMode(mode);
#ifdef WINDOWS
  if (!isSocket(fd))
    return OK;
#endif

  while(1) {
    fd_set fdset, *readFDs=NULL, *writeFDs=NULL;
    FD_ZERO(&fdset);
    OZ_FD_SET(fd,&fdset);

    if (mode==SEL_READ) {
      readFDs = &fdset;
    } else {
      writeFDs = &fdset;
    }

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    int ret = nonBlockSelect(fd+1, readFDs, writeFDs);
    if (ret >= 0 || ossockerrno() != EINTR) {
      return ret;
    }
  }
}

/* -------------------------------------------------------------------------
 * subroutines for AM::handleIO
 * ------------------------------------------------------------------------- */

// another copy since it must be preserved between 'osFirstSelect()'
// and 'osNextSelect()';
static fd_set tmpFDs[2];

/* signals are blocked */
int osFirstSelect()
{
 loop:
  tmpFDs[SEL_READ]  = globalFDs[SEL_READ];
  tmpFDs[SEL_WRITE] = globalFDs[SEL_WRITE];

  int numbOfFDs = osSelect(&tmpFDs[SEL_READ], &tmpFDs[SEL_WRITE],
                           (unsigned int *) WAIT_NULL);

  if (numbOfFDs < 0) {
    if (ossockerrno() == EINTR) goto loop;
    if (ossockerrno() != EBADF) { /* some pipes may have been closed */
      printfds(&tmpFDs[SEL_READ]);
      printfds(&tmpFDs[SEL_WRITE]);
      ozpwarning("select failed");
    }
    osClearSocketErrors();
  }
  return numbOfFDs;
}

Bool osNextSelect(int fd, int mode)
{
  CheckMode(mode);

  if (FD_ISSET(fd,&tmpFDs[mode])) {
    OZ_FD_CLR(fd,&tmpFDs[mode]);
    return OK;
  }
  return NO;
}

/* -------------------------------------------------------------------------
 * subroutine for AM::checkIO
 * ------------------------------------------------------------------------- */


// called from AM::checkIO
int osCheckIO()
{
 loop:
  copyFDs[SEL_READ]  = globalFDs[SEL_READ];
  copyFDs[SEL_WRITE] = globalFDs[SEL_WRITE];

  int numbOfFDs = osSelect(&copyFDs[SEL_READ], &copyFDs[SEL_WRITE],
                           (unsigned int *) WAIT_NULL);
  if (numbOfFDs < 0) {
    if (ossockerrno() == EINTR) goto loop;
    if (ossockerrno() != EBADF) { /* some pipes may have been closed */
      printfds(&globalFDs[SEL_READ]);
      printfds(&globalFDs[SEL_WRITE]);
      ozpwarning("checkIO: select failed");
    }
    osClearSocketErrors();
  }
  return numbOfFDs;
}





/*
 *  we want to kill all our children on exit *
 */


class ChildProc {
public:
  pid_t pid;
  ChildProc *next;
  ChildProc(pid_t p, ChildProc *nxt) : next(nxt), pid(p) {}
  static ChildProc *allchildren;
  static void add(pid_t p)
  {
    allchildren = new ChildProc(p,allchildren);
  }
};

ChildProc *ChildProc::allchildren = NULL;


void addChildProc(pid_t pid)
{
  ChildProc::add(pid);
}


void osExit(int status)
{
  /* terminate all our children */
  ChildProc *aux = ChildProc::allchildren;
  while(aux) {
    (void) oskill(aux->pid,SIGTERM);
    aux = aux->next;
  }

  dropHandlers();
#ifdef WINDOWS
  ExitProcess(status);
#else
  exit(status);
#endif
}




int osread(int fd, void *buf, unsigned int len)
{
  return rawread(fd, buf, len);
}

int ossaferead(int fd, char *buf, unsigned int len)
{
 loop:
  int ret = osread(fd,buf,len);
  if (ret < 0 && ossockerrno()==EINTR)
    goto loop;
  return ret;
}


/* currently no wrapping for write */
int oswrite(int fd, void *buf, unsigned int len)
{
  return rawwrite(fd, buf, len);
}

int ossafewrite(int fd, char *buf, unsigned int len)
{
  int origLen = len;

 loop:
  int written = oswrite(fd,buf,len);
  if (written < 0) {
    if (ossockerrno()==EINTR) goto loop;
    return written;
  }
  if ((unsigned)written<len) {
    buf += written;
    len -= written;
    goto loop;
  }
  return origLen;
}

int osclose(int fd)
{
  OZ_FD_CLR((unsigned int)fd,&globalFDs[SEL_READ]);
  OZ_FD_CLR((unsigned int)fd,&globalFDs[SEL_WRITE]);

#ifdef WINDOWS
  // never close stdin on Windows, leads to problems
  if (fd == wrappedStdin)
    return 0;

  Assert(fd!=STDIN_FILENO);

  if (isSocket(fd)) {
    OZ_FD_CLR((unsigned int)fd,&socketFDs);
    return closesocket(fd);
  }
#endif
  return close(fd);
}

int osopen(const char *path, int flags, int mode)
{
  int ret = open(path,flags,mode);
  if (ret >= 0)
    OZ_FD_CLR((unsigned int)ret,&socketFDs);
  return ret;
}

int ossocket(int domain, int type, int protocol)
{
  int ret = socket(domain,type,protocol);
  if (ret >= 0)
    registerSocket(ret);
  return ret;
}

int osaccept(int s, struct sockaddr *addr, int *addrlen)
{
#if __GLIBC__ == 2
  int ret = accept(s,addr,(unsigned int*)addrlen);
#else
  int ret = accept(s,addr,(socklen_t*) addrlen);
#endif
  if (ret >= 0)
    registerSocket(ret);
  return ret;
}


int osconnect(int s, struct sockaddr *addr, int namelen)
{
  osBlockSignals();
  int ret = connect(s,addr,namelen);
  osUnblockSignals();
  return ret;
}


int ossockerrno()
{
#ifdef WINDOWS
  int ret = WSAGetLastError();
  return ret != 0 ? ret : errno;
#else
  return errno;
#endif
}

void ossleep(int secs)
{
#ifdef WINDOWS
  Sleep(secs*1000);
#else
  sleep(secs);
#endif
}

int osgetpid()
{
  int pid = getpid();
  return pid>0 ? pid : -pid;  // fucking Windows 95 returns negative pids
}

/* fgets may return NULL under Solaris if
 * interupted by the timer signal for example
 */
char *osfgets(char *s, int n, FILE *stream)
{
  osBlockSignals();
  char *ret = fgets(s,n,stream);
  osUnblockSignals();

  return ret;
}


#ifdef XXWINDOWS

// execution of C++ initializers

extern "C" {

extern void (*__CTOR_LIST__)();

int WINAPI dll_entry(int a,int b,int c)
{
  void (**pfunc)() = &__CTOR_LIST__;

  for (int i = 1; pfunc[i]; i++) {
    (pfunc[i])();
  }
  return 1;
}

}

#endif

/* -----------------------------------------------------------------
   stack dump
   ----------------------------------------------------------------- */

#if 0 && !defined(WINDOWS) && !defined(DEBUG_CHECK)

/* try to attach gdb to us and print a stack dump */

void osStackDump()
{
  fprintf(stderr,"Stack Dump:\n");

  char *tmpfile = ostmpnam(NULL);
  FILE *tmpout = fopen(tmpfile,"w");
  if (tmpout==NULL)
    tmpout = stdout;

  fprintf(tmpout,"set confirm off\nbacktrace\nkill\nquit\n");
  if (tmpout != stdout) {
    fclose(tmpout);
    char buf[1000];
    sprintf(buf,"gdb -batch -n -x %s -c %d %s",tmpfile,osgetpid(),ozconf.emuexe);
    osSystem(buf);
  }

  unlink(tmpfile);
}

#else

void osStackDump() {}

#endif


/* -----------------------------------------------------------------
   dynamic link objects files
   ----------------------------------------------------------------- */

TaggedRef osDlopen(char *filename, void **hdl)
{
#ifdef HPUX_700

  shl_t shlhandle;
  shlhandle = shl_load(filename,
                       BIND_IMMEDIATE | BIND_NONFATAL |
                       BIND_NOSTART | BIND_VERBOSE, 0L);

  if (shlhandle == NULL) {
    return oz_atom("osDlopen failed");
  }
  void *handle = (void*)shlhandle;

#elif defined(HAVE_DLOPEN)

  // RTLD_GLOBAL is important: it serves the same purpose
  // as -rdynamic for executables: newly loaded objects
  // are linked against symbols coming from already
  // liked objects (example: libfd and libschedule)

  void *handle=dlopen(filename, RTLD_NOW | RTLD_GLOBAL);

  if (!handle) {
    return oz_atom(dlerror());
  }

#elif defined(WINDOWS)

  void *handle = (void *)LoadLibrary(filename);
  if (!handle) {
    return oz_int(GetLastError());
  }

#else

  void *handle = 0; // otherwise the rest does not compile
  return oz_atom("osDlopen not supported");

#endif

  *hdl = handle;
  return 0;
}

int osDlclose(void* handle)
{
#ifdef HAVE_DLOPEN
  if (dlclose(handle)) {
    return -1;
  }
#endif

#ifdef WINDOWS
  FreeLibrary((HMODULE)handle);
#endif

  return 0;
}

#if defined(HAVE_DLOPEN)
void *osDlsym(void *handle,const char *name) { return dlsym(handle,name); }
#elif defined(HPUX_700)
void *osDlsym(void *handle,const char *name)
{
  void *symaddr;

  int symbol = shl_findsym((shl_t*)&handle, name, TYPE_PROCEDURE, &symaddr);
  if (symbol != 0) {
    return NULL;
  }

  return symaddr;
}
#elif defined(WINDOWS)
void *osDlsym(void *h, const char *name)
{
  HMODULE handle = (HMODULE) h;
  FARPROC ret = GetProcAddress(handle,name);
  if (ret == NULL) {
    // try prepending a "_"
    char buf[1000];
    sprintf(buf,"_%s",name);
    ret = GetProcAddress(handle,buf);
  }
  return ret;
}
#else
void *osDlsym(void *h, const char *name) {}
#endif

/* readdir and friends for MSVC++:
 * stolen from egcs distribution */

#ifdef _MSC_VER

#define SUFFIX  "*"
#define SLASH   "\\"


DIR* opendir(const char* szPath)
{
        DIR* nd;
        struct stat statDir;

        errno = 0;

        if (!szPath)
        {
                errno = EFAULT;
                return (DIR*) 0;
        }

        if (szPath[0] == '\0')
        {
                errno = ENOTDIR;
                return (DIR*) 0;
        }

        /* Attempt to determine if the given path really is a directory. */
        if (stat (szPath, &statDir))
        {
                /* Error, stat should have set an error value. */
                return (DIR*) 0;
        }

        if (!S_ISDIR(statDir.st_mode))
        {
                /* Error, stat reports not a directory. */
                errno = ENOTDIR;
                return (DIR*) 0;
        }

        /* Allocate enough space to store DIR structure and the complete
         * directory path given. */
        nd = (DIR*) malloc (sizeof(DIR) + strlen(szPath) + strlen(SLASH) +
                strlen(SUFFIX));

        if (!nd)
        {
                /* Error, out of memory. */
                errno = ENOMEM;
                return (DIR*) 0;
        }

        /* Create the search expression. */
        strcpy(nd->dd_name, szPath);

        /* Add on a slash if the path does not end with one. */
        if (nd->dd_name[0] != '\0' &&
            nd->dd_name[strlen(nd->dd_name)-1] != '/' &&
            nd->dd_name[strlen(nd->dd_name)-1] != '\\')
        {
                strcat(nd->dd_name, SLASH);
        }

        /* Add on the search pattern */
        strcat(nd->dd_name, SUFFIX);

        /* Initialize handle to -1 so that a premature closedir doesn't try
         * to call _findclose on it. */
        nd->dd_handle = -1;

        /* Initialize the status. */
        nd->dd_stat = 0;

        /* Initialize the dirent structure. ino and reclen are invalid under
         * Win32, and name simply points at the appropriate part of the
         * findfirst_t structure. */
        nd->dd_dir.d_ino = 0;
        nd->dd_dir.d_reclen = 0;
        nd->dd_dir.d_namlen = 0;
        nd->dd_dir.d_name = nd->dd_dta.name;

        return nd;
}


/*
 * readdir
 *
 * Return a pointer to a dirent structure filled with the information on the
 * next entry in the directory.
 */
struct dirent *
readdir( DIR *dirp )
{
        errno = 0;

        /* Check for valid DIR struct. */
        if (!dirp)
        {
                errno = EFAULT;
                return (struct dirent*) 0;
        }

        if (dirp->dd_dir.d_name != dirp->dd_dta.name)
        {
                /* The structure does not seem to be set up correctly. */
                errno = EINVAL;
                return (struct dirent*) 0;
        }

        if (dirp->dd_stat < 0)
        {
                /* We have already returned all files in the directory
                 * (or the structure has an invalid dd_stat). */
                return (struct dirent *) 0;
        }
        else if (dirp->dd_stat == 0)
        {
                /* We haven't started the search yet. */
                /* Start the search */
                dirp->dd_handle = _findfirst(dirp->dd_name, &(dirp->dd_dta));

                if (dirp->dd_handle == -1)
                {
                        /* Whoops! Seems there are no files in that
                         * directory. */
                        dirp->dd_stat = -1;
                }
                else
                {
                        dirp->dd_stat = 1;
                }
        }
        else
        {
                /* Get the next search entry. */
                if (_findnext(dirp->dd_handle, &(dirp->dd_dta)))
                {
                        /* We are off the end or otherwise error. */
                        _findclose (dirp->dd_handle);
                        dirp->dd_handle = -1;
                        dirp->dd_stat = -1;
                }
                else
                {
                        /* Update the status to indicate the correct
                         * number. */
                        dirp->dd_stat++;
                }
        }

        if (dirp->dd_stat > 0)
        {
                /* Successfully got an entry. Everything about the file is
                 * already appropriately filled in except the length of the
                 * file name. */
                dirp->dd_dir.d_namlen = strlen(dirp->dd_dir.d_name);
                return &dirp->dd_dir;
        }

        return (struct dirent*) 0;
}


/*
 * closedir
 *
 * Frees up resources allocated by opendir.
 */
int
closedir (DIR* dirp)
{
        int     rc;

        errno = 0;
        rc = 0;

        if (!dirp)
        {
                errno = EFAULT;
                return -1;
        }

        if (dirp->dd_handle != -1)
        {
                rc = _findclose(dirp->dd_handle);
        }

        /* Delete the dir structure. */
        free (dirp);

        return rc;
}

#endif

#ifdef WINDOWS
/* stolen from cygwin32 */
int ossocketpair (int, int type, int, int *sb)
{
  int res = -1;
  SOCKET insock, outsock, newsock;
  struct sockaddr_in sock_in;
  int len = sizeof (sock_in);

  newsock = ossocket(AF_INET, type, 0);
  if (newsock == INVALID_SOCKET) {
    goto done;
  }

  /* bind the socket to any unused port */
  sock_in.sin_family = AF_INET;
  sock_in.sin_port = 0;
  sock_in.sin_addr.s_addr = INADDR_ANY;
  if (bind(newsock, (struct sockaddr *) &sock_in, sizeof (sock_in)) < 0) {
    goto done;
  }

  if (getsockname(newsock, (struct sockaddr *) &sock_in, &len) < 0) {
    closesocket(newsock);
    goto done;
  }
  listen(newsock, 2);

  /* create a connecting socket */
  outsock = ossocket(AF_INET, type, 0);
  if (outsock == INVALID_SOCKET) {
    closesocket(newsock);
    goto done;
  }
  sock_in.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

  /* Do a connect and accept the connection */
  if (connect(outsock, (struct sockaddr *) &sock_in, sizeof (sock_in)) < 0) {
    closesocket(newsock);
    closesocket(outsock);
    goto done;
  }
  insock = osaccept(newsock, (struct sockaddr *) &sock_in, &len);
  if (insock == INVALID_SOCKET) {
    closesocket(newsock);
    closesocket(outsock);
    goto done;
  }

  closesocket(newsock);
  res = 0;

  sb[0] = insock;
  sb[1] = outsock;

done:
  return res;
}


class InOut {
public:
  SOCKET fd1;
  HANDLE fd2;
  InOut(SOCKET f1, HANDLE f2): fd1(f1), fd2(f2) {}
};


#define bufSz 10000

static DWORD __stdcall readerThread(void *p)
{
  InOut *io = (InOut*) p;
  SOCKET out = io->fd1;
  HANDLE in = io->fd2;
  delete io;

  char buf[bufSz];
  while(1) {
    DWORD count;
    if (ReadFile(in,buf,bufSz,&count,0)==FALSE) {
      //message("ReadFile(%d) failed: %d\n",in,GetLastError());
      break;
    }

  loop:
    int sent= send(out,buf,count,0);
    if (sent<0) {
      // message("send failed: %d\n",WSAGetLastError());
      break;
    }
    count -= sent;
    if (count > 0)
      goto loop;
  }
  CloseHandle(in);
  osclose(out);
  ExitThread(1);
  return 1;
}


static DWORD __stdcall writerThread(void *p)
{
  InOut *io = (InOut*) p;
  SOCKET in = io->fd1;
  HANDLE out = io->fd2;
  delete io;

  char buf[bufSz];
  while(1) {
    int got = recv(in,buf,bufSz,0);
    if (got<0) {
      //message("recv(%d) failed: %d\n",in,WSAGetLastError());
      break;
    }

  loop:
    DWORD count;
    if (WriteFile(out,buf,got,&count,0)==FALSE) {
      //message("WriteFile(%d) failed: %d\n",out,GetLastError());
      break;
    }
    got -= count;
    if (got>0)
      goto loop;
  }
  CloseHandle(out);
  osclose(in);
  ExitThread(1);
  return 1;
}


void createReader(SOCKET s,HANDLE h)
{
  DWORD tid;
  HANDLE th = CreateThread(NULL,10000,&readerThread,new InOut(s,h),0,&tid);
  CloseHandle(th);
}

void createWriter(SOCKET s,HANDLE h)
{
  DWORD tid;
  HANDLE th = CreateThread(NULL,10000,&writerThread,new InOut(s,h),0,&tid);
  CloseHandle(th);
}

#endif
