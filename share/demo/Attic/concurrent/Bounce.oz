%%%
%%% Authors:
%%%   Michael Mehl (mehl@dfki.de)
%%%   Gert Smolka (smolka@dfki.de)
%%%   Christian Schulte (schulte@dfki.de)
%%%   Joerg Wuertz (wuertz@dfki.de)
%%%
%%% Copyright:
%%%   Michael Mehl, 1998
%%%   Gert Smolka, 1998
%%%   Christian Schulte, 1998
%%%   Joerg Wuertz, 1998
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

   functor MakeBounce prop once

   import
      Tk

      Applet
      
   body

      Applet.spec = single(title(type:string default:"Oz Bouncer"))

      CanvasWidth  = 400.0
      CanvasHeight = 400.0
      BallRadius   = 50.0
      XDelta       = 5.0
      YDelta       = 5.0
      Gravity      = 3.0
      Fraction     = 0.9
      RepeatTime   = 100
      Mouth        = 30
      MouthDelta   = 5
      
      BackColor    = '#fffff0'
      BallColors   = case Tk.isColor then
			red  | blue | yellow | green  |
			plum | cyan | tan    | bisque | BallColors
		     else
			black | BallColors
		     end
      
      class Ball
	 from Time.repeat Tk.canvasTag
	 prop final
	    
	 attr
	    x:      0.0
	    xd:     XDelta
	    y:      0.0
	    yd:     YDelta
	    mouth:  Mouth
	    d:      ~MouthDelta
	    
	 meth init(X Y Canvas Color)
	    Tk.canvasTag,tkInit(parent:Canvas)
	    {Canvas tk(crea arc X Y X+BallRadius Y+BallRadius
		       fill:Color start:30 extent:300 tag:self)}
	    x <- X
	    y <- Y
	    Time.repeat, setRepAll(action:bounce delay:RepeatTime)
	    
	    thread
	       try {self go}
	       catch system(...) then skip
	       end
	    end
	 end
	 
	 meth bounce
	    %increment @x and @d
	    x <- @x + @xd
	    case @x =< 0.0 then
	       x  <- 0.0
	       xd <- ~@xd
	    elsecase @x>=CanvasWidth-BallRadius then
	       x  <- CanvasWidth-BallRadius
	       xd <- ~@xd
	    else skip end
	    %increment @y and @yd
	    y  <- @y - @yd
	    yd <- @yd - Gravity
	    case @y>=CanvasHeight-BallRadius then
	       y  <- CanvasHeight-BallRadius
	       yd <- Fraction * ~@yd
	    else skip end
	    Tk.canvasTag,tk(coords @x @y @x+BallRadius @y+BallRadius)
	    %% set the new mouth
	    mouth <- @mouth+@d
	    case @mouth>=Mouth          then mouth <- Mouth      d <- ~@d
	    elsecase @mouth=<MouthDelta then mouth <- MouthDelta d <- ~@d
	    else skip end
	    Tk.canvasTag,tk(itemconf start:@mouth extent:360-2*@mouth)
	 end
	 
	 meth close
	    Time.repeat, stop
	    Tk.canvasTag, tkClose
	 end
      end
      
      class Manager
	 from Tk.canvas
	 prop final
	 attr
	    Balls:  nil
	    Colors: BallColors
	    
	 meth init(parent:P)
	    Tk.canvas,tkInit(parent:P bg:BackColor bd:3 relief:sunken
			     width:CanvasWidth height:CanvasHeight)
	    Tk.canvas,tkBind(action: self # NewBall
			     event:  '<1>'
			     args:   [float(x) float(y)])
	    Tk.canvas,tkBind(action: self # KillBall
			     event:  '<3>')
	 end
	 
	 meth NewBall(X Y)
	    C|Cr  = @Colors
	 in
	    Balls  <- {New Ball init(X Y self C)}|@Balls
	    Colors <- Cr
	 end

	 meth KillBall
	    case @Balls of nil then skip
	    [] B|Br then {B close} Balls <- Br
	    end
	 end
      end
      

      {Tk.send pack({New Manager init(parent:Applet.toplevel)})}

   end

in
    
    MakeBounce

end
