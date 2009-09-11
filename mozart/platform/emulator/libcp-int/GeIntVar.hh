/*
 *  Main authors:
 *     Raphael Collet <raph@info.ucl.ac.be>
 *     Gustavo Gutierrez <ggutierrez@cic.puj.edu.co>
 *     Alberto Delgado <adelgado@cic.puj.edu.co> 
 *     Alejandro Arbelaez <aarbelaez@cic.puj.edu.co>
 *
 *  Contributing authors:
 *     Andres Felipe Barco <anfelbar@univalle.edu.co>
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

#ifndef __GECODE_INT_VAR_HH__
#define __GECODE_INT_VAR_HH__

#include "GeVar.hh"
#include "../libcp-set/GeSetVar.hh"

using namespace Gecode;
using namespace Gecode::Int;

// A GeIntVar interfaces an IntVar inside a GenericSpace.
class GeIntVar : public GeVar {
protected:
  /// copy constructor
  GeIntVar(GeIntVar& gv) :
    GeVar(gv) {}

public:
  GeIntVar(int index) :
    GeVar(index,T_GeIntVar) {}
  
  /**
   * \brief Return a IntView from the corresponding IntVarImpl
   * associated with this GeIntVar.
   */
  IntView getIntView(void){
    return IntView(static_cast<IntVarImp*>(getGSpace()->getVar(index)));
  }
  
  /**
     \brief Put in out a text representation of the variable.
  */
  void toStream(ostream &out);

  GeVarType type() { return getType(); }

  virtual ExtVar* gCollectV() { return new GeIntVar(*this); }
  virtual ExtVar* sCloneV() { return new GeIntVar(*this); }

  /** 
   * \brief Test whether \a v contains a valid element for the domain
   * of variable. \a v must be a smallInt between Gecode::Limits::Int::int_min
   * and Gecode::Limits::Int::int_max. This is a compatibility problem between
   * Oz and Gecode.
   * 
   * @param v An OZ_Term containing a possible value of the variable domain
   */ 
  virtual OZ_Term       statusV();
  //  virtual void printStreamV(ostream &out,int depth);
  virtual VarImpBase* clone(void);
  virtual bool intersect(TaggedRef x);
  
  virtual bool In(TaggedRef x);

  //clone para crear variable local desde los propagadores.
  virtual TaggedRef clone(TaggedRef v);

  virtual bool hasSameDomain(TaggedRef v);

  virtual bool IsEmptyInter(TaggedRef* var1, TaggedRef* var2);

  virtual TaggedRef newVar(void);

  virtual void propagator(GenericSpace *s, GeVar *lgevar, GeVar *rgevar) {
    IntView lintvar = (static_cast<GeIntVar*>(lgevar))->getIntView();
    IntView rintvar = (static_cast<GeIntVar*>(rgevar))->getIntView();    
    
    rel(s,lintvar,IRT_EQ,rintvar);
  }

  virtual ModEvent bind(GenericSpace *s, GeVar *v, OZ_Term val) {
    int n = OZ_intToC(val);
    return getIntView().eq(s,n);
  }

  virtual Bool validV(OZ_Term v);
    
  // reflection mechanism 
  virtual bool assigned(void) {
    return IntView(static_cast<IntVarImp*>(getGSpace()->getVar(index))).assigned();
  }
  
  virtual OZ_Term getVal(void) {
    return OZ_int(IntView(static_cast<IntVarImp*>(getGSpace()->getVar(index))).val());
  }
  
  virtual void ensureDomReflection(void) {
    postDomReflector<IntView, IntVarImp, PC_INT_DOM>(getGSpace(),this);
  }
  
  virtual int degree(void) { 
    return IntView(static_cast<IntVarImp*>(getGSpace()->getVar(index))).degree(); 
  }

};

/// Register the reflection propagators for IntVars.
namespace {
  using namespace Gecode::Int;
  
  // ValReflector propagator
  Reflection::ActorRegistrar<ValReflector<IntView> > IntValRefl;

  // DomReflector
  Reflection::ActorRegistrar<DomReflector<IntView,PC_INT_DOM> > IntDomRefl;
}

inline OZ_Term new_GeIntVar(const IntSet& dom) {
  GenericSpace* sp = oz_currentBoard()->getGenericSpace();
  IntVar *x = new IntVar(sp,dom);
  GeIntVar *nv = new GeIntVar(sp->getVarsSize());
  OzVariable* ov   = extVar2Var(nv);
  OZ_Term ref      = makeTaggedRef(newTaggedVar(ov));
  int index        = sp->newVar(static_cast<VarImpBase*>(x->var()), ref);

  if (oz_onToplevel())
    oz_currentBoard()->getGenericSpace()->makeUnstable();

  //nv->ensureValReflection();
  postValReflector<IntView,IntVarImp>(sp,index);
  delete x;
  return ref;
}

inline OZ_Term new_GeIntVarCompl(const IntSet& dom) {
  GenericSpace *sp = oz_currentBoard()->getGenericSpace();
  IntVar *x = new IntVar(sp,dom);
  ViewRanges<IntView> xvr(*x);
  IntVar xcompl(sp, Gecode::Int::Limits::min, Gecode::Int::Limits::max);
  IntView xcv(xcompl);
  xcv.minus_r(sp, xvr);
  GeIntVar *nv     = new GeIntVar(sp->getVarsSize());
  OzVariable *ov   = extVar2Var(nv);
  OZ_Term ref      = makeTaggedRef(newTaggedVar(ov));
  int index        = sp->newVar(static_cast<VarImpBase*>(xcompl.var()), ref);

  if (oz_onToplevel())
    oz_currentBoard()->getGenericSpace()->makeUnstable();

  //nv->ensureValReflection();
  postValReflector<IntView,IntVarImp>(sp,index);
  delete x;
  return ref;
}

/**
   \brief Checks if the OZ_Term v represents a integer constraint
   variable in the store.
*/
inline 
bool OZ_isGeIntVar(OZ_Term v) { 
  if (OZ_isInt(v)) {
    int i = OZ_intToC(v);
    return i <= Gecode::Int::Limits::max && i >= Gecode::Int::Limits::min;
  }
  OZ_Term v_local = OZ_deref(v);
  if (oz_isGeVar(v_local)) {
    GeVar *gv = static_cast< GeVar*>(oz_getExtVar(v_local));
    return gv->getType() == T_GeIntVar;
  }
  return false;
}

/**
   \brief Retrieves a GeIntVar from an OZ_Term. cgv parameter indicates
   if checking for globality needs to be performed.
*/
inline
GeIntVar* get_GeIntVar(OZ_Term v, bool cgv = true) {
  Assert(OZ_isGeIntVar(v));
  return static_cast<GeIntVar*>(get_GeVar<IntVarImp,PC_INT_DOM>(v,cgv));
}

/**
 * \briefd Retrieves a IntView from an OZ_Term.
 * @param v must be a finite domab variable (GeIntVar)
 * Space stability is not afected with this function
 */
inline
IntView get_IntView(OZ_Term v){
  return get_GeIntVar(v)->getIntView();
}

/**
   \brief Retrieve gecode variable pointer from an OZ_Term without afecting 
   space stability. A call to this method will not make the gecode
   space unstable.
*/

/*** Fuction for type checking of posting functions ***/
/**
   \brief Checks if the term is a intconlevel
*/
inline
bool OZ_isIntConLevel(OZ_Term t) {
  if(OZ_isInt(t)){
    int icl = OZ_intToC(t);
    return icl == ICL_VAL 
      || icl == ICL_BND 
      || icl == ICL_DOM 
      || icl == ICL_DEF ? true : false;
  }
  return false; 
}


/**
   \brief Checks if the term is a IntRelType.
*/
inline
bool OZ_isIntRelType(OZ_Term t){
  if(OZ_isInt(t)){
    int v = OZ_intToC(t);
    return v == IRT_EQ 
      || v == IRT_NQ 
      || v == IRT_LQ 
      || v == IRT_LE 
      || v == IRT_GQ 
      || v == IRT_GR ? true : false;
  }
  return false; 
}

/**
   \brief Iterates over a mozart vector (list, tuple or record) \a t and
   test if every element satisfies \a checkOne.
*/
inline
bool checkAll(OZ_Term t, bool (*checkOne)(OZ_Term)) {
  int sz = 0;
  if(OZ_isCons(t)) {
    sz = OZ_length(t);
    for(int i=0; OZ_isCons(t); t=OZ_tail(t))
      if (!checkOne(OZ_head(t)))
      	return false;
    return true;
  }
  else if(OZ_isTuple(t)) {
    sz=OZ_width(t);					
    for(int i=0; i < sz; i++) {
      OZ_Term _tmp = OZ_getArg(t,i);
      if (!checkOne(_tmp))
      	return false;
    }
    return true;
  }
  else if(OZ_isRecord(t)){
    OZ_Term al = OZ_arityList(t);
    sz = OZ_width(t);
    IntArgs _array(sz);
    for(int i = 0; OZ_isCons(al); al=OZ_tail(al))
      if(!checkOne(OZ_subtree(t,OZ_head(al))))
      	return false;
    return true;
    
  }
  return false;
}


inline
bool checkSome(OZ_Term t, bool (*checkOne)(OZ_Term)) {
  int sz = 0;
  bool ret = false;
  if(OZ_isCons(t)) {
    sz = OZ_length(t);
    for(int i=0; OZ_isCons(t); t=OZ_tail(t))
      if (checkOne(OZ_head(t))) 
      	return true;
    return false;
  }
  else if(OZ_isTuple(t)) {
    sz=OZ_width(t);					
    for(int i=0; i < sz; i++) {
      OZ_Term _tmp = OZ_getArg(t,i);
      if (checkOne(_tmp))
      	return true;
    }
    return false;
  }
  else if(OZ_isRecord(t)){
    OZ_Term al = OZ_arityList(t);
    sz = OZ_width(t);
    IntArgs _array(sz);
    for(int i = 0; OZ_isCons(al); al=OZ_tail(al))
      if(checkOne(OZ_subtree(t,OZ_head(al))))
      	return true;
    return false;
    
  }
  return false;
}

/**
   \brief Checks if the term is a IntVarArgs.
*/
inline
bool OZ_isIntVarArgs(OZ_Term t) {
  return checkSome(t, &OZ_isGeIntVar); 
}

/**
   \brief Checks if the term is a SetVarArgs.
*/
inline
bool OZ_isSetVarArgs(OZ_Term t) {
  return checkAll(t,&OZ_isGeSetVar);
}


// this function is needed to cast int to bool.
inline
bool IsInt(OZ_Term t) { return OZ_isInt(t);}

/**
   \brief Checks if the term is a IntArgs.
*/
inline
bool OZ_isIntArgs(OZ_Term t) {
  // TODO: Why the old definition requiered to test for a literal??
  return checkAll(t, &IsInt); 
}

/**
   \brief Checks if the term is a TupleSet.
*/
inline
bool OZ_isTupleSet(OZ_Term v){
  if(OZ_isCons(v)){
    for (int i = 0; OZ_isCons(v); v=OZ_tail(v), i++) {
      OZ_Term _val = OZ_head(v);
      if (!OZ_isIntArgs(_val)) {
	return false;
      }				
    }
    return true;
  }
  return false;
}

/**
   \brief Checks if the term is a TransitionS.
*/
inline
bool OZ_isTransitionS(OZ_Term tr){
  if(OZ_isTuple(tr)){
    return OZ_isInt(OZ_getArg(tr,0)) && OZ_isInt(OZ_getArg(tr,1)) && OZ_isInt(OZ_getArg(tr,2)) ? true : false;
  }
  return false;
}

/**
   \brief Checks if the term is a DFA.
*/
inline
bool OZ_isDFA(OZ_Term _t){
  if(OZ_isRecord(_t)){
    OZ_Term _inputl = OZ_arityList(_t);
    OZ_Term _inputs = OZ_subtree(_t, OZ_head(_inputl));
    int _istate     = OZ_intToC(_inputs);
    OZ_Term _tl    = OZ_subtree(_t, OZ_head(OZ_tail(_inputl)));
    for(int i=0; OZ_isCons(_tl); _tl=OZ_tail(_tl)) {
      if(!OZ_isTransitionS(OZ_head(_tl))) return false;
    }
    return true;
  }
  return false;
}

/**
   \brief Checks if a term \a t is a BoolVar or a compatible
   (int 0 or 1).
*/
inline
bool isBoolVarArg(OZ_Term t) {
  if (OZ_isGeBoolVar(t))
    return true;
  else
    if (OZ_isInt(t)) {
      int b = OZ_intToC(t);
      return (b==1 || b==0) ? true : false;
    } else
      return false;
}

inline
bool OZ_isBoolVarArgs(OZ_Term t) {
  //return checkSome(t, &isBoolVarArg);
  //we don't want to have an array with boolvars and intvars or viceversa.
  return checkAll(t, &isBoolVarArg);
}

/**
   \brief Checks if the term is a IntSet.
*/
inline
bool OZ_isIntSet(OZ_Term t){
  if(OZ_isCons(t)){
    for (int i = 0; OZ_isCons(t); t=OZ_tail(t), i++) {
      OZ_Term val = OZ_head(t);
      if(!OZ_isInt(val)) {
	if(!OZ_isTuple(val)) return false;
	else if(!OZ_isInt(OZ_getArg(val,0)) || !OZ_isInt(OZ_getArg(val,0))) return false;
      }
    }
    return true;
  }else if(OZ_isTuple(t)){
    return OZ_isInt(OZ_getArg(t,0)) && OZ_isInt(OZ_getArg(t,0)) ? true : false;
  }
  return false;
}

/**
   \brief Checks if the term is a IntSetArgs.
*/
inline
bool OZ_isIntSetArgs(OZ_Term t){
  if(OZ_isCons(t)){
    for (int i = 0; OZ_isCons(t); t=OZ_tail(t), i++) {
      OZ_Term val = OZ_head(t);
      if(!OZ_isIntSet(val)) return false;
    }	
    return true;
  }
}

/**
   Needed by GeMozProp-bulitins.cc
*/
inline
OZ_Term * vectorToOzTerms2(OZ_Term t, int &sz)
{
  OZ_Term * v;

  if (OZ_isLiteral(t)) {

    sz = 0; 
    v = NULL;

  } else if (OZ_isCons(t)) {

    sz = OZ_length(t);
    v = OZ_hallocOzTerms(sz);
    for (int i = 0; OZ_isCons(t); t = OZ_tail(t)) 
      v[i++] = OZ_head(t);

  } else if (OZ_isTuple(t)) {

    sz = OZ_width(t);
    v = OZ_hallocOzTerms(sz);
    for (int i = 0; i < sz; i += 1) 
      v[i] = OZ_getArg(t, i);

  } else {

    //OZ_ASSERT(OZ_isRecord(t));

    OZ_Term al = OZ_arityList(t);
    sz = OZ_width(t);

    v = OZ_hallocOzTerms(sz);
    for (int i = 0; OZ_isCons(al); al = OZ_tail(al)) 
      v[i++] = OZ_subtree(t, OZ_head(al));

  } 

  return v;
}

void module_init_geintvar(void);

// Init the the module containing the propagators
//void module_init_geintVarProp(void);
void geivp_init(void);
#endif
