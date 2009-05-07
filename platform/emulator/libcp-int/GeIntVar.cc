/*
 *  Main authors:
 *     Raphael Collet <raph@info.ucl.ac.be>
 *     Gustavo Gutierrez <ggutierrez@cic.puj.edu.co>
 *     Alberto Delgado <adelgado@cic.puj.edu.co> 
 *     Alejandro Arbelaez <aarbelaez@cic.puj.edu.co>
 *
 *  Contributing authors:
 *			Andres Felipe Barco <anfelbar@univalle.edu.co>
 *  Copyright:
 *    Alberto Delgado, 2006-2007
 *    Alejandro Arbelaez, 2006-2007
 *    Gustavo Gutierrez, 2006-2007
 *    Raphael Collet, 2006-2007
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


#include "GeIntVar.hh"
#include "distribute.hh"
#include "unify.hh"


using namespace Gecode;
using namespace Gecode::Int;

Bool GeIntVar::validV(OZ_Term val) {
  // BigInts cannot be represented by normal c int type. 
  // SmallInts are just c ints so we only allow that kind of integer values
  // References:
  //   FD.sup = 134 217 726
  //   FD.inf = 0
  // In Gecode:
  //   Gecode::Limits::Int::max
  //   Gecode::Limits::Int::min
  if (OZ_isSmallInt(val)) {
    int n = OZ_intToC(val);
    if(n >= Int::Limits::min &&
       n <= Int::Limits::max)
      {
	IntVar *iv = getIntVarInfoPtr();
	IntView w((*iv));
	delete iv;
	return w.in(n);
      }
    else {
      GEOZ_DEBUG_PRINT(("Invalid integer.\n All domain ranges must be between %d and %d",Int::Limits::min,Int::Limits::max));
      return false;
    }
  }
  return false;
}

OZ_Term GeIntVar::statusV() {
  return OZ_mkTupleC("kinded", 1, OZ_atom("int"));
}


VarImpBase* GeIntVar::clone(void) {
  GenericSpace* gs = getGSpace(); //extVar2Var(this)->getBoardInternal()->getGenericSpace(true);
  Assert(gs);
  IntVar *v = getIntVarInfoPtr();
  IntVar x;
  x.update(gs,false,(*v));
  delete v;
  return x.var();
}


//(this) is the global variable
//x is the local variable,  the one that its domain is modified
inline
bool GeIntVar::intersect(TaggedRef x) {
  IntVar *gv = getIntVarInfoPtr();
  ViewRanges<IntView> gvr(*gv);

  IntVar *liv = get_IntVarInfoPtr(x);
  IntView vw(*liv);
  
  //printf("GeIntVar.cc min1=%d - max1=%d -- min2=%d - max2=%d\n",vw.min(), vw.max(),tmp2.min(),tmp2.max());fflush(stdout);  
  //TODO: Ask Alejandro about the use of getGSpace() instead of oz_currentBoard()
  delete gv;
  delete liv;
  return (vw.inter_r(oz_currentBoard()->getGenericSpace(),gvr)==ME_GEN_FAILED ? false: true);
}

//(this) is the global variable
//lx is the local value
inline
bool GeIntVar::In(TaggedRef lx) {
  IntVar *gv = getIntVarInfoPtr();
  IntView vw(*gv);
  delete gv;
  return vw.in(oz_intToC(lx));
}

TaggedRef GeIntVar::clone(TaggedRef v) {
  Assert(OZ_isGeIntVar(v));
  OZ_Term lv = new_GeIntVar(IntSet(Int::Limits::min,Int::Limits::max));
  get_GeIntVar(v,false)->intersect(lv);
  return lv;
}

inline
bool GeIntVar::hasSameDomain(TaggedRef v) {
  Assert(OZ_isGeIntVar(v));
  IntVar *v1 = get_IntVarInfoPtr(v);
  IntVar *v2 = getIntVarInfoPtr();
  ViewRanges< IntView > vr1 (*v1);
  ViewRanges< IntView > vr2 (*v2);
  delete v1;
  delete v2;
  
  while(true) {
    if(!vr1() && !vr2()) return true;
    if(!vr1() || !vr2()) return false;
    if( (vr1.min() != vr2.min()) || (vr1.max() != vr2.max() ) ) return false;
    ++vr1; ++vr2;
  }
}

inline
TaggedRef GeIntVar::newVar(void) {
  return new_GeIntVar(IntSet(Int::Limits::min,
			     Int::Limits::max));
}


inline
bool GeIntVar::IsEmptyInter(TaggedRef* var1,  TaggedRef* var2) {
  IntVar *v1 = get_IntVarInfoPtr(*var1);
  IntVar *v2 = get_IntVarInfoPtr(*var2);
  ViewRanges<IntView > vr1 (*v1);
  ViewRanges<IntView > vr2 (*v2);
  delete v1;
  delete v2;
  
  while(true) {
    if(!vr1() || !vr2() ) return true;
    if( vr2.min() <= vr2.max() && vr2.max() <= vr1.max() && vr1.min() <= vr2.max() )
      return false;
    if( vr1.min() <= vr2.max() && vr1.max() <= vr2.max() && vr2.min() <= vr1.max() )
      return false;
    
    if( vr2.max() <= vr1.max() ) ++vr2;
    ++vr1;
  }
}

void GeIntVar::toStream(ostream &out) {
  IntVar *iv =  getIntVarInfoPtr();
  std::stringstream oss;
  oss << (*iv);
  out << "<GeIntVar " << oss.str().c_str() << ">"; 
  delete iv;
}

// Init the the module containing the propagators
//void module_init_geintVarProp(void);
void geivp_init(void){
  //gfd_dist_init();
}
  


#ifndef MODULES_LINK_STATIC
#include "../modGFDB-if.cc"
#endif