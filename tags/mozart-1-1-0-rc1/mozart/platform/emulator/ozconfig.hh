/*
 *  Authors:
 *    Michael Mehl (mehl@dfki.de)
 * 
 *  Contributors:
 *    optional, Contributor's name (Contributor's email address)
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

#ifndef __CONFH
#define __CONFH

#ifdef INTERFACE
#pragma interface
#endif

// this class contains the configurable parameters
class ConfigData {
public:
  int printDepth;
  int printWidth;

  int errorPrintDepth; 
  int errorPrintWidth;
  int errorThreadDepth; // int, how many tasks are printed    
  int errorDebug;       // bool, add debug information

  int showIdleMessage;	// show message on idle
  int showSuspension;   // show message when a suspension is created
  int useFutures;       // use futures for synchronization

  int stopOnToplevelFailure;  // enter machine level debugger on TOPLEVEL FAIL

  int gcFlag;                 // request GC to run
  int gcVerbosity;            // GC verbosity level
  int codeGCcycles;           // after that many GCs a code GC will be done

  int heapThreshold;
  int heapMinSize;
  int heapFree;
  int heapTolerance;

  int stackMinSize;

  int timeDetailed;

  int hiMidRatio;
  int midLowRatio;

  int debugIP;
  int debugPerdio;
  int perdioMinimal;
  
  // Info needed for flowcontrol in ports
  int perdioFlowBufferSize;
  int perdioFlowBufferTime;

  //
  // For Perdio's 'Remote' and 'Connection';
  int perdioTimeout;
  
  // global handler
  int perdioSeifHandler;

  int perdioUseAltVarProtocol;

  int perdioTempRetryCeiling;
  int perdioTempRetryFloor;
  int perdioTempRetryFactor;
  int perdioMaxTCPCache;
  int perdioCheckAliveInterval;

  int dpTableDefaultOwnerTableSize;
  int dpTableDefaultBorrowTableSize;
  int dpTableLowLimit;
  int dpTableExpandFactor;
  int dpTableBuffer;
  int dpTableWorthwhileRealloc;

  int closetime;

  int numToplevelVars;

  int dumpCore;

  int runningUnderEmacs;

  char *ozHome;
  char *osname, *cpu;

  char *emuexe;
  char *emuhome;

  /* command line arguments visible from Oz */
  char **argV;
  int argC;

  // root functor's url
  char *url;
  int gui;
  
public:
  ConfigData() {};
  void init();
};

extern ConfigData ozconf;

#endif
