%%%
%%% Authors:
%%%   Michael Mehl (mehl@dfki.de)
%%%
%%% Copyright:
%%%   Michael Mehl, 1998
%%%
%%% Last change:
%%%   $Date$ by $Author$
%%%   $Revision$
%%%
%%% This file is part of Mozart, an implementation
%%% of Oz 3
%%%    $MOZARTURL$
%%%
%%% See the file "LICENSE" or
%%%    $LICENSEURL$
%%% for information on usage and redistribution
%%% of this file, and for a DISCLAIMER OF ALL
%%% WARRANTIES.
%%%

fun {$ IMPORT}
   \insert 'lib/import.oz'
in

   float(proc {$}
	    D1 = [1.0 1.49 1.5 0.1 0.5
	 % 1.12e10
	 % 1.12e30
		  1.0e3
	 % 1.0e37
		  1.43e~4 1.43e~14 1.43e~34
		  ~1.0 ~1.49 ~1.5 ~0.1 ~0.5
	 % ~1.12e10
	 % ~1.12e30
		  ~1.43e~4 ~1.43e~14 ~1.43e~34]
	 in
	    {Map D1 Float.toString _}
	    {Map D1 Float.toInt _}
	    {Map D1 Float.round _}
	 end
	keys:[module float])
end

/*
{Show .0}
{Show 0.0}
{Show 1.0e500}
{Show ~1.0e500}
{Show ~1.0e~500}
{Show 1.0e~500}
{Show ~0.0}
{Show 0.0}
{Show 0.0/0.0}
{Show 1.0/0.0}
{Show ~1.e500/0.0}
{Show 1.0}
{Show 1.0e167}
{Show ~1.0}
{Show 1.0e~167}
{Show ~1.0e167}
{Show ~1.0e~167}
*/
