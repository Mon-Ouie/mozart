/*
 *  Authors:
 *    Michael Mehl (mehl@dfki.de)
 *
 *  Contributors:
 *    optional, Contributor's name (Contributor's email address)
 *
 *  Copyright:
 *    Michael Mehl (1997)
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

#include "am.hh"
#include "indexing.hh"

#ifdef FASTREGACCESS
#define GETREGARG(pc)   (getRegArg(pc)>>2)
#else
#define GETREGARG(pc)   getRegArg(pc)
#endif

#define ISREAD(i) { int _i=(i); if (_i<maxX && xUsage[_i]==0) xUsage[_i]=1; }

#define ISREAD_TO(args) { for (int _j=0;_j<args; _j++) { ISREAD(_j); } }

#define ISWRITE(i)                                      \
{                                                       \
  int _i = (i);                                         \
  if (_i<maxX && xUsage[_i]==0) {                       \
    current->writer=new Writer(_i,current->writer);     \
    xUsage[_i]=-1;                                      \
  }                                                     \
}

#define ISLOC(bi,loc)                                                   \
{                                                                       \
  for (int _j=0;_j<bi->getInArity(); _j++) { ISREAD(loc->in(_j)); }     \
  for (int _j=0;_j<bi->getOutArity(); _j++) { ISWRITE(loc->out(_j));    \
  }                                                                     \
}

#define BREAK           current->pcEnd = PC; goto outerLoop;

#define PUSH(offset)     todo = new Segment(PC+offset,todo,current->writer);

#define CONTINUE(newpc) PUSH(newpc); current->pcEnd = PC; goto outerLoop2;


class Writer {
public:
  int i;
  Writer *next;
  Writer(int i,Writer *next) : i(i),next(next) {}
};

class Segment {
public:
  ProgramCounter pc;
  ProgramCounter pcEnd;
  Writer *writer;
  Segment *next;
  Segment(ProgramCounter pc,Segment *next, Writer *w);
  ~Segment();
};


Segment::Segment(ProgramCounter pc,Segment *next, Writer *w)
  : pc(pc),next(next)
{
  // copy writer
  writer = 0;
  while (w) {
    writer = new Writer(w->i,writer);
    w=w->next;
  }
}


Segment::~Segment()
{
  for (Writer *w=writer; w;) {
    Writer *n=w->next;
    delete w;
    w=n;
  }
  writer = 0;
}


static HashTable livenesscache(HT_INTKEY,100);


int CodeArea::livenessX(ProgramCounter from, TaggedRef *X,int maxX)
{
  void *aux = livenesscache.htFind(ToInt32(from));
  if (aux != htEmpty) {
    return ToInt32(aux);
  }
  int ret = livenessXInternal(from,X,maxX);
  livenesscache.htAdd(ToInt32(from),ToPointer(ret));
  return ret;
}

int CodeArea::livenessXInternal(ProgramCounter from, TaggedRef *X,int maxX)
{
  if (maxX<=0) maxX = NumberOfXRegisters;
  int *xUsage = new int[maxX];
  for (int i=0;i<maxX; i++) xUsage[i]=0;

  Segment *todo = new Segment(from,0,0);
  Segment *seen = 0;
  Segment *current = 0;
  ProgramCounter PC;
  goto outerLoop2;

outerLoop:
  {
    // unset write flags
    for (Writer *w = current->writer; w;) {
      Writer *t=w->next;
      Assert(w->i<maxX);
      Assert(xUsage[w->i]==-1);
      xUsage[w->i]=0;
      delete w;
      w = t;
    }
    current->writer = 0;
  }

outerLoop2:
  {
    // terminate if stack is empty
    if (!todo) {
      int nextI=0;
      int deleted=0;
      if (X) {
        for (int i=0; i<maxX; i++) {
          if (xUsage[i]==1) {
            nextI=max(nextI,i+1);
          } else {
            X[i]=0;
          }
        }
      } else {
        for (int i=0; i<maxX; i++) {
          if (xUsage[i]==1) {
            nextI=max(nextI,i+1);
          }
        }
      }

      while (seen) {
        Segment *n = seen->next;
        delete seen;
        seen = n;
      }
      delete [] xUsage;
      return nextI;
    }

    // next from todo list
    current = todo;
    PC = current->pc;
    todo = current->next;

    // add current to segment list
    {
      Segment **prev = &seen;
      Segment *next = *prev;
      while (next) {
        if (PC >= next->pc) {
          if (PC <= next->pcEnd) {
            delete current;
            goto outerLoop2;
          } else {
            prev = &next->next;
            next = *prev;
          }
        } else {
          break;
        }
      }

      current->next=next;
      *prev = current;

      // install writer flags
      for (Writer *w = current->writer; w;) {
        Writer *n = w->next;
        xUsage[w->i]=-1;
        w=n;
      }
    }

  innerLoop:
    {
      Opcode op = getOpcode(PC);

      switch (op) {
      case FASTCALL:
      case FASTTAILCALL:
      case GENFASTCALL:
        {
          AbstractionEntry *ae = (AbstractionEntry *) getAdressArg(PC+1);
          Abstraction *ab = ae->getAbstr();
          ISREAD_TO(ab->getArity());
          BREAK;
        }

      case MARSHALLEDFASTCALL:
        {
          int i  = getPosIntArg(PC+2)>>1;
          ISREAD_TO(i);
          BREAK;
        }

      case CALLBI:
        ISLOC(GetBI(PC+1),GetLoc(PC+2));
        break;

      case TESTBI:
        ISLOC(GetBI(PC+1),GetLoc(PC+2));
        PUSH(getLabelArg(PC+3));
        break;

      case INLINEPLUS1:
      case INLINEMINUS1:
        ISREAD(GETREGARG(PC+1));
        ISWRITE(GETREGARG(PC+2)); // must be last
        break;

      case INLINEPLUS:
      case INLINEMINUS:
        ISREAD(GETREGARG(PC+1));
        ISREAD(GETREGARG(PC+2));
        ISWRITE(GETREGARG(PC+3)); // must be last
        break;

      case INLINEDOT:
        ISREAD(GETREGARG(PC+1));
        ISWRITE(GETREGARG(PC+3));
        break;

      case INLINEAT:
        ISWRITE(GETREGARG(PC+2));
        break;

      case INLINEASSIGN:
        ISREAD(GETREGARG(PC+2));
        break;

      case INLINEUPARROW:
        ISREAD(GETREGARG(PC+1));
        ISREAD(GETREGARG(PC+2));
        ISWRITE(GETREGARG(PC+3)); // must be last?
        break;

      case SHALLOWGUARD:
        PUSH(getLabelArg(PC+1));
        break;

      case TESTLE:
      case TESTLT:
        ISREAD(GETREGARG(PC+1));
        ISREAD(GETREGARG(PC+2));
        ISWRITE(GETREGARG(PC+3));
        PUSH(getLabelArg(PC+4));
        break;

      case FAILURE:
      case RETURN:
      case ENDDEFINITION:
      case WAIT:
      case WAITTOP:
      case ASK:
      case CREATECOND:
      case CREATEOR:
      case CREATEENUMOR:
      case CREATECHOICE:
      case CLAUSE:
      case EMPTYCLAUSE:
      case NEXTCLAUSE:
      case LASTCLAUSE:
        BREAK;

      case EXHANDLER:
        CONTINUE(getLabelArg(PC+1));

      case LOCKTHREAD:
        ISREAD(GETREGARG(PC+2));
        break;

      case DEFINITIONCOPY:
      case DEFINITION:
        {
          AssRegArray *list           = (AssRegArray*) getAdressArg(PC+5);
          int size = list->getSize();
          for (int i = 0; i < size; i++) {
            switch ((*list)[i].kind) {
            case XReg: ISREAD((*list)[i].number); break;
            default: break;
            }
          }

          if (op==DEFINITIONCOPY) { // Michael please check !!!
            ISREAD(GETREGARG(PC+1));
          }
          ISWRITE(GETREGARG(PC+1));
          CONTINUE(getLabelArg(PC+2));
        }

      case BRANCH:
        {
          int lbl = getLabelArg(PC+1);
          CONTINUE(lbl);
        }

      case TAILSENDMSGX:
      case SENDMSGX:
        ISREAD(GETREGARG(PC+2));
        // fall through
      case TAILSENDMSGY:
      case TAILSENDMSGG:
      case SENDMSGY:
      case SENDMSGG:
        {
          SRecordArity arity = (SRecordArity) getAdressArg(PC+3);
          ISREAD_TO(getWidth(arity));
          BREAK;
        }
      case TAILAPPLMETHX:
      case APPLMETHX:
        ISREAD(GETREGARG(PC+2));
        // fall through
      case TAILAPPLMETHY:
      case TAILAPPLMETHG:
      case APPLMETHY:
      case APPLMETHG:
        {
          ApplMethInfoClass *ami = (ApplMethInfoClass*) getAdressArg(PC+1);
          SRecordArity arity     = ami->arity;
          ISREAD_TO(getWidth(arity));
          BREAK;
        }
      case CALLX:
      case TAILCALLX:
        ISREAD(GETREGARG(PC+1));
        // fall through
      case CALLY:
      case CALLG:
      case TAILCALLY:
      case TAILCALLG:
        ISREAD_TO(getPosIntArg(PC+2));
        BREAK;

      case THREAD:
        CONTINUE(getLabelArg(PC+1));

      case THREADX:
        ISREAD_TO(getPosIntArg(PC+1));
        CONTINUE(getLabelArg(PC+2));

      case GENCALL:
        {
          GenCallInfoClass *gci = (GenCallInfoClass*)getAdressArg(PC+1);
          ISREAD_TO(getWidth(gci->arity));
          BREAK;
        }

      case MOVEXX:
        ISREAD(GETREGARG(PC+1));
        ISWRITE(GETREGARG(PC+2));
        break;

      case MOVEXY:
      case MOVEXG:
        ISREAD(GETREGARG(PC+1));
        break;
      case MOVEYX:
      case MOVEGX:
        ISWRITE(GETREGARG(PC+2));
        break;
      case MOVEMOVEXYXY:
        ISREAD(GETREGARG(PC+1));
        ISREAD(GETREGARG(PC+3));
        break;
      case MOVEMOVEYXYX:
        ISWRITE(GETREGARG(PC+2));
        ISWRITE(GETREGARG(PC+4));
        break;
      case MOVEMOVEYXXY:
        ISWRITE(GETREGARG(PC+2));
        ISREAD(GETREGARG(PC+3));
        break;
      case MOVEMOVEXYYX:
        ISREAD(GETREGARG(PC+1));
        ISWRITE(GETREGARG(PC+4));
        break;
      case GETSELF:
        ISWRITE(GETREGARG(PC+1));
        break;
      case CREATEVARIABLEX:
        ISWRITE(GETREGARG(PC+1));
        break;
      case CREATEVARIABLEMOVEX:
        ISWRITE(GETREGARG(PC+1));
        // fall through
      case CREATEVARIABLEMOVEY:
      case CREATEVARIABLEMOVEG:
        ISWRITE(GETREGARG(PC+2));
        break;
      case UNIFYXX:
        ISREAD(GETREGARG(PC+1));
        ISREAD(GETREGARG(PC+2));
        break;
      case UNIFYGX:
      case UNIFYYX:
        ISREAD(GETREGARG(PC+2));
        break;
      case UNIFYXY:
      case UNIFYXG:
        ISREAD(GETREGARG(PC+1));
        break;
      case PUTRECORDX:
        ISWRITE(GETREGARG(PC+3));
        break;
      case PUTCONSTANTX:
        ISWRITE(GETREGARG(PC+2));
        break;
      case PUTLISTX:
        ISWRITE(GETREGARG(PC+1));
        break;
      case SETVARIABLEX:
        ISWRITE(GETREGARG(PC+1));
        break;
      case SETVALUEX:
        ISREAD(GETREGARG(PC+1));
        break;
      case GETRECORDX:
        ISREAD(GETREGARG(PC+3));
        break;
      case TESTLITERALX:
      case TESTBOOLX:
      case TESTNUMBERX:
        ISREAD(GETREGARG(PC+1));
        // fall through
      case TESTLITERALY:
      case TESTLITERALG:
      case TESTBOOLY:
      case TESTBOOLG:
      case TESTNUMBERY:
      case TESTNUMBERG:
        PUSH(getLabelArg(PC+3));
        PUSH(getLabelArg(PC+4));
        break;
      case GETLITERALX:
      case GETNUMBERX:
        ISREAD(GETREGARG(PC+2));
        break;
      case GETLISTVALVARX:
        ISREAD(GETREGARG(PC+2));
        // fall through
      case GETLISTVALVARY:
      case GETLISTVALVARG:
        ISREAD(GETREGARG(PC+1));
        ISWRITE(GETREGARG(PC+3));
        break;
      case GETLISTX:
        ISREAD(GETREGARG(PC+1));
        break;
      case GETVARIABLEX:
        ISWRITE(GETREGARG(PC+1));
        break;

      case GETVARVARXX:
        ISWRITE(GETREGARG(PC+1));
        ISWRITE(GETREGARG(PC+2));
        break;
      case GETVARVARXY:
      case GETVARVARXG:
        ISWRITE(GETREGARG(PC+1));
        break;
      case GETVARVARYX:
      case GETVARVARGX:
        ISWRITE(GETREGARG(PC+2));
        break;
      case UNIFYVARIABLEX:
        ISWRITE(GETREGARG(PC+1));
        break;
      case UNIFYVALUEX:
        ISREAD(GETREGARG(PC+1));
        break;
      case UNIFYVALVARXX:
        ISREAD(GETREGARG(PC+1));
        ISWRITE(GETREGARG(PC+2));
        break;
      case UNIFYVALVARXY:
      case UNIFYVALVARXG:
        ISREAD(GETREGARG(PC+1));
        break;
      case UNIFYVALVARYX:
      case UNIFYVALVARGX:
        ISWRITE(GETREGARG(PC+2));
        break;

      case MATCHX:
        ISREAD(GETREGARG(PC+1));
        // fall through
      case MATCHY:
      case MATCHG:
        {

#define DOTABLE(TAB)                                            \
          {                                                     \
            for (int i=0;i<table->size;i++) {                   \
              if (TAB[i]) {                                     \
                for (HTEntry* aux = TAB[i]; aux!=NULL;          \
                     aux=aux->getNext()) {                      \
                  PUSH(aux->getLabel());                        \
                }                                               \
              }                                                 \
            }                                                   \
          }

          IHashTable *table = (IHashTable *) getAdressArg(PC+2);
          PUSH(table->elseLabel);
          if (table->elseLabel != table->listLabel) PUSH(table->listLabel);
          if (table->literalTable) DOTABLE(table->literalTable);
          if (table->functorTable) DOTABLE(table->functorTable);
          if (table->numberTable) DOTABLE(table->numberTable);
          BREAK;
        }

      default:
        // no usage of X registers
        break;
      }
      PC += sizeOf(op);
      goto innerLoop;
    }
  }
}

#ifdef DEBUG_LIVENESS
void checkLiveness(ProgramCounter PC, int n, TaggedRef *X, int maxX)
{
  int m=CodeArea::livenessX(PC,X,maxX);
  if (m!=n) {
    if (m>n) printf("##########\n");
    printf("liveness(%p)=%d != %d\n",PC,m,n);
    displayCode(CodeArea::definitionStart(PC),-1);
  }
}
#endif
