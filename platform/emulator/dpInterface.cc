/*
 *  Authors:
 *    Per Brand, Konstantin Popov
 * 
 *  Contributors:
 * 
 *  Copyright:
 *    Per Brand, Konstantin Popov 1998
 * 
 *  Last change:
 *    $Date$ by $Author$
 *    $Revision$
 * 
 *  This file is part of Mozart, an implementation 
 *  of Oz 3:
 *     http://mozart.ps.uni-sb.de
 * 
 *  See the file "LICENSE" or
 *     http://mozart.ps.uni-sb.de/LICENSE.html
 *  for information on usage and redistribution 
 *  of this file, and for a DISCLAIMER OF ALL 
 *  WARRANTIES.
 *
 */

#ifdef INTERFACE
#pragma interface "dpInterface.hh"
#endif

#include "base.hh"
#include "dpInterface.hh"
#include "value.hh"
#include "os.hh"
#include "space.hh"

//
Bool isPerdioInitializedStub()
{
  return (NO);
}

//
OZ_Return portSendStub(Tertiary *p, TaggedRef msg)
{
  error("'portSend' called without DP library?");
  return (PROCEED);
}
OZ_Return cellDoExchangeStub(Tertiary*,TaggedRef,TaggedRef)
{
  error("'cellDoExchange' called without DP library?");
  return (PROCEED);
}
OZ_Return cellDoAccessStub(Tertiary*,TaggedRef)
{
  error("'cellDoAccess' called without DP library?");
  return (PROCEED);
}
OZ_Return cellAtAccessStub(Tertiary*,TaggedRef,TaggedRef)
{
  error("'cellAtAccess' called without DP library?");
  return (PROCEED);
}
OZ_Return cellAtExchangeStub(Tertiary*,TaggedRef,TaggedRef)
{
  error("'cellAtExchange' called without DP library?");
  return (PROCEED);
}
OZ_Return cellAssignExchangeStub(Tertiary*,TaggedRef,TaggedRef)
{
  error("'cellAssignExchange' called without DP library?");
  return (PROCEED);
}

// lock/unlock (interface) methods/their usage may be optimized
// further, e.g. inline cases when distributed locks are currently
// local;
// interface;
void lockLockProxyStub(Tertiary *t, Thread *thr)
{
  error("'lockLockProxy' called without DP library?");
}
void lockLockManagerOutlineStub(LockManagerEmul *lfu, Thread *thr)
{
  error("'lockLockManagerOutline' called without DP library?");
}
void unlockLockManagerOutlineStub(LockManagerEmul *lfu, Thread *thr)
{
  error("'unlockLockManagerOutline' called without DP library?");
}
void lockLockFrameOutlineStub(LockFrameEmul *lfu, Thread *thr)
{
  error("'lockLockFrameOutline' called without DP library?");
}
void unlockLockFrameOutlineStub(LockFrameEmul *lfu, Thread *thr)
{
  error("'unlockLockFrameOutline' called without DP library?");
}

//
Bool marshalTertiaryStub(Tertiary *t, MarshalTag tag, MsgBuffer *bs)
{
  error("'marshalTertiary' called without DP library?");
  return (NO);
}
OZ_Term unmarshalTertiaryStub(MsgBuffer *bs, MarshalTag tag)
{
  error("'unmarshalTertiary' called without DP library?");
  return ((OZ_Term) 0);
}
OZ_Term unmarshalOwnerStub(MsgBuffer *bs,MarshalTag mt)
{
  error("'unmarshalOwner' called without DP library?");
  return ((OZ_Term) 0);
}
//
OZ_Term unmarshalVarStub(MsgBuffer*)
{
  error("'unmarshalVar' called without DP library?");
  return ((OZ_Term) 0);
}
Bool marshalVariableStub(TaggedRef*, MsgBuffer*)
{
  error("'marshalVariable' called without DP library?");
  return (NO);
}
void marshalObjectStub(ConstTerm *t, MsgBuffer *bs)
{
  error("'marshalObject' called without DP library?");
}
void marshalSPPStub(TaggedRef term, MsgBuffer *bs,Bool trail)
{
  error("'marshalSPP' called without DP library?");
}

// interface for GC;
void gcProxyRecurseStub(Tertiary *t)
{
  error("'gcProxyRecurse' called without DP library?");
}
void gcManagerRecurseStub(Tertiary *t)
{
  error("'gcManagerRecurse' called without DP library?");
}
ConstTerm* gcDistResourceStub(ConstTerm*)
{
  error("'gcDistResource' called without DP library?");
  return ((ConstTerm *) 0);
}

//
void gcDistCellRecurseStub(Tertiary *t)
{
  error("'gcDistCellRecurse' called without DP library?");
}
void gcDistLockRecurseStub(Tertiary *t)
{
  error("'gcDistLockRecurse' called without DP library?");
}
//
void gcDistPortRecurseStub(Tertiary *t)
{
  error("'gcDistPortRecurse' called without DP library?");
}
//
// (Only) gc method - for cells & locks (because we have to know
// whether they are accessible locally or not);
ConstTerm* auxGcDistCellStub(Tertiary *t)
{
  error("'auxGcDistCell' called without DP library?");
  return ((ConstTerm *) 0);
}
ConstTerm* auxGcDistLockStub(Tertiary *t)
{
  error("'auxGcDistLock' called without DP library?");
  return ((ConstTerm *) 0);
}

//
ConstTerm *gcStatefulSpecStub(Tertiary *t)
{
  error("'gcStatefulSpec' called without DP library?");
  return ((ConstTerm *) 0);
}

//
void gcBorrowTableUnusedFramesStub() {}
void gcFrameToProxyStub() {}

//
void gcPerdioFinalStub() {}
void gcPerdioRootsStub() {}
void gcEntityInfoStub(Tertiary *t)
{
  Assert(t->getInfo() == (EntityInfo *) 0);
}

// exit hook;
void dpExitStub()
{
  if (!(*isPerdioInitialized)())
    return;

  // the following steps should be performed some
  // where inside libdp:
  //   1. send back credits (make borrow table empty)
  //   2. check threads suspended on perdio events
  //   3. check owner table empty

  oz_deinstallPath(oz_rootBoard());

  osSetAlarmTimer(0);

  unsigned sleepTime = 100; // time in ms to wait before exiting
                             // maybe some of the handlers should call osExit??
  while (sleepTime>0) {
    unsigned long idle_start = osTotalTime();
    osUnblockSignals();
    osBlockSelect(sleepTime);
    osBlockSignals(NO);
    sleepTime = (osTotalTime() - idle_start);
    oz_io_handle();
  }
}


// Debug stuff;
#ifdef DEBUG_CHECK
void maybeDebugBufferGetStub(BYTE b) {}
void maybeDebugBufferPutStub(BYTE b) {}
#endif

//
// Link interface function pointers against stubs;

//
Bool (*isPerdioInitialized)() = isPerdioInitializedStub;

// 
OZ_Return (*portSend)(Tertiary *p, TaggedRef msg)
  = portSendStub;
OZ_Return (*cellDoExchange)(Tertiary*,TaggedRef,TaggedRef)
  = cellDoExchangeStub;
OZ_Return (*cellDoAccess)(Tertiary*,TaggedRef)
  = cellDoAccessStub;
OZ_Return (*cellAtAccess)(Tertiary*,TaggedRef,TaggedRef)
  = cellAtAccessStub;
OZ_Return (*cellAtExchange)(Tertiary*,TaggedRef,TaggedRef)
  = cellAtExchangeStub;
OZ_Return (*cellAssignExchange)(Tertiary*,TaggedRef,TaggedRef)
  = cellAssignExchangeStub;

// lock/unlock (interface) methods/their usage may be optimized
// further, e.g. inline cases when distributed locks are currently
// local;
void (*lockLockProxy)(Tertiary *t, Thread *thr)
  = lockLockProxyStub;
void (*lockLockManagerOutline)(LockManagerEmul *lfu, Thread *thr)
  = lockLockManagerOutlineStub;
void (*unlockLockManagerOutline)(LockManagerEmul *lfu, Thread *thr)
  = unlockLockManagerOutlineStub;
void (*lockLockFrameOutline)(LockFrameEmul *lfu, Thread *thr)
  = lockLockFrameOutlineStub;
void (*unlockLockFrameOutline)(LockFrameEmul *lfu, Thread *thr)
  = unlockLockFrameOutlineStub;

//
Bool (*marshalTertiary)(Tertiary *t, MarshalTag tag, MsgBuffer *bs)
  = marshalTertiaryStub;
OZ_Term (*unmarshalTertiary)(MsgBuffer *bs, MarshalTag tag)
  = unmarshalTertiaryStub;
OZ_Term (*unmarshalOwner)(MsgBuffer *bs,MarshalTag mt)
  = unmarshalOwnerStub;
//
OZ_Term (*unmarshalVar)(MsgBuffer*)
  = unmarshalVarStub;
Bool (*marshalVariable)(TaggedRef*, MsgBuffer*)
  = marshalVariableStub;
void (*marshalObject)(ConstTerm *t, MsgBuffer *bs)
  = marshalObjectStub;
void (*marshalSPP)(TaggedRef term, MsgBuffer *bs,Bool trail)
  = marshalSPPStub;

//
void (*gcProxyRecurse)(Tertiary *t)
  = gcProxyRecurseStub;
void (*gcManagerRecurse)(Tertiary *t)
  = gcManagerRecurseStub;
ConstTerm* (*gcDistResource)(ConstTerm*)
  = gcDistResourceStub;
void (*gcDistCellRecurse)(Tertiary *t)
  = gcDistCellRecurseStub;
void (*gcDistLockRecurse)(Tertiary *t)
  = gcDistLockRecurseStub;
void (*gcDistPortRecurse)(Tertiary *t)
  = gcDistPortRecurseStub;
//
// (Only) gc method - for cells & locks (because we have to know
// whether they are accessible locally or not);
ConstTerm* (*auxGcDistCell)(Tertiary *t)
  = auxGcDistCellStub;
ConstTerm* (*auxGcDistLock)(Tertiary *t)
  = auxGcDistLockStub;

//
ConstTerm* (*gcStatefulSpec)(Tertiary *t)
  = gcStatefulSpecStub;

//
void (*gcBorrowTableUnusedFrames)()
  = gcBorrowTableUnusedFramesStub;
void (*gcFrameToProxy)()
  = gcFrameToProxyStub;

//
void (*gcPerdioFinal)()
  = gcPerdioFinalStub;
void (*gcPerdioRoots)()
  = gcPerdioRootsStub;
void (*gcEntityInfo)(Tertiary*)
  = gcEntityInfoStub;

// exit hook;
void (*dpExit)()
  = dpExitStub;

// Debug stuff;
#ifdef DEBUG_CHECK
void (*maybeDebugBufferGet)(BYTE b)
  = maybeDebugBufferGetStub;
void (*maybeDebugBufferPut)(BYTE b)
  = maybeDebugBufferPutStub;
#endif
