%%%
%%% Author:
%%%   Benjamin Lorenz <lorenz@ps.uni-sb.de>
%%%
%%% Copyright:
%%%   Benjamin Lorenz, 1997
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

Profiler =
{New class
        from
           Gui

        meth init
           Gui,init
        end

        meth off
           Time.repeat,stop
           {Tk.send wm(withdraw self.toplevel)}
           {EnqueueCompilerQuery setSwitch(profile false)}
           {Profile.mode false}
        end

        meth on
           {Tk.batch [update(idletasks)
                      wm(deiconify self.toplevel)]}
           {EnqueueCompilerQuery setSwitch(profile true)}
           {Profile.mode true}
           {Profile.reset}
           if {Cget update} > 0 then
              Time.repeat,go
           end
        end

     end init}
