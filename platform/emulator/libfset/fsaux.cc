/*
 *  Authors:
 *    Tobias Mueller (tmueller@ps.uni-sb.de)
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

#include <stdarg.h>

#include "fsaux.hh"

OZ_BI_define(fsp_init, 0,1)
{
#ifdef OZ_DEBUG
  oz_fsetdebugprint("*** DEBUG-FSETLIB ***");
#elif OZ_PROFILE
  oz_fsetdebugprint("*** PROFILE-FSETLIB ***");
#endif
  //  cout << "fsetlib " << __DATE__ << ' ' << __TIME__ << endl << flush;
  OZ_RETURN_ATOM(__DATE__ " (" __TIME__ ")");
} OZ_BI_end


extern FILE *cpi_fileout;

void oz_fsetdebugprint(char *format, ...)
{
  va_list ap;
  va_start(ap,format);
  vfprintf(cpi_fileout,format,ap);
  va_end(ap);

  fprintf(cpi_fileout, "\n");
  fflush(cpi_fileout);
}
