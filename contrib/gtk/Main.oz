%%%
%%% Author:
%%%   Thorsten Brunklaus <bruni@ps.uni-sb.de>
%%%
%%% Copyright:
%%%   Thorsten Brunklaus, 2001
%%%
%%% Last Change:
%%%   $Date$ by $Author$
%%%   $Revision$
%%%
%%% This file is part of Mozart, an implementation of Oz 3:
%%%   http://www.mozart-oz.org
%%%
%%% See the file "LICENSE" or
%%%   http://www.mozart-oz.org/LICENSE.html
%%% for information on usage and redistribution
%%% of this file, and for a DISCLAIMER OF ALL
%%% WARRANTIES.
%%%

functor $
import
   Application
   Module
   System
   Property
   Parser  at 'parser.so{native}'
   Prepare at 'Prepare.ozf'
   Flatten at 'Flatten.ozf'
   Collect at 'Collect.ozf'
   ToolKit at 'ToolKit.ozf'
define
   %%
   %% Cleanup preprocessed data
   %%
   TreeSource = "gtkraw.c"
   PrepTree   = "gtkheader.c"
   {Prepare.'prepare' TreeSource PrepTree}
   %%
   %% Parse Tree and create Binding
   %%
   case {Parser.parse PrepTree}
   of 'parse error'(...) then
      {System.show 'parse error'}
   [] ParseTree then
      FlatTree  = {Record.toList {Flatten.flatten ParseTree}}
      Args      = {Application.getArgs plain}
      DoNative
      ArgTail   = case Args
                  of ["--no-native" _] then DoNative = false Args.2
                  else DoNative = true Args
                  end
      [Wrapper] = {Module.link ArgTail}
   in
      %% 4 Phases
      %% 1. Create Native Functors containing the functions
      %% 2. Create Oz/Alice Wrap Functors for functions
      %%    This phase produces the class hierarchy information
      %%    and makes it persistent
      %% 3. Create Native Functors containing the field accessors
      %%    This phase needs the the hierarchy data as input
      %% 4. create Oz/Alice Wrap Functors for fields
      try
         TypeDict = {Collect.collect FlatTree}
      in
         {System.show 'Processing Native Functions...'}
         if DoNative then {ToolKit.createFuncs TypeDict} end
         {System.show 'Processing Function Wrapper...'}
         {Wrapper.createFuncs TypeDict}
         {System.show 'Processing Native Field Accessors...'}
         if DoNative then {ToolKit.createFields TypeDict} end
         {System.show 'Processing Field Accessor Wrapper...'}
         {Wrapper.createFields TypeDict}
         {Application.exit 0}
      catch E then
         {Property.put 'print.width' 10000}
         {Property.put 'print.depth' 10000}
         {System.show 'format exception: '#E}
      end
   end
end
