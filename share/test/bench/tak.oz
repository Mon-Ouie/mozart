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
%%%    http://mozart.ps.uni-sb.de
%%%
%%% See the file "LICENSE" or
%%%    http://mozart.ps.uni-sb.de/LICENSE.html
%%% for information on usage and redistribution
%%% of this file, and for a DISCLAIMER OF ALL
%%% WARRANTIES.
%%%

functor $

export
   Return

body

   fun {Tak X Y Z}
      case X=<Y then Z
      else {TakHelp X Y Z}
      end
   end

   fun {TakHelp X Y Z}
      {Tak {Tak X-1 Y Z} {Tak Y-1 Z X} {Tak Z-1 X Y}}
   end


   fun {TakB N} {Tak 3*N 2*N N} end

   fun {TakF X Y Z}
      case X=<Y then Z
      else {TakFHelp X Y Z}
      end
   end

   fun {TakFHelp X Y Z}
      {TakF
       thread {TakF X-1 Y Z} end
       thread {TakF Y-1 Z X} end
       thread {TakF Z-1 X Y} end}
   end


   fun {TakFB N} {TakF 3*N 2*N N} end

   Return = tak([normal(proc {$}
                           _={TakB 7}
                        end
                        keys:[bench takeushi]
                        bench:true)
                 'thread'(proc {$}
                             {Wait {TakFB 6}}
                          end
                          keys:[bench takeushi 'thread']
                          bench:true)
                ])
end
