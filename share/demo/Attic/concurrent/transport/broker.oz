%%%
%%% Authors:
%%%   Christian Schulte (schulte@dfki.de)
%%%
%%% Copyright:
%%%   Christian Schulte, 1998
%%%
%%% Last change:
%%%   $Date$ by $Author$
%%%   $Revision$
%%%
%%% This file is part of Mozart, an implementation
%%% of Oz 3
%%%    $MOZARTURL$
%%%
%%% See the file "LICENSE" or
%%%    $LICENSEURL$
%%% for information on usage and redistribution
%%% of this file, and for a DISCLAIMER OF ALL
%%% WARRANTIES.
%%%

class Broker from Contract History
   prop final
   feat toplevel map

   meth init(toplevel:T)
      Contract, init
      History, init(master:T suffix:'Broker')
      self.toplevel = T
   end

   meth announce(...) = M
      A R
   in
      Contract, {Adjoin M announce(answer:?A reply:R)}
      case A==reject then skip else R=grant end
      History, M
   end

   meth add(company:C driver:D<=unit ...) = M
      case D==unit then
         Contract,add(C {NewAgent Company init(name:C toplevel:self.toplevel)})
      else
         {Contract,get(C $) add(driver:D city:M.city)}
      end
   end

   meth remove(company:C driver:D<=unit)
      case D==unit then Contract,remove(C) else
         {Contract,get(C $) remove(D)}
      end
   end

   meth close
      History,  tkClose
      Contract, close
   end

end
