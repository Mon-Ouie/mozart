%%%
%%% Authors:
%%%   Nils Franz�n (nilsf@sics.se)
%%%   Simon Lindblom (simon@sics.se)
%%%
%%% Copyright:
%%%   Nils Franz�n, 1998
%%%   Simon Lindblom, 1998
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

functor
require
   Meths(getApplicationInfo:S_getApplicationInfo editApplication:S_editApplication) at 'methods.ozf'
import
   Tk
export
   start:Start
define
   proc{Start Id Server}
      T={New Tk.toplevel tkInit(title:"Edit Application")}
      V1 V2 V3 V4
      Index={NewCell 0}
      GO
      proc{Start2}
         A=S_editApplication(name: {V1 tkReturnString($)}
                             serverurl:{V2 tkReturnString($)}
                             clienturl:{V3 tkReturnString($)}
                             author: Args.author
                             description: {V4 tkReturnString($)}
                             id: Args.id)
      in
         {Wait A.name} {Wait A.serverurl} {Wait A.clienturl}
         {Wait A.description}
         {T tkClose}

         {Server A}
      end

      Args

      proc{NewEntry Title Value V}
         O N E L={New Tk.label tkInit(parent:T text:Title)}
      in
         {Exchange Index O N} N=O+1
         V={New Tk.variable tkInit(Value)}
         E={New Tk.entry tkInit(parent:T width:50 textvariable:V)}
         {Tk.batch [grid(L row:N column:0 sticky:e)
                    grid(E row:N column:1 sticky:w)]}
         {E tkBind(event:'<Return>' action:proc{$} GO=unit end)}
         if N==1 then {Tk.send focus(E)} else skip end
      end
   in
      Args = {Server S_getApplicationInfo(id:Id info:$)}

      V1={NewEntry "Application Name:" Args.name}
      V4={NewEntry "Description:" Args.description}
      V2={NewEntry "Server URL:" Args.serverurl}
      V3={NewEntry "Client URL:" Args.clienturl}

      {Wait GO}
      {Start2}
   end
end
