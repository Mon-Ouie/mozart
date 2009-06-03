/*
 *  Authors:
 *   Zacharias El Banna
 *   Erik Klintskog
 *
 *  Contributors:
 *    Raphael Collet (raph@info.ucl.ac.be)
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

#ifndef __DSS_CLASSES_HH
#define __DSS_CLASSES_HH

#ifdef DSS_INTERFACE
#pragma interface
#endif

#include "dss_enums.hh"
#include <stddef.h> // size_t



class PstOutContainerInterface;
class PstInContainerInterface;
class DssWriteBuffer;
class DssReadBuffer;
class CoordinatorAssistant;

#ifndef WIN32
#define DSSDLLSPEC
#else
#ifdef DSS_STATIC_LINK
#define DSSDLLSPEC
#else
#ifdef DSS_EXPORTING
#define DSSDLLSPEC __declspec(dllexport)
#else
#define DSSDLLSPEC __declspec(dllimport)
#endif
#endif
#endif

#ifdef ANDROID
#define DSS_NO_RTTI
#endif

//****************************** Not in the documantation yet *************************

// A ThreadMediator is used to provide the DSS an interface to a
// suspended operation in the programming system.  The operation can
// be resumed in one of three ways:
//  - resumeDoLocal: the entity state is available locally;
//  - resumeRemoteDone: the result of the operation is given;
//  - resumeFailed: the entity is in state permfail.
class DSSDLLSPEC ThreadMediator{
public:
  ThreadMediator();
  virtual ~ThreadMediator() {}
  virtual void resumeDoLocal()=0;
  virtual void resumeRemoteDone(PstInContainerInterface* pstin)=0;
  virtual void resumeFailed()=0;
};


// A DssThreadId represents a thread in the programming system.
class DSSDLLSPEC DssThreadId{
private:
  ThreadMediator* a_thread;

public:
  DssThreadId():a_thread(NULL){}
  virtual ~DssThreadId() {}
  void setThreadMediator(ThreadMediator* t) { a_thread = t; }
  ThreadMediator* getThreadMediator() { return a_thread; }

  virtual void dispose() = 0;

  MACRO_NO_DEFAULT_CONSTRUCTORS(DssThreadId);
};





/************************* THE ABSTRACT ENTITY *************************/

// These abstract classes provide the functionalities of a distributed
// entity (abstract operations, reference consistency, fault status)
// in the DSS, with a single API.  In order to implement a distributed
// entity, the user extends one of those classes, and implements the
// corresponding callbacks.  Abstract entities are specialized into
// four categories, namely mutable, relaxed mutable, monotonic, and
// immutable.

// The abstract classes result from the merging of former interfaces
// AbstractEntity and Mediator (by raph).
#ifdef DSS_NO_RTTI
class MutableAbstractEntity;
class RelaxedMutableAbstractEntity;
class MonotonicAbstractEntity;
class ImmutableAbstractEntity;
class VAbstractEntity;
#endif
class DSSDLLSPEC AbstractEntity {
protected:
  CoordinatorAssistant* a_proxy;

  AbstractEntity();

public:
  virtual ~AbstractEntity();

  /************************* SUPPLIED BY DSS *************************/

  virtual AbstractEntityName getAEName() const { return AEN_NOT_DEFINED; }

  // get/set coordination proxy (see note below)
  CoordinatorAssistant* getCoordinatorAssistant() const { return a_proxy; }
  void setCoordinatorAssistant(CoordinatorAssistant*);

  // abstract operation Kill - try to make the fault state permfail.
  // This operation is asynchronous, and not guaranteed to succeed.
  // Monitor requests to notify failures eagerly to this proxy.
  OpRetVal abstractOperation_Kill();
  OpRetVal abstractOperation_Monitor();

  /************************* SUPPLIED BY USER *************************/

  // both the following return the entity's current state.  The first
  // one is used to make a copy of the state, while the second one may
  // turn the entity into a skeleton.
  virtual PstOutContainerInterface* retrieveEntityRepresentation() = 0;
  virtual PstOutContainerInterface* deinstallEntityRepresentation() {
    return retrieveEntityRepresentation(); }
  // set the entity's new state
  virtual void installEntityRepresentation(PstInContainerInterface*) = 0;

  // report the entity's new fault state
  virtual void reportFaultState(const FaultState& fs) = 0;
#ifdef DSS_NO_RTTI
  virtual MutableAbstractEntity* asMutableAbstractEntity(){return 0;}
  virtual RelaxedMutableAbstractEntity* asRelaxedMutableAbstractEntity(){return 0;}
  virtual MonotonicAbstractEntity* asMonotonicAbstractEntity(){return 0;}
  virtual ImmutableAbstractEntity* asImmutableAbstractEntity(){return 0;}
  virtual VAbstractEntity* asVAbstractEntity(){return 0;}
#endif
  MACRO_NO_DEFAULT_CONSTRUCTORS(AbstractEntity);
};

#ifdef DSS_NO_RTTI
class DSSDLLSPEC VAbstractEntity: public virtual AbstractEntity{
  virtual VAbstractEntity* asVAbstractEntity(){return this;}
};
#endif

// ABOUT THE COORDINATION PROXY.  (1) Coordination proxies are created
// by a DSS_Object.  A proxy p is attached to its abstract entity ae
// by the call ae->setCoordinatorAssistant(p).
// (2) The proxy is deleted by the DSS when either ae is deleted, or
// ae->setCoordinatorAssistant(NULL) is called.  The latter localizes
// the entity.



// The following specializations define the abstract operations
// specific to each category.
//
// Note the use of virtual inheritance, to avoid multiple inheritance
// issues.  One can define an generic extension of AbstractEntity, and
// then specialize it to specific kinds of entities.  The performance
// overhead of virtual inheritance is pretty small.  A typical
// inheritance diagram is shown below, with virtual inheritance placed
// as shown.
//
//                            AbstractEntity
//                               /      \.
//                      virtual /        \ virtual
//            MutableAbstractEntity      MyEntity
//                              \        /
//                               \      /
//                           MyMutableEntity
//
// When there is no RTTI, we cannot dynamic_cast an AbstractEntity to a MyEntity
// or a MutableAbstractEntity. Therefore, we introduce virtual methods in AbstractEntity
// to 'cast' an AbstractEntity to MutableAbstractEntity, RelaxedMutableAbstractEntity, etc.
// We also introduce a class VAbstractEntity, virtually inheriting from AbstractEntity,
// to which we can 'cast' from AbstractEntity by a virtual method and from which MyEntity
// can directly derive.
//
// The diagram thus become:
//                            AbstractEntity
//                               /      \.
//                      virtual /        \ virtual
//                             /         VAbstractEntity
//                            /           |
//            MutableAbstractEntity       |
//                            \           |
//                             \         MyEntity
//                              \        /
//                               \      /
//                           MyMutableEntity


class DSSDLLSPEC MutableAbstractEntity: public virtual AbstractEntity{
public:
  MutableAbstractEntity();

  /************************* SUPPLIED BY DSS *************************/
  virtual AbstractEntityName getAEName() const { return AEN_MUTABLE; }
  OpRetVal abstractOperation_Read(DssThreadId*, PstOutContainerInterface**&);
  OpRetVal abstractOperation_Write(DssThreadId*, PstOutContainerInterface**&);
#ifdef DSS_NO_RTTI
  virtual MutableAbstractEntity* asMutableAbstractEntity(){return this;}
#endif

  /************************* SUPPLIED BY USER *************************/
  virtual void callback_Write(DssThreadId* id_of_calling_thread,
                              PstInContainerInterface* operation,
                              PstOutContainerInterface*& answer)=0;
  virtual void callback_Read(DssThreadId* id_of_calling_thread,
                             PstInContainerInterface* operation,
                             PstOutContainerInterface*& answer)=0;
};


class DSSDLLSPEC RelaxedMutableAbstractEntity: public virtual AbstractEntity{
public:
  RelaxedMutableAbstractEntity();

  /************************* SUPPLIED BY DSS *************************/
  virtual AbstractEntityName getAEName() const { return AEN_RELAXED_MUTABLE; }
  OpRetVal abstractOperation_Read(DssThreadId*, PstOutContainerInterface**&);
  OpRetVal abstractOperation_Write(PstOutContainerInterface**&);
#ifdef DSS_NO_RTTI
  virtual RelaxedMutableAbstractEntity* asRelaxedMutableAbstractEntity(){return this;}
#endif

  /************************* SUPPLIED BY USER *************************/
  virtual void callback_Write(DssThreadId* id_of_calling_thread,
                              PstInContainerInterface* operation)=0;
  virtual void callback_Read(DssThreadId* id_of_calling_thread,
                             PstInContainerInterface* operation,
                             PstOutContainerInterface*& answer)=0;
};


class DSSDLLSPEC MonotonicAbstractEntity: public virtual AbstractEntity{
public:
  MonotonicAbstractEntity();

  /************************* SUPPLIED BY DSS *************************/
  virtual AbstractEntityName getAEName() const { return AEN_TRANSIENT; }
  OpRetVal abstractOperation_Bind(DssThreadId*, PstOutContainerInterface**&);
  OpRetVal abstractOperation_Append(DssThreadId*, PstOutContainerInterface**&);
#ifdef DSS_NO_RTTI
  virtual MonotonicAbstractEntity* asMonotonicAbstractEntity(){return this;}
#endif

  /************************* SUPPLIED BY USER *************************/
  virtual void callback_Bind(PstInContainerInterface* operation) = 0;
  virtual void callback_Append(PstInContainerInterface* operation) = 0;
  // summarize past Append operations; the answer (if given) is sent
  // to a new proxy as one single Append operation.
  virtual void callback_Changes(PstOutContainerInterface*& answer)=0;
};


class DSSDLLSPEC ImmutableAbstractEntity: public virtual AbstractEntity{
public:
  ImmutableAbstractEntity();

  /************************* SUPPLIED BY DSS *************************/
  virtual AbstractEntityName getAEName() const { return AEN_IMMUTABLE; }
  OpRetVal abstractOperation_Read(DssThreadId*, PstOutContainerInterface**&);
#ifdef DSS_NO_RTTI
  virtual ImmutableAbstractEntity* asImmutableAbstractEntity(){return this;}
#endif

  /************************* SUPPLIED BY USER *************************/
  virtual void callback_Read(DssThreadId* id_of_calling_thread,
                             PstInContainerInterface* operation,
                             PstOutContainerInterface*& answer)=0;

  // Note.  The programming system may want to localize the entity
  // once its state has been installed.  However it should not delete
  // the abstract entity before all suspended read operations have
  // been woken up.  This is because the protocol proxy is deleted
  // together with the abstract entity.
};





/************************* THE COORDINATION PROXY *************************/

// This object provides an API to the coordination architecture, the
// reference consistency protocols, the fault status, and marshaling.

class DSSDLLSPEC CoordinatorAssistant {
public:
  virtual ~CoordinatorAssistant() {}

  virtual AbstractEntity* getAbstractEntity() const = 0;
  virtual void getParameters(ProtocolName&,AccessArchitecture&,RCalg&) const=0;

  // ******************** Coordination Manipulation ********************
  virtual bool  manipulateCNET(void* argument) = 0;

  // ********************** Reference Consistency **********************
  virtual bool  manipulateRC(const RCalg& alg,
                             const RCop&  op,
                             opaque&      argument)=0;

  virtual DSS_GC     getDssDGCStatus()=0;
  virtual bool       clearWeakRoot()=0;

  // ************************* Fault interface *************************
  virtual FaultState  getRegisteredFS() const = 0;
  virtual void        setRegisteredFS(const FaultState&) = 0;
  virtual FaultState  getFaultState() const= 0;

  // *********************** Marshaler interface ***********************
  // returns true if the entity's state should be marshaled as well
  // (immediate copy for immutables)
  virtual bool marshal(DssWriteBuffer*, const ProxyMarshalFlag&) = 0;
  virtual int  getMarshaledSize(const ProxyMarshalFlag&) = 0;
};





// ********************** Marshaling and unmarshaling routines **********************
//Imported by the DSS from the MAP

class DSSDLLSPEC PstInContainerInterface{
public:
  PstInContainerInterface();
  virtual ~PstInContainerInterface() {}
  virtual bool unmarshal(DssReadBuffer*) = 0;
  virtual void dispose() = 0;
  virtual PstOutContainerInterface* loopBack2Out() = 0;
};

class DSSDLLSPEC PstOutContainerInterface{
public:
  PstOutContainerInterface();
  virtual ~PstOutContainerInterface() {}
  virtual bool marshal(DssWriteBuffer*) = 0;
  virtual void resetMarshaling() = 0;
  virtual void dispose() = 0;
  virtual PstInContainerInterface* loopBack2In() = 0;
  virtual PstOutContainerInterface* duplicate() = 0;
};

// ********************************* I/O routines **********************************
// Imported


// ******************************* Connection Routines ********************************
// Exported




// NOT IN DOCUMENTATION YET!
//
// The mediation entry, i.e. the entry to the MAP.
//

class DSSDLLSPEC Mediation_Object{
public:
  Mediation_Object();
  virtual ~Mediation_Object() {}

  virtual PstInContainerInterface* createPstInContainer()=0;
  virtual void GL_error(const char* const format, ...)=0;
  virtual void GL_warning(const char* const format, ...)=0;

};

#endif
