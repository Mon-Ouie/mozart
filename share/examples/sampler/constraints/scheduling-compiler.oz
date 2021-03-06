%%%
%%% Authors:
%%%   Christian Schulte <schulte@ps.uni-sb.de>
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
%%%    http://www.mozart-oz.org
%%%
%%% See the file "LICENSE" or
%%%    http://www.mozart-oz.org/LICENSE.html
%%% for information on usage and redistribution
%%% of this file, and for a DISCLAIMER OF ALL
%%% WARRANTIES.
%%%

%% The scheduling compiler
declare
local
   fun {GetDur TaskSpec}
      {List.toRecord dur {Map TaskSpec fun {$ T}
                                          {Label T}#T.dur
                                       end}}
   end
   fun {GetStart TaskSpec}
      MaxTime = {FoldL TaskSpec fun {$ Time T} 
                                   Time+T.dur
                                end 0}
      Tasks   = {Map TaskSpec Label}
   in
      {FD.record start Tasks 0#MaxTime}
   end
   fun {GetTasksOnResource TaskSpec}
      D={Dictionary.new}
   in
      {ForAll TaskSpec 
       proc {$ T}
          if {HasFeature T res} then R=T.res in
             {Dictionary.put D R {Label T}|{Dictionary.condGet D R nil}}
          end
       end}
      {Dictionary.toRecord tor D}
   end
in
   fun {Compile Spec}
      TaskSpec    = Spec.tasks
      Constraints = if {HasFeature Spec constraints} then
		       Spec.constraints
		    else
		       proc {$ _ _} skip end
		    end
      Dur         = {GetDur TaskSpec}
      TasksOnRes  = {GetTasksOnResource TaskSpec}
   in
      proc {$ Start}
         Start = {GetStart TaskSpec}
         {ForAll TaskSpec
          proc {$ T}
             {ForAll {CondSelect T pre nil}
              proc {$ P}
                 Start.P + Dur.P =<: Start.{Label T}
              end}
          end}
         {Constraints Start Dur}
         {Schedule.serialized TasksOnRes Start Dur}
         {Schedule.firstsDist TasksOnRes Start Dur}
         choice skip end
         {Record.forAll Start proc {$ S} 
                                 S={FD.reflect.min S} 
                              end}
      end
   end 
end
