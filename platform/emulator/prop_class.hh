/*
 *  Authors:
 *    Kostja Popow (popow@ps.uni-sb.de)
 *    Tobias Mueller (tmueller@ps.uni-sb.de)
 *
 *  Contributors:
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

#ifndef __PROP_CLASS_HH
#define __PROP_CLASS_HH

#include "suspendable.hh"

class Propagator : public Suspendable {
private:
  OZ_Propagator *     _p;
  static Propagator * _runningPropagator;

public:
  USEFREELISTMEMORY;
  NO_DEFAULT_CONSTRUCTORS(Propagator);

  Propagator(OZ_Propagator * p, Board * b)
    : Suspendable(p->isMonotonic() ? 0 : SF_NMO, b), _p(p) {
    Assert(p && b);
  }

  OZ_Propagator * getPropagator(void) {
    return _p;
  }
  void setPropagator(OZ_Propagator * p) {
    Assert (p && _p);
    _p = p;
    if (!_p->isMonotonic())
      setNMO();
  }

  static void setRunningPropagator(Propagator * p) {
    _runningPropagator = p;
  }
  static Propagator * getRunningPropagator(void) {
    return _runningPropagator;
  }

  void dispose(void) {
    delete _p;
  }

  Propagator * gcPropagator(void);
  void gcRecurse(void);

  OZ_NonMonotonic::order_t getOrder(void) {
    return _p->getOrder();
  }

  OZ_Propagator * swapPropagator(OZ_Propagator * prop) {
    OZ_Propagator * p = _p;
    setPropagator(prop);
    return p;
  }

  OZPRINTLONG;

};

inline
Bool isUnifyCurrentPropagator () {
  return Propagator::getRunningPropagator()->isUnify();
}

#endif
