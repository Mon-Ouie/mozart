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

local

   \insert 'transport/country.oz'
   
   functor MakeTransport prop once

   import
      Tk

      TkTools

      Applet

      OS

      Search.{SearchBest = 'SearchBest'}

      FD

   body
      Applet.spec = single(defaults(type:bool default:true)
			   random(type:bool default:true)
			   title(type:string default:"Transportation"))

      \insert 'transport/configure.oz'
      \insert 'transport/widgets.oz'
      \insert 'transport/randomizer.oz'
      \insert 'transport/agent.oz'
      \insert 'transport/frontend.oz'
      \insert 'transport/makeplan.oz'
      \insert 'transport/contract.oz'
      \insert 'transport/truck.oz'
      \insert 'transport/driver.oz'
      \insert 'transport/company.oz'
      \insert 'transport/broker.oz'

      F = {New Frontend init(toplevel:Applet.toplevel)}

      case Applet.args.defaults orelse Applet.args.random then
	 {F addDefaults}
      else skip
      end
      case Applet.args.random then
	 {F random}
      else skip
      end
   
   end

in
    
    MakeTransport

end
