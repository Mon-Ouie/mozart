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
 *     http://www.mozart-oz.org
 *
 *  See the file "LICENSE" or
 *     http://www.mozart-oz.org/LICENSE.html
 *  for information on usage and redistribution
 *  of this file, and for a DISCLAIMER OF ALL
 *  WARRANTIES.
 *
 */

#ifndef __DISJOINT_HH__
#define __DISJOINT_HH__

#include "std.hh"
#include "prop_fncts.hh"

//-----------------------------------------------------------------------------

class SchedCDPropagator : public Propagator_D_I_D_I {
  friend INIT_FUNC(fdp_init);
private:
  static OZ_PropagatorProfile profile;
public:
  SchedCDPropagator(OZ_Term x, OZ_Term xd, OZ_Term y, OZ_Term yd)
    : Propagator_D_I_D_I(x, xd, y, yd) {}

  virtual OZ_Return propagate(void);
  virtual OZ_PropagatorProfile * getProfile(void) const { return &profile; }
};

class SchedCDBPropagator : public Propagator_D_I_D_I_D {
  friend INIT_FUNC(fdp_init);
private:
  static OZ_PropagatorProfile profile;
public:
  SchedCDBPropagator(OZ_Term x, OZ_Term xd, OZ_Term y, OZ_Term yd, OZ_Term b)
    : Propagator_D_I_D_I_D(x, xd, y, yd, b) {}

  virtual OZ_Return propagate(void);
  virtual OZ_PropagatorProfile * getProfile(void) const { return &profile; }
};

//-----------------------------------------------------------------------------

class TasksOverlapPropagator : public Propagator_D_I_D_I_D {
  friend INIT_FUNC(fdp_nit);

private:
  static OZ_PropagatorProfile profile;
  //
                // clause 1: t1 + d1 >: t2 /\ t2 + d2 >: t1 /\ o =: 1
  enum _var_ix1 {_cl1_t1 = 0, _cl1_t2, _cl1_o,
                 // clause 2: t1 + d1 =<: t2 /\ o =: 0
                 _cl2_t1, _cl2_t2, _cl2_o,
                 // clause 3: t2 + d2 =<: t1 /\ o =: 0
                 _cl3_t1, _cl3_t2, _cl3_o, nb_lvars };
                 // constant values
  enum _var_ix2 {_d1 = nb_lvars, _d2, nb_consts};
  //
  OZ_FiniteDomain _ld[nb_lvars];
  //
  int _first;
  PEL_FDProfile     _x_profile, _y_profile;
  PEL_PropFnctTable _prop_fnct_table;
  PEL_ParamTable    _param_table;
  PEL_FDEventLists  _el[nb_lvars];
  PEL_PropQueue     _prop_queue_cl1, _prop_queue_cl2, _prop_queue_cl3;
  //
public:
  TasksOverlapPropagator(OZ_Term x, OZ_Term xd, OZ_Term y, OZ_Term yd,
                         OZ_Term o);
  //
  virtual OZ_Return propagate(void);
  virtual OZ_PropagatorProfile * getProfile(void) const { return &profile; }
  virtual void updateHeapRefs(OZ_Boolean duplicate = OZ_FALSE) {
    Propagator_D_I_D_I_D::updateHeapRefs(duplicate);
    //
    // here goes the additional stuff:
    _prop_fnct_table.gc();
    _param_table.gc();
    for (int i = nb_lvars; i--; ) {
      _ld[i].copyExtension();
      _el[i].gc();
    }
  }
  virtual size_t sizeOf(void) { return sizeof(TasksOverlapPropagator); }
};

//-----------------------------------------------------------------------------

#endif // __DISJOINT_HH__
