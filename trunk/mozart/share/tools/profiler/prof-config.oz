%%% $Id$
%%% Benjamin Lorenz <lorenz@ps.uni-sb.de>


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% some helpers

S2A = String.toAtom  %% string to atom
fun {VS2A X}         %% virtual string to atom
   {S2A {VirtualString.toString X}}
end


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% Text
%%

Version                = 'Aug 19 1997'
TitleName              = 'Oz Profiler'
IconName               = 'Profiler'

Platform               = local
			    X#Y = {System.get platform}
			 in
			    {VS2A X#'-'#Y}
			 end
WindowsPlatform        = 'win32-i486'

NameOfRalf             = 'Ralf Scheidhauer'
NameOfBenni            = 'Benjamin Lorenz'

EmailOfBoth            = '{scheidhr,lorenz}@ps.uni-sb.de'
EmailOfBenni           = 'lorenz@ps.uni-sb.de'

BarCanvasTitle         = 'Procedures'
ProcTextTitle          = 'Proc Info'
SumTextTitle           = 'Summary'

UpdateButtonText       = ' update'
ResetButtonText        = ' reset'
SortButtonText         = 'Sort By:'

ProfilerMessagePrefix  = fun {$}
                            'Profiler[' # {Thread.id {Thread.this}} # ']: '
                         end
ProfilerErrorPrefix    = 'Profiler ERROR: '

DotEnd                 = '.end'


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% Geometry
%%
ToplevelGeometry       = '450x290'

BarCanvasWidth         = 300
ProcTextWidth          = 24
ProcTextHeight         = 7
SumTextWidth           = 24
SumTextHeight          = 5

NoBorderSize           = 0
SmallBorderSize        = 1
BorderSize             = 2

PadXButton             = 5
PadYButton             = 3

ScrollbarWidth         = 10


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% Fonts
%%

SmallFont
SmallBoldFont
DefaultFont
BoldFont

case Platform == WindowsPlatform then
   SmallFont           = '-*-courier-medium-r-*-*-12-*-*-*-*-*-*-*'
   SmallBoldFont       = '-*-courier-bold-r-*-*-12-*-*-*-*-*-*-*'
   DefaultFont         = SmallFont
   BoldFont            = SmallBoldFont
else
   SmallFont           = '6x13'
   SmallBoldFont       = '6x13bold'
   DefaultFont         = '7x13'
   BoldFont            = '7x13bold'
end

ButtonFont             = '-adobe-helvetica-medium-r-normal-*-10-*-*-*-*-*-*-*'
TitleFont              = '-adobe-helvetica-bold-r-normal-*-10-*-*-*-*-*-*-*'
StatusFont             = TitleFont
HelpTitleFont          = '-adobe-helvetica-bold-r-*-*-18-*-*-*-*-*-*-*'
HelpFont               = '-adobe-helvetica-medium-r-*-*-12-*-*-*-*-*-*-*'


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% Files
%%

HomeDir                = {VS2A {OS.getEnv 'HOME'} # '/'}

OzRawPath              = {OS.getEnv 'OZPATH'}
FieldSeparator         = case Platform == WindowsPlatform then &; else &: end
OzPath

local
   fun {PathList RawPath} % RawPath must be of type string
      H T P in
      {List.takeDropWhile RawPath fun {$ C} C \= FieldSeparator end H T}
      P = {VS2A H#'/'}
      case T == nil then P|nil
      else P|{PathList T.2}
      end
   end
in
   OzPath = {PathList OzRawPath}
end

BitMapDir              = {System.get home} # '/lib/bitmaps/'
BitMap                 = '@' # BitMapDir # 'debugger.xbm'


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% Miscellaneous
%%

TextCursor             = left_ptr

HelpEvent              = '<3>'

PrintDepth             = 2  % for System.valueToVirtualString
PrintWidth             = 3

TimeoutToStatus        = 210

UpdateTimes            = [0     # 'never'
			  2000  # '2s'
			  5000  # '5s'
			  10000 # '10s'
			  30000 # '30s']


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% Colors and colormodel related stuff
%%

DefaultBackground
DefaultForeground
SelectedBackground
SelectedForeground

case Tk.isColor andthen Platform \= WindowsPlatform then
   DefaultBackground       = '#f0f0f0'
   DefaultForeground       = black
   SelectedBackground      = '#7070c0'
   SelectedForeground      = white
else
   DefaultBackground       = white
   DefaultForeground       = black
   SelectedBackground      = black
   SelectedForeground      = white
end


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% the config object to read/write changeable options
%% first, some initial values... (read from a config file someday?)

ConfigEmacs     = {Not {System.get standalone}} % should we use Emacs?
ConfigUpdate    = UpdateTimes.1.1  % Automatic update interval
ConfigThreshold = t(calls:2 closures:2 samples:2 heap:256)

local

   Config =
   {New
    class
       
       attr
	  emacs     : ConfigEmacs
	  update    : ConfigUpdate
	  threshold : ConfigThreshold
	  
       meth init
	  skip
       end
       
       meth toggle(What)
	  What <- {Not @What}
       end
       
       meth set(What Value)
	  What <- Value
       end
       
       meth get(What $)
	  @What
       end
       
    end init}
   
in
   
   proc {Ctoggle What}
      {Config toggle(What)}
   end
   
   proc {Cset What Value}
      {Config set(What Value)}
   end
   
   fun {Cget What}
      {Config get(What $)}
   end
   
end

%%
%%%%%%%%%%%%%%%%%%%%%%%% End of config.oz %%%%%%%%%%%%%%%%%%%%%%
