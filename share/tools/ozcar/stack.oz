%%% $Id$
%%% Benjamin Lorenz <lorenz@ps.uni-sb.de>

local

   fun {S2F Nr Id Dir File Line Time Name Args Builtin}
      frame(nr      : Nr
            id      : Id
            dir     : Dir
            file    : File
            line    : Line
            time    : Time
            name    : Name
            args    : Args
            builtin : Builtin)
   end

   local
      fun {LastDebug F}
         case {Label F.1} == debug then
            case {Label F.2.1} == debug then
               {LastDebug F.2}
            else
               F
            end
         else
            nodebug | F
         end
      end
      proc {DoStackForAllInd Xs I P}
         case Xs
         of _|nil   then skip
         [] _|_|nil then skip
         [] X|D|A|B then
            Y|Z|T = {LastDebug D|A|B}
         in
            case Y == nodebug then
               {DoStackForAllInd Z|T I P}
            else
               case {Label Z} == builtin then
                  {P I {S2F I 0 enter X.file X.line
                        Y.1.2.1 Z.name Z.args true}}
               else
                  {P I {S2F I Y.1.1 enter X.file X.line
                        Y.1.2.1 Z.name Y.1.2.2 false}}
               end
               {DoStackForAllInd Z|T I+1 P}
            end
         else {OzcarError 'strange stack?!'}
         end
      end
   in
      proc {StackForAllInd Xs P}
         {DoStackForAllInd {LastDebug Xs}.2 1 P}
      end
   end

in

   class StackManager

      prop
         locking

      feat
         T              % the thread...
         I              % ...with it's ID
         D              % dictionary for stackframes

      attr
         Size           % current size of stack
         SP             % StackPointer (SP==Size+1 or SP==Size)
         Rebuild        % should we re-calculate the stack
                        % when the next 'step' message arrives?

         Sync  : _      % sync block/cont actions
         Delay : false

      meth init(thr:Thr id:ID)
         self.T = Thr
         self.I = ID
         self.D = {Dictionary.new}
         Size    <- 0
         SP      <- 1
         Rebuild <- false
      end

      meth blockMsg(Ack)
         New in
         Sync  <- New = unit
         Delay <- true
         thread
            {WaitOr New {Alarm TimeoutToBlock}}
            lock
               Delay <- false
               case {IsDet New} then Ack = no else Ack = ok end
            end
         end
      end

      meth contMsg(Ack)
         case @Delay then Ack = no else Ack = ok end
         Sync <- _ = unit
      end

      meth rebuild(Flag)
         Rebuild <- Flag
      end

      meth printException(Q)
         %{Browse Q.2}
         Exc#Stack = Q
         H|T = case Stack.1 == nil then Stack.2.2 else Stack end
         C = case Exc.1.1 == noElse then %% correct the line number
                {Record.adjoinAt H line Exc.1.2}
             else H end
         S = builtin(name:'Raise' args:[Exc]) | debug([0 999999999]) | C | T
         Status
      in
         {Error.debug.doOzError Exc}
         {Error.debug.last Status}

         {Ozcar rawStatus('Exception: ' # Status.1 # {Lines Status.2})}

         StackManager,ReCalculate({Reverse S})
         {Ozcar scrollbar(file:C.file line:C.line
                          color:ScrollbarBlockedColor what:appl)}
      end

      meth print
         case @Rebuild then
            StackManager,ReCalculate
         else
            Frames = {Ditems self.D}
            Depth  = @Size
            Last   = case Depth > 0 then {Dget self.D Depth} else nil end
         in
            {Ozcar printStack(id:self.I frames:Frames depth:Depth last:Last)}
         end
      end

      meth getTop($)
         S = @Size
      in
         case S == 0 then nil else {Dget self.D S} end
      end

      meth printTop
         case @Rebuild then
            StackManager,ReCalculate
         else
            S = @Size
         in
            case S == 0 then skip else
               TopFrame = {Dget self.D S}
            in
               {Ozcar printStackFrame(frame:TopFrame delete:true)}
               %{Ozcar printEnv(frame:0)}
            end
         end
      end

      meth step(name:N args:A builtin:B file:F line:L time:T frame:FrameId)
         Frame = {S2F @SP FrameId enter F L T N A B}
      in
         {Dput self.D @SP Frame}
         SP   <- @SP + 1
         Size <- @SP - 1
      end

      meth exit(FrameId)
         S = @Size
      in
         case S == 0 then skip else
            Key   = case S == @SP then {Dremove self.D S} S-1 else S end
            Frame
            NewFrame
         in
            SP   <- @SP - 1
            Size <- Key
            case Key > 0 then
               Frame    = {Dget self.D Key}
               NewFrame = {Record.adjoinAt Frame dir leave}
               {Dput self.D Key NewFrame}
            else
               {Thread.resume self.T}
            end
         end
      end

      %% local helpers %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

      meth GetStack($)
         {Reverse {Dbg.taskstack self.T MaxStackSize}}
      end

      meth RemoveAllFrames
         Frames = {Dkeys self.D}
      in
         {ForAll Frames proc {$ Frame} {Dremove self.D Frame} end}
      end

      meth ReCalculate(S<=noStack)
         CurrentStack = case S == noStack then StackManager,GetStack($)
                        else S end
      in
         {OzcarMessage 're-calculating stack of thread #' # self.I}
         {Ozcar removeSkippedProcs(self.I)}
         StackManager,rebuild(false)
         StackManager,RemoveAllFrames
         {StackForAllInd CurrentStack
          proc {$ Key Frame}
             {Dput self.D Key Frame}
          end}
         Size <- {Length {Dkeys self.D}}
         SP   <- @Size + 1
         StackManager,print
      end

      %% access methods %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

      meth getThread($)
         self.T
      end

      meth getId($)
         self.I
      end

      meth getPos(file:?F line:?L)
         S = @Size
      in
         case S == 0 then
            F = undef
            L = 0
         else
            TopFrame = {Dget self.D S}
         in
            F = TopFrame.file
            L = TopFrame.line
         end
      end

   end
end
