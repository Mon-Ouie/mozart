%  Programming Systems Lab, University of Saarland,
%  Geb. 45, Postfach 15 11 50, D-66041 Saarbruecken.
%  Author: Konstantin Popov & Co.
%  (i.e. all people who make proposals, advices and other rats at all:))
%  Last modified: $Date$ by $Author$
%  Version: $Revision$

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%
%%%
%%%   FBrowserClass - this is class which is given to users and which
%%%  instance is used for the default Oz browser;
%%%
%%%
%%%

%%%
%%%
%%% FBrowserClass;
%%%
%%% Non-local methods which are denoted by names are used primarily
%%% as event handlers in window manager (aka e.g. 'Help');
%%%
%%%
class FBrowserClass
   from MyClosableObject
   prop locking
   feat
   %%
   %% some (internal) objects;
      Store                     %  parameters store;
      BrowserBuffer             %  currently browsed terms (queue);
      BrowserStream             %  draw requests (queue);
   %%
      GetTermObjs               %  a function;

   %%
   attr
      selected: InitValue       %  selected term's object;

   %%
   %%
   %%
   meth init(origWindow:       OrigWindow         <= InitValue
             screen:           Screen             <= InitValue)
\ifdef DEBUG_BO
      {Show 'FBrowserClass::init is applied'}
\endif
      lock
         %%
         self.Store =
         {New StoreClass
          [init
           store(StoreXSize IXSize)
           store(StoreYSize IYSize)
           store(StoreXMinSize IXMinSize)
           store(StoreYMinSize IYMinSize)
           store(StoreTWWidth 0)
           store(StoreDepth IDepth)
           store(StoreWidth IWidth)
           store(StoreFillStyle IFillStyle)
           store(StoreArityType IArityType)
           store(StoreSmallNames ISmallNames)
           store(StoreAreStrings IAreStrings)
           store(StoreAreVSs IAreVSs)
           store(StoreDepthInc IDepthInc)
           store(StoreWidthInc IWidthInc)
           store(StoreAreSeparators ISeparators)
           store(StoreRepMode IRepMode)
           store(StoreTWFont ITWFontUnknown)     % first approximation;
           store(StoreBufferSize IBufferSize)
           store(StoreWithMenus true)            % hardwired;
           store(StoreIsWindow false)
           store(StoreAreMenus false)
           store(StoreBrowserObj self)
           store(StoreStreamObj self.BrowserStream)
           store(StoreOrigWindow OrigWindow)
           store(StoreScreen Screen)
           store(StoreBreak false)
           store(StoreSeqNum 0)]}

         %%
         self.BrowserBuffer = {New BrowserBufferClass init(IBufferSize)}

         %%
         %% 'ManagerObject' is not directly accessible - but it can be
         %% closed by means of queuing of 'close' message;
         local Stream ManagerObject in
            %%
            %% only 'getContent' functionality is delegated to the
            %% manager object. A list of term objects is necessary in
            %% order to perform 'check layout' step when idle;
            self.GetTermObjs = fun {$} {self.BrowserBuffer getContent($)} end

            %%
            Stream = self.BrowserStream = {New BrowserStreamClass init}

            %%
            ManagerObject =
            {New BrowserManagerClass init(store:          self.Store
                                          getTermObjsFun: self.GetTermObjs)}
         end

         %%
\ifdef DEBUG_BO
         {Show 'FBrowserClass::init is finished'}
\endif
      end
   end

   %%
   %% Terminate browsing ASAP - by means of setting of
   %% a global flag ('StoreBreak') which is respected when new
   %% term objects are created;
   %% This flag must be reset by the drawing process (sitting
   %% behind the 'BrowserStream') when a currently last entry
   %% has been processed;
   meth break
\ifdef DEBUG_BO
      {Show 'FBrowserClass::break is applied'}
\endif
      %%
      {self.Store store(StoreBreak true)}
   end

   %%
   %% Break + purge unprocessed suspensions + undraw everything;
   meth !Reset
\ifdef DEBUG_BO
      {Show 'FBrowserClass::Reset is applied'}
\endif
      lock
         %%
         FBrowserClass , break

         %%
         FBrowserClass , UnsetSelected

         %% everything pending is cancelled;
         {self.BrowserBuffer purgeSusps}

         %%  'FBrowserClass::Undraw' is an "in-thread" method;
         FBrowserClass , Undraw({self.BrowserBuffer getSize($)})

         %%
         {Wait {self.BrowserStream enq(sync($))}}

         %%
\ifdef DEBUG_BO
         {Show 'FBrowserClass::Reset is finished'}
\endif
      end
   end

   %%
   %%  ... and close the window;
   meth closeWindow
\ifdef DEBUG_BO
      {Show 'FBrowserClass::closeWindow is applied'}
\endif
      lock
         %%
         FBrowserClass , Reset
         {self.BrowserStream enq(closeWindow)}

         %%
\ifdef DEBUG_BO
         {Show 'FBrowserClass::closeWindow is finished'}
\endif
      end
   end

   %%
   meth closeMenus
\ifdef DEBUG_BO
      {Show 'FBrowserClass::closeMenus is applied'}
\endif
      %%
      {self.BrowserStream enq(closeMenus)}

      %%
\ifdef DEBUG_BO
      {Show 'FBrowserClass::closeMenus is finished'}
\endif
   end

   %%
   meth close
\ifdef DEBUG_BO
      {Show 'FBrowserClass::close is applied'}
\endif
      %%
      lock
         FBrowserClass , break

         %%
         {Wait {self.BrowserStream [enq(sync($)) enq(close)]}}

         %%
         {self.BrowserBuffer purgeSusps}

         %%
         {self.BrowserStream close}
         {self.BrowserBuffer close}
         {self.Store close}

         %%
         %% simply throw away everything else;
         %% That's not my problem if somebody will send messages here ;-)
         MyClosableObject , close
\ifdef DEBUG_BO
         {Show 'FBrowserClass::close is finished'}
\endif
      end
   end

   %%
   meth createWindow
\ifdef DEBUG_BO
      {Show 'FBrowserClass::createWindow is applied'}
\endif
      %%
      lock
         case {self.Store read(StoreIsWindow $)} then skip
         else
            {self.BrowserStream enq(createWindow)}

            %%
            case {self.Store read(StoreWithMenus $)} then
               FBrowserClass , createMenus
            else skip
            end
         end

         %%
\ifdef DEBUG_BO
         {Show 'FBrowserClass::createWindow is finished'}
\endif
      end
   end

   %%
   meth createMenus
\ifdef DEBUG_BO
      {Show 'FBrowserClass::createMenus is applied'}
\endif
      %%
      lock
         case {self.Store read(StoreAreMenus $)} then skip
         else
            {self.BrowserStream
             [enq(createMenus)
              enq(entriesDisable([unselect break rebrowse
                                  process clear clearAllButLast
                                  checkLayout expand shrink deref]))]}
         end

         %%
\ifdef DEBUG_BO
         {Show 'FBrowserClass::createMenus is finished'}
\endif
      end
   end

   %%
   meth toggleMenus
\ifdef DEBUG_BO
      {Show 'FBrowserClass::toggleMenus is applied'}
\endif
      %%
      lock
         FBrowserClass ,
         case {self.Store read(StoreAreMenus $)} then closeMenus
         else createMenus
         end
\ifdef DEBUG_BO
         {Show 'FBrowserClass::toggleMenus is finished'}
\endif
      end
   end

   %%
   meth focusIn
\ifdef DEBUG_BO
      {Show 'FBrowserClass::focusIn is applied'}
\endif
      {self.BrowserStream enq(focusIn)}

      %%
\ifdef DEBUG_BO
      {Show 'FBrowserClass::focusIn is finished'}
\endif
   end

   %%
   meth !ScrollTo(Obj Kind)
\ifdef DEBUG_BO
      {Show 'FBrowserClass::ScrollTo is applied'}
\endif
      %%
      lock
         local RootTermObj NN in
            RootTermObj = {GetRootTermObject Obj}
            NN =
            RootTermObj.seqNumber + case Kind of 'forward' then 1 else ~1 end

            %%
            case {Filter {self.GetTermObjs} fun {$ TO} TO.seqNumber == NN end}
            of [NewRootTO] then
               %%
               {self.BrowserStream enq(pick(NewRootTO 'begin' 'top'))}
            else
               %% there is none - move to the top/bottom;
               {self.BrowserStream
                enq(pick(RootTermObj
                         case Kind of 'forward' then 'end'
                      else 'begin'
                      end 'any'))}
            end
\ifdef DEBUG_BO
            {Show 'FBrowserClass::ScrollTo is finished'}
\endif
         end
      end
   end

   %%
   %%  'browse' method (+ buffer maintaining);
   %% This method may suspend the caller thread if the buffer is full
   %% (but the object state is released);
   %%
   meth browse(Term)
\ifdef DEBUG_BO
      {Show 'FBrowserClass::browse is applied'#Term}
\endif
      local RootTermObj Sync ProceedProc DiscardProc in
         lock
            %%
            FBrowserClass , createWindow    % check it;

            %%
            case
               {self.BrowserBuffer getSize($)} >=
               {self.Store read(StoreBufferSize $)}
            then
               %% Startup a thread which eventually cleans up some place
               %% in the buffer;
               thread
                  {self UndrawWait}
               end
            else skip           % just put a new one;
            end

            %%
            proc {ProceedProc}
               %%  spawns drawing work;
               {self.BrowserStream [enq(browse(Term RootTermObj))
                                    enq(entriesEnable([clear checkLayout]))]}

               %%
               Sync = unit
            end
            %%
            proc {DiscardProc}
               Sync = unit
            end

            %%
            %%  allocate a slot inside of the buffer;
            %%  RootTermObj is yet a variable;
            {self.BrowserBuffer enq(RootTermObj ProceedProc DiscardProc)}

            %%
            %% it might be a little bit too early, but it *must* be
            %% inside the "touched" region (since e.g. 'BrowserBuffer'
            %% can be closed already when it's applied);
            case {self.BrowserBuffer getSize($)} > 1 then
               {self.BrowserStream
                enq(entriesEnable([clearAllButLast checkLayout]))}
            else skip
            end

            %%
\ifdef DEBUG_BO
         {Show 'FBrowserClass::browse is finished'}
\endif
         end

         %%
         %% the object state is free;
         {Wait Sync}
      end
   end

   %%
   %% update the size of a buffer, and if necessary -
   %% wakeup suspended 'Browse' threads;
   meth !SetBufferSize(NewSize)
\ifdef DEBUG_BO
      {Show 'FBrowserClass::SetBufferSize is applied'}
\endif
      %%
      lock
         case {IsInt NewSize} andthen NewSize > 0 then CurrentSize in
            {self.Store store(StoreBufferSize NewSize)}
            CurrentSize = {self.BrowserBuffer getSize($)}

            %%
            {self.BrowserBuffer resize(NewSize)}

            %%
            case NewSize < CurrentSize then
               %%
               FBrowserClass , Undraw(CurrentSize - NewSize)
            else skip
            end
         else {BrowserError 'Illegal size of the browser buffer'}
         end

         %%
\ifdef DEBUG_BO
         {Show 'FBrowserClass::SetBufferSize is finished'}
\endif
      end
   end

   %%
   meth !ChangeBufferSize(Inc)
\ifdef DEBUG_BO
      {Show 'FBrowserClass::ChangeBufferSize is applied'}
\endif
      FBrowserClass
      , SetBufferSize({self.Store read(StoreBufferSize $)} + Inc)
   end

   %%
   %%
   meth rebrowse
\ifdef DEBUG_BO
      {Show 'FBrowserClass::rebrowse is applied'}
\endif
      %%
      lock
         case @selected == InitValue then skip
         else Obj in
            Obj = @selected

            %%
            {self.BrowserStream enq(subtermChanged(Obj.ParentObj Obj))}
            FBrowserClass , UnsetSelected
         end

         %%
\ifdef DEBUG_BO
         {Show 'FBrowserClass::rebrowse is finished'}
\endif
      end
   end

   %%
   %% clear method;
   %%
   meth clear
\ifdef DEBUG_BO
      {Show 'FBrowserClass::clear is applied'}
\endif
      %%
      lock
         local CurrentSize in
            CurrentSize = {self.BrowserBuffer getSize($)}

            %%
            FBrowserClass , Undraw(CurrentSize)

            %%
            {self.BrowserStream
             enq(entriesDisable([clear clearAllButLast checkLayout]))}
         end

         %%
\ifdef DEBUG_BO
         {Show 'FBrowserClass::clear is finished'}
\endif
      end
   end

   %%
   %%
   meth clearAllButLast
\ifdef DEBUG_BO
      {Show 'FBrowserClass::ClearAllButLast is applied'}
\endif
      lock
         local CurrentSize in
            %%
            CurrentSize = {self.BrowserBuffer getSize($)}

            %%
            case CurrentSize > 1 then
               FBrowserClass , Undraw(CurrentSize - 1)
            else skip
            end

            %%
            {self.BrowserStream enq(entriesDisable([clearAllButLast]))}

            %%
\ifdef DEBUG_BO
            {Show 'FBrowserClass::ClearAllButLast is finished'}
\endif
         end
      end
   end

   %%
   %% Undraw some terms - forking off an undrawing work if
   %% he term is not selected;
   %% (synchronous method - "in thread");
   %%
   meth Undraw(N)
\ifdef DEBUG_BO
      {Show 'FBrowserClass::Undraw is applied'}
\endif
      %%
      case N > 0 andthen {self.BrowserBuffer getSize($)} > 0 then
         RootTermObj Sync ProceedProc DiscardProc
      in
         %%
         proc {ProceedProc}
            %%  fork off an "undraw" job;
            {self.BrowserStream enq(undraw(RootTermObj))}

            %%
            Sync = unit
         end
         %%
         proc {DiscardProc}
            Sync = unit
         end

         %%
         {self.BrowserBuffer deq(RootTermObj ProceedProc DiscardProc)}

         %%
         {Wait Sync}

         lock
            %%
            %% becomes empty...
            case {self.BrowserBuffer getSize($)}
            of 0 then {self.BrowserStream
                       enq(entriesDisable([clear clearAllButLast
                                           checkLayout]))}
            [] 1 then {self.BrowserStream
                       enq(entriesDisable([clearAllButLast]))}
            else skip
            end

            %%
            %% Unselect it if it was;
            case {GetRootTermObject @selected} == RootTermObj
            then FBrowserClass , UnsetSelected
            else skip
            end

            %%
            FBrowserClass , Undraw(N-1)
         end
      else skip
      end

      %%
\ifdef DEBUG_BO
      {Show 'FBrowserClass::Undraw is finished'}
\endif
   end

   %%
   %% Undraw a term when it becomes unselected (if it was at all);
   meth UndrawWait
\ifdef DEBUG_BO
      {Show 'FBrowserClass::UndrawWait is applied'}
\endif
      %%
      lock
         case {self.BrowserBuffer getSize($)} > 0 then RootTermObj in
            %%
            %% state is free already;
            {self Undraw(1)}
         else {BrowserError 'FBrowserClass::UndrawWait: no terms??!'}
         end

         %%
\ifdef DEBUG_BO
         {Show 'FBrowserClass::UndrawWait is finished'}
\endif
      end
   end

   %%
   %%
   meth option(...)=M
\ifdef DEBUG_BO
      {Show 'FBrowserClass::option is applied'#M}
\endif
      %%
      case M.1
      of !SpecialON                        then
         %%
         %% These are options that are not accessible through
         %% menus;
         {ForAll {Filter {Arity M} fun {$ F} F \= 1 end}
          proc {$ F}
             case F
             of !BrowserXSize                  then
                case {IsInt M.F} andthen M.F > 1 then
                   {self.Store store(StoreXSize M.F)}
                   {self.BrowserStream enq(resetWindowSize)}
                else {BrowserError 'Illegal value for browser\'s "xSize"'}
                end

             [] !BrowserYSize                  then
                case {IsInt M.F} andthen M.F > 1 then
                   {self.Store store(StoreYSize M.F)}
                   {self.BrowserStream enq(resetWindowSize)}
                else
                   {BrowserError 'Illegal value for browser\'s "ySize"'}
                end

             [] !BrowserXMinSize               then
                case {IsInt M.F} andthen M.F >= IXMinSize then
                   {self.Store store(StoreXMinSize M.F)}
                   {self.BrowserStream enq(resetWindowSize)}
                else {BrowserError
                      'Illegal value for browser\'s "xMinSize"'}
                end

             [] !BrowserYMinSize               then
                case {IsInt M.F} andthen M.F > IYMinSize then
                   {self.Store store(StoreYMinSize M.F)}
                   {self.BrowserStream enq(resetWindowSize)}
                else {BrowserError
                      'Illegal value for browser\'s "yMinSize"'}
                end

             else
                {BrowserError 'Unknown "special" option: ' #
                 {String.toAtom {System.valueToVirtualString F 0 0}}}
             end
          end}

         %%
      [] !BufferON                         then
         %%
         {ForAll {Filter {Arity M} fun {$ F} F \= 1 end}
          proc {$ F}
             case F
             of !BrowserBufferSize             then
                {self SetBufferSize(M.F)}

             [] !BrowserSeparators             then
                case M.F of true then
                   %%
                   {self.Store store(StoreAreSeparators true)}
                elseof false then
                   %%
                   {self.Store store(StoreAreSeparators false)}
                else {BrowserError
                      'Illegal value of browser\'s "separators" option'}
                end

             else {BrowserError 'Unknown "buffer" option: ' #
                   {String.toAtom {System.valueToVirtualString F 0 0}}}
             end
          end}

      [] !RepresentationON                 then
         %%
         {ForAll {Filter {Arity M} fun {$ F} F \= 1 end}
          proc {$ F}
             case F
             of !BrowserRepMode                then
                {self.Store
                 store(StoreRepMode
                       case M.F
                       of tree     then TreeRep
                       [] graph    then GraphRep
                       [] minGraph then MinGraphRep
                       else
                          {BrowserError
                           'Illegal value of browser\'s (representation) mode'}
                          {self.Store read(StoreRepMode $)}
                       end)}

             [] !BrowserChunkFields            then
                case M.F of true then
                   %%
                   {self.Store store(StoreArityType TrueArity)}
                elseof false then
                   %%
                   {self.Store store(StoreArityType AtomicArity)}
                else
                   {BrowserError
                    'Illegal value of browser\'s "privateChunkFields" option'}
                end

             [] !BrowserNamesAndProcs          then
                case M.F of false then
                   %%
                   {self.Store store(StoreSmallNames true)}
                elseof true then
                   %%
                   {self.Store store(StoreSmallNames false)}
                else
                   {BrowserError
                    'Illegal value of parameter browser\'s "detailedNamesAndProcedurs" option'}
                end

             [] !BrowserStrings                then
                case M.F of true then
                   %%
                   {self.Store store(StoreAreStrings true)}
                elseof false then
                   %%
                   {self.Store store(StoreAreStrings false)}
                else
                   {BrowserError
                    'Illegal value of parameter BrowserStrings'}
                end

             [] !BrowserVirtualStrings         then
                case M.F of true then
                   %%
                   {self.Store store(StoreAreVSs true)}
                elseof false then
                   %%
                   {self.Store store(StoreAreVSs false)}
                else
                   {BrowserError
                    'Illegal value of parameter BrowserVirtualStrings'}
                end

             else {BrowserError 'Unknown "representation" option: ' #
                   {String.toAtom {System.valueToVirtualString F 0 0}}}
             end
          end}

      [] !DisplayON                        then
         %%
         {ForAll {Filter {Arity M} fun {$ F} F \= 1 end}
          proc {$ F}
             case F
             of !BrowserDepth                  then
                {self SetDepth(M.F)}
                {self UpdateSizes}

             [] !BrowserWidth                  then
                {self SetWidth(M.F)}
                {self UpdateSizes}

             [] !BrowserDepthInc               then
                {self SetDInc(M.F)}

             [] !BrowserWidthInc               then
                {self SetWInc(M.F)}

             else {BrowserError 'Unknown "display parameters" option: ' #
                   {String.toAtom {System.valueToVirtualString F 0 0}}}
             end
          end}

      [] !LayoutON                         then
         %%
         {ForAll {Filter {Arity M} fun {$ F} F \= 1 end}
          proc {$ F}
             case F
             of !BrowserFontSize               then StoredFN Fonts in
                StoredFN = {self.Store read(StoreTWFont $)}
                Fonts = {Filter IKnownCourFonts
                         fun {$ Font}
                            Font.size == M.F andthen
                            Font.wght == StoredFN.wght
                         end}

                %%
                case Fonts
                of [Font] then
                   %%
                   %%  must leave the object's state!
                   thread
                      case {self.BrowserStream enq(setTWFont(Font $))}
                      then skip
                      else {BrowserError
                            'Illegal value of browser\'s "fontSize" option'}
                      end
                   end
                else {BrowserError
                      'Illegal value of browser\'s "fontSize" option'}
                end

             [] !BrowserBold                   then StoredFN Wght Fonts in
                StoredFN = {self.Store read(StoreTWFont $)}
                Wght = case M.F then bold else medium end
                Fonts = {Filter IKnownCourFonts
                         fun {$ F}
                            F.wght == Wght andthen
                            F.size == StoredFN.size
                         end}

                %%
                case Fonts
                of [Font] then
                   %%
                   thread
                      case {self.BrowserStream enq(setTWFont(Font $))}
                      then skip
                      else {BrowserError
                            'Illegal value of browser\'s "fontSize" option'}
                      end
                   end
                else {BrowserError
                      'Illegal value of browser\'s "fontSize" option'}
                end

             [] !BrowserRecordFieldsAligned    then
                case M.F of true then
                   %%
                   {self.Store store(StoreFillStyle Expanded)}
                elseof false then
                   %%
                   {self.Store store(StoreFillStyle Filled)}
                else
                   {BrowserError
                    'Illegal value of browser\'s "allignRecordFields" option'}
                end

             else {BrowserError 'Unknown "layout" option: ' #
                   {String.toAtom {System.valueToVirtualString F 0 0}}}
             end
          end}

      else {BrowserError 'Unknown option group: ' #
            {String.toAtom {System.valueToVirtualString M.1 0 0}}}
      end

      %%
   end

   %%
   %%
   meth add(Action
            label:Label <= local N = {System.printName Action} in
                              case N of '' then 'NoLabel' else N end
                           end)
\ifdef DEBUG_BO
      {Show 'FBrowserClass::add is applied'}
\endif
      {self.BrowserStream enq(addAction(Action Label))}
\ifdef DEBUG_BO
      {Show 'FBrowserClass::addProcessAction is finished'}
\endif
   end

   %%
   %%
   meth set(Action)
\ifdef DEBUG_BO
      {Show 'FBrowserClass::setProcessAction is applied'}
\endif
      {self.BrowserStream enq(setAction(Action))}
\ifdef DEBUG_BO
      {Show 'FBrowserClass::set is finished'}
\endif
   end

   %%
   %%  Acepts 'all' as a special keyword;
   meth delete(Action)
\ifdef DEBUG_BO
      {Show 'FBrowserClass::delete is applied'}
\endif
      {self.BrowserStream enq(removeAction(Action))}
\ifdef DEBUG_BO
      {Show 'FBrowserClass::removeProcessAction is finished'}
\endif
   end

   %%
   %% SetSelected ('<1>' event for a term);
   %% The 'AreCommas' argument means whether the term that is selected
   %% have been "width"-constrained;
   %%
   meth !SetSelected(Obj AreCommas)
\ifdef DEBUG_BO
      {Show 'FBrowserClass::SetSelected is applied'#Obj.term#Obj.type}
\endif
      %%
      lock
         FBrowserClass , UnsetSelected

         %%
         selected <- Obj
         thread {Obj Highlight} end

         %%
         {self.BrowserStream
          enq(entriesEnable([unselect rebrowse process]))}

         %%
         case Obj.type of !T_Shrunken then
            {self.BrowserStream [enq(entriesDisable([deref shrink]))
                                 enq(entriesEnable([expand]))]}
         elseof !T_Reference then
            {self.BrowserStream [enq(entriesDisable([expand shrink]))
                                 enq(entriesEnable([deref]))]}
         elsecase AreCommas then
            {self.BrowserStream [enq(entriesDisable([deref]))
                                 enq(entriesEnable([expand shrink]))]}
         else
            {self.BrowserStream [enq(entriesDisable([expand deref]))
                                 enq(entriesEnable([shrink]))]}
         end

         %%
\ifdef DEBUG_BO
         {Show 'FBrowserClass::SetSelected is finished'}
\endif
      end
   end

   %%
   %%
   meth !UnsetSelected
\ifdef DEBUG_BO
      {Show 'FBrowserClass::UnsetSelected is applied'}
\endif
      lock
         selected <- InitValue

         %%
         {self.BrowserStream
          [enq(unHighlightTerm)
           enq(entriesDisable([unselect rebrowse process
                               expand shrink deref]))]}

         %%
\ifdef DEBUG_BO
         {Show 'FBrowserClass::UnsetSelected is finished'}
\endif
      end
   end

   %%
   %%
   meth !SelExpand
\ifdef DEBUG_BO
      {Show 'FBrowserClass::SelExpand is applied'}
\endif
      %%
      lock
         case @selected == InitValue then skip
         else
            %%
            {self.BrowserStream enq(expand(@selected))}

            %%
            FBrowserClass , UnsetSelected
         end

         %%
\ifdef DEBUG_BO
         {Show 'FBrowserClass::SelExpand is finished'}
\endif
      end
   end

   %%
   %%
   meth !SelShrink
\ifdef DEBUG_BO
      {Show 'FBrowserClass::SelShrink is applied'}
\endif
      %%
      lock
         case @selected == InitValue then skip
         else
            %%
            {self.BrowserStream enq(shrink(@selected))}

            %%
            FBrowserClass , UnsetSelected
         end

         %%
\ifdef DEBUG_BO
         {Show 'FBrowserClass::SelShrink is finished'}
\endif
      end
   end

   %%
   %%
   meth !Process
\ifdef DEBUG_BO
      {Show 'FBrowserClass::Process is applied'}
\endif
      %%
      local Selected in
         Selected = @selected   % a snapshot;

         %%
         case Selected == InitValue then skip
         else
            Action = {self.Store read(StoreProcessAction $)}
            proc {CrashProc E T D}
               {Show '*********************************************'}
               {Show 'Exception occured in ProcessAction:'#E}
               {Show 'ProcessAction was '#Action}
            end
         in
            %%
            try {Action Selected.term}
            catch failure(debug:D) then {CrashProc failure unit D}
            [] error(T debug:D) then {CrashProc error T D}
            [] system(T debug:D) then {CrashProc system T D}
            end
\ifdef DEBUG_RM
            {Selected debugShow}
\endif
         end
      end

      %%
\ifdef DEBUG_BO
      {Show 'FBrowserClass::Process is finished'}
\endif
   end

   %%
   %%
   meth !SelDeref
\ifdef DEBUG_BO
      {Show 'FBrowserClass::SelDeref is applied'}
\endif
      %%
      lock
         case @selected == InitValue then skip
         else
            %%
            {self.BrowserStream enq(deref(@selected))}

            %%
            FBrowserClass , UnsetSelected
         end

         %%
\ifdef DEBUG_BO
         {Show 'FBrowserClass::SelDeref is finished'}
\endif
      end
   end

   %%
   meth !About
\ifdef DEBUG_BO
      {Show 'FBrowserClass::About is applied'}
\endif
      %%
      {self.BrowserStream enq(makeAbout)}
      %%
\ifdef DEBUG_BO
      {Show 'FBrowserClass::About is finished'}
\endif
   end

   %%
   meth !SetDepth(Depth)
      lock
         case {IsInt Depth} andthen Depth > 0 then
            {self.Store store(StoreDepth Depth)}
         else {BrowserError 'Illegal value of parameter BrowserDepth'}
         end
      end
   end

   %%
   meth !ChangeDepth(Inc)
      lock
         FBrowserClass , SetDepth({self.Store read(StoreDepth $)} + Inc)
         FBrowserClass , UpdateSizes
      end
   end

   %%
   meth !SetWidth(Width)
      lock
         case {IsInt Width} andthen Width > 1 then
            {self.Store store(StoreWidth Width)}
         else {BrowserError 'Illegal value of parameter BrowserWidth'}
         end
      end
   end

   %%
   meth !ChangeWidth(Inc)
      lock
         FBrowserClass , SetWidth({self.Store read(StoreWidth $)} + Inc)
         FBrowserClass , UpdateSizes
      end
   end

   %%
   meth !SetDInc(DI)
      lock
         case {IsInt DI} andthen DI > 0 then
            {self.Store store(StoreDepthInc DI)}
         else {BrowserError 'Illegal value of parameter BrowserDepthInc'}
         end
      end
   end

   %%
   meth !ChangeDInc(Inc)
      lock
         FBrowserClass , SetDInc({self.Store read(StoreDepthInc $)} + Inc)
      end
   end

   %%
   meth !SetWInc(WI)
      lock
         case {IsInt WI} andthen WI > 0 then
            {self.Store store(StoreWidthInc WI)}
         else {BrowserError 'Illegal value of parameter BrowserWidthInc'}
         end
      end
   end

   %%
   meth !ChangeWInc(Inc)
      lock
         FBrowserClass , SetWInc({self.Store read(StoreWidthInc $)} + Inc)
      end
   end

   %%
   %% Updates (increases) depth&width of terms actually shown;
   meth !UpdateSizes
\ifdef DEBUG_BO
      {Show 'FBrowserClass::UpdateSizes is applied'}
\endif
      %%
      lock
         {ForAll {self.BrowserBuffer getContent($)}
          proc {$ RootTermObj}
             {self.BrowserStream enq(updateSize(RootTermObj))}
          end}

         %%
\ifdef DEBUG_BO
         {Show 'FBrowserClass::UpdateSizes is finished'}
\endif
      end
   end

   %%
   %% Check the layout;
   meth checkLayout
\ifdef DEBUG_BO
      {Show 'FBrowserClass::checkLayt is applied'}
\endif
      lock
         local CLProc in
            %%
            proc {CLProc RootTermObj}
               {self.BrowserStream enq(checkLayoutReq(RootTermObj))}
            end

            %%
            {ForAll {self.BrowserBuffer getContent($)} CLProc}
\ifdef DEBUG_BO
            {Show 'FBrowserClass::checkLayt is finished'}
\endif
         end
      end
   end

   %%
   %% In fact, this is the 'ConfigureNotify' (X11 event) handler;
   %% It's here 'cause browser buffer's content is necessary;
   %%
   meth !SetTWWidth(Width)
\ifdef DEBUG_BO
      {Show 'FBrowserClass::SetTWWidth is applied'}
      thread {Wait Width} {Show '... Width = '#Width} end
\endif
      %%
      {Wait Width}
      lock
         {self.Store store(StoreTWWidth Width)}

         %%
         {self  checkLayout}

         %%
\ifdef DEBUG_BO
         {Show 'FBrowserClass::SetTWWidth is finished'}
\endif
      end
   end

   %%
end
