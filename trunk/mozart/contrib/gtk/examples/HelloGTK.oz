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
   class MyToplevel from GTK.window
      meth new
	 GTK.window, new(GTK.'WINDOW_TOPLEVEL')
	 GTK.window, setBorderWidth(10)
	 GTK.window, setTitle("Hello GTK")
	 {self signalConnect('delete-event' deleteEvent _)}
      end
      meth deleteEvent(Args)
	 %% CAUTION: At this time, the underlying objects has been destroyed.
	 %% CAUTION: This event is solely intended for oz side cleanup code.
	 %% CAUTION: If you want eager finalisation of object wrappers then
	 %% CAUTION: connect the delete event handler using a procedure
	 %% CAUTION: rather than a object method.
	 {System.show 'deleteEvent occured'}
	 {Application.exit 0}
      end
   end

   %% Create Button class
   class MyButton from GTK.button
      meth new
	 GTK.button, newWithLabel("Hello, GTK!")
	 GTK.button, signalConnect('clicked' clickedEvent _)
      end
      meth clickedEvent(Args)
	 {System.show 'ClickedEvent occured'}
      end
   end

   Toplevel = {New MyToplevel new}
   Button   = {New MyButton new}

   %% Make Butten child of Toplevel
   {Toplevel add(Button)}
   %% Make it all visible
   {Toplevel showAll}
end
