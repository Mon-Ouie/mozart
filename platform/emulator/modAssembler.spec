###
### Authors:
###   Christian Schulte <schulte@ps.uni-sb.de>
###
### Copyright:
###   Christian Schulte, 2000
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

# -*-perl-*-

$module_init_fun_name = "disassembler_init";

%builtins_all =
    (
     'disassemble'	=> { in  => ['+procedure'],
			     out => ['+value'],
			     BI  => BIdisassemble },
     );
1;;
