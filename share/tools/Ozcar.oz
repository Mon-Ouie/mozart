%%%
%%% Authors:
%%%   Benjamin Lorenz (lorenz@ps.uni-sb.de)
%%%
%%% Contributor:
%%%   Christian Schulte
%%%
%%% Copyright:
%%%   Benjamin Lorenz, 1997
%%%   Christian Schulte, 1998
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

functor $

import
   System
   Property
   Debug
   FD
   FS
   Tk
   TkTools
   Error
   Browser.{browse}
   
   Emacs.{getOPI
	  condSend}
   
   Compiler.{engine
	     quietInterface}

   DebugSupport
      from 'x-oz-boot:Debug'
   
export
   'Ozcar': Ozcar
   
   'object': Ozcar

body
   \insert 'ozcar/config'
   \insert 'ozcar/prelude'

   \insert 'ozcar/tree'
   \insert 'ozcar/thread'
   \insert 'ozcar/stack'

   \insert 'ozcar/source'

   \insert 'ozcar/menu'
   \insert 'ozcar/dialog'
   \insert 'ozcar/help'
   \insert 'ozcar/gui'

   \insert 'ozcar/ozcar'
end
