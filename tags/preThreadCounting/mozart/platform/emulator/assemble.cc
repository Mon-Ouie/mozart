/*
  Hydra Project, DFKI Saarbruecken,
  Stuhlsatzenhausweg 3, D-66123 Saarbruecken, Phone (+49) 681 302-5312
  Author: popow
  Last modified: $Date$ from $Author$
  Version: $Revision$
  State: $State$

  */


#include "am.hh"
#include "thread.hh"
#include "assemble.hh"
#include "builtins.hh"



OZ_C_proc_begin(BIstringToOp,2)
{
  OZ_declareAtomArg("stringToOp",0,str);
  OZ_Term out = OZ_getCArg(1);

  Opcode oc = CodeArea::stringToOp(str);
  if (oc == ERROR) {
    return OZ_unifyInt(out,-1);
  } else {
    return OZ_unifyInt(out,(int)oc);
  }
}
OZ_C_proc_end



OZ_C_proc_begin(BIsizeOfOp,2)
{
  OZ_declareIntArg("sizeOfOp",0,opcode);
  OZ_Term out = OZ_getCArg(1);

  if (opcode > (int) ERROR || opcode < 0) {
    return OZ_unifyInt(out,-1);
  }

  int sz = sizeOf((Opcode)opcode);

  return OZ_unifyInt(out,sz);
}
OZ_C_proc_end


OZ_C_proc_begin(BInewCodeArea,2)
{
  OZ_declareIntArg("newCodeArea",0,sz);
  OZ_Term out = OZ_getCArg(1);

  ProgramCounter mypc;
  CodeArea *result = new CodeArea(NULL,sz,mypc);
      
  if (mypc == NOCODE) {
    return OZ_unifyInt(out,-1);
  }

  return OZ_unifyInt(out,(int)mypc);
}
OZ_C_proc_end



#define PCIN(fun)   OZ_declareIntArg(fun,0,help); \
                    ProgramCounter pc = (ProgramCounter) help;

#define RET(Call,N) return OZ_unifyInt(OZ_getCArg(N),(int)Call);


OZ_C_proc_begin(BIscheduleCode,3)
{
  PCIN("scheduleCode");
  OZ_Term greglist = OZ_getCArg(1);

  int size = OZ_length(greglist);
  RefsArray gregs = (size == 0) ? NULL : allocateRefsArray(size);

  int i = 0;
  while(!OZ_isNil(greglist)) {
    if (i >= size) {
      error("list should be empty");
    }
    gregs[i++] = OZ_head(greglist);
    greglist = OZ_tail(greglist);
  }

  PrTabEntry *predd = new PrTabEntry(OZ_nil(),0,size);
  predd->PC = pc;
  Abstraction *p = new Abstraction(predd, gregs, tagged2Literal(OZ_nil()));

  return OZ_unify(OZ_getCArg(2),makeTaggedSRecord(p));
}
OZ_C_proc_end

OZ_C_proc_begin(BIwriteOpcode,3)
{
  PCIN("writeOpcode");
  OZ_declareIntArg("writeOpcode",1,opcode);

  RET(CodeArea::writeOpcode((Opcode)opcode,pc),2);
}
OZ_C_proc_end


  
OZ_C_proc_begin(BIwriteBuiltin,3)
{
  PCIN("BIwriteBuiltin");
  OZ_Term bi  = OZ_getCArg(1);

  DEREF(bi,_1,_2);
  if (!isSRecord(bi) || tagged2SRecord(bi)->getType() != R_BUILTIN) {
    warning("writeBuiltin: builtin expected", OZ_toC(bi));
    return FAILED;
  }

  BuiltinTabEntry *found = ((Builtin*)tagged2SRecord(bi))->getBITabEntry();

  RET(CodeArea::writeBuiltin(found,pc),2);
}
OZ_C_proc_end


OZ_C_proc_begin(BIwriteConst,3)
{
  PCIN("writeOpcode");
  OZ_Term literal = OZ_getCArg(1);
  DEREF(literal,_1,_2);

  RET(CodeArea::writeLiteral(literal,pc),2);
}
OZ_C_proc_end


OZ_C_proc_begin(BIwriteInt,3)
{
  PCIN("writeInt");
  OZ_Term i = OZ_getCArg(1);
  DEREF(i,_1,_2);

  RET(CodeArea::writeInt(i,pc),2);
}
OZ_C_proc_end



OZ_C_proc_begin(BIwriteReg,3)
{
  PCIN("writeReg");
  OZ_declareIntArg("writeReg",1,reg);

  RET(CodeArea::writeRegIndex(reg,pc),2);
}
OZ_C_proc_end


OZ_C_proc_begin(BIwritePosint,3)
{
  PCIN("writePosint");
  OZ_declareIntArg("writePosint",1,i);

  RET(CodeArea::writeInt(i,pc),2);
}
OZ_C_proc_end


OZ_C_proc_begin(BIwriteArity,3)
{
  PCIN("writeArity");
  OZ_declareIntArg("writeArity",1,i);

  RET(CodeArea::writeArity(i,pc),2);
}
OZ_C_proc_end


OZ_C_proc_begin(BIwriteRecordArity,3)
{
  PCIN("writeRecordArity");
  Arity *ar = SRecord::aritytable.find(OZ_getCArg(1));

  RET(CodeArea::writeRecordArity(ar,pc),2);
}
OZ_C_proc_end


OZ_C_proc_begin(BIwritePredicateRef,3)
{
  PCIN("writePosint");
  OZ_declareIntArg("writePredicateRef",1,i);

  RET(CodeArea::writePredicateRef(i,pc),2);
}
OZ_C_proc_end


OZ_C_proc_begin(BIwriteLabel,4)
{
  PCIN("writeReg");
  OZ_declareIntArg("writeReg",1,start);
  OZ_declareIntArg("writeReg",2,lab);

  RET(CodeArea::writeLabel(lab,(ProgramCounter)start,pc),3);
}
OZ_C_proc_end


OZ_C_proc_begin(BIwritePredId,4)
{
  PCIN("writeReg");
  OZ_Term name = OZ_getCArg(1);
  DEREF(name,_1,_2);
  OZ_declareIntArg("writePredId",2,arity);

  int nGRegs = 0;
  PrTabEntry *pred = new PrTabEntry(name,arity,nGRegs);
  pc = CodeArea::writeBuiltin((BuiltinTabEntry*) pred, pc);
  pred->PC = pc+1;

  RET(pc,3);
}
OZ_C_proc_end



void BIinitAssembler()
{
  BIadd("stringToOp",        2, BIstringToOp);
  BIadd("sizeOfOp",          2, BIsizeOfOp);
  BIadd("newCodeArea",       2, BInewCodeArea);
  BIadd("scheduleCode",      3, BIscheduleCode);
  BIadd("writeOpcode",       3, BIwriteOpcode);
  BIadd("writeReg",          3, BIwriteReg);
  BIadd("writeBuiltin",      3, BIwriteBuiltin);
  BIadd("writeConst",        3, BIwriteConst);
  BIadd("writePosint",       3, BIwritePosint);
  BIadd("writeInt",          3, BIwriteInt);
  BIadd("writeArity",        3, BIwriteArity);
  BIadd("writeRecordArity",  3, BIwriteRecordArity);
  BIadd("writePredicateRef", 3, BIwritePredicateRef);
  BIadd("writeLabel",        4, BIwriteLabel);
  BIadd("writePredId",       4, BIwritePredId);
}

