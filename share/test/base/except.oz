functor $ prop once

import
   System
   Open

export
   Return

body
   % tests may be performed (1) either at top-level or in a space
   % and (2) either width debugging on or off.
   local
      proc {DebugGet B} {System.property.get 'errors.debug' B} end
      proc {DebugSet B} {System.property.put 'errors.debug' B} end
      % perform the test in the appropriate context
      fun {Test Fun Debug TopLevel}
         D = {DebugGet}
      in
         try
            {DebugSet Debug}
            case TopLevel then {Fun Debug true}
            elsecase {Space.askVerbose
                      {Space.new fun {$} {Fun Debug false} end}}
            of succeeded(entailed) then true else false end
         finally
            {DebugSet D}
         end
      end
   in
      % L is a list of pairs of booleans DEBUG#TOPLEVEL, each
      % describing a context in which the test is to be performed
      % TEST returns true iff the test succeeds in all the
      % given contexts
      fun {TEST F L}
         case L of nil then true
         elseof (D#T)|L then
            {Test F D T} andthen {TEST F L}
         end
      end
   end
   % the various lists of contexts used in this test suite
   `?debug ?toplevel` = [true#true false#true false#true false#false]
   `-debug +toplevel` = [false#true]
   `+debug ?toplevel` = [true#true true#false]
   `-debug ?toplevel` = [false#true false#false]

   % NOTATION: each test is preceded by a comment that indicates
   % in what contexts the test should be performed. ?debug means
   % that both debug on and debug off should be tried. +debug
   % means only debug on. -debug means only debug off.

   % TELL       failure 2=3     (?debug ?toplevel)
   fun {TELL Debug Toplevel}
      proc {Fail X} 2=X end
   in
      try {Fail 3} false
      catch E then
         case Debug then
            case E of failure(debug:d(info:_ loc:_ stack:_))
            then true else false end
         else
            case E of failure(debug:unit)
            then true else false end
         end
      end
   end

   % DOT1       a(1).b==unit    (?debug ?toplevel)
   fun {DOT1 Debug TopLevel}
      try {Id a(1)}.b==unit catch F then
         case F of error(_ debug:d(info:_ loc:_ stack:_))
         then true else false end
      end
   end
   fun {Id X} X end

   % OPEN       system Open non existent file   (?debug ?toplevel)
   fun {OPEN Debug TopLevel}
      try {New Open.file init(name:"NoSuchFile") _} false
      catch F then
         case TopLevel then
            case Debug then
               case F of system(_ debug:d(info:_ loc:_ stack:_))
               then true else false end
            else
               case F of system(_ debug:unit)
               then true else false end
            end
         else
            case F of error(kernel(globalState io)
                            debug:d(info:_ loc:_ stack:_))
            then true else false end
         end
      end
   end

   % LOCK       raising exception out LOCK/END  (-debug +toplevel)
   fun {LOCK Debug TopLevel}
      S={Space.new
         proc {$ Root}
            Zlock={NewLock}
            T1 T2 T3 Synch
         in
            Root=(T3#Synch)
            thread
               try
                  lock Zlock then
                     T1=a {Wait Synch} raise t end
                  end
               catch t then skip end
            end
            thread T2=b lock Zlock then T3=c end end
         end}
   in
      case (thread {Space.askVerbose S} end)
      of succeeded(suspended) then
         {Space.inject S
          proc {$ _#Synch} Synch=unit end}
         case (thread {Space.askVerbose S} end)
         of succeeded(entailed) then
            case {Space.merge S} of
               c#unit then true
            else false end
         else false end
      else false end
   end

   % WITH1:     RAISE/WITH/END  (?debug ?toplevel)
   fun {WITH1 Debug TopLevel}
      try (raise t with foo end)
      catch F then
         case F of t then true else false end
      end
   end

   % WITH2:     RAISE/WITH/END  (?debug ?toplevel)
   fun {WITH2 Debug TopLevel}
      try (raise t(debug:d) with foo end)
      catch F then
         case Debug then
            case F of t(debug:d(info:foo loc:_ stack:_))
            then true else false end
         else
            case F of t(debug:d)
            then true else false end
         end
      end
   end

   % DOT2:      type error for '.'      (+debug ?toplevel)
   fun {DOT2 Debug TopLevel}
      try a.{Id a(b)}
      catch F then
         case F of error(kernel(type _ _ _ _ _) debug:_)
         then true end
      end
   end

   % Finally:   finally         (-debug ?toplevel)
   fun {FINALLY Debug TopLevel}
      fun {Boom} raise t end end
      proc {Set X} X=a end
      X
   in
      try
         try {Boom} finally {Set X} end
      catch t then
         {IsDet X} andthen X==a
      end
   end

   Return=except({Map
           [
            tell(     fun{$}{TEST TELL    `?debug ?toplevel`}end)
            dot1(     fun{$}{TEST DOT1    `?debug ?toplevel`}end)
            open(     fun{$}{TEST OPEN    `?debug ?toplevel`}end)
            'lock'(   fun{$}{TEST LOCK    `-debug +toplevel`}end)
            with1(    fun{$}{TEST WITH1   `?debug ?toplevel`}end)
            with2(    fun{$}{TEST WITH2   `?debug ?toplevel`}end)
            dot2(     fun{$}{TEST DOT2    `+debug ?toplevel`}end)
            'finally'(fun{$}{TEST FINALLY `-debug ?toplevel`}end)
           ]
           fun {$ T}
              L={Label T}
           in
              L(equal(T.1 true) keys:[except])
           end}
         )
end
