/*
 *  Authors:
 *    Michael Mehl (mehl@dfki.de)
 *    Kostja Popow (popow@ps.uni-sb.de)
 *    Ralf Scheidhauer (Ralf.Scheidhauer@ps.uni-sb.de)
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

#if defined(INTERFACE) && !defined(VAR_ALL)
#pragma implementation "var_base.hh"
#endif

#include "var_base.hh"
#include "var_all.hh"

/*
 * Class VariableNamer: assign names to variables
 */

class VariableNamer {
public:
  TaggedRef var;
  const char *name;
  VariableNamer *next;
  VariableNamer(TaggedRef var, const char *name, VariableNamer *next)
    : var(var), name(name), next(next) {}
};

static
VariableNamer *allnames = NULL;

const char *oz_varGetName(TaggedRef v)
{
  for (VariableNamer *i = allnames; i!=NULL; i = i->next) {
    if (OZ_isVariable(i->var) && oz_eq(i->var,v)) {
      return i->name;
    }
  }
  return "_";
}

void oz_varAddName(TaggedRef v, const char *nm)
{
  /* check if already in there */
  Assert(nm && *nm);
  VariableNamer *aux = allnames;
  while(aux) {
    if (strcmp(aux->name,nm)==0) {
      aux->var=v;
      return;
    }
    aux = aux->next;
  }
  
  /* not found so add a new one */
  static int counter = 50; // mm2: magic constant
  if (counter-- == 0) {
    oz_varCleanup();
    counter = 50;
  }
  VariableNamer *n = new VariableNamer(v,nm,allnames);
  allnames = n;
  OZ_protect(&n->var);
}

/* remove all entries, that are not a variable */
void oz_varCleanup()
{
  VariableNamer *aux = allnames;
  allnames = NULL;
  while(aux) {
    VariableNamer *aux1 = aux;
    aux = aux->next;
    if (OZ_isVariable(aux1->var)) {
      aux1->next = allnames;
      allnames = aux1;
    } else {      
      OZ_unprotect(&aux1->var);
      delete aux1;
    }
  }
}

#ifdef DEBUG_STABLE
Thread *board_constraints_thr = NULL;
SuspList * board_constraints = NULL;

void printBCDebug(Board * b) { printBC(cerr, b); }

void printBC(ostream &ofile, Board * b)
{
  SuspList *sl;
  Board *hb;

  sl = board_constraints; 
  board_constraints = (SuspList *) NULL;

  while (sl != NULL) {
    Thread *thr = sl->getElem ();
    if (thr->isDeadThread () ||
        (hb = thr->getBoard()) == NULL ||
        hb->isFailed ()) {
      sl = sl->dispose ();
      continue;
    }

    thr->print (ofile);
    ofile << endl;
    if (b) { 
      ofile << "    ---> " << (void *) b << endl; 
    }

    sl = sl->getNext();
    board_constraints = new SuspList (thr, board_constraints);
  }

  ofile.flush();
}

#endif

Bool oz_var_valid(OzVariable *cv,TaggedRef *ptr,TaggedRef val) {
  return oz_var_validINLINE(cv,ptr,val);
}

OZ_Return oz_var_unify(OzVariable *cv,TaggedRef *ptr,TaggedRef *val,
		      ByteCode *scp) {
  return oz_var_unifyINLINE(cv,ptr,val,scp);
}

OZ_Return oz_var_bind(OzVariable *cv,TaggedRef *ptr,TaggedRef val,
		      ByteCode *scp) {
  return oz_var_bindINLINE(cv,ptr,val,scp);
}

void oz_var_addSusp(OzVariable *cv, TaggedRef *v, Suspension susp,
		   int unstable = TRUE) {
  oz_var_addSuspINLINE(cv, v, susp, unstable);
}

void oz_var_printStream(ostream &out, const char *s, OzVariable *cv, int depth)
{
  switch (cv->getType()) {
  case OZ_VAR_SIMPLE:
    out << s;
    ((SimpleVar *)cv)->printStream(out,depth); return;
  case OZ_VAR_FUTURE:
    out << s;
    ((Future *)cv)->printStream(out,depth); return;
  case OZ_VAR_BOOL:
    out << s;
    ((OzBoolVariable*)cv)->printStream(out,depth); return;
  case OZ_VAR_FD:
    out << s;
    ((OzFDVariable*)cv)->printStream(out,depth); return;
  case OZ_VAR_OF:
    ((OzOFVariable*)cv)->printStream(out,depth); return;
  case OZ_VAR_FS:
    out << s;
    ((OzFSVariable*)cv)->printStream(out,depth); return;
  case OZ_VAR_CT:
    out << s;
    ((OzCtVariable*)cv)->printStream(out,depth); return;
  case OZ_VAR_EXT:
    out << s;
    ((ExtVar *)cv)->printStreamV(out,depth); return;
  default:
    error("not impl"); return;
  }
}

int oz_var_getSuspListLength(OzVariable *cv)
{
  Assert(cv->getType()!=OZ_VAR_INVALID);

  switch (cv->getType()){
  case OZ_VAR_BOOL:   return ((OzBoolVariable*)cv)->getSuspListLength();
  case OZ_VAR_FD:     return ((OzFDVariable*)cv)->getSuspListLength();
  case OZ_VAR_OF:     return ((OzOFVariable*)cv)->getSuspListLength();
  case OZ_VAR_FS:     return ((OzFSVariable*)cv)->getSuspListLength();
  case OZ_VAR_EXT:    return ((ExtVar *)cv)->getSuspListLengthV();
  default:            return cv->getSuspListLengthS();
  }
}
