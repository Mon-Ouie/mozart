/*
 *  Authors:
 *    Erik Klintskog (erik@sics.se)
 *
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

#ifndef __RESOURCE_HH
#define __RESOURCE_HH

#ifdef INTERFACE
#pragma interface
#endif

#include "base.hh"
#include "value.hh"

/************************************************************/
/*  Defines                                                 */
/************************************************************/

#define RESOURCE_HASH_TABLE_DEFAULT_SIZE 25
#define RESOURCE_NOT_IN_TABLE 0-1

enum DPResourceType{
  UD_unknown = 0,

  UD_thread,
  UD_array,
  UD_dictionary,
  UD_last
};

extern char *dpresource_names[];

/************************************************************/
/*  Defines                                                 */
/************************************************************/

class DistResource: public Tertiary{
public:
  NO_DEFAULT_CONSTRUCTORS(DistResource)
  DistResource(int i):Tertiary(i,Co_Resource,Te_Proxy){}

};
/************************************************************/
/*  ResourceTable                                           */
/************************************************************/


class ResourceHashTable: public GenHashTable {
  unsigned int hash(TaggedRef entity){
    return (unsigned int) entity;}

public:
  ResourceHashTable(int i):GenHashTable(i){}

  //
  void add(OZ_Term entity, int oti) {
    // kost@ : this is what we can deal with:
    Assert((!oz_isRef(entity) && !oz_isVar(entity)) ||
           (oz_isRef(entity) && oz_isVar(*tagged2Ref(entity))));
    Assert(find(entity) == RESOURCE_NOT_IN_TABLE);
    unsigned int hvalue;
    GenHashBaseKey *ghbk;
    GenHashEntry *ghe;

    //
    hvalue = hash(entity);
    GenCast(entity, OZ_Term, ghbk, GenHashBaseKey*);
    GenCast(oti, int, ghe, GenHashEntry*);
    GenHashTable::htAddU(hvalue, ghbk, ghe);
  }

  //
  int find(TaggedRef entity) {
    // kost@ : this is what we can deal with:
    Assert((!oz_isRef(entity) && !oz_isVar(entity)) ||
           (oz_isRef(entity) && oz_isVar(*tagged2Ref(entity))));
    unsigned int hvalue = hash(entity);
    GenHashNode *aux;

    //
  repeat:
    aux = htFindFirstU(hvalue);
    while (aux){
      OZ_Term te;

      //
      GenCast(aux->getBaseKey(), GenHashBaseKey*, te, OZ_Term);

      //
      // Now, there are three cases: found, not found, and found a
      // dead entry;
      if (te == entity) {
        // that's the entry we're talking about: let's check whether
        // the corresponding oe entry is still alive:
        int oti;
        OwnerEntry *oe;

        //
        GenCast(aux->getEntry(), GenHashEntry*, oti, int);
        oe = OT->index2entry(oti);

        //
        if (oe && oe->isRef() && oe->getRef() == entity) {
          return (oti);         // found!
        } else {
          // The wrong one: that is, the current entry is outdated
          // and should be removed;
          (void) htSubU(hvalue, aux);

          // must start from scratch since 'htSub()' is NOT compatible
          // with 'htFindFirst()' & Co.;
          goto repeat;
        }
        Assert(0);

        //
      } if (oz_isRef(te) && !oz_isVar(*tagged2Ref(te))) {
        // bound variables can be (and should be) discarded as well;
        (void) htSubU(hvalue, aux);
        goto repeat;
      } else {
        aux = htFindNextU(aux, hvalue);
      }
    }

    //
    return (RESOURCE_NOT_IN_TABLE);
  }

  //
  void gcResourceTable();
};

extern ResourceHashTable *resourceTable;

#define RHT resourceTable

ConstTerm* gcDistResourceImpl(ConstTerm*);

#endif
