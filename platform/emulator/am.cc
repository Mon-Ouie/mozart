/*
  Hydra Project, DFKI Saarbruecken,
  Stuhlsatzenhausweg 3, D-66123 Saarbruecken, Phone (+49) 681 302-5312
  Author: popow, mehl
  Last modified: $Date$ from $Author$
  Version: $Revision$
  State: $State$

      (main) AM::procedure;
      another things.
  ------------------------------------------------------------------------
*/

#if defined(__GNUC__) && !defined(NOPRAGMA)
#pragma implementation "am.hh"
#endif

#include "am.hh"

#include "genvar.hh"
#include "fdbuilti.hh"

AM am;

/* -------------------------------------------------------------------------
 * Init and exit AM
 * -------------------------------------------------------------------------*/

int main (int argc,char **argv)
{
  am.init(argc,argv);
  engine();
  am.exitOz(0);
  return 0;  // to make CC quiet
}

static
void usage(int /* argc */,char **argv) {
  fprintf(stderr,
          "usage: %s [-E] [-S file | -f file] [-d] [-c compiler]\n",
          argv[0]);
  exit(1);
}

static
char *getOptArg(int &i, int argc, char **argv)
{
  i++;
  if (i == argc) {
    usage(argc,argv);
    return NULL;
  }

  return argv[i];
}


static
void printBanner()
{
  version();

#ifdef DEBUG_DET
  warning("Sleep disabled.");
#endif

#ifndef TM_LP
  warning("Local propagation turned off.");
#endif

#ifdef DEBUG_CHECK
  printf("Compile Flags:"
         " DEBUG_CHECK"
#ifdef DEBUG_DET
         " DEBUG_DET"
#endif
#ifdef DEBUG_TRACE
         " DEBUG_TRACE"
#endif
#ifdef DEBUG_GC
         " DEBUG_GC"
#endif
#ifdef DEBUG_FD
         " DEBUG_FD"
#endif
#ifdef PROFILE_FD
         " PROFILE_FD"
#endif
#ifdef RECINSTRFETCH
         " RECINSTRFETCH=%d", RECINSTRFETCH
#endif
         );
#endif

  printf("\n");

#ifdef PROFILE
  printf("Compiled to support gprof-profiling.\n");
#ifdef DEBUG_DET
  printf("Deterministic scheduling.\n");
#else
  printf("Time-slice scheduling.\n");
#endif
#endif

#ifdef THREADED
  // printf("Using threaded code (abs jumps).\n");
#else
  printf("Not using threaded code.\n");
#endif
}

void AM::init(int argc,char **argv)
{
  ozconf.init();

  suspCallHandler=makeTaggedNULL();
  suspendVarList=makeTaggedNULL();

  int c;

  char *compilerName;
  if (!(compilerName = getenv("OZCOMPILER"))) {
    compilerName = OzCompiler;
  }

  char *tmp;
  if ((tmp = getenv("OZPATH"))) {
    ozconf.ozPath = tmp;
    ozconf.linkPath = tmp;
  }

  if ((tmp = getenv("OZLINKPATH"))) {
    ozconf.linkPath = tmp;
  }

  char *compilerFIFO = NULL;  // path name where to connect to
  char *precompiledFile = NULL;
  Bool quiet = FALSE;

  /* process command line arguments */
  ozconf.argV = NULL;
  ozconf.argC = 0;
  for (int i=1; i<argc; i++) {
    if (strcmp(argv[i],"-E")==0) {
      ozconf.runningUnderEmacs = 1;
      continue;
    }
    if (strcmp(argv[i],"-d")==0) {
      tracerOn();
      continue;
    }
    if (strcmp(argv[i],"-quiet")==0) {
      quiet = TRUE;
      continue;
    }
    if (strcmp(argv[i],"-c")==0) {
      compilerName = getOptArg(i,argc,argv);
      continue;
    }
    if (strcmp(argv[i],"-S")==0) {
      compilerFIFO = getOptArg(i,argc,argv);
      continue;
    }
    if (strcmp(argv[i],"-f")==0) {
      precompiledFile = getOptArg(i,argc,argv);
      continue;
    }

    if (strcmp(argv[i],"-a")==0) {
      ozconf.argC = argc-i-1;
      ozconf.argV = argv+i+1;
      break;
    }

    usage(argc,argv);
  }

  int moreThanOne = 0;
  moreThanOne += (compilerFIFO != NULL);
  moreThanOne += (precompiledFile != NULL);
  if (moreThanOne > 1) {
     fprintf(stderr,"Specify only one of '-s' and '-f' and '-S'.\n");
     usage(argc,argv);
   }

  if (quiet == FALSE) {
    printBanner();
  }

  isStandaloneF=NO;
  if (compilerFIFO != NULL) {
    compStream = connectCompiler(compilerFIFO);
  } else if (precompiledFile != NULL) {
    compStream = useFile(precompiledFile);
    isStandaloneF=OK;
  } else {
    compStream = execCompiler(compilerName);
  }

  if (compStream == NULL) {
    fprintf(stderr,"Cannot open code input\n");
    exit(1);
  }
  checkVersion();

  extern void DLinit(char *name);
  DLinit(argv[0]);

  BIfdHeadManager::initStaticData();
  BIfdBodyManager::initStaticData();

  initMemoryManagement();

// not changeable
  // SizeOfWorkingArea,NumberOfXRegisters,NumberOfYRegisters


// internal registers
  statusReg    = (StatusBit)0;
  criticalFlag = NO;
  xRegs        = allocateStaticRefsArray(NumberOfXRegisters);

  rootBoard = new Board(NULL,Bo_Root);
  rootBoard->setInstalled();
  currentBoard = NULL;
  setCurrent(rootBoard,OK);
  currentSolveBoard = (Board *) NULL;
  wasSolveSet = NO;

  initLiterals();

  initThreads();

  // builtins
  BuiltinTabEntry *entry = BIinit();
  if (!entry) {
    error("BIinit failed");
    exit(1);
  }

  initTagged();
  SolveActor::Init();

  toplevelVars = allocateRefsArray(ozconf.numToplevelVars);

  Builtin *bi = new Builtin(entry,makeTaggedNULL());
  toplevelVars[0] = makeTaggedConst(bi);

  osInit();
  ioNodes = new Board*[osOpenMax()];

  if (!isStandalone()) {
    osWatchReadFD(compStream->csfileno());
  }

  osInitSignals();
  osSetAlarmTimer(CLOCK_TICK);

  // --> make sure that we check for input from compiler
  setSFlag(IOReady);

#ifdef DEBUG_CHECK
  dontPropagate = NO;
#endif
}

void AM::checkVersion()
{
  /* check version */
  char s[100];
  char *ss = compStream->csgets(s,100);
  if (ss && ss[strlen(ss)-1] == '\n')
    ss[strlen(ss)-1] = '\0';
  if (ss != NULL && strcmp(ss,OZVERSION) != 0) {
    fprintf(stderr,"*** Wrong version from compiler\n");
    fprintf(stderr,"*** Expected: '%s', got: '%s'\n",OZVERSION,ss);
    sleep(3);
    exit(1);
  }
}

void AM::exitOz(int status)
{
  compStream->csclose();
  osKillChilds();
  exit(status);
}


void AM::suspendEngine()
{
 // ******** GC **********
  idleGC();

  ozstat.printIdle(stdout);

  if (osBlockSignals() == NO) {
    DebugCheckT(warning("suspendEngine: there are blocked signals"));
  }

  while (1) {

    if (isSetSFlag(UserAlarm)) {
      handleUser();
    }

    if (isSetSFlag(IOReady) || !compStream->bufEmpty()) {
      handleIO();
    }

    if (threadQueueIsEmpty()) {
      if (isStandalone() && !compStream->cseof()) {
        loadQuery(compStream);
        continue;
      }
    } else {
      break;
    }

    // turn off alarm if no user request
    if (userCounter == 0) {
      osSetAlarmTimer(0);
    }

    Assert(compStream->bufEmpty());

    osBlockSelect();
    setSFlag(IOReady);
  }

  if (DebugCheckT(1||) ozconf.showIdleMessage) {
    printf("running...\n");
  }

  // restart alarm
  osSetAlarmTimer(CLOCK_TICK);

  osUnblockSignals();
}


/* -------------------------------------------------------------------------
 * Unification
 * -------------------------------------------------------------------------*/


Bool AM::fastUnifyOutline(TaggedRef ref1, TaggedRef ref2, Bool prop)
{
  return fastUnify(ref1, ref2, prop);
}

// unify and manage rebindTrail
Bool AM::unify(TaggedRef t1, TaggedRef t2, Bool prop)
{
  CHECK_NONVAR(t1); CHECK_NONVAR(t2);
  rebindTrail.init(isToplevel() && prop);
  Bool result = performUnify(&t1, &t2, prop);

  // unwindRebindTrail
  TaggedRef *refPtr;
  TaggedRef value;

  while (!rebindTrail.isEmpty ()) {
    rebindTrail.popCouple(refPtr, value);
    doBind(refPtr,value);
  }

/*
  temporarily deactivated 'cause of DECLAREBI_USEINLINEFUN2

  LOCAL_PROPAGATION(Assert(localPropStore.isEmpty() ||
                           localPropStore.isInLocalPropagation()));
                           */

  return result;
}


#define Swap(A,B,Type) { Type help=A; A=B; B=help; }

Bool AM::performUnify(TaggedRef *termPtr1, TaggedRef *termPtr2, Bool prop)
{
  int argSize;
  RefsArray args1, args2;

/*
  temporarily deactivated 'cause of DECLAREBI_USEINLINEFUN2

  LOCAL_PROPAGATION(Assert(localPropStore.isEmpty() ||
                           localPropStore.isInLocalPropagation()));
                           */
start:
  DEREFPTR(term1,termPtr1,tag1);
  DEREFPTR(term2,termPtr2,tag2);

  // identical terms ?
  if (term1 == term2 &&
      (!isUVar(term1) || termPtr1 == termPtr2)) {
    return OK;
  }

  if (isAnyVar(term1)) {
    if (isAnyVar(term2)) {
      goto var_var;
    } else {
      goto var_nonvar;
    }
  } else {
    if (isAnyVar(term2)) {
      Swap(term1,term2,TaggedRef);
      Swap(termPtr1,termPtr2,TaggedRef*);
      Swap(tag1,tag2,TypeOfTerm);
      goto var_nonvar;
    } else {
      goto nonvar_nonvar;
    }
  }


 /*************/
 var_nonvar:

  if (isCVar(tag1)) {
    return tagged2CVar(term1)->unify(termPtr1, term1, termPtr2, term2, prop);
  }

  bindToNonvar(termPtr1, term1, term2, prop);
  return OK;



 /*************/
 var_var:

  /*
   * The implemented partial order for binding variables to variables is:
   *   local -> global
   *   UVAR/SVAR -> CVAR (prefer binding nonCVars to CVars)
   *   local newer -> local older
   */
  if (isNotCVar(tag1)) {
    if ( isNotCVar(tag2) && isLocalVariable(term2) &&
         /*
          * prefer also binding of UVars: otherwise if we bind SVar to UVar
          * suspensions attached to SVar will be woken, which is redundant!
          */
         (!isLocalVariable(term1) ||
          (isUVar(term2) && !isUVar(term1)) ||
           heapNewer(termPtr2,termPtr1))) {
      bind(termPtr2, term2, termPtr1, prop); /* prefer binding of newer to older variables */
    } else {
      bind(termPtr1, term1, termPtr2, prop);
    }
    return OK;
  }

  if (isNotCVar(tag2)) {
    bind(termPtr2, term2, termPtr1, prop);
    return OK;
  }

  Assert(isCVar(tag1) && isCVar(tag2));
  return tagged2CVar(term1)->unify(termPtr1,term1,termPtr2,term2,prop);



 /*************/
 nonvar_nonvar:

  if (tag1 != tag2) {
    return NO;
  }

  switch ( tag1 ) {

  case CONST:
    return tagged2Const(term1)->unify(term2,prop);

  case LTUPLE:
    {
      args1 = tagged2LTuple(term1)->getRef();
      args2 = tagged2LTuple(term2)->getRef();
      argSize = 2;
      goto unify_args;
    }

  case STUPLE:
    {
      STuple *st1 = tagged2STuple(term1);
      STuple *st2 = tagged2STuple(term2);

      if ( ! st1->compareFunctor(st2) ) {
        return NO;
      }

      argSize = st1->getSize();
      args1 = st1->getRef();
      args2 = st2->getRef();

      goto unify_args;
    }

  case SRECORD:
    {
      SRecord *sr1 = tagged2SRecord(term1);
      SRecord *sr2 = tagged2SRecord(term2);

      if (! sr1->compareFunctor(sr2)) {
        return NO;
      }

      argSize = sr1->getWidth();
      args1 = sr1->getRef();
      args2 = sr2->getRef();

      goto unify_args;
    }

  case LITERAL:
    /* literals unify if their pointers are equal */
    return NO;

  case FLOAT:
  case BIGINT:
  case SMALLINT:
    return numberEq(term1,term2);

  default:
    return NO;
  }


 /*************/
 unify_args:

  rebind(termPtr2,term1);
  for (int i = 0; i < argSize-1; i++ ) {
    if (!performUnify(args1+i,args2+i, prop)) {
      return NO;
    }
  }

  /* tail recursion optimization */
  termPtr1 = args1+argSize-1;
  termPtr2 = args2+argSize-1;
  goto start;
}

BFlag AM::isBetween(Board *to, Board *varHome)
{
 loop:
  if (to == currentBoard) return B_BETWEEN;
  if (to == varHome) return B_NOT_BETWEEN;
  to = to->getParentAndTest();
  if (!to) return B_DEAD;
  goto loop;
}

// val is used because it may be a variable which must suspend.
//  if det X then ... fi
//  X = Y
// --> if det Y then ... fi

SuspList * AM::checkSuspensionList(SVariable * var, TaggedRef taggedvar,
                                   SuspList * suspList,
                                   TaggedRef term,
                                   PropCaller calledBy)
{
  Assert(isRef(term) || !isAnyVar(term));

  SuspList * retSuspList = NULL;

  // see the reduction of solve actor by the enumeration;
  DebugCheck(dontPropagate == OK, return (suspList));

  PROFILE_CODE1(FDProfiles.inc_item(no_calls_checksusplist);)

  while (suspList) {
    PROFILE_CODE1(FDProfiles.inc_item(susps_per_checksusplist);)

    Suspension * susp = suspList->getElem();

    if (susp->isDead()) {
      suspList = suspList->dispose();
      continue;
    }

PROFILE_CODE1
  (
   if (var->getBoardFast() == am.currentBoard) {
     if (susp->getBoardFast() == am.currentBoard)
       FDProfiles.inc_item(from_home_to_home_hits);
     else
       FDProfiles.inc_item(from_home_to_deep_hits);
   } else {
     Board * b = susp->getBoardFast();
     if (b == var->getBoardFast())
       FDProfiles.inc_item(from_deep_to_home_misses);
     else if (am.isBetween(b, var->getBoardFast())==B_BETWEEN)
       FDProfiles.inc_item(from_deep_to_deep_hits);
     else
       FDProfiles.inc_item(from_deep_to_deep_misses);
   }
   )

  // already propagated susps remain in suspList
    if (! susp->isPropagated()) {
      if ((suspList->checkCondition(taggedvar, term)) &&
          (susp->wakeUp(var->getBoardFast(), calledBy))) {
        // dispose only non-resistant susps
        if (! susp->isResistant()) {
          suspList = suspList->dispose();
          continue;
        } else if (calledBy) {
          susp->markUnifySusp();
        }
      }
    } else if (calledBy && susp->isResistant() && ! susp->isUnifySusp())
      if (isBetween(susp->getBoardFast(), var->getBoardFast())!=B_NOT_BETWEEN)
        susp->markUnifySusp();

    // susp cannot be woken up therefore relink it
    SuspList * first = suspList;
    suspList = suspList->getNext();
    first->setNext(retSuspList);
    retSuspList = first;
  } // while

  return retSuspList;
}

// exception from general rule that arguments are never variables!
//  term may be an
void AM::genericBind(TaggedRef *varPtr, TaggedRef var,
                     TaggedRef *termPtr, TaggedRef term,
                     Bool prop)
     /* bind var to term;         */
{
  Assert(!isCVar(var) && !isRef(term));

  /* first step: do suspensions */
  if (prop && isSVar(var)) {

    checkSuspensionList(var, isAnyVar(term) ? makeTaggedRef(termPtr) : term,
                        pc_std_unif);

    LOCAL_PROPAGATION(Assert(localPropStore.isEmpty() ||
                             localPropStore.isInLocalPropagation()));
    DebugCheckT(Board *hb=tagged2SuspVar(var)->getBoardFast());
    Assert(!hb->isReflected());
    Assert(!hb->getSolveBoard() ||
           !hb->getSolveBoard()->isReflected());
  }

  Assert(!isUVar(var) ||
         !tagged2VarHome(var)->getBoardFast()->isReflected());

  /* second step: mark binding for non-local variable in trail;     */
  /* also mark such (i.e. this) variable in suspention list;        */
  if ( !isLocalVariable(var) || prop==NO ) {
    trail.pushRef(varPtr,var);
  } else  { // isLocalVariable(var)
    if (isSVar(var)) {
      tagged2SVar(var)->dispose();
    }
  }

  doBind(varPtr,isAnyVar(term) ? makeTaggedRef(termPtr) : term);
}


void AM::doBindAndTrail(TaggedRef v, TaggedRef * vp, TaggedRef t)
{
  trail.pushRef(vp, v);

  CHECK_NONVAR(t);
  *vp = t;
}

void AM::doBindAndTrailAndIP(TaggedRef v, TaggedRef * vp, TaggedRef t,
                             GenCVariable * lv, GenCVariable * gv,
                             Bool prop)
{
  lv->installPropagators(gv,prop);
  trail.pushRef(vp, v);

  *vp = t;
}

/*
  install every board from the currentBoard to 'n'
  and move cursor to 'n'

  algm
    find common parent board of 'to' and 'currentBoard'
    deinstall until common parent (go upward)
    install (go downward)
  pre:
     'to' ist not deref'd
     'to' may be committed, failed or discarded
   ret:
     INST_OK         installation successful
     INST_FAILED     installation of board failed
     INST_REJECTED   'to' is failed or discarded board
     */
InstType AM::installPath(Board *to)
{
  if (to->isInstalled()) {
    deinstallPath(to);
    return INST_OK;
  }

  Assert(to != rootBoard);

  Board *par=to->getParentAndTest();
  if (!par) {
    return INST_REJECTED;
  }

  InstType ret = installPath(par);
  if (ret != INST_OK) {
    return ret;
  }

  setCurrent(to);
  to->setInstalled();

  trail.pushMark();
  if (!installScript(to->getScriptRef())) {
    return INST_FAILED;
  }
  return INST_OK;
}

void AM::reduceTrailOnUnitCommit()
{
  int numbOfCons = trail.chunkSize();

  Board *bb = currentBoard;

  bb->newScript(numbOfCons);

  for (int index = 0; index < numbOfCons; index++) {
    TaggedRef *refPtr;
    TaggedRef value;
    trail.popRef(refPtr,value);

    Assert(isRef(*refPtr) || !isAnyVar(*refPtr));
    Assert(isAnyVar(value));

    bb->setScript(index,refPtr,*refPtr);

    unBind(refPtr,value);
  }
  trail.popMark();
}

// only used in deinstall
void AM::reduceTrailOnSuspend()
{
  int numbOfCons = trail.chunkSize();

  if (numbOfCons > 0) {
    Board *bb = currentBoard;
    bb->newScript(numbOfCons);

    Bool used = NO;
    // one single suspension for all
    Suspension *susp = new Suspension(bb);

    for (int index = 0; index < numbOfCons; index++) {
      TaggedRef *refPtr;
      TaggedRef value;
      trail.popRef(refPtr,value);

      Assert(isRef(*refPtr) || !isAnyVar(*refPtr));
      Assert(isAnyVar(value));

      bb->setScript(index,refPtr,*refPtr);

      TaggedRef oldVal = makeTaggedRef(refPtr);
      DEREF(oldVal,ptrOldVal,tagOldVal);

      unBind(refPtr,value);

      if (isNotCVar(value)) {
        taggedBecomesSuspVar(refPtr)->addSuspension(susp);
        used = OK;
      }

      if (isAnyVar(oldVal)) {
        // local generic variables are allowed to occure here
        Assert(!isLocalVariable (oldVal) || !isNotCVar(oldVal));

        // add susps to global non-cvars
        if(!isLocalVariable(oldVal) && isNotCVar(tagOldVal)) {
          taggedBecomesSuspVar(ptrOldVal)->addSuspension (susp);
          used = OK;
        }
      }
    }
    if (!used) {
      susp->killSusp();
    }
  }
  trail.popMark();
}


void AM::reduceTrailOnFail()
{
  int numbOfCons = trail.chunkSize();
  for (int index = 0; index < numbOfCons; index++) {
    TaggedRef *refPtr;
    TaggedRef value;
    trail.popRef(refPtr,value);
    unBind(refPtr,value);
  }
  trail.popMark();
}

/*
 * shallow guards sometimes do not bind variables but only push them
 * return the list of variable in am.suspendVarList
 */
void AM::reduceTrailOnShallow(int numbOfCons)
{
  am.suspendVarList=makeTaggedNULL();
  for (int i = 0; i < numbOfCons; i++) {
    TaggedRef *refPtr;
    TaggedRef value;
    trail.popRef(refPtr,value);

    Assert(isAnyVar(value));

    TaggedRef oldVal = makeTaggedRef(refPtr);
    DEREF(oldVal,ptrOldVal,_1);

    unBind(refPtr,value);

    /* test if only trailed to create suspension and not bound ? */
    if (refPtr!=ptrOldVal) {
      if (isAnyVar(oldVal)) {
        addSuspendVarList(makeTaggedRef(ptrOldVal));
      }
    }

    addSuspendVarList(makeTaggedRef(refPtr));
  }
  trail.popMark();
}

/* -------------------------------------------------------------------------
 * OFS
 * -------------------------------------------------------------------------*/

// Check if there exists an S_ofs (Open Feature Structure) suspension in the suspList
Bool AM::hasOFSSuspension(SuspList *suspList)
{
    while (suspList) {
        Suspension *susp=suspList->getElem();
        if (susp->isOFSSusp()) return TRUE;
        suspList = suspList->getNext();
    }
    return FALSE;
}


// Add list of features to each OFS-marked suspension list
// 'flist' has three possible values: a single feature (literal), a nonempty list of
// features, or NULL (no extra features).  'determined'==TRUE iff the unify makes the
// OFS determined.  'var' (which must be deref'ed) is used to make sure that
// features are added only to variables that are indeed waiting for features.
// This routine is inspired by am.checkSuspensionList, and must track all changes to it.
void AM::addFeatOFSSuspensionList(TaggedRef var,
                                  SuspList* suspList,
                                  TaggedRef flist,
                                  Bool determ)
{
    while (suspList) {
        Suspension *susp=suspList->getElem();

        if (susp->isDead()) {
            suspList=suspList->getNext();
            continue;
        }

        if (susp->isOFSSusp()) {
            CFuncContinuation *cont=susp->getCCont();
            RefsArray xregs=cont->getX();

            // Only add features if var and fvar are the same:
            TaggedRef fvar=xregs[0];
            DEREF(fvar,_1,_2);
            if (var!=fvar) {
                suspList=suspList->getNext();
                continue;
            }
            // Only add features if the 'kill' variable is undetermined:
            TaggedRef killl=xregs[1];
            DEREF(killl,_,killTag);
            if (!isAnyVar(killTag)) {
                suspList=suspList->getNext();
                continue;
            }

            // Add the feature or list to the diff. list in xregs[3] and xregs[4]:
            if (flist) {
                if (isLiteral(flist))
                    xregs[3]=cons(flist,xregs[3]);
                else {
                    // flist must be a list
                    Assert(isLTuple(flist));
                    TaggedRef tmplist=flist;
                    while (tmplist!=AtomNil) {
                        xregs[3]=cons(head(tmplist),xregs[3]);
                        tmplist=tail(tmplist);
                    }
                }
            }
            if (determ) {
                // FS is det.: tail of list must be bound to nil: (always succeeds)
                // Do *not* use unification to do this binding!
                TaggedRef tl=xregs[4];
                DEREF(tl,tailPtr,tailTag);
                switch (tailTag) {
                case LITERAL:
                    Assert(tl==AtomNil);
                    break;
                case UVAR:
                    doBind(tailPtr, AtomNil);
                    break;
                default:
                    Assert(FALSE);
                }
            }
        }
        suspList = suspList->getNext();
    }
}

/* -------------------------------------------------------------------------
 * MISC
 * -------------------------------------------------------------------------*/

void AM::awakeNode(Board *node)
{
  if (!node) return;

  node = node->getBoardFast();

#ifndef NEWCOUNTER
  node->decSuspCount();
#endif
  scheduleWakeup(node, NO);    // diese scheiss-'meta-logik' Dinge!!!! (KP???)
}


#ifdef DEBUG_CHECK
static Board *oldBoard = (Board *) NULL;
static Board *oldSolveBoard = (Board *) NULL;
#endif

void AM::setCurrent(Board *c, Bool checkNotGC)
{
  Assert(!c->isCommitted() && !c->isFailed());
  Assert(!checkNotGC || oldBoard == currentBoard);

  currentBoard = c;
  currentUVarPrototype = makeTaggedUVar(c);
  DebugCheckT(oldBoard=c);

  if (c->isSolve ()) {
    Assert(!checkNotGC || oldSolveBoard == currentSolveBoard);

    currentSolveBoard = c;
    wasSolveSet = OK;
    DebugCheckT (oldSolveBoard = c);
  } else if (wasSolveSet == OK) {
    Assert(!checkNotGC || oldSolveBoard == currentSolveBoard);

    currentSolveBoard = c->getSolveBoard();
    wasSolveSet = NO;
    DebugCheckT (oldSolveBoard = currentSolveBoard);
  }
}

Bool AM::loadQuery(CompStream *fd)
{
  unsigned int starttime = osUserTime();

  ProgramCounter pc;

  Bool ret = CodeArea::load(fd,pc);

  if (ret == OK && pc != NOCODE) {
    addToplevel(pc);
  }

  ozstat.timeForLoading.incf(osUserTime()-starttime);

  return ret;
}

void AM::select(int fd)
{
  if (currentBoard == rootBoard) {
    warning("select not allowed on toplevel");
    return;
  }
  if (osTestSelect(fd)) return;
  currentBoard->incSuspCount();
  Assert(currentBoard);
  ioNodes[fd]=currentBoard;
  osWatchReadFD(fd);
}

// called if IOReady (signals are blocked)
void AM::handleIO()
{
  unsetSFlag(IOReady);
  int numbOfFDs=osFirstReadSelect();
  Bool doCompiler=NO;

  // check input from compiler
  if (osNextReadSelect(compStream->csfileno())) {
    doCompiler=OK;
  }

  if (doCompiler || !compStream->bufEmpty()) {
    do {
      loadQuery(compStream);
    } while(!compStream->bufEmpty());
    numbOfFDs--;
  }

  // find the node to awake
  for ( int index = 0; numbOfFDs > 0; index++ ) {
    if ( osNextReadSelect(index) ) {
      numbOfFDs--;
      awakeNode(ioNodes[index]);
      ioNodes[index] = NULL;
    }
  }
}

// called from signal handler
void AM::checkIO()
{
  int numbOfFDs = osCheckIO();
  if (!isCritical() && (numbOfFDs > 0 || !compStream->bufEmpty())) {
    setSFlag(IOReady);
  }
}

/* -------------------------------------------------------------------------
 * Search
 * -------------------------------------------------------------------------*/

/*
 * increment/decrement the thread counter
 * in every solve board above
 * if "stable" generate a new thread "solve waker"
 * NOTE:
 *   there may be failed board in between which must be simply ignored
 */
void AM::incSolveThreads(Board *bb,int n)
{
  while (!bb->isRoot()) {
    Assert(!bb->isCommitted());
    if (bb->isSolve()) {
      Assert(!bb->isReflected());
      SolveActor *sa = SolveActor::Cast(bb->getActor());
      if (!sa->isCommitted()) { // notification board below failed solve
        sa->incThreads(n);
        if (isStableSolve(sa)) {
          scheduleSolve(bb);
        }
      }
    }
    bb = bb->getParentFast();
  }
}

void AM::decSolveThreads(Board *bb)
{
  incSolveThreads(bb,-1);
}

/*
  find "stable" board
  */
Board *AM::findStableSolve(Board *bb)
{
  while (!bb->isRoot()) {
    Assert(!bb->isCommitted() && !bb->isFailed());
    if (bb->isSolve()) {
      Assert(!bb->isReflected());
      SolveActor *sa = SolveActor::Cast(bb->getActor());
      Assert(!sa->isCommitted());
      if (isStableSolve(sa)) {
        return bb;
      }
    }
    bb = bb->getParentFast();
  }
  return 0;
}

void AM::setExtSuspension(Board *varHome, Suspension *susp)
{
  Board *bb = currentBoard;
  Bool wasFound = NO;
  Assert(!varHome->isCommitted());

  while (bb != varHome) {
    Assert(!bb->isRoot());
    Assert(!bb->isCommitted() && !bb->isFailed());
    if (bb->isSolve()) {
      SolveActor *sa = SolveActor::Cast(bb->getActor());
      sa->addSuspension(susp);
      wasFound = OK;
    }
    bb = bb->getParentFast();
  }
  if (wasFound == OK)
    susp->setExtSusp ();
}

// expects susp to be external
Bool AM::_checkExtSuspension (Suspension *susp)
{
  Assert(susp->isExtSusp());

  Board *sb = susp->getBoardFast();

  sb = sb->getSolveBoard();

  Bool wasFound = (sb != 0);
  while (sb) {
    Assert(sb->isSolve());

    SolveActor *sa = SolveActor::Cast(sb->getActor());
    if (isStableSolve(sa)) {
      scheduleSolve(sb);
      /*
       * Note:
       *  The observation is that some actors which have
       *  imposed instability could be discarded by reduction
       *  of other such actors. It means, that the stability
       *  condition can not be COMPLETELY controlled by the
       *  absence of active threads;
       * Note too:
       *  If the node is not yet stable, it means that there
       *  are other external suspension(s) and/or threads.
       *  Therefore it need not be waked.
       */
    }
    sb=sa->getBoardFast()->getSolveBoard();
  }
  return wasFound;
}

//  'OZ_CFun' solveActorWaker;
// No arguments actually, but the type 'OZ_CFun' is fixed;
OZ_Bool AM::SolveActorWaker(int n, TaggedRef *args)
{
  Assert(n == 0);
  Board *bb = am.currentBoard;
  Assert(bb && bb->isSolve());
  Assert(!bb->isCommitted() && !bb->isFailed());
  SolveActor *sa = SolveActor::Cast(bb->getActor());
  // DebugCheckT (message ("SolveActor::Waker (@0x%x)\n", (void *) sa));

  Assert(!bb->isReflected());

  sa->decThreads ();      // get rid of threads - '1' in creator;
  // after return we are going to the "reduce" state,
  // so reduce the actor if possible;
  return PROCEED;    // always;
}

Bool AM::isStableSolve(SolveActor *sa)
{
  if (sa->getThreads() != 0)
    return NO;
  if (sa->getSolveBoard() == currentBoard &&
      !trail.isEmptyChunk())
    return NO;
  // simply "don't worry" if in all other cases it is too weak;
  return sa->areNoExtSuspensions();
}

/* ------------------------------------------------------------------------
 * Threads
 *
 * runnable queue: threadsHead, threadsTail
 * memory optimization: threadsFreeList
 * running thread: currentThread
 * toplevel: rootThread and toplevelQueue
 * ------------------------------------------------------------------------ */

void AM::initThreads()
{
  threadsHead     = (Thread *) NULL;
  threadsTail     = (Thread *) NULL;
  threadsFreeList = (Thread *) NULL;

  currentThread   = (Thread *) NULL;

  rootThread      = newThread(ozconf.defaultPriority,rootBoard);
  toplevelQueue   = (Toplevel *) NULL;
}

void AM::pushDebug(Board *n, Chunk *def, int arity, RefsArray args)
{
  currentThread->pushDebug(n,new OzDebug(def,arity,args));
}

void AM::scheduleSuspCont(Board *bb, int prio, Continuation *c,
                          Bool wasExtSusp)
{
  Thread *th = newThread(prio,bb);
  if (currentSolveBoard != (Board *) NULL || wasExtSusp == OK) {
    incSolveThreads(bb);
    th->setNotificationBoard(bb);
  }
  th->pushCont(bb,c->getPC(),c->getY(),c->getG(),
               c->getX(),c->getXSize(), NO);
  scheduleThread(th);
}

void AM::scheduleSuspCCont(Board *bb, int prio,
                           CFuncContinuation *c, Bool wasExtSusp,
                           Suspension *s)
{
  Thread *th = newThread(prio,bb);
  if (currentSolveBoard != (Board *) NULL || wasExtSusp == OK) {
    incSolveThreads(bb);
    th->setNotificationBoard(bb);
  }
  th->pushCFunCont(bb,c->getCFunc(),s,c->getX(),c->getXSize(),NO);
  scheduleThread(th);
}


// create a new thread to reduce a solve actor;
void AM::scheduleSolve(Board *bb)
{
  Assert(!bb->isCommitted() && !bb->isFailed() && bb->isSolve());

  // message("ScheduleSolve (@0x%x)\n", (void *) bb->getActor ());

  Actor *aa=bb->getActor();
  Thread *th = newThread(aa->getPriority(),bb);
  Board *nb = aa->getBoardFast()->getSolveBoard();
  if (nb) incSolveThreads(nb);
  th->setNotificationBoard(nb);
  th->pushNervous(bb);
  bb->setNervous();
  scheduleThread(th);
}

// create a new thread after wakeup (nervous)
void AM::scheduleWakeup(Board *bb, Bool wasExtSusp)
{
  Assert(!bb->isCommitted());
  Thread *th = newThread(bb->getActor()->getPriority(),bb);
  if (currentSolveBoard != (Board *) NULL || wasExtSusp == OK) {
    incSolveThreads(bb);
    th->setNotificationBoard(bb);
  }
  th->pushNervous(bb);
  bb->setNervous();
  scheduleThread(th);
}


/*
 * Toplevel is a queue of toplevel queries, which must be executed
 * sequentially.
 *   see checkToplevel()  called when disposing a thread
 *       addToplevel()    called when a new query arrives
 */
class Toplevel {
public:
  ProgramCounter pc;
  Toplevel *next;
  Toplevel(ProgramCounter p, Toplevel *nxt) : pc(p), next(nxt) {}
};

/*
 * push a new toplevel continuation to the rootThread
 * PRE: the rootThread is empty
 * called from checkToplevel and addToplevel
 */
void AM::pushToplevel(ProgramCounter pc)
{
  Assert(rootThread->isEmpty());
  rootBoard->incSuspCount();
  rootThread->pushCont(rootBoard,pc,toplevelVars);
  if (rootThread!=currentThread && !isScheduled(rootThread)) {
    scheduleThread(rootThread);
  }
}

/*
 * in dispose: check if there more toplevel tasks
 */
void AM::checkToplevel()
{
  if (toplevelQueue) {
    Verbose((VERB_THREAD,"checkToplevel: pushNext: 0x%x\n",this));
    Toplevel **last;
    for (last = &toplevelQueue;
         ((*last)->next) != NULL;
         last = &((*last)->next)) {
    }
    pushToplevel((*last)->pc);
    *last = (Toplevel *) NULL;
  }
}

void AM::addToplevel(ProgramCounter pc)
{
  if (rootThread->isEmpty()) {
    Verbose((VERB_THREAD,"addToplevel: push\n"));
    pushToplevel(pc);
  } else {
    Verbose((VERB_THREAD,"addToplevel: delay\n"));
    toplevelQueue = new Toplevel(pc,toplevelQueue);
  }
}

/* -------------------------------------------------------------------------
 * Signals
 * -------------------------------------------------------------------------*/

void handlerUSR1()
{
  message("Error handler entered ****\n");

  CodeArea::writeInstr();

  tracerOn(); trace("halt");
  message("Error handler exit ****\n");
}

void handlerINT()
{
  message("SIG INT ****\n");
  am.exitOz(1);
}

void handlerTERM()
{
  message("SIG TERM ****\n");
  am.exitOz(0);
}

void handlerMessage()
{
  message("SIGNAL inside signal handler ***\n");
}

void handlerSEGV()
{
  CodeArea::writeInstr();
  message("**** segmentation violation ****\n");
  longjmp(am.engineEnvironment,SEGVIO);
}

void handlerBUS()
{
  CodeArea::writeInstr();
  message("**** bus error ****\n");
  longjmp(am.engineEnvironment,BUSERROR);
}

void handlerPIPE()
{
  message("write on a pipe or other socket with no one to read it ****\n");
}

void handlerCHLD()
{
  DebugCheckT(message("a child process' state changed ****\n"));
}

void handlerFPE()
{
  OZ_warning("signal: floating point exception");
}

// signal handler
void handlerALRM()
{
  am.handleAlarm();
}

/* -------------------------------------------------------------------------
 * Alarm handling
 * -------------------------------------------------------------------------*/

void AM::handleAlarm()
{
  if (isCritical()) {  /* wait for next ALRM signal */
    return;
  }

  if (threadSwitchCounter > 0) {
    if (--threadSwitchCounter == 0) {
      setSFlag(ThreadSwitch);
    }
  }

  if (userCounter > 0) {
    if (--userCounter <= 0) {
      setSFlag(UserAlarm);
    }
  }

  checkGC();

  checkIO();
}

/* handleUserAlarm:
    if UserAlarm-SFLAG is set this method is called
    interrupts should already be disabled by the parent procedure
    */
void AM::handleUser()
{
  unsetSFlag(UserAlarm);
  int nextMS = wakeUser();
  userCounter = osMsToClockTick(nextMS);
}

int AM::setUserAlarmTimer(int ms)
{
  osBlockSignals();

  int ret=osClockTickToMs(userCounter);
  userCounter=osMsToClockTick(ms);

  osUnblockSignals();

  return ret;
}

class Sleep {
public:
  Sleep *next;
  int time;
  TaggedRef node;
public:
  Sleep(int t, TaggedRef n,Sleep *a)
  : time(t), node(n), next(a) {
    OZ_protect(&node);
  }
  ~Sleep() {
    OZ_unprotect(&node);
  }
};

void AM::insertUser(int t,TaggedRef node)
{
  if (sleepQueue) {
    int rest = setUserAlarmTimer(0);
    Assert(isSetSFlag(UserAlarm) || rest > 0);

    if (rest <= t) {
      setUserAlarmTimer(rest);
      t = t-rest;
    } else {
      setUserAlarmTimer(t);
      sleepQueue->time = rest - t;
      t = 0;
    }
  } else {
    setUserAlarmTimer(t);
    t = 0;
  }

  Sleep **prev = &sleepQueue;
  for (Sleep *a = *prev; a; prev = &a->next, a=a->next) {
    if (t <= a->time) {
      a->time = a->time - t;
      *prev = new Sleep(t,node,a);
      return;
    }
  }
  *prev = new Sleep(t,node,NULL);
}

int AM::wakeUser()
{
  if (!sleepQueue) {
    OZ_warning("wake: nobody to wake up");
    return 0;
  }
  while (sleepQueue && sleepQueue->time==0) {
    awakeNode((Board *) tagged2Const(sleepQueue->node));
    Sleep *tmp = sleepQueue->next;
    delete sleepQueue;
    sleepQueue = tmp;
  }
  if (sleepQueue) {
    int ret = sleepQueue->time;
    sleepQueue->time = 0;
    return ret;
  }
  return 0;
}


#ifdef OUTLINE
#define inline
#include "am.icc"
#undef inline
#endif



// ---------------------------------------------------------------------
