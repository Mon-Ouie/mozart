functor
import
   Externalizable('class':XT)
   Admin(manager:Manager)
   GetSlot('class':GS)
   Wget(wgetPkg wgetDoc)
   URL(toString resolve toBase make)
   Entry('class':EntryClass)
   HTML_Package('class':HTMLClass)
   Regex(compile search group allMatches make) at 'x-oz://contrib/regex'
   Text(strip:Strip split:Split)
   MogulID(normalizeID:NormalizeID)
export
   'class' : Package
define
   %% !!! there should be a `validate' method to make sure that
   %% all slots have ok values
   class Package from GS XT EntryClass HTMLClass
      attr id pid url blurb provides requires content_type
	 url_pkg url_doc body author contact keywords
	 categories url_doc_extra
      meth init(Msg Id Url Pid Prev)
	 {Manager incTrace('--> init Package '#Id)}
	 try
	    EntryClass,init(Msg)
	    id           <- {NormalizeID Id Pid}
	    pid          <- Pid
	    url          <- Url
	    blurb        <- {Msg condGet1('blurb' unit $)}
	    provides     <- {Msg condGet('provides' nil $)}
	    requires     <- {Msg condGet('requires' nil $)}
	    url_pkg      <- {Msg condGet('url-pkg' nil $)}
	    url_doc      <- {Msg condGet('url-doc' nil $)}
	    author       <- {Msg condGet('author' nil $)}
	    contact      <- {Msg condGet('contact' nil $)}
	    keywords     <- {Append {Msg getSplit('keyword' $)}
			     {Msg getSplit('keywords' $)}}
	    categories   <- {self getCategories($)}
	    url_doc_extra<- {Msg condGet('url-doc-extra' nil $)}
	    %% !!! here we should copy the persistent info from Prev
	 finally
	    {Manager decTrace('<-- init Package '#Id)}
	 end
      end
      meth extern_label($) 'package' end
      meth extern_slots($)
	 [id pid url blurb provides requires content_type
	  url_pkg url_doc body author contact keywords
	  categories url_doc_extra]
      end
      meth printOut(Margin Out DB)
	 {Out write(vs:Margin#' '#@id#' (package)\n')}
      end
      meth updatePub(DB)
	 {Manager incTrace('--> updatePub package '#@id)}
	 try
	    for U in @url_pkg do {self UpdatePkg(U DB)} end
	    for U in @url_doc do {self UpdateDoc(U DB)} end
	    for U in @url_doc_extra do {self UpdateDocExtra(U DB)} end
	 finally
	    {Manager decTrace('<-- updatePub package '#@id)}
	 end
      end
      meth get_id_as_rel_path($)
	 {Manager id_to_relurl(@id $)}
      end
      meth UpdatePkg(U DB)
	 M = {Regex.search RE_PROVIDES U}
	 U2 = {Regex.group 2 M U}
      in
	 {Manager trace('Downloading pkg '#U2)}
	 try {Wget.wgetPkg U2
	      {URL.toString
	       {URL.resolve
		{URL.toBase {Manager get_pkgdir($)}}
		{self get_id_as_rel_path($)}}}}
	 catch mogul(...)=E then
	    {Manager addReport(update_pub_pkg(@id) E)}
	 end
      end
      meth UpdateDoc(U DB)
	 {Manager trace('Downloading doc '#U)}
	 try {Wget.wgetDoc U
	      {URL.toString
	       {URL.resolve
		{URL.toBase {Manager get_docdir($)}}
		{self get_id_as_rel_path($)}}}}
	 catch mogul(...)=E then
	    {Manager addReport(update_pub_doc(@id) E)}
	 end
      end
      meth UpdateDocExtra(U DB)
	 {Manager trace('Downloading doc extra '#U)}
	 DocDir = {URL.toString
		   {URL.resolve
		    {URL.toBase {Manager get_docdir($)}}
		    {self get_id_as_rel_path($)}}}
	 Dir Url
      in
	 case {Regex.search RE_DOC_EXTRA U}
	 of false then Dir=DocDir Url=U
	 [] M then Dir=DocDir#'/'#{Strip {Regex.group 1 M U}}
	    Url = {Strip {Regex.group 2 M U}}
	 end
	 try {Wget.wgetDoc Url Dir}
	 catch mogul(...)=E then
	    {Manager addReport(update_pub_doc_extra(@id) E)}
	 end
      end
      meth updateProvided(DB D)
	 {Manager incTrace('--> updateProvided package '#@id)}
	 try
	    for X in @provides do
	       S = {VirtualString.toAtom
		    case {Regex.search RE_PROVIDES X}
		    of false then X
		    [] M then {Strip {Regex.group 2 M X}} end}
	       L = {Dictionary.condGet D S nil}
	    in
	       if {Not {Member @id L}} then
		  {Dictionary.put D S @id|L}
	       end
	    end
	 finally
	    {Manager decTrace('<-- updateProvided package '#@id)}
	 end
      end
      meth getCategories($)
	 D = {NewDictionary}
      in
	 for X in @provides do
	    for M in {Regex.allMatches RE_CATEGORY X} do
	       for S in {Split {Regex.group 1 M X} RE_WORD_SEP} do
		  A = {VirtualString.toAtom S}
	       in
		  if A\='' then D.A := true end
	       end
	    end
	 end
	 {Dictionary.keys D}
      end
      %%
      meth updatePkgList(DB L $) @id|L end
      %%
      %%
      meth updateAuthorList(DB L $) 
	 package(id:@id authors:@author)|L
      end
      %%
      meth hasCategory(C $)
	 {Member C @categories}
      end
   end
   %%
   RE_PROVIDES = {Regex.compile '^(\\[[^]]*\\][[:space:]]*)*(.*)$' [extended]}
   RE_CATEGORY = {Regex.make '^\\[([^]]*)\\]'}
   RE_WORD_SEP = {Regex.make '[[:space:],;]+'}
   RE_DOC_EXTRA= {Regex.make '^\\[([^]]+)\\][[:space:]]+(.+)$'}
end
