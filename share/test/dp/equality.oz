%%%
%%% Authors:
%%%   Erik Klintskog (erik@sics.se)
%%%
%%% Copyright:
%%%   Erik Klintskog, 1998
%%%
%%% Last change:
%%%   $Date$Author: 
%%%   $Revision: 
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

functor

import
   Remote(manager)
   OS(uName)
   System
%   Fault(install deinstall)
export
   Return
define
   PP = pp(_ _ _ _)
   RM = pp(_ _ _ _)
   
   MyStream
      
   PP.1 = {NewPort MyStream}
   
   Dist = [2 3 4 1 2 3 4 2
	   1 2 1 2 3 2 1 2
	   3 4 3 2 1 1 2 3 1]
   
   CC = {NewCell _}
   
   proc{GCdo}
      Pa2   Pa3   Pa4
   in
      {Send PP.2 gcDo(Pa2)}
      {Send PP.3 gcDo(Pa3)}
      {Send PP.4 gcDo(Pa4)}
      {Wait Pa2}
      {Wait Pa3}
      {Wait Pa4}
      {System.gcDo}
   end
   
   proc{RRsend RR L}
      H = RR.1
   in
      {Assign CC RR.2#_}
      {For 2 4 1 proc{$ P}
		    {Send PP.P newEntity(RR.2)}
		 end}
      {Send PP.(L.1) H(RR.2 L.2)}
      if {Access CC}.2 == ok then
	 skip
      else
	 raise {Access CC}.2 end
      end
      {GCdo}
   end
/*   
   proc{Watch A B}
      {Send PP.1 siteFault(siteDown)}
   end
*/   
   thread 
      try
	 {ForAll MyStream
	  
	  proc{$ X}
	     case X of entity(R L) then
		{Access CC}.1 = R
		if L == nil then 
		   {Access CC}.2 = ok
		else
		   {Send PP.(L.1) entity(R L.2)}
		end
	     elseof  gcDo(A) then
		A = unit
	     elseof silentDeath then
		raise hell end
	     elseof siteFault(M) then
		raise M end
	     end
	  end}
      catch EXP then
	 if (EXP == hell) then   skip
	 else   {Access CC}.2 = EXP end
      end
   end
   
   {For 2 4 1 proc{$ Nr}
		 RM.Nr={New Remote.manager
			init(host:{OS.uName}.nodename)}
		 {RM.Nr ping}
		 {RM.Nr apply(url:'' functor
				     import
					System
				     define
					local
					   MyStream
					   MemCell = {NewCell apa}
					in
					   PP.Nr = {NewPort MyStream}
					   thread
					      try
						 {ForAll MyStream
						  proc{$ X}
						     case X of entity(R L) then 
							{Access MemCell} = R
							{Send PP.(L.1) entity(R L.2)}
						     elseof newEntity(E) then
							{Assign MemCell E}
						     elseof gcDo(A) then
							{System.gcDo}
							A = unit
						     end
						  end}
					      catch M then
						 {Send PP.1 siteFault(M)}
					      end
					   end
					end
				     end
			     )}
		 {RM.Nr ping}
		 {Wait PP.Nr}
%		 {Fault.install PP.Nr watcher('cond':permHome)
%		  Watch}
	      end}
   
   Return=
   dp([equality([atom(
		    proc{$}
		       {RRsend entity#apa  Dist}
		    end
		    keys:[remote])
		 list(
		    proc{$}
		       {RRsend entity#[apa bapa rapa skrapa]  Dist}
		    end
		    keys:[remote])
		 string(
		    proc{$}
		       {RRsend entity#"apan bapa rapar sa att det i marken skrapar"  Dist}
		    end
		    keys:[remote])
		 name(
		    proc{$}
		       {RRsend entity#{NewName}  Dist}
		    end
		    keys:[remote])
		 'lock'(
		    proc{$}
			      {RRsend entity#{NewLock}  Dist}
			   end
		    keys:[remote])
		 cell(
		    proc{$}
		       {RRsend entity#{NewCell apa}  Dist}
		    end
		    keys:[remote])
		 port(
		    proc{$}
		       {RRsend entity#{NewPort _ $}  Dist}
		    end
		    keys:[remote])
		 'proc'(
		    proc{$}
		       {RRsend entity#proc{$ D } A = 2 in  D=A*2 end  Dist}
		    end
		    keys:[remote])
		 object(
		    proc{$}
		       {RRsend entity#{New class $
					      feat a
					      meth init self.a = 6 end
					   end
				       init}  Dist}
		    end
		    keys:[remote])
		 'class'(
		    proc{$}
		       {RRsend entity#class $
					 feat a
					 meth init self.a = 6 end
				      end
			Dist}
		    end
		    keys:[remote])
		 close(
		    proc {$}
		       {For 2 4 1 proc{$ Nr}
%				     {Fault.deinstall PP.Nr
%				      watcher('cond':permHome) Watch}
				     {RM.Nr close}
				  end}
				
		       {Send PP.1 silentDeath}
		    end
		    keys:[remote])
	       ])
      ])
end







