/*
  Hydra Project, DFKI Saarbruecken,
  Stuhlsatzenhausweg 3, W-66123 Saarbruecken, Phone (+49) 681 302-5312
  Author: scheidhr & mehl
  Last modified: $Date$ from $Author$
  Version: $Revision$
  State: $State$
  */

/*
 * rudimentary debug support
 */


#if defined(__GNUC__) && !defined(NOPRAGMA)
#pragma implementation "debug.hh"
#endif

#include <string.h>
#include <signal.h>
#include <setjmp.h>

#include "am.hh"
#include "debug.hh"

unsigned long OzDebug::goalCounter = 1;

// if this is OK we trace every call, otherwise only those abstractions
// with spy points on them, should be member of an own class
static Bool stepMode = NO;  
static Bool skipMode = NO;  

OZ_C_proc_begin(BItraceOn, 0)
{
  stepMode = OK;
  return PROCEED;
}
OZ_C_proc_end

OZ_C_proc_begin(BItraceOff, 0)
{
  stepMode = NO;
  return PROCEED;
}
OZ_C_proc_end


void dbgMessage(char *s)
{
  fprintf(stderr,s);
  fflush(stderr);
}

void dbgPrint(TaggedRef t)
{
  taggedPrint(t,ozconf.printDepth);
}


OZ_C_proc_begin(BIspy, 1)
{
  NONVAR(OZ_getCArg(0),predd,predTag);
  if (!isAbstraction(predd)) {
    OZ_warning("spy: abstraction expected, got: %s",OZ_toC(predd));
    return FAILED;
  }

  ((Abstraction *)tagged2Const(predd))->getPred()->setSpyFlag();
  return PROCEED;
}
OZ_C_proc_end


OZ_C_proc_begin(BIdisplayCode, 2)
{
  OZ_declareIntArg("displayCode",0,pc);
  OZ_declareIntArg("displayCode",1,size);
  displayCode((ProgramCounter)ToPointer(pc),size);
  return PROCEED;
}
OZ_C_proc_end


OZ_C_proc_begin(BInospy, 1)
{
  NONVAR(OZ_getCArg(0),predd,predTag);
  if (!isAbstraction(predd)) {
    OZ_warning("nospy: abstraction expected, got: %s",OZ_toC(predd));
    return FAILED;
  }

  ((Abstraction *)tagged2SRecord(predd))->getPred()->unsetSpyFlag();
  return PROCEED;
}
OZ_C_proc_end



static Bool isSpied(Chunk *def)
{
  if (isAbstraction(def)) {
    return ((Abstraction*)def)->getPred()->getSpyFlag();
  }    
  return NO;
}

static char *getPrintName(Chunk *def)
{
  if (isAbstraction(def)) {
    return ((Abstraction*)def)->getPrintName();
  }

  return ((Builtin*)def)->getPrintName();
}


static void setSpyFlag(Chunk *def)
{
  if (isAbstraction(def)) {
    ((Abstraction*)def)->getPred()->setSpyFlag();
  } else {
    fprintf(stderr,"Cannot set spy flag for builtins\n");
  }
}

static void unsetSpyFlag(Chunk *def)
{
  if (isAbstraction(def)) {
    ((Abstraction*)def)->getPred()->unsetSpyFlag();
  }
}


char *ternaryInfixes [] = {
  "+", "-", "*", "mod", "div", ".", "/",
  NULL
  };

char *binaryInfixes [] = {
  "<", "=<", ">", ">=", "=",
  NULL
  };


Bool isInTable(Chunk *def, char **table)
{
  char *pn = getPrintName(def);
  for (char **i=table; *i; i++) {
    if (strcmp(*i,pn) == 0) {
      return OK;
    }
  }

  return NO;
}

typedef enum {
  PORT_ENTER,
  PORT_EXIT,
  PORT_FAIL,
  PORT_SUSPEND,
  PORT_ELSE,
  PORT_LAST
} DBGPort;


static char *portMap[PORT_LAST];


static int initPortMap()
{
  portMap[PORT_EXIT]    = "Exit:    ";
  portMap[PORT_FAIL]    = "Fail:    ";
  portMap[PORT_ENTER]   = "Enter:   ";
  portMap[PORT_SUSPEND] = "Suspend: ";
  portMap[PORT_ELSE]    = "         ";
  return 1;
}


static int dummy = initPortMap();


void printCall(DBGPort port, Chunk *def, int arity, TaggedRef *argss,
	       unsigned long goal)
{
//  prefixError(); // to show up emulator buffer in emacs
  
  fprintf(stderr,"\t%c %s%5d ",
	  isSpied(def) ? '*' : ' ',
	  portMap[port],
	  goal);

  if (arity == 3 && isInTable(def,ternaryInfixes)) {
    // print it as "X + Y = Z"
    dbgPrint(argss[0]);
    fprintf(stderr," %s ",getPrintName(def));
    dbgPrint(argss[1]);
    fprintf(stderr," = ");
    dbgPrint(argss[2]);
    fprintf(stderr," ? ");
    return;
  }
  
  if (arity == 2 && isInTable(def,binaryInfixes)) {
    // print it as "X < Y"
    dbgPrint(argss[0]);
    fprintf(stderr," %s ",getPrintName(def));
    dbgPrint(argss[1]);
    fprintf(stderr," ? ");
    return;
  }
  
  fprintf(stderr,"{%s", getPrintName(def));
  for(int i=0; i<arity; i++) {
    dbgMessage(" ");    
    dbgPrint(argss[i]);
  }

  if (port != PORT_ELSE) {
    dbgMessage("} ? ");
  } else {
    dbgMessage("}\n");
  }
}


void OzDebug::printCall()
{
  ::printCall(PORT_ELSE, pred, getRefsArraySize(args), args, goalNum);
}


typedef enum {
  DBG_NONE,
  DBG_SKIP,
  DBG_LEAP,
  DBG_EMUL,
  DBG_CREEP,
  DBG_DEPTH,
  DBG_STACK,
  DBG_SPY,
  DBG_NOSPY,
  DBG_HELP,
  DBG_NODBG
} dbgOption;


const struct {
  char *command;
  dbgOption val;
} commandArray[] = {
     {"s", DBG_SKIP},
     {"l", DBG_LEAP},
     {"m", DBG_EMUL},
     {"c", DBG_CREEP},
     {"",  DBG_CREEP},
     {"+", DBG_SPY},
     {"-", DBG_NOSPY},
     {"h", DBG_HELP},
     {"?", DBG_HELP},
     {"n", DBG_NODBG},
     {NULL, DBG_NONE}
   };


dbgOption dbgReadInput(int &helpArg)
{
  char buf[1000];
  gets(buf);
  for (int i=0; commandArray[i].command; i++) {
    if (strcmp(buf,commandArray[i].command) == 0) {
      return commandArray[i].val;
    }
  }
  
  if (strncmp(buf,"<",1) == 0) {
    if (sscanf(buf+1,"%d",&helpArg) != -1) {
      return DBG_DEPTH;
    }
  }

  if (strncmp(buf,"g",1) == 0) {
    if (sscanf(buf+1,"%d",&helpArg) != -1) {
      return DBG_STACK;
    }
  }

  return DBG_NONE;
}

void showCall(DBGPort port, Board *b, Chunk *def, int arity, TaggedRef *args,
	      unsigned long goal)
{
  Bool goon = OK;
  while(goon) {
    printCall(port, def, arity, args, goal);
    int help=0;

    switch (dbgReadInput(help)) {

    case DBG_SPY:
      setSpyFlag(def);
      break;

    case DBG_NOSPY:
      unsetSpyFlag(def);
      break;

    case DBG_NODBG:
      stepMode = NO;
      skipMode = NO;
      am.unsetSFlag(DebugMode);
      return;

    case DBG_DEPTH:
      fprintf(stderr,"   Switching print depth to %d\n",help);
      ozconf.printDepth = help;
      break;

    case DBG_STACK:
      am.currentThread->printTaskStack(NULL,NO,help);
      break;

    case DBG_EMUL:
      tracerOn(); trace("debug");
      break;

    case DBG_SKIP:
      if (port == PORT_EXIT) {
	fprintf(stderr, "   skip not applicable to this port\n");
      } else {
	if (isAbstraction(def)) {
	  skipMode  = OK;
	  stepMode = NO;
	}
	goon = NO;
      }
      break;

    case DBG_CREEP:
      stepMode = OK;
      skipMode  = NO;
      goon = NO;
      break;

    case DBG_LEAP:
      stepMode = NO;
      skipMode  = NO;
      goon = NO;
      break;

    case DBG_NONE:
      dbgMessage("\n   Command not understood, possible values:\n");
    case DBG_HELP:
      dbgMessage("\n\tl       leap\n");
      dbgMessage("\t<cr>    creap\n");
      dbgMessage("\ts       skip\n");
      dbgMessage("\t+       spy this\n");
      dbgMessage("\t-       nospy this\n");
      dbgMessage("\tn       nodebug\n");
      dbgMessage("\tm       machine debugger\n");
      dbgMessage("\t< <n>   set print depth to <n>\n\n");
      break;

    default:
      error("never go here");
      break;
    }
  }

  if (port == PORT_ENTER &&
      isAbstraction(def)) {
    am.pushDebug(def,arity,args);
  }
}


void enterCall(Board *b, Chunk *def, int arity, TaggedRef *args)
{
  if (skipMode ||
      stepMode == NO && isSpied(def) == NO) {
    return;
  }

  showCall(PORT_ENTER,b,def,arity,args,OzDebug::goalCounter);
}



void exitCall(OZ_Bool bol, OzDebug *deb)
{ 
  if (skipMode == NO &&
      stepMode == NO &&
      isSpied(deb->pred) == NO ||
      !am.isSetSFlag(DebugMode)) {
    return;
  }

  DBGPort port = PORT_EXIT;
  if (bol == FAILED) 
    port = PORT_FAIL;
  showCall(port,NULL,deb->pred,
	   deb->args ? getRefsArraySize(deb->args) : 0,
	   deb->args,
	   deb->goalNum);

  delete deb;
}

void exitBuiltin(OZ_Bool bol, Builtin *bi, int arity, TaggedRef *args)
{
  if (stepMode == OK && am.isSetSFlag(DebugMode)) {
    exitCall(bol, new OzDebug(bi,arity,args));
  }
}


/*
 * the machine level debugger starts here
 */

#define MaxLine 100

inline	void printLong(TaggedRef term, int depth) {
  taggedPrintLong(term, depth ? depth : ozconf.printDepth);
}

inline	void printShort(TaggedRef term) {
  taggedPrint(term);
}

static Bool mode=NO;

void tracerOn()
{
  mode = OK;
}

void tracerOff()
{
  mode = NO;
}

Bool trace(char *s,Board *board,Actor *actor,
	   ProgramCounter PC,RefsArray Y,RefsArray G)
{
  static char command[MaxLine];

  if (!mode) {
    return OK;
  }
  if (!board) {
    board = am.currentBoard;
  }
  if (PC != NOCODE) {
    CodeArea::display(PC, 1);
  }
  board->printDebug();
  if (actor) {
    printf(" -- ");
    actor->print();
  }
  while (1) {
    printf("\nBREAK");
    if (am.isSetSFlag()) {
      printf("[S:0x%x=");
      if (am.isSetSFlag(ThreadSwitch)) {
	printf("P");
      }
      printf("]");
    }
    printf(" %s> ",s);
    fflush(stdout);
    if (fgets(command,MaxLine,stdin) == (char *) NULL) {
      printf("read no input\n");
      printf("exit\n");
      sprintf(command,"e\n");
    } else if (feof(stdin)) {
      clearerr(stdin);
      printf("exit\n");
      sprintf(command,"e\n");
    }
    if (command[0] == '\n') {
      sprintf(command,"s\n");
    }
    char *c = &command[1];
    while (*c != '\n') c++;
    *c = '\0';

    switch (command[0]) {
    case 'a':
      printAtomTab();
      break;
    case 'b':
      builtinTab.print();
      break;
    case 'c':
      mode = NO;
      return OK;
    case 'd':
      if (PC != NOCODE) {
	CodeArea::display(CodeArea::definitionStart(PC),1,stdout);
      }
      break;
    case 'e':
      printf("*** Leaving Oz\n");
      am.exitOz(0);
    case 'f':
      return NO;
    case 'p':
      board->printLongDebug();
      break;
    case 's':
      mode = OK;
      return OK;
    case 't':
      am.currentThread->printLong(cout,10,0);
      break;
    case 'A':
      am.print();
      break;
    case 'B':
      am.printBoards();
      break;
    case 'D':
      {
	static ProgramCounter from=0;
	static int size=0;
	sscanf(&command[1],"%x %d",&from,&size);
	if (size == 0)
	  size = 10;
	CodeArea::display(from,size);
      }
      break;
    case 'M':
      {
	ProgramCounter from=0;
	int size=0;
	sscanf(&command[1],"%x %d",&from,&size);
	printf("%x:",from);
	for (int i = 0; i < 20; i++)
	  printf(" %d",CodeArea::getWord(from+i));
	printf("\n%x:\n",from+20);
      }
      break;
    case 'T':
      am.printThreads();
      break;

    case '?':
      {
// CC does not like strings with CR
	static char *help[]
	  = {"\nOnline Help:",
	     "^D      = e",
	     "RET     = s",
	     "a       print atom tab",
	     "b       print builtin tab",
	     "c       continue (debug mode off)",
	     "d       display current definition",
	     "e       exit oz",
	     "f       fail",
	     "p       print current board (long)",
	     "s       continue with full debug mode",
	     "t       print current taskstack",
	     "A	print class AM",
	     "D %x %d Display code",
	     "M %x %d view Memory <from> <len>",
	     "T       print class Thread",
	     "?       this help",
	     "\nin emulate mode additionally",
	     "d %d    display current line(s) code",
	     "x %d    display X register",
	     "X %d    display X register (long)",
	     "        for y,g also",
	     NULL};
	for (char **s = help; *s; s++) {
	  printf("%s\n",*s);
	}
	break;
      }
    default:
      if (PC != NOCODE) {
	switch (command[0]) {
	case 'd':
	  {
	    int size=0;
	    sscanf(&command[1],"%d",&size);
	    CodeArea::display(PC, size ? size : 10);
	  }
	  break;
	case 'g':
	  {
	    int numb=0;
	    sscanf(&command[1],"%d %d",&numb);
	    printf ("G[%d] = ", numb);
	    printShort(G[numb]);
	    printf ("\n");
	  }
	  break;
	case 'G':
	  {
	    int numb=0,depth=0;
	    sscanf(&command[1],"%d %d",&numb,&depth);
	    printf ( "G[%d]:\n", numb);
	    printLong(G[numb],depth);
	    printf ( "\n");
	  }
	  break;
	case 'x':
	  {
	    int numb=0,depth=0;
	    sscanf(&command[1],"%d %d",&numb,&depth);
	    printf ("X[%d] = ", numb);
	    printShort(am.xRegs[numb]);
	    printf ("\n");
	  }
	  break;
	case 'X':
	  {
	    int numb=0,depth=0;
	    sscanf(&command[1],"%d %d",&numb,&depth);
	    printf ("X[%d]:\n", numb);
	    printLong(am.xRegs[numb],depth);
	  }
	  break;
	case 'y':
	  {
	    int numb=0;
	    sscanf(&command[1],"%d",&numb);
	    printf ("Y[%d] = ", numb);
	    printShort(Y[numb]);
	    printf ("\n");
	  }
	  break;
	case 'Y':
	  {
	    int numb=0,depth=0;
	    sscanf(&command[1],"%d %d",&numb,&depth);
	    printf ("Y[%d]:\n", numb);
	    printLong(Y[numb],depth);
	  }
	  break;
	default:
	  printf("unknown emulate command '%s'\n",command);
	  printf("help with '?'\n");
	  break;
	}
      } else {
	printf("unknown command '%s'\n",command);
	printf("help with '?'\n");
      }
      break;
    }
  }
}
