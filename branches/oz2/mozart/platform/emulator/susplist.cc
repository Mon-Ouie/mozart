/*
  Hydra Project, DFKI Saarbruecken,
  Stuhlsatzenhausweg 3, D-66123 Saarbruecken, Phone (+49) 681 302-5312
  Author: mehl,tmueller,popow
  Last modified: $Date$ from $Author$
  Version: $Revision$
  State: $State$

  ------------------------------------------------------------------------
*/


#if defined(INTERFACE) && !defined(PEANUTS)
#pragma implementation "susplist.hh"
#endif

#include "am.hh"

#include "genvar.hh"
#include "fdprofil.hh"

#ifdef OUTLINE
#define inline
#endif

//-----------------------------------------------------------------------------
//                          class SuspList

int SuspList::length(void)
{
  int i=0;
  for(SuspList * aux = this; aux != NULL; aux = aux->next) {
    if (!aux->getElem()->isDeadThread () &&
	!aux->getElem()->isPropagated () &&
	aux->getElem()->getBoard()) {
      i++;
    }
  }
  return i;
} 

int SuspList::lengthProp(void)
{
  int i=0;
  for(SuspList * aux = this; aux != NULL; aux = aux->next) {
    if (!aux->getElem()->isDeadThread () &&
	aux->getElem()->isPropagated () &&
	aux->getElem()->getBoard()) {
      i++;
    }
  }
  return i;
}

//-----------------------------------------------------------------------------

SuspList * installPropagators(SuspList * local_list, SuspList * glob_list,
			      Board * glob_home)
{
  SuspList * aux = local_list, * ret_list = local_list;

  
  // mark up local suspensions
  while (aux) {
    aux->getElem()->markTagged();
    aux = aux->getNext();
  }

  // create references to suspensions of global variable
  aux = glob_list;
  while (aux) {
    Thread *thr = aux->getElem();
    
    if (!(thr->isDeadThread ()) && 
	(thr->isPropagator()) &&
	!(thr->isTagged ()) && 
	am.isBetween (thr->getBoard(), glob_home ) == B_BETWEEN) {
      ret_list = new SuspList (thr, ret_list);
    }
    
    aux = aux->getNext();
  }

  aux = local_list;
  while (aux) {
    aux->getElem()->unmarkTagged();
    aux = aux->getNext();
  }
  
  return ret_list;
}

#ifdef OUTLINE
#define inline
#include "susplist.icc"
#undef inline
#endif
