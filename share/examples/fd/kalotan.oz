%%%
%%% Authors:
%%%   Gert Smolka <smolka@ps.uni-sb.de>
%%%
%%% Copyright:
%%%   Gert Smolka, 1998
%%%
%%% Last change:
%%%   $Date$ by $Author$
%%%   $Revision$
%%%
%%% This file is part of Mozart, an implementation
%%% of Oz 3
%%%    http://www.mozart-oz.org
%%%
%%% See the file "LICENSE" or
%%%    http://www.mozart-oz.org/LICENSE.html
%%% for information on usage and redistribution
%%% of this file, and for a DISCLAIMER OF ALL
%%% WARRANTIES.
%%%

declare
proc {Problem Solution}
   Vars
   [Sex Claim Truth        % Kibi
    Sex1 Truth1            % Parent 1
    Sex2 Truth2a Truth2b]  % Parent 2
      = Vars
in
   Vars:::0#1
   (Claim=:Sex)=:Truth
   Sex+Truth>:0
   (Claim=:0)=:Truth1
   Sex1+Truth1>:0
   (Sex=:1)=:Truth2a
   (Truth=:0)=:Truth2b
   Sex2+Truth2a+Truth2b=:2
   Sex1\=:Sex2
   Solution=Sex#Sex1#Sex2
   {FD.distribute ff Vars}
end

{ExploreOne Problem}