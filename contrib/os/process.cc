#include "oz_api.h"
#include "extension.hh"
#include <unistd.h>

class Process : public Extension {
public:
  pid_t pid;
  Process(pid_t i):Extension(),pid(i){}
  Process(Process&);
  // Extension
  static int id;
  virtual int getIdV() { return id; }
  virtual OZ_Term typeV() { return OZ_atom("process"); }
  virtual void printStreamV(ostream &out,int depth = 10);
  virtual Extension* gcV();
  //virtual void gcRecurseV();
};

int Process::id;

inline Bool oz_isProcess(OZ_Term t)
{
  return oz_isExtension(t) &&
    oz_tagged2Extension(t)->getIdV()==Process::id;
}

Bool OZ_isProcess(OZ_Term t)
{ return oz_isProcess(oz_deref(t)); }

inline Process* tagged2Process(OZ_Term t)
{
  Assert(oz_isProcess(t));
  return (Process*) oz_tagged2Extension(t);
}

Process* OZ_toProcess(OZ_Term t)
{ return tagged2Process(oz_deref(t)); }

void Process::printStreamV(ostream &out,int depth = 10)
{
  out << "<process " << pid << ">";
}

Extension* Process::gcV()
{
  return new Process(pid);
}

typedef struct { int from; int to; } fdPair;

extern void addChildProc(pid_t);

Process* makeProcess(char* const argv[],
                     fdPair map[],int n)
{
  pid_t pid;
  pid = fork();
  if (pid<0) return 0;
  if (pid==0) {
    // CHILD
    //
    // install IO redirections
    //
    // every `from' that appears as a `to' needs duping
    for(int i=0;i<n;i++) {
      int from = map[i].from;
      for (int j=0;j<n;j++)
        if (from==map[j].to) {
          cerr << "==> DUP " << from;
          map[i].from = dup(from);
          cerr << " -> " << map[i].from << endl;
          break;
        }
    }
    // dup2(from,to) for each entry
    for(int i=0;i<n;i++) {
      cerr << "==> DUP2 " << map[i].from << " -> " << map[i].to;
      int ret = dup2(map[i].from,map[i].to);
      cerr << ((ret<0)?" FAILED":"") << endl;
    }
    for(int i=0;i<n;i++) {
      cerr << "==> CLOSE " << map[i].from << endl;
      close(map[i].from);
    }
    //
    // exec program
    //
    execvp(argv[0],argv);
    fprintf(stderr,"execvp failed: %s\n",argv[0]);
    exit(-1);
  } else {
    // PARENT
    addChildProc(pid);
    return new Process(pid);
  }
}

OZ_BI_define(process_make,3,1)
{
  //{Process.make CMD ARGS IOMAP}
  //
  // IOMAP is a list of pairs of integers FROM#TO
  // indicating that file descriptor FROM must be duped into
  // file descriptor TO
  //
  // we assume that the caller has verified that
  // - all arguments are fully instantiated
  // - CMD is a virtual string
  // - ARGS is a list of virtual strings
  // - IOMAP is a list of pairs of non-negative integers
  // it's easier to check this in Oz than in C++
  OZ_declareTerm(0,CMD);
  OZ_declareTerm(1,ARGS);
  OZ_declareTerm(2,IOMAP);
  //
  // create argv
  //
  int args_len = 0;
  OZ_Term list;
  list = ARGS;
  while (!OZ_isNil(list)) { args_len++; list=OZ_tail(list); }
  char**argv = (char**) malloc((2+args_len) * sizeof(char*));
  list = ARGS;
  argv[0] = strdup(OZ_virtualStringToC(CMD,0));
  for(int i=1;i<=args_len;i++,list=OZ_tail(list))
    argv[i] = strdup(OZ_virtualStringToC(OZ_head(list),0));
  argv[args_len+1] = 0;
  //
  // create the iomap
  //
  int iomap_len = 0;
  list = IOMAP;
  while (!OZ_isNil(list)) { iomap_len++; list=OZ_tail(list); }
  fdPair *iomap = new fdPair[iomap_len];
  list = IOMAP;
  for(int i=0;i<iomap_len;i++,list=OZ_tail(list)) {
    OZ_Term p = OZ_head(list);
    iomap[i].from = OZ_intToC(OZ_getArg(p,0));
    iomap[i].to   = OZ_intToC(OZ_getArg(p,1));
  }
  //
  // create the process value
  //
  Process* p = makeProcess(argv,iomap,iomap_len);
  //
  // free temp arrays
  //
  for(int i=0;i<=args_len;i++) free(argv[i]);
  free(argv);
  delete iomap;
  if (p==0)
    return OZ_raiseErrorC("process",1,OZ_atom("forkFailed"));
  OZ_RETURN(oz_makeTaggedExtension(p));
} OZ_BI_end

OZ_BI_define(process_is,1,1)
{
  OZ_declareDetTerm(0,X);
  OZ_RETURN_BOOL(OZ_isProcess(X));
} OZ_BI_end

extern "C"
{
  OZ_C_proc_interface * oz_init_module(void)
  {
    static OZ_C_proc_interface i_table[] = {
      {"make"           ,3,1,process_make},
      {"is"             ,1,1,process_is},
      {0,0,0,0}
    };
    Process::id = oz_newUniqueId();
    return i_table;
  }
} /* extern "C" */
