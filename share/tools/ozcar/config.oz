%%% $Id$
%%% Benjamin Lorenz <lorenz@ps.uni-sb.de>

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% Strings
%%
Version                = '0.9 (Dec 96)'
TitleName              = 'Oz Debugger Interface'
IconName               = 'Ozcar'

SourceWindowTitle      = 'Ozcar Source Window'
SourceWindowIcon       = 'Ozcar Source'

TreeTitle              = 'Thread Tree'
StackTitle             = 'Stack'
AltStackTitle          = 'Stack of Thread  #'
EnvTitle               = 'Environment'
AltEnvTitle            = 'Environment of Frame  #'

StatusInitText         = 'No current thread'
StatusEndText          = 'See you again...'

ApplPrefixText         = 'Current application: '
ApplLabelInit          = ' none'

InvalidThreadID        = 'Invalid Thread ID in step message'
NoFileInfo             = 'step message without line number information, ' #
                         'continuing thread #'
EarlyThreadDeath       = '...hm, but it has died already?!'
KnownThread            = 'Got known thread'
NewThread              = 'Got new thread'
ID                     = fun {$ I} ' (id ' # I # ')' end

OzcarMessagePrefix     = 'Ozcar: '

/*                       Just for you, you stupid emacs!
end end end
*/

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% Geometry
%%
ToplevelGeometry       = '600x420+3+100'
SourceWindowGeometry   = '501x549+620+100'  %% I really hate hardcoding this
                                            %% but window managers seem
                                            %% to be f*cking stupid :-((
SourceWindowTextSize   = 80 # 50

ThreadTreeWidth        = 120
ThreadTreeStretchX     = 11
ThreadTreeStretchY     = 13
ThreadTreeOffset       = 4

TextWidth              = 50

SmallBorderSize        = 0
BorderSize             = 2

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% Fonts
%%
SmallFont              = '6x10'
DefaultFont            = '7x13'
BoldFont               = '7x13bold'
ThreadTreeFont         = fixed
ThreadTreeBoldFont     = '6x13bold'
ButtonFont             = '-adobe-helvetica-medium-r-normal-*-10-*-*-*-*-*-*-*'
TitleFont              = '-adobe-helvetica-bold-r-normal-*-10-*-*-*-*-*-*-*'

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% Files
%%
BitMapDir              = {System.get home} # '/lib/bitmaps/'
BitMap                 = '@' # BitMapDir # 'debugger.xbm'

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% Miscellaneous
%%
TextCursor             = left_ptr

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% Colors and colormodel related stuff
%%
/* hi, emacs! end*/

DefaultBackground
ScrollbarApplColor
ScrollbarBlockedColor
ScrollbarStackColor
RunningThreadColor
BlockedThreadColor
DeadThreadColor
ZombieThreadColor
TrunkColor
RunningThreadText
BlockedThreadText
DeadThreadText
ProcColor
BuiltinColor

case Tk.isColor then
   %% main window
   DefaultBackground       = '#f0f0f0'

   %% source window
   ScrollbarApplColor      = '#00a000'
   ScrollbarBlockedColor   = BlockedThreadColor
   ScrollbarStackColor     = '#7070f0'

   %% thread forest window
   RunningThreadColor      = '#00b000'
   BlockedThreadColor      = '#e07070'
   DeadThreadColor         = '#505050'
   ZombieThreadColor       = '#f000f0'
   TrunkColor              = '#a00000'

   RunningThreadText       = nil
   BlockedThreadText       = nil
   DeadThreadText          = nil

   %% application trace window
   ProcColor               = '#0000c0'
   BuiltinColor            = '#c00000'
else
   %% main window
   DefaultBackground       = white

   %% source window
   ScrollbarApplColor      = black
   ScrollbarBlockedColor   = black
   ScrollbarStackColor     = black

   %% thread forest window
   RunningThreadColor      = black
   BlockedThreadColor      = black
   DeadThreadColor         = black
   ZombieThreadColor       = black
   TrunkColor              = black

   RunningThreadText       = nil
   BlockedThreadText       = '(b)'
   DeadThreadText          = '(t)'

   %% application trace window
   ProcColor               = black
   BuiltinColor            = black
end

SourceTextForeground    = black
SourceTextInvForeground = white
SourceTextBackground    = white

ScrollbarColors         = colors(appl  : ScrollbarApplColor
                                 stack : ScrollbarStackColor)


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% the config object to read/write changeable options
%% first, some initial values... (read from a config file someday?)

ConfigVerbose              = false  %% debug messages in Emulator buffer?
ConfigStepSystemProcedures = false  %% step on all system procedures (`...`)?

ConfigEnvSystemVariables   = true   %% filter system variables in Env Window?
ConfigEnvProcedures        = false  %% filter procedures in Env Window?

Config =
{New
 class

    attr
       verbose :               ConfigVerbose
       stepSystemProcedures :  ConfigStepSystemProcedures
       envSystemVariables :    ConfigEnvSystemVariables
       envProcedures :         ConfigEnvProcedures

    meth init
       skip
    end

    meth toggle(What)
       What <- {Not @What}
    end

    meth get(What $)
       @What
    end

 end init}

fun {Cget What}
   {Config get(What $)}
end

%%
%%%%%%%%%%%%%%%%%%%%%%%% End of config.oz %%%%%%%%%%%%%%%%%%%%%%
%%
