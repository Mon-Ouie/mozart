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
%%%    http://mozart.ps.uni-sb.de
%%%
%%% See the file "LICENSE" or
%%%    http://mozart.ps.uni-sb.de/LICENSE.html
%%% for information on usage and redistribution
%%% of this file, and for a DISCLAIMER OF ALL
%%% WARRANTIES.
%%%

functor
import
   Open(file text)
export
   text: TextFile
   read: ReadFile
   write: WriteFile
define
   class TextFile from Open.file Open.text
      prop final
      meth readAll($)
	 case TextFile, getS($) of false then ""
	 elseof S then S#'\n'#TextFile, readAll($)
	 end
      end
   end

   proc {ReadFile File ?VS} F in
      F = {New TextFile init(name: File flags: [read])}
      {F readAll(?VS)}
      {F close()}
   end

   proc {WriteFile VS File} F in
      F = {New Open.file init(name: File flags: [write create truncate])}
      {F write(vs: VS)}
      {F close()}
   end
end
