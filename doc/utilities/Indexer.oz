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
%%%   $MOZARTURL$
%%%
%%% See the file "LICENSE" or
%%%   $LICENSEURL$
%%% for information on usage and redistribution
%%% of this file, and for a DISCLAIMER OF ALL
%%% WARRANTIES.
%%%

local
   %--** Sorting should be language-dependent!
   %--** unsupported: &� &� &� &�

   ReplaceList = [[&� &� &� &� &� &�]#[&A]
		  [&�]#[&A &e]
		  [&�]#[&C]
		  [&� &� &� &�]#[&E]
		  [&� &� &� &�]#[&I]
		  [&�]#[&N]
		  [&� &� &� &� &� &�]#[&O]
		  [&� &� &� &�]#[&U]
		  [&�]#[&Y]
		  [&�]#[&s &s]
		  [&� &� &� &� &� &�]#[&a]
		  [&�]#[&a &e]
		  [&�]#[&c]
		  [&� &� &� &�]#[&e]
		  [&� &� &� &�]#[&i]
		  [&�]#[&n]
		  [&� &� &� &� &� &�]#[&o]
		  [&� &� &� &�]#[&u]
		  [&� &�]#[&y]]

   ReplaceMap = {List.toRecord replaceMap
		 {FoldR ReplaceList
		  fun {$ Cs#Replacement In}
		     {FoldR Cs fun {$ C In} C#Replacement|In end In}
		  end nil}}

   OrderList = [% Symbols
		&! &" &# &$ &% && &' &( &) &* &+ &, &- &. &/ &: &; &< &= &> &?
		&@ &[ &\\ &] &^ &_ &` &{ &| &} &~ &� &� &� &� &� &� &� &� &�
		&� &� &� &� &� &� &� &� &� &� &� &� &� &� &� &� &� &� &� &� &�
		&� &� &�

		% Numbers
		&0 &1 &2 &3 &4 &5 &6 &7 &8 &9

		% Letters
		&A &a &B &b &C &c &D &d &E &e &F &f &G &g &H &h &I &i &J &j
		&K &k &L &l &M &m &N &n &O &o &P &p &Q &q &R &r &S &s &T &t
		&U &u &V &v &W &w &X &x &Y &y &Z &z]

   OrderMap = {List.toRecord orderMap
	       {List.mapInd OrderList fun {$ I C} C#I end}}

   InverseOrderMap = {List.toTuple inverseOrderMap OrderList}
in
   functor
   import
      HTML(empty: EMPTY
	   seq: SEQ
	   pcdata: PCDATA)
   export
      'class': IndexerClass
   define
      local
	 fun {MakeSortKeySub S}
	    case S of C|Cr then
	       if {Char.isSpace C} then {MakeSortKeySub Cr}
	       else
		  case {CondSelect ReplaceMap C unit} of unit then
		     OrderMap.C|{MakeSortKeySub Cr}
		  elseof L then {MakeSortKeySub {Append L Cr}}
		  end
	       end
	    [] nil then ""
	    end
	 end
      in
	 fun {MakeSortKey VS#_}
	    {String.toAtom {MakeSortKeySub {VirtualString.toString VS}}}
	 end
      end

      fun {KeyLess Xs Ys}
	 case Xs of X|Xr then
	    case Ys of Y|Yr then X < Y orelse X == Y andthen {KeyLess Xr Yr}
	    [] nil then false
	    end
	 [] nil then true
	 end
      end

      fun {GetGroup K}
	 case {Atom.toString K.1} of C|_ then C1 in
	    C1 = InverseOrderMap.C
	    if {Char.isAlpha C1} then {Char.toUpper C1}
	    elseif {Char.isDigit C1} then &0
	    else &*
	    end
	 [] nil then &*
	 end
      end

      fun {Group Es}
	 case Es of E|Er then G Gs Rest in
	    G = {GetGroup E.1}
	    {List.takeDropWhile Es fun {$ E} {GetGroup E.1} == G end ?Gs ?Rest}
	    G#Gs|{Group Rest}
	 [] nil then nil
	 end
      end

      fun {MakeHierarchy Es}
	 case Es of E|_ then And Ys Ns Zs Singles SubItems DT in
	    _#(And|_)#_ = E
	    {List.partition Es fun {$ _#(And1|_)#_} And1 == And end ?Ys ?Ns}
	    Zs = {Map Ys fun {$ Ks#Ands#HTML} Ks#Ands.2#HTML end}
	    {List.takeDropWhile Zs fun {$ _#Ands#_} Ands == nil end
	     ?Singles ?SubItems}
	    DT = case Singles of nil then And.2
		 else
		    SEQ(And.2|PCDATA(': ')|
			{List.foldRTail Singles
			 fun {$ _#_#A|Ar In}
			    A|case Ar of _|_ then PCDATA(', ')
			      else EMPTY
			      end|In
			 end nil})
		 end
	    dl(dt(DT)
	       case SubItems of nil then EMPTY
	       else dd(SEQ({MakeHierarchy SubItems}))
	       end)|
	    {MakeHierarchy Ns}
	 [] nil then nil
	 end
      end

      class IndexerClass
	 attr Entries: unit
	 meth init()
	    Entries <- nil
	 end
	 meth enter(Ands HTML)
	    Entries <- Ands#HTML|@Entries
	 end
	 meth empty($)
	    @Entries == nil
	 end
	 meth process(?HTML)
	    thread Es SortedEs Groups in
	       Es = {Map @Entries
		     fun {$ Ands#HTML}
			{Map Ands MakeSortKey}#Ands#HTML
		     end}
	       SortedEs = {Sort Es fun {$ X Y} {KeyLess X.1 Y.1} end}
	       Groups = {Group SortedEs}
	       HTML = SEQ({Map Groups
			   fun {$ G#Es}
			      'div'(h3('class': [margin]
				       PCDATA(case G of &* then 'Symbols'
					      [] &0 then 'Numbers'
					      else [G]
					      end))
				    SEQ({MakeHierarchy Es}))
			   end})
	    end
	 end
      end
   end
end
