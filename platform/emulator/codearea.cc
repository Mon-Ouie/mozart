/*
 *  Authors:
 *    Ralf Scheidhauer (Ralf.Scheidhauer@ps.uni-sb.de)
 *
 *  Contributors:
 *    Michael Mehl (mehl@dfki.de)
 *    Benjamin Lorenz (lorenz@ps.uni-sb.de)
 *    Leif Kornstaedt (kornstae@ps.uni-sb.de)
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

#include "codearea.hh"
#include "indexing.hh"

#ifdef RECINSTRFETCH

int CodeArea::fetchedInstr = 0;
ProgramCounter CodeArea::ops[RECINSTRFETCH];



void CodeArea::recordInstr(ProgramCounter PC){
  ops[fetchedInstr] = PC;
  fetchedInstr++;
  if(fetchedInstr >= RECINSTRFETCH)
    fetchedInstr = 0;
}

#endif

HashTable CodeArea::atomTab(HT_CHARKEY,10000);
HashTable CodeArea::nameTab(HT_CHARKEY,1000);
int CodeArea::totalSize = 0; /* in bytes */
CodeArea *CodeArea::allBlocks = NULL;

#ifdef THREADED
void **CodeArea::globalInstrTable = 0;
HashTable *CodeArea::opcodeTable = 0;
#endif




inline
Literal *addToLiteralTab(const char *str, HashTable *table,
                         Bool isName, Bool needsDup) {
  Literal *found = (Literal *) table->htFind(str);

  if (found != (Literal *) htEmpty) {
    return found;
  }

  if (needsDup)
    str = strdup(str);

  if (isName) {
    found = NamedName::newNamedName(str);
  } else {
    found = Atom::newAtom(str);
  }

  table->htAdd(str,found);
  return found;
}


OZ_Term OZ_atom(OZ_CONST char *str)
{
  CHECK_STRPTR(str);
  Literal *lit=addToLiteralTab(str,&CodeArea::atomTab,NO,OK);
  return makeTaggedLiteral(lit);
}

OZ_Term oz_atomNoDup(OZ_CONST char *str) {
  CHECK_STRPTR(str);
  Literal *lit=addToLiteralTab(str,&CodeArea::atomTab,NO,NO);
  return makeTaggedLiteral(lit);
}

TaggedRef oz_uniqueName(const char *str)
{
  CHECK_STRPTR(str);
  Literal *lit = addToLiteralTab(str,&CodeArea::nameTab,OK,OK);
  lit->setFlag(Lit_isUniqueName);
  return makeTaggedLiteral(lit);
}


/*
  we store the absolute adress of the indices in the
  instruction tables
  */

#ifdef THREADED
AdressOpcode CodeArea::opcodeToAdress(Opcode oc)
{
  return ToInt32(globalInstrTable[oc]);
}


Opcode CodeArea::adressToOpcode(AdressOpcode adr)
{
  void *ret = opcodeTable->htFind(adr);
  if (ret == htEmpty) return OZERROR;
  return (Opcode) ToInt32(ret);

  /*
    for(int i = 0; i < (int) OZERROR; i++)
      if (ToInt32(globalInstrTable[i]) == adr)
        return (Opcode)i;
    return OZERROR;
  */
}

#else /* THREADED */
AdressOpcode CodeArea::opcodeToAdress(Opcode oc)  { return  oc; }
Opcode CodeArea::adressToOpcode(AdressOpcode adr) { return adr; }
#endif /* THREADED */


AbstractionEntry *AbstractionEntry::allEntries = NULL;

//
// kost@ : 'Abstraction' can be set only once!
void AbstractionEntry::setPred(Abstraction *ab)
{
  Assert(!copyable && !abstr);

  Assert(ab);
  abstr = makeTaggedConst(ab);
  pc    = ab->getPC();
  arity = ab->getArity();

  // indexing on X[0] optimized !!!
  if (pc != NOCODE &&
      CodeArea::getOpcode(pc) == MATCHX &&
      getXRegArg(pc+1) == 0) {
    indexTable = (IHashTable *) getAdressArg(pc+2);
  } else {
    indexTable = NULL;
  }
}


#define DISPATCH() PC += sizeOf(op); break



const char *getBIName(ProgramCounter PC)
{
  Builtin* entry = (Builtin*) getAdressArg(PC);
  return entry->getPrintName();
}


ProgramCounter CodeArea::printDef(ProgramCounter PC,FILE *out)
{
  ProgramCounter definitionPC = definitionStart(PC);
  if (definitionPC == NOCODE) {
    fprintf(out,"***\tspecial task or on toplevel (PC=%s)\n",
            opcodeToString(getOpcode(PC)));
    fflush(out);
    return definitionPC;
  }

  XReg reg;
  int next, line,colum;
  TaggedRef comment, predName, file;
  getDefinitionArgs(definitionPC,reg,next,file,line,colum,predName);
  getNextDebugInfoArgs(PC,file,line,colum,comment);

  const char *name = OZ_atomToC(predName);
  fprintf(out,"***\tprocedure");
  if (*name) fprintf(out," '%s'",name);
  fprintf(out," f: \"%s\" l: %d c: %d",
          OZ_atomToC(file),
          line,colum);
  fprintf(out," PC: %p\n",definitionPC);
  fflush(out);
  return definitionPC;
}

TaggedRef CodeArea::dbgGetDef(ProgramCounter PC, ProgramCounter definitionPC,
                              int frameId, RefsArray Y, Abstraction *CAP)
{
  XReg reg;
  int next, line, colum;
  TaggedRef comment, predName, file;
  // file & line might be overwritten some lines later ...
  getDefinitionArgs(definitionPC,reg,next,file,line,colum,predName);

  // if we are lucky there's some debuginfo and we can determine
  // the exact position inside the procedure application
  //--** problem: these are the coordinates of the corresponding exit
  //     instruction
  getNextDebugInfoArgs(PC,file,line,colum,comment);

  TaggedRef pairlist = oz_nil();
  pairlist =
    oz_cons(OZ_pairAI("time",findTimeStamp(PC)),
         oz_cons(OZ_pairA("data",makeTaggedConst(CAP)),
              oz_cons(OZ_pairA("file",file),
                   oz_cons(OZ_pairAI("line",line < 0? -line: line),
                        oz_cons(OZ_pairA("column",OZ_int(colum)),
                             oz_cons(OZ_pairAI("PC",(int)PC),
                                  oz_cons(OZ_pairA("kind",OZ_atom("call")),
                                       oz_cons(OZ_pairA("origin",
                                                  OZ_atom("procedureFrame")),
                                            pairlist))))))));
  if (frameId != -1)
    pairlist = oz_cons(OZ_pairAI("frameID",frameId),pairlist);
  else
    pairlist = oz_cons(OZ_pairA("vars",getFrameVariables(PC,Y,CAP)),pairlist);

  return OZ_recordInit(OZ_atom("entry"), pairlist);
}

TaggedRef CodeArea::getFrameVariables(ProgramCounter PC,
                                      RefsArray Y, Abstraction *CAP) {
  TaggedRef locals = oz_nil();
  TaggedRef globals = oz_nil();

  ProgramCounter aux = definitionEnd(PC);

  if (aux != NOCODE) {
    aux += sizeOf(getOpcode(aux));

    for (int i=0; getOpcode(aux) == LOCALVARNAME; i++) {
      if (Y) {
        TaggedRef aux1 = getLiteralArg(aux+1);
        if (!oz_eq(aux1, AtomEmpty) && Y[i] != NameVoidRegister) {
          locals = oz_cons(OZ_mkTupleC("#", 2, aux1, Y[i]), locals);
        }
      }
      aux += sizeOf(getOpcode(aux));
    }
    locals = reverseC(locals);

    int gsize=CAP->getPred()->getGSize();
    if (gsize>0) {
      for (int i=0; getOpcode(aux) == GLOBALVARNAME; i++) {
        TaggedRef aux1 = getLiteralArg(aux+1);
        if (!oz_eq(aux1, AtomEmpty)) {
          globals = oz_cons(OZ_mkTupleC("#", 2, aux1, CAP->getG(i)), globals);
        }
        aux += sizeOf(getOpcode(aux));
      }
      globals = reverseC(globals);
    }
  }

  TaggedRef pairlist =
    oz_cons(OZ_pairA("Y", locals),
         oz_cons(OZ_pairA("G", globals),
              oz_nil()));

  TaggedRef ret = OZ_recordInit(OZ_atom("v"), pairlist);
  return ret;
}

ProgramCounter CodeArea::definitionStart(ProgramCounter from)
{
  ProgramCounter ret = definitionEnd(from);
  if (ret == NOCODE)
    return ret;
  else
    return ret+getLabelArg(ret+1);
}


Bool CodeArea::getNextDebugInfoArgs(ProgramCounter PC,
                                    TaggedRef &file, int &line, int &colum,
                                    TaggedRef &comment)
{
  ProgramCounter end = definitionEnd(PC);
  if (end == NOCODE)
    return NO;

  while (PC < end) {
    Opcode op = getOpcode(PC);
    switch (op) {
    case DEBUGENTRY:
    case DEBUGEXIT:
      file    = getTaggedArg(PC+1);
      line    = OZ_intToC(getTaggedArg(PC+2));
      colum   = OZ_intToC(getTaggedArg(PC+3));
      comment = getTaggedArg(PC+4);
      return OK;
    case DEFINITION:
    case DEFINITIONCOPY:
      PC += getLabelArg(PC+2);
      break;
    case ENDOFFILE:
    case OZERROR:
      return NO;
    default:
      DISPATCH();
    }
  }
  return NO;
}


/* find the end of the definition where from points into */
ProgramCounter CodeArea::definitionEnd(ProgramCounter PC)
{
  while (1) {
    Opcode op = getOpcode(PC);
    switch (op) {
    case DEFINITION:
    case DEFINITIONCOPY:
      PC += getLabelArg(PC+2);
      break;
    case ENDDEFINITION:
      return PC;
    case TASKXCONT:
    case TASKDEBUGCONT:
    case TASKCALLCONT:
    case TASKLOCK:
    case TASKSETSELF:
    case TASKCATCH:
    case TASKEMPTYSTACK:
    case TASKPROFILECALL:
    case ENDOFFILE:
    case OZERROR:
    case GLOBALVARNAME:   // last instr in CodeArea::init
      return NOCODE;
    default:
      DISPATCH();
    }
  }
}

void displayCode(ProgramCounter from, int ssize)
{
  CodeArea::display(from,ssize,stderr);
  fflush(stderr);
}

void displayDef(ProgramCounter from, int ssize)
{
  ProgramCounter start=CodeArea::printDef(from,stderr);
  if (start != NOCODE) CodeArea::display(start,ssize,stderr,from);
}


inline
ProgramCounter computeLabelArg(ProgramCounter pc, ProgramCounter arg)
{
  return pc+getLabelArg(arg);
}


void CodeArea::getDefinitionArgs(ProgramCounter PC,
                                 XReg &reg, int &next,
                                 TaggedRef &file, int &line, int &colum,
                                 TaggedRef &predName)
{
  Assert(getOpcode(PC) == DEFINITION || getOpcode(PC) == DEFINITIONCOPY);
  PrTabEntry *pred = getPredArg(PC+3);

  reg      = XRegToInt(getXRegArg(PC+1));
  next     = getLabelArg(PC+2);
  if (pred!=NULL) {
    file     = pred->getFile();
    line     = pred->getLine();
    colum    = pred->getColumn();
    predName = OZ_atom((OZ_CONST char*)pred->getPrintName());
  } else {
    file     = AtomEmpty;
    line     = colum = 0;
    predName = AtomEmpty;
  }
}

static
void printLoc(FILE *ofile,Builtin * bi, OZ_Location *loc) {
  if (bi->getInArity()) {
    fprintf(ofile,"[");
    for (int i=0; i<bi->getInArity(); i++) {
      fprintf(ofile,"%sx(%d)",i?" ":"",loc->getIndex(i));
    }
    fprintf(ofile,"]");
  } else
    fprintf(ofile,"nil");
  fprintf(ofile,"#");
  if (bi->getOutArity()) {
    fprintf(ofile,"[");
    for (int i=0; i<bi->getOutArity(); i++) {
      fprintf(ofile,"%sx(%d)",i?" ":"",loc->getIndex(i));
    }
    fprintf(ofile,"]");
  } else
    fprintf(ofile,"nil");
}

void CodeArea::display(ProgramCounter from, int sz, FILE* ofile,
                       ProgramCounter to)
{
  ProgramCounter PC = from;
  int defCount = 0; // counter for nested definitions
  for (int i = 1; i <= sz || sz <= 0 ; i++) {

    if (sz <=0 && to != NOCODE && PC > to) {
      fflush(ofile);
      return;
    }

    fprintf(ofile, "%p:\t", PC);
    Opcode op = getOpcode(PC);
    if (op == OZERROR || op == ENDOFFILE) {
      fprintf(ofile,"End of code block reached\n");
      fflush(ofile);
      return;
    }

    fprintf(ofile, "%03d\t%s", op, opcodeToString(op));

    switch (op) {
    case SKIP:
    case RETURN:
    case DEALLOCATEL:
    case DEALLOCATEL1:
    case DEALLOCATEL2:
    case DEALLOCATEL3:
    case DEALLOCATEL4:
    case DEALLOCATEL5:
    case DEALLOCATEL6:
    case DEALLOCATEL7:
    case DEALLOCATEL8:
    case DEALLOCATEL9:
    case DEALLOCATEL10:
    case ALLOCATEL1:
    case ALLOCATEL2:
    case ALLOCATEL3:
    case ALLOCATEL4:
    case ALLOCATEL5:
    case ALLOCATEL6:
    case ALLOCATEL7:
    case ALLOCATEL8:
    case ALLOCATEL9:
    case ALLOCATEL10:
    case PROFILEPROC:
    case POPEX:
    case TASKXCONT:
    case TASKDEBUGCONT:
    case TASKCALLCONT:
    case TASKLOCK:
    case TASKSETSELF:
    case TASKCATCH:
    case TASKEMPTYSTACK:
    case TASKPROFILECALL:
      fprintf(ofile, "\n");
      DISPATCH();

    case DEBUGENTRY:
    case DEBUGEXIT:
      {
        fprintf(ofile, "(%s ",  toC(getTaggedArg(PC+1)));
        fprintf(ofile, "%s ",   toC(getTaggedArg(PC+2)));
        fprintf(ofile, "%s ",   toC(getTaggedArg(PC+3)));
        fprintf(ofile, "%s)\n", toC(getTaggedArg(PC+4)));
        DISPATCH();
      }

    case PUTLISTX:
    case FUNRETURNX:
    case SETVALUEX:
    case GETLISTX:
    case UNIFYVALUEX:
    case GETVARIABLEX:
    case SETVARIABLEX:
    case UNIFYVARIABLEX:
    case GETRETURNX:
    case CREATEVARIABLEX:
    case GETSELF:
      fprintf(ofile, "(x(%d))\n", XRegToInt(getXRegArg(PC+1)));
      DISPATCH();

    case PUTLISTY:
    case FUNRETURNY:
    case SETVALUEY:
    case GETLISTY:
    case UNIFYVALUEY:
    case GETVARIABLEY:
    case SETVARIABLEY:
    case UNIFYVARIABLEY:
    case GETRETURNY:
    case CREATEVARIABLEY:
    case CLEARY:
      fprintf(ofile, "(y(%d))\n", YRegToInt(getYRegArg(PC+1)));
      DISPATCH();

    case FUNRETURNG:
    case SETVALUEG:
    case GETLISTG:
    case UNIFYVALUEG:
    case GETRETURNG:
    case SETSELFG:
      fprintf(ofile, "(g(%d))\n", GRegToInt(getGRegArg(PC+1)));
      DISPATCH();

    case CREATEVARIABLEMOVEX:
      fprintf (ofile, "(x(%d) x(%d))\n",
               XRegToInt(getXRegArg(PC+1)),
               XRegToInt(getXRegArg(PC+2)));
      DISPATCH();

    case CREATEVARIABLEMOVEY:
      fprintf (ofile, "(y(%d) x(%d))\n",
               YRegToInt(getYRegArg(PC+1)),
               XRegToInt(getXRegArg(PC+2)));
      DISPATCH();

    case GETLISTVALVARX:
      fprintf (ofile, "(x(%d) x(%d) x(%d))\n",
               XRegToInt(getXRegArg(PC+1)),
               XRegToInt(getXRegArg(PC+2)),
               XRegToInt(getXRegArg(PC+3)));
      DISPATCH();

    case MOVEMOVEXYXY:
      fprintf (ofile, "(x(%d) y(%d) x(%d) y(%d))\n",
              XRegToInt(getXRegArg(PC+1)), YRegToInt(getYRegArg(PC+2)),
              XRegToInt(getXRegArg(PC+3)), YRegToInt(getYRegArg(PC+4)));
      DISPATCH();

    case MOVEMOVEYXXY:
      fprintf (ofile, "(y(%d) x(%d) x(%d) y(%d))\n",
              YRegToInt(getYRegArg(PC+1)), XRegToInt(getXRegArg(PC+2)),
              XRegToInt(getXRegArg(PC+3)), YRegToInt(getYRegArg(PC+4)));
      DISPATCH();

    case MOVEMOVEYXYX:
      fprintf (ofile, "(y(%d) x(%d) y(%d) x(%d))\n",
               YRegToInt(getYRegArg(PC+1)), XRegToInt(getXRegArg(PC+2)),
               YRegToInt(getYRegArg(PC+3)), XRegToInt(getXRegArg(PC+4)));
      DISPATCH();

    case MOVEMOVEXYYX:
      fprintf (ofile, "(x(%d) y(%d) y(%d) x(%d))\n",
              XRegToInt(getXRegArg(PC+1)), YRegToInt(getYRegArg(PC+2)),
              YRegToInt(getYRegArg(PC+3)), XRegToInt(getXRegArg(PC+4)));
      DISPATCH();

    case TESTLITERALX:
    case TESTNUMBERX:
      {
        TaggedRef tagged = getTaggedArg(PC+2);
        fprintf (ofile,
                 "(x(%d) %s %p)\n",
                 XRegToInt(getXRegArg(PC+1)),
                 toC(tagged),
                 computeLabelArg(PC,PC+3));
        DISPATCH();
      }

    case TESTLITERALY:
    case TESTNUMBERY:
      {
        TaggedRef tagged = getTaggedArg(PC+2);
        fprintf (ofile,
                 "(y(%d) %s %p)\n",
                 YRegToInt(getYRegArg(PC+1)),
                 toC(tagged),
                 computeLabelArg(PC,PC+3));
        DISPATCH();
      }

    case TESTLITERALG:
    case TESTNUMBERG:
      {
        TaggedRef tagged = getTaggedArg(PC+2);
        fprintf (ofile,
                 "(g(%d) %s %p)\n",
                 GRegToInt(getGRegArg(PC+1)),
                 toC(tagged),
                 computeLabelArg(PC,PC+3));
        DISPATCH();
      }

    case TESTRECORDX:
      {
        TaggedRef tagged = getTaggedArg(PC+2);
        fprintf(ofile, "(%d %s ", XRegToInt(getXRegArg(PC+1)), toC(tagged));
        SRecordArity sra = (SRecordArity) getAdressArg(PC+3);
        if (sraIsTuple(sra))
          fprintf(ofile, "%d", getTupleWidth(sra));
        else
          fprintf(ofile, "%s", toC(sraGetArityList(sra)));
        fprintf(ofile, " %p)\n", computeLabelArg(PC,PC+4));
        DISPATCH();
      }

    case TESTRECORDY:
      {
        TaggedRef tagged = getTaggedArg(PC+2);
        fprintf(ofile, "(%d %s ", YRegToInt(getYRegArg(PC+1)), toC(tagged));
        SRecordArity sra = (SRecordArity) getAdressArg(PC+3);
        if (sraIsTuple(sra))
          fprintf(ofile, "%d", getTupleWidth(sra));
        else
          fprintf(ofile, "%s", toC(sraGetArityList(sra)));
        fprintf(ofile, " %p)\n", computeLabelArg(PC,PC+4));
        DISPATCH();
      }

    case TESTRECORDG:
      {
        TaggedRef tagged = getTaggedArg(PC+2);
        fprintf(ofile, "(%d %s ", GRegToInt(getGRegArg(PC+1)), toC(tagged));
        SRecordArity sra = (SRecordArity) getAdressArg(PC+3);
        if (sraIsTuple(sra))
          fprintf(ofile, "%d", getTupleWidth(sra));
        else
          fprintf(ofile, "%s", toC(sraGetArityList(sra)));
        fprintf(ofile, " %p)\n", computeLabelArg(PC,PC+4));
        DISPATCH();
      }

    case TESTLISTX:
      {
        fprintf(ofile,
                "(%d %p)\n",
                XRegToInt(getXRegArg(PC+1)),
                computeLabelArg(PC,PC+2));
        DISPATCH();
      }

    case TESTLISTY:
      {
        fprintf(ofile,
                "(%d %p)\n",
                YRegToInt(getYRegArg(PC+1)),
                computeLabelArg(PC,PC+2));
        DISPATCH();
      }

    case TESTLISTG:
      {
        fprintf(ofile,
                "(%d %p)\n",
                GRegToInt(getGRegArg(PC+1)),
                computeLabelArg(PC,PC+2));
        DISPATCH();
      }

    case TESTBOOLX:
      {
        fprintf (ofile,
                 "(%d %p %p)\n",
                 XRegToInt(getXRegArg(PC+1)),
                 computeLabelArg(PC,PC+2),
                 computeLabelArg(PC,PC+3));
        DISPATCH();
      }

    case TESTBOOLY:
      {
        fprintf (ofile,
                 "(%d %p %p)\n",
                 YRegToInt(getYRegArg(PC+1)),
                 computeLabelArg(PC,PC+2),
                 computeLabelArg(PC,PC+3));
        DISPATCH();
      }

    case TESTBOOLG:
      {
        fprintf (ofile,
                 "(%d %p %p)\n",
                 GRegToInt(getGRegArg(PC+1)),
                 computeLabelArg(PC,PC+2),
                 computeLabelArg(PC,PC+3));
        DISPATCH();
      }

    case TESTLE:
    case TESTLT:
      fprintf (ofile,
               "(x(%d) x(%d) x(%d) %p)\n",
               XRegToInt(getXRegArg(PC+1)),
               XRegToInt(getXRegArg(PC+2)),
               XRegToInt(getXRegArg(PC+3)),
               computeLabelArg(PC,PC+4));
      DISPATCH();

    case INLINEPLUS1:
    case INLINEMINUS1:
      fprintf (ofile,
               "(x(%d) x(%d))\n",
               XRegToInt(getXRegArg(PC+1)),
               XRegToInt(getXRegArg(PC+2)));
      DISPATCH();

    case INLINEPLUS:
    case INLINEMINUS:
      {
        fprintf (ofile,
                 "(x(%d) x(%d) x(%d))\n",
                 XRegToInt(getXRegArg(PC+1)),
                 XRegToInt(getXRegArg(PC+2)),
                 XRegToInt(getXRegArg(PC+3)));
      }
      DISPATCH();

    case INLINEDOT:
      {
        TaggedRef literal = getLiteralArg(PC+2);
        fprintf (ofile,
                 "(x(%d) %s x(%d))\n",
                 XRegToInt(getXRegArg(PC+1)),
                 toC(literal),
                 XRegToInt(getXRegArg(PC+3)));
      }
      DISPATCH();

    case INLINEAT:
    case INLINEASSIGN:
      {
        TaggedRef literal = getLiteralArg(PC+1);
        fprintf (ofile,
                 "(%s x(%d))\n",
                 toC(literal),
                 XRegToInt(getXRegArg(PC+2)));
      }
      DISPATCH();

    case CALLBI: // mm2
      fprintf(ofile, "(%s ", getBIName(PC+1));
      printLoc(ofile,GetBI(PC+1),GetLoc(PC+2));
      fprintf(ofile, ")\n");
      DISPATCH();

    case TESTBI: // mm2
      fprintf(ofile, "(%s ", getBIName(PC+1));
      printLoc(ofile,GetBI(PC+1),GetLoc(PC+2));
      fprintf(ofile, " %p)\n",computeLabelArg(PC,PC+3));
      DISPATCH();

    case CALLGLOBAL:
      {
        fprintf(ofile,"(%d %d)\n"
                ,XRegToInt(getXRegArg(PC+1)),getPosIntArg(PC+2));
        DISPATCH();
      }

    case FASTCALL:
    case FASTTAILCALL:
    case CALLPROCEDUREREF:
      {
        AbstractionEntry *entry = (AbstractionEntry *) getAdressArg(PC+1);
        fprintf(ofile,"(%p[pc:%p n:%d] %d)\n",entry,
                entry->getPC(),entry->getArity(),getPosIntArg(PC+2));
        DISPATCH();
      }

    case SETPROCEDUREREF:
      {
        AbstractionEntry *entry = (AbstractionEntry *) getAdressArg(PC+1);
        fprintf(ofile,"(%p[pc:%p n:%d])\n",entry,
                entry->getPC(),entry->getArity());
        DISPATCH();
      }

    case SENDMSGX:
    case TAILSENDMSGX:
      {
        TaggedRef mn = getLiteralArg(PC+1);
        fprintf(ofile, "(%s %d ", toC(mn), XRegToInt(getXRegArg(PC+2)));
        SRecordArity sra = (SRecordArity) getAdressArg(PC+3);
        if (sraIsTuple(sra))
          fprintf(ofile, "%d", getTupleWidth(sra));
        else
          fprintf(ofile, "%s", toC(sraGetArityList(sra)));
        fprintf(ofile, " cache)\n");
        DISPATCH();
      }

    case SENDMSGY:
    case TAILSENDMSGY:
      {
        TaggedRef mn = getLiteralArg(PC+1);
        fprintf(ofile, "(%s %d ", toC(mn), YRegToInt(getYRegArg(PC+2)));
        SRecordArity sra = (SRecordArity) getAdressArg(PC+3);
        if (sraIsTuple(sra))
          fprintf(ofile, "%d", getTupleWidth(sra));
        else
          fprintf(ofile, "%s", toC(sraGetArityList(sra)));
        fprintf(ofile, " cache)\n");
        DISPATCH();
      }

    case SENDMSGG:
    case TAILSENDMSGG:
      {
        TaggedRef mn = getLiteralArg(PC+1);
        fprintf(ofile, "(%s %d ", toC(mn), GRegToInt(getGRegArg(PC+2)));
        SRecordArity sra = (SRecordArity) getAdressArg(PC+3);
        if (sraIsTuple(sra))
          fprintf(ofile, "%d", getTupleWidth(sra));
        else
          fprintf(ofile, "%s", toC(sraGetArityList(sra)));
        fprintf(ofile, " cache)\n");
        DISPATCH();
      }

    case CALLX:
    case TAILCALLX:
      {
        XReg reg = XRegToInt(getXRegArg(PC+1));
        fprintf(ofile, "(%d %d)\n", reg, getPosIntArg(PC+2));
      }
      DISPATCH();

    case CALLY:
      {
        YReg reg = YRegToInt(getYRegArg(PC+1));
        fprintf(ofile, "(%d %d)\n", reg, getPosIntArg(PC+2));
      }
      DISPATCH();

    case CALLG:
    case TAILCALLG:
      {
        GReg reg = GRegToInt(getGRegArg(PC+1));
        fprintf(ofile, "(%d %d)\n", reg, getPosIntArg(PC+2));
      }
      DISPATCH();

    case SETVOID:
    case GETVOID:
    case UNIFYVOID:
    case ALLOCATEL:
      fprintf(ofile, "(%d)\n", getPosIntArg(PC+1));
      DISPATCH();

    case UNIFYNUMBER:
      {
        fprintf(ofile, "(%s)\n", toC(getNumberArg(PC+1)));
      }
      DISPATCH();

    case GETNUMBERX:
      {
        fprintf(ofile, "(%s %d)\n", toC(getNumberArg(PC+1)),
                XRegToInt(getXRegArg(PC+2)));
      }
      DISPATCH();

    case GETNUMBERY:
      {
        fprintf(ofile, "(%s %d)\n", toC(getNumberArg(PC+1)),
                YRegToInt(getYRegArg(PC+2)));
      }
      DISPATCH();

    case GETNUMBERG:
      {
        fprintf(ofile, "(%s %d)\n", toC(getNumberArg(PC+1)),
                GRegToInt(getGRegArg(PC+2)));
      }
      DISPATCH();

    case MOVEGY:
    case UNIFYVALVARGY:
      {
        GReg reg = GRegToInt(getGRegArg(PC+1));
        fprintf(ofile, "(%d %d)\n", reg, YRegToInt(getYRegArg(PC+2)));
      }
      DISPATCH();

    case MOVEGX:
    case UNIFYVALVARGX:
      {
        GReg reg = GRegToInt(getGRegArg(PC+1));
        fprintf(ofile, "(%d %d)\n", reg, XRegToInt(getXRegArg(PC+2)));
      }
      DISPATCH();

    case MOVEYX:
    case GETVARVARYX:
    case UNIFYVALVARYX:
      {
        YReg reg = YRegToInt(getYRegArg(PC+1));
        fprintf(ofile, "(%d %d)\n", reg, YRegToInt(getYRegArg(PC+2)));
      }
      DISPATCH();

    case UNIFYXG:
      {
        XReg reg = XRegToInt(getXRegArg(PC+1));
        fprintf(ofile, "(%d %d)\n", reg, GRegToInt(getGRegArg(PC+2)));
      }
      DISPATCH();

    case MOVEXY:
    case UNIFYXY:
    case GETVARVARXY:
    case UNIFYVALVARXY:
      {
        XReg reg = XRegToInt(getXRegArg(PC+1));
        fprintf(ofile, "(%d %d)\n", reg, YRegToInt(getYRegArg(PC+2)));
      }
      DISPATCH();

    case MOVEXX:
    case UNIFYXX:
    case GETVARVARXX:
    case UNIFYVALVARXX:
      {
        XReg reg = XRegToInt(getXRegArg(PC+1));
        fprintf(ofile, "(%d %d)\n", reg, XRegToInt(getXRegArg(PC+2)));
      }
      DISPATCH();

    case SETCONSTANT:
    case UNIFYLITERAL:
    case GLOBALVARNAME:
    case LOCALVARNAME:
      {
        TaggedRef tagged = getTaggedArg(PC+1);
        fprintf(ofile, "(%s)\n", toC(tagged));
      }
      DISPATCH();

    case DEFINITION:
    case DEFINITIONCOPY:
      {
        defCount++;

        XReg reg;
        int next,line,colum;
        TaggedRef file, predName;
        getDefinitionArgs(PC,reg,next,file,line,colum,predName);
        PrTabEntry *predd = getPredArg(PC+3);
        AbstractionEntry *predEntry = (AbstractionEntry*) getAdressArg(PC+4);
        AssRegArray *list = (AssRegArray*) getAdressArg(PC+5);
        fprintf(ofile,"(x(%d) %d pid(%s",reg,next,toC(predName));
        fprintf(ofile," %d",predd->getArity());
        fprintf(ofile," pos(%s %d %d)",
                OZ_atomToC(file),line,colum);
        fprintf(ofile," [");
        fprintf(ofile,"%s",predd->isSited()?" sited":"");
        fprintf(ofile," ]");
        fprintf(ofile," %d",predd->getMaxX());
        fprintf(ofile,") ");
        if (predEntry)
          fprintf(ofile,"%p ",predEntry);
        else
          fprintf(ofile,"unit ");

        int size = list->getSize();
        if (size == 0)
          fprintf(ofile,"nil)\n");
        else {
          fprintf(ofile,"[");
          for (int k = 0; k < size; k++) {
            switch ((*list)[k].kind) {
            case K_XReg: fprintf(ofile,"x(%d)",(*list)[k].number); break;
            case K_YReg: fprintf(ofile,"y(%d)",(*list)[k].number); break;
            case K_GReg: fprintf(ofile,"g(%d)",(*list)[k].number); break;
            }
            if (k != size - 1)
              fprintf(ofile, " ");
          }
          fprintf(ofile, "])\n");
        }
      }
      DISPATCH();

    case PUTRECORDX:
    case GETRECORDX:
      {
        TaggedRef literal = getLiteralArg(PC+1);
        fprintf(ofile, "(%s ", toC(literal));
        SRecordArity sra = (SRecordArity) getAdressArg(PC+2);
        if (sraIsTuple(sra))
          fprintf(ofile, "%d", getTupleWidth(sra));
        else
          fprintf(ofile, "%s", toC(sraGetArityList(sra)));
        fprintf(ofile, " %d)\n", XRegToInt(getXRegArg(PC+3)));
      }
      DISPATCH();

    case PUTRECORDY:
    case GETRECORDY:
      {
        TaggedRef literal = getLiteralArg(PC+1);
        fprintf(ofile, "(%s ", toC(literal));
        SRecordArity sra = (SRecordArity) getAdressArg(PC+2);
        if (sraIsTuple(sra))
          fprintf(ofile, "%d", getTupleWidth(sra));
        else
          fprintf(ofile, "%s", toC(sraGetArityList(sra)));
        fprintf(ofile, " %d)\n", YRegToInt(getYRegArg(PC+3)));
      }
      DISPATCH();

    case GETRECORDG:
      {
        TaggedRef literal = getLiteralArg(PC+1);
        fprintf(ofile, "(%s ", toC(literal));
        SRecordArity sra = (SRecordArity) getAdressArg(PC+2);
        if (sraIsTuple(sra))
          fprintf(ofile, "%d", getTupleWidth(sra));
        else
          fprintf(ofile, "%s", toC(sraGetArityList(sra)));
        fprintf(ofile, " %d)\n", GRegToInt(getGRegArg(PC+3)));
      }
      DISPATCH();

    case PUTCONSTANTX:
    case GETLITERALX:
      {
        TaggedRef tagged = getTaggedArg(PC+1);

        fprintf(ofile, "(%s %d)\n", toC(tagged),
                 XRegToInt(getXRegArg(PC+2)));
      }
      DISPATCH();

    case PUTCONSTANTY:
    case GETLITERALY:
      {
        TaggedRef tagged = getTaggedArg(PC+1);

        fprintf(ofile, "(%s %d)\n", toC(tagged),
                 YRegToInt(getYRegArg(PC+2)));
      }
      DISPATCH();

    case GETLITERALG:
      {
        TaggedRef tagged = getTaggedArg(PC+1);

        fprintf(ofile, "(%s %d)\n", toC(tagged),
                 GRegToInt(getGRegArg(PC+2)));
      }
      DISPATCH();

    case ENDDEFINITION:
      fprintf(ofile, "(%p)\n", computeLabelArg(PC,PC+1));
      if (sz<=0 && defCount<=1) {
        fflush(ofile);
        return;
      }
      defCount--;
      DISPATCH();

    case BRANCH:
    case EXHANDLER:
      fprintf(ofile, "(%p)\n", computeLabelArg(PC,PC+1));
      DISPATCH();

    case MATCHX:
      {
        fprintf(ofile, "(%d ...)\n", XRegToInt(getXRegArg(PC+1)));
      }
      DISPATCH();

    case MATCHY:
      {
        fprintf(ofile, "(%d ...)\n", YRegToInt(getYRegArg(PC+1)));
      }
      DISPATCH();

    case MATCHG:
      {
        fprintf(ofile, "(%d ...)\n", GRegToInt(getGRegArg(PC+1)));
      }
      DISPATCH();

    case LOCKTHREAD:
      {
        ProgramCounter lbl = computeLabelArg(PC,PC+1);
        int n      = XRegToInt(getXRegArg(PC+2));
        fprintf(ofile, "(%p x(%d))\n", lbl, n);
      }
      DISPATCH();

    case CALLMETHOD:
      {
        CallMethodInfo *cmi = (CallMethodInfo*)getAdressArg(PC+1);
        fprintf(ofile, "(%p[ri:%d l:%s", cmi,cmi->regIndex, toC(cmi->mn));
        fprintf(ofile, " a:%s] %d)\n", toC(sraGetArityList(cmi->arity)),
                getPosIntArg(PC+2));
        DISPATCH();
      }

    case CALLCONSTANT:
      {
        fprintf(ofile, "(%s %d)\n",toC(getTaggedArg(PC+1)),getPosIntArg(PC+2));
        DISPATCH();
      }

    default:
      fprintf(ofile,"Illegal instruction");
      fflush(ofile);
      return;
    }
  }
}

#undef DISPATCH

ProgramCounter
  C_XCONT_Ptr,
  C_DEBUG_CONT_Ptr,
  C_CALL_CONT_Ptr,
  C_LOCK_Ptr,
  C_SET_SELF_Ptr,
  C_SET_ABSTR_Ptr,
  C_CATCH_Ptr,
  C_EMPTY_STACK;



CodeArea::~CodeArea()
{
#ifdef DEBUG_CHECK
  memset(getStart(),-1,size*sizeof(ByteCode));
#else
  delete [] getStart();
#endif
  gclist->dispose();
}


CodeArea::CodeArea(int sz)
{
  allocateBlock(sz);
  referenced = NO;
  gclist     = NULL;
}

void CodeArea::init(void **instrTable)
{
#ifdef THREADED
  globalInstrTable = instrTable;
  opcodeTable = new HashTable(HT_INTKEY,(int) (OZERROR*1.5));
  for (int i=0; i<=OZERROR; i++) {
    opcodeTable->htAdd(ToInt32(globalInstrTable[i]),ToPointer(i));
  }
#endif
  CodeArea *code = new CodeArea(20);
  C_XCONT_Ptr        = code->getStart();
  C_DEBUG_CONT_Ptr   = writeOpcode(TASKXCONT,       C_XCONT_Ptr);
  C_CALL_CONT_Ptr    = writeOpcode(TASKDEBUGCONT,   C_DEBUG_CONT_Ptr);
  C_LOCK_Ptr         = writeOpcode(TASKCALLCONT,    C_CALL_CONT_Ptr);
  C_SET_SELF_Ptr     = writeOpcode(TASKLOCK,        C_LOCK_Ptr);
  C_SET_ABSTR_Ptr    = writeOpcode(TASKSETSELF,     C_SET_SELF_Ptr);
  C_CATCH_Ptr        = writeOpcode(TASKPROFILECALL, C_SET_ABSTR_Ptr);
  C_EMPTY_STACK      = writeOpcode(TASKCATCH,       C_CATCH_Ptr);
  ProgramCounter aux = writeOpcode(TASKEMPTYSTACK,  C_EMPTY_STACK);
  /* mark end with GLOBALVARNAME, so definitionEnd works properly */
  (void) writeOpcode(GLOBALVARNAME,aux);
}

#ifdef RECINSTRFETCH

#define InstrDumpFile "fetchedInstr.dump"

void CodeArea::writeInstr(void){
  FILE* ofile;
  if((ofile = fopen(InstrDumpFile, "w"))){
    int i = fetchedInstr;
//    ofile=stdout;
    do {
      if (ops[i]) {
        display(ops[i], 1, ofile);
      }
      i++;
      if(i >= RECINSTRFETCH)
        i = 0;
    } while (i != fetchedInstr);
    fclose(ofile);
    fprintf(stderr,
            "Wrote the %d most recently fetched instructions in file '%s'\n",
            RECINSTRFETCH, InstrDumpFile);
    fflush(stderr);
  } else
    OZ_error("Cannot open file '%s'.", InstrDumpFile);
} // CodeArea::writeInstr
#endif

#ifdef DEBUG_CHECK
// for debugging
void printWhere(ostream &stream,ProgramCounter PC)
{
  PC = CodeArea::definitionStart(PC);

  if (PC == NOCODE) {
    stream << "in toplevel code";
  } else {
    TaggedRef file      = getLiteralArg(PC+3);
    TaggedRef line      = getNumberArg(PC+4);
    PrTabEntry *pred    = getPredArg(PC+5);

    stream << "procedure "
           << (pred ? pred->getPrintName() : "(NULL)")
           << " in file \""
           << toC(file)
           << "\", line "
           << toC(line);
  }
}
#endif


CodeArea *CodeArea::findBlock(ProgramCounter PC)
{
  CodeArea *aux = allBlocks;
  while (aux) {
    ByteCode *start = aux->getStart();
    if (start <= PC && PC < start + aux->size) {
      return aux;
    }
    aux = aux->nextBlock;
  }
  Assert(0);
  return NULL;
}


void CodeArea::unprotect(TaggedRef* t)
{
  gclist->remove(t);
}


void CodeGCList::remove(TaggedRef *t)
{
  for (CodeGCList *aux = this; aux!=NULL; aux = aux->next) {
    for (int i=0; i<codeGCListBlockSize; i++) {
      if ((TaggedRef*)aux->block[i].pc == t) {
        aux->block[i].pc  = NULL;
        aux->block[i].tag = C_FREE;
        return;
      }
    }
  }

  Assert(0);
}



ProgramCounter CodeArea::writeCache(ProgramCounter PC)
{
  InlineCache *cache = (InlineCache *) PC;
  PC = writeInt(0, PC);
  PC = writeInt(0, PC);
  cache->invalidate();
  protectInlineCache(cache);
  return PC;
}

void CodeArea::allocateBlock(int sz)
{
  size = sz + 1;
  codeBlock  = new ByteCode[size]; /* allocation via malloc! */
  writeOpcode(ENDOFFILE,codeBlock+sz); /* mark the end, so that
                                        * displayCode and friends work */
  totalSize += size * sizeof(ByteCode);
  wPtr       = codeBlock;
  nextBlock  = allBlocks;
  allBlocks  = this;
  timeStamp  = time(0);
}


// kost@ : TODO : with disappearance of the old marshaler, this must
// become a procedure returning nothing;
ProgramCounter CodeArea::writeTagged(TaggedRef t, ProgramCounter ptr)
{
  Assert(getStart()<=ptr && ptr < getStart()+size);
  ProgramCounter ret = writeWord(t,ptr);
  TaggedRef *tptr = (TaggedRef *)ptr;
  if (!needsNoCollection(*tptr)) {
    checkPtr(tptr);
    gclist = gclist->addTagged((ProgramCounter)tptr);
  }
  return ret;
}


ProgramCounter CodeArea::writeAbstractionEntry(AbstractionEntry *p, ProgramCounter ptr)
{
  ProgramCounter ret = writeAddress(p,ptr);
  checkPtr(ptr);
  gclist = gclist->addAbstractionEntry(ptr);
  return ret;
}


/*
 * OZ_ID_LOC
 *
 */

#define MAX_LOC_HOPELESS   8
#define MAX_LOC_HASH       61

#define LOC_FP_SHIFT       4
#define LOC_FP_MASK        15

OZ_LocList * OZ_Location::cache[MAX_LOC_HASH];
TaggedRef  * OZ_Location::new_map[NumberOfXRegisters];


OZ_Location * OZ_ID_LOC;

void initOzIdLoc(void) {
  OZ_Location::initCache();
  OZ_Location::initLocation();
  for (int i=NumberOfXRegisters; i--;  )
    OZ_Location::set(i,i);
  OZ_ID_LOC = OZ_Location::getLocation(NumberOfXRegisters);
}

TaggedRef OZ_Location::getArgs(Builtin * bi) {
  TaggedRef out=oz_nil();
  int i;
  for (i=bi->getOutArity(); i--; ) {
    out=oz_cons(oz_newVariable(),out);
  }
  for (i=bi->getInArity(); i--; ) {
    out=oz_cons(getInValue(i),out);
  }
  return out;
}

TaggedRef OZ_Location::getInArgs(Builtin * bi) {
  TaggedRef out=oz_nil();
  for (int i=bi->getInArity(); i--; )
    out=oz_cons(getInValue(i),out);
  return out;
}

void OZ_Location::initCache(void) {
  for (int i=MAX_LOC_HASH; i--; )
    cache[i] = (OZ_LocList *) NULL;
}

OZ_Location * OZ_Location::getLocation(int n) {
  int fp = -1;
  int sfp;

  if (n <= MAX_LOC_HOPELESS) {

    /*
     * Compute finger print:
     *   The finger print must contain the size literally!
     *
     */

    fp = 0;

    for (int i = n; i--; )
      fp = (fp << 1) + getNewIndex(i);

    sfp = fp % MAX_LOC_HASH;

    fp = (fp << LOC_FP_SHIFT) + n;

    OZ_LocList * ll = cache[sfp];

    while (ll) {

      if ((ll->loc->fingerprint >> LOC_FP_SHIFT) != (fp >> LOC_FP_SHIFT))
        goto next;

      if ((ll->loc->fingerprint & LOC_FP_MASK) < n)
        goto next;

      for (int i = n; i--; )
        if (ll->loc->map[i] != new_map[i])
          goto next;

      // Cache hit!
      return ll->loc;

    next:
      ll = ll->next;

    }

  }

  OZ_Location * l = alloc(n);

  if (fp != -1) {
    l->fingerprint = fp;
    cache[sfp]     = new OZ_LocList(l,cache[sfp]);
  }

  for (int i = n; i--; )
    l->map[i] = new_map[i];

  return l;

}
