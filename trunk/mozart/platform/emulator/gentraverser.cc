/*
 *  Authors:
 *    Kostja Popov (kost@sics.se)
 * 
 *  Contributors:
 *    optional, Contributor's name (Contributor's email address)
 * 
 *  Copyright:
 *    Organization or Person (Year(s))
 * 
 *  Last change:
 *    $Date$
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

#if defined(INTERFACE)
#pragma implementation "gentraverser.hh"
#endif

#include "base.hh"
#include "gentraverser.hh"
#include "pickle.hh"

//
// kost@ : there is no 'fsetcore.hh';
extern void makeFSetValue(OZ_Term,OZ_Term*);

//
// A different one because of effeciency reasons. A 'ProcessNodeProc' 
// could well be supplied...
void GenTraverser::doit()
{
  while (!isEmpty()) {
    OZ_Term t = get();
    // a push-pop pair for the topmost entry is saved:
  bypass:
    CrazyDebug(incDebugNODES(););
    DEREF(t, tPtr, tTag);

    //
    switch(tTag) {

    case SMALLINT:
      processSmallInt(t);
      break;

    case OZFLOAT:
      processFloat(t);
      break;

    case LITERAL:
      {
	int ind = find(t);
	if (ind >= 0) {
	  (void) processRepetition(ind);
	  continue;
	}
	processLiteral(t);
	break;
      }

    case LTUPLE:
      {
	int ind = find(t);
	if (ind >= 0) {
	  (void) processRepetition(ind);
	  continue;
	}

	//
	if (!processLTuple(t)) {
	  LTuple *l = tagged2LTuple(t);
	  ensureFree(1);
	  put(l->getTail());
	  t = l->getHead();
	  goto bypass;
	}
	break;
      }

    case SRECORD:
      {
	int ind = find(t);
	if (ind >= 0) {
	  (void) processRepetition(ind);
	  continue;
	}

	//
	if (!processSRecord(t)) {
	  SRecord *rec = tagged2SRecord(t);
	  TaggedRef label = rec->getLabel();
	  int argno = rec->getWidth();

	  // 
	  // The order is: label, [arity], subtrees...
	  // Both tuple and record args appear in a reverse order;
	  ensureFree(argno+1);	// pessimistic approximation;
	  for(int i = 0; i < argno; i++)
	    put(rec->getArg(i));
	  if (!rec->isTuple())
	    put(rec->getArityList());
	  t = rec->getLabel();
	  goto bypass;
	}
	break;
      }

  case EXT:
    processExtension(t);
    break;

  case OZCONST:
    {
      int ind = find(t);
      if (ind >= 0) {
	(void) processRepetition(ind);
	continue;
      }

      //
      ConstTerm *ct = tagged2Const(t);
      switch (ct->getType()) {

      case Co_BigInt:
	processBigInt(t, ct);
	break;

      case Co_Dictionary:
	if (!processDictionary(t, ct)) {
	  OzDictionary *d = (OzDictionary *) ct;
	  // kost@ : what the hell is going on here???
	  int i = d->getFirst();
	  i = d->getNext(i);
	  // (pairs will be added on the receiver site in reverse order);
	  ensureFree(i+i);
	  while(i>=0) {
	    put(d->getValue(i));
	    put(d->getKey(i));
	    i = d->getNext(i);
	  }
	}
	break;

      case Co_Builtin:
	processBuiltin(t, ct);
	break;

      case Co_Chunk:
	if (!processChunk(t, ct)) {
	  SChunk *ch = (SChunk *) ct;
	  t = ch->getValue();
	  goto bypass;
	}
	break;

      case Co_Class:
	if (!processClass(t, ct)) {
	  ObjectClass *cl = (ObjectClass *) ct;
	  SRecord *fs = cl->getFeatures();
	  if (fs)
	    t = makeTaggedSRecord(fs);
	  else
	    t = oz_nil();
	  goto bypass;
	}
	break;

      case Co_Abstraction:
	{
	  if (!processAbstraction(t, ct)) {
	    Abstraction *pp = (Abstraction *) ct;
	    int gs = pp->getPred()->getGSize();
	    //
	    // in the stream: file, name, registers, code area:
	    ensureFree(gs+2);
	    for (int i=0; i < gs; i++)
	      put(pp->getG(i));
	    //
	    put(pp->getName());
	    put(pp->getPred()->getFile());
	  }
	}
	break;

      case Co_Object:
	processObject(t, ct);
	break;

      case Co_Lock:
	processLock(t, (Tertiary *) ct);
	break;
      case Co_Cell:
	processCell(t, (Tertiary *) ct);
	break;
      case Co_Port:
	processPort(t, (Tertiary *) ct);
	break;
      case Co_Resource:
	processResource(t, (Tertiary *) ct);
	break;

      default:
	processNoGood(t,OK);
	break;
      }
      break;
    }

    case FSETVALUE:
      if (!processFSETValue(t)) {
	t = tagged2FSetValue(t)->getKnownInList();
	goto bypass;
      }
      break;

    case UVAR:
      processUVar(tPtr);
      break;

    case CVAR:
      {
	int ind = find(t);
	if (ind >= 0) {
	  (void) processRepetition(ind);
	  continue;
	}

	//
	OZ_Term value;
	if ((value = processCVar(tPtr))) {
	  t = value;
	  goto bypass;
	}
	break;
      }

    case GCTAG:
      {
	// If the argument is zero then the task is empty:
	void *arg = getPtr();

	//
	if (arg) {
	  MarshalerBinaryAreaProcessor proc =
	    (MarshalerBinaryAreaProcessor) lookupPtr();
	  // 'proc' is preserved in stack;
	  StackEntry *se = putPtrSERef(0);
	  putInt(taggedBATask);	// pre-cooked;

	  //
	  if (!(*proc)(this, arg)) {
	    // not yet done - restore the argument back;
	    updateSEPtr(se, arg);
	  }
	  // ... otherwise do nothing: the empty task will be
	  // discarded later - 
	} else {
	  CrazyDebug(decDebugNODES(););
	  // - here, to be exact:
	  dropEntry();		// 'proc';
	}
	break;
      }

    default:
      processNoGood(t,NO);
      break;
    }
  }
}

//
// Code fragments that create particular data structures should be
// factorized away, but this would make comparing this unmarshaler
// with the recursive one unfair (since it's not done for that one
// neither). 
// 
// An obvious factorization is to have 'constructXXX(...)' functions
// (or virtual methods, if more than one builder are anticipated) that
// take necessary arguments and do whatever necessary for creating the 
// structure. Arguments are (a) all the non-oz-terms, aka gnames, etc.
// and (b) non-proper subtrees, e.g. a record's arity list.
//
// Handling proper subtrees is done by the builder, symmetrically to the
// GenTraverser.

//
void
Builder::buildValueOutline(OZ_Term value, BTFrame *frame,
			   BuilderTaskType type)
{
  Assert(type != BT_spointer);
  Bool doMemo = NO;

  //
  // Iteration invariant: there are correct 'frame' and 'type', but 
  // no argument.
  // Procedure invariant: it gets frame but must get rid of it;
repeat:
  //
  switch(type) {

    //
    // Though it's handled inline, we can get here iteratively:
  case BT_spointer:
    {
      GetBTTaskPtr1(frame, OZ_Term*, spointer);
      DiscardBTFrame(frame);
      *spointer = value;
      break;
    }

  case BT_spointer_iterate:
    {
      GetBTTaskPtr1(frame, OZ_Term*, spointer);
      *spointer = value;
      CrazyDebug(incDebugNODES(););
      DiscardBTFrame(frame);
      DebugCode(value = (OZ_Term) -1;);	// 'value' is expired;
      GetBTTaskTypeNoDecl(frame, type);
      goto repeat;
    }

  case BT_buildValue:
    {
      GetBTTaskArg1NoDecl(frame, OZ_Term, value);
      DiscardBTFrame(frame);
      GetBTTaskTypeNoDecl(frame, type);
      goto repeat;
    }

  case BT_makeTupleMemo:
    doMemo = OK;
    // fall through;
  case BT_makeTuple:
    {
      GetBTTaskArg1(frame, int, arity);
      SRecord *rec = SRecord::newSRecord(value, arity);
      OZ_Term recTerm = makeTaggedSRecord(rec);
      if (doMemo) {
	GetBTTaskArg2(frame, int, memoIndex);
	set(recTerm, memoIndex);
	doMemo = NO;
      }

      //
      GetBTNextTaskType(frame, nt);
      if (nt == BT_spointer) {
	CrazyDebug(incDebugNODES(););
	GetBTNextTaskPtr1(frame, OZ_Term*, spointer);
	*spointer = recTerm;
	DiscardBT2Frames(frame);

	//
	OZ_Term *args = rec->getRef();
	EnsureBTSpace(frame, arity);
	while(arity-- > 0) {
	  PutBTTaskPtr(frame, BT_spointer, args++);
	}
      } else {
	//
	ReplaceBTTask1stArg(frame, BT_buildValue, recTerm);

	//
	OZ_Term *args = rec->getRef();
	// put tasks in reverse order (since subtrees will appear in
	// the normal order):
	args = args;		// after the last one;
	EnsureBTSpace(frame, arity);
	arity--;
	PutBTTaskPtr(frame, BT_spointer_iterate, args++);
	while (arity-- > 0) {
	  PutBTTaskPtr(frame, BT_spointer, args++);
	}
      }
      break;
    }

  case BT_takeRecordLabel:
    {
      ReplaceBTTask1stArg(frame, BT_takeRecordArity, value);
      break;
    }

  case BT_takeRecordLabelMemo:
    {
      ReplaceBTTask1stArg(frame, BT_takeRecordArityMemo, value);
      break;
    }

  case BT_takeRecordArity:
    {
      ReplaceBTTask2ndArg(frame, BT_makeRecord_intermediate, value);
      break;
    }

  case BT_takeRecordArityMemo:
    {
      ReplaceBTTask2ndArg(frame, BT_makeRecordMemo_intermediate, value);
      break;
    }

  case BT_makeRecordMemo_intermediate:
    doMemo = OK;
    // fall through;

  case BT_makeRecord_intermediate:
    {
      GetBTTaskArg1(frame, OZ_Term, label);
      GetBTTaskArg2(frame, OZ_Term, arity);

      //
      OZ_Term sortedArity = arity;
      if (!isSorted(arity)) {
	int arityLen;
	TaggedRef aux = duplist(arity, arityLen);
	sortedArity = sortlist(aux, arityLen);
      }
      //
      SRecord *rec =
	SRecord::newSRecord(label, aritytable.find(sortedArity));
      OZ_Term recTerm = makeTaggedSRecord(rec);
      if (doMemo) {
	GetNextBTFrameArg1(frame, int, memoIndex);
	set(recTerm, memoIndex);
	doMemo = NO;
	if (isGCTaggedInt(value)) {
	  // fprintf(stdout, " non-existing value (%d)!\n", 
	  //         getGCTaggedInt(value));
	  //
	  // That must be the same slot: a missing value (i.e. we see
	  // here 'GC' TaggedRef) can occur only when an intermediate
	  // task (like this one) matches a 'DIF_REF' token from the
	  // stream for the same (currently only record) term.
	  // Observe, there is no place for recursion etc., thus, only
	  // one term may miss in the table, and that one must be
	  // 'memoIndex'.
	  Assert(memoIndex == getGCTaggedInt(value));
	  value = recTerm;
	}
      } else {
	Assert(!isGCTaggedInt(value));
      }
      DiscardBT2Frames(frame);

      //
      GetBTTaskType(frame, nt);
      if (nt == BT_spointer) {
	CrazyDebug(incDebugNODES(););
	GetBTTaskPtr1(frame, OZ_Term*, spointer);
	*spointer = recTerm;
	DiscardBTFrame(frame);

	//
	while (oz_isCons(arity)) {
	  EnsureBTSpace1Frame(frame);
	  PutBTTaskPtrArg(frame, BT_recordArg, rec, oz_head(arity));
	  arity = oz_tail(arity);
	}
      } else {
	//
	EnsureBTSpace1Frame(frame);
	PutBTTaskPtrArg(frame, BT_recordArg_iterate, rec, oz_head(arity));
	arity = oz_tail(arity);
	while (oz_isCons(arity)) {
	  EnsureBTSpace1Frame(frame);
	  PutBTTaskPtrArg(frame, BT_recordArg, rec, oz_head(arity));
	  arity = oz_tail(arity);
	}
      }

      //
      // 'value' is preserved;
      GetBTTaskTypeNoDecl(frame, type);
      goto repeat;
    }

  case BT_recordArg:
    {
      GetBTTaskPtr1(frame, SRecord*, rec);
      GetBTTaskArg2(frame, OZ_Term, fea);
      DiscardBTFrame(frame);
      rec->setFeature(fea, value);
      break;
    }

  case BT_recordArg_iterate:
    {
      GetBTTaskPtr1(frame, SRecord*, rec);
      GetBTTaskArg2(frame, OZ_Term, fea);
      DiscardBTFrame(frame);
      rec->setFeature(fea, value);
      //
      CrazyDebug(incDebugNODES(););
      value = makeTaggedSRecord(rec); // new value;
      GetBTTaskTypeNoDecl(frame, type);
      goto repeat;
    }

  case BT_dictKey:
    {
      GetBTTaskPtr1(frame, OzDictionary*, dict);
      // 'dict' remains in place:
      ReplaceBTTask2ndArg(frame, BT_dictVal, value);
      break;
    }

  case BT_dictVal:
    {
      GetBTTaskPtr1(frame, OzDictionary*, dict);
      GetBTTaskArg2(frame, OZ_Term, key);
      DiscardBTFrame(frame);
      dict->setArg(key, value);
      break;
    }

  case BT_fsetvalue:
    {
      ReplaceBTTask2ndArg(frame, BT_fsetvalueFinal, value);
      break;
    }

  case BT_fsetvalueMemo:
    {
      ReplaceBTTask2ndArg(frame, BT_fsetvalueFinalMemo, value);
      break;
    }

  case BT_fsetvalueFinal:
    {
      OZ_Term ret;
      DiscardBTFrame(frame);
      makeFSetValue(value, &ret);
      value = ret;
      GetBTTaskTypeNoDecl(frame, type);
      goto repeat;
    }

  case BT_fsetvalueFinalMemo:
    {
      GetBTTaskArg1(frame, int, memoIndex);
      DiscardBTFrame(frame);
      OZ_Term ret;
      makeFSetValue(value, &ret);
      value = ret;
      set(value, memoIndex);
      GetBTTaskTypeNoDecl(frame, type);
      goto repeat;
    }

  case BT_chunkMemo:
    doMemo = OK;
    // fall through;
  case BT_chunk:
    {
      Assert(oz_onToplevel());
      GetBTTaskPtr1(frame, GName*, gname);

      //
      OZ_Term chunkTerm;
      SChunk *sc = new SChunk(am.currentBoard(), 0);
      sc->setGName(gname);
      chunkTerm = makeTaggedConst(sc);
      addGName(gname, chunkTerm);
      sc->import(value);

      //
      value = chunkTerm;
      if (doMemo) {
	GetBTTaskArg2(frame, int, memoIndex);
	set(value, memoIndex);
	doMemo = NO;
      }
      DiscardBTFrame(frame);
      GetBTTaskTypeNoDecl(frame, type);
      goto repeat;
    }

  case BT_classFeatures:
    {
      Assert(oz_isSRecord(value));
      GetBTTaskPtr1(frame, ObjectClass*, cl);
      GetBTTaskArg2(frame, int, flags);
      DiscardBTFrame(frame);

      //
      SRecord *feat = tagged2SRecord(value);
      TaggedRef ff = feat->getFeature(NameOoFeat);
      //
      cl->import(value,
		 feat->getFeature(NameOoFastMeth),
		 oz_isSRecord(ff) ? ff : makeTaggedNULL(),
		 feat->getFeature(NameOoDefaults),
		 flags);

      //
      value = makeTaggedConst(cl);
      GetBTTaskTypeNoDecl(frame, type);
      goto repeat;
    }

  case BT_procFile:
    {
      ReplaceBTTask1stArg(frame, BT_proc, value);
      break;
    }

  case BT_procFileMemo:
    {
      ReplaceBTTask1stArg(frame, BT_procMemo, value);
      break;
    }

  case BT_procMemo:
    doMemo = OK;
    // fall through;
  case BT_proc:
    {
      OZ_Term name = value;
      GetBTTaskArg1(frame, OZ_Term, file);
      DiscardBTFrame(frame);
      GetBTFramePtr1(frame, GName*, gname);
      GetBTFramePtr2(frame, ProgramCounter, pc);
      GetBTFrameArg3(frame, int, maybeMemoIndex);
      DiscardBTFrame(frame);
      GetBTFrameArg1(frame, int, maxX);
      GetBTFrameArg2(frame, int, line);
      GetBTFrameArg3(frame, int, column);
      DiscardBTFrame(frame);
      GetBTFrameArg1(frame, int, arity);
      GetBTFrameArg2(frame, int, gsize);
      DiscardBTFrame(frame);

      //
      Assert(gname);		// must be an unknown procedure here;
      OZ_Term procTerm;
      // kost@ : 'flags' are obviously not used (otherwise something
      // would not work: flags are not passed as e.g. 'file' is);
      PrTabEntry *pr = new PrTabEntry(name, mkTupleWidth(arity),
				      file, line, column,
				      oz_nil(), maxX);
      pr->PC = pc;
      pr->setGSize(gsize);
      Abstraction *pp = Abstraction::newAbstraction(pr, am.currentBoard());
      procTerm = makeTaggedConst(pp);
      pp->setGName(gname);
      addGName(gname, procTerm);

      //
      if (doMemo) {
	set(procTerm, maybeMemoIndex);
	doMemo = NO;
      }

      //
      if (gsize > 0) {
	// reverse order... and don't bother with 'spointer' tasks:
	// just issue an '_iterate' task;
	EnsureBTSpace(frame, gsize);
	PutBTTaskPtrArg(frame, BT_closureElem_iterate, pp, 0);
	for (int i = 1; i < gsize; i++) {
	  PutBTTaskPtrArg(frame, BT_closureElem, pp, i);
	}
	break;			// BT_proc:
      } else {
	value = makeTaggedConst(pp);
	GetBTTaskTypeNoDecl(frame, type);
	goto repeat;
      }

      //
      // (code area is done by the user himself;)
      Assert(0);
    }

  case BT_closureElem:
    {
      GetBTTaskPtr1(frame, Abstraction*, pp);
      GetBTTaskArg2(frame, int, ind);
      DiscardBTFrame(frame);
      pp->initG(ind, value);
      break;
    }

  case BT_closureElem_iterate:
    {
      GetBTTaskPtr1(frame, Abstraction*, pp);
      GetBTTaskArg2(frame, int, ind);
      DiscardBTFrame(frame);
      pp->initG(ind, value);
      //
      CrazyDebug(incDebugNODES(););
      value = makeTaggedConst(pp);
      GetBTTaskTypeNoDecl(frame, type);
      goto repeat;
    }

  //
  // 'BT_binary' is transient here: it must be either saved or
  // discarded if it's already done;
  case BT_binary:
    {
      GetBTTaskPtr1(frame, void*, arg);
      if (arg)
	binaryTasks.save(arg);
      DiscardBTFrame(frame);
      GetBTTaskTypeNoDecl(frame, type);
      goto repeat;
    }

  case BT_binary_getValue:
    {
      GetBTTaskPtr1(frame, OzValueProcessor, proc);
      GetBTTaskPtr2(frame, void*, arg);
      DiscardBTFrame(frame);
      (*proc)(arg, value);
      break;
    }

  case BT_binary_getCompleteValue:
    {
      ReplaceBTTask1stArg(frame, BT_binary_getValue_intermediate, value);
      break;
    }

  case BT_binary_getValue_intermediate:
    {
      GetBTTaskArg1(frame, OZ_Term, ozValue);
      DiscardBTFrame(frame);
      GetBTFramePtr1(frame, OzValueProcessor, proc);
      GetBTFramePtr2(frame, void*, arg);
      DiscardBTFrame(frame);
      //
      (*proc)(arg, ozValue);

      //
      // 'value' is preserved;
      GetBTTaskTypeNoDecl(frame, type);
      goto repeat;
    }

  default:
    OZD_error("Builder: unknown task!");
  }

  //
  while (!binaryTasks.isEmpty()) {
    void *arg = binaryTasks.pop();
    Assert(arg);
    EnsureBTSpace1Frame(frame);
    PutBTTaskPtr(frame, BT_binary, arg);
  }
  //
  SetBTFrame(frame);
}

