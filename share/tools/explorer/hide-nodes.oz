%%%  Programming Systems Lab, DFKI Saarbruecken,
%%%  Stuhlsatzenhausweg 3, D-66123 Saarbruecken, Phone (+49) 681 302-5312
%%%  Author: Christian Schulte
%%%  Email: schulte@dfki.uni-sb.de
%%%  Last modified: $Date$ by $Author$
%%%  Version: $Revision$

local

   HideTree = {NewName}

   fun {SkipFailed Ns}
      case Ns of nil then nil
      [] N|Nr then
         case N.kind of failed then {SkipFailed Nr}
         else N|{SkipFailed Nr}
         end
      end
   end

   fun {IsFailedHidable Ns}
      case Ns of nil then False
      [] N|Nr then
         (N.kind==choose andthen {N isFailedHidable($)})
         orelse {IsFailedHidable Nr}
      end
   end

   fun {IsUnhidable Ns}
      case Ns of nil then False
      [] N|Nr then
         (N.kind==choose andthen {N isUnhidable($)})
         orelse {IsUnhidable Nr}
      end
   end

   proc {HideTreeKids Ks}
      case Ks of nil then true
      [] K|Kr then {K HideTree} {HideTreeKids Kr}
      end
   end

   proc {HideUndrawn Ks}
      case Ks of nil then true
      [] K|Kr then {K hideUndrawn} {HideUndrawn Kr}
      end
   end

   class Inner
      attr
         isHidden: False

      meth dirtyUp
         case @isDirty then true else
            isDirty <- True
            {self.mom dirtyUp}
         end
      end

      meth hide
         <<deleteTree>>
         <<Inner dirtyUp>>
         case @isHidden then
            isDirty  <- True
            isDrawn  <- False
            isHidden <- False
         else
            isHidden <- True
            isDirty  <- True
            isDrawn  <- False
            {HideTreeKids @kids}
         end
      end

      meth !HideTree
         isDirty  <- True
         isDrawn  <- False
         shape    <- nil
         {HideTreeKids @kids}
      end

      meth UnhideTreeKids(Ks $)
         case Ks of nil then False
         [] K|Kr then
            case K.kind==choose andthen {K UnhideTree($)} then
               isDirty  <- True
               <<Inner UnhideTreeKids(Kr _)>>
               True
            else <<Inner UnhideTreeKids(Kr $)>>
            end
         end
      end

      meth UnhideTree($)
         case @isHidden then
            isHidden <- False
            isDrawn  <- False
            isDirty  <- True
            <<deleteTree>>
            <<Inner UnhideTreeKids(@kids _)>>
            True
         else <<Inner UnhideTreeKids(@kids $)>>
         end
      end

      meth unhideTree
         case <<Inner UnhideTree($)>> then {self.mom dirtyUp}
         else true
         end
      end

      meth HideFailedKids(Ks $)
        case Ks of nil then False
        [] K|Kr then
           case K.kind==choose andthen {K HideFailed($)} then
              isDirty <- True
              <<Inner HideFailedKids(Kr _)>>
              True
           else <<Inner HideFailedKids(Kr $)>>
           end
        end
      end

      meth HideFailed($)
         case @isHidden then False
         elsecase @choices>0 orelse @isSolBelow then
            <<Inner HideFailedKids(@kids $)>>
         else
            isHidden <- True
            <<deleteTree>>
            {HideTreeKids @kids}
            isDrawn <- False
            isDirty <- True
            True
         end
      end

      meth hideFailed
         case <<Inner HideFailed($)>> then {self.mom dirtyUp}
         else true
         end
      end

      meth UnhideButFailedKids(Ks $)
        case Ks of nil then False
        [] K|Kr then
           case K.kind==choose andthen {K UnhideButFailed($)} then
              isDirty <- True
              <<Inner UnhideButFailedKids(Kr _)>>
              True
           else <<Inner UnhideButFailedKids(Kr $)>>
           end
        end
      end

      meth UnhideButFailed(?IsDirty)
         case @choices>0 orelse @isSolBelow then
            case @isHidden then
               isHidden <- False
               isDrawn  <- False
               isDirty  <- True
               <<deleteTree>>
               IsDirty = True
               <<Inner UnhideButFailedKids(@kids _)>>
            else
               <<Inner UnhideButFailedKids(@kids IsDirty)>>
            end
         elsecase @isHidden then
            IsDirty=False
         else
            isHidden <- True
            <<deleteTree>>
            {HideTreeKids @kids}
            isDrawn <- False
            isDirty <- True
            IsDirty=True
         end
      end

      meth unhideButFailed
         case <<Inner UnhideButFailed($)>> then {self.mom dirtyUp}
         else true
         end
      end

      meth isFailedHidable($)
         NonFailedKids={SkipFailed @kids}
      in
         case @isHidden then False
         else @choices==0 andthen NonFailedKids==nil
            orelse {IsFailedHidable NonFailedKids}
         end
      end

      meth isUnhidable($)
         @isHidden orelse {IsUnhidable @kids}
      end

      meth isHidden($)
         @isHidden
      end

      meth hideUndrawn
         case @isDirty then
            case @isHidden then true else
               case @isDrawn then {HideUndrawn @kids}
               else isHidden <- True <<Inner HideTree>>
               end
            end
         else true
         end
      end

   end

   class Leaf
      meth isFailedHidable($)
         False
      end
      meth isUnhidable($)
         False
      end
      meth isHidden($)
         False
      end
      meth hide
         true
      end
      meth hideFailed
         true
      end
      meth !HideTree
         true
      end
      meth hideUndrawn
         true
      end
   end

   class Sentinel
      meth dirtyUp
         true
      end
   end

in

   HideNodes=c(choose:    Inner
               failed:    Leaf
               succeeded: Leaf
               blocked:   Leaf
               sentinel:  Sentinel)

end
