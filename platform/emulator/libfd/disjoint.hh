/*
 *  Authors:
 *    Joerg Wuertz (wuertz@dfki.de)
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

#ifndef __DISJOINT_HH__
#define __DISJOINT_HH__

#include "std.hh"

//-----------------------------------------------------------------------------

class SchedCDPropagator : public Propagator_D_I_D_I {
private:
  static OZ_PropagatorProfile profile;
public:
  SchedCDPropagator(OZ_Term x, OZ_Term xd, OZ_Term y, OZ_Term yd)
    : Propagator_D_I_D_I(x, xd, y, yd) {}
  
  virtual OZ_Return propagate(void);
  virtual OZ_PropagatorProfile * getProfile(void) const { return &profile; }
};

class SchedCDBPropagator : public Propagator_D_I_D_I_D {
private:
  static OZ_PropagatorProfile profile;
public:
  SchedCDBPropagator(OZ_Term x, OZ_Term xd, OZ_Term y, OZ_Term yd, OZ_Term b)
    : Propagator_D_I_D_I_D(x, xd, y, yd, b) {}
  
  virtual OZ_Return propagate(void);
  virtual OZ_PropagatorProfile * getProfile(void) const { return &profile; }
};


#endif // __DISJOINT_HH__
