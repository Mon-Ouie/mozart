%%%
%%% Authors:
%%%   Author's name (Author's email address)
%%%
%%% Contributors:
%%%   optional, Contributor's name (Contributor's email address)
%%%
%%% Copyright:
%%%   Organization or Person (Year(s))
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
%%% $Id$
%%% Benjamin Lorenz <lorenz@ps.uni-sb.de>

declare
   NewOzcar
in

fun
\ifdef NEWCOMPILER
   instantiate
\endif
   {NewOzcar IMPORT}
   \insert 'SP.env'
      = IMPORT.'SP'
   \insert 'WP.env'
      = IMPORT.'WP'
   \insert 'Browser.env'
      = IMPORT.'Browser'
   \insert 'Emacs.env'
      = IMPORT.'Emacs'
   \insert 'Compiler.env'
      = IMPORT.'Compiler'

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
in
   \insert 'Ozcar.env'
end



