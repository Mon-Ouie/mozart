###
### Authors:
###   Denys Duchier <duchier@ps.uni-sb.de>
###   Christian Schulte <schulte@dfki.de>
###
### Copyright:
###   Denys Duchier, 1998
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
     'is'		=> { in  => ['+value'],
			     out => ['+bool'],
			     bi  => BIisNameB},

     'new'		=> { in  => [],
			     out => ['+name'],
			     BI  => BInewName},

     'newUnique'	=> { in  => ['+atom'],
			     out => ['+name'],
			     BI  => BInewUniqueName},
     );
1;;
