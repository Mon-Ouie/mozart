/*
  Hydra Project, DFKI Saarbruecken,
  Stuhlsatzenhausweg 3, D-W-6600 Saarbruecken 11, Phone (+49) 681 302-5261
  Author: tmueller
  Last modified: $Date$ from $Author$
  Version: $Revision$
  State: $State$

  ------------------------------------------------------------------------
*/

#if defined(__GNUC__) && !defined(NOPRAGMA)
#pragma implementation "hashtbl.hh"
#endif

#include <math.h>
#include <malloc.h>

#include "hashtbl.hh"
#include "misc.hh"


const int STEP=5;         
const double MAXFULL=0.6; // The max. load of HashTable


inline Bool isPrime(int prime)
{
  if (prime%2 == 0) {
    return NO;
  }
  for(int i=3; i<=sqrt(prime); i+=2) {
    if (prime%i == 0) {
      return NO;
    }
  }

  return OK;  
}

int nextPrime(int prime)
{
  if (prime%2 == 0) {
    prime++;
  }

  while(1) {
    if (isPrime(prime)) {
      return prime;
    }
    prime += 2;
  }
}



HashTable::HashTable(hashType typ, int s)
{
  DebugCheck(s==0, error("HashTable initialized with zero size"););
  s = nextPrime(s);
  tableSize = s;
  type = typ;
  counter = 0;
  percent = (int) (MAXFULL * tableSize);
  table = new HashNode[tableSize];
  for(int i=0; i<tableSize; i++) {
    table[i].setEmpty();
  }
}

HashTable::~HashTable() {
  // dispose hash table itself
    delete [] table;
}



// M e t h o d s

inline int HashTable::hashFunc(intlong i) {
  return i % tableSize;
}

inline int HashTable::hashFunc(char *s) {
// 'hashfunc' is taken from 'Aho,Sethi,Ullman: Compilers ...', page 436
  char *p = s;
  unsigned h = 0, g;
  for(; *p; p++) {
    h = (h << 4) + (*p);
    if ((g = h & 0xf0000000)) {
      h = h ^ (g >> 24);
      h = h ^ g;
    }
  }
  return h % tableSize;
}

unsigned HashTable::memRequired(int valSize)
{
  unsigned mem = tableSize * sizeof(HashNode);
  for(int i = 0; i < tableSize; i++){
    HashNode *lnp = &table[i];
    if (type == CHARTYPE) {
      if (! lnp->isEmpty()) {
	mem += sizeof(HashNode);
	mem += strlen(lnp->key.fstr);
	mem += valSize;
      } else {
	if (! lnp->isEmpty()) {
	  mem += sizeof(HashNode);
	  mem += sizeof(lnp->key.fint);
	  mem += valSize;
	}
      }
    }
  }
  return mem;
}

// add a new entry. Replace it iff "replace == OK" 
// otherwise return NO



void HashTable::resize()
{
  int oldSize = tableSize;
  tableSize = nextPrime(tableSize*2);
  counter = 0;
  percent = (int) (MAXFULL * tableSize);
  HashNode* neu = new HashNode[tableSize];
  HashNode* old = table;    
  table = neu;
  int i;
  for (i=0; i<tableSize; i++) 
    neu[i].setEmpty();
  if (type == INTTYPE) {
    for (i=0; i<oldSize; i++) {
      if (! old[i].isEmpty()) 
	aadd(old[i].value,old[i].key.fint,NO);
    }
  } else {
    for (i=0;i<oldSize;i++) {
      if (! old[i].isEmpty()) {
	aadd(old[i].value,old[i].key.fstr,NO);
	free(old[i].key.fstr);
      }
    }
  }
  delete [] old;
}


inline int incKey(int key, int s)
{
  key += STEP;
  if (key >= s) {
    key -= s;
  }
  return key;
}


inline int HashTable::findIndex(char *s)
{
  int key = hashFunc(s);
  while (! table[key].isEmpty() && (strcmp(table[key].key.fstr,s)!=0)) {
    key = incKey(key,tableSize);
  }
  return key;
}

inline int HashTable::findIndex(intlong i)
{
  int key = hashFunc(i);
  while (! table[key].isEmpty() && table[key].key.fint != i) {
    key = incKey(key,tableSize);
  }
  return key;
}


// add a new entry. Replace it iff "replace == OK" 
// otherwise return NO


Bool HashTable::aadd(void *d, char *s, Bool replace)
{
  if (counter > percent)
    resize();
  
  int key = findIndex(s);
  if (! table[key].isEmpty()) {     // already in there
    if (replace == NO) {
      return NO;
    } else {               // replace old entry
      free(table[key].key.fstr);
    }
  } else {
    counter++;
  }
  
  table[key].key.fstr  = ozstrdup(s);
  table[key].value = d;
  return OK;
}

Bool HashTable::aadd(void *d, intlong i, Bool replace)
{
  if (counter > percent)
    resize();
  
  int key = findIndex(i);
  if (! table[key].isEmpty()) {     // already in there
    if (replace == NO) {
      return NO;
    } // else replace old entry
  } else {
    counter++;
  }
  
  table[key].key.fint  = i;
  table[key].value = d;
  return OK;
}


void *HashTable::ffind(char *s)
{
  int key = findIndex(s);
  return (table[key].isEmpty())
    ? htEmpty : table[key].value;
}

void *HashTable::ffind(intlong i)
{
  int key = findIndex(i);
  return (table[key].isEmpty())
    ? htEmpty : table[key].value;
}

int HashTable::lengthList(int i)
{
  int key;
  if (type == CHARTYPE) 
    key = hashFunc(table[i].key.fstr);
  else 
    key = hashFunc(table[i].key.fint);
  int ret = 1;
  while(key != i) {
    ret++;
    key = incKey(key,tableSize);
  }
  return ret;
}

void HashTable::print()
{
  if (type == CHARTYPE) {
    for(int i = 0; i < tableSize; i++) {
      if (! table[i].isEmpty()) {
	printf("table[%d] = <%s,0x%x>\n", i, table[i].key.fstr, table[i].value);
      }
    }
  } else {
    for(int i = 0; i < tableSize; i++) {
      if (!table[i].isEmpty()) {
	printf("table[%d] = <%d,0x%x>\n", i, table[i].key.fint, table[i].value);
      }
    }
  }
  printStatistic();
}

void HashTable::printStatistic()
{
  int maxx = 0, sum = 0, collpl = 0, coll = 0;
  for(int i = 0; i < tableSize; i++) {
    if (table[i].isEmpty())
      continue;
    int l = lengthList(i);
    maxx = maxx > l ? maxx : l;
    sum += l;
    coll  += l > 1 ? l - 1 : 0;
    collpl += l > 1 ? 1 : 0;
  }
  printf("\nHashtable-Statistics:\n");
  printf("\tmaximum bucket length     : %d\n", maxx);
  printf("\tnumber of collision places: %d\n", collpl);
  printf("\tnumber of collisions      : %d\n", coll);
  printf("\t%d table entries have been used for %d literals (%d%%)\n", 
	 tableSize, counter, counter*100/tableSize);
}

