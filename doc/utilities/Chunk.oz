functor
import
   Narrator SGML ErrorListener
export
   GetChunk
define
   class OzDocToCode from Narrator.'class'
      attr Reporter Defs Refs
      meth init
	 Reporter <- Narrator.'class',init($)
	 {@Reporter setLogPhases(true)}
      end
      meth getChunk(File Title Chunk)
	 {@Reporter startBatch()}
	 {@Reporter startPhase('parsing SGML input')}
	 Node = {SGML.parse File @Reporter}
      in
	 if {@Reporter hasSeenError($)} then skip else
	    Defs <- {Dictionary.new}
	    Refs <- {Dictionary.new}
	    {@Reporter startPhase('processing chunks')}
	    OzDocToCode,Process(Node)
	    %% instantiate all refs
	    {@Reporter startPhase('resolving chunk references')}
	    {ForAll {Dictionary.entries @Refs}
	     proc {$ Key#Code}
		{Dictionary.get @Defs Key Code}
	     end}
	    {@Reporter startPhase('looking up target chunk')}
	    OzDocToCode,get(Title Chunk)
	    if {@Reporter hasSeenError($)} then
	       {@Reporter endBatch(rejected)}
	    else
	       {@Reporter endBatch(accepted)}
	    end
	    {@Reporter tell(done())}
	 end
      end
      meth get(Title $)
	 {Dictionary.get @Defs {VirtualString.toAtom Title}}
      end
      %%
      %% obtain a (uninstantiated) reference to a code chunk
      %%
      meth Get(Key $)
	 if {Dictionary.member @Refs Key} then
	    {Dictionary.get @Refs Key}
	 else X in
	    {Dictionary.put @Refs Key X}
	    X
	 end
      end
      %%
      %% extract textual data from tree
      %%
      meth Content(Node $)
	 case Node of _|_ then Node
	 elseof nil then nil
	 elseof pi(S) then '\&'#S#';'
	 else OzDocToCode,BatchContent(Node 1 $) end
      end
      meth BatchContent(Node I $)
	 if {HasFeature Node I} then
	    OzDocToCode,Content(Node.I $) #
	    OzDocToCode,BatchContent(Node I+1 $)
	 else nil end
      end
      %%
      %% process all chunks
      %%
      meth Process(Node)
	 case Node of _|_ then skip
	 elseof pi(_) then skip
	 elsecase {Label Node} of 'chunk' then
	    OzDocToCode,ProcessChunk(Node)
	 else
	    OzDocToCode,BatchProcess(Node 1)
	 end
      end
      meth BatchProcess(Node I)
	 if {HasFeature Node I} then
	    OzDocToCode,Process(Node.I)
	    OzDocToCode,BatchProcess(Node I+1)
	 end
      end
      meth ProcessChunk(Node)
	 Title = OzDocToCode,Content(Node.1 $)
	 Code  = OzDocToCode,BatchCode(Node.2 1 $)
	 Key   = {VirtualString.toAtom Title}
      in
	 {Dictionary.put @Defs Key
	  {Dictionary.condGet @Defs Key nil}#Code}
      end
      meth BatchCode(Node I $)
	 if {HasFeature Node I} then M=Node.I in
	    if {Label M}=='chunk.ref' then
	       Title = OzDocToCode,Content(M $)
	       Key   = {VirtualString.toAtom Title}
	    in
	       chunk(OzDocToCode,Get(Key $))
	    else
	       case M of _|_ then M
	       elsecase M of nil then nil
	       end
	    end
	    # OzDocToCode,BatchCode(Node I+1 $)
	 else nil end
      end
   end
   %%
   class MyListener from ErrorListener.'class'
      attr Sync: unit
      meth init(O X)
	 Sync <- X
	 ErrorListener.'class', init(O ServeOne true)
      end
      meth ServeOne(M)
	 case M of done() then @Sync = unit
	 else skip
	 end
      end
   end
   %%
   proc {GetChunk File Title IndentedCode}
      O = {New OzDocToCode init}
      Sync
      L = {New MyListener init(O Sync)}
      Code
   in
      {O getChunk(File Title Code)}
      {Wait Sync}
      if {L hasErrors($)} then raise error end end
      {New Indentor init(Code IndentedCode) _}
   end
   %%
   class Indentor
      attr head tail column margin flushed
      meth init(Code Result)
	 head<-Result tail<-Result
	 column<-0
	 margin<-0
	 flushed<-false
	 Indentor,entercode(Code)
	 @tail=nil
      end
      meth PUTC(C) L in
	 @tail=C|L
	 tail<-L
      end
      meth PUTMARGIN(N)
	 if N==0 then skip else
	    Indentor,PUTC(& )
	    Indentor,PUTMARGIN(N-1)
	 end
      end
      meth PUTTAB
	 Indentor,putc(& )
	 if @column mod 8 \= 0 then
	    Indentor,PUTTAB
	 end
      end
      meth putc(C)
	 if C==&\n then
	    column<-@margin flushed<-false
	    Indentor,PUTC(C)
	 else
	    if @flushed then skip
	    else
	       Indentor,PUTMARGIN(@margin)
	       flushed<-true
	    end
	    if C==&\t then
	       Indentor,PUTTAB
	    else
	       column<-@column+1
	       Indentor,PUTC(C)
	    end
	 end
      end
      meth entercode(Code)
	 case Code of X#Y then
	    Indentor,entercode(X)
	    Indentor,entercode(Y)
	 elseof chunk(Code) then
	    Margin = @margin
	 in
	    margin<-if @flushed then @column else @margin end
	    Indentor,entercode(Code)
	    margin<-Margin
	 elseof nil then skip
	 else
	    Indentor,enterstring(Code)
	 end
      end
      meth enterstring(Str)
	 case Str of H|T then
	    Indentor,putc(H)
	    Indentor,enterstring(T)
	 elseof nil then skip end
      end
   end
end
