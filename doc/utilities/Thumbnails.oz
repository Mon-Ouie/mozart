%%%
%%% Author:
%%%   Leif Kornstaedt <kornstae@ps.uni-sb.de>
%%%
%%% Copyright:
%%%   Leif Kornstaedt, 1998
%%%
%%% Last change:
%%%   $Date$ by $Author$
%%%   $Revision$
%%%
%%% This file is part of Mozart, an implementation of Oz 3:
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
   OS(system)
export
   'class': ThumbnailsClass
define
   GIF2THUMBNAIL = 'gif2thumbnail'

   class ThumbnailsClass
      attr DirName: unit N: unit
      meth init(Dir)
         DirName <- Dir
         N <- 0
      end
      meth get(Dir FileName ?OutFileName)
         N <- @N + 1
         OutFileName = 'thumbnail'#@N#'.gif'
         case
            {OS.system GIF2THUMBNAIL#' '#Dir#FileName#' '#@DirName#'/'#OutFileName}
         of 0 then skip
         elseof I then
            {Exception.raiseError ozDoc(thumbnail FileName I)}
         end
      end
   end
end
