functor
import
   Main  at 'x-oz://system/Main.ozf'
   DPPane(siteStatistics) at 'x-oz://boot/DPPane'
   Connection
export
   Open
   OpenNetInfo
   Client
   Server
define

      class DPclient
      prop locking
      attr
         state:starting

      feat
         serverPort
         site

      meth init(Tick)
         S P = {NewPort S}
         Site = {Filter {DPPane.siteStatistics} fun{$ M} M.state == mine end}.1
      in
         self.site = site(ip:Site.ip port:Site.port)
         self.serverPort = {Connection.take Tick}
         {Send self.serverPort connecting(P self.site)}
         thread {ForAll S self} end
      end

      meth connected
         state <- waiting
      end

      meth start(Time)
         %% skiping accumulated data
         _ = {DPPane.siteStatistics}
         state <- running(Time)
         {self update(Time)}
      end

      meth update(Time)
         if @state == running(Time) then
            try
               {Send self.serverPort data({DPPane.siteStatistics} self.site)}
               if Time == 0 then skip
               else {Delay Time} end
            catch _ then
               state <- crashed
            end
            {self update(Time)}
         end
      end

      meth stop
         state <- waiting
      end
   end

   proc{Client Tick}
      N O DPC in
      DPC = {New DPclient init(Tick)}
   end


   Open=Main.open
   OpenNetInfo=Main.openNetInfo
   Server=Main.server
end
