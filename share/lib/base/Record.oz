%%%
%%% Authors:
%%%   Martin Henz (henz@iscs.nus.edu.sg)
%%%   Christian Schulte (schulte@dfki.de)
%%%
%%% Copyright:
%%%   Martin Henz, 1997
%%%   Christian Schulte, 1997
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


%%
%% Global
%%

local
   fun {MakeEmpty As}
      case As of nil then nil
      [] A|Ar then A#_|{MakeEmpty Ar}
      end
   end
in
   fun {MakeRecord L As}
      {List.toRecord L {MakeEmpty As}}
   end
end


%%
%% Module
%%
local
   local
      BIMonitorArity = Boot_Record.monitorArity
   in
      proc {MonitorArity R P S}
         U in
         {BIMonitorArity R U S}
         proc {P} U=unit end
      end

      proc {ReflectArity R S}
         {BIMonitorArity R unit S}
      end
   end

   fun {MakePairs As R}
      case As of nil then nil
      [] A|Ar then A#R.A|{MakePairs Ar R}
      end
   end

   fun {Subtract R F}
      {List.toRecord {Label R} {MakePairs {List.subtract {Arity R} F} R}}
   end

   %%
   %% Higher-order Stuff without Indices
   %%
   fun {Map As X P}
      case As of nil then nil
      [] A|Ar then A#{P X.A}|{Map Ar X P}
      end
   end

   proc {MapT I W X P Y}
      case I=<W then {P X.I Y.I} {MapT I+1 W X P Y}
      else skip
      end
   end

   fun {FoldL As X P Z}
      case As of nil then Z
      [] A|Ar then {FoldL Ar X P {P Z X.A}}
      end
   end

   fun {FoldLT I W X P Z}
      case I<W then {FoldLT I+1 W X P {P Z X.I}}
      else {P Z X.I}
      end
   end

   fun {FoldR As X P Z}
      case As of nil then Z
      [] A|Ar then {P X.A {FoldR Ar X P Z}}
      end
   end

   fun {FoldRT I W X P Z}
      case I<W then {P X.I {FoldRT I+1 W X P Z}}
      else {P X.I Z}
      end
   end

   proc {ForAll As X P}
      case As of nil then skip
      [] A|Ar then {P X.A} {ForAll Ar X P}
      end
   end

   proc {ForAllT I W X P}
      case I=<W then {P X.I} {ForAllT I+1 W X P}
      else skip
      end
   end

   fun {All As X P}
      case As of nil then true
      [] A|Ar then {P X.A} andthen {All Ar X P}
      end
   end

   fun {AllT I W X P}
      case I=<W then {P X.I} andthen {AllT I+1 W X P}
      else true
      end
   end

   fun {Some As X P}
      case As of nil then false
      [] A|Ar then {P X.A} orelse {Some Ar X P}
      end
   end

   fun {SomeT I W X P}
      I=<W andthen ({P X.I} orelse {SomeT I+1 W X P})
   end

   fun {Filter As X P}
      case As of nil then nil
      [] A|Ar then XA=X.A in
         case {P XA} then A#XA|{Filter Ar X P}
         else {Filter Ar X P}
         end
      end
   end

   proc {Part As X P ?Bs ?Cs}
      case As of nil then Bs=nil Cs=nil
      [] A|Ar then XA=X.A in
         case {P XA} then Bs=A#XA|{Part Ar X P $ Cs}
         else Cs=A#XA|{Part Ar X P Bs $}
         end
      end
   end

   fun {Take As X P}
      case As of nil then nil
      [] A|Ar then XA=X.A in
         case {P XA} then A#XA|{Take Ar X P} else nil end
      end
   end

   fun {Drop As X P}
      case As of nil then nil
      [] A|Ar then XA=X.A in
         case {P XA} then nil else A#XA|{Drop Ar X P} end
      end
   end

   proc {TakeDrop As X P ?Bs ?Cs}
      case As of nil then Bs=nil Cs=nil
      [] A|Ar then XA=X.A in
         case {P XA} then Bs=A#XA|{TakeDrop Ar X P $ Cs}
         else Bs=nil Cs={MakePairs As X}
         end
      end
   end

   %%
   %% Higher-order Stuff with Indices
   %%
   fun {MapInd As X P}
      case As of nil then nil
      [] A|Ar then A#{P A X.A}|{MapInd Ar X P}
      end
   end

   proc {MapIndT I W X P Y}
      case I=<W then {P I X.I Y.I} {MapIndT I+1 W X P Y}
      else skip
      end
   end

   fun {FoldLInd As X P Z}
      case As of nil then Z
      [] A|Ar then {FoldLInd Ar X P {P A Z X.A}}
      end
   end

   fun {FoldLIndT I W X P Z}
      case I=<W then {FoldLIndT I+1 W X P {P I Z X.I}}
      else Z
      end
   end

   fun {FoldRInd As X P Z}
      case As of nil then Z
      [] A|Ar then {P A X.A {FoldRInd Ar X P Z}}
      end
   end

   fun {FoldRIndT I W X P Z}
      case I=<W then {P I X.I {FoldRIndT I+1 W X P Z}}
      else Z
      end
   end

   proc {ForAllInd As X P}
      case As of nil then skip
      [] A|Ar then {P A X.A} {ForAllInd Ar X P}
      end
   end

   proc {ForAllIndT I W X P}
      case I=<W then {P I X.I} {ForAllIndT I+1 W X P}
      else skip
      end
   end

   fun {AllInd As X P}
      case As of nil then true
      [] A|Ar then {P A X.A} andthen {AllInd Ar X P}
      end
   end

   fun {AllIndT I W X P}
      case I=<W then {P I X.I} andthen {AllIndT I+1 W X P}
      else true
      end
   end

   fun {SomeInd As X P}
      case As of nil then false
      [] A|Ar then {P A X.A} orelse {SomeInd Ar X P}
      end
   end

   fun {SomeIndT I W X P}
      I=<W andthen ({P I X.I} orelse {SomeIndT I+1 W X P})
   end

   fun {FilterInd As X P}
      case As of nil then nil
      [] A|Ar then XA=X.A in
         case {P A XA} then A#XA|{FilterInd Ar X P}
         else {FilterInd Ar X P}
         end
      end
   end

   proc {PartInd As X P ?Bs ?Cs}
      case As of nil then Bs=nil Cs=nil
      [] A|Ar then XA=X.A in
         case {P A XA} then Bs=A#XA|{PartInd Ar X P $ Cs}
         else Cs=A#XA|{PartInd Ar X P Bs $}
         end
      end
   end

   fun {TakeInd As X P}
      case As of nil then nil
      [] A|Ar then XA=X.A in
         case {P A XA} then A#XA|{TakeInd Ar X P} else nil end
      end
   end

   fun {DropInd As X P}
      case As of nil then nil
      [] A|Ar then XA=X.A in
         case {P A XA} then nil else A#XA|{DropInd Ar X P} end
      end
   end

   proc {TakeDropInd As X P ?Bs ?Cs}
      case As of nil then Bs=nil Cs=nil
      [] A|Ar then XA=X.A in
         case {P A XA} then Bs=A#XA|{TakeDropInd Ar X P $ Cs}
         else Bs=nil Cs={MakePairs As X}
         end
      end
   end

   fun {Zip As R1 R2 P}
      case As of nil then nil
      [] A|Ar then
         case {HasFeature R1 A} then A#{P R1.A R2.A}|{Zip Ar R1 R2 P}
         else {Zip Ar R1 R2 P}
         end
      end
   end

   proc {ZipT I W T1 T2 P T3}
      case I=<W then T3.I={P T1.I T2.I} {ZipT I+1 W T1 T2 P T3}
      else skip end
   end

   fun {ToList As R}
      case As of nil then nil
      [] A|Ar then R.A|{ToList Ar R}
      end
   end

   fun {ToListT I W T}
      case I>W then nil else T.I|{ToListT I+1 W T} end
   end

   fun {ToListInd As R}
      case As of nil then nil
      [] A|Ar then A#R.A|{ToListInd Ar R}
      end
   end

   fun {ToListIndT I W T}
      case I>W then nil else I#T.I|{ToListIndT I+1 W T} end
   end

   proc {TupleToDictionary N T D}
      case N==0 then skip else
         {Dictionary.put D N T.N} {TupleToDictionary N-1 T D}
      end
   end

   proc {RecordToDictionary As R D}
      case As of nil then skip
      [] A|Ar then
         {Dictionary.put D A R.A} {RecordToDictionary Ar R D}
      end
   end

in

   Record = record(is:           IsRecord
                   make:         MakeRecord

                   label:        Label
                   width:        Width

                   isC:          IsRecordC
                   tell:         TellRecord
                   '^':          Boot_Record.'^'
                   widthC:       WidthC
                   monitorArity: MonitorArity
                   reflectArity: ReflectArity
                   hasLabel:     Boot_Record.hasLabel

                   arity:        Arity
                   adjoin:       Adjoin
                   adjoinAt:     AdjoinAt
                   adjoinList:   AdjoinList

                   subtract:     Subtract

                   toList:
                      fun {$ R}
                         case {IsTuple R} then {ToListT 1 {Width R} R}
                         else {ToList {Arity R} R}
                         end
                      end
                   toListInd:
                      fun {$ R}
                         case {IsTuple R} then {ToListIndT 1 {Width R} R}
                         else {ToListInd {Arity R} R}
                         end
                      end

                   map:
                      proc {$ R1 P R2}
                         case {IsTuple R1} then
                            W={Width R1}
                         in
                            {MakeTuple {Label R1} W R2}
                            {MapT 1 W R1 P R2}
                         else
                            {List.toRecord {Label R1} {Map {Arity R1} R1 P} R2}
                         end
                      end
                   foldL:
                      fun {$ R P Z}
                         case {IsTuple R} then
                            case {IsLiteral R} then Z
                            else {FoldLT 1 {Width R} R P Z}
                            end
                         else {FoldL {Arity R} R P Z}
                         end
                      end
                   foldR:
                      fun {$ R P Z}
                         case {IsTuple R} then
                            case {IsLiteral R} then Z
                            else {FoldRT 1 {Width R} R P Z}
                            end
                         else {FoldR {Arity R} R P Z}
                         end
                      end
                   forAll:
                      proc {$ R P}
                        case {IsTuple R} then {ForAllT 1 {Width R} R P}
                        else {ForAll {Arity R} R P}
                        end
                      end
                   all:
                      fun {$ R P}
                        case {IsTuple R} then {AllT 1 {Width R} R P}
                        else {All {Arity R} R P}
                        end
                      end
                   some:
                      fun {$ R P}
                         case {IsTuple R} then {SomeT 1 {Width R} R P}
                         else {Some {Arity R} R P}
                         end
                      end
                   filter:
                      fun {$ R P}
                         {List.toRecord {Label R} {Filter {Arity R} R P}}
                      end
                   partition:
                      proc {$ R1 P ?R2 ?R3}
                         AXs BXs L={Label R1}
                      in
                         {Part {Arity R1} R1 P ?AXs ?BXs}
                         R2={List.toRecord L AXs}
                         R3={List.toRecord L BXs}
                      end
                   takeWhile:
                      fun {$ R P}
                         {List.toRecord {Label R} {Take {Arity R} R P}}
                      end
                   dropWhile:
                      fun {$ R P}
                         {List.toRecord {Label R} {Drop {Arity R} R P}}
                      end
                   takeDropWhile:
                      proc {$ R1 P ?R2 ?R3}
                         AXs BXs L={Label R1}
                      in
                         {TakeDrop {Arity R1} R1 P ?AXs ?BXs}
                         R2={List.toRecord L AXs}
                         R3={List.toRecord L BXs}
                      end

                   mapInd:
                      proc {$ R1 P ?R2}
                         case {IsTuple R1} then
                            W={Width R1}
                         in
                            {MakeTuple {Label R1} W ?R2}
                            {MapIndT 1 W R1 P R2}
                         else
                            {List.toRecord {Label R1}
                             {MapInd {Arity R1} R1 P} ?R2}
                         end
                      end
                   foldLInd:
                      fun {$ R P Z}
                         case {IsTuple R} then
                            case {IsLiteral R} then Z
                            else {FoldLIndT 1 {Width R} R P Z}
                            end
                         else {FoldLInd {Arity R} R P Z}
                         end
                      end
                   foldRInd:
                      fun {$ R P Z}
                         case {IsTuple R} then
                            case {IsLiteral R} then Z
                            else {FoldRIndT 1 {Width R} R P Z}
                            end
                         else {FoldRInd {Arity R} R P Z}
                         end
                      end
                   forAllInd:
                      proc {$ R P}
                         case {IsTuple R} then {ForAllIndT 1 {Width R} R P}
                         else {ForAllInd {Arity R} R P}
                         end
                      end
                   allInd:
                      fun {$ R P}
                         case {IsTuple R} then {AllIndT 1 {Width R} R P}
                         else {AllInd {Arity R} R P}
                         end
                      end
                   someInd:
                      fun {$ R P}
                         case {IsTuple R} then {SomeIndT 1 {Width R} R P}
                         else {SomeInd {Arity R} R P}
                         end
                      end
                   filterInd:
                      fun {$ R P}
                         {List.toRecord {Label R} {FilterInd {Arity R} R P}}
                      end
                   partitionInd:
                      proc {$ R1 P ?R2 ?R3}
                         AXs BXs L={Label R1}
                      in
                         {PartInd {Arity R1} R1 P ?AXs ?BXs}
                         R2={List.toRecord L AXs}
                         R3={List.toRecord L BXs}
                      end
                   takeWhileInd:
                      fun {$ R P}
                         {List.toRecord {Label R} {TakeInd {Arity R} R P}}
                      end
                   dropWhileInd:
                      fun {$ R P}
                         {List.toRecord {Label R} {DropInd {Arity R} R P}}
                      end
                   takeDropWhileInd:
                      proc {$ R1 P ?R2 ?R3}
                         AXs BXs L={Label R1}
                      in
                         {TakeDropInd {Arity R1} R1 P ?AXs ?BXs}
                         R2={List.toRecord L AXs}
                         R3={List.toRecord L BXs}
                      end

                   zip:
                      proc {$ R1 R2 P ?R3}
                         case {IsTuple R1} andthen {IsTuple R2} then
                            W={Min {Width R1} {Width R2}}
                         in
                            {MakeTuple {Label R1} W ?R3}
                            {ZipT 1 W R1 R2 P R3}
                         else
                            {List.toRecord {Label R1}
                             {Zip {Arity R1} R1 R2 P} ?R3}
                         end
                      end
                   toDictionary:
                      proc {$ R ?D}
                         D={Dictionary.new}
                         case {IsTuple R} then
                            {TupleToDictionary {Width R} R D}
                         else
                            {RecordToDictionary {Arity R} R D}
                         end
                      end
                  )

end
