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
 *     http://mozart.ps.uni-sb.de
 *
 *  See the file "LICENSE" or
 *     http://mozart.ps.uni-sb.de/LICENSE.html
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
/*===================================================================
 * RESOURCE
 *=================================================================== */

class DistResource: public Tertiary{
  friend void ConstTerm::gcConstRecurse(void);
public:
  NO_DEFAULT_CONSTRUCTORS(DistResource);
  DistResource(int i):Tertiary(i,Co_Resource,Te_Proxy){}

};
/************************************************************/
/*  ResourceTable                                           */
/************************************************************/


class ResourceHashTable: public GenHashTable{
  int hash(TaggedRef entity){
    return (int) entity;}

public:
  ResourceHashTable(int i):GenHashTable(i){}

  void add(TaggedRef entity, int index){
    Assert(find(entity)==RESOURCE_NOT_IN_TABLE);
    int hvalue = hash(entity);
    GenHashTable::htAdd(hvalue,(GenHashBaseKey*)entity,
                        (GenHashEntry*)index);}

  int find(TaggedRef entity){
     int hvalue = hash(entity);
     GenHashNode *aux = htFindFirst(hvalue);
     if(aux)
       return (int)aux->getEntry();
     return RESOURCE_NOT_IN_TABLE;
  }

  void gcResourceTable();
};

extern ResourceHashTable *resourceTable;

#define RHT resourceTable

ConstTerm* gcDistResourceImpl(ConstTerm*);

#endif
