functor
import
   Tk
   Powered_by_oz_gif(image:Logo)
export
   ShowLogo
define
   proc{ShowLogo2 Mess1 Mess2 StartD StopD}
      thread
         T={New Tk.toplevel tkInit(bg:nil withdraw:true)}
         F={New Tk.frame tkInit(parent:T bg:white bd:1 relief:solid)}
         SX={Tk.returnInt winfo(screenwidth(T))}
         SY={Tk.returnInt winfo(screenheight(T))}
         L0={New Tk.label tkInit(parent:F text:Mess1.text bg:Mess1.bg fg:Mess1.fg
                                 font:{New Tk.font tkInit(family:helvetica size:Mess1.size weight:bold)})}
         L1={New Tk.label tkInit(parent:F text:Mess2.text bg:Mess2.bg fg:Mess2.fg
                                 font:{New Tk.font tkInit(family:helvetica size:Mess2.size weight:normal)})}
         ImageL1={New Tk.label tkInit(parent:F image:Logo bg:white)}
         TX TY
      in
         {Tk.batch [grid(L0 row:0 column:0 sticky:we ipadx:5 ipady:5)
                    grid(L1 row:1 column:0 sticky:we ipadx:5 ipady:0)
                    grid(ImageL1 row:2 column:0 ipadx:10 ipady:5 sticky:we)
                    grid(F row:0 column:0)
                   ]}
         {Delay StartD.delay} % Should at least be 100ms, otherwise following winfo-calls will fail
         TY={Tk.returnInt winfo(height(F))}
         TX={Tk.returnInt winfo(width(F))}
         {Tk.batch [wm(overrideredirect T true)
                    wm(geometry T '+'#((SX-TX) div 2)#'+'#((SY-TY) div 3))
                    wm(deiconify T)
                   ]}
         {Wait StopD.delay} %{Delay StopD.delay}
         {T tkClose}
      end
   end
   proc{ShowLogo Start Stop}
      {ShowLogo2
       main(size:18 fg:red bg:white text:"Mozart Instant Messenger")
       sub(size:10 fg:black bg:white text:"Written by Nils Franzen and Simon Lindblom")
       start(delay:Start)
       stop(delay:Stop)}
   end
end

/*

declare [A]={Module.apply [AA]}
ShowLogo=A.showLogo

%%% Typical use

{ShowLogo
 main(size:24 fg:red bg:white text:"W  e  b  M  e  e  t  i  n  g")
 sub(size:10 fg:black bg:white text:"(c) 1999 Swedish Institute of Computer Science")
 start(delay:1000)
 stop(delay:6000)}


*/

/*

%%% Delayed Stop

declare Stop
{ShowLogo
 main(size:34 fg:deeppink bg:darkslategrey text:"W e b M e e t i n g")
 sub(size:10 fg:black bg:white text:"(c) 1999 Swedish Institute of Computer Science")
 start(delay:1000)
 stop(delay:Stop)}

Stop=1 % Feed this line to close window

*/
