/* 
 * winMain.c --
 *
 *	Main entry point for wish and other Tk-based applications.
 *
 * Copyright (c) 1995 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#ifdef __WATCOMC__
#pragma aux (cdecl) Tk_MainWindow;
#pragma aux (cdecl) Tcl_AppendResult;
#pragma aux (cdecl) Tcl_CreateCommand;
#pragma aux (cdecl) Tcl_Merge;
#pragma aux (cdecl) Tcl_SetVar;
#pragma aux (cdecl) Tcl_CreateInterp;
#pragma aux (cdecl) Tcl_GetVar;
#pragma aux (cdecl) Tcl_GetVar2;
#pragma aux (cdecl) Tcl_DStringFree;
#pragma aux (cdecl) Tcl_DStringValue;
#pragma aux (cdecl) Tcl_DStringSetLength;
#pragma aux (cdecl) Tcl_Eval;
#pragma aux (cdecl) Tcl_Init;
#pragma aux (cdecl) Tcl_SetPanicProc;
#pragma aux (cdecl) Tcl_SetVar2;
#pragma aux (cdecl) Tk_ParseArgv;
#pragma aux (cdecl) Tcl_DStringInit;
#pragma aux (cdecl) Tcl_ResetResult;
#pragma aux (cdecl) Tcl_VarEval;
#pragma aux (cdecl) Tk_Init;
#pragma aux (cdecl) TkWinXInit;
#pragma aux (cdecl) Tk_MainLoop;
#pragma aux (cdecl) Tcl_DStringAppend;
#pragma aux (cdecl) Tcl_CommandComplete;
#pragma aux (cdecl) Tcl_AsyncCreate;
#pragma aux (cdecl) Tcl_AsyncMark;
#pragma aux (cdecl) Tcl_DoWhenIdle;
#pragma aux (cdecl) Tcl_Alloc;
#pragma aux (cdecl) Tcl_Free;
#pragma aux (cdecl) PutsCmd;
#pragma aux (cdecl) idleProc;
#pragma aux (cdecl) asyncHandler;
#endif

#ifdef _MSC_VER
#define _hdopen(file,flags) _open_osfhandle(file,flags)
#endif

#include <windows.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <process.h>

#define Tk_MainLoop xx1
#define Tk_MainWindow xx2
#include "tk.h"
#undef Tk_MainLoop
#undef Tk_MainWindow

static void cdecl WishPanic(char *x,...);
static unsigned __stdcall readerThread(void *arg);
static int cdecl asyncHandler(ClientData cd, Tcl_Interp *i, int code);

extern void cdecl Tk_MainLoop();
extern Tk_Window cdecl Tk_MainWindow(Tcl_Interp *interp);


#define XXDEBUG

#ifdef DEBUG
FILE *dbgout = NULL, *dbgin = NULL;
#endif

FILE *outstream;

/*
 * Global variables used by the main program:
 */

static Tcl_Interp *interp;	/* Interpreter for this application. */
static char argv0[255];		/* Buffer used to hold argv0. */

/*
 * Command-line options:
 */

static char *fileName = NULL;

static Tk_ArgvInfo argTable[] = {
    {(char *) NULL, TK_ARGV_END, (char *) NULL, (char *) NULL,
	(char *) NULL}
};

int
PutsCmd(clientData, inter, argc, argv)
    ClientData clientData;		/* ConsoleInfo pointer. */
    Tcl_Interp *inter;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    char **argv;			/* Argument strings. */
{
    FILE *f;
    int i, newline;
    char *fileId;

    i = 1;
    newline = 1;
    if ((argc >= 2) && (strcmp(argv[1], "-nonewline") == 0)) {
      newline = 0;
      i++;
    }

    if ((i < (argc-3)) || (i >= argc)) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" ?-nonewline? ?fileId? string\"", (char *) NULL);
	return TCL_ERROR;
    }

    if (i == (argc-1)) {
	fileId = "stdout";
    } else {
	fileId = argv[i];
	i++;
    }

    f = outstream;

    clearerr(f);
    fputs(argv[i], f);
    if (newline) {
      fputc('\n', f);
    }
    fflush(f);

#ifdef DEBUG
    fprintf(dbgout,"********puts(%d):\n%s\n",inter,argv[i]); fflush(dbgout);
#endif

    if (ferror(f)) {
      WishPanic("Connection to engine lost");
      ExitProcess(1);
      return TCL_ERROR;
    }
    return TCL_OK;
}



typedef struct {
  char *cmd;
  HANDLE event;
  DWORD toplevelThread;
  Tcl_AsyncHandler ash;
} ReaderInfo;

/*
 *----------------------------------------------------------------------
 *
 * WinMain --
 *
 *	Main entry point from Windows.
 *
 * Results:
 *	Returns false if initialization fails, otherwise it never
 *	returns. 
 *
 * Side effects:
 *	Just about anything, since from here we call arbitrary Tcl code.
 *
 *----------------------------------------------------------------------
 */



/* THE TWO FOLLOWING FUNCTIONS HAVE BEEN COPIED FROM EMULATOR */


unsigned __stdcall watchEmulatorThread(void *arg)
{
  HANDLE handle = (HANDLE) arg;
  DWORD ret = WaitForSingleObject(handle,INFINITE);
  if (ret != WAIT_OBJECT_0) {
    WishPanic("WaitForSingleObject(0x%x) failed: %d (error=%d)",
	      handle,ret,GetLastError());
    ExitThread(0);
  }
  ExitProcess(0);
  return 1;
}

/* there are no process groups under Win32
 * so Emulator hands its pid via envvar OZPPID to emulator
 * it then creates a thread watching whether the Emulator is still living
 * and terminating otherwise
 */
void watchParent()
{
  char buf[100];
  int pid;
  HANDLE handle;
  unsigned thrid;

  if (GetEnvironmentVariable("OZPPID",buf,sizeof(buf)) == 0) {
    WishPanic("getenv failed");
  }

  pid = atoi(buf);
  handle = OpenProcess(SYNCHRONIZE, 0, pid);
  if (handle==0) {
    WishPanic("OpenProcess(%d) failed",pid);
  } else {
    _beginthreadex(0,0,watchEmulatorThread,handle,0,&thrid);
  }
}



int APIENTRY
WinMain(hInstance, hPrevInstance, lpszCmdLine, nCmdShow)
    HINSTANCE hInstance;
    HINSTANCE hPrevInstance;
    LPSTR lpszCmdLine;
    int nCmdShow;
{
    char **argv, **argvlist, *args, *p, *ozhome;
    int argc, size, i, code;
    char buf[300];
    size_t length;

#ifdef DEBUG
      dbgin  = fopen("c:\\tmp\\wishdbgin","w");
      dbgout = fopen("c:\\tmp\\wishdbgout","w");
      if (dbgin == NULL || dbgout == NULL)
	WishPanic("cannot open dbgin/dbgout");
#endif

    Tcl_SetPanicProc(WishPanic);

    TkWinXInit(hInstance);

    /*
     * Increase the application queue size from default value of 8.
     * At the default value, cross application SendMessage of WM_KILLFOCUS
     * will fail because the handler will not be able to do a PostMessage!
     * This is only needed for Windows 3.x, since NT dynamically expands
     * the queue.
     */
    SetMessageQueue(64);

    interp = Tcl_CreateInterp();

    /* set TCL_LIBRARY and TK_LIBRARY */
    ozhome = getenv("OZHOME");
    if (ozhome == NULL) {
      WishPanic("OZHOME not set\n");
    }

    if (getenv("TCL_LIBRARY") == NULL) {
      sprintf(buf,"%s\\ozwish\\lib\\tcl\\",ozhome);
      Tcl_SetVar(interp, "tcl_library", buf, TCL_GLOBAL_ONLY);
    }

    if (getenv("TK_LIBRARY") == NULL) {
      sprintf(buf,"%s/ozwish/lib/tk",ozhome);
      Tcl_SetVar2(interp, "env", "TK_LIBRARY", buf, TCL_GLOBAL_ONLY);
    }

    /*
     * First get an upper bound on the size of the argv array by counting the
     * number of whitespace characters in the string.
     */

    for (size = 1, p = lpszCmdLine; *p != '\0'; p++) {
      if (isspace(*p)) {
	size++;
      }
    }
    size++;			/* Leave space for final NULL pointer. */
    argvlist = (char **) ckalloc((unsigned) (size * sizeof(char *)));
    argv = argvlist;

    /*
     * Split the command line into words, and store pointers to the start of
     * each word into the argv array.  Skips leading whitespace on each word.
     */

    for (i = 0, p = lpszCmdLine; *p != '\0'; i++) {
	while (isspace(*p)) {
	    p++;
	}
	if (*p == '\0') {
	    break;
	}
	argv[i] = p;
	while (*p != '\0' && !isspace(*p)) {
	    p++;
	}
	if (*p != '\0') {
	    *p = '\0';
	    p++;
	}
    }
    argv[i] = NULL;
    argc = i;

    /*
     * Parse command-line arguments.  A leading "-file" argument is
     * ignored (a historical relic from the distant past).  If the
     * next argument doesn't start with a "-" then strip it off and
     * use it as the name of a script file to process.  Also check
     * for other standard arguments, such as "-geometry", anywhere
     * in the argument list.
     */

    GetModuleFileName(NULL, argv0, 255);

    if (argc > 0) {
      length = strlen(argv[0]);
      if ((length >= 2) && (strncmp(argv[0], "-file", length) == 0)) {
	argc--;
	argv++;
      }
    }
    if ((argc > 0) && (argv[0][0] != '-')) {
      fileName = argv[0];
      argc--;
      argv++;
    }

    if (Tk_ParseArgv(interp, (Tk_Window) NULL, &argc, argv, argTable,
	    TK_ARGV_DONT_SKIP_FIRST_ARG) != TCL_OK) {
      WishPanic("%s\n", interp->result);
    }
    if (fileName != NULL) {
      p = fileName;
    } else {
      p = argv0;
    }

    /*
     * Make command-line arguments available in the Tcl variables "argc"
     * and "argv".    Also set the "geometry" variable from the geometry
     * specified on the command line.
     */

    args = Tcl_Merge(argc, argv);
    Tcl_SetVar(interp, "argv", args, TCL_GLOBAL_ONLY);
    ckfree(args);
    sprintf(buf, "%d", argc);
    Tcl_SetVar(interp, "argc", buf, TCL_GLOBAL_ONLY);
    Tcl_SetVar(interp, "argv0", (fileName != NULL) ? fileName : argv0,
	    TCL_GLOBAL_ONLY);

    Tcl_SetVar2(interp, "env", "DISPLAY", "localhost:0", TCL_GLOBAL_ONLY);

    Tcl_SetVar(interp, "tcl_interactive", "0", TCL_GLOBAL_ONLY);

    /*
     * Invoke application-specific initialization.
     */

    if (Tcl_AppInit(interp) != TCL_OK) {
	WishPanic("Tcl_AppInit failed: %s\n", interp->result);
    }

    Tcl_ResetResult(interp);

    /*
     * Loop infinitely, waiting for commands to execute.  When there
     * are no windows left, Tk_MainLoop returns and we exit.
     */

    ckfree((char *)argvlist);

    Tcl_CreateCommand(interp, "puts", PutsCmd,  (ClientData) NULL,
		      (Tcl_CmdDeleteProc *) NULL);

    setmode(fileno(stdout),O_BINARY);
    setmode(fileno(stdin),O_BINARY);

    outstream = fdopen(fileno(stdout),"wb");
    
    /* mm: do not show the main window */
    code = Tcl_Eval(interp, "wm withdraw . ");
    if (code != TCL_OK) {
      fprintf(outstream, "w %s\n.\n", interp->result);
      fflush(outstream); /* added mm */
    }

    {
      ReaderInfo *info = (ReaderInfo*) malloc(sizeof(ReaderInfo));
      unsigned tid;
      unsigned long thread;

      info->ash            = Tcl_AsyncCreate(asyncHandler,(ClientData)info);
      info->event          = CreateEvent(NULL, TRUE, FALSE, NULL);
      info->toplevelThread = GetCurrentThreadId();
      info->cmd            = NULL;

      thread =_beginthreadex(NULL,0,readerThread,info,0,&tid);
      if (thread==0) {
	fprintf(outstream, "w reader thread creation failed\n.\n");
	fflush(outstream); /* added mm */
	exit(1);
      }
    }

    watchParent();
    Tk_MainLoop();

    /*
     * Don't exit directly, but rather invoke the Tcl "exit" command.
     * This gives the application the opportunity to redefine "exit"
     * to do additional cleanup.
     */
    /*      TerminateThread(thread,0);
	    Tcl_Eval(interp, "exit");*/
    /*    exit(0);*/
    ExitProcess(0);
    return 0;
}




int
Tcl_AppInit(interp)
    Tcl_Interp *interp;		/* Interpreter for application. */
{
  if (Tcl_Init(interp) == TCL_ERROR) {
    return TCL_ERROR;
  }
  if (Tk_Init(interp) == TCL_ERROR) {
    return TCL_ERROR;
  }

  return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * WishPanic --
 *
 *	Display a message and exit.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Exits the program.
 *
 *----------------------------------------------------------------------
 */


void cdecl
WishPanic TCL_VARARGS_DEF(char *,arg1)
{
    va_list argList;
    char buf[1024];
    char *format;
    
    format = TCL_VARARGS_START(char *,arg1,argList);
    vsprintf(buf, format, argList);

    MessageBeep(MB_ICONEXCLAMATION);
    MessageBox(NULL, buf, "Fatal Error in Wish",
            MB_ICONSTOP | MB_OK | MB_TASKMODAL | MB_SETFOREGROUND);
    ExitProcess(1);
}

void cdecl idleProc(ClientData cd)
{
  ReaderInfo *ri = (ReaderInfo*) cd;
  int code;

#ifdef DEBUG
  fprintf(dbgin,"************* idleProc called:\n%s\n", ri->cmd);
  fflush(dbgin);
#endif

  code = Tcl_Eval(interp, ri->cmd);

  if (*interp->result != 0) {
    if (code != TCL_OK) {
#ifdef DEBUG
      fprintf(dbgin,"Error:  %s\n", interp->result);
      fflush(dbgin);
#endif
      fprintf(outstream,"w --- %s", ri->cmd);
      fprintf(outstream,"---  %s\n---\n.\n", interp->result);
      fflush(outstream); /* by mm */
    }
  }

  /* force readerThread to read next input */
  SetEvent(ri->event);
}


int cdecl asyncHandler(ClientData cd, Tcl_Interp *i, int code)
{
  Tcl_DoWhenIdle(idleProc,cd);
  return code;
}


static Tcl_DString command;	/* Used to assemble lines of terminal input
				 * into Tcl commands. */

static unsigned __stdcall readerThread(void *arg)
{
#define BUFFER_SIZE 4000
  ReaderInfo *ri = (ReaderInfo *)arg;
  char input[BUFFER_SIZE+1];
  int count;
  int fdin = _hdopen((int)GetStdHandle(STD_INPUT_HANDLE),O_RDONLY|O_BINARY);

  Tcl_DStringInit(&command);
    
#define TclRead read

  while(1) {

#ifdef DEBUG
    fprintf(dbgout,"before read\n"); fflush(dbgout);
#endif

    count = read(fdin, input, BUFFER_SIZE);

    if (count <= 0) {
      WishPanic("Connection to engine lost");
    }

    ri->cmd = Tcl_DStringAppend(&command, input, count);

    if ((input[count-1] != '\n') && (input[count-1] != ';') ||
	!Tcl_CommandComplete(ri->cmd)) {
      continue;
    }
  
    ResetEvent(ri->event);
    Tcl_AsyncMark(ri->ash);

    /* wake up toplevel */
    PostThreadMessage(ri->toplevelThread,WM_NULL,0,0);

    if (WaitForSingleObject(ri->event, INFINITE) != WAIT_OBJECT_0)
      WishPanic("readerThread: wait failed");

    Tcl_DStringFree(&command);
  }
  
  return 0;
}

