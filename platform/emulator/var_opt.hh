/*
 *  Authors:
 *    Konstantin Popov <kost@sics.se>
 *
 *  Contributors:
 *    optional, Contributor's name (Contributor's email address)
 *
 *  Copyright:
 *    Konstantin Popov (2000)
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

#ifndef __OPTVAR__H__
#define __OPTVAR__H__

#if defined(INTERFACE)
#pragma interface
#endif

#include "var_base.hh"

//
class OptVar: public OzVariable {
public:
  OptVar(Board *bb) : OzVariable(OZ_VAR_OPT, bb) {}

  OZ_Return bind(TaggedRef* vPtr, TaggedRef t);
  OZ_Return unify(TaggedRef* vPtr, TaggedRef *tPtr);

  OZ_Return valid(TaggedRef /* val */) { return OK; }

  // disposing of opt var"s is done only when its space is gone.
  void dispose(void) {}

  void printStream(ostream &out, int depth = 10) {
    out << "<optimized>";
  }
  void printLongStream(ostream &out, int depth = 10, int offset = 0);
};

#endif /* __SIMPLEVAR__H__ */
