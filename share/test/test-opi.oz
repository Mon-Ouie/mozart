%%%
%%% Authors:
%%%   Christian Schulte (schulte@dfki.de)
%%%
%%% Copyright:
%%%   Christian Schulte, 1998
%%%
%%% Last change:
%%%   $Date$ by $Author$
%%%   $Revision$
%%%
%%% This file is part of Mozart, an implementation
%%% of Oz 3
%%%    http://mozart.ps.uni-sb.de
%%%
%%% See the file "LICENSE" or
%%%    http://mozart.ps.uni-sb.de/LICENSE.html
%%% for information on usage and redistribution
%%% of this file, and for a DISCLAIMER OF ALL
%%% WARRANTIES.
%%%

{OS.chDir {OS.getEnv 'HOME'}#'/mozart/share/test'}

declare
T={{New Module.manager init}
   link(url:{OS.getEnv 'HOME'}#'/mozart/share/test/te.ozf' $)}.run

{T argv(verbose:  true
	usage:    false
	help:     false
	keys:     nil   % nil for `all' or a non-empty list of strings: ["fs"]
	ignores:  nil   % nil for `none' or a non-empty list of strings
	tests:    nil   % nil for `none' or a non-empty list of strings
	do:       true
	time:     ""
	gc:       0
        threads:  1) _}

{Property.put 'messages.idle' true}
