/*
 *  Authors:
 *    Konstantin Popov
 *
 *  Contributors:
 *
 *  Copyright:
 *    Konstantin Popov 1997-1998
 *
 *  Last change:
 *    $Date$ by $Author$
 *    $Revision$
 *
 *  This file is part of Mozart, an implementation
 *  of Oz 3:
 *     $MOZARTURL$
 *
 *  See the file "LICENSE" or
 *     $LICENSEURL$
 *  for information on usage and redistribution
 *  of this file, and for a DISCLAIMER OF ALL
 *  WARRANTIES.
 *
 */

#ifndef __VS_LOCKHH
#define __VS_LOCKHH

#include "base.hh"

#ifdef VIRTUALSITES

#ifdef INTERFACE
#pragma interface
#endif

//
// This code is cloned from the OzPar's "asm_core.hh".
//

#if defined(volatile)
#define __volatile_f
#undef  volatile
#else
#undef  __volatile_f
#endif

/*
 * Basic types;
 */
typedef unsigned int Value;

//
// This works properly on Sun"s only by now;
#if  defined(SUNOS_SPARC) || defined(SOLARIS_SPARC)

//
// Basic swap;
#ifdef __GNUC__

#define ASM_SWAP(cell, value)                                   \
({                                                              \
  Value out;                                                    \
  asm volatile ("swap %3,%0"                                    \
                : "=r" (out),   "=m" (cell)     /* output */    \
                : "0"  (value), "m"  (cell));   /* input  */    \
  out;                                                          \
})

#else

#define ASM_SWAP(cell, value)                                   \
({                                                              \
  Value out = (Value) cell;                                     \
  cell = (Value) value;                                         \
  out;                                                          \
})

#endif

#else  // !(defined(SUNOS_SPARC) || defined(SOLARIS_SPARC))

#define ASM_SWAP(cell, value)                                   \
({                                                              \
  Value out = (Value) cell;                                     \
  cell = (Value) value;                                         \
  out;                                                          \
})

#endif

//
#define PAR_UNLOCKED    0x0
#define PAR_LOCKED      0xffffffff

//
class LockObj {
private:
  volatile Value lc;

  //
  // Note that these really should be protected methods, since
  // they are to be used by methods of its superclasses only;
protected:
  LockObj() : lc(PAR_UNLOCKED) {}

  //
  Bool isUnlocked()     { return (lc == PAR_UNLOCKED); }
  Bool isLocked()       { return (lc == PAR_LOCKED);   }

  //
  void lock() {
    do {
      if (ASM_SWAP(lc, PAR_LOCKED) == PAR_UNLOCKED) break;
      while (lc == PAR_LOCKED) continue; // spin in local cache;
    } while(1);
  }

  //
  Bool tryToLock() {
    if (ASM_SWAP(lc, PAR_LOCKED) == PAR_UNLOCKED) return (TRUE);
    else return (FALSE);
  }

  //
  void unlock() { lc = PAR_UNLOCKED; }
};

#if defined(__volatile_f)
#define volatile
#undef __volatile_f
#endif

#endif // VIRTUALSITES

#endif __ASM_CORE_H
