/*
 *  Authors:
 *    Author's name (Author's email address)
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
 *     $MOZARTURL$
 *
 *  See the file "LICENSE" or
 *     $LICENSEURL$
 *  for information on usage and redistribution
 *  of this file, and for a DISCLAIMER OF ALL
 *  WARRANTIES.
 *
 */
/*
  Hydra Project, DFKI Saarbruecken,
  Stuhlsatzenhausweg 3, D-66123 Saarbruecken, Phone (+49) 681 302-5312
  Author: tmueller
  Last modified: $Date$ from $Author$
  Version: $Revision$
  State: $State$

  ------------------------------------------------------------------------
*/

#ifndef __INTSETS_HH__
#define __INTSETS_HH__

#include "fsstd.hh"

class FSetsMinPropagator : public Propagator_S_D {
private:
  static OZ_CFunHeader header;
public:
  FSetsMinPropagator(OZ_Term v, OZ_Term i)
    : Propagator_S_D(v, i) {}

  virtual OZ_Return propagate(void);

  virtual OZ_CFunHeader * getHeader(void) const {
    return &header;
  }
};

class FSetsMaxPropagator : public Propagator_S_D {
private:
  static OZ_CFunHeader header;
public:
  FSetsMaxPropagator(OZ_Term v, OZ_Term i)
    : Propagator_S_D(v, i) {}

  virtual OZ_Return propagate(void);

  virtual OZ_CFunHeader * getHeader(void) const {
    return &header;
  }
};

class Propagator_S : public OZ_Propagator {
protected:
  OZ_Term _s;
public:
  Propagator_S(OZ_Term s)
    : _s(s) {}
  virtual size_t sizeOf(void) {
    return sizeof(Propagator_S);
  }
  virtual void updateHeapRefs(OZ_Boolean) {
    OZ_updateHeapTerm(_s);
  }
  virtual OZ_Term getParameters(void) const {
    return OZ_cons(_s,OZ_nil());
  }
};

class FSetsConvexPropagator : public Propagator_S {
private:
  static OZ_CFunHeader header;
public:
  FSetsConvexPropagator(OZ_Term v)
    : Propagator_S(v) {}
  virtual OZ_Return propagate(void);
  virtual OZ_CFunHeader * getHeader(void) const {
    return &header;
  }
};


class FSetMatchPropagator : public Propagator_S_VD {
private:
  static OZ_CFunHeader header;

  int _firsttime, _last_min, _last_max, _l, _k;

public:
  FSetMatchPropagator(OZ_Term s, OZ_Term vd)
    : _firsttime(1), Propagator_S_VD(s, vd) { }
  virtual OZ_Return propagate(void);
  virtual OZ_CFunHeader * getHeader(void) const {
    return &header;
  }
  virtual size_t sizeOf(void) {
    return sizeof(FSetMatchPropagator);
  }
};

class FSetMinNPropagator : public Propagator_S_VD {
private:
  static OZ_CFunHeader header;

  int _firsttime, _last_min, _last_max, _l, _k;

public:
  FSetMinNPropagator(OZ_Term s, OZ_Term vd)
    : _firsttime(1), Propagator_S_VD(s, vd) { }
  virtual OZ_Return propagate(void);
  virtual OZ_CFunHeader * getHeader(void) const {
    return &header;
  }
  virtual size_t sizeOf(void) {
    return sizeof(FSetMinNPropagator);
  }
};

class FSetMaxNPropagator : public Propagator_S_VD {
private:
  static OZ_CFunHeader header;

  int _firsttime, _last_min, _last_max, _l, _k;

public:
  FSetMaxNPropagator(OZ_Term s, OZ_Term vd)
    : _firsttime(1), Propagator_S_VD(s, vd) { }
  virtual OZ_Return propagate(void);
  virtual OZ_CFunHeader * getHeader(void) const {
    return &header;
  }
  virtual size_t sizeOf(void) {
    return sizeof(FSetMaxNPropagator);
  }
};

class FSetSeqPropagator : public Propagator_VS {
private:
  static OZ_CFunHeader header;

public:
  FSetSeqPropagator(OZ_Term s)
    : Propagator_VS(s) { }
  virtual OZ_Return propagate(void);
  virtual OZ_CFunHeader * getHeader(void) const {
    return &header;
  }
};

#endif /* __INTSETS_HH__ */


//-----------------------------------------------------------------------------
// eof
