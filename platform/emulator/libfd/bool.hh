/*
 *  Authors:
 *    Tobias Mueller (tmueller@ps.uni-sb.de)
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

#ifndef __BOOL_HH__
#define __BOOL_HH__

#include "std.hh"

//-----------------------------------------------------------------------------

class ConjunctionPropagator : public Propagator_D_D_D {
private:
  static OZ_CFunHeader spawner;
public:
  ConjunctionPropagator(OZ_Term x, OZ_Term y, OZ_Term z)
    : Propagator_D_D_D(x, y, z) { };

  virtual OZ_Return propagate(void);
  virtual OZ_CFunHeader * getHeader(void) const { return &spawner; }
};

//-----------------------------------------------------------------------------

class DisjunctionPropagator : public Propagator_D_D_D {
private:
  static OZ_CFunHeader spawner;
public:
  DisjunctionPropagator(OZ_Term x, OZ_Term y, OZ_Term z)
    : Propagator_D_D_D(x, y, z) { };

  virtual OZ_Return propagate(void);
  virtual OZ_CFunHeader * getHeader(void) const { return &spawner; }
};

//-----------------------------------------------------------------------------

class XDisjunctionPropagator : public Propagator_D_D_D {
private:
  static OZ_CFunHeader spawner;
public:
  XDisjunctionPropagator(OZ_Term x, OZ_Term y, OZ_Term z)
    : Propagator_D_D_D(x, y, z) { };

  virtual OZ_Return propagate(void);
  virtual OZ_CFunHeader * getHeader(void) const { return &spawner; }
};

//-----------------------------------------------------------------------------

class ImplicationPropagator : public Propagator_D_D_D {
private:
  static OZ_CFunHeader spawner;
public:
  ImplicationPropagator(OZ_Term x, OZ_Term y, OZ_Term z)
    : Propagator_D_D_D(x, y, z) { };

  virtual OZ_Return propagate(void);
  virtual OZ_CFunHeader * getHeader(void) const { return &spawner; }
};

//-----------------------------------------------------------------------------

class EquivalencePropagator : public Propagator_D_D_D {
private:
  static OZ_CFunHeader spawner;
public:
  EquivalencePropagator(OZ_Term x, OZ_Term y, OZ_Term z)
    : Propagator_D_D_D(x, y, z) { };

  virtual OZ_Return propagate(void);
  virtual OZ_CFunHeader * getHeader(void) const { return &spawner; }
};

//-----------------------------------------------------------------------------

class NegationPropagator : public Propagator_D_D {
private:
  static OZ_CFunHeader spawner;
public:
  NegationPropagator(OZ_Term x, OZ_Term y)
    : Propagator_D_D(x, y) { };

  virtual OZ_Return propagate(void);
  virtual OZ_CFunHeader * getHeader(void) const { return &spawner; }
};


#endif
