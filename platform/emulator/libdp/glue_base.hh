/*
 *  Authors:
 *    Erik Klintskog, 2002
 *    Zacharias El Banna, 2002
 * 
 *  Contributors:
 *    optional, Contributor's name (Contributor's email address)
 * 
 *  Copyright:
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

#ifndef _GLUE_BASE_HH
#define _GLUE_BASE_HH


/*
  Protected Oz Data
*/
extern OZ_Term g_connectPort;
extern OZ_Term g_kbrStreamPort;
extern OZ_Term g_defaultAcceptProcedure;
extern OZ_Term g_faultPort;

void doPortSend(OzPort *port, TaggedRef val, Board*);

#endif
