functor
import
   OS(system)
   Admin(manager:Manager)
   URL(make)
   Directory(mkDir:MkDir)
export
   WgetPkg WgetDoc
define
   proc {WgetPkg PkgUrl PkgDir}
      if {Manager ignoreURL(PkgUrl $)} then
	 {Manager trace('Ignoring URL '#PkgUrl)}
      else
	 {MkDir PkgDir}
	 Cmd = {Manager get_wget($)}
	 #if {Manager is_verbose($)} then ' -v' else ' -nv' end
	 #' -N -nH -nd -P "'#PkgDir#'" "'#PkgUrl#'"'
      in
	 {Manager trace(Cmd)}
	 try
	    if {OS.system Cmd}\=0 then raise oops end end
	 catch _ then {Raise mogul(wget_pkg(PkgUrl))} end
      end
   end
   proc {WgetDoc DocUrl DocDir}
      if {Manager ignoreURL(DocUrl $)} then
	 {Manager trace('Ignoring URL '#DocUrl)}
      else
	 {MkDir DocDir}
	 Cuts = {Length {URL.make DocUrl}.path} - 1
	 Cmd  = {Manager get_wget($)}
	 #if {Manager is_verbose($)} then ' -v' else ' -nv' end
	 #' -N -nH --cut-dirs='#Cuts#' -r -p -k --no-parent -Q 1m -P "'
	 #DocDir#'" "'#DocUrl#'"'
      in
	 {Manager trace(Cmd)}
	 try
	    if {OS.system Cmd}\=0 then raise oops end end
	 catch _ then {Raise mogul(wget_doc(DocUrl))} end
      end
   end
end
