%%% $Id$
%%% Benjamin Lorenz <lorenz@ps.uni-sb.de>

%% Until the emacs interface has been fixed, you'll find
%% lines with "funny" comments like here:
%% StatusInitText = 'No current thread' /* end */


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% Text
%%
Version                = '0.9.9 (Jan 97)'
TitleName              = 'Oz Debugger Interface'
IconName               = 'Ozcar'

SourceWindowTitle      = 'Ozcar Source Window'
SourceWindowIcon       = 'Ozcar Source'

TreeTitle              = 'Thread Tree'
StackTitle             = 'Stack'
AltStackTitle          = 'Stack of Thread  #'

LocalEnvTitle          = 'Local Environment'
GlobalEnvTitle         = 'Global Environment'
AltLocalEnvTitle       = 'Local Environment of Frame  #'
AltGlobalEnvTitle      = 'Global Environment of Frame  #'

StatusInitText         = 'No current thread' /* end */
StatusEndText          = 'See you again...'

ApplPrefixText         = 'Application:'
ApplFilePrefixText     = 'File:'

InvalidThreadID        = 'Invalid Thread ID in step message' /* end */
NoFileInfo             = 'step message without line number information, ' #
                         'continuing thread #' /* end */
NoFileBlockInfo        = ' blocks without line number information'
EarlyThreadDeath       = '...hm, but it has died already?!'
KnownThread            = 'Got known thread' /* end */
NewThread              = 'Got new thread' /* end */

UnknownSuspThread      = 'Unknown suspending thread' /* end */
UnknownWokenThread     = 'Unknown woken thread' /* end */
UnknownMessage         = 'Unknown message on stream' /* end */	
UnknownTermThread      = 'Unknown terminating thread' /* end */

ID                     = fun {$ I} ' (id ' # I # ')' end
OzcarMessagePrefix     = 'Ozcar: '
FileLineSeparator      = ' '

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% Types, Names and Atoms
%%
ArrayType              = '<array>'
ThreadType             = '<thread>'
CellType               = '<cell>'
ClassType              = '<class>' /* end */
DictionaryType         = '<dict>'
FloatType              = '<float>'
ListType               = '<list>'
UnitType               = 'unit'
NameType               = '<name>'
LockType               = '<lock>' /* end */
ObjectType             = '<object>'
PortType               = '<port>'
ProcedureType          = '<proc>' /* end */
TupleType              = '<tuple>'
RecordType             = '<record>'
ChunkType              = '<chunk>'
UnknownType            = '<???>'
UnboundType            = '_'
UnAllocatedType        = 'unalloc'

NilAtom                = "'nil'"
ConsAtom               = "'|'"
HashAtom               = "'#'"

TrueName               = 'true'
FalseName              = 'false'

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% Geometry
%%
ToplevelGeometry       = '590x420+7+50'
SourceWindowGeometry   = '496x564+612+50'   %% I really hate hardcoding this
                                            %% but window managers seem
                                            %% to be f*cking stupid :-((
SourceWindowTextSize   = 80 # 40

ThreadTreeWidth        = 0
ThreadTreeStretchX     = 11
ThreadTreeStretchY     = 14
ThreadTreeOffset       = 4

StackTextWidth         = 0
EnvTextWidth           = 24
					
SmallBorderSize        = 0
BorderSize             = 2

/* end */
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% Fonts
%%
SmallFont              = '6x13'
SmallBoldFont          = '6x13bold'
DefaultFont            = '7x13'
BoldFont               = '7x13bold'
ThreadTreeFont         = DefaultFont
ThreadTreeBoldFont     = BoldFont
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
DefaultForeground
SelectedBackground
SelectedForeground
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
   DefaultForeground       = black
   SelectedBackground      = ScrollbarStackColor
   SelectedForeground      = white
   
   %% source window
   ScrollbarApplColor      = '#00a000'
   ScrollbarBlockedColor   = BlockedThreadColor
   ScrollbarStackColor     = '#7070c0'

   %% thread forest window
   RunningThreadColor      = ScrollbarApplColor
   BlockedThreadColor      = '#e07070'
   DeadThreadColor         = '#505050'
   ZombieThreadColor       = '#f000f0'
   TrunkColor              = black % '#a00000'

   RunningThreadText       = nil
   BlockedThreadText       = nil
   DeadThreadText          = nil

   %% application trace window
   ProcColor               = '#0000c0'
   BuiltinColor            = '#c00000'
else
   %% main window
   DefaultBackground       = white
   DefaultForeground       = black
   SelectedBackground      = black
   SelectedForeground      = white

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
ConfigStepRecordBuiltin    = false  %% step on builtin 'record' ?
ConfigStepDotBuiltin       = true   %% step on builtin '.'      ?
ConfigStepWidthBuiltin     = true   %% step on builtin 'width'  ?

ConfigEnvSystemVariables   = true   %% filter system variables in Env Window?
ConfigEnvProcedures        = false  %% filter procedures in Env Window?

Config =
{New
 class
    
    attr
       verbose :               ConfigVerbose
       stepSystemProcedures :  ConfigStepSystemProcedures
       stepRecordBuiltin :     ConfigStepRecordBuiltin
       stepDotBuiltin :        ConfigStepDotBuiltin
       stepWidthBuiltin :      ConfigStepWidthBuiltin
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
