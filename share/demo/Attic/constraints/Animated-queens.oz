%%%
%%% Authors:
%%%   Christian Schulte (schulte@dfki.de)
%%%
%%% Copyright:
%%%   Christian Schulte, 1997
%%%
%%% Last change:
%%%   $Date$ by $Author$
%%%   $Revision$
%%%
%%% This file is part of Mozart, an implementation
%%% of Oz 3
%%%    $MOZARTURL$
%%%
%%% See the file "LICENSE" or
%%%    $LICENSEURL$
%%% for information on usage and redistribution
%%% of this file, and for a DISCLAIMER OF ALL
%%% WARRANTIES.
%%%

local

   functor MakeQueens prop once

   import
      FD

      Tk

      TkTools

      Applet

   body
      URL = 'http://www.ps.uni-sb.de/ozhome/demo/images/animated-queens/'

      MaxWidth      = 600 % How large can the drawing area be

      FailWidth     = 10

      DefaultSize   = 6   % With which N-Queens problem should we start

      ParamWinTitle = 'Animated Queens: Size'

      LargeFont     = '-Adobe-times-bold-r-normal--*-240*'

      BlackColor    #
      WhiteColor    #
      QueenColor    #
      CrossColor    #
      FailColor     = case Tk.isColor then
                         gray85 # gray95 # darkorange1 # gray75 # firebrick
                      else black # white # black # black # black
                      end

      %% Derived Parameters
      WidthByMag    = s(micro:2 tiny:5 small:10 middle:25 large:50)

      MaxBoardSize  = 255

      QueenByMag = q(micro:  {New Tk.image
                              tkInit(type:bitmap foreground:QueenColor
                                     url: URL # 'micro-queen.xbm')}
                     tiny:   {New Tk.image
                              tkInit(type:bitmap foreground:QueenColor
                                     url: URL # 'tiny-queen.xbm')}
                     small:  {New Tk.image
                              tkInit(type:bitmap foreground:QueenColor
                                     url: URL # 'small-queen.xbm')}
                     middle: {New Tk.image
                              tkInit(type:bitmap foreground:QueenColor
                                     url: URL # 'middle-queen.xbm')}
                     large:  {New Tk.image
                              tkInit(type:bitmap foreground:QueenColor
                                     url: URL # 'large-queen.xbm')})

      CrossByMag = c(micro:  false
                     tiny:   false
                     small:  false
                     middle: {New Tk.image
                              tkInit(type:bitmap foreground:CrossColor
                                     url: URL # 'middle-cross.xbm')}
                     large:  {New Tk.image
                              tkInit(type:bitmap foreground:CrossColor
                                     url: URL # 'large-cross.xbm')})

      NaiveStrat     = 1
      FirstFailStrat = 2
      UpFirstStrat   = 3
      MiddleOutStrat = 4

      DefaultStrat   = MiddleOutStrat

      %%
      %% The problem solving part
      %%

      local

         fun {OrderUp X Y}
            SizeX = {FD.reflect.size X}
            SizeY = {FD.reflect.size Y}
         in
            SizeX < SizeY orelse
            SizeX==SizeY andthen
            {FD.reflect.min X} < {FD.reflect.min Y}
         end

         fun {QueensSolver Size Strategy}
            Enum = case Strategy
                   of !NaiveStrat     then naive
                   [] !FirstFailStrat then ff
                   [] !UpFirstStrat   then generic(order:OrderUp)
                   [] !MiddleOutStrat then generic(value:mid)
                end
         in
            proc {$ Xs}
               Xs = {FD.list Size 1#Size}
               {FD.distinct Xs}
               {FD.distinctOffset Xs {List.number 1 Size 1}}
               {FD.distinctOffset Xs {List.number Size 1 ~1}}
               {FD.distribute Enum Xs}
            end
         end

      in

         class Engine
            from BaseObject

            feat
               canvas

            attr
               Stack:     nil
               Stopped:   false

            meth init(Size Strategy Canvas)
               S={Space.new {QueensSolver Size Strategy}}
            in
               Stopped   <- false
               Stack     <- [S]
            end

            meth next
               case @Stack
               of nil then
                  {self finish}
                  Stopped <- true
                  {self.canvas stop}
               [] S|Sr then
                  case S==backtrack then
                     Stack <- Sr
                     {self backtrack}
                     Engine,next
                  else
                     {self show(S)}
                     case {Space.ask S}
                     of alternatives(M) then
                        C={Space.clone S}
                     in
                        {Space.commit S 1}
                        {Space.commit C 2#M}
                        Stack   <- S|C|backtrack|Sr
                     [] failed then
                        Stack   <- backtrack|Sr
                     [] succeeded then
                        Stack   <- backtrack|Sr
                        Stopped <- true
                        {self.canvas stop}
                     end
                  end
               end
            end

            meth sol
               Engine, next
               case @Stopped then skip else
                  Engine,sol
               end
            end

            meth stop
               Stopped <- true
            end

            meth start
               Stopped <- false
            end

         end
      end

      local

         fun {ReflectForCrosses Xs}
            case Xs of nil then nil
            [] X|Xr then {FD.reflect.domList X}|{ReflectForCrosses Xr}
            end
         end

         fun {Reflect Xs}
            case Xs of nil then nil
            [] X|Xr then
               case {FD.reflect.size X}==1 then X else void end|{Reflect Xr}
            end
         end

      in

         fun {MakePainter Canvas Mag Size}
            Width = WidthByMag.Mag
            Cross = CrossByMag.Mag
            Queen = QueenByMag.Mag
            Total = Size * Width
            Fail0 = Total div 4
            Fail1 = Fail0 + Total div 2

            proc {DrawFail T}
               {Canvas tk(create line Fail0 Fail0 Fail1 Fail1
                          width:FailWidth fill:FailColor capstyle:round tags:T)}
               {Canvas tk(create line Fail0 Fail1 Fail1 Fail0
                          width:FailWidth fill:FailColor capstyle:round tags:T)}
            end

            proc {DrawQueen X Y T}
               case X==void orelse Y==void then skip else
                  {Canvas tk(crea image (X-1)*Width (Y-1)*Width
                             image:  Queen
                             tags:   T
                             anchor: nw)}
               end
            end

            UpdateBoard
            ReflectBoard

            case Cross\=false then
               proc {DrawCross X Y T}
                  {Canvas tk(crea image (X-1)*Width (Y-1)*Width
                             image:  Cross
                             tags:   T
                             anchor: nw)}
               end
               proc {DrawCrosses Os Ns I T}
                  case Os of nil then skip
                  [] O|Or then
                     case Ns of nil then {DrawCross O I T} {DrawCrosses Or Ns I T}
                     [] N|Nr then
                        {DrawCrosses Or case O<N then {DrawCross O I T} Ns
                                        else Nr end I T}
                     end
                  end
               end
            in
               proc {UpdateBoard Os Ns I T}
                  case Os of nil then skip
                  [] O|Or then N|Nr=Ns in
                     case O.2==nil then skip else
                        case N of [M] then {DrawQueen M I T} else skip end
                        {DrawCrosses O N I T}
                     end
                     {UpdateBoard Or Nr I+1 T}
                  end
               end
               ReflectBoard = ReflectForCrosses
            else
               proc {UpdateBoard Os Ns I T}
                  case Os of nil then skip
                  [] O|Or then N|Nr=Ns in
                     case O==N then skip else {DrawQueen N I T} end
                     {UpdateBoard Or Nr I+1 T}
                  end
               end
               ReflectBoard = Reflect
            end
         in

            class $
               from Engine
               attr Stack:nil

               meth backtrack
                  case @Stack of nil then skip
                  [] S|Sr then {S.2 tkClose} Stack <- Sr
                  end
               end

               meth show(S)
                  NewT = {New Tk.canvasTag tkInit(parent:Canvas)}
               in
                  case {Space.ask S}
                  of failed then
                     {DrawFail NewT}
                     Stack <- _#NewT|@Stack
                  else
                     NewB = {ReflectBoard {Space.merge {Space.clone S}}}
                  in
                     case @Stack of nil then
                        Stack <- [NewB#NewT]
                     [] OldB#OldT|Sr then
                        {UpdateBoard OldB NewB 1 NewT}
                        Stack <- NewB#NewT|@Stack
                     end
                  end
               end

               meth finish
                  {Canvas tk(delete all)}
                  thread
                     {Canvas.sol  tk(conf state:disabled)}
                     {Canvas.next tk(conf state:disabled)}
                  end
               end
            end
         end

      end


      class Board
         from Tk.canvas

         prop
            locking

         attr
            engine
            size:     DefaultSize
            strategy: DefaultStrat

         feat
            stop
            next
            sol
            toplevel

         meth init(Top)
            lock
               self.toplevel = Top
               StratVar = {New Tk.variable tkInit(DefaultStrat)}
               Menu     = {TkTools.menubar Top Top
                           [menubutton(text:'Queens' feature:queens
                                       menu:    [command(label:  'About Queens'
                                                         action: self # about)
                                                 command(label:  'Restart Search'
                                                         action: self # start)
                                                 separator
                                                 command(label:  'Quit'
                                                         action: self # close)])
                            menubutton(text:'Options' feature:options
                                       menu: [radiobutton(label:  'No Heuristic'
                                                          var:    StratVar
                                                          value:  NaiveStrat
                                                          action:
                                                             self #
                                                          setStrategy(NaiveStrat))
                                              radiobutton(label:  'Least First'
                                                          var:    StratVar
                                                          value:  FirstFailStrat
                                                          action:
                                                          self #
                                                          setStrategy(FirstFailStrat))
                                              radiobutton(label:  'Smart Least First'
                                                          var:    StratVar
                                                          value:  UpFirstStrat
                                                          action:
                                                             self #
                                                          setStrategy(UpFirstStrat))
                                              radiobutton(label:  'Middle First'
                                                          var:    StratVar
                                                          value:  MiddleOutStrat
                                                          action:
                                                             self #
                                                          setStrategy(MiddleOutStrat))
                                              separator
                                              command(label:  'Change Size'
                                                      action: self # setSize)])]
                           nil}
               Frame  = {New Tk.frame    tkInit(parent: Top)}

               Tk.canvas,tkInit(parent:             Top
                                relief:             sunken
                                bd:                 2
                                xscrollincrement:   1
                                yscrollincrement:   1
                                highlightthickness: 0
                                background:         WhiteColor)
               {self tk(xview scroll ~2 units)}
               {self tk(yview scroll ~2 units)}

               Stop   = {New Tk.button   tkInit(parent: Frame
                                                text:   'Stop'
                                                state:  disabled
                                                action: self # stop)}
               Next   = {New Tk.button   tkInit(parent: Frame
                                                text:   'Next Step'
                                                action: self # next)}
               Sol    = {New Tk.button   tkInit(parent: Frame
                                                text:   'Next Solution'
                                                action: self # sol)}
            in
               {Menu.options.menu tk(conf tearoff:false)}
               {Menu.queens.menu  tk(conf tearoff:false)}
               {Tk.batch [pack(Menu side:top fill:x)
                          pack(Stop Next Sol fill:x side:left)
                          pack(self Frame padx:4 pady:4 side:top)]}
               self.stop   = Stop
               self.next   = Next
               self.sol    = Sol
               Board,start
            end
         end

         meth setStrategy(Strat)
            lock
               strategy <- Strat
               Board,stop
               Board,start
            end
         end

         meth setSize
            lock
               Size
               Dialog = {New TkTools.dialog
                         tkInit(title:   ParamWinTitle
                                master:  self.toplevel
                                buttons: ['Okay' #
                                          tkClose(proc {$}
                                                     Size={Top tkReturnInt(get $)}
                                                  end)]
                                pack:    false
                                focus:   1
                                default: 1)}
               Frame  = {New TkTools.textframe tkInit(parent: Dialog
                                                      text:   'Board Size')}
               Top    = {New Tk.scale tkInit(parent:    Frame.inner
                                             'from':    4
                                             to:        MaxBoardSize
                                             length:    8#c
                                             orient:    horizo
                                             showvalue: true)}
            in
               {Top tk(set @size)}
               {Tk.batch [pack(Top) pack(Frame fill:both)]}
               {Dialog tkPack}
               {Wait Size}
               size <- Size
               Board,stop
               Board,start
            end
         end

         meth about
            lock
               Dialog = {New TkTools.dialog tkInit(title:   ParamWinTitle
                                                   buttons: ['Okay' # tkClose]
                                                   default: 1
                                                   focus:   1
                                                   master:  self.toplevel)}
               TitleAndQueen = {New Tk.frame tkInit(parent: Dialog)}
               Title = {New Tk.label tkInit(parent: TitleAndQueen
                                            font:   LargeFont
                                            fg:     blue
                                            text:   'Animated Queens')}
               Queen = {New Tk.label tkInit(parent: TitleAndQueen
                                            image:  QueenByMag.large)}
               Author = {New Tk.label tkInit(parent: Dialog
                                             text:('Christian Schulte\n' #
                                                   '(schulte@dfki.uni-sb.de)\n'))}
            in
               {Tk.batch [pack(Queen Title
                               side:left fill:both ipadx:2#m ipady:2#m)
                          pack(TitleAndQueen Author
                               side:top padx:2#m pady:2#m)]}
               {Wait Dialog.tkClosed}
            end
         end

         meth start
            lock
               Size     = @size
               Strat    = @strategy
               Mag      = case     Size*WidthByMag.large =<MaxWidth then large
                          elsecase Size*WidthByMag.middle=<MaxWidth then middle
                          elsecase Size*WidthByMag.small =<MaxWidth then small
                          elsecase Size*WidthByMag.tiny  =<MaxWidth then tiny
                          else micro
                          end
               MagWidth = WidthByMag.Mag
               Width    = Size*MagWidth
            in
               {self tk(delete all)}
               {self tk(configure width:Width height:Width)}
               {For 0 Size-1 2
                proc {$ I}
                   {For 1 Size-1 2
                    proc {$ J}
                       {self tk(crea rectangle
                                I*MagWidth     J*MagWidth
                                (I+1)*MagWidth (J+1)*MagWidth
                                fill:BlackColor outline:'')}
                    end}
                end}
               {For 1 Size-1 2
                proc {$ I}
                   {For 0 Size-1 2
                    proc {$ J}
                       {self tk(crea rectangle
                                I*MagWidth      J*MagWidth
                                (I+1)*MagWidth (J+1)*MagWidth
                                fill:BlackColor outline:'')}
                    end}
                end}
               {self.sol  tk(conf state:normal)}
               {self.next tk(conf state:normal)}
               engine <- {New {MakePainter self Mag Size}
                          init(Size Strat self)}
            end
         end

         meth stop
            {@engine stop}
            {self.stop tk(conf state:disabled)}
         end

         meth next
            lock
               {self.sol  tk(conf state:disabled)}
               {self.next tk(conf state:disabled)}
               {@engine next}
               {self.sol  tk(conf state:normal)}
               {self.next tk(conf state:normal)}
            end
         end

         meth sol
            lock
               {self.sol  tk(conf state:disabled)}
               {self.next tk(conf state:disabled)}
               {self.stop tk(conf state:normal)}
               {@engine start}
               {@engine sol}
               {self.sol  tk(conf state:normal)}
               {self.next tk(conf state:normal)}
            end
         end

         meth close
            lock
               Board,       stop
               {self.toplevel tkClose}
               {Wait _}
            end
         end

      end

      {New Board init(Applet.toplevel) _}

   end

in

   {Application.applet
    'animated-queens.oza'

    MakeQueens

    single(title(type:string default:"Animated Queens"))
   }

end
