/*
  Hydra Project, DFKI Saarbruecken,
  Stuhlsatzenhausweg 3, D-66123 Saarbruecken, Phone (+49) 681 302-5312
  Author: tmueller
  Last modified: $Date$ from $Author$
  Version: $Revision$
  State: $State$

  ------------------------------------------------------------------------
*/

#ifndef __FDHOOK__H__
#define __FDHOOK__H__

#ifdef __GNUC__
#pragma interface
#endif

#include "types.hh"

#include "am.hh"
#include "suspension.hh"

extern Suspension * FDcurrentTaskSusp;

void reviveCurrentTaskSusp(void);
void killPropagatedCurrentTaskSusp(void);
void dismissCurrentTaskSusp(void);

void undoTrailing(int n);

SuspList * addVirtualConstr(SuspList * list, SuspList * elem, Board * home);

Suspension * makeHeadSuspension(OZ_Bool (*fun)(int, OZ_Term[]),
				OZ_Term * args, int arity);

void addVirtualConstr(SVariable * var, SuspList * elem);

#endif
