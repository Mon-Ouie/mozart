/*
 *  Authors:
 *    Ralf Scheidhauer (Ralf.Scheidhauer@ps.uni-sb.de)
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
 *     $MOZARTURL$
 * 
 *  See the file "LICENSE" or
 *     $LICENSEURL$
 *  for information on usage and redistribution 
 *  of this file, and for a DISCLAIMER OF ALL 
 *  WARRANTIES.
 *
 */

/* "windows.h" defines some constants, that are also used in Oz,
 * so this file MUST BE INCLUDED BEFORE ANY OTHER FILE
 */


#ifndef __WSOCK_H__
#define __WSOCK_H__

#ifdef HAVE_CONFIG_H
#include "conf.h"
#endif

#ifdef WINDOWS

#define NOMINMAX
#define Bool WinBool
#define min winmin
#define max winmax

#include <windows.h>

#undef min
#undef max

#undef FAILED /* used in oz.h as well */
#undef Bool

#ifdef GNUWIN32

/* The headers of gnu win32 are incomplete: */

/* do not redefine FD_* macros, have to use win32 versions */
#define _POSIX_SOURCE  
#include <sys/types.h>
#undef _POSIX_SOURCE

#include <sys/times.h>
#include <fcntl.h>

extern "C" {

#include "winsock.h"

  inline void _endthreadex( unsigned __retval )
  {
    ExitThread(__retval);
  }

#define _beginthreadex(security, stack_size,fun,args,initflag,thrdaddr) \
  CreateThread(security,stack_size,fun,args,initflag,thrdaddr);

}


#endif

#endif

#endif


