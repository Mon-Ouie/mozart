/*
 *  Authors:
 *    Per Brand (perbrand@sics.se)
 *    Erik Klintskog (erik@sics.se)
 * 
 *  Contributors:
 *    optional, Contributor's name (Contributor's email address)
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
 *     http://mozart.ps.uni-sb.de
 * 
 *  See the file "LICENSE" or
 *     http://mozart.ps.uni-sb.de/LICENSE.html
 *  for information on usage and redistribution 
 *  of this file, and for a DISCLAIMER OF ALL 
 *  WARRANTIES.
 *
 */

#ifndef __FAILHH
#define __FAILHH

#ifdef INTERFACE  
#pragma interface
#endif

#include "base.hh"
#include "value.hh"
#include "dpBase.hh"
#include "comm.hh"
#include "genhashtbl.hh"

typedef unsigned int FaultInfo;

enum WatcherKind{
  RETRY      = 1,
  PERSISTENT = 2,
  W_CELL     = 4
};

enum EntityCondFlags{
  ENTITY_NORMAL = 0,
  PERM_BLOCKED  = 2,       
  TEMP_BLOCKED  = 1,
  PERM_ALL      = 4,
  TEMP_ALL      = 8,
  PERM_SOME     = 16,
  TEMP_SOME     = 32,
  TEMP_SOME_M   = 64,// must come after TEMP_SOME
  PERM_ME       = 128,
  TEMP_ME       = 256,
  TEMP_ME_M     = 512, // must come after TEMP_ME
  TEMP_FLOW     =1024
};

typedef unsigned int EntityCond;

inline EntityCond evalTrueEntityCond(EntityCond c){
  return (((c & (TEMP_ME_M)<<1) | ((c & TEMP_SOME_M) << 1) |
	  (c & (~(TEMP_ME_M | TEMP_SOME_M | TEMP_FLOW)))) |
	  (c & (TEMP_FLOW)<<2) | (c & (TEMP_FLOW)<<9));}

inline EntityCond msgEntityCond(EntityCond ec){
  if(!(ec & TEMP_SOME|TEMP_ME)) return ec;
  if(ec & TEMP_SOME) {
    ec |= TEMP_SOME_M;
    ec &= ~TEMP_SOME;}
  if(ec & TEMP_ME) {
    ec |= TEMP_ME_M;
    ec &= ~TEMP_ME;}
  return ec;}

inline Bool isHandlerCondition(EntityCond ec){
  if(ec & (PERM_BLOCKED|TEMP_BLOCKED)) {
    Assert((ec & (PERM_BLOCKED|TEMP_BLOCKED))==ec);
    return TRUE;}
  return FALSE;}

extern TaggedRef BI_probe;

class EntityInfo{
  friend class Tertiary;
public:
  Watcher *watchers;
  EntityCond entityCond;

  USEHEAPMEMORY;
  NO_DEFAULT_CONSTRUCTORS(EntityInfo);

  EntityInfo(Watcher *w){
    entityCond=ENTITY_NORMAL;
    watchers = w;}

  EntityInfo(EntityCond c){
    entityCond=c;
    watchers=NULL;}

  EntityCond getEntityCond(){
    return evalTrueEntityCond(entityCond);}

  Bool addEntityCond(EntityCond ec){
    if((entityCond | ec) ==entityCond) return FALSE;
    entityCond |= ec;
    return TRUE;}

  void subEntityCond(EntityCond ec){
    entityCond &= ~ec;}

  EntityCond getSummaryWatchCond();

  void gcWatchers();
};

#define TWIN_GC    1
extern Twin *usedTwins;
extern Watcher* globalWatcher;
void gcGlobalWatcher();
void gcTwins();

class Twin{
friend class Watcher;
public:
  unsigned int flags;
  Twin *next;
  int cellCtr;
  int lockCtr;
  
  void* operator new(size_t size){
    Assert(size==16);
    return (Chain *)genFreeListManager->getOne_4();}        
  
  void free(){
    genFreeListManager->putOne_4((FreeListEntry*) this);}    

  Twin(){
    flags=0;
    cellCtr=0;
    lockCtr=0;
    next=usedTwins;
    usedTwins=this;}

  void setGCMark(){
    flags = TWIN_GC;}

  Bool hasGCMark(){
    return flags & TWIN_GC;}    

  void resetGCMark(){
    flags = 0;}    
};


class Watcher{
friend class Tertiary;
friend class EntityInfo;
public:
  TaggedRef proc;
  Watcher *next;
  Thread *thread; // thread of handler
  short kind;
  unsigned short watchcond;
  Twin* twin;

  USEHEAPMEMORY;

  NO_DEFAULT_CONSTRUCTORS(Watcher);

  Watcher(TaggedRef p,Thread* t,EntityCond wc, short k){
    proc=p;
    next=NULL;
    thread=t;
    watchcond=wc;
    twin=NULL;
    kind=k;}
  
  Bool matches(TaggedRef p,Thread* t,EntityCond wc, short k){
    if(p!=proc) return FALSE;
    if(t!=thread) return FALSE;
    if(watchcond!=wc) return FALSE;
    if(kind!=k) return FALSE;
    return TRUE;}

  Twin* cellTwin(){
    kind |= W_CELL;
    Assert(twin==NULL);
    twin = new Twin();
    return twin;}

  void lockTwin(Twin* tw){
    Assert(twin==NULL);    
    twin=tw;}

  Bool fire(){
    if(kind & W_CELL){
      if(twin->lockCtr>twin->cellCtr) return FALSE;
      twin->cellCtr++;
      return TRUE;}
    else{
      if(twin->cellCtr>twin->lockCtr) return FALSE;
      twin->lockCtr++;
      return TRUE;}}

  Bool isFired(){
    if(twin==NULL) return FALSE;
    if(twin->cellCtr>0) return TRUE;
    if(twin->lockCtr>0) return TRUE;    
    return FALSE;}

  Bool isRetry(){return kind & RETRY;} 
  Bool isPersistent(){return kind & PERSISTENT;}
  Bool isSiteBased(){
    Assert(isHandler());
    return thread==NULL;}
  Bool isHandler(){return isHandlerCondition(watchcond);}
  Bool isCellPartObject(){return kind & W_CELL;}
  
  void setNext(Watcher* w){next=w;}

  Watcher* getNext(){return next;}

  void invokeHandler(EntityCond,TaggedRef,TaggedRef);

  void invokeWatcher(EntityCond, TaggedRef);

  Thread* getThread(){Assert(thread!=NULL);return thread;}

  Bool isTriggered(EntityCond ec){
    if(ec & watchcond) return OK;
    return NO;}

  EntityCond getWatchCond(){return watchcond;}

  EntityCond getEffWatching();
};


// MEREGCON void gcEntityInfo(Tertiary *t);

void gcEntityInfoImpl(Tertiary *t);

inline Bool errorIgnore(Tertiary *t) {
  EntityInfo* info = t->getInfo();
  if (info == NULL) return OK;
  return NO;}

inline EntityCond getEntityCond(Tertiary *t) {
  EntityInfo* info = t->getInfo();
  if (info == NULL) return ENTITY_NORMAL;
  return info->getEntityCond();}

inline Watcher** getWatcherBase(Tertiary *t){
  EntityInfo* info = t->getInfo();
  if(info==NULL) return NULL;
  if(info->watchers==NULL) return NULL;
  return &(info->watchers);}

void insertWatcherAtProxy(Tertiary *t, Watcher* w);
void insertWatcherAtManager(Tertiary *t, Watcher* w);

inline Bool someTempCondition(EntityCond ec){
  return ec & (TEMP_SOME|TEMP_BLOCKED|TEMP_ME|TEMP_ALL);}

inline Bool addEntityCond(Tertiary *t, EntityCond c){
  EntityInfo* info = t->getInfo();
  if(info==NULL){
    t->setInfo(new EntityInfo(c));
    return TRUE;}
  return info->addEntityCond(c);}

inline Bool addEntityCondMsg(Tertiary *t, EntityCond c){
  return addEntityCond(t,msgEntityCond(c));}

inline void subEntityCond(Tertiary *t, EntityCond c){
  EntityInfo* info = t->getInfo();
  Assert(info!=NULL);
  info->subEntityCond(c);}

inline void subEntityCondMsg(Tertiary *t, EntityCond c){
  subEntityCond(t,msgEntityCond(c));}

void tertiaryInstallProbe(DSite* s,ProbeType pt,Tertiary *t);

inline Watcher *getWatchersIfExist(Tertiary *t){
  EntityInfo* info = t->getInfo();
  if(info==NULL){return NULL;}
  return info->watchers;}

EntityCond askPart(Tertiary*, EntityCond);

ProbeType managerProbePart(Tertiary*, EntityCond ec);

inline EntityCond getSummaryWatchCond(Tertiary* t){
  if(t->getInfo()==NULL) return ENTITY_NORMAL;
  return t->getInfo()->getSummaryWatchCond();}

void installWatcher(Tertiary *t, EntityCond,TaggedRef,Thread*,Bool,Bool);
Bool deinstallWatcher(Tertiary *t, EntityCond,TaggedRef,Thread*,Bool,Bool);
void entityProblem(Tertiary *t);
void deferEntityProblem(Tertiary *t);
void managerProbeFault(Tertiary *t, DSite*, int);
void proxyProbeFault(Tertiary *t, int);

Bool entityCondMeToBlocked(Tertiary* t);

inline void removeBlocked(Tertiary* t){
  if(t->getInfo()==NULL) return;
  subEntityCond(t,PERM_BLOCKED|TEMP_BLOCKED);}

TaggedRef listifyWatcherCond(EntityCond);

void entityProblem(Tertiary*);
void gcTwins();

void initManagerForFailure(Tertiary*);
void initProxyForFailure(Tertiary*);

OZ_Return WatcherInstall(Tertiary*, SRecord*, TaggedRef);
OZ_Return WatcherDeInstall(Tertiary*, SRecord*, TaggedRef);

/**********************   DeferEvents   ******************/
enum DeferType{ 
  DEFER_PROXY_PROBLEM,
  DEFER_MANAGER_PROBLEM,
  DEFER_ENTITY_PROBLEM};
    
class DeferElement{
public:
  DeferElement *next;
  Tertiary  *tert;
  DSite     *site;
  DeferType  type;
  int        prob;
  DeferElement(){}
};

extern DeferElement* DeferdEvents;
extern TaggedRef BI_defer;
void gcDeferEvents();

#define IncorrectFaultSpecification oz_raise(E_ERROR,E_SYSTEM,"incorrect fault specification",0)

#define HandlerExists oz_raise(E_ERROR,E_SYSTEM,"handler exists",0)





/* __DPFAILHH */
#endif 


