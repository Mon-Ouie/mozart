/*
 *  Authors:
 *    Tobias Mueller <tmueller@ps.uni-sb.de>
 *    Ralf Scheidhauer <Ralf.Scheidhauer@ps.uni-sb.de>
 * 
 *  Contributors:
 *    Konstantin Popov <kost@sics.se>
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

#ifndef __HASHTABLEH
#define __HASHTABLEH

#ifdef INTERFACE
#pragma interface
#endif

#include "base.hh"

#define htEmpty ((void*) -1L)

class SHT_HashNode;

//
// The one with the paramount efficiency (atom- and name- hash
// tables). That is, we don't really care how expensive is the
// "insert" operation, both in terms of speed and memory.

//
// The first key/value pair is stored in the table, and subsequent
// ones outside it;
class SHT_HashNode {
private:
  const char *key;
  void* value;
  SHT_HashNode *next;

public:
  void setEmpty() { key = (const char*) htEmpty; }
  Bool isEmpty()  { return (key == (const char*) htEmpty); }

  //
  void setKey(const char *sIn) { key = sIn; }
  void setValue(void *valueIn) { value = valueIn; }
  void setNext(SHT_HashNode *nextIn) { next = nextIn; }

  //
  SHT_HashNode() { setEmpty(); }
  SHT_HashNode(const char *s, void *valueIn, SHT_HashNode *nextIn)
    : value(valueIn), next(nextIn)
  {
    setKey(s);
    Assert(!isEmpty());
  }

  //
  const char* getKey() { return (key); }
  void* getValue() { return (value); }
  SHT_HashNode *getNext() { return (next); }
};

//
class StringHashTable {
protected:
  int tableSize;		// 
  SHT_HashNode* table;		// 
  int counter;      // number of entries
  int percent;      // if more than percent is used, we reallocate

private:
  int lengthList(int i);

protected:
  unsigned int hashFunc(const char *);
  void resize();

public:
  StringHashTable(int sz);
  ~StringHashTable();

  //
  int getSize() { return counter; }
  void mkEmpty();

  //
  void htAdd(const char *k, void *val);
  void *htFind(const char *);

  //
  void print();
  void printStatistic();
  unsigned memRequired(int valSize = 0);
  int getTblSize() { return (tableSize); }

  //
protected:
  SHT_HashNode *getFirst();
  SHT_HashNode *getNext(SHT_HashNode *hn);
};


//
// Compact one (does not allocate additional memory outside the table:
// "open addressing"). "delete" is not supported. Find/insert are
// supposed to be still reasonably fast..

//
class AHT_HashNode {
private:
  intlong key;
  void* value;

public:
  void setEmpty() { key = (intlong) htEmpty; }
  Bool isEmpty()  { return (key == (intlong) htEmpty); }

  //
  AHT_HashNode() { setEmpty(); }

  //
  void setKey(intlong iIn) { key = iIn; }
  void setValue(void *vIn) { value = vIn; }

  //
  intlong getKey() { return (key); }
  void* getValue() { return (value); }
};

//
class AddressHashTable {
protected:
  int tableSize;		// 
  int incStepMod;		// an integer slightly < tableSize;
  AHT_HashNode* table;		// 
  int counter;      // number of entries
  int percent;      // if more than percent is used, we reallocate
  DebugCode(int nsearch;);	// number of searches;
  DebugCode(int tries;);	// accumulated;
  DebugCode(int maxtries;);

protected:
  unsigned int primeHashFunc(intlong);
  unsigned int incHashFunc(intlong);
  unsigned int getStepN(unsigned int pkey, unsigned int ikey, int i);

  unsigned int findIndex(intlong);
  void resize();

public:
  AddressHashTable(int sz);
  ~AddressHashTable();

  //
  int getSize() { return counter; }
  void mkEmpty();

  //
  void htAdd(intlong k, void *val);
  void *htFind(intlong);

  //
  DebugCode(void print(););
  DebugCode(void printStatistic(););
  unsigned memRequired(int valSize = 0);
  int getTblSize() { return (tableSize); }

  //
protected:
  AHT_HashNode *getNext(AHT_HashNode *hn) {
    for (hn++; hn < table+tableSize; hn++) {
      if (!hn->isEmpty())
	return (hn);
    }
    return ((AHT_HashNode *) 0);
  }
  AHT_HashNode *getFirst() { return (getNext(table-1)); }
};


//
class AHT_HashNodeCnt {
private:
  intlong key;
  void* value;
  unsigned int cnt;

  //
public:
  AHT_HashNodeCnt() { cnt = 0; }

  unsigned int getCnt() { return (cnt); }
  void setCnt(unsigned int cntIn) { cnt = cntIn; }

  //
  void setKey(intlong iIn) { key = iIn; }
  void setValue(void *vIn) { value = vIn; }

  //
  intlong getKey() { return (key); }
  void* getValue() { return (value); }
};

//
// Only 'intlong' keys are supported by now;
class AddressHashTableFastReset {
private:
  int tableSize;
  int bits;			// pkey & skey;
  int rsBits;			// right shifht;
  int slsBits;			// skey left shift;
  int counter;      // number of entries
  int percent;      // if more than percent is used, we reallocate
  unsigned int pass;		// current pass
  // (inits to 1 since AHT_HashNodeCnt inits it to 0);
  AHT_HashNodeCnt *table;
  int lastKey;
  DebugCode(intlong lastK;);
  DebugCode(int nsearch;);	// number of searches since last 'mkEmpty()';
  DebugCode(int tries;);	// accumulated over 'nsearch';
  DebugCode(int maxtries;);
  DebugCode(int nsearchAcc;);	// .. since object construction;
  DebugCode(int triesAcc;);	// accumulated over 'nsearchAcc';

  //
private:
  unsigned int primeHashFunc(intlong);
  unsigned int incHashFunc(intlong);

  unsigned int findIndex(intlong i);
  void mkTable();
  void resize();

  //
public:
  AddressHashTableFastReset(int sz);
  ~AddressHashTableFastReset();

  //
  int getSize() { return (counter); }
  void htAdd(intlong k, void *val);
  void htAddLastNotFound(intlong k, void *val);
  void *htFind(intlong k);
  void mkEmpty();

  //
  // for e.g. garbage collection:
  AHT_HashNodeCnt *getNext(AHT_HashNodeCnt *hn) {
    for (hn--; hn >= table; hn--) {
      if (hn->getCnt() == pass)
	return (hn);
    }
    return ((AHT_HashNodeCnt *) 0);
  }
  AHT_HashNodeCnt *getFirst() { return (getNext(table+tableSize)); }

  //
  DebugCode(void print(););
  DebugCode(void printStatistics(int th = 0););
};

#endif


