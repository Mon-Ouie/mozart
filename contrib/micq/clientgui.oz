%%%
%%% Authors:
%%%   Nils Franz�n (nilsf@sics.se)
%%%   Simon Lindblom (simon@sics.se)
%%%
%%% Copyright:
%%%   Nils Franz�n, 1998
%%%   Simon Lindblom, 1998
%%%
%%% Last change:
%%%   $Date$ by $Author$
%%%   $Revision$
%%%
%%% This file is part of Mozart, an implementation
%%% of Oz 3
%%%    http://mozart.ps.uni-sb.de
%%%
%%% See the file "LICENSE" or
%%%    http://mozart.ps.uni-sb.de/LICENSE.html
%%% for information on usage and redistribution
%%% of this file, and for a DISCLAIMER OF ALL
%%% WARRANTIES.
%%%

functor
require
   DefaultSettings(pictureUrl:PicturesURL) at 'defaultsettings.ozf'
   Meths(%getApplicationInfo:S_getApplicationInfo
         %getapplication:S_getapplication
         message:S_message
         %logout:S_logout
         addFriends:S_addFriends
         removeFriend:S_removeFriend
         %getFriends:S_getFriends
         setStatus:S_setStatus
         %searchFriends:S_searchFriends
         %login:S_login
         updateSettings:S_updateSettings
         %addApplication:S_addApplication
         %editApplication:S_editApplication
         %inviteUser:S_inviteUser
         %addUser:S_addUser
         %removeUser:S_removeUser
         %getInfo:S_getInfo
         %updateUser:S_updateUser
         messageAck:S_messageAck
         %removeMessage:S_removeMessage
         getUserName:S_getUserName
         %removeApplication:S_removeApplication
        ) at 'methods.ozf'
import
   DP(open) at 'x-oz://contrib/tools/DistPanel'
   Tk TkTools(dialog error)
   Pop(popup:Popup) at 'popup.ozf'
   DD(dragAndDrop:DragAndDrop) at 'draganddrop.ozf'
   Message(new:ComposeMess read:ReadMess) at 'messagegui.ozf'
   NewFriendsGUI(start:AddFriends) at 'newfriendsgui.ozf'
   EditAccount at 'editaccount.ozf'
   AddApplicationGUI at 'addapplicationgui.ozf'
   Dialog(view) at 'dialoggui.ozf'
   System
   Panel
   Browser(browse:Browse)
   ConfigClient(start) at 'configureclient.ozf'
   EditApplicationGUI( start: EditApp) at 'editapplicationgui.ozf'
   DisplayMess(display) at 'messagedisplay.ozf'
export
   start:Start
   shutdown:Shutdown
   addapp:AddApp
   startapp:StartApp
   friends:Friends
   removeFriend:RemoveFriendEntry
   receiveMessage:ReceiveMessage
   messageAck:MessageAck
   changeStatus:ChangeStatus
   setInfo:SetInfo
   errorBox:ErrorBox
   updateUser:UpdateUser
   invite:InviteClient
   insertMess:InsertOldMessages
   removeApp:RemoveApp
   updateApp:UpdateApp
define
   GUIisStarted
   Server  Client  ClientID
   DB={Dictionary.new}
   CLock={NewLock}
   MyData={NewCell nil}
   OldMessages
   Applications={NewCell nil}

   FontLabel={New Tk.font tkInit(size:8 family:helvetica)}
   FontSeparator={New Tk.font tkInit(size:8 family:courier)}
   FontSystem={New Tk.font tkInit(size:10 family:times)}

   ClientGUISettings={NewCell ui(fontsize:8
                                 foreground:nil
                                 background:nil)}

   proc{UpdateUISettings} S={Access ClientGUISettings} in
      {FontLabel tk(config size:S.fontsize)}
      if S.foreground\=nil andthen S.background\=nil then
         {Tk.send tk_setPalette(background(S.background) foreground(S.foreground))}
      end
      thread {Server S_updateSettings(id:ClientID settings:S)} end
   end

   AnimateThread
   proc{Animate} Is={Dictionary.items DB} in
      {ForAll Is proc{$ X} {X.widget animate} end}
      {Delay 800}
      {Animate}
   end

   LetterImage={New Tk.image tkInit(type:photo format:gif url:PicturesURL#'letter.gif')}
   AwayImage={New Tk.image tkInit(type:photo format:gif url:PicturesURL#'away_t.gif')}
   OnlineImage={New Tk.image tkInit(type:photo format:gif url:PicturesURL#'online_t.gif')}
   OfflineImage={New Tk.image tkInit(type:photo format:gif url:PicturesURL#'offline_t.gif')}
   UnreadSentMailImage={New Tk.image tkInit(type:photo format:gif url:PicturesURL#'eyes_t.gif')}

   class DDLabel from Tk.label DragAndDrop
      prop locking
      feat parent
      meth tkInit(parent:P ...)=M
         self.parent=P
         Tk.label, {Adjoin usefont(font:FontLabel) M}
         DragAndDrop, dragAndDropInit
      end
      meth getState($) {self.parent getState($)} end
   end
   class DDUserLabel from DDLabel
      prop final
      meth setState(X)
         if {Label X}==application then {self.parent sendInvitation(X)}
         else {Tk.send bell} end
      end
   end
   class DDAppLabel from DDLabel
      prop final
      meth setState(X)
         if {Label X}==user andthen X.online==true then
            {self.parent sendInvitation(X)}
         else {Tk.send bell} end
      end
   end

   class UserEntry from Tk.frame
      prop locking
      feat
         id label image
      attr
         sentCount:nil
         newmessages:nil
         oldmessages:nil
         sentmessages:nil
         name counter:0

         currentImage:1
         currentImage0:nil
         currentImage1:OnlineImage

      meth sendInvitation(X)
         if {Dictionary.get DB self.id}.online==true then
            {Client invite(id:self.id application:X.id)}
         else {Tk.send bell} end
      end
      meth getState($) user(id:self.id name:@name online:{Dictionary.get DB self.id}.online) end
      meth tkInit(id:ID name:T parent:P ...)=M
         L I
         M1={Record.subtract {Record.subtract M id} name}
         Tk.frame, M1
      in
         self.id=ID
         self.image=I={New Tk.label tkInit(parent:self image:OfflineImage width:15 height:14)}
         self.label=L={New DDUserLabel tkInit(parent:self justify:left anchor:w text: "["#ID#"] "#T)}
         name<-T
         {Tk.batch [grid(L row:0 column:1 sticky:we)
                    grid(I row:0 column:0 sticky:w)
                    grid(columnconfigure self 1 weight:1)]}
         local
            B=proc{$}
                 if @newmessages\=nil then {self readNewMessage()}
                 else {self writeNewMessage(ID)} end
              end
            A=proc{$} E={Dictionary.get DB ID} in
                 {Popup [if @newmessages==nil then ignore
                         else L={Length @newmessages} in
                            ("Read New Message ("#L#" unread)")#proc{$} {self readNewMessage()} end
                         end
                         ("Send to "#E.name)#proc{$} {self writeNewMessage(ID)} end
                         if @oldmessages==nil andthen @sentmessages==nil then ignore
                         else "View Dialog"#proc{$} {self viewDialog} end end
                         separator
                         ("Remove "#E.name)#proc {$}{RemoveFriend E}end
                        ] self}
              end
         in
            {L tkBind(event:'<Double-1>' action:B)}
            {I tkBind(event:'<Double-1>' action:B)}
            {L tkBind(event:'<3>' action:A)}
            {I tkBind(event:'<3>' action:A)}
         end
      end

      meth setMessages(old:O<=nil new:N<=nil sent:S<=nil sentCount:SC<=nil)
         if N \= nil then newmessages<-N {self checkUnreadMail} end
         if O \= nil then oldmessages<-O end
         if S \= nil then sentmessages<-S end
         sentCount<-SC
      end

      meth getMessages(old:O new:N sent:S sentCount:SC<=_)
         N=@newmessages
         O=@oldmessages
         S=@sentmessages
         SC=@sentCount
      end

      meth getLID($) O N in
         O=counter<-N
         N=O+1
      end

      meth checkUnreadMail()
         if {Length @newmessages}>0 then {self haveUnreadMail(true)}
         else {self haveUnreadMail(false)} end
      end

      meth receiveMessage(message:M mid:MID sender:S date: D reply_to:Re status:T)=R O N in
         if T==new then
            O=newmessages<-N
            {self haveUnreadMail(true)}
            {Tk.send bell} % Notify the user that a new message arrived
         else O=oldmessages<-N end
         N={Append O [{Record.adjoin lid(lid:{self getLID($)} type: received) R}]}
      end

      meth readNewMessage()
         Os Ns Oo No M
         E={Dictionary.get DB self.id}
      in
         Os=newmessages<-Ns
         if Os\=nil then
            if {Length Os}=<1 then
               {self haveUnreadMail(false)}
            end
            M={Nth Os 1}
            Ns={List.drop Os 1}
            {Server S_messageAck(mid:M.mid id:ClientID)}
            Oo=oldmessages<-No
            No={Append Oo [M]}
            {self readMessage(entry:E message:M)}
         else
            Os=Ns
         end
      end

      meth writeNewMessage(ID message:Mess<=nil reply_to:RPT<=nil) E={Dictionary.get DB ID} in
         {ComposeMess message(user:user(id:E.id name:E.name)
                                 send:proc{$ IDs X} MID D in
                                         {Server S_message(sender:ClientID
                                                         receiver:IDs
                                                         message:X
                                                         reply_to:RPT
                                                         mid:MID
                                                         date:D)}

                                         lock CLock then
                                            %% Store the message
                                            {ForAll IDs proc{$ I}  E={Dictionary.get DB I} in
                                                           {E.widget saveSent(message:X
                                                                              reply_to:RPT mid:MID
                                                                              date:D incCount:true)}
                                                        end}
                                         end
                                      end
                              message:Mess)}
      end

      meth readMessage(entry:E message:M)
         {ReadMess {Record.adjoin M
                    read(user:used(name:E.name id:E.id)
                         send:proc{$ X} {self writeNewMessage(E.id message:X reply_to:M.mid)} end)}}
      end

      meth hasReadMail(mid: M)
         sentCount <- {List.subtract @sentCount M }
      end

      meth saveSent(message:M mid:Mid date:D reply_to:R incCount:IC<=false) O N in
         if IC==true then
            sentCount <- Mid|@sentCount
         end
         O=sentmessages<-N
         N={Append O [message(lid:{self getLID($)} date: D mid: Mid type:sent reply_to: R message: M)]}
      end
      meth ClearOldAndSentMessages
         oldmessages<-nil  sentmessages<-nil
         {Client clearHistory(friend: self.id)}
      end
      meth viewDialog E={Dictionary.get DB self.id} in
         {System.show {Append @oldmessages @sentmessages}}
         {Dialog.view E self {Append @oldmessages @sentmessages} DB proc{$} {self ClearOldAndSentMessages} end}
      end
      meth haveUnreadMail(X)
         if X==true then currentImage0<-LetterImage
         else currentImage0<-nil end
      end

      meth away() currentImage1<-AwayImage end
      meth available() currentImage1<-if {Dictionary.get DB self.id}.online==true then OnlineImage
                                      else OfflineImage end
      end

      meth animate O N in
         O=currentImage<-N
         if O==0 then I=@currentImage0 in
            if I\=nil then {self.image tk(config image:I)} end
            N=1
         elseif O==1 then
            if {Length @sentCount}>0 then
               {self.image tk(config image:UnreadSentMailImage)}
               N=2
            else
               {self.image tk(config image:@currentImage1)}
               N=3
            end
         elseif O==4 then
            {self.image tk(config image:@currentImage1)}
            N=0
         else
            N=O+1
         end
      end
   end

   class OthersEntry from UserEntry
      attr  currentImage1:OfflineImage
      meth tkInit(id:ID name:T parent:P ...)=M
         L I
         M1={Record.subtract {Record.subtract M id} name}
         Tk.frame, M1
      in
         self.id=ID
         self.image=I={New Tk.label tkInit(parent:self image:OfflineImage width:15 height:14)}
         self.label=L={New DDLabel tkInit(parent:self justify:left anchor:w text:"["#ID#"] "#T)}
         name<-T
         {Tk.batch [grid(L row:0 column:1 sticky:we)
                    grid(I row:0 column:0 sticky:w)
                    grid(columnconfigure self 1 weight:1)]}
         local
            A=proc{$} if @newmessages\=nil then {self readNewMessage()} end end
            B=proc{$}
                 {Popup [if @newmessages==nil then ignore
                         else L={Length @newmessages} in
                            ("Read New Message ("#L#" unread)")#proc{$} {self readNewMessage()} end
                         end
                         separator
                         ("Add "#T#" as friend")#proc{$} {Server S_addFriends(id:ClientID friends:[ID])} end
                         ("Remove "#T)#proc{$} {Others remove(id:self.id)}  end
                        ] self}
              end
         in
            {L tkBind(event:'<Double-1>' action:A)}
            {I tkBind(event:'<Double-1>' action:A)}
            {L tkBind(event:'<3>' action:B)}
            {I tkBind(event:'<3>' action:B)}
         end
      end
      meth readNewMessage() E Os Ns Oo No M in
         Os=newmessages<-Ns
         if Os\=nil then
            if {Length Os}=<1 then
               {self haveUnreadMail(false)}
            end
            M={Nth Os 1}
            Ns={List.drop Os 1}
            {Server S_messageAck( mid: M.mid id: ClientID )}
            Oo=oldmessages<-No
            No={Append Oo [M]}
            if {Dictionary.member DB M.sender} then
               E={Dictionary.get DB M.sender}
            else
               E=user(id:M.sender name:nil)
            end
            {self readMessage(entry:E message:M)}
         else Os=Ns end
      end
   end

   class AppEntry from Tk.frame
      attr
         name
      feat
         id label

      meth sendInvitation(X) {Client invite(id:X.id application:self.id)} end
      meth getState($) application(id:self.id name:@name) end
      meth tkInit(id:ID name:T parent:P ...)=M
         L M1={Record.subtract {Record.subtract M id} name}
         Tk.frame, M1
      in
         self.id=ID
         self.label=L={New DDAppLabel tkInit(parent:self anchor:w justify:left text:T)}
         name<-T
         {Tk.batch [grid(L row:0 column:0 sticky:we)
                    grid(columnconfigure self 0 weight:1)]}
         {L tkBind(event:'<Double-1>'
                   action:proc{$} {Client startClient(application:self.id)} end)}
         {L tkBind(event:'<3>'
                   action:proc{$}
                             Os={Map {Filter {Dictionary.items DB} fun{$ X} X.online\=false end}
                                 fun{$ Y} Y.name#proc{$} {Client invite(id:Y.id application:ID)} end end}
                          in
                             {Popup [if Os\=nil then "Invite"#Os else ignore end
                                     ("Start a "#T#" client")#proc {$}
                                                                 {Client startClient(application: self.id)}
                                                              end
                                     ("Halt "#T#" Server")#proc {$} {HaltApplication ID} end
                                     separator
                                     "Logout"#Kill
                                    ] self}
                          end)}
      end
   end

   class UserBox from Tk.frame
      feat type
      attr row:0
      meth tkInit(name:N type:OL parent:P ...)=M
         Tk.frame, tkInit(parent:P bd:0)
         TL  Row={self getRow($)}
         F1={New Tk.frame tkInit(parent:self bd:0)}
         fun{NF} {New Tk.frame tkInit(parent:F1 bd:1 relief:sunken height:2 width:10)} end
      in
         self.type=OL
         TL={New Tk.label tkInit(parent:F1 text:N font:FontSeparator)}
         {Tk.batch [grid({NF} row:0 column:0 sticky:we padx:2)
                    grid(TL row:0 column:1)
                    grid({NF} row:0 column:2 sticky:we padx:2)
                    grid(F1 row:Row column:0 sticky:we)
                    grid(columnconfigure F1 0 weight:1)
                    grid(columnconfigure F1 1 weight:0)
                    grid(columnconfigure F1 2 weight:1)
                    grid(columnconfigure self 0 weight:1)]}
      end

      meth getRow($) O N in
         O=row<-N
         N=O+1
      end

      meth add(id:ID name:T)
         if {Dictionary.member DB ID} then E={Dictionary.get DB ID} in
            if E.online\=others then
               raise entryAllreadyExists(ID T) end
            end

            %% Now, the user must be in the "others"-box, we can do a simple ChangeStatus
            {Others remove(id:E.id)}
            {self move(id:E.id entry:E)}
         else
            E={New UserEntry tkInit(id:ID parent:self name:T)}
            R={self getRow($)}
         in
            {Dictionary.put DB ID entry(id:ID name:T widget:E online:if self.type==online then true else false end)}
            {Tk.send grid(E row:R column:0 sticky:we)}
            {E available}
         end
      end

      meth move(id:ID entry:E)
         O N S SC R={self getRow($)}
         W={New UserEntry tkInit(id:ID parent:self name:E.name)}
      in
         {E.widget getMessages(old:O new:N sent:S sentCount:SC)}
         {W setMessages(old:O new:N sent:S sentCount:SC)}
         {Dictionary.put DB ID {Record.adjoin E entry(widget:W online:if self.type==online then true
                                                                      else false end)}}
         {Tk.send grid(W row:R column:0 sticky:we)}
         {W available}
      end

      meth remove(id:ID) E={Dictionary.get DB ID} in
         {Tk.send grid(forget(E.widget))}
         {Dictionary.remove DB ID}
      end
   end

   class OthersBox from UserBox
      prop final
      meth add(id:ID name:T)
         if {Dictionary.member DB ID} then
            raise entryAllreadyExists(ID T) end
         else % Make the frame visible!
            E={New OthersEntry tkInit(id:ID parent:self name:T)}
            R={self getRow($)}
         in
            if R==2 then {Tk.send grid(self column:0 row:3 sticky:news)} end %Others
            {Dictionary.put DB ID entry(id:ID name:T widget:E online:others)}
            {Tk.send grid(E row:R column:0 sticky:we)}
         end
      end
      meth move(...)=M raise thisMethodShouldNotBeInvokedError('OthersBox' M) end end
   end

   class AppBox from Tk.frame
      feat appDB
      attr row:0
      meth tkInit(name:N parent:P ...)=M
         Tk.frame, tkInit(parent:P bd:0)
         TL  Row={self getRow($)}
         F1={New Tk.frame tkInit(parent:self bd:0)}
         fun{NF} {New Tk.frame tkInit(parent:F1 bd:1 relief:sunken height:2 width:10)} end
      in
         self.appDB={Dictionary.new}
         TL={New Tk.label tkInit(parent:F1 text:N font:FontSeparator)}
         {Tk.batch [grid({NF} row:0 column:0 sticky:we padx:2)
                    grid(TL row:0 column:1)
                    grid({NF} row:0 column:2 sticky:we padx:2)
                    grid(F1 row:Row column:0 sticky:we)
                    grid(columnconfigure F1 0 weight:1)
                    grid(columnconfigure F1 1 weight:0)
                    grid(columnconfigure F1 2 weight:1)
                    grid(columnconfigure self 0 weight:1)]}
      end
      meth getRow($) O N in
         O=row<-N
         N=O+1
      end
      meth add(id:ID name:T)
         E={New AppEntry tkInit(id:ID parent:self name:T)}
         R={self getRow($)}
      in
         if R==2 then {Tk.send grid(self column:0 row:4 sticky:news)} end %Apps
         {Dictionary.put self.appDB ID app(id:ID name:T widget:E)}
         {Tk.send grid(E row:R column:0 sticky:we)}
      end

      meth remove(id:ID) E={Dictionary.get self.appDB ID} in
         {Tk.send grid(forget(E.widget))}
         {Dictionary.remove self.appDB ID}
      end
   end

   proc{Kill}
      {Thread.terminate AnimateThread}
      {Client logout}
   end


   T Online Offline Others Apps StatusV

   proc{ErrorBox M}
      E=if {IsDet T} then {New TkTools.error tkInit(master:T text:M)}
        else {New TkTools.error tkInit(text:M)} end
   in
      {Tk.send bell} {Wait E.tkClosed}
   end

   proc{Shutdown} if {IsDet GUIisStarted} then {T tkClose} end end

 %   fun {GetAllMessages} E Tmp in
%       {Dictionary.items DB E}
%       Tmp = {Map E fun{$ X} O S in
%                     {X.widget getMessages(new: _ old: O sent: S)}
%                     messages(id : X.id old: O sent: S)
%                  end}
%       Tmp
%    end

   proc{RemoveFriend Friend}
      lock CLock then
         {Server S_removeFriend(id:ClientID friend:Friend.id)}
         {RemoveFriendEntry Friend.id}
      end
   end

   proc{RemoveFriendEntry Friend}
      try E={Dictionary.get DB Friend} in
         if E.online == true then
            {Online remove(id: E.id)}
         else
            {Offline remove(id: E.id)}
         end
      catch _ then skip end
   end

   proc {HaltApplication Id}
      {Client haltApplication(application: Id)}
      {Apps remove(id:Id)}
   end

   proc{ReceiveMessage M}
      lock CLock then
         if {Dictionary.member DB M.sender} then E={Dictionary.get DB M.sender} in
            {E.widget receiveMessage(message:M.message mid:M.mid sender:M.sender date:M.date
                                     reply_to:M.reply_to status:new)}
         else
            try
               Name={Server S_getUserName(id:M.sender name:$)}
               {Others add(id:M.sender name:Name)}
               E={Dictionary.get DB M.sender}
            in
               {E.widget receiveMessage(message:M.message mid:M.mid sender:M.sender date:M.date
                                        reply_to: M.reply_to status: new)}
            catch noSuchEntry(...) then skip end
         end
      end
   end

   proc{AddApp As} O N in
      {Exchange Applications O N}

      N={Append As O}
   end
   proc{RemoveApp Id} O N in
      {Exchange Applications O N}
      N={Filter O fun {$ X} X.id\=Id end}
   end
   proc{UpdateApp A} O N in
      {Exchange Applications O N}
      N={Map O fun {$ X} if X.id==A.id then A else X
                         end end}
   end

   proc{StartApp X} {Apps add(id:X.id name:X.name )} end

   proc{InviteClient SID N Desc IsOk}
      E={Dictionary.get DB SID}

      D={New TkTools.dialog
         tkInit(title:"Start Application"
                buttons: ['Okay'#proc {$} IsOk=true end
                          'More Information..'#proc{$}
                                                  {DisplayMess.display
                                                   "Information about "#N
                                                   Desc "Close Window"}
                                               end
                          'Cancel' # proc{$} IsOk=false end]
                default: 1)}
      L={New Tk.label tkInit(parent:D text:"Start Application: "#N#" from "#E.name#"?" )}
   in
      {Tk.send pack(L)}
      thread
         {Wait IsOk}
         {D tkClose}
      end
   end

   proc{InsertOldMessages}
      try
         {ForAll OldMessages proc{$ X}
                                if X.id \= nil then
                                   try E = {Dictionary.get DB X.id}.widget in
                                      {ForAll X.old proc{$ Y}
                                                       {E receiveMessage(status:old
                                                                         message:Y.message
                                                                         mid:Y.mid
                                                                         sender:Y.sender
                                                                         date:Y.date
                                                                         reply_to:Y.reply_to)}
                                                    end}
                                      {ForAll X.sent proc{$ Y}
                                                        {E saveSent(message:Y.message
                                                                    mid:Y.mid
                                                                    date:Y.date
                                                                    reply_to:Y.reply_to)}
                                                     end}

                                   catch _ then skip end % Removed Browsing. /Simon
                                end
                             end}
      catch X then {Browse insertOldMessages(X)} end
   end
   proc{Friends F}
      lock CLock then
         {ForAll F.online proc{$ X} {Online add(id:X.id name:X.name)}
                             if X.online==away then E={Dictionary.get DB X.id} in
                                {E.widget away()}
                             end
                          end}
         {ForAll F.offline proc{$ X} {Offline add(id:X.id name:X.name)} end}
      end
   end

   proc{UpdateUser M}
      %% How about messages here?!? /Nils 17/11-98
      lock CLock then
         E = {Dictionary.get DB M.id}
         N = changeStatus( id: M.id online: M.online )
      in
         if E.online==false then
            {Offline remove(id:E.id)}
            {Offline add(id:M.id name:M.name)}
         else
            {Online remove(id:E.id)}
            {Online add(id:E.id name:M.name)}
         end
         {ChangeStatus N}
      end
   end

   proc{ChangeStatus M}
      try
         lock CLock then E={Dictionary.get DB M.id} in
            if M.online==online then
               if E.online==false then
                  {Offline remove(id:M.id)}
                  {Online move(id:M.id entry:E)}
               end
               local E={Dictionary.get DB M.id} in
                  {E.widget available()}
               end
            elseif M.online==away then
               if E.online==false then
                  {Offline remove(id:M.id)}
                  {Online move(id:M.id entry:E)}
               end
               local E={Dictionary.get DB M.id} in
                  {E.widget away()}
               end
            else
               {Online remove(id:M.id)}
               {Offline move(id:M.id entry:E)}
               local E={Dictionary.get DB M.id} in
                  {E.widget available()}
               end
            end
         end
      catch X then {Browse X} {System.show clienterror(X)} end
   end

   proc{MessageAck M} E={Dictionary.get DB M.id} in
      {E.widget hasReadMail(mid:M.mid)}
   end

   proc{SetInfo I}
      {Assign MyData I}
      {Tk.send wm(title T I.firstname#" "#I.lastname)}
   end

   proc{Start C S CID M Settings}
      StatusL LogoL F StatusF LogoI
   in
      GUIisStarted=true
      Client=C
      Server=S
      ClientID=CID
      OldMessages=M

      if Settings\=nil then
         {Assign ClientGUISettings Settings}
         {FontLabel tk(config size:Settings.fontsize)}
         {Tk.send tk_setPalette(background(Settings.background) foreground(Settings.foreground))}
      end

      %% Start Graphics
      T={New Tk.toplevel tkInit(title:"Client" delete:Kill)}
      F={New Tk.frame tkInit(parent:T relief:sunken bd:2)}
      Online={New UserBox tkInit(parent:F name:"online" type:online)}
      Offline={New UserBox tkInit(parent:F name:"offline" type:offline)}
      Others={New OthersBox tkInit(parent:F name:"others" type:others)}
      Apps={New AppBox tkInit(parent:F name:"apps")}

      StatusV={New Tk.variable tkInit('Online')}

      StatusF={New Tk.frame tkInit(parent:T bd:0)}
      StatusL={New Tk.label tkInit(parent:StatusF relief:sunken bd:2
                                   textvariable:StatusV font:FontSystem)}
      LogoI={New Tk.image tkInit(type:photo format:gif url:PicturesURL#'logo.gif')}
      LogoL={New Tk.label tkInit(parent:StatusF image:LogoI)}

      {Tk.batch [grid(LogoL column:0 row:1 sticky:we padx:2)
                 grid(StatusL column:1 row:1 sticky:we)
                 grid(columnconfigure StatusF 1 weight:1)]}

      {Tk.batch [grid(Online    column:0 row:1 sticky:news)
                 grid(Offline   column:0 row:2 sticky:news)
%                grid(Others    column:0 row:3 sticky:news) % Added in OthersBox
%                grid(Apps      column:0 row:4 sticky:news) % Added in AppBox
                 grid(F         column:0 row:5 sticky:news ipady:1 ipadx:1 padx:2 pady:2)
                 grid(StatusF   column:0 row:7 sticky:news pady:1 padx:2)
                 grid(columnconfigure T 0 weight:1)
                 grid(columnconfigure F 0 weight:1)
                 wm(resizable T 1 0)]}

      thread
         AnimateThread={Thread.this}
         {Thread.setThisPriority low}
         {Animate}
      end

      %% Set bindings
      local
         A=proc{$}
              Aps={Filter {Access Applications} fun {$ X} X.author==ClientID end}
              Aps1={Map Aps fun{$ X} (X.name#" ("#X.id#")")#proc{$} {Client removeApplication(aid:X.id)} end end}
              Aps2={Map Aps fun{$ X} (X.name#" ("#X.id#")")#proc{$} {EditApp X.id Server} end end}
           in
              {Popup ["Add Friends"#proc{$}
                                       {AddFriends addfriends(server:Server
                                                              id:ClientID
                                                              friends:{Dictionary.keys DB})}
                                    end
                      separator
                      "Edit Personal Information"#proc{$}
                                                     {EditAccount.start {Access MyData} Server Client}
                                                  end
                      "Edit Client Settings"#proc{$} Change in
                                                {ConfigClient.start ClientGUISettings Change}
                                                if Change==true then {UpdateUISettings} end
                                             end
                      separator
                      "Applications"#["Start Application"#{Map {Access Applications}
                                                           fun{$ X} (X.name#" ("#X.id#")")#
                                                              proc{$} {Client startapplication(id:X.id)} end end}
                                      separator
                                      "Add Application"#proc {$} {AddApplicationGUI.start ClientID Server} end
                                      if Aps1==nil then ignore else "Remove Application"#Aps1 end
                                      if Aps2==nil then ignore else "Edit Application"#Aps2 end
                                     ]
                      separator
                      "Debugging"#["Start Distribution Panel"#proc{$} {DP.open} end
                                   "Start Panel"#proc{$} {Panel.object open} end
                                   "Browse DB"#proc{$} {Browse {Dictionary.entries DB}} end
                                  ]
                      separator
                      "Help"#proc{$}
                                {DisplayMess.display "Help"
                                 "This help will improve someday:)!\n\n"#
                                 " 1. Click on 'Add friends!' in popupmenu\n"#
                                 "    * The information in 'Add friends' need only to be\n"#
                                 "      partially correct.\n"#
                                 " 2. Use <Button-3> to get a popup menu on your friends\n"#
                                 "\nEnjoy!\n\nSend feedback (and bug-reports) to nilsf@sics.se or simon@sics.se"
                                 "Close Help Window"}
                             end
                      separator
                      "Logout"#Kill
                     ] T}
           end
         B=proc{$}
              {Popup ["Online"#proc{$}
                                  {StatusV tkSet('Online')}
                                  {Server S_setStatus(id:ClientID online:online)}
                               end
                      "Away"#proc{$}
                                {StatusV tkSet('Away')}
                                {Server S_setStatus(id:ClientID online:away)}
                             end
                      "Offline/Disconnected"#proc{$}
                                                {StatusV tkSet('Offline')}
                                                {Server S_setStatus(id:ClientID online:offline)}
                                             end
                     ] T}
           end

      in
         {LogoL tkBind(event:'<1>' action:A)}
         {LogoL tkBind(event:'<3>' action:A)}
         {StatusL tkBind(event:'<1>' action:B)}
         {StatusL tkBind(event:'<3>' action:B)}
      end
   end
in
   skip
end
