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
   System(show)
   GTK at 'x-oz://system/gtk/GTK.ozf'
define
   %% Create Toplevel window class
   class MyToplevel from GTK.fileSelection
      attr
	 okButton     %% Ok Button
	 cancelButton %% Cancel Button
      meth new
	 OkButton     = @okButton
	 CancelButton = @cancelButton
      in
	 GTK.fileSelection, new("Select File")
	 OkButton     = {self fileSelectionGetFieldOkButton($)}
	 CancelButton = {self fileSelectionGetFieldCancelButton($)}
	 {OkButton signalConnect('clicked' proc {$ _}
					      {self handleOk}
					   end nil _)}
	 {OkButton signalConnect('clicked' proc {$ _}
					      {self handleCancel}
					   end nil _)}
      end
      meth handleOk
	 File = {self getFilename($)}
      in
	 {System.show {String.toAtom {VirtualString.toString File}}}
	 {Application.exit 0}
      end
      meth handleCancel
	 {Application.exit 0}
      end
      meth deleteEvent(Event)
	 %% Caution: At this time, the underlying GTK object
	 %% Caution: has been destroyed already
	 %% Caution: Destruction also includes all attached child objects.
	 %% Caution: This event is solely intended to do OZ side
	 %% Caution: cleanup via calling close
	 {System.show 'delete Event occured'}
	 {self close}
	 {Application.exit 0}
      end
   end

   Toplevel = {New MyToplevel new}
   %% Make it all visible
   {Toplevel showAll}
end

