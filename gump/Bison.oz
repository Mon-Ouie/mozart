%%%
%%% Author:
%%%   Leif Kornstaedt <kornstae@ps.uni-sb.de>
%%%
%%% Copyright:
%%%   Leif Kornstaedt, 1996-1998
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

%%
%% Interface to the Bison parse table generator
%%

local
   BisonModule
   MAXSHORT = 32767
   L = {NewLock}
in
   fun {Bison NSymbols Grammar VerboseFile Rep}
      case Grammar.4 == nil then   % no rules
         {Rep error(kind: 'bison error' msg: 'empty grammar')}
      elsecase NSymbols > MAXSHORT then
         {Rep error(kind: 'bison error'
                    msg: ('too many symbols (tokens plus nonterminals);'#
                          'maximum allowed is '#MAXSHORT))}
      else
         lock L then
            case {IsFree BisonModule} then
               BisonModule = {Foreign.require 'tools/gump/ozbison.dl'
                              bison(generate: 3)}
            else skip
            end
         end
         {BisonModule.generate Grammar VerboseFile}
      end
   end
end
