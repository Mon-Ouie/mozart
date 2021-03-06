proc {Compiler.evalExpression VS Env ?Kill ?Result} E I S in
   E = {New Compiler.engine init()}
   I = {New Compiler.interface init(E)}
   {E enqueue(mergeEnv(Env))}
   {E enqueue(setSwitch(expression true))}
   {E enqueue(setSwitch(threadedqueries false))}
   {E enqueue(feedVirtualString(VS return(result: ?Result)))}
   thread T in
      T = {Thread.this}
      proc {Kill}
	 {E clearQueue()}
	 {E interrupt()}
	 try
	    {Thread.terminate T}
	    S = killed
	 catch _ then skip   % already dead
	 end
      end
      {I sync()}
      if {I hasErrors($)} then Ms in
	 {I getMessages(?Ms)}
	 S = error(compiler(evalExpression VS Ms))
      else
	 S = success
      end
   end
   case S of error(M) then
      {Exception.raiseError M}
   [] success then skip
   [] killed then skip
   end
end
