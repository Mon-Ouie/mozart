/*
 *  Authors:
 *    Christian Schulte <schulte@ps.uni-sb.de>
 * 
 *  Contributors:
 *    Ralf Scheidhauer (Ralf.Scheidhauer@ps.uni-sb.de)
 *    Michael Mehl (mehl@dfki.de)
 * 
 *  Copyright:
 *    Michael Mehl, 1995-1999
 *    Ralf Scheidhauer, 1995-1999
 *    Christian Schulte, 2000
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

#ifndef __REFSARRAY_HH__
#define __REFSARRAY_HH__

#ifdef INTERFACE
#pragma interface
#endif

#include "tagged.hh"

class RefsArray {
private:
  int       _len;
  TaggedRef _a[1];

public:

  int getLen(void) {
    return _len>>1;
  }

  void setLen(int n) {
    _len = n<<1;
  }

  int cacIsMarked(void) {
    return _len & 1;
  }

  void cacMark(RefsArray * fwd) {
    _len = ((int) fwd) | 1;
  }
  
  int32 ** cacGetMarkField(void) {
    return (int32 **) &_len;
  }

  RefsArray * cacGetFwd(void) {
    Assert(cacIsMarked());
    return (RefsArray *) (_len&~1);
  }

  RefsArray * gCollect(void);
  RefsArray * sClone(void);

  static RefsArray * allocate(int n, Bool init=OK) {
    Assert(n > 0);
    RefsArray * ra = (RefsArray *) oz_freeListMalloc(sizeof(RefsArray) +
						     (n-1)*sizeof(TaggedRef));
    ra->setLen(n);
    if (init) {
      register TaggedRef nvr = NameVoidRegister;
      switch (n) {
      case 10: ra->_a[9] = nvr;
      case  9: ra->_a[8] = nvr;
      case  8: ra->_a[7] = nvr;
      case  7: ra->_a[6] = nvr;
      case  6: ra->_a[5] = nvr;
      case  5: ra->_a[4] = nvr;
      case  4: ra->_a[3] = nvr;
      case  3: ra->_a[2] = nvr;
      case  2: ra->_a[1] = nvr;
      case  1: ra->_a[0] = nvr;
	break;
      default:
	for (int i = n; i--; ) 
	  ra->_a[i] = nvr;
	break;
      }
    } 
    return ra;
  }

  void dispose(int n) {
    oz_freeListDispose(this, (sizeof(RefsArray) +
			      (n-1)*sizeof(TaggedRef)));
  }
  
  void dispose(void) {
    dispose(getLen());
  }
  
  TaggedRef * getArgRef(int i) {
    return &(_a[i]);
  }

  TaggedRef * getFastArgRef(int i) {
    return (TaggedRef *) (((char *) &(_a[0])) + (i - sizeof(int)));
  }

  TaggedRef getArg(int i) {
    return _a[i];
  }

  void setArg(int i, TaggedRef x) {
    _a[i] = x;
  }

  TaggedRef * getArgsRef(void) {
    return &(_a[0]);
  }

  static RefsArray * copy(TaggedRef * x, int n) {
    RefsArray * c = allocate(n,NO);
    for (int i=n; i--; )
      c->_a[i] = x[i];
    return c;
  }

  static RefsArray * copy(RefsArray * x, int n) {
    RefsArray * c = allocate(n,NO);
    for (int i=n; i--; )
      c->_a[i] = x->_a[i];
    return c;
  }

};


#endif


