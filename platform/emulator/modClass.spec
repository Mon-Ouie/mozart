###
### Authors:
###   Denys Duchier <duchier@ps.uni-sb.de>
###   Michael Mehl <mehl@dfki.de>
###   Christian Schulte <schulte@ps.uni-sb.de>
###
### Copyright:
###   Denys Duchier, 1998
###   Michael Mehl, 1998
###   Christian Schulte, 1998
###
### Last change:
###   $Date$ by $Author$
###   $Revision$
###
### This file is part of Mozart, an implementation 
### of Oz 3:
###    http://mozart.ps.uni-sb.de
###
### See the file "LICENSE" or
###    http://mozart.ps.uni-sb.de/LICENSE.html
### for information on usage and redistribution 
### of this file, and for a DISCLAIMER OF ALL 
### WARRANTIES.
###

# -*-perl-*-

%builtins_all =
    (
     'new'	 => { in  => ['record','+bool','+bool'],
		      out => ['+class'],
		      bi  => BInewClass},
     'is'	 => { in  => ['+value'],
		      out => ['+bool'],
		      bi  => BIclassIs},
     'isSited'	 => { in  => ['+class'],
		      out => ['+bool'],
		      bi  => BIclassIsSited},
     'isLocking' => { in  => ['+class'],
		      out => ['+bool'],
		      bi  => BIclassIsLocking},
     );
1;;
