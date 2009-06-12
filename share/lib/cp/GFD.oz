%%%
%%% Authors:
%%%     Gustavo Gutierrez <ggutierrez@cic.puj.edu.co>
%%%     Alberto Delgado <adelgado@cic.puj.edu.co>
%%%     Alejandro Arbelaez <aarbelaez@puj.edu.co>
%%%
%%%  Contributors:
%%%     Andres Felipe Barco <anfelbar@univalle.edu.co>
%%%     Gustavo A. Gomez Farhat <gafarhat@univalle.edu.co>
%%%	Victor Rivera Zuniga <varivera@javerianacali.edu.co>
%%%
%%% Copyright:
%%%     Gustavo Gutierrez, 2006
%%%     Alberto Delgado, 2006
%%%     Alejandro Arbelaez, 2006
%%%
%%% Last change:
%%%   $Date: 2006-10-19T01:44:35.108050Z $ by $Author: ggutierrez $
%%%   $Revision: 2 $
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

functor
   
require
   CpSupport(vectorToType:   VectorToType
	     vectorToList:   VectorToList
	     vectorToTuple:  VectorToTuple
	    )
   
import
   GFDB at 'x-oz://boot/GFDB'
   GFDP at 'x-oz://boot/GFDP'
   Space
   System(nbSusps show)
   
prepare
   %%This record must reflect the IntRelType in gecode/int.hh
   Rt = '#'('=:':0     %Equality
	    '\\=:':1    %Disequality
	    '=<:':2    %Less or equal
	    '<:':3     %Less
	    '>=:':4    %Greater or equal
	    '>:':5     %Greater
	   )
   %%This record must reflect the IntConLevel
   Cl = '#'(
	   val: 0   %Value consistency
	   bnd: 1   %Bounds consistency
	   dom: 2   %Domain consistency
	   def: 3   %The default consistency for a constraint
	   )
   IA = '#'(
	   min: 0  %Select smallest value.
	   med: 1  %Select median value.
	   max: 2  %Select maximum value.
	   )

   Pk = '#'(
	   def:   0  %Make a default decision
	   speed: 1  %Prefer speed over memory consumption
	   memory:2  %Prefer little memory over speed
	   )
  
   
export
   %% Telling domains
   int   : FdInt
   dom   : FdDom
   decl  : FdDecl
   list  : FdList
   tuple : FdTuple
   record: FdRecord
   
   %% Reflection
   reflect : FdReflect
   
   %% Watching Domains
   watch   : FdWatch
   
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%% Clasify!!!!%%%%%%%%%%%%%
   
   %% Propagators
   int_sortedness:       Int_sortedness
   
   %%Mozart Propagators (backward compatibility))
   int_sumCN:            Int_sumCN
     %int_reified_int:      Int_reified_int
   
   
   %%Miscellaneous propagators
   plus:            Plus
   plusD:           PlusD
   minus:           Minus
   minusD:          MinusD
   times:           Times
   timesD:          TimesD
   power:	    Power
   divI:            DivI
   modI:	    ModI
   divD:            DivI2
   modD:            ModI2
   distance:	    DistanceI
   less:            Less
   lessEq:          LessEq
   greater:         Greater
   greaterEq:       GreaterEq
   disjoint:        Disjoint 
   disjointC:       DisjointC
   sqrt:            Sqrt
   divmod:          DivMod
   max:             FdMaximum
   min:             FdMinimum
   
   
   %%Generic Propagators
   sum:             Sum
   sumC:            SumC
   sumCN:           SumCN
   sumD:            SumD
   sumCD:           SumCD
   sumAC:           SumAC
   sumACN:          SumACN
   
   %%Reified Propagators
   reified:        Reified
   
   %%Symbolic propagators
   distinct:        Distinct
   distinctB:       DistinctB
   distinctD:       DistinctD
   distinctOffset:  DistinctOffset
   distinctP: DistinctP
   atMost:          AtMost
   atLeast:         AtLeast
   exactly:         Exactly
   %lex:             Lex
   
   %% Assignment propagators
   assign: Assign
   
   int_ext: Int_ext
   %%Propagators
   %Abs
   sortedness: Int_sortedness
   
   %% Propagators Builtins
   
   domP: Dom
   relP: RelP
   element: Element
   channel: Channel
   circuit: Circuit
   sortedP: Sorted
   extensionalP: Extensional
   cumulatives: Cumulatives
   minP: MinP
   maxP: MaxP
   abs: Abs
   multP: MultP
   linearP: LinearP
   countP: CountP
   
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
   
   %% Distribution
   distribute : FdDistribute
   %distributeC : FdDistributeC
   distributeBR : FdDistributeBR
   %% Miscelaneus
   inf : FdInf
   sup : FdSup
   is  : FdIs
   
   %% Relation types
   rt: Rt
   
   %% Consistency levels
   cl: Cl
   
   %% Propagation kind
   pk: Pk
   
   %% Integer assignment
   ia: IA
   
define
   
   %% Telling domains
   FdInt = GFDB.int
   local
      
      proc {ListDom Xs Dom}
	 case Xs of nil then skip
	 [] X|Xr then {FdInt Dom X} {ListDom Xr Dom}
	 end
      end
      
      proc {TupleDom N T Dom}
	 if N>0 then {FdInt Dom T.N} {TupleDom N-1 T Dom} end
      end
      
      proc {RecordDom As R Dom}
	 case As of nil then skip
	 [] A|Ar then {FdInt Dom R.A} {RecordDom Ar R Dom}
	 end
      end
      
   in
      fun {FdDecl}
	 {FdInt {GFDB.inf}#{GFDB.sup}}
      end
      
      proc {FdDom Dom Vec}
	 case {VectorToType Vec}
	 of list   then {ListDom Vec Dom}
	 [] tuple  then {TupleDom {Width Vec} Vec Dom}
	 [] record then {RecordDom {Arity Vec} Vec Dom}
	 end
      end
      
      fun {FdList N Dom}
	 if N>0 then {FdInt Dom}|{FdList N-1 Dom}
	 else nil
	 end
      end
      
      proc {FdTuple L N Dom ?T}
	 T={MakeTuple L N} {TupleDom N T Dom}
      end
      
      proc {FdRecord L As Dom ?R}
	 R={MakeRecord L As} {RecordDom As R Dom}
      end
   end
   
   %% Reflection
   local
      fun {NBSusps X}
	 {GFDB.'reflect.nbProp' X} + {System.nbSusps X}
      end
   in
      FdReflect = reflect(min    : GFDB.'reflect.min'
			  max    : GFDB.'reflect.max'
			  size   : GFDB.'reflect.size'  %% cardinality
			  dom    : GFDB.'reflect.dom'
			  domList: GFDB.'reflect.domList'
			  nextSmaller : GFDB.'reflect.nextSmaller'
			  nextLarger : GFDB.'reflect.nextLarger'
			  med    : GFDB.'reflect.med'   %% median of domain
			  %% distance between maximum and minimum
			  width  : GFDB.'reflect.width' 
			  %% regret of domain minimum (distance to next larger value).
			  regretMin : GFDB.'reflect.regretMin'
			  %% regret of domain maximum (distance to next smaller value). 
			  regretMax : GFDB.'reflect.regretMax'
			  %% number of propagators associated with the variable
			  nbProp: GFDB.'reflect.nbProp'
			  %% number of suspendables associated with the variables
			  nbSusps: NBSusps 
			 )
   end
   %% Watching variables
   %% Not tested yet!!
   local
      fun{WatchDomain P}
	 proc{$ D1 D2 B} BTmp in
	    BTmp = {FdInt 0#1}
	    {Wait D2}
	    {P D1 BTmp D2}
	    {Wait BTmp}
	    if BTmp == 1 then B = true else B = false end
	 end
      end
      WatchMin = {WatchDomain GFDP.'watch.max'}
      WatchMax = {WatchDomain GFDP.'watch.min'}
      WatchSize = {WatchDomain GFDP.'watch.size'}
   in
      FdWatch = watch(size: WatchSize 
		      min : WatchMin
		      max : WatchMax)
   end
   
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%% Clasify!!!!%%%%%%%%%%%%%
   Int_sortedness = GFDP.'int_sortedness'
   
   %%Mozart Propagators (backward compatibility))
   
   Int_sumCN =   GFDP.int_sumCN
   Disjoint = GFDP.int_disjoint
      
   %% Assignment propagators
   
   proc {Assign Spec V}
      {Wait {GFDP.assign Spec V}}
   end
   
   Int_ext = GFDP.'int_ext'
   %% Backward compatibility propagators
   
   %% Propagators Builtins      
   proc {Dom S}
      Sc = {Adjoin '#'(cl:Cl.def) S}
      W = {Record.width Sc}
   in
      case W
      of 4 then
	 {GFDP.gfd_dom_5 Sc.1 Sc.2 Sc.3 Sc.cl}
      [] 3 then
	 {GFDP.gfd_dom_4 Sc.1 Sc.2 Sc.cl}
      [] 5 then
	 {GFDP.gfd_dom_6 Sc.1 Sc.2 Sc.3 Sc.4 Sc.cl}
      else
	 raise malformed('Domain constraint post') end
      end
   end

%%%
   
   proc {RelP S}
      Sc = {Adjoin '#'(cl:Cl.def) S}
      W = {Record.width Sc}
   in
      %% Assert 2 is a GFD.rt.*
      case W
      of 4 then
	 if {Value.hasFeature Rt Sc.2} then
	    {GFDP.gfd_rel_5 Sc.1 Rt.(Sc.2) Sc.3 Sc.cl}
	 else
	    {GFDP.gfd_rel_5 Sc.1 Sc.2 Sc.3 Sc.cl}
	 end
      []  5 then
	 {GFDP.gfd_rel_6 Sc.1 Rt.(Sc.2) Sc.3 Sc.4 Sc.cl}
      [] 3 then
	 {GFDP.gfd_rel_4 Sc.1 Rt.(Sc.2) Sc.cl}
      else
	 raise malformed('Rel post') end
      end
   end
   
   proc {Element S}
      Sc = {Adjoin '#'(cl:Cl.def) S}
      W = {Record.width Sc}
   in
      case W
      of 4 then
	 {GFDP.gfd_element_5 Sc.1 Sc.2 Sc.3 Sc.cl}
      else
	 raise malformed('Element constraint post') end
      end
   end
   
   proc {Channel S}
      Sc = {Adjoin '#'(cl:Cl.def) S}
      W = {Record.width Sc}
   in
      case W
      of 3 then
	 {GFDP.gfd_channel_4 Sc.1 Sc.2 Sc.cl}
      [] 4 then
	 % TODO: Sc.3 can be 0 by default.
	 {GFDP.gfd_channel_5 Sc.1 Sc.2 Sc.3 Sc.cl}
      else
	 raise malformed('Channel constraint post') end
      end
   end
   
   proc {Circuit S}
      Sc = {Adjoin '#'(cl:Cl.def) S}
      W = {Record.width Sc}
   in
      case W
      of 2 then
	 {GFDP.gfd_circuit_3 Sc.1 Sc.cl}
      else
	 raise malformed('Circuit constraint post') end
      end
   end
   
   proc {Sorted S}
      Sc = {Adjoin '#'(cl:Cl.def) S}
      W = {Record.width Sc}
   in
      case W
      of 3 then
	 {GFDP.gfd_sorted_4 Sc.1 Sc.2 Sc.cl}
      [] 4 then
	 {GFDP.gfd_sorted_5 Sc.1 Sc.2 Sc.3 Sc.cl}
      else
	 raise malformed('Sorted constraint post') end
      end
   end
   
   proc {Extensional S}
      Sc = {Adjoin '#'(cl:Cl.def pk:Pk.def) S}
      W = {Record.width Sc}
   in
      case W
      of 4 then
	 {GFDP.gfd_extensional_4 Sc.1 Sc.2 Sc.cl Sc.pk}
      else
	 raise malformed('Extensional constraint post') end
      end
   end
   
   proc {Cumulatives S}
      Sc = {Adjoin '#'(cl:Cl.def) S}
      W = {Record.width Sc}
   in
      case W
      of 8 then
	 {GFDP.gfd_cumulatives_9 Sc.1 Sc.2 Sc.3 Sc.4 Sc.5 Sc.6 Sc.7 Sc.cl}
      else
	 raise malformed('Cumulatives constraint post') end
      end
   end
   
   proc {MinP S}
      Sc = {Adjoin '#'(cl:Cl.def) S}
      W = {Record.width Sc}
   in
      case W
      of 3 then
	 {GFDP.gfd_min_4 Sc.1 Sc.2 Sc.cl}
      [] 4 then
	 {GFDP.gfd_min_5 Sc.1 Sc.2 Sc.3 Sc.cl}
      else
	 raise malformed('Min constraint post') end
      end
   end
   
   
   proc {MaxP S}
      Sc = {Adjoin '#'(cl:Cl.def) S}
      W = {Record.width Sc}
   in
      case W
      of 3 then
	 {GFDP.gfd_max_4 Sc.1 Sc.2 Sc.cl}
      [] 4 then
	 {GFDP.gfd_max_5 Sc.1 Sc.2 Sc.3 Sc.cl}
      else
	 raise malformed('Max constraint post') end
      end
   end
   
   proc {MultP S}
      Sc = {Adjoin '#'(cl:Cl.def) S}
      W = {Record.width Sc}
   in
      case W
      of 4 then
	 {GFDP.gfd_mult_5 Sc.1 Sc.2 Sc.3 Sc.cl}
      else
	 raise malformed('Mult constraint post') end
      end
   end
   
   proc {Abs S}
      Sc = {Adjoin '#'(cl:Cl.def) S}
      W = {Record.width Sc}
   in
      case W
      of 3 then
	 {GFDP.gfd_abs_4 Sc.1 Sc.2 Sc.cl}
      else
	 raise malformed('Abs constraint post') end
      end
   end
   
   proc {LinearP S}
      Sc = {Adjoin '#'(cl:Cl.def) S}
      W = {Record.width Sc}
   in
      case W
      of 4 then
	 {GFDP.gfd_linear_5 Sc.1 Rt.(Sc.2) Sc.3 Sc.cl}
      [] 5 then
	 if {IsAtom Sc.3} andthen {Value.hasFeature Rt Sc.3} then
	    {GFDP.gfd_linear_6 Sc.1 Sc.2 Rt.(Sc.3) Sc.4 Sc.cl}
	 elseif {IsAtom Sc.2} andthen {Value.hasFeature Rt Sc.2} then
	    {GFDP.gfd_linear_6 Sc.1 Rt.(Sc.2) Sc.3 Sc.4 Sc.cl}
	 else
	    {GFDP.gfd_linear_6 Sc.1 Sc.2 Sc.3 Sc.4 Sc.cl}
	 end
      [] 6 then
	 {GFDP.gfd_linear_7 Sc.1 Sc.2 Rt.(Sc.3) Sc.4 Sc.5 Sc.cl}
      else
	 raise malformed('Linear constraint post') end
      end
   end
   
   proc {Distinct Vec}
      {GFDP.gfd_distinct_3 Vec Cl.val}
   end

   proc {DistinctB Vec}
      {GFDP.gfd_distinct_3 Vec Cl.bnd}
   end

   proc {DistinctD Vec}
      {GFDP.gfd_distinct_3 Vec Cl.dom}
   end

   proc {DistinctOffset S}
      Sc = {Adjoin '#'(cl:Cl.def) S}
      W = {Record.width Sc}
   in
      case W
      of 3 then
	 % Notice that in Mozart the integer list is the second argument, but in
	 % Gecode it is the first one
	 {GFDP.gfd_distinct_4 Sc.2 Sc.1 Sc.cl}
      else
	 raise malformed('DistinctOffset constraint post') end
      end
   end
   
   proc {DistinctP S}
      Sc = {Adjoin '#'(cl:Cl.def) S}
      W = {Record.width Sc}
   in
      case W
      of 2 then
	 {GFDP.gfd_distinct_3 Sc.1 Sc.cl}
      [] 3 then
	 {GFDP.gfd_distinct_4 Sc.2 Sc.1 Sc.cl}
      else
	 raise malformed('Distinct constraint post') end
      end
   end
     
   proc {CountP S}
      Sc = {Adjoin '#'(cl:Cl.def) S}
      W = {Record.width Sc}
   in
      case W
      of 3 then
	 {GFDP.gfd_count_4 Sc.1 Sc.2 Sc.cl}
      [] 4 then
	 {GFDP.gfd_count_5 Sc.1 Sc.2 Sc.3 Sc.cl}
      [] 5 then
      % Post propagator count_6 support domain-consistency only.
	 if {Value.hasFeature Rt Sc.3} then
	    {GFDP.gfd_count_6 Sc.1 Sc.2 Rt.(Sc.3) Sc.4 Cl.dom}
	 else
	    {GFDP.gfd_count_6 Sc.1 Sc.2 Sc.3 Sc.4 Cl.dom}
	 end
      else
	 raise malformed('Count constraint post') end
      end
   end
   
   \insert GeMozProp.oz
   
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
   
   %% Miscelaneus
	 
   proc {DivP S}
      Sc = {Adjoin '#'(cl:Cl.def) S}
      W = {Record.width Sc}
   in
      case W
      of 4 then
	 {GFDP.gfd_div_5 Sc.1 Sc.2 Sc.3 Sc.cl}
      else
	 raise malformed('Mult constraint post') end
      end
   end

   proc {DivMod S}
      Sc = {Adjoin '#'(cl:Cl.def) S}
      W = {Record.width Sc}
   in
      case W
      of 5 then
	 {GFDP.gfd_divmod_6 Sc.1 Sc.2 Sc.3 Sc.4 Sc.cl}
      else
	 raise malformed('divmod constraint post') end
      end
   end

   proc {Sqrt S}
      Sc = {Adjoin '#'(cl:Cl.def) S}
      W = {Record.width Sc}
   in
      case W
      of 3 then
	 {GFDP.gfd_sqrt_4 Sc.1 Sc.2 Sc.cl}
      else
	 raise malformed('Sqrt constraint post') end
      end
   end
   
   proc {ModP S}
      Sc = {Adjoin '#'(cl:Cl.def) S}
      W = {Record.width Sc}
   in
      case W
      of 4 then
	 {GFDP.gfd_mod_5 Sc.1 Sc.2 Sc.3 Sc.cl}
      else
	 raise malformed('Mod constraint post') end
      end
   end
   
   
   proc {PowerP S}
      Sc = {Adjoin '#'(cl:Cl.def) S}
      W = {Record.width Sc}
   in
      case W
      of 4 then
	 {GFDP.gfd_power_5 Sc.1 Sc.2 Sc.3 Sc.cl}
      else
	 raise malformed('Power constraint post') end
      end
   end

   proc {FdMaximum V1 V2 V3}
      {GFDP.gfd_max_5 V1 V2 V3 Cl.def}
   end

   proc {FdMinimum V1 V2 V3}
      {GFDP.gfd_min_5 V1 V2 V3 Cl.def}
   end

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
   
   %% Reified constraint not supported by Gecode
   
   proc {SumACP S}
      Sc = {Adjoin '#'(cl:Cl.def) S}
      W = {Record.width Sc}
   in
      case W
	 of 6 then
	 {GFDP.reified_sumAC Sc.1 Sc.2 Sc.3 Sc.4 Sc.5 Sc.cl}
      else
	 raise malformed('sumAC constraint post') end
      end
   end
   
   proc {SumCNP S}
      Sc = {Adjoin '#'(cl:Cl.def) S}
      W = {Record.width Sc}
   in
      case W
      of 6 then
	 {GFDP.reified_sumCN Sc.1 Sc.2 Sc.3 Sc.4 Sc.5 Sc.cl}
      else
	 raise malformed('sumCN constraint post') end
      end
   end

   proc {SumACNP S}
      Sc = {Adjoin '#'(cl:Cl.def) S}
      W = {Record.width Sc}
   in
      case W
      of 6 then
	 {GFDP.reified_sumACN Sc.1 Sc.2 Sc.3 Sc.4 Sc.5 Sc.cl}
      else
	 raise malformed('sumCN constraint post') end
      end
   end
      
   FdInf = {GFDB.inf}
   FdSup = {GFDB.sup}
   FdIs  = GFDB.is
   
   %FdDistributeC = GFDP.distribute
   local
      \insert GeIntVarDist
   in
      FdDistribute = IntVarDistribute
   end
   local
      \insert GeIntVarDistBR
   in
      FdDistributeBR = GFDDistribute
   end
end