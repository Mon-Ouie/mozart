functor
import
   Regex at 'x-oz://contrib/regex'
   Text(
      split1	: Split1
      split	: Split
      strip	: Strip
      ) at 'x-ozlib://duchier/lib/String.ozf'
   Open
   URL(toAtom resolve make toBase)
export
   'class'	: Message
   ParseInternal Parse ParseNodup
   BurstInternal Burst BurstNodup
   Slurp ParseOptions
define
   class Message
      attr head body:unit
      meth init
	 head <- {NewDictionary}
      end
      meth putBody(V) body<-V end
      meth getBody($) @body end
      meth member(Key $)
	 {Dictionary.member @head Key}
      end
      meth check_keys(Keys)
	 for K in Keys do
	    if {Not {self member(K $)}} then
	       {Exception.raiseError ozpm(missing_key(K))}
	    end
	 end
      end
      meth entries($) {Dictionary.entries @head} end
      meth keys(   $) {Dictionary.keys    @head} end
      meth items(  $) {Dictionary.items   @head} end
      meth put(K V) {Dictionary.put @head K V} end
      meth get(K V) {Dictionary.get @head K V} end
      meth condGet(K D $) {Dictionary.condGet @head K D} end
      meth condGet1(K D $)
	 case {self condGet(K [D] $)} of H|_ then H end
      end
      meth get1(K $)
	 case {Dictionary.get @head K} of H|_ then H end
      end
      meth push(K V nodup:Nodup<=false)
	 if {self member(K $)} then
	    if Nodup then
	       {Raise mogul(duplicate_key(K))}
	    end
	    {self put(K {Append {self get(K $)} [V]})}
	 else
	    {self put(K [V])}
	 end
      end
      %% getSplit(K $) is useful whem K may be repeated and each
      %% time contains a list of words.  this returns the total
      %% list of words.
      meth getSplit(K $)
	 {FoldR {self condGet(K nil $)}
	  fun {$ V Accu}
	     {Append {Map {Split RE_WORD_SEPARATOR V} Strip} Accu}
	  end nil}
      end
   end
   %%
   %% parse email-like data into a Message object
   %%
   local
      RE_HEADER = {Regex.compile '([^:\n]+):(([^\n]|\n[ \t])*(\n|$))' [extended]}
   in
      fun {ParseInternal Data Nodup}
	 Msg = {New Message init}
	 Head =
	 case {Split1 Data '\r?\n\r?\n'}
	 of [H  ] then {Strip H}
	 [] [H B] then
	    {Msg putBody({Strip B})}
	    {Strip H}
	 end
      in
	 for M in {Regex.allMatches RE_HEADER Head} do
	    K = {Strip {Regex.group 1 M Head}}
	    V = {Strip {Regex.group 2 M Head}}
	 in
	    {Msg push({VirtualString.toAtom K} V nodup:Nodup)}
	 end
	 Msg
      end
   end
   fun {Parse      Data} {ParseInternal Data false} end
   fun {ParseNodup Data} {ParseInternal Data true } end
   %%
   %% burst a digest of email-like data separated by SEP and
   %% return a list of Messages
   %%
   fun {BurstInternal Data Sep Nodup}
      Res = {NewCell nil}
   in
      for D in {Split {Strip Data} Sep} do
	 D2 = {Strip D}
      in
	 if {ByteString.width D2}\=0 then L in
	    {Exchange Res L {ParseInternal D2 Nodup}|L}
	 end
      end
      {Reverse {Access Res}}
   end
   fun {Burst      Data Sep} {BurstInternal Data Sep false} end
   fun {BurstNodup Data Sep} {BurstInternal Data Sep true } end
   %%
   %% Read all the data from a url
   %%
   fun {Slurp U}
      FD = {New Open.file init(url:U)}
   in
      try {FD read(list:$ size:all)}
      catch _ then {Raise mogul(slurp(U))} unit
      finally {FD close} end
   end
   %%
   RE_WORD_SEPARATOR = {Regex.make '[[:space:],;]+'}
   %%
   RE_OPTIONS_VALUE = {Regex.make '^(\\[[^]]*\\][[:space:]]*)((.|\n)*)$'}
   RE_OPTIONS       = {Regex.make '\\[([^]]*)\\]'}
   RE_OPTION        = {Regex.make '([^,]+),?'}
   RE_OPTION_EQUAL  = {Regex.make '^([^=]+)=(.*)$'}
   fun {ParseOptions S}
      M = {Regex.search RE_OPTIONS_VALUE S}
   in
      if M==false then o(options:nil value:S)
      else
	 Options = {Strip {Regex.group 1 M S}}
	 Value   = {Strip {Regex.group 2 M S}}
      in
	 o(options:
	      {Regex.foldR RE_OPTIONS Options
	       fun {$ M L}
		  Opts = {Strip {Regex.group 1 M Options}}
	       in
		  {Regex.foldR RE_OPTION Opts
		   fun {$ M L}
		      O = {Strip {Regex.group 1 M Opts}}
		      M2 = {Regex.search RE_OPTION_EQUAL O}
		   in
		      if M2==false then O else
			 {Strip {Regex.group 1 M2 O}}#
			 {Strip {Regex.group 2 M2 O}}
		      end
		      |L
		   end L}
	       end nil}
	   value: Value)
      end
   end
end
