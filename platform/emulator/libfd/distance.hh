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

#include "std.hh"

class DistancePropagatorLeq : public Propagator_D_D_D_I {
private:
  static OZ_CFunHeader spawner;
public:
  DistancePropagatorLeq(OZ_Term x, OZ_Term y,  OZ_Term z, int c)
    : Propagator_D_D_D_I(x, y, z, c) {}

  virtual OZ_Return propagate(void);
  virtual OZ_CFunHeader * getHeader(void) const { return &spawner; }
  virtual OZ_Term getParameters(void) const { return Propagator_D_D_D_I::getParameters(SUM_OP_LEQ); }
};

class DistancePropagatorGeq : public Propagator_D_D_D_I {
private:
  static OZ_CFunHeader spawner;
public:
  DistancePropagatorGeq(OZ_Term x, OZ_Term y,  OZ_Term z, int c)
    : Propagator_D_D_D_I(x, y, z, c) {}

  virtual OZ_Return propagate(void);
  virtual OZ_CFunHeader * getHeader(void) const { return &spawner; }
  virtual OZ_Term getParameters(void) const { return Propagator_D_D_D_I::getParameters(SUM_OP_GEQ); }
};

class DistancePropagatorEq : public Propagator_D_D_D_I {
private:
  static OZ_CFunHeader spawner;
public:
  DistancePropagatorEq(OZ_Term x, OZ_Term y,  OZ_Term z, int c)
    : Propagator_D_D_D_I(x, y, z, c) {}

  virtual OZ_Return propagate(void);
  virtual OZ_CFunHeader * getHeader(void) const { return &spawner; }
  virtual OZ_Term getParameters(void) const { return Propagator_D_D_D_I::getParameters(SUM_OP_EQ); }
};

class DistancePropagatorNeq : public Propagator_D_D_D_I {
private:
  static OZ_CFunHeader spawner;
public:
  DistancePropagatorNeq(OZ_Term x, OZ_Term y,  OZ_Term z, int c)
    : Propagator_D_D_D_I(x, y, z, c) {}

  virtual OZ_Return propagate(void);
  virtual OZ_CFunHeader * getHeader(void) const { return &spawner; }
  virtual OZ_Term getParameters(void) const { return Propagator_D_D_D_I::getParameters(SUM_OP_NEQ); }
};
