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
};

class ProxyList {
public:
  int sd;
  ProxyList *next;
public:
  ProxyList(int sd,ProxyList *next) :sd(sd),next(next) {}
};

class PerdioVar: public GenCVariable {
  TaggedPtr tagged;
  union {
    TaggedRef binding;
    ProxyList *proxies;
  } u;
public:
  PerdioVar() : GenCVariable(PerdioVariable) {
    u.proxies=0;
    tagged.setType(PV_MANAGER);
  }

  PerdioVar(int i) : GenCVariable(PerdioVariable) {
    u.binding=0;
    tagged.setType(PV_PROXY);
    tagged.setIndex(i);
  }

  void globalize(int i) { tagged.setType(PV_MANAGER); tagged.setIndex(i); }

  Bool isManager() { return tagged.getType()==PV_MANAGER; }
  Bool isProxy() { return tagged.getType()==PV_PROXY; }

  int getIndex() { return tagged.getIndex(); }
  void setIndex(int i) { tagged.setIndex(i); }

  Bool valid(TaggedRef *varPtr, TaggedRef v);

  size_t getSize(void) { return sizeof(PerdioVar); }

  void registerSite(int sd) {
    Assert(isManager());
    u.proxies = new ProxyList(sd,u.proxies);
  }

  void primBind(TaggedRef *lPtr,TaggedRef v);
  Bool unifyPerdioVar(TaggedRef * vptr, TaggedRef * tptr, Bool prop);

  OZ_Term getVal() { Assert(isProxy()); return u.binding; }
  int hasVal() { Assert(isProxy()); return u.binding!=0; }
  void setVal(OZ_Term t) { Assert(isProxy()); u.binding=t; }
  ProxyList *getProxies() { Assert(isManager()); return u.proxies; }

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
