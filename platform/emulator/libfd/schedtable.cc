/*
 *  Authors:
 *    Christian Schulte (schulte@dfki.de)
 *
 *  Copyright:
 *    Christian Schulte, 1997, 1998
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

#ifdef HAVE_CONFIG_H
#include "conf.h"
#endif

#include "../oz.h"

/*
 * The builtin table
 */

#ifndef MODULES_LINK_STATIC

#include "../modSchedule.dcl"

OZ_C_proc_interface mod_int_Schedule[] = {
#include "../modSchedule.tbl"
 {0,0,0,0}
};

#endif
