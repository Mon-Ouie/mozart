%%%
%%% Authors:
%%%   Christian Schulte <schulte@ps.uni-sb.de>
%%%
%%% Copyright:
%%%   Christian Schulte, 1999
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

functor

import
   Space System FD

export
   Return

define

   fun {DerefBlocked S}
      case S of blocked(S) then {DerefBlocked S} else S end
   end

   fun {AskVerbose S}
      {DerefBlocked {Space.askVerbose S}}
   end

   
   Return=
   space([port(proc {$}
		  Xs
		  P={NewPort Xs}
		  Goods

		  G1|G2|G3|G4|G5|_=Goods

		  local
		     proc {Find X Y|Yr}
			if {System.eq X Y} then skip else {Find X Yr} end
		     end
		     proc {CheckSame Xs Ys}
			case Xs
			of nil then skip
			[] X|Xr then {Find X Ys} {CheckSame Xr Ys}
			end
		     end
		  in
		     proc {Check X Cs}
			case
			   try
			      {Port.send P X} nil
			   catch error(kernel(spaceSituatedness Xs) ...) then
			      Xs
			   end
			of nil then skip
			[] Xs then {CheckSame Xs Cs}
			end
		     end
		  end

		  G1 = a(b c|d)  
		  G2 = FD
		  G3 = local Z in
			  Z=a(a(a:Z) Z Z|Z) Z
		       end
		  G4 = local Z in
			  Z=a(a(a:Z) Z Z|Z Append Object.base) Z
		       end
		  G5 = local Z F in
			  F={fun lazy {$ X} X end 1}
			  Z=a(a(a:Z) Z Z|Z F Append Object.base) Z
		       end
		  
		  S={Space.new proc {$ X}
				  proc {P} skip end
				  proc {Q} skip end
			       in
				  {Check G1 nil} 
				  {Check G2 nil}
				  {Check a(X X P) [X P]}
				  {Check a(X X P Q P Q) [X P Q]}
				  {Check a(X X) [X]}
				  local Z in
				     Z=a(X X P Q Z Z|Z)
				     {Check Z [X P Q]}
				  end
				  {Check G3 nil}
				  {Check G4 nil}
				  {Check G5 nil}
				  local Z F in
				     F={fun lazy {$ X} X end 1}
				     Z=a(a(a:Z) Z Z|Z F P Append Object.base)
				     {Check Z [P]}
				  end
				  local Z F in
				     F={fun lazy {$ X} X end 1}
				     Z=a(a(a:Z) Z Z|Z F P Q X
					 Append Object.base)
				     {Check Z [P Q X]}
				  end
			       end}
	       in

		  thread
		     {ForAll {List.zip Xs Goods fun {$ A G} A#G end}
		      proc {$ A#G}
			 B=(A==G)
		      in
			 {IsDet B true} B=true
		      end}
		  end

		  {AskVerbose S}=succeeded(entailed)
		  
	       end
	       keys:[port space situatedness])])
end



