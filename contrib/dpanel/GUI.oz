functor 
import
   Tk
   Widgets(cardFrame toplevel)
   Graph(graph:GraphClass)
   System
export
   Open
   ReOpen
   ssites:SSites
   sactivity:SActive
   srtt:SRTT
   
   osites:OSites
   oactive:OActive
   onumber:ONumber
   
   bsites:BSites
   bactive:BActive
   bnumber:BNumber

   nilist:NIList
   ninumber:NINumber
   nibyte:NIByte
define
   SSites SActive SRTT
   OSites OActive ONumber
   BSites BActive BNumber
   NIList NINumber NIByte

   class TitleGraph from Tk.frame
      feat graph 

      meth tkInit(parent:P title:T ...)=M G L Init in
	 Tk.frame, tkInit(parent:P)
	 Init={Record.adjoin {Record.subtract M title} init(parent:self)}
	 self.graph=G={New GraphClass Init}
	 L={New Tk.label tkInit(parent:self text:T)}
	 {Tk.batch [grid(L row:0 column:0 sticky:we)
		    grid(G row:1 column:0 sticky:news)]}
      end

      meth otherwise(X)
	 {self.graph X}
      end
   end
   
   class SiteList from Tk.frame
      prop
	 final locking
      feat
	 listbox
	 InternalHelpFun1
	 entryDict
	 entryTag
      attr
	 cy:1
	 width:18*8
	 height:17*8
	 action:proc{$ _} skip end
	 lineSize:14
	 nextFree:[0]

      meth tkInit(...)=M
	 LB SY
      in
	 Tk.frame, M
	 self.entryDict = {NewDictionary}
	 self.listbox=LB={New Tk.canvas tkInit(parent:self
					       bd:2
					       relief:sunken
					       bg:white
					       width:@width
					       height:@height)}
	 self.entryTag = {New Tk.canvasTag tkInit(parent:self.listbox)}
	 {self.listbox tkBind(event:'<1>' args: [ int(y)]
			      action: proc{$  CY}
					 Y={self.listbox tkReturnInt(canvasy CY $)}
					 Line
					 Found
				      in
					 Line = (Y -5)  div @lineSize
					 {System.show Line}
					 Found = {Filter {Dictionary.items self.entryDict}
							  fun{$ E}
							     E.line == Line
							  end}
					 {System.show Found}
					 case Found of [E] then
					    {self Action(E.key)}
					 else skip end 
				      end)}
	 
	 
	 SY={New Tk.scrollbar tkInit(parent:self
				     width:8)}
	 {Tk.addYScrollbar LB SY}
	 {Tk.batch [grid(LB row:0 column:0 sticky:news)
		    grid(SY row:0 column:1 sticky:ns)
		    grid(columnconfigure self 0 weight:1)
		    grid(rowconfigure self 0 weight:1)]}
      end
   
      meth getEntry($)
	 case @nextFree of
	    [A] then
	    nextFree <- [A+1]
	    A
	 elseof A|R then
	    nextFree <- R
	    A
	 end
      end
      
      meth putEntry(E)
	 nextFree <- E|@nextFree
      end
      
      meth setAction(P)
	 action<-P
      end

      meth Action(A)
	 {@action A}
      end
   
      meth addSite(Ks)
	 R = {Map Ks fun{$ K}
			Line = {self getEntry($)}
			S=site(text:K.text
			       key:K.key
			       fg:{CondSelect K fg black}
			       bg:{CondSelect K bg white}
			       line: Line 
			       fgtag:Line)
		     in
			self.entryDict.(K.key):=S
			S
		     end}
      in
	 if Ks \= nil then 
	    {self Draw(R)}
	 end
      end
      
      meth deleteSite(Ks)=M
	 {System.show M}
	 {ForAll Ks proc{$ K}
		       if {Dictionary.member self.entryDict K} then
			  E = self.entryDict.K in
			  {System.show member(K)#E}
			  {self.listbox tk(delete E.fgtag )}
			  {self putEntry(E.line)}
			  {Dictionary.remove self.entryDict K} 
		       else
			  {System.show notMember(K)}
		       end
		    end}
      end

      meth updateEntry(K T)
	 if {Dictionary.member self.entryDict K} then
	    E = self.entryDict.K in
	    {self.listbox tk(itemconfig E.fgtag text:T)}
	 end
      end
      
      meth Draw(Ss)
	 DC=self.listbox
	 Y1
      in
	 {ForAll Ss proc{$ X}
		       T = {New Tk.canvasTag tkInit(parent:DC)}
		    in
		       {DC tk(crea text 5 X.line * @lineSize + 5 
			      text:X.text anchor:nw fill:X.fg tags:T)}
		    end}
	 Y1 = (({List.sort  @nextFree Value.'>'}.1)  +1) *  @lineSize 
	 {self.listbox tk(configure scrollregion:q(0 0 1000  Y1 + 5 ))}
      end

      
      meth setColour(key:K bg:_ fg:FG) = M 
	 if {Dictionary.member self.entryDict  K} then 
	    S = self.entryDict.K in
	    {self.listbox tk(itemconfig S.fgtag fill:FG)}
	    self.entryDict.K:={Record.adjoinAt S fg FG}
	 end
      end
   end

   Toplevel
   proc{ReOpen}
      {Toplevel tkShow}
   end
	       
   proc{Open  RunSync}
      T=Toplevel={New Widgets.toplevel tkInit(title:"Distribution Panel"
						 delete:proc{$}
							   {T tkHide}
							   {Exchange RunSync unit _}
							end)}
	 CardF={New Widgets.cardFrame tkInit(parent:T padx:10 pady:10 width:900 height:190)}
	 SiteF OwnerF BorrowF NetInfoF
      in
	 %% Site frame
	 SiteF={New Tk.frame tkInit(parent:CardF)}
	 SSites={New SiteList tkInit(parent:SiteF)}
	 SActive={New TitleGraph tkInit(parent:SiteF
					title:"#Activity/s"
					miny:1.0
					maxy:11.0
					dim:''
					fill:false)}
	 SRTT={New TitleGraph tkInit(parent:SiteF
				     title:"Last RTT (ms)"
				     miny:1.0
				     maxy:11.0
				     dim:''
				     fill:false)}
   
	 %% Owner frame
	 OwnerF={New Tk.frame tkInit(parent:CardF)}
	 OSites={New SiteList tkInit(parent:OwnerF)}
	 OActive={New TitleGraph tkInit(parent:OwnerF
					title:"#Active/s"
					miny:1.0
					maxy:11.0
					dim:''
					fill:true)}
	 ONumber={New TitleGraph tkInit(parent:OwnerF
					title:"#Entities/s"
					miny:1.0
					maxy:11.0
					dim:''
					fill:true)}
   

	 %% Borrow frame
	 BorrowF={New Tk.frame tkInit(parent:CardF)}
	 BSites={New SiteList tkInit(parent:BorrowF)}
	 BActive={New TitleGraph tkInit(parent:BorrowF
					title:"#Active/s"
					miny:1.0
					maxy:11.0
					dim:''
					fill:true)}
	 BNumber={New TitleGraph tkInit(parent:BorrowF
					title:"#Entities/s"
					miny:1.0
					maxy:11.0
					dim:''
					fill:true)}
   
	 %% Net info frame
	 NetInfoF={New Tk.frame tkInit(parent:CardF)}
	 NIList={New SiteList tkInit(parent:NetInfoF)}
	 NINumber={New TitleGraph tkInit(parent:NetInfoF
					 title:"Number"
					 miny:1.0
					 maxy:11.0
					 dim:''
					 fill:false)}
	 NIByte={New TitleGraph tkInit(parent:NetInfoF
				       title:"Byte"
				       miny:1.0
				       maxy:11.0
				       dim:''
				       fill:true)}
   

	 {Tk.batch [grid(SSites	row:0 column:0 sticky:news)
		    grid(SActive	row:0 column:1 sticky:news)
		    grid(SRTT   	row:0 column:2 sticky:news) 

		    grid(OSites	row:0 column:0 sticky:news)
		    grid(OActive	 row:0 column:1 sticky:news)
		    grid(ONumber	row:0 column:2 sticky:news) 

		    grid(BSites	row:0 column:0 sticky:news)
		    grid(BActive	row:0 column:1 sticky:news)
		    grid(BNumber	row:0 column:2 sticky:news) 

		    grid(NIList	row:0 column:0 sticky:news)
		    grid(NINumber	row:0 column:1 sticky:news)
		    grid(NIByte	row:0 column:2 sticky:news) 
		 
		    grid(columnconfigure SiteF 0 weight:1)
		    grid(columnconfigure OwnerF 0 weight:1)
		    grid(columnconfigure BorrowF 0 weight:1)
		    grid(columnconfigure NetInfoF 0 weight:1)

		    grid(rowconfigure SiteF 0 weight:1)
		    grid(rowconfigure OwnerF 0 weight:1)
		    grid(rowconfigure BorrowF 0 weight:1)
		    grid(rowconfigure NetInfoF 0 weight:1)
		   ]}

	 {CardF addCard(id:1 title:" Site Info " frame:SiteF)}
	 {CardF addCard(id:2 title:" Owner Info " frame:OwnerF)}
	 {CardF addCard(id:3 title:" Borrow Info " frame:BorrowF)}
	 {CardF addCard(id:4 title:" Net Info " frame:NetInfoF)}
      
	 {Tk.batch [grid(columnconfigure T 0 weight:1)
		    grid(rowconfigure T 0 weight:1)
		    grid(CardF row:0 column:0 sticky:news)]}
   end
end




