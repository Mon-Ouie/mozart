%%%
%%% Authors:
%%%   Leif Kornstaedt <kornstae@ps.uni-sb.de>
%%%   Tobias Mueller <tmueller@ps.uni-sb.de>
%%%   Christian Schulte <schulte@ps.uni-sb.de>
%%%
%%% Copyright:
%%%   Tobias Mueller and Leif Kornstaedt, 1998
%%%
%%% Last change:
%%%   $Date$ by $Author$
%%%   $Revision$
%%%
%%% This file is part of Mozart, an implementation of Oz 3:
%%%    http://www.mozart-oz.org
%%%
%%% See the file "LICENSE" or
%%%    http://www.mozart-oz.org/LICENSE.html
%%% for information on usage and redistribution
%%% of this file, and for a DISCLAIMER OF ALL
%%% WARRANTIES.
%%%

functor
import
   OS(system tmpnam unlink stat)
   File(baseName changeExtension)
export
   'class': PostScriptToGIFClass
define

   fun {Exists Name}
      try
         {OS.stat Name}=_ true
      catch _ then false
      end
   end

   fun {FilenameExplode Name}
      Prefix Suffix
   in
      {String.token {Reverse {VirtualString.toString Name}} &. Prefix Suffix}
      %%  Basename    #   Extension
      {Reverse Suffix}#{Reverse Prefix}
   end

   proc {PsToPpm PsName PpmName}
      _#Extension = {FilenameExplode PsName}
      PsCat = if Extension=="gz" then 'gzip -dc '#PsName
              elseif {Exists PsName#'.gz'} then 'gzip -dc '#PsName#'.gz'
              elseif {Exists PsName#'.eps.gz'} then 'gzip -dc '#PsName#'.eps.gz'
              elseif {Exists PsName#'.ps.gz'} then 'gzip -dc '#PsName#'.ps.gz'
              elseif {Exists PsName#'.eps'} then 'cat '#PsName#'.eps'
              elseif {Exists PsName#'.ps'} then 'cat '#PsName#'.ps'
              else 'cat '#PsName end
      Cmd   = ('('#PsCat#'; echo quit) | '#
               'gs -q -dNOPAUSE '#
               '-dTextAlphaBits=4 -dGraphicsAlphaBits=4 -r102 '#
               '-sDEVICE=ppmraw -sOutputFile='#PpmName#' - 1>&2')
   in
      case {OS.system Cmd} of 0 then skip
      elseof I then
         {Exception.raiseError ozDoc(gs PsName PpmName I)}
      end
   end

   proc {PpmToGif PpmName Info GifName} Cmd in
      Cmd  = ('pnmcrop < '#PpmName#' 2> /dev/null | '#
              if Info == '' then ''
              else 'pnmscale '#Info#' | '
              end#
              'ppmquant 256 2> /dev/null | '#
              'ppmtogif -interlace -transparent rgbi:1/1/1 2> /dev/null | '#
              'cat > '#GifName)
      case {OS.system Cmd} of 0 then skip
      elseof I then
         {Exception.raiseError ozDoc(ppmtogif {VirtualString.toAtom Cmd} I)}
      end
   end

   class PostScriptToGIFClass
      attr
         DirName: unit
         Keep:    false
      meth init(Dir KeepPictures)
         DirName <- Dir
         Keep    <- KeepPictures
      end
      meth convertPostScript(InName Info ?OutName)
         Basename#_ = {FilenameExplode InName}
         !OutName   = Basename#'.gif'
         FullName   = @DirName#'/'#OutName
      in
         if {Not @Keep andthen {Exists FullName}} then
            PpmName = {OS.tmpnam}
         in
            try
               {PsToPpm InName PpmName}
               {PpmToGif PpmName Info @DirName#'/'#OutName}
            finally
               try {OS.unlink PpmName} catch _ then skip end
            end
         end
      end
   end
end
