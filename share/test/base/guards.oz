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


%%% TODO: every test should be a new procedure


functor

export
   Return
define
   Initial={NewName}
   C={NewCell Initial}
   proc {Start V} {Exchange C Initial V} end
   proc {Next V1 V2} {Exchange C V1 V2} end
   proc {Final V} {Exchange C V Initial} end
   fun {Id X} X end
   Return =
   guards(
         proc {$}
            cond skip then {Start 1} else fail end
            {Final 1}

            cond skip then {Start 1}
            [] fail then fail
            else fail
            end
            {Final 1}

            cond fail then fail
            [] skip then {Start 1}
            else fail
            end
            {Final 1}

            cond fail then fail else {Start 1} end
            {Final 1}

            cond fail then fail
            [] fail then fail
            else
               {Start 1}
            end
            {Final 1}

            %% vars equal suspension
            local X Y Sync in
               thread
                  cond X=Y then {Next 1 2} Sync=1
                  else fail
                  end
               end
               {Start 1}
               Y=X
               {Wait Sync}
            end
            {Final 2}

         local X Sync in
            thread
               cond X = 1 then {Next 1 2} Sync=1 end
            end
            {Start 1}
            X = 1
            {Wait Sync}
         end
         {Final 2}

         local X Sync in
            thread
               cond X = 1 then {Next 1 2} Sync=1
               [] X = 2 then fail
               end
            end
            {Start 1}
            X = 1
            {Wait Sync}
         end
         {Final 2}

         local X Sync in
            thread
               cond X = 1 then fail
               [] X = 2 then {Next 1 2} Sync=1
               end
            end
            {Start 1}
            X = 2
            {Wait Sync}
         end
         {Final 2}

local X Sync in
   thread
      cond X = 1 then fail
      else {Next 1 2} Sync=1
      end
   end
   {Start 1}
   X = 2
   {Wait Sync}
end
{Final 2}

cond cond _ = 1 then skip else fail end then {Start 1} else fail end
{Final 1}

% -- deep
cond
   cond fail then fail
   else fail
   end
then fail
else {Start 1}
end
{Final 1}

% -- more than 2 clauses
local X Sync in
   thread
      or X = 1 then {Next 1 2} Sync=1
      [] X = 2 then fail
      [] X = 3 then fail
      end
   end
   X = 1
   {Start 1}
   {Wait Sync}
end
{Final 2}

cond X in
   X = {Id 4}
   or X = 1 then fail
   [] X = 2 then fail
   end
then fail
else {Start 1}
end
{Final 1}

% -- or
% or top commit
or skip
[] skip then fail
end


cond or skip
   [] skip then fail
   end
then {Start 1}
end
{Final 1}

local X in
   thread
      or X = 1
      [] skip
      end
   end
   X = 3
end

local X in
   cond or X = 1 then skip
      [] skip then skip
      end
   then {Start 1}
   else fail
   end
   X = 3
   {Next 1 2}
end
{Final 2}

% or top commit suspend
local X in
   thread
      or X = 1
      [] X = 2
      end
   end
   X = 1
end

cond X in
   thread
      or X = 1
      [] X = 2
      end
   end
   X = 1
then {Start 1}
else fail
end
{Final 1}

local X in
   thread
      or X = 1
      [] X = 2
      end
   end
   X = 2
end

cond X in
   thread
      or X = 1
      [] X = 2
      end
   end
   X = 2
then {Start 1}
else fail
end
{Final 1}

% or unit commit
local
   Sync
in
   thread
      or skip then {Next 1 2} Sync=1
      [] fail
      end
   end
   {Start 1}
   {Wait Sync}
   {Final 2}
end

local
   proc {Dummy} skip end
in
   cond or skip then {Dummy}
      [] fail
      end
   then {Start 1}
   else fail
   end
end
{Final 1}

or fail
[] skip then skip
end

local
   proc {Dummy} skip end
in
   cond or fail then skip
      [] skip then {Dummy}
      end
   then {Start 1}
   else fail
   end
end
{Final 1}

% or unit commit suspend
local X Sync in
   thread
      or X = 1 then {Next 1 2} Sync=1
      [] X = 2 then fail
      end
   end
   X = 1
   {Start 1}
   {Wait Sync}
end
{Final 2}

local
   proc {Dummy} skip end
in
   cond X in
      thread
         or X = 1 then {Dummy}
         [] X = 2 then fail
         end
      end
      X = 1
   then {Start 1}
   else fail
   end
end
{Final 1}

local X Sync in
   thread
      or X = 1 then fail
      [] X = 2 then {Next 1 2} Sync=1
      end
   end
   X = 2
   {Start 1}
   {Wait Sync}
end
{Final 2}

cond X in
   thread
      or X = 1 then fail
      [] X = 2 then skip
      end
   end
   X = 2
then {Start 1}
else fail
end
{Final 1}

cond X in or thread X = 1 end [] fail end then skip else fail end

cond X in  or X = 1 [] fail end then skip else fail end

cond X in  thread or thread X = 1 end [] fail end end then skip else fail end

cond X in  thread or X = 1 [] fail end end then skip else fail end

local X Sync in
   thread
      cond
         or
            cond X = 1 then skip else fail end
         [] fail
         end
      then Sync=1
      else fail
      end
   end
   X = 1
   {Start 1}
   {Wait Sync}
end
{Final 1}

% test args
% mm2: strange execution order !!!
local P Y Sync in
   proc {P A Y}
      thread
         cond Y = 1 then (A == ok)=true Sync=1
         [] Y = 2 then fail
         else fail
         end
      end
   end
   {P ok Y}
   Y = 1
   {Start 1}
   {Wait Sync}
end
{Final 1}

local P Y Sync in
   proc {P A Y}
      thread
         cond Y = 1 then fail
         [] Y = 2 then (A==ok)=true Sync=1
         else fail
         end
      end
   end
   {P ok Y}
   Y = 2
   {Start 1}
   {Wait Sync}
end
{Final 1}

local P Y Sync in
   proc {P A Y}
      thread
         cond Y = 1 then fail
         else (A==ok)=true Sync=1
         end
      end
   end
   {P ok Y}
   {Start 1}
   Y = 2
   {Wait Sync}
end
{Final 1}

local P in
   proc {P A B}
      cond A = f(_) then {Start 1}
      [] A = g(_) then {Next 1 2}
      else {Next 2 3}
      end
   end
   {P f(a) b}
   {P g(a) b}
   {P h(a) b}
end
{Final 3}

% propagation test
local X Y Sync in
   thread
      cond
         cond X = 1 then Y = 1 else fail end
         cond Y = 1 then skip else fail end
      then
         {Next 1 2} Sync=1
      else fail
      end
   end
   {Start 1}
   X = 1
   {Thread.preempt {Thread.this}} % fire cond
   {Thread.preempt {Thread.this}} % fire cond
   or Y = 1 [] fail end
   {Wait Sync}
   {Final 2}
end

local X Y Sync in
   thread
      cond
         thread
            or Z in X = 1 Y = 1
               cond Z = 1
               then skip
               else fail
               end
            then Z = 1
            [] X = 2 Y = 2 then fail
            end
         end
         cond Y = 1 then skip
         else fail
         end
      then
         {Next 1 2} Sync=1
      else fail
      end
   end
   {Start 1}
   X = 1
   or Y = 1 [] fail end
   {Wait Sync}
end
{Final 2}

% critical pair: unit commit & top commit detected concurrently
or thread skip end [] thread fail end end
or thread fail end [] thread skip end end

% fail both
cond X Y in
   thread Y = go or X = 1 [] X = 2 end end
   {Wait Y} X=3
then fail
else skip
end

% "eager" propagation
local
   proc {Loop} {Loop} end
in
   cond X in
      thread cond X = 1 then raise a end else fail end end
      X = 2
      {Loop}
   then
      fail
   else
      skip
   end
end

         end
         keys:[guards actor])
end
