/*
  Hydra Project, DFKI Saarbruecken,
  Stuhlsatzenhausweg 3, D-66123 Saarbruecken, Phone (+49) 681 302-5312
  Author: scheidhr
  Last modified: $Date$ from $Author$
  Version: $Revision$
  State: $State$

  ------------------------------------------------------------------------
*/

#ifndef __dvar__hh__
#define __dvar__hh__


#if defined(INTERFACE)
#pragma interface
#endif

#include "genvar.hh"
#include "oz.h"

enum PV_TYPES {
  PV_MANAGER,
  PV_PROXY,
  PV_TERTPROXY,
};

class ProxyList {
public:
  Site* sd;
  ProxyList *next;
public:
  ProxyList(Site* sd,ProxyList *next) :sd(sd),next(next) {}

  USEFREELISTMEMORY;

  void dispose() 
  {
    freeListDispose(this,sizeof(ProxyList));
  }
  ProxyList *gcProxyList();

};

class PendBinding {
public:
  TaggedRef val;
  Thread *thread;
  PendBinding *next;
public:
  PendBinding() { DebugCheckT(val=4711; thread=0; next=this;) }
  PendBinding(TaggedRef val,Thread *th,PendBinding *next)
    : val(val), thread(th), next(next) {}
  USEFREELISTMEMORY;

  void dispose() 
  {
    freeListDispose(this,sizeof(PendBinding));
  }
  PendBinding *gcPendBinding();
};

class PerdioVar: public GenCVariable {
  TaggedPtr tagged;  // TODO: check if can reuse home
  union {
    PendBinding *bindings;
    ProxyList *proxies;
    Tertiary *tert;
  } u;
public:
  PerdioVar() : GenCVariable(PerdioVariable) {
    u.proxies=0;
    tagged.setType(PV_MANAGER);
  }

  PerdioVar(int i) : GenCVariable(PerdioVariable) {
    u.bindings=0;
    tagged.setType(PV_PROXY);
    setIndex(i);
  }

  PerdioVar(Tertiary *t, int i) : GenCVariable(PerdioVariable) {
    u.tert = t;
    tagged.setType(PV_TERTPROXY);
    setIndex(i);
  }

  void globalize(int i) { tagged.setType(PV_MANAGER); tagged.setIndex(i); }

  Bool isManager()   { return tagged.getType()==PV_MANAGER; }
  Bool isProxy()     { return tagged.getType()==PV_PROXY; }
  Bool isTertProxy() { return tagged.getType()==PV_TERTPROXY; }

  int getIndex() { return tagged.getIndex(); }
  void setIndex(int i) { tagged.setIndex(i); if (isTertProxy()) u.tert->setIndex(i); }

  Bool valid(TaggedRef *varPtr, TaggedRef v);
  
  size_t getSize(void) { return sizeof(PerdioVar); }
  
  Tertiary *getTertiary() { Assert(isTertProxy()); return u.tert; };

  void registerSite(Site* sd) {
    Assert(isManager());
    u.proxies = new ProxyList(sd,u.proxies);
  }

  Bool isRegistered(Site* sd) {
    Assert(isManager());
    for (ProxyList *pl = u.proxies; pl != 0; pl=pl->next) {
      if (pl->sd==sd) return OK;
    }
    return NO;
  }

  void primBind(TaggedRef *lPtr,TaggedRef v);
  Bool unifyPerdioVar(TaggedRef * vptr, TaggedRef * tptr, Bool prop);

  int hasVal() { Assert(isProxy()); return u.bindings!=0; }
  void setVal(OZ_Term t) {
    Assert(isProxy());
    Assert(u.bindings==0);
    oz_stop(oz_currentThread);
    u.bindings=new PendBinding(t,oz_currentThread,0);
  }
  void pushVal(OZ_Term t) {
    Assert(isProxy());
    Assert(u.bindings!=0);
    oz_stop(oz_currentThread);
    u.bindings->next=new PendBinding(t,oz_currentThread,u.bindings->next);
  }
  void redirect(OZ_Term val);
  void acknowledge(OZ_Term *ptr);

  ProxyList *getProxies() { Assert(isManager()); return u.proxies; }

  void addSuspPerdioVar();

  void gcPerdioVar(void);
};

inline
Bool isPerdioVar(TaggedRef term)
{
  GCDEBUG(term);
  return isCVar(term) && (tagged2CVar(term)->getType() == PerdioVariable);
}

inline
PerdioVar *tagged2PerdioVar(TaggedRef t) {
  Assert(isPerdioVar(t));
  return (PerdioVar *) tagged2CVar(t);
}

#endif
