/*
  Hydra Project, DFKI Saarbruecken,
  Stuhlsatzenhausweg 3, D-66123 Saarbruecken, Phone (+49) 681 302-5312
  Author: tmueller
  Last modified: $Date$ from $Author$
  Version: $Revision$
  State: $State$

  ------------------------------------------------------------------------
*/

#include "fsaux.hh"

OZ_C_proc_begin(fsp_init, 1)
{
#ifdef OZ_DEBUG
  cout << "*** DEBUG-FSETLIB ***" << endl << flush;
#elif OZ_PROFILE
  cout << "*** PROFILE-FSETLIB ***" << endl << flush;
#endif
  //  cout << "fsetlib " << __DATE__ << ' ' << __TIME__ << endl << flush;
  return OZ_unifyAtom(OZ_args[0], __DATE__ " (" __TIME__ ")");
}
OZ_C_proc_end

