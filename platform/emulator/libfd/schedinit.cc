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
 *     http://mozart.ps.uni-sb.de
 *
 *  See the file "LICENSE" or
 *     http://mozart.ps.uni-sb.de/LICENSE.html
 *  for information on usage and redistribution
 *  of this file, and for a DISCLAIMER OF ALL
 *  WARRANTIES.
 *
 */

#include "scheduling.hh"
#include "schedulingDist.hh"
#include "schedulingDistAux.hh"
#include "taskintervals.hh"
#include "streamProps.hh"

void sched_init(void)
{
#ifdef OZ_DEBUG
  oz_fsetdebugprint("*** DEBUG-FDLIB ***");
#elif OZ_PROFILE
  oz_fsetdebugprint("*** PROFILE-FDLIB ***");
#endif

  SchedCardPropagator::profile =         "sched_disjoint_card";
  CPIteratePropagator::profile =         "sched_cpIterate";
  CPIteratePropagatorCap::profile =      "sched_cpIterateCap";
  CPIteratePropagatorCapUp::profile =    "sched_cpIterateCapUp";
  DisjunctivePropagator::profile =       "sched_disjunctive";
  TaskIntervalsProof::profile =          "sched_taskIntervalsProof";
  FirstsLasts::profile =                 "sched_firstsLasts";
  DisjunctivePropagatorStream::profile = "sched_disjunctiveStream";
  CPIteratePropagatorCumTI::profile =    "sched_cumulativeTI";
  TaskIntervalsPropagator::profile =     "sched_taskIntervals";
}
