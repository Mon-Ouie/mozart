###
### Authors:
###   Per Brand <perbrand@sics.se>
###
### Copyright:
###   Per Brand, 1998
###
### Last change:
###   $Date$ by $Author$
###   $Revision$
###
### This file is part of Mozart, an implementation 
### of Oz 3:
###    http://www.mozart-oz.org
###
### See the file "LICENSE" or
###    http://www.mozart-oz.org/LICENSE.html
### for information on usage and redistribution 
### of this file, and for a DISCLAIMER OF ALL 
### WARRANTIES.
###

%builtins_all =
(
    'interDistHandlerInstall'=>  { in  => ['value','value'],
			     out => ['+bool'],
			     BI  => BIinterDistHandlerInstall},

    'interDistHandlerDeInstall'=>{ in  => ['value','value'],
			     out => ['+bool'],
			     BI  => BIinterDistHandlerDeInstall},

 );


