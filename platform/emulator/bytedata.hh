/*
 *  Authors:
 *    Denys Duchier (duchier@ps.uni-sb.de)
 *
 *  Copyright:
 *    Denys Duchier (1998)
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

#ifndef __BYTEDATAH
#define __BYTEDATAH

#include "builtins.hh"
#include "string.h"
#include "pickle.hh"

// -------------------------------------------------------------------
// BytePointer simply encapsulates a pointer to an array of bytes
// It forms the common basis of the implementation for BitString,
// BitArray, ByteString, ByteArray
// -------------------------------------------------------------------

class BytePtr {
protected:
  BYTE *data;
public:
  BYTE *allocate(int bytes) {
    return (BYTE *) alignedMalloc(bytes,sizeof(double));
  }
  virtual int getSize() = 0;    // number of bytes in data array
  BYTE *cloneData() {
    int size = getSize();
    if (size==0) return NULL;
    BYTE * newData = allocate(size);
    memcpy((void*)newData,(const void*)data,size);
    return newData;
  }
  BYTE& operator[](int i) { return data[i]; }
  BYTE& getByte(int i) { return data[i]; }
};

// -------------------------------------------------------------------
// BitData
//      specializes BytePtr and contains the additional functionality
// shared by BitString and BitArray
// -------------------------------------------------------------------

#define BITS_PER_BYTE 8

class BitData: public BytePtr {
protected:
  int width;
public:
  int getWidth() { return width; }
  virtual int getSize() {
    return width/BITS_PER_BYTE
      +  ((width%BITS_PER_BYTE)?1:0);
  }
  BitData operator=(const BitData&);
  BitData(){};
  BitData(int w) : width(w) {
    int size = getSize();
    data = allocate(size);
    memset((void*)data,0,size);
  }
  BitData(BitData&);
  Bool checkIndex(int i) { return i>=0 && i<width; }
  Bool equal(BitData*);
  void bitPrintStream(ostream &out);
  int  get(int);
  void put(int,Bool);
  void conj(BitData*);
  void disj(BitData*);
  void nega();
  void nimpl(BitData*);
  Bool disjoint(BitData*);
  int  card();
  void zeroLeftOver();
};

// -------------------------------------------------------------------
// BitString
// -------------------------------------------------------------------

class BitString: public OZ_Extension, public BitData {
protected:
  friend Bool oz_isBitString(TaggedRef);
  friend void BitString_init();
public:
  virtual int getIdV() { return OZ_E_BITSTRING; }
  virtual OZ_Term typeV();
  virtual Bool isChunkV() { return NO; }
  virtual OZ_Return eqV(OZ_Term);
  virtual int marshalV(void*);
  virtual OZ_Term printV(int depth = 10);
  virtual OZ_Extension* gcV() { return clone(); }
  BitString operator=(const BitString&);
  BitString() : OZ_Extension() {}
  BitString(int w) : OZ_Extension(), BitData(w) {}
  BitString *clone();
};

inline Bool oz_isBitString(TaggedRef term)
{
  term = oz_deref(term);
  return oz_isExtension(term) &&
    oz_tagged2Extension(term)->getIdV()==OZ_E_BITSTRING;
}

inline BitString *tagged2BitString(TaggedRef term)
{
  Assert(oz_isBitString(term));
  return (BitString *) oz_tagged2Extension(oz_deref(term));
}

#define oz_declareBitStringIN(ARG,VAR)          \
BitString *VAR;                                 \
{                                               \
  oz_declareNonvarIN(ARG,_VAR);                 \
  if (!oz_isBitString(oz_deref(_VAR))) {        \
    oz_typeError(ARG,"BitString");              \
  } else {                                      \
    VAR = tagged2BitString(oz_deref(_VAR));     \
  }                                             \
}

// -------------------------------------------------------------------
// ByteData
//      specializes BytePtr and contains the additional functionality
// shared by ByteString and ByteArray
// -------------------------------------------------------------------

class ByteData: public BytePtr {
protected:
  int width;
public:
  int getWidth() { return width; }
  BYTE* getData() { return data; }
  virtual int getSize() { return width; }
  ByteData operator=(const ByteData&);
  ByteData(){};
  ByteData(int w) : width(w) {
    data = allocate(w);
    memset((void*)data,0,w);
  }
  ByteData(ByteData&);
  Bool checkIndex(int i) { return i>=0 && i<width; }
  Bool equal(ByteData*);
  void bytePrintStream(ostream& out);
  BYTE get(int i) { Assert(checkIndex(i)); return data[i]; }
  void put(int i,BYTE b) {
    Assert(checkIndex(i));
    data[i] = b;
  }
  void copy(ByteData*other,int offset) {
    memcpy((void*)(data+offset),(void*)other->data,other->width);
  }
  void copy(char*str,int n,int offset=0) {
    memcpy((void*)(data+offset),(void*)str,n);
  }
  void slice(ByteData*other,int from,int to) {
    memcpy((void*)data,(void*)(other->data+from),to-from);
  }
};

// -------------------------------------------------------------------
// ByteString
// -------------------------------------------------------------------

class ByteString: public OZ_Extension, public ByteData {
protected:
  friend Bool oz_isByteString(TaggedRef);
  friend void ByteString_init();
public:
  virtual int getIdV() { return OZ_E_BYTESTRING; }
  virtual OZ_Term typeV();
  virtual Bool isChunkV() { return NO; }
  virtual OZ_Return eqV(OZ_Term);
  virtual int marshalV(void*);
  virtual OZ_Term printV(int depth = 10);
  virtual OZ_Extension* gcV() { return clone(); }
  ByteString operator=(const ByteString&);
  ByteString() : OZ_Extension() {}
  ByteString(int w) : OZ_Extension(), ByteData(w) {}
  ByteString *clone();
};

inline Bool oz_isByteString(TaggedRef term)
{
  return oz_isExtension(term) &&
    oz_tagged2Extension(term)->getIdV()==OZ_E_BYTESTRING;
}

inline ByteString *tagged2ByteString(TaggedRef term)
{
  Assert(oz_isByteString(term));
  return (ByteString *) oz_tagged2Extension(oz_deref(term));
}

#define oz_declareByteStringIN(ARG,VAR)         \
ByteString *VAR;                                \
{                                               \
  oz_declareNonvarIN(ARG,_VAR);                 \
  if (!oz_isByteString(oz_deref(_VAR))) {       \
    oz_typeError(ARG,"ByteString");             \
  } else {                                      \
    VAR = tagged2ByteString(oz_deref(_VAR));    \
  }                                             \
}

#endif
