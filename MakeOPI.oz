%%%
%%% Author:
%%%   Leif Kornstaedt <kornstae@ps.uni-sb.de>
%%%
%%% Contributor:
%%%   Christian Schulte <schulte@dfki.de>
%%%
%%% Copyright:
%%%   Leif Kornstaedt, 1997
%%%   Christian Schulte, 1998
%%%
%%% Last change:
%%%   $Date$ by $Author$
%%%   $Revision$
%%%
%%% This file is part of Mozart, an implementation of Oz 3:
%%%    $MOZARTURL$
%%%
%%% See the file "LICENSE" or
%%%    $LICENSEURL$
%%% for information on usage and redistribution
%%% of this file, and for a DISCLAIMER OF ALL
%%% WARRANTIES.
%%%

\ifdef LILO

{Application.syslet
 'opi'

 functor $

 import
    SP.{System          = 'System'
        Exit            = 'Exit'
        Print           = 'Print'
        Show            = 'Show'
        PutProperty     = 'PutProperty'
        GetProperty     = 'GetProperty'
        CondGetProperty = 'CondGetProperty'
        Foreign         = 'Foreign'
        Debug           = 'Debug'
        Error           = 'Error'
        Finalize        = 'Finalize'}

    OP.{OS   = 'OS'
        Open = 'Open'
        Load = 'Load'
        Save = 'Save'
        URL  = 'URL'}

    AP
    CP
    DP
    WP
    Panel
    Browser
    Explorer
    Compiler
    CompilerPanel
    Emacs
    Ozcar
    Profiler
    Gump
    GumpScanner
    GumpParser
    Misc

 export
    'StartOpi': StartOPI

 body

    ImportEnv = i('SP':            SP
                  'OP':            OP
                  'AP':            AP
                  'CP':            CP
                  'DP':            DP
                  'WP':            WP
                  'Panel':         Panel
                  'Browser':       Browser
                  'Explorer':      Explorer
                  'Compiler':      Compiler
                  'CompilerPanel': CompilerPanel
                  'Emacs':         Emacs
                  'Ozcar':         Ozcar
                  'Profiler':      Profiler
                  'Gump':          Gump
                  'GumpScanner':   GumpScanner
                  'GumpParser':    GumpParser
                  'Misc':          Misc)

 in

    proc {StartOPI _ _} OPICompiler CompilerUI Sock CompilerReadEvalLoop in
       local
          OZVERSION = {System.property.get 'oz.version'}
          DATE = {System.property.get 'oz.date'}
       in
          {System.printError
           'Mozart Engine '#OZVERSION#' of '#DATE#' playing Oz 3\n\n'}
       end
       {System.property.put 'oz.standalone' false}

       OPICompiler = {New Compiler.'Compiler'.engine init()}

       local
          Env = {Record.foldL ImportEnv Adjoin env()}
       in
          {OPICompiler enqueue(mergeEnv(Env))}
       end

       CompilerUI = {New Emacs.'Emacs'.interface init(OPICompiler)}
       {CompilerUI getSocket(?Sock)}
       {{`Builtin` setOPICompiler 1} CompilerUI}

       % Try to load some ozrc file:
       local
          HOME = {OS.getEnv 'HOME'}
       in
          case HOME == false then skip
          else
             fun {FileExists FN}
                try
                   F = {New Open.file init(name:FN)}
                in
                   {F close} true
                catch _ then false
                end
             end
             OZRC = {OS.getEnv 'OZRC'}
          in
             case OZRC \= false andthen {FileExists OZRC} then
                {OPICompiler enqueue(feedFile(OZRC))}
             elsecase {FileExists HOME#'/.oz/ozrc'} then
                {OPICompiler enqueue(feedFile(HOME#'/.oz/ozrc'))}
             elsecase {FileExists HOME#'/.ozrc'} then   % note: deprecated
                {OPICompiler enqueue(feedFile(HOME#'/.ozrc'))}
             else
                skip
             end
          end
       end

       proc {CompilerReadEvalLoop} VS0 VS in
          {Sock readQuery(?VS0)}
          VS = case VS0 of ""#'\n'#VS1 then VS1 else VS0 end
          {OPICompiler enqueue(feedVirtualString(VS))}
          {CompilerReadEvalLoop}
       end

       {CompilerReadEvalLoop}
    end
 end

 plain}


\else

{Application.syslet
 'opi'
 full('SP':            eager
      'OP':            eager
      'AP':            lazy
      'CP':            eager
      'DP':            lazy
      'WP':            lazy
      'Panel':         lazy
      'Browser':       lazy
      'Explorer':      lazy
      'Compiler':      eager
      'CompilerPanel': lazy
      'Emacs':         lazy
      'Ozcar':         lazy
      'Profiler':      lazy
      'Gump':          lazy
      'GumpScanner':   lazy
      'GumpParser':    lazy
      'Misc':          lazy)

 proc instantiate {$ IMPORT ?StartOPI}
    \insert SP.env
    = IMPORT.'SP'
    \insert OP.env
    = IMPORT.'OP'
    \insert Emacs.env
    = IMPORT.'Emacs'
    \insert Compiler.env
    = IMPORT.'Compiler'
 in
    proc {StartOPI _ _} OPICompiler CompilerUI Sock CompilerReadEvalLoop in
       local
          OZVERSION = {System.property.get 'oz.version'}
          DATE = {System.property.get 'oz.date'}
       in
          {System.printError
           'Mozart Engine '#OZVERSION#' of '#DATE#' playing Oz 3\n\n'}
       end
       {System.property.put 'oz.standalone' false}

       OPICompiler = {New Compiler.engine init()}
       local
          Env = {Record.foldL IMPORT Adjoin env()}
       in
          {OPICompiler enqueue(mergeEnv(Env))}
       end
       CompilerUI = {New Emacs.interface init(OPICompiler)}
       {CompilerUI getSocket(?Sock)}
       {{`Builtin` setOPICompiler 1} CompilerUI}

       % Try to load some ozrc file:
       local
          HOME = {OS.getEnv 'HOME'}
       in
          case HOME == false then skip
          else
             fun {FileExists FN}
                try
                   F = {New Open.file init(name:FN)}
                in
                   {F close} true
                catch _ then false
                end
             end
             OZRC = {OS.getEnv 'OZRC'}
          in
             case OZRC \= false andthen {FileExists OZRC} then
                {OPICompiler enqueue(feedFile(OZRC))}
             elsecase {FileExists HOME#'/.oz/ozrc'} then
                {OPICompiler enqueue(feedFile(HOME#'/.oz/ozrc'))}
             elsecase {FileExists HOME#'/.ozrc'} then   % note: deprecated
                {OPICompiler enqueue(feedFile(HOME#'/.ozrc'))}
             else
                skip
             end
          end
       end

       proc {CompilerReadEvalLoop} VS0 VS in
          {Sock readQuery(?VS0)}
          VS = case VS0 of ""#'\n'#VS1 then VS1 else VS0 end
          {OPICompiler enqueue(feedVirtualString(VS))}
          {CompilerReadEvalLoop}
       end

       {CompilerReadEvalLoop}
    end
 end

 plain}


\endif
