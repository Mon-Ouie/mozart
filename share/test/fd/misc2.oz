functor

import

   FD

export
   Return
define

   MiscTest =
   fun {$ N T}
      L = {StringToAtom {VirtualString.toString N}}
   in
      L(equal(T 1) keys: [fd])
   end


   Return=
   fd([misc2([
              {MiscTest 1
               fun {$} X Y Z D in X = 3 Z = 1 {FD.decl D}
                  thread {FD.sumC [1 2 3] [X Y Z] '=:' D} end
                  Y = 2
                  cond D = 10 then 1 else 0 end
               end}

              {MiscTest 2
               fun {$} X Y Z D in X = 3 Z = 1 {FD.decl D}
                  thread {FD.sumC a(1 2 3) a(a: X b:Y z:Z) '=:' D} end
                  Y = 2
                  cond D = 10 then 1 else 0 end
               end}

              {MiscTest 3
               fun {$} X Y Z D in X = 3 Z = 1 {FD.decl D}
                  thread {FD.sumC a( c:3 a:1 b:2 ) a(a: X k:Y w:Z) '=:' D} end
                  Y = 2
                  cond D = 10 then 1 else 0 end
               end}

              {MiscTest 4
               fun {$} X Y Z D in {FD.decl D}
                  thread {FD.sumCN [1 2 3] [[X X] a(Y Y) b(x:Z y:Z)] '=:' D} end
                  Y = 2
                  Z = 4
                  X = 3
                  cond D = 65 then 1 else 0 end
               end}

              {MiscTest 5
               fun {$} X Y Z D in {FD.decl D}
                  thread {FD.sumCN [1 2 3] a([X X] a(Y Y) b(x:Z y:Z)) '=:' D} end
                  Y = 2
                  Z = 4
                  X = 3
                  cond D = 65 then 1 else 0 end
               end}

              {MiscTest 6
               fun {$} X Y Z D in {FD.decl D}
                  thread {FD.sumCN [1 2 3] b(x:[X X] y:a(Y Y) z:b(x:Z y:Z)) '=:' D} end
                  Y = 2
                  Z = 4
                  X = 3
                  cond D = 65 then 1 else 0 end
               end}
             ])
      ])

end
