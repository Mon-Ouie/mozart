###
### Authors:
###   Denys Duchier <duchier@ps.uni-sb.de>
###   Christian Schulte <schulte@ps.uni-sb.de>
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
###    http://www.mozart-oz.org
###
### See the file "LICENSE" or
###    http://www.mozart-oz.org/LICENSE.html
### for information on usage and redistribution
### of this file, and for a DISCLAIMER OF ALL
### WARRANTIES.
###

# -*-perl-*-

%builtins_all =
    (
     '.'                => { in  => ['*recordCOrChunk','+feature'],
                             out => ['value'],
                             bi  => BIdot},

     'dotAssign'        => { in  => ['*recordCOrChunk','+feature','value'],
                             out => [],
                             BI  => BIdotAssign},

     'wait'             => { in  => ['+value'],
                             out => [],
                             bi  => BIwait},

     'waitQuiet'        => { in  => ['+value'],
                             out => [],
                             bi  => BIwaitQuiet},

     'waitOr'           => { in  => ['value','value'],
                             out => [],
                             BI  => BIwaitOr},

     'isFree'           => { in  => ['value'],
                             out => ['+bool'],
                             bi  => BIisFree,
                             test => 0},

     'isKinded'         => { in  => ['value'],
                             out => ['+bool'],
                             bi  => BIisKinded,
                             test => 0},

     'isFuture'         => { in  => ['value'],
                             out => ['+bool'],
                             bi  => BIisFuture,
                             test => 0},

     'isDet'            => { in  => ['value'],
                             out => ['+bool'],
                             bi  => BIisDet,
                             test => 0},

     'max'              => { in  => ['+comparable','+comparable'],
                             out => ['+comparable'],
                             bi  => BImax},

     'min'              => { in  => ['+comparable','+comparable'],
                             out => ['+comparable'],
                             bi  => BImin},

     'hasFeature'       => { in  => ['*recordCOrChunk','+feature'],
                             out => ['+bool'],
                             bi  => BIhasFeature},

     'condSelect'       => { in  => ['*recordCOrChunk','+feature','value'],
                             out => ['value'],
                             bi  => BImatchDefault},

     'byNeed'           => { in  => ['value'],
                             out => ['value'],
                             BI  => BIbyNeed},

     'byNeedDot'        => { in  => ['value','+feature'],
                             out => ['value'],
                             BI  => BIbyNeedDot},

     'future'           => { in  => ['value'],
                             out => ['value'],
                             BI  => BIfuture},

     '!!'               => { in  => ['value'],
                             out => ['value'],
                             BI  => BIfuture},

     '=='               => { in  => ['*value','*value'],
                             out => ['+bool'],
                             bi  => BIeqB,
                             negated => '\\\\='},

     '\\\\='            => { in  => ['*value','*value'],
                             out => ['+bool'],
                             bi  => BIneqB,
                             negated => '=='},

     '<'                => { in  => ['+comparable','+comparable'],
                             out => ['+bool'],
                             bi  => BIlessFun,
                             negated => '>='},

     '=<'               => { in  => ['+comparable','+comparable'],
                             out => ['+bool'],
                             bi  => BIleFun,
                             negated => '>'},

     '>'                => { in  => ['+comparable','+comparable'],
                             out => ['+bool'],
                             bi  => BIgreatFun,
                             negated => '=<'},

     '>='               => { in  => ['+comparable','+comparable'],
                             out => ['+bool'],
                             bi  => BIgeFun,
                             negated => '<'},

     '='                => { in  => ['value','value'],
                             out => [],
                             BI  => BIunify},

     'status'           => { in  => ['value'],
                             out => ['+tuple'],
                             bi  => BIstatus},

     'type'             => { in  => ['+value'],
                             out => ['+atom'],
                             bi  => BItermType},

     'toVirtualString'  => { in  => ['value','+int','+int'],
                             out => ['+string'],
                             BI  => BItermToVS},

     'nameVariable'    => { in  => ['value','+atom'],
                            out => [],
                            BI  => BIvalueNameVariable},
     );
1;;
