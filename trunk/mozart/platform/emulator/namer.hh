/*
 *  Authors:
 *    Tobias Mueller (tmueller@ps.uni-sb.de)
 * 
 *  Contributors:
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

#ifndef __NAMER_HH__
#define __NAMER_HH__

//////////////////////////////////////////////////////////////////////////////

#define NEW_NAMER

#ifdef NEW_NAMER
#define NAME_PROPAGATORS
#endif

#ifdef NEW_NAMER_DEBUG
#define NEW_NAMER_DEBUG_PRINT(A) printf A; fflush(stdout)
#else
#define NEW_NAMER_DEBUG_PRINT(A)
#endif


#ifdef NEW_NAMER

// to enable a class to be garbage collected by `GCMeManager' is has
// to be derived from `GCMe'
class GCMe {
public:
  virtual void gc(void) = 0;
};

class GCMeManager {
private:
  static GCMeManager * _head;
  GCMeManager * _next;
  GCMe * _object;
  GCMeManager(GCMe * o, GCMeManager * n) : _object(o), _next(n) {}
public:
  static void registerObject(GCMe * o) {
    for (GCMeManager * tmp = _head; tmp; tmp = tmp->_next)
      if (tmp->_object == o) // already registered
	return; 
    
    _head = new GCMeManager(o, _head);
  }
  static void unregisterObject(GCMe * o) {
    GCMeManager * tmp = _head;
    GCMeManager ** next_before = &_head;

    while (tmp) {
      if (tmp->_object == o) // found, remove it  
	break;
      next_before = &(tmp->_next);
      tmp = tmp->_next;
    }
    
    if (! tmp) // not registered
      return;

    *next_before = tmp->_next; // skip `tmp'
    delete tmp;
  }
  static void gc(void) {
    for (GCMeManager * tmp = _head; tmp; tmp = tmp->_next)
      tmp->_object->gc();
  }
};


//=============================================================================

Bool isGcMarkedNamer(OZ_Term);
void GcIndexNamer(OZ_Term &);
void GcDataNamer(const char * &);
OZ_Term derefIndexNamer(OZ_Term);
const char * toStringNamer(const char *);

Bool isGcMarkedNamer(Propagator *);
void GcIndexNamer(Propagator * &p);
void GcDataNamer(OZ_Term &);
Propagator * derefIndexNamer(Propagator *);
const char * toStringNamer(OZ_Term);

template <class T_INDEX, class T_NAME>
class Namer : public GCMe {
private:
  T_INDEX _index;
  T_NAME _name;
  Namer<T_INDEX, T_NAME> * _next;

  static Namer<T_INDEX, T_NAME> * _head;

  Namer(T_INDEX index, T_NAME name, Namer<T_INDEX, T_NAME> * next)
    : _index(index), _name(name), _next(next) {}
public:
  Namer(void) { 
    GCMeManager::registerObject(this);
    _head = NULL; 
  }
  static Namer<T_INDEX, T_NAME> * getHead(void) { return _head; }
  static T_NAME getName(T_INDEX index) {
    for (Namer<T_INDEX, T_NAME> * tmp = _head; tmp; tmp = tmp->_next) {
      tmp->_index = derefIndexNamer(tmp->_index);
      if (tmp->_index == index) {
	return tmp->_name;
      }
    }
    return (T_NAME) NULL;
  }
  static void addName(T_INDEX index, T_NAME name) {
    for (Namer<T_INDEX, T_NAME> * tmp = _head; tmp; tmp = tmp->_next)
      if (tmp->_index == index)  // it is already contained
	return;

    _head = new Namer<T_INDEX, T_NAME>(index, name, _head);
    
    NEW_NAMER_DEBUG_PRINT(("adding %s\n", toStringNamer(name)));
  }
  static void cloneEntry(T_INDEX index_org, T_INDEX index_clone) {
    T_NAME name = getName(index_org); 
    
    if (!name) 
      return;
    
    addName(index_clone, name);
  }
  void gc(void) {
    Namer<T_INDEX, T_NAME> * tmp = _head;
    _head = NULL;

    while (tmp) {
      NEW_NAMER_DEBUG_PRINT(("what 0x%x ", (OZ_Term) tmp->_index));

      if (isGcMarkedNamer(tmp->_index)) {
	NEW_NAMER_DEBUG_PRINT(("keeping %s\n", toStringNamer(tmp->_name)));

	GcIndexNamer(tmp->_index);
	GcDataNamer(tmp->_name);
	Namer<T_INDEX, T_NAME> * tmp_add = tmp;
	tmp = tmp->_next;
	tmp_add->_next = _head;
	_head = tmp_add;
      } else {
	NEW_NAMER_DEBUG_PRINT(("deleting %s\n", toStringNamer(tmp->_name)));

	Namer<T_INDEX, T_NAME> * tmp_delete = tmp;
	tmp = tmp->_next;
	delete tmp_delete;
      }
      
    }
  }
};
  
#endif /* NEW_NAMER */

/* ------------------------------------------------------------------------
 * maintain the mapping of variables to print names
 * ------------------------------------------------------------------------ */

const char *oz_varGetName(TaggedRef v);
void oz_varAddName(TaggedRef v, const char *nm);
void oz_varCleanup();


#ifdef NEW_NAMER
OZ_Term oz_propGetName(Propagator *);
void oz_propAddName(Propagator *, OZ_Term);
#endif

//////////////////////////////////////////////////////////////////////////////
#endif /* __NAMER_HH__ */
