/*
 *  Main authors:
 *     Raphael Collet <raph@info.ucl.ac.be>
 *     Gustavo Gutierrez <ggutierrez@cic.puj.edu.co>
 *     Alberto Delgado <adelgado@cic.puj.edu.co>
 *
 *  Contributing authors:
 *     Andres Felipe Barco <anfelbar@univalle.edu.co>
 *     Gustavo A. Gomez Farhat <gafarhat@univalle.edu.co>
 *
 *  Copyright:
 *     Gustavo Gutierrez, 2006
 *     Alberto Delgado, 2006
 *
 *  Last modified:
 *     $Date$
 *     $Revision$
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

#include "GeSetVar.hh"
#include "unify.hh"

using namespace Gecode;
using namespace Gecode::Set;


//A setvalue is a valid one when  Glb of the variable is a subset of Glb of the value and 
//Lub of the value is a subset of the of the Lub of the variable.
Bool GeSetVar::validV(OZ_Term val) {
  if(SetValueM::OZ_isSetValueM(val)){
    IntSetRanges tmpGl(SetValueM::tagged2SetVal(val)->getLBValue());
    IntSetRanges tmpLu(SetValueM::tagged2SetVal(val)->getUBValue());

    SetView ViewVar = getSetView();

    Gecode::Set::GlbRanges<Gecode::Set::SetView> x0lb(ViewVar);
    Gecode::Set::LubRanges<Gecode::Set::SetView> x0ub(ViewVar);

    if(Iter::Ranges::subset(x0lb,tmpGl)&&Iter::Ranges::subset(tmpLu,x0ub)) return true;   
    else return false;  
    return true;
  }
  return false;
}

OZ_Term GeSetVar::statusV() {
  return OZ_mkTupleC("kinded", 1, OZ_atom("set"));
}


VarImpBase* GeSetVar::clone(void) {
  GenericSpace* gs = getGSpace(); //extVar2Var(this)->getBoardInternal()->getGenericSpace(true);
  Assert(gs);
  SetView sv = getSetView();
  SetVar *v = new SetVar(sv);
  SetVar x;
  x.update(gs,false,*v);
  delete v;
  return x.var();
}


//(this) is the global variable
//x is the local variable,  the one that its domain is modified
//this is quite similar to bind method
inline
bool GeSetVar::intersect(TaggedRef x) {
  SetView sview1 = getSetView();
  SetView sview2 = get_SetView(x);
  
  LubRanges<SetView> x0ub(sview1);
  GlbRanges<SetView> x0lb(sview1);  
 
  IntSetRanges tmpUB();
  if(sview2.intersectI(oz_currentBoard()->getGenericSpace(),x0ub)!=Gecode::ME_GEN_FAILED)
    return (sview2.includeI(oz_currentBoard()->getGenericSpace(),x0lb)==Gecode::ME_GEN_FAILED ? false: true);
  else
    return false;
}

//(this) is the global variable
//lx is the local value
// If lx's Lub is a subset of the variable then In returns true otherwise it returns false
inline
bool GeSetVar::In(TaggedRef lx) {
  Set::LubRanges<Set::SetView> lxLub(get_SetView(lx));
  Set::LubRanges<Set::SetView> vLub(getSetView());
  return (Iter::Ranges::subset(lxLub,vLub));
}

TaggedRef GeSetVar::clone(TaggedRef v) {
  Assert(OZ_isGeSetVar(v));
  
  OZ_Term lv = new_GeSetVar(IntSet(lim_inf, lim_sup), IntSet(lim_inf, lim_sup));
  get_GeSetVar(v,false)->intersect(lv);
  return lv;
}

inline
bool GeSetVar::hasSameDomain(TaggedRef v) {
  SetView vwg = getSetView();
  SetVar vwl = get_SetView(v);

  LubRanges<SetView> xLub(vwl);
  LubRanges<SetView> xGub(vwg);

  GlbRanges<SetView> xLlb(vwl);
  GlbRanges<SetView> xGlb(vwg);

  return (Iter::Ranges::equal(xLub,xGub)&&Iter::Ranges::equal(xLlb,xGlb));

}

inline
TaggedRef GeSetVar::newVar(void) {
  return new_GeSetVar(IntSet(lim_inf, lim_sup),
		      IntSet(lim_inf, lim_sup));
}



//We have to be very carefull here,  ther order in the parameters of the function DOES matter!!!!
inline
bool GeSetVar::IsEmptyInter(TaggedRef* var1,  TaggedRef* var2) {

  LubRanges<SetView> x1ub(get_SetView(*var1));
  LubRanges<SetView> x2ub(get_SetView(*var2));

  //GlbRanges<SetView> x1lb(v1);
  //GlbRanges<SetView> x2lb(v2);

  return !Iter::Ranges::subset(x2ub,x1ub);
}

void GeSetVar::toStream(ostream &out) {
  std::stringstream oss;
  oss << getSetView();
  out << "<GeSetVar " << oss.str().c_str() << ">"; 
}
  

void gesvp_init(void){
  
}

#ifndef MODULES_LINK_STATIC
#include "../modGFSB-if.cc"
#endif
