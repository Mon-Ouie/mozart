/*
 *  Authors:
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


#if defined(INTERFACE) && !defined(PEANUTS)
#pragma implementation "indexing.hh"
#endif

#include "am.hh"
#include "indexing.hh"
#include "allgenvar.hh"

EntryTable newEntryTable(int sz)
{
  EntryTable help = new HTEntry*[sz];
  for (int i = 0; i <sz; i++)
    help[i] = (HTEntry*)NULL;

  return help;
}


int *IHashTable::add(Literal *constant, int label) 
{
  numentries++;
  unsigned int hsh = hash(constant->hash());
  
  if (literalTable == NULL)
    literalTable = newEntryTable(size); 
  
  /* we do not check, whether it is already in there */
  literalTable[hsh] = new HTEntry(constant,label,literalTable[hsh]);

  return literalTable[hsh]->getLabelRef();
}

int *IHashTable::add(Literal *name, SRecordArity arity,
				int label)
{
  numentries++;
  unsigned int hsh = hash(name->hash());
  
  if (functorTable == NULL)
    functorTable = newEntryTable(size); 
  
  /* we do not check, whether it is already in there */
  functorTable[hsh] = new HTEntry(name, arity, label, functorTable[hsh]);

  return functorTable[hsh]->getLabelRef();
}


int *IHashTable::add(TaggedRef number, int label)
{
  numentries++;

  unsigned int hsh;
  switch (tagTypeOf(number)) {

  case OZFLOAT:  hsh = tagged2Float(number)->hash();  break;
  case OZCONST:  hsh = tagged2BigInt(number)->hash(); break;
  case SMALLINT: hsh = smallIntHash(number);          break;    
  default:       Assert(0); return 0;
  }

  hsh = hash(hsh);

  if (numberTable == NULL)
    numberTable = newEntryTable(size); 
  
  /* we do not check, whether it is already in there */
  numberTable[hsh] = new HTEntry(number, label, numberTable[hsh]);

  return numberTable[hsh]->getLabelRef();
}


// - 'table' holds the code to branch to when indexing
// How it works:
// If none of the numbers is member of the domain, no guard can ever be
// entailed therefore goto to the else-branch. Otherwise goto varLabel, which
// wait for determination of the variable. Usually if unconstrained variables
// get bound to each other, det-nodes are not reentered, but since
// unifying two fd variables may result in a singleton (ie. determined term),
// det-nodes are reentered and we achieve completeness.

Bool IHashTable::disentailed(GenCVariable *cvar, TaggedRef *ptr)
{
  switch (cvar->getType()) {
  case FDVariable: 
  case BoolVariable: 
    {
      /* if there are no integer guards goto else-branch */
      if (!numberTable) {
	return OK;
      }
      
      // if there is at least one integer member of the domain then goto varLabel
      for (int i = 0; i < size; i++) {
	for (HTEntry* aux = numberTable[i]; aux!=NULL; aux=aux->getNext()) {
	  if (oz_cv_validINLINE(cvar,ptr,aux->getNumber()))
	    return NO;
	}
      }

      return OK;      
    }
  case OFSVariable:
    {
      GenOFSVariable *ofsvar = (GenOFSVariable*) cvar;
      if (listLabel && !ofsvar->disentailed(tagged2Literal(AtomCons),2))
	return NO;

      if (literalTable) {
	for (int i = 0; i < size; i++) {
	  for (HTEntry* aux = literalTable[i]; aux!=NULL; aux=aux->getNext()) {
	    if (!ofsvar->disentailed(aux->getLiteral(),(int)0))
	      return NO;
	  }
	}
      }

      if (functorTable) {
	for (int i = 0; i < size; i++) {
	  for (HTEntry* aux = functorTable[i]; aux!=NULL; aux=aux->getNext()) {
	    SRecordArity arity;
	    Literal *label = aux->getFunctor(arity);
	    if (sraIsTuple(arity)) {
	      if (!ofsvar->disentailed(label,getTupleWidth(arity)))
		return NO;
	    } else {
	      if (!ofsvar->disentailed(label,getRecordArity(arity)))
		return NO;
	    }
	  }
	}
      }
      return OK;      
    }

  // mm2: hack: an arbitrary number is check for validity
  case PerdioVariable:
    return !oz_cv_validINLINE((GenCVariable *)cvar,ptr,OZ_int(4711));

  default:    
    return NO;
  }
}

int switchOnTermOutline(TaggedRef term, TaggedRef *termPtr,
			IHashTable *table, TaggedRef *&sP)
{
  int offset = table->getElse();
  if (oz_isSRecord(term)) {
    if (table->functorTable) {
      SRecord *rec = tagged2SRecord(term);
      Literal *lname = rec->getLabelLiteral();
      Assert(lname!=NULL);
      unsigned int hsh = table->hash(lname->hash());
      offset = table->functorTable[hsh]->lookup(lname,rec->getSRecordArity(),offset);
      sP = rec->getRef();
    }
    return offset;
  }

  if (oz_isLiteral(term)) {
    if (table->literalTable) {
      unsigned int hsh = table->hash(tagged2Literal(term)->hash());
      offset = table->literalTable[hsh]->lookup(tagged2Literal(term),offset);
    }
    return offset;
  }

  if (isUVar(term)) {
    return 0;
  }

  if (oz_isSmallInt(term)) {
    if (table->numberTable) {
      int hsh = table->hash(smallIntHash(term));
      offset = table->numberTable[hsh]->lookup(term,offset);
    }
    return offset;
  }

  if (oz_isFloat(term)) {
    if (table->numberTable) {
      unsigned int hsh = table->hash(tagged2Float(term)->hash());
      offset = table->numberTable[hsh]->lookup(term,offset);
    }
    return offset;
  }

  if (oz_isBigInt(term)) {
    if (table->numberTable) {
      unsigned int hsh = table->hash(tagged2BigInt(term)->hash());
      offset =table->numberTable[hsh]->lookup(term,offset);
    }
    return offset;
  }

  if (oz_isFree(term)) {
    return 0;
  }

  if (isCVar(term) && !table->disentailed(tagged2CVar(term),termPtr)) {
    return 0;
  }

  if (isSVar(term)) {
    return 0;
  }

  return offset;
}
