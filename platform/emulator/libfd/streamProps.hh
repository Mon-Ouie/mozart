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
 *     http://mozart.ps.uni-sb.de
 *
 *  See the file "LICENSE" or
 *     http://mozart.ps.uni-sb.de/LICENSE.html
 *  for information on usage and redistribution
 *  of this file, and for a DISCLAIMER OF ALL
 *  WARRANTIES.
 *
 */

#include "std.hh"

//-----------------------------------------------------------------------------
class DisjunctivePropagatorStream : public OZ_Propagator {
private:
  // The finite domains
  OZ_Term * reg_fds;
  // overall number of FDs
  int reg_size;

  // The durations
  int * reg_durs;

  OZ_Term stream;

  static OZ_PropagatorProfile profile;
public:
  DisjunctivePropagatorStream(OZ_Term, OZ_Term, OZ_Term);
  ~DisjunctivePropagatorStream();
  virtual size_t sizeOf(void) { return sizeof(DisjunctivePropagatorStream); }
  virtual void updateHeapRefs(OZ_Boolean);
  virtual OZ_Return propagate(void);
  virtual OZ_Term getParameters(void) const { RETURN_LIST1(stream); }
  virtual OZ_PropagatorProfile * getProfile(void) const { return &profile; }
};

//-----------------------------------------------------------------------------
class DistinctPropagatorStream : public OZ_Propagator {
private:
  // The finite domains
  OZ_Term * reg_fds;
  // overall number of FDs
  int reg_size;

  OZ_Term stream;

  static OZ_PropagatorProfile profile;
public:
  DistinctPropagatorStream(OZ_Term, OZ_Term);
  ~DistinctPropagatorStream();
  virtual size_t sizeOf(void) { return sizeof(DistinctPropagatorStream); }
  virtual void updateHeapRefs(OZ_Boolean);
  virtual OZ_Return propagate(void);
  virtual OZ_Term getParameters(void) const { RETURN_LIST1(stream); }
  virtual OZ_PropagatorProfile * getProfile(void) const { return &profile; }
};
